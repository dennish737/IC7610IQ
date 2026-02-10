

#include <atomic>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

#define POINTER_SIZE 64

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
// One of the problems with ring buffer, is the reqirement to wrap around when we hit the in, and our devices
// do not know how to 'wrap', they only want to work with continous buffers. Also when wrapping, mutex are required to 
// protect the pointer calculatons in a SPSC model. To solve this, we can add estra 'copy' operation, by using a continous 
// temp buffer to get the data from the device, then copy that buffer into the ring buffer, then need another copy to 
// copy the data in the ring buffer to the'user' buffer, allowing us to handle the wrap around issue. another approach,
// in Linux, is to use a mmap to create a virtual buffer. For windows we would need to use VirtuaAlloc, and our space 
// would need to be multiples of the system  page size. This creates a compatibility problem, between Windows and Linux
// Another approch is is to use a bi-partite circular buffer (BipBuffer). You can find an explination of BipBuffer at
// https://ferrous-systems.com/blog/lock-free-ring-buffer/.
// The major advantages of using BipBuffer are:
//	1) We can use atomic variables instead of mutexes elemenating accidential deadlock conditions.
//	2) Code si compatable for Linux, Windows, and microcontrolers
//	3) We can eliminate the copy from the temp bufer to the ring buffer, by using reservations
//	4) We can also elminate the copy from the ring buffer to the user buffer.
//
// Usage:
// 		Producer: 
//			1) Reserves a block of data
//			2) passes the pointer to the block to the device, as a request for data of size bytes
//			3) Device files the buffer, and returns producer.
//			4) Producer commits the write, signaling data is available to read.
//			5) If there is insufficient space, reserve returns a 'null', signaling buffer full
//		Consummer:
//			1) Consumer request a read of n bytes, and receives a pointer for the read block, and size
//			2) when the data is consumed, the data is release by making a release call. freeingthe block for a write.
//			3)If there is no data in the buffer, the request returns a null signaling an empty buffer.
	
class SPSC_BipBuffer {
    uint8_t* buffer_;
    size_t capacity_;
    
    // Padded to prevent false sharing
    alignas(POINTER_SIZE) std::atomic<size_t> writePtr_{0};
    alignas(POINTER_SIZE) std::atomic<size_t> readPtr_{0};
    // ... Additional state for tracking 2 regions (Region A/B)
	alignas(POINTER_SIZE) std::atomic_size_t invalidPtr_; 	
	bool write_wrapped_; /**< Write wrapped flag, used only in the producer */
    bool read_wrapped_;  /**< Read wrapped flag, used only in the consumer */
    
SPSC_BipBuffer::CalcFree(const size_t windx, const size_t readindx)
{
	if (readindx > writeindx) {
		return (readindx - writeindx) - 1U;
	} else {
		return (capacity_ - (writeindx - readindx)) -1U;
	}
}
		
public:
    SPSC_BipBuffer(size_t capacity) : capacity_(capacity) {
		readPtr_ = 0U;
		writePtr_ = 0U;
		invalidPtr_ = 0U;
		read_wrapped_ = false;
        buffer_ = new uint8_t[capacity];
		//uiint8_t data_[capacity]
    }
	
	~SPSC_BipBuffer()
	{
	
	}
	
	size_t SPSC_BipBuffer::bufferSize()
	{
		return capacity_;
	}
	
	bool SPSC_BipBuffer::bufferIsEmpty()
	{
		const size_t r_indx = readPtr_.load(std::memory_order_relaxed);
		const size_t w_indx = writePtr_.load(std::memory_order_acquire);
		if (r_indx == w_indx) {
			return true;
		} else {
			return false;
		}
	}

    // Producer calls this
    uint8_t* Reserve(size_t size) {
        // Find largest available contiguous block, handle wrap-around

		const size_t w_indx = writePtr_.load(std::memory_order_relaxed);
		const size_t r_indx = readPtr_.load(std::memory_order_acquire);

		const size_t free = CalcFree(w_indx, r_indx);
		const size_t linear_space = capacity_ - r_indx;
		const size_t linear_free = std::min(free, linear_space);

		/* Try to find enough linear space until the end of the buffer */
		if (size <= linear_free) {
			return &buffer[w_indx];
		}

		/* If that doesn't work try from the beginning of the buffer */
		if (size <= free - linear_free) {
			write_wrapped_ = true;
			return &buffer[0];
		}

		/* Could not find free linear space with required size */
		return nullptr;
    }

    void Commit(size_t size) {
        //writePtr_.store(writePtr_.load() + size);
		size_t w_indx = writePtr_.load(std::memory_order_relaxed);
		size_t i_indx = invalidPtr_.load(std::memory_order_relaxed);

		/* If the write wrapped set the invalidate index and reset write index*/
		if (write_wrapped_) {
			write_wrapped_ = false;
			i_indx = w_windx;
			w_indx = 0U;
		}

		// Increment the write index
		w_indx += size;

		/* If we wrote over invalidated parts of the buffer move the invalidate
		 * index
		 */
		if (w_indx > i_indx) {
			i_indx = w_indx;
		}

		// Wrap to 0 if needed
		if (w_indx == size) {
			w_indx = 0U;
		}

		/* Store the indexes with adequate memory ordering */
		invalidPtr_.store(i_indx, std::memory_order_release);
		writePtr_.store(w, std::memory_order_release);
    }

    // Consumer calls this
    const std::pair<uint8_t*,size_t> GetReadPtr(size_t* size) {
        // Check if data is available
        // ...
        //return &buffer_[readPtr_.load()];
		/* Preload variables with adequate memory ordering */
		const size_t r_indx = readPtr_.load(std::memory_order_relaxed);
		const size_t w_indx = writePtr_.load(std::memory_order_acquire);
		const size_t i_indx = invalidPtr_.load(std::memory_order_acquire);

		/* When read and write indexes are equal, the buffer is empty */
		if (r_indx == w_indx) {
			return std::make_pair(nullptr, 0U);
		}

		/* Simplest case, read index is behind the write index */
		if (r_indx < w_indx) {
			return std::make_pair(&buffer_[r_indx], w_indx - r_indx);
		}

		/* Read index reached the invalidate index, make the read wrap */
		if (r_indx == i_indx) {
			read_wrapped_ = true;
			return std::make_pair(&buffer_[0], w_indx);
		}

		/* There is some data until the invalidate index */
		return std::make_pair(&buffer_[r_indx], i_indx - r_indx);
    }

    void Release(size_t size) {
        //readPtr_.store(readPtr_.load() + size);

		size_t r_indx = readPtr_.load(std::memory_order_relaxed);

		/* If the read wrapped, overflow the read index */
		if (read_wrapped_) {
			read_wrapped_ = false;
			r_indx = 0U;
		}

		/* Increment the read index and wrap to 0 if needed */
		r_indx += size;
		if (r_indx == capacity_) {
			r_indx = 0U;
		}

		/* Store the indexes with adequate memory ordering */
		readPtr_.store(r_indx, std::memory_order_release);
    }
};
