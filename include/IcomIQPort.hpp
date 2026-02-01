/******************************************
 *
 * IcomIQPort class include file
 * IcomIQPort.hpp
 *****************************************/
#ifndef ICOMIQPORT
#define ICOMIQPORT

//#include <SoapySDR/Logger.h>
#include <list>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdint>
#include <chrono>
#include <thread>
#include <sstream>
#include <complex>

#include <unistd.h> // For usleep

#include "ftd3xx.h"

/* 
 Communications to control Icom Equipment is done using the Communication Information version V (5),  CIV or CI-V protocol
 For radios that support IQ streaming (e.g. IC 7610), there are two CIV ports: a standard usb/serial port, and a high
 speed usb port for IQ streaming.

 When using the IQ stream, you must use the limited CIV command defined in the Icom "I/Q OUTPUT REFERENCE GUIDE", to 
 control the stream output. 
 
  To optimize data transfer, data is moved in a 32 bit (4 byte) at a time. Because of this, CIV command and responces 
  must be padded to 4 byte boundaries. 
  
  The IcomIQPort class provides a wrapper class, with methods to initialize, send/receive command/responces, and 
  receive data from the I/Q port. 
  
  The FT60x provides two interfaces (0 and 1) with interface 0 providing CIV control signals, and Interface 1 providing 
  high speed data transfers. The high speed channel configuration is set for the FT245(single-channel)mode.
  
  The Icom IC-7610's IP+ function is a setting that optimizes the Direct Sampling System's Analog-to-Digital Converter (
  ADC) for better Intermodulation Distortion (IMD) performance, especially with strong signals, by adding dither to 
  improve the IP3 (Third-order Intercept Point) without killing sensitivity. You turn it ON to prioritize IP quality 
  (better for contests) and OFF to prioritize sensitivity (general use), though many users find minimal difference, 
  with its main benefit being managing strong adjacent signals. 
  
  Note: channel configuration is typ
*/

#include <queue>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <chrono>
#include <thread>
#include <memory>
#include <iostream>

#include <complex>

#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <windows.h>
#include <unistd.h> // For usleep

#include "version.h"
#include "common.h"
#include "ftd3xx.h"

/*
//#define TIMEOUT 100 // ms
#define CMD_TIMEOUT 5000// ms
#define DATA_TIMEOUT 5000 // ms

#define DEFAULT_ASYNC_BUFFERS 0
// Each sample consist of a complex Short I and Q sample (2bytes + 2 bytes = 4 bytes)

#define CIV_ACK_NAK_OFFSET 4
#define IQ_ACK_NAK_MSG_LEN 8
*/

enum class IcomIQSources : uint8_t {
	iqDataOff = 0x00,
	iqDataMain = 0x01,
	idDataSub = 0x02
};

enum class IcomIQPreAmp : uint8_t {
	PAMPS_OFF = 0x00,
	PAMP1_ON = 0x01,
	PAMP2_ON = 0x02
};
	
class IcomIQPort {
private:
	FT_HANDLE handle;
	FT_DEVICE_LIST_INFO_NODE iqNode;
	size_t _civBufferSize = 256;
	bool _isInitialized;
	bool _isOpen;
	int  _cmdTimeout;
	int  _dataTimeout;
	bool _iqDataEnabled;
	bool _IPPlus;
	bool _DIGI_SEL;
	std::string version_ = VERSION_ICOMIQ_PORT;
	
	std::atomic<bool> _running;
	std::thread _read_thread;
	//std::mutex _data_mutex;
	//std::condition_variable _data_cv;
	//bool _data_ready;
	//std::queue<DataPacket> _data_queue;
	
	std::string _deviceSerialNum;

	size_t _buffer_size;
	uint16_t _num_buffers;
	
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

	BlockingRingBuffer iqBuffer_ ;
	void print_vector(std::vector<uint8_t> v);
	void CheckStatus(const FT_STATUS status, const char* functionName);
	void GetInitialRadioParams();
	void AsyncReadWorker();
public:
	IcomIQPort (void);
	void init (std::string deviceSerialNum); //find the Icom IQ port and initialize it
	static std::string getDeviceSerialNum();
	~IcomIQPort();
	void close();
	bool isInitialized() const {return _isInitialized;} // test to see if the port is initialized
	std::string version() { return version_; }
	bool isOpen(void) const {return _isOpen;} // test to see ifthe port is opened
	bool sendIQCommand( std::vector<uint8_t> cmd); //send a command or data to IC7610 I/Q port
	int  readIQReply( std::vector<uint8_t>& buffer); // read response or data from IC7610 I/Q port
	int  icomIQCommand( std::vector<uint8_t> cmd, std::vector<uint8_t> &reply); // sends a command and reads the reply
	void setTimeout(uint8_t channelID, int timeOut); // set command timeout
	int  getTimeout(uint8_t channelID) const {return ((channelID == CMD_IN || channelID == CMD_OUT)?_cmdTimeout : _dataTimeout);} // get current timout value
	bool enableIQData(uint8_t source); // enable the I/Q channel
	void disableIQData(void); // disable I/Q data
	bool isIQDataEnabled(void) const {return _iqDataEnabled;} // test if I/Q data is enabled
	int  readIQData( std::complex<short> *buffer, size_t buffer_size,void* overlapped_ = NULL); // read data into a buffer
	
	// convert an int to bcd
	int read_bcd(int n) { return (10 * ((n & 0xf0) >> 4) + (n & 0x0f));}
	// convert a uint32_ to bcd digit
    int bcd_digit(uint32_t n, int value){return (n / value) % 10;}
	// convert uint32_t to bcddigits
	int bcd_digits(uint32_t n, int value){return 0x10 * bcd_digit(n, 10 * value) | bcd_digit(n, value);}
	
	//size_t iqGetBufferSize() { return _buffer_size;};
	//void iqSetBufferSize(size_t size) {_buffer_size = size;};
	//size_t iqGetNumberOfBuffer() { return (size_t)_num_buffers; };
	//void iqSetNumberOfBuffers(size_t numBuffers) {_num_buffers = (uint16_t)numBuffers;};
	std::string iqGetChipConfiguration(); 
	std::string iqGetDevicveDescriptor();
	int iqGetRFGain(uint8_t vfo);
	int iqSetRFGain(uint8_t vfo, int rfgain);	//rfgain must be a value between 0 and 255
	int iqGetPreAmpStatus(uint8_t vfo);			// Returns 0 = off, 1 =PAMP1 On, 2 = PAMP2 On


	uint8_t iqGetAttenuatorSettings(uint8_t vfo);
	uint8_t iqSetAttenuatorSettings(uint8_t vfo, int attenuation);  //attenuation settings are in 3db increments to 45
	std::string iqGetAntenna(uint8_t vfo);
	int iqSetAntenna(uint8_t vfo, int antenna, bool status);
	
	
	int iqGetFrequency(uint8_t vfo);
	int iqSetFrequency(uint8_t vfo, uint32_t frequency);
	
	bool iqGetDIGI_SEL_Status(uint8_t vfo);
	void iqSetDIGI_SEL_Status(uint8_t vfo, bool status);
	//Intermodulation Performance Plus IP+
	bool iqGetIP_Status(uint8_t vfo);
	void iqSetIP_Status(uint8_t vfo, bool status);
	
	//std::vector<uint8_t> iqGetLatestData()
	int  iqAbortPipe(uint8_t pipe=IQ_IN);
	bool iqSetStreamPipe(uint8_t channelID, size_t streamBufferSize);
	bool iqClearStreamPipe(uint8_t channelID);
	int  iqInitializeOverlapped(void* overlapped_);
	int  iqReleaseOverlapped(void* overlapped_);
	bool iqAsyncStart(uint8_t vfo);
	void iqAsyncStop(void);
	int  iqReadBuf(std::complex<short> *data, size_t numSamples);
	size_t iqGetSizeOfAvailableData();
	void iqClearReadBuf( void) {iqBuffer_.clear_ringbuffer();};
	void iqAsyncReadWorker();
	//BlockingRingBuffer iqBuffer_ (16384);
			
};


#endif