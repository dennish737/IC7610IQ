// BlockingRingBuffer.hpp

#ifndef BLOCKING_RING_BUFFER
#define BLOCKING_RING_BUFFER

#include "common.h"
// Ringbuffer for data
struct BlockingRingBuffer {
public:
	// The data comming from the IC7610  are interleaved I/Q short values, where I is real part,
	// and Q is the complex part. When reading data, from the IC7610, each sample is 4 bytes,
	// and we read the data in 'blocks', which are multiples of 512 bytes (128 complex values). 
	// A recomended size is 1024 complex short (IQ_BUFFER_SIZE) or  (4096 bytes) per block.
	// This buffer should represent the "native" format for the device (e.g. std::vector<complex<short>>).
	// The buffer should hold NUM_BLOCKS, so the capacity should be NUM_BLOCKS * IQ_BUFFER_SIZE
	//
	// Buffer capacty:
	//	Capacity (samples)	Size in Bytes	Num 4K Blocks
	//		128					512
	//		256					1024
	//		512					2048
	//		1024				4096			1
	//		2048				8192			2
	//		4096				16384			4
	//		8192				32768			8
	//		16384				65526			16
	//		32768				131062			32
	//
	// When we read from the device, we need to read in bytes, then copy the data into the buffer, 
	// converting the bytes to complex short.  This is easily done by a simple memcpy if the Endian
	// is the same, other wise we need to swap the bytes. Handeling endianness is TBD
			
	BlockingRingBuffer(): buff_size_(32768) {
		capacity_ = buff_size_ + 1;
		head_ = 0;
		tail_ = 0;
		buffer_.resize(buff_size_ + 1); 
		// Extra element to distinguish full/empty states
		//assert((capacity_ & (capacity_ - 1)) == 0); // Enforce power of two capacity for efficiency
	}

	// Bulk write operation: blocks if not enough space is available, this is data received from
	// the IC7610. 
	// data is always a pointer to an uin8_t byte array, and count = number of bytes to add:
	// not count must be ina integer multiple of BYTES_PER_SAMPLE )
	size_t writebuf(uint8_t *data, size_t count) {
		
		/*
		if (count % sizeof(std::complex<short>) != 0) {
			// Must have pairs of shorts (real/imaginary)
			std::string errorMessage = "Byte data size not a multiple of: " + 
					std::to_string(sizeof(std::complex<short>));
			throw std::runtime_error(errorMessage);	
		}
		*/
		
		size_t samplesToWrite = count/BYTES_PER_SAMPLE;
		{
			
			std::unique_lock<std::mutex> lock(mutex_);

			// Wait until enough space is available
			not_full_.wait(lock, [&] { return available_space() >= samplesToWrite; });
			
			// handle the wrap around issue 
			size_t current_tail = tail_;
			size_t space_to_end = (capacity_ - current_tail);
			//T* raw_data_ptr = buffer_->data();
			if (samplesToWrite <= space_to_end){
				// continous write
				// std::memcopy(void* dest, const void* src, size_t numbytes)
				
				std::memcpy(&*(buffer_.begin() + current_tail), data, count );
				tail_ = (current_tail + samplesToWrite ) % capacity_;
			} else {
				// Write wraps around
				size_t first_chunk_size = space_to_end;
				size_t second_chunk_size = samplesToWrite - first_chunk_size;
				std::memcpy(&*(buffer_.begin() + current_tail), data, first_chunk_size * BYTES_PER_SAMPLE);
				std::memcpy(&*(buffer_.begin()) , data + first_chunk_size, second_chunk_size * BYTES_PER_SAMPLE );
				tail_ = second_chunk_size;

			}
		}


		not_empty_.notify_one(); // Notify consumers that data is available
		return count;
	}

	// Bulk read operation: blocks if not enough data is available
	size_t readbuf(std::complex<short> *destination, size_t count) {
		// we want to be able to map the data into different types/
		// the data received is always complex short (32 bits, 4 bytes), per
		// sample. We can return the data as Interleaved I/Q (short) complex short, or
		// complex float or complex double.
		//std::cout << " readbuf called" << std::endl;	
		{
			std::unique_lock<std::mutex> lock(mutex_);
			//std::cout << "available data = " << available_data() << std::endl;
			
			// Wait until data is available (can modify to wait for 'count' items if needed, but this is simpler)
			not_empty_.wait(lock, [&] { return available_data() >= count; });
			//std::cout << " readbuf data available" << std::endl;
			//T* raw_data_ptr = buffer_->data();
			// we have enough data to complete: Note other option is to return data available

			size_t current_head = head_;
			size_t data_to_end = capacity_ - current_head;

			if (count <= data_to_end) {
				// Contiguous read
				//std::copy(buffer_.begin() + current_head, buffer_.begin() + current_head + count, destination);
				head_ = (current_head + count) % capacity_;
			} else {
				// Read wraps around
				size_t first_chunk_size = data_to_end;
				size_t second_chunk_size = count - first_chunk_size;

				std::copy(buffer_.begin() + current_head, buffer_.begin() + current_head + first_chunk_size,
							 destination);
				std::copy(buffer_.begin(), buffer_.begin() + second_chunk_size, (destination + first_chunk_size));
				head_ = second_chunk_size;
			}
		}
		
		not_full_.notify_one(); // Notify producers that space is available
		return count;
	}

	// Check approximate current size (for demonstration/monitoring)
	size_t size() const {
		//std::lock_guard<std::mutex> lock(mutex_);
		return available_data();
	}

	void clear_ringbuffer()
	{
		{
			std::unique_lock<std::mutex> lock(mutex_);
			head_ = 0;
			tail_ = 0;
			// memset( *ptr , int value, size_t num_bytes)
			memset(buffer_.data(), 0, capacity_* BYTES_PER_SAMPLE);
		}
		not_full_.notify_one();
	}



	size_t available_data() const {
		return (tail_ >= head_) ? (tail_ - head_) : (capacity_ - head_ + tail_);
	}

	size_t available_space() const {
		// One element is always unused to distinguish between full and empty
		return (capacity_  - available_data()- 1 ); // One slot is left unused to distinguish full/empty
	}
	
	bool isEmpty() const {
		return head_ == tail_;
	}

	bool isFull() const {
		return (tail_ + 1) % capacity_ == head_;
	}

private:
	size_t buff_size_;
	size_t capacity_;
	//std::unique_ptr<std::vector<>> buffer_;
	//std::vector<std::unique_ptr<T>[]> buffer_;
	std::vector<std::complex<short>> buffer_;
	size_t head_; // Producer index
	size_t tail_; // Consumer index

	mutable std::mutex mutex_;
	std::condition_variable not_empty_;
	std::condition_variable not_full_;
	std::atomic<bool> stop_thread{false};
};

#endif /*BlockingRingBuffer.hpp */
