/***********************************************************************
 * Device interface for IC7610IQ
 * Note - commens are from SoapySDR Documentation
 **********************************************************************/

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.h>
#include <SoapySDR/Logger.h>
#include <SoapySDR/Registry.hpp>

#include "IcomIQPort.hpp"
#include "common.h"

#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <iomanip>
#include <cstdint>
#include <chrono>
#include <thread>
#include <sstream>
#include <complex>
//#include <libserialport.h> // Assumes you have the serial library set up
#include "ftd3xx.h"



#include <windows.h>

#include <atomic>


#include "IcomIQPort.hpp"

#define IC7610_RX_PAMP1_MAX_DB 12
#define IC7610_RX_PAMP2_MAX_DB 16
#define IC7610_RX_AMP_MAX_DB 0

enum class IC7610FormatTypes : int{
	IC7610_INVALID,
	IC7610_SHORT,
	IC7610_COMPLEX_SHORT,
	IC7610_COMPLEX_FLOAT,
	IC7610_COMPLEX_DOUBLE
};

class SoapyIC7610SDR : public SoapySDR::Device
{
private:
    std::string serial;
    //IcomCIVPort civPort;
	IcomIQPort  iqPort;
	//bool _civ_port;
	bool _iq_port;		// the device
	bool _ipplus;
	bool _digi_sel;


    std::string rxFormat;
	int sample_rate = 1920000;
	int _rfGain;
	int _attenuation;
	double _frequency;
	uint8_t _vfo;
	std::string _antenna;
	
	/// Mutex protecting all use of the device _dev and other instance variables.
	/// Most of the  IcomIQPort API is thread-safe, however the activateStream() method in
	/// this library can close and re-open the device, so all use of _dev must be protected.
	/// We also need to pretect access to the buffers. Note stream are typicall run in a seperate thread
	/// and are blocked until completion. 
	mutable std::mutex	_device_mutex;

	//std::thread write_thread; no write on IC7610
	SoapySDR::Stream* const RX_STREAM = (SoapySDR::Stream*) 0x1;

	struct Stream {
	private:						
			//IC7610FormatTypes format; // used to determine the output format	
			std::string format;
			bool _initialized;
			bool opened;		// true is stream is open - active
			

	/**********************************************************************
	 * Devices like HACKRF, LIME allow for for direct buffer access and async callbacks
	 * Call backs not really availabe withthe FTDI D3XX driver. So the driver wrapper uses 
	 * a Ring Buffer with a with a thread to to read the data from the device and store it.
	 * 
	 * Because of there is no need for buffer definitions here.
	 *
	 * Also the natimve format from the IC7610 isa complex<short> (4 byte interlive IQ data)
	 * Thus the data comming from the device is always a complex<short>. The advantage of this
	 * is we can easly convert a complex<short> to a pair of short S16, int s32, or complex floats and double
	 ***********************************************************************************/
	 
		
	public:
		// only 1 read buffer is allocated
		Stream()
		{
			opened = false;
			//_stream_rxFormat;	
			//_stream_sample_rate = sample_rate;
			//_stream_rfGain = SoapyIC7610SDR::_rfGain;
			//_stream_attenuation = SoapyIC7610SDR::_attenuation;
			//_stream_frequency = SoapyIC7610SDR::_frequency;
			
			//format = IC7610FormatTypes::IC7610_COMPLEX_SHORT;	
			format = SOAPY_SDR_CS16;	
		}
							
		
		bool isInitialized() { return _initialized; };
		bool isOpened() { return opened; };
		//size_t getNumBuffers() const { return buf_num;};
		//size_t getBubufferLen()const { return buf_len; };
		
		//size_t getNumIQBlocks() const {return numIQBlocks_; };
		//void setNumIQBlocks(size_t size) {if (size > 0) numIQBlocks_ = size; };
		//size_t getIQBlockSize()const {return blockSize_; };	
		//void setIQBlockSize(size_t size) {
		//	// block must be mutiple of 512 
		//	if ((size % 512) == 0){
		//		blockSize_ = size;
		//	}
		//}
		~Stream() {  _initialized = false;}
		std::string getFormat() const { return format; };
		void setFormat( std::string value) { format = value; };
		
		//void clear_buffers();
		//void allocate_buffers();
		//void streamStarted() { opened = true; };
		//void shutdownStream() { clear_buffers(); opened = false; }	

	};


	struct RXStream: Stream {
		int sample_rate = 1920000;
		//long long time_interval = (long long)((1.0/sample_rate)* 10^9);
		int rfGain = 0.0;
		int attenuation;
		double frequency;
		uint8_t _vfo;
		std::string antenna;
		double preampGain; 
		double attGain = 0.0;
		
		bool overflow;
				
	};


	RXStream _rx_stream;
	
	// worker functions **************************************
	template <typename T>
	bool is_number(const std::string& s)
	{
		T num; 
		std::stringstream ss(s); 
		ss >> num; 
		return ss && ss.eof();
	};
	
	IC7610FormatTypes  validFormat(const std::string &format)
	{
		IC7610FormatTypes  valid = IC7610FormatTypes::IC7610_INVALID;
		if (format == SOAPY_SDR_S16)
		{
			return IC7610FormatTypes::IC7610_SHORT;
		} else if(format == SOAPY_SDR_CS16){
			return IC7610FormatTypes::IC7610_COMPLEX_SHORT;
		} else if(format == SOAPY_SDR_CF32) {
			return IC7610FormatTypes::IC7610_COMPLEX_FLOAT;
		}else if(format == SOAPY_SDR_CF64){
			return IC7610FormatTypes::IC7610_COMPLEX_DOUBLE;
		};
		
		return IC7610FormatTypes::IC7610_INVALID;
}
		
	
	
public:
    SoapyIC7610SDR(const SoapySDR::Kwargs &args);
    ~SoapyIC7610SDR(void);

    // Identification API

    std::string getDriverKey(void) const;
    std::string getHardwareKey(void) const;
    SoapySDR::Kwargs getHardwareInfo(void) const;

    // Channels API
    size_t getNumChannels(const int dir) const;
    bool getFullDuplex(const int direction, const size_t channel) const;

    // Stream API
    std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const;
    std::string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const;
    SoapySDR::ArgInfoList getStreamArgsInfo(const int direction, const size_t channel) const;	
    SoapySDR::Stream *setupStream(const int direction, const std::string &format, const std::vector<size_t> &channels =
            std::vector<size_t>(), const SoapySDR::Kwargs &args = SoapySDR::Kwargs());	
    void closeStream(SoapySDR::Stream *stream);
    size_t getStreamMTU(SoapySDR::Stream *stream) const;
    int activateStream(
            SoapySDR::Stream *stream,
            const int flags = 0,
            const long long timeNs = 0,
            const size_t numElems = 0);
    int deactivateStream(SoapySDR::Stream *stream, const int flags = 0, const long long timeNs = 0);
    int readStream(
            SoapySDR::Stream *stream,
            void * const *buffs,
            const size_t numElems,
            int &flags,
            long long &timeNs,
            const long timeoutUs = 100000);

//	void releaseReadBuffer(
//			SoapySDR::Stream *stream,
//			const size_t handle);
	
//	int readStreamStatus(
//			SoapySDR::Stream *stream,
//			size_t &chanMask,
//			int &flags,
//			long long &timeNs,
//			const long timeoutUs
//	);			
void clear_buffers() { iqPort.iqClearReadBuf(); };
    // *******************************************************************
    //  * Direct buffer access API : Not Supported
    //  ******************************************************************
    // size_t getNumDirectAccessBuffers(SoapySDR::Stream *stream);
    // int getDirectAccessBufferAddrs(SoapySDR::Stream *stream, const size_t handle, void **buffs);
    // int acquireReadBuffer(
    //     SoapySDR::Stream *stream,
    //     size_t &handle,
    //     const void **buffs,
    //     int &flags,
    //     long long &timeNs,
    //     const long timeoutUs = 100000);
    // void releaseReadBuffer(
    //     SoapySDR::Stream *stream,
    //     const size_t handle);
    // *******************************************************************
    //  * Settings API
    //  ******************************************************************/
    // SoapySDR::ArgInfoList getSettingInfo(void) const;
    // void writeSetting(const std::string &key, const std::string &value);
    // std::string readSetting(const std::string &key) const;
	
    // /*******************************************************************
    //  * Antenna API
    //  ******************************************************************/
    std::vector<std::string> listAntennas(const int direction, const size_t channel) const;
    void setAntenna(const int direction, const size_t channel, const std::string &name);
    std::string getAntenna(const int direction, const size_t channel) const;
    // /*******************************************************************
    //  * Frontend corrections API
    //  ******************************************************************/
    bool hasDCOffsetMode(const int direction, const size_t channel) const { (void*)direction; return false;};
	bool hasDCOffset(const int direction, const size_t channel) const {(void*)direction; return false;};
	bool hasIQBalanceMode(const int direction, const size_t channel) const {(void*)direction; return false;};
    bool hasFrequencyCorrection(const int direction, const size_t channel) const {(void*)direction; return false;};
    // void setFrequencyCorrection(const int direction, const size_t channel, const double value);
    // double getFrequencyCorrection(const int direction, const size_t channel) const;
    // /*******************************************************************
    //  * Gain API
    //  ******************************************************************/
    std::vector<std::string> listGains(const int direction, const size_t channel) const;
    bool hasGainMode(const int direction, const size_t channel) const {(void*)direction; return false;};
    void setGainMode(const int direction, const size_t channel, const bool automatic);
    bool getGainMode(const int direction, const size_t channel) const;
    void setGain(const int direction, const size_t channel, const double value);
    void setGain(const int direction, const size_t channel, const std::string &name, const double value);
    double getGain(const int direction, const size_t channel, const std::string &name) const;
    SoapySDR::Range getGainRange(const int direction, const size_t channel, const std::string &name) const;
    // /*******************************************************************
    //  * Frequency API
    //  ******************************************************************/
    void setFrequency(
            const int direction,
            const size_t channel,
            const std::string &name,
            const double frequency,
            const SoapySDR::Kwargs &args = SoapySDR::Kwargs());
			
    double getFrequency(const int direction, const size_t channel) const;
	double getFrequency(const int direction, const size_t channel, const std::string &name) const;
	
    std::vector<std::string> listFrequencies(const int direction, const size_t channel) const;
    SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel, const std::string &name = "RF") const;
    SoapySDR::ArgInfoList getFrequencyArgsInfo(const int direction, const size_t channel) const;

    // Sample Rate API
    void setSampleRate(const int direction, const size_t channel, const double rate);
    double getSampleRate(const int direction, const size_t channel) const;
    std::vector<double> listSampleRates(const int direction, const size_t channel) const;
    // SoapySDR::RangeList getSampleRateRange(const int direction, const size_t channel) const;
    // void setBandwidth(const int direction, const size_t channel, const double bw);
    // double getBandwidth(const int direction, const size_t channel) const;
    // std::vector<double> listBandwidths(const int direction, const size_t channel) const;
    // SoapySDR::RangeList getBandwidthRange(const int direction, const size_t channel) const;
    // /*******************************************************************
    //  * Time API
    //  ******************************************************************/
    // std::vector<std::string> listTimeSources(void) const;
    // std::string getTimeSource(void) const;
    // bool hasHardwareTime(const std::string &what = "") const;
    // long long getHardwareTime(const std::string &what = "") const;
    // void setHardwareTime(const long long timeNs, const std::string &what = "");
    // /*******************************************************************
    //  * Utility
    //  ******************************************************************/
    // static std::string rtlTunerToString(rtlsdr_tuner tunerType);
    // static rtlsdr_tuner rtlStringToTuner(std::string tunerType);
    // static int getE4000Gain(int stage, int gain);
    // /*******************************************************************

};