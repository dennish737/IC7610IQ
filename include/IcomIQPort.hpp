/** 
 * @file IcomIQPort.hpp
 * @bried A class to to provide control of the IC7610 through the IQ port
 * @details
 * 	The Icom IC7610 radio provides a high speed IQ out put using an FTDI FT60x device. FTDI also 
 * 	provides a d3xx ftd3xx library which allows for control ofthe radio and collection of data. 
 *  The FTDI library, is a 'raw' library, providing a basic interface to the radio sending 
 *  control messages and collecting data from the radio. Unfortunatly the user is left to 
 *  work out the control strings, and messaging handeling. 
 *  The IcomIQPort class, provides a set of methods for each of the allowed control functions
 *  and data collection modes.
 *  For more information on controling the IC7610 and collecting data, you should read the 
 *  FTDI Application Note AN_379 D3XX Programmmers Guide. in the Documents Directory.
 *  
 *	Communications to control Icom Equipment is done using the Communication Information version V (5),  CIV or CI-V protocol
 * 	For radios that support IQ streaming (e.g. IC 7610), there are two CIV ports: a standard usb/serial port, and a high
 *	peed usb port for IQ streaming.
 *
 *	hen controling the radio, you want to use the the standard serial port, and the IcomCIVPort library. When usingthe IQ data port,
 *	you will need to use the IcomIQPort library for sending and receiving data.
 *
 *	This file is for the IcomIQPort.
 *
 *	When using the IQ stream, you must use the limited CIV command defined in the Icom "I/Q OUTPUT REFERENCE GUIDE", to 
 *	control the stream output. 
 *
 *	To optimize data transfer, data is moved in a 32 bit (4 byte) at a time. Because of this, CIV command and responces 
 *	 must be padded to 4 byte boundaries. 
 * 
 *	The IcomIQPort class provides a wrapper class, with methods to initialize, send/receive command/responces, and 
 *	receive data from the I/Q port. 
 * 
 * The FT60x provides two interfaces (0 and 1) with interface 0 providing CIV control signals, and Interface 1 providing 
 * high speed data transfers. The high speed channel configuration is set for the FT245(single-channel)mode.
 * 
 * The Icom IC-7610's IP+ function is a setting that optimizes the Direct Sampling System's Analog-to-Digital Converter (
 * ADC) for better Intermodulation Distortion (IMD) performance, especially with strong signals, by adding dither to 
 * improve the IP3 (Third-order Intercept Point) without killing sensitivity. You turn it ON to prioritize IP quality 
 * (better for contests) and OFF to prioritize sensitivity (general use), though many users find minimal difference, 
 * with its main benefit being managing strong adjacent signals. 
 */

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



#include <queue>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <chrono>
#include <thread>
#include <memory>
#include <iostream>
#include <functional>

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


/**
 *  @bried enumerated class of Icom 7610 Sources (off, main and sub)
 */
enum class IcomIQSources : uint8_t {
	iqDataOff = 0x00,
	iqDataMain = 0x01,
	idDataSub = 0x02
};

/** 
 * @bried enumerated preamp identifiers (off, preamp 1 on, preamp 2 on)
 */
enum class IcomIQPreAmp : uint8_t {
	PAMPS_OFF = 0x00,
	PAMP1_ON = 0x01,
	PAMP2_ON = 0x02
};


// Forward Declartion for IcomIQPort
class IcomIQPort;

/*
 * @bried Structure used for Async/ Callback operation
 */
// Define a Custom Overlapped Structure used by IcomIQPort class for overlay
struct iqASYNC_CONTEXT {
    OVERLAPPED overlapped;    // Must be the first member
    IcomIQPort* contextPort;  // pointer to IcomIQPort class for device
    uint8_t* buffer;          // Data buffer for this request
    size_t bufferSize;        // Size of the buffer
	long unsigned int bytesUsed;		  // initially zero
};

/* 
 * @brief IcomIQPort Class
 */
class IcomIQPort : public std::enable_shared_from_this<IcomIQPort>{
private:
	FT_DEVICE_LIST_INFO_NODE iqNode;

	size_t _civBufferSize = 256;
	bool _isInitialized;
	bool _isOpen;
	bool _isStream;
	int  _cmdTimeout;
	int  _dataTimeout;
	bool _iqDataEnabled;
	bool _IPPlus;
	bool _DIGI_SEL;
	std::string version_ = VERSION_ICOMIQ_PORT;
	std::shared_ptr<spdlog::logger> _logger;
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
	std::string to_hex(const unsigned char* data, size_t len);
	void CheckStatus(const FT_STATUS status, const char* functionName);
	void GetInitialRadioParams();
	void AsyncReadWorker();
protected:
	FT_HANDLE ftHandle;
	//UCHAR pipeID = IQ_IN;

public:
	/*
	 * @brief name
	 * @params
	 * 
	 */
	IcomIQPort (void);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	void init (std::string deviceSerialNum, bool isStream = false); //find the Icom IQ port and initialize it
	/*
	 * @brief name
	 * @params
	 * 
	 */
	static std::string getDeviceSerialNum();
	/*
	 * @brief name
	 * @params
	 * 
	 */
	~IcomIQPort();
	/*
	 * @brief name
	 * @params
	 * 
	 */
	void close();
	/*
	 * @brief name
	 * @params
	 * 
	 */
	bool isInitialized() const {return _isInitialized;} // test to see if the port is initialized
		/*
	 * @brief name
	 * @params
	 * 
	 */
	bool isStream() const {return _isStream;} 
	/*
	 * @brief name
	 * @params
	 * 
	 */
	std::string version() { return version_; }
	/*
	 * @brief name
	 * @params
	 * 
	 */
	FT_HANDLE getHandle() {return ftHandle;}
	/*
	 * @brief name
	 * @params
	 * 
	 */
	bool isOpen(void) const {return _isOpen;} // test to see ifthe port is opened
	/*
	 * @brief name
	 * @params
	 * 
	 */
	bool sendIQCommand( std::vector<uint8_t> cmd); //send a command or data to IC7610 I/Q port
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  readIQReply( std::vector<uint8_t>& buffer); // read response or data from IC7610 I/Q port
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  icomIQCommand( std::vector<uint8_t> cmd, std::vector<uint8_t> &reply); // sends a command and reads the reply
	/*
	 * @brief name
	 * @params
	 * 
	 */
	void setTimeout(uint8_t channelID, int timeOut); // set command timeout
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  getTimeout(uint8_t channelID) const {return ((channelID == CMD_IN || channelID == CMD_OUT)?_cmdTimeout : _dataTimeout);} // get current timout value
	/*
	 * @brief name
	 * @params
	 * 
	 */
	bool enableIQData(uint8_t source); // enable the I/Q channel
	/*
	 * @brief name
	 * @params
	 * 
	 */
	void disableIQData(void); // disable I/Q data
	/*
	 * @brief name
	 * @params
	 * 
	 */
	bool isIQDataEnabled(void) const {return _iqDataEnabled;} // test if I/Q data is enabled
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  readIQData( std::complex<short> *buffer, size_t buffer_size,void* overlapped_ = NULL); // read data into a buffer

	// convert an int to bcd
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int read_bcd(int n) { return (10 * ((n & 0xf0) >> 4) + (n & 0x0f));}
	// convert a uint32_ to bcd digit
	/*
	 * @brief name
	 * @params
	 * 
	 */
    int bcd_digit(uint32_t n, int value){return (n / value) % 10;}
	/*
	 * @brief name
	 * @params
	 * 
	 */
	// convert uint32_t to bcddigits
	int bcd_digits(uint32_t n, int value){return 0x10 * bcd_digit(n, 10 * value) | bcd_digit(n, value);}
	/*
	 * @brief name
	 * @params
	 * 
	 */
	std::string iqGetChipConfiguration(); 
	/*
	 * @brief name
	 * @params
	 * 
	 */
	std::string iqGetDevicveDescriptor();
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int iqGetRFGain(uint8_t vfo);
		/*
	 * @brief name
	 * @params
	 * 
	 */
	int iqSetRFGain(uint8_t vfo, int rfgain);	//rfgain must be a value between 0 and 255
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int iqGetPreAmpStatus(uint8_t vfo);			// Returns 0 = off, 1 =PAMP1 On, 2 = PAMP2 On
	/*
	 * @brief name
	 * @params
	 * 
	 */
	uint8_t iqGetAttenuatorSettings(uint8_t vfo);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	uint8_t iqSetAttenuatorSettings(uint8_t vfo, int attenuation);  //attenuation settings are in 3db increments to 45
		/*
	 * @brief name
	 * @params
	 * 
	 */
	std::string iqGetAntenna(uint8_t vfo);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int iqSetAntenna(uint8_t vfo, int antenna, bool status);
	/*
	 * @brief name
	 * @params
	 * 
	 */	
	int iqGetFrequency(uint8_t vfo);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int iqSetFrequency(uint8_t vfo, uint32_t frequency);
	
	/*
	 * @brief name
	 * @params
	 * 
	 */
	bool iqGetDIGI_SEL_Status(uint8_t vfo);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	void iqSetDIGI_SEL_Status(uint8_t vfo, bool status);

	/*
	 * @brief name
	 * @params
	 * 
	 */
	//Intermodulation Performance Plus IP+
	bool iqGetIP_Status(uint8_t vfo);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	void iqSetIP_Status(uint8_t vfo, bool status);
	
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  iqAbortPipe(uint8_t pipe=IQ_IN);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	bool iqSetStreamPipe(uint8_t channelID, size_t streamBufferSize);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	bool iqClearStreamPipe(uint8_t channelID);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  iqInitializeOverlapped(void* overlapped_);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  iqReleaseOverlapped(void* overlapped);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  iqGetOverlappedResults( iqASYNC_CONTEXT* context, bool wait);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  readIQDataEx(uint8_t *buffer, size_t buffer_size, void* overlapped);


	//int  SetCallback(std::function<void(void* context, uint8_t pipeId, size_t count )> callback, void* callbackContext);
	//int  ClearCallback();
	/*
	 * @brief name
	 * @params
	 * 
	 */
	bool iqAsyncStart(uint8_t vfo);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	void iqAsyncStop(void);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	int  iqReadBuf(std::complex<short> *data, size_t numSamples);
	/*
	 * @brief name
	 * @params
	 * 
	 */
	size_t iqGetSizeOfAvailableData();
	/*
	 * @brief name
	 * @params
	 * 
	 */
	void iqClearReadBuf( void) {iqBuffer_.clear_ringbuffer();};
	/*
	 * @brief name
	 * @params
	 * 
	 */
	void iqAsyncReadWorker();
	//BlockingRingBuffer iqBuffer_ (16384);
			
};


#endif