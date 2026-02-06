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

#include "BlockingRingBuffer.hpp"

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