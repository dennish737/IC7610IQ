#include "SoapyIC7610SDR.hpp"
#include "IcomCIVPort.hpp"
#include "IcomIQPort.hpp"
#include "common.h"
#include "ftd3xx.h"
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Logger.h>
#include <memory>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <complex>
#include <chrono>
#include <iterator>
#include <algorithm>
#include <thread>

#include <libserialport.h>
#include <unistd.h> // For usleep

/*******************************************************************
 * Stream API
 ******************************************************************
 *	virtual std::vector<std::string> SoapySDR::Device::getStreamFormats	(
 *			const int 	direction,
 *			const size_t 	channel) const
 *		Query a list of the available stream formats.
 *		Parameters:
 *			direction	the channel direction RX or TX
 *			channel	an available channel on the device
 *		Returns
 *			a list of allowed format strings. A format string representing the desired buffer format in read/writeStream()
 *				The first character selects the number type:
 *					"C" means complex
 *					"F" means floating point
 *					"S" means signed integer
 *					"U" means unsigned integer
 *				The type character is followed by the number of bits per number 
 *				(complex is 2x this size per sample)
 *				Example format strings:
 *
 *					"CF32" - complex float32 (8 bytes per element)
 *					"CS16" - complex int16 (4 bytes per element)
 *					"CS12" - complex int12 (3 bytes per element)
 *					"CS4" - complex int4 (1 byte per element)
 *					"S32" - int32 (4 bytes per element)
 *					"U8" - uint8 (1 byte per element)
 *
 *		Valid Soapy Formats for IC7610:
 *			Complex formats: These store IQ data as a pair of numbers.
 *			CS16: Complex Signed 16-bit is only format allowed
 *******************************************************************/
std::vector<std::string> SoapyIC7610SDR::getStreamFormats(const int direction, const size_t channel) const
{
    //check that direction is SOAPY_SDR_RX. Note SOAPY_SDR_TX is not supported
     if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
	 } 
	std::vector<std::string> formats;

	formats.push_back(SOAPY_SDR_S16);
	formats.push_back(SOAPY_SDR_CS16);
	formats.push_back(SOAPY_SDR_CF32);
	formats.push_back(SOAPY_SDR_CF64);
	return formats;
}

/*******************************************************************
*	virtual std::string SoapySDR::Device::getNativeStreamFormat	(
*			const int 	direction,
*			const size_t 	channel,
*			double & 	fullScale) const
*
*		Get the hardware's native stream format for this channel. 
*		This is the format used by the underlying transport layer, and 
*		the direct buffer access API calls (when available).
*		Parameters
*			direction	the channel direction RX or TX
*			channel	an available channel on the device
*			[out]	fullScale	the maximum possible value
*		Returns
*			the native stream buffer format string
*******************************************************************/
//std::string SoapyIC7610SDR::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const;
std::string   SoapyIC7610SDR::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const {
    //check that direction is SOAPY_SDR_RX. Note SOAPY_SDR_TX is not supported
     if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7600-SDR is RX only, use SOAPY_SDR_RX");
     }
 
     fullScale = 1<<15; //32768
     return SOAPY_SDR_CS16;
}

//	virtual size_t SoapySDR::Device::getStreamMTU(Stream * 	stream	)	const
//		Get the stream's maximum transmission unit (MTU) in number of elements. 
//		The MTU specifies the maximum payload transfer in a stream operation. 
//		This value can be used as a stream buffer allocation size that can best 
//		optimize throughput given the underlying stream implementation.
//		Parameters
//			stream	the opaque pointer to a stream handle
//		Returns
//			the MTU in number of stream elements (never zero)	
size_t SoapyIC7610SDR::getStreamMTU(SoapySDR::Stream *stream) const
{
	// data returned from the device is always a complex<short> (4 bytes of 2 sort I/Q interleave)
	// normally this is based on the IQ_BLOCK_SIZE/BYTES_PER_SAMPLE (~1024).
	// If we use a 'short' format, then the number of samples will be 2 time this value.
	size_t mtuSize = IQ_BLOCK_LENGTH / BYTES_PER_SAMPLE;
	//if (_rx_stream.getFormat() == IC7610FormatTypes::IC7610_SHORT) {mtuSize *= 2;};
	if (_rx_stream.getFormat() == SOAPY_SDR_S16) {mtuSize *= 2;};
    return mtuSize;
}

// buffers used for reading 
// One buffer is required for each channel
// what is returned is an array of buffer pointers
// The size of the buffer should be the MTU size
/*
void SoapyIC7610SDR::Stream::allocate_buffers(){

	_buffs = (uint8_t * *) malloc( buf_num * sizeof(uint8_t *) );
	if ( _buffs ) {
		for ( unsigned int i = 0; i < buf_num; ++i ) {
			_buffs[i] = (uint8_t *) malloc( buf_len );
		}
	}
}	


void SoapyIC7610SDR::Stream::clear_buffers() {
	clear_ringbuffer();
	
	/* used for direct access buffers
	if ( _buffs unsigned int i = 0; i < buf_num; ++i ) {
			if ( _buffs[i] ) {
				free( _buffs[i] );
			}
		}
		free( _buffs );
		_buffs = NULL;
	}

	_buf_count = 0;
	buf_head = 0;
	_remainderSamps = 0;
	_remainderOffset = 0;
	_remainderBuff = nullptr;
	_remainderHandle = -1;
	*/
/*
}
*/

// virtual ArgInfoList SoapySDR::Device::getStreamArgsInfo	(
//		const int 	direction,
//		const size_t 	channel ) const
//	Query the argument info description for stream args.
//	Parameters
//		direction	the channel direction RX or TX
//		channel	an available channel on the device
//	Returns
//		a list of argument info structures
SoapySDR::ArgInfoList SoapyIC7610SDR::getStreamArgsInfo(const int direction, const size_t channel) const {
    //check that direction is SOAPY_SDR_RX
     if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
     }

    SoapySDR::ArgInfoList streamArgs;

    SoapySDR::ArgInfo samplerateArg;
    samplerateArg.key = "sample_rate";
    samplerateArg.value = std::to_string(_rx_stream.sample_rate);
    samplerateArg.name = "sample rate frequncy";
    samplerateArg.description = "The IC7610 has a fixed sample rate";
    samplerateArg.units = "Hz";
    samplerateArg.type = SoapySDR::ArgInfo::INT;

    streamArgs.push_back(samplerateArg);

    SoapySDR::ArgInfo frequencyArg;
    frequencyArg.key = "frequency";
    frequencyArg.value = std::to_string(_rx_stream.frequency);
    frequencyArg.name = "Center Frequency";
    frequencyArg.description = "Sample center frequency";
    frequencyArg.units = "Hz";
    frequencyArg.type = SoapySDR::ArgInfo::INT;

    streamArgs.push_back(frequencyArg);

	// tbd add other available args

    return streamArgs;
}

//	virtual Stream* SoapySDR::Device::setupStream(
//			const int 	direction,
//			const std::string & 	format,
//			const std::vector< size_t > &channels = std::vector< size_t >(),
//			const Kwargs & 	args = Kwargs() )	
//		Initialize a stream given a list of channels and stream arguments. 
//		The implementation may change switches or power-up components. 
//		All stream API calls should be usable with the new stream object 
//		after setupStream() is complete, regardless of the activity state.
//		The API allows any number of simultaneous TX and RX streams, but 
//		many dual-channel devices are limited to one stream in each direction, 
//		using either one or both channels. This call will throw an exception 
//		if an unsupported combination is requested, or if a requested channel 
//		in this direction is already in use by another stream.
//		When multiple channels are added to a stream, they are typically expected 
//		to have the same sample rate. See setSampleRate().
//		Parameters
//			direction	the channel direction (SOAPY_SDR_RX or SOAPY_SDR_TX)
//			format	A string representing the desired buffer format in read()

//			channels	a list of channels or empty for automatic.
//			args	stream args or empty for defaults.
//					Recommended keys to use in the args dictionary:
//
//					"WIRE" - format of the samples between device and host
//		Returns
//			an opaque pointer to a stream handle.
//			The returned stream is not required to have internal locking, 
//			and may not be used concurrently from multiple threads.	
SoapySDR::Stream *SoapyIC7610SDR::setupStream(
        const int direction,
        const std::string &format,
        const std::vector<size_t> &channels,
        const SoapySDR::Kwargs &args)
{
    if (direction != SOAPY_SDR_RX)
    {
		// IQ streams are read only		
        throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
		//return SOAPY_SDR_NOT_SUPPORTED;
    }

    //check the channel configuration
    if (channels.size() > 1 or (channels.size() > 0 and channels.at(0) != 0))
    {
        throw std::runtime_error("setupStream invalid channel selection");
		//return SOAPY_SDR_NOT_SUPPORTED;
    }

    //check the format
	int32_t i = (int)validFormat(format);
    if (i > 0)
    {
		//_rx_stream.rxFormat = format;
        _rx_stream.setFormat( format) ;
    } else {
		_rx_stream.setFormat( SOAPY_SDR_CS16);
		//_rx_stream.setFormat( IC7610FormatTypes::IC7610_COMPLEX_SHORT);
        //throw std::runtime_error(
        //      "setupStream invalid format '" + format
        //          + "' -- use getStreamFormats() to get list of valid format. Default used.");
		//return SOAPY_SDR_NOT_SUPPORTED;
    };
	
	// set the stream variables

	//_rx_stream._stream_sample_rate = 1920000;
	//_rx_stream._stream_rfGain = _rfGain;
	//_rx_stream._stream_attenuation = _attenuation;
	//_rx_stream._stream_frequency = _frequency;
	//_rx_stream._stream_vfo = _vfo;
	//_rx_stream._stream_antenna = _antenna;
	// this is where we allocate buffers for direct access 
	
	
	// mark the stream as ready, not active	
	
    //clear async fifo counts
    //_buf_tail = 0;
    //_buf_count = 0;
    //_buf_head = 0;

    //allocate buffers
    //_buffs.resize(numBuffers);
    //for (auto &buff : _buffs) buff.data.reserve(buf_len);
    //for (auto &buff : _buffs) buff.data.resize(buf_len);


    return (SoapySDR::Stream *) this;
}

//	virtual void SoapySDR::Device::closeStream	(	Stream * 	stream	)
//		Close an open stream created by setupStream The implementation may change switches or power-down components.
//
//	Parameters
//		stream	the opaque pointer to a stream handle
void SoapyIC7610SDR::closeStream(SoapySDR::Stream *stream)
{
    deactivateStream(stream, 0, 0);
}



//	virtual int SoapySDR::Device::activateStream(	
//			Stream * 	stream,
//			const int 	flags = 0,
//			const long long 	timeNs = 0,
//			const size_t 	numElems = 0 )	
//		Activate a stream. Call activate to prepare a stream before using read/write(). 
//		The implementation control switches or stimulate data flow.
//
//		The timeNs is only valid when the flags have SOAPY_SDR_HAS_TIME. The numElems 
//		count can be used to request a finite burst size. The SOAPY_SDR_END_BURST flag 
//		can signal end on the finite burst. Not all implementations will support the 
//		full range of options. In this case, the implementation returns 
//		SOAPY_SDR_NOT_SUPPORTED.
//
//		Parameters
//			stream	the opaque pointer to a stream handle
//			flags	optional flag indicators about the stream
//			timeNs	optional activation time in nanoseconds
//			numElems	optional element count for burst control
//		Returns
//			0 for success or error code on failure
int SoapyIC7610SDR::activateStream(
        SoapySDR::Stream *stream,
        const int flags,
        const long long timeNs,
        const size_t numElems)
{
	printf("activateStream - flagd= %d, numElements = %lld\n", flags, numElems);
	int status;
	if( flags != 0) {return SOAPY_SDR_NOT_SUPPORTED; };

	status = 0;

	if (not _rx_stream.isOpened())
	{
		
		//TBD - assuming that if thread  is not joinable, it is not running
		//		This assumption may not be valid,
		//TBD radio setup for stream (e.g. frequency, gain, ...
		// try to enable the iq port. if we fail do not start
		
		status = iqPort.enableIQData(_vfo)? 0 : -1;
		if (status < 0) {
			printf(" unable to initialize port \n");
		} else {
			//clear the pipe
			iqPort.iqAbortPipe(IQ_IN);
		}
		/*
		if (iqPort.iqAsyncStart(_vfo))){
			// clear the port before 

		} else {
			// failed to enable
			status = -1;
		}
		*/
		
	} else {
		// there is an active reader, return -1
		status = -1;
	}
    return status;
}

//	virtual int SoapySDR::Device::deactivateStream(	
//			Stream * 	stream,
//			const int 	flags = 0,
//			const long long 	timeNs = 0 )	
//		Deactivate a stream. Call deactivate when not using using read/write(). 
//		The implementation control switches or halt data flow.
//		The timeNs is only valid when the flags have SOAPY_SDR_HAS_TIME. Not all implementations will support the full range of options. In this case, the implementation returns SOAPY_SDR_NOT_SUPPORTED.
//		Parameters
//			stream	the opaque pointer to a stream handle
//			flags	optional flag indicators about the stream
//			timeNs	optional deactivation time in nanoseconds
//		Returns
//			0 for success or error code on failure
int SoapyIC7610SDR::deactivateStream(SoapySDR::Stream *stream, const int flags, const long long timeNs)
{
	printf("flags = %d\n",flags);
    //if (flags != 0) return SOAPY_SDR_NOT_SUPPORTED;
	iqPort.iqAsyncStop();
	
	iqPort.disableIQData();
	iqPort.iqClearReadBuf();

    return 0;
}

//	virtual int SoapySDR::Device::readStream(	
//			Stream * 	stream,
//			void *const * 	buffs,
//			const size_t 	numElems,
//			int & 	flags,
//			long long & 	timeNs,
//			const long 	timeoutUs = 100000 )	
//		Read elements from a stream for reception. This is a multi-channel call, and buffs 
//		should be an array of void *, where each pointer will be filled with data from a 
//		different channel.
//
//		Client code compatibility: The readStream() call should be well defined at all times, 
//		including prior to activation and after deactivation. When inactive, readStream() should 
//		implement the timeout specified by the caller and return SOAPY_SDR_TIMEOUT.
//
//		Parameters
//			stream	the opaque pointer to a stream handle
//			buffs	an array of void* buffers num chans in size
//			numElems	the number of elements in each buffer
//			flags	optional flag indicators about the result
//			timeNs	the buffer's timestamp in nanoseconds
//			timeoutUs	the timeout in microseconds
//		Returns
//			the number of elements read per buffer or error code
int SoapyIC7610SDR::readStream(
        SoapySDR::Stream *stream,
        void * const *buffs, // one buffer for each channel in the system buffs[0] is the read buffer
        const size_t numElems,	// The number of elements (samples per channel requested
        int &flags,				// not used
        long long &timeNs,		// time for first sample
        const long timeoutUs)	// timeout duration in microseconds
{
    //drop remainder buffer on reset
	/*
    if (resetBuffer and bufferedElems != 0)
    {
        bufferedElems = 0;
        this->releaseReadBuffer(stream, _handle);
    }
	*/
	printf(" readStream called, numElems = %d \n", numElems);

	int res;
	DWORD count;

	size_t returnedElems = std::min(numElems,this->getStreamMTU(stream)); // get the smaller number
	printf(" readStream called, returnedElems = %d \n", returnedElems);
	
	// The Icom 7610 always returns a buffer of complex<short>, which is 4 bytes. We can return this data as a short, of interleaved I/Q data
	// or S16 format.  When we do this the number of complex elements is 1/2 the requested. In all other cases the number of complex elements
	// are returnedElements, which is the number requested or the max number of 1024.
	std::cout << " format: " << _rx_stream.getFormat() << std::endl;
	if (_rx_stream.getFormat() == SOAPY_SDR_S16) {
		returnedElems /= 2;	
	}
		
	// The other formats supported are CS16 (complex<short>), CF32 (complex<float> and CF64 <complex<double>). CF32 and CF64 are normalized 
	// to be between 1 and -1.
	const float norm = 32768.0f;
	
	// get a complex<short> vector of returnedElems size
	std::vector<std::complex<short>>buff(returnedElems);
	//int samplesRead = iqPort.iqReadBuf(buff.data(), returnedElems);
	//uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(buff.data());
	int samplesRead = iqPort.readIQData(buff.data(), returnedElems, NULL);
	std::cout << " readIQData returned: " << samplesRead << std::endl;
	
	// process the data
	if (samplesRead > 0) {
		// process the data
		std::cout << " Process data - format: " << _rx_stream.getFormat() << std::endl;
		
		if (_rx_stream.getFormat() == SOAPY_SDR_S16) {
			std::cout << " Process data - SOAPY_SDR_S16: " << std::endl;
			/*
			short *samples_cs16 = (short *)buffs[0];
			size_t s_size = sizeof(short);
			for (int32_t i = 0; i < samplesRead; i++) {
				samples_cs16[i*s_size] = buff[i].real();
				samples_cs16[i*s_size + 1] = buff[i].imag();
				i++;
			}
			*/
			size_t s_num_bytes = samplesRead * sizeof(std::complex<short>);
			std::cout << "bytes to transfer = " << s_num_bytes << std::endl;
			std::memcpy(buff.data(), buffs[0], s_num_bytes);
			
		} else if (_rx_stream.getFormat() == SOAPY_SDR_CS16 ){
			std::cout << " Process data - SOAPY_SDR_CS16: " << std::endl;
			/*
			std::complex<short> *samples_cs16 = (std::complex<short> *)buffs[0];			
			for (int32_t i = 0; i < samplesRead; i++) {
				samples_cs16[i] = buff[i];
			}
			*/
			size_t cs_num_bytes = samplesRead * sizeof(std::complex<short>);
			std::cout << "bytes to transfer = " << cs_num_bytes << std::endl;
			std::memcpy(buff.data(), buffs[0], cs_num_bytes);
			
		} else if (_rx_stream.getFormat() == SOAPY_SDR_CF32 ){
			std::cout << " Process data - SOAPY_SDR_CF32: " << std::endl;
			float *samples_cf32 = (float *)buffs[0];
			for (int32_t i = 0; i < samplesRead; i++){
			std::complex<float> c_float(
					static_cast<float>(buff[i].real())/norm,
					static_cast<float>(buff[i].imag())/norm
					);
				samples_cf32[i*2] = c_float.real();
				samples_cf32[i *2 +1] = c_float.imag();
			}
			
		} else if (_rx_stream.getFormat() == SOAPY_SDR_CF64 ){
			std::cout << " Process data - SOAPY_SDR_CF64: " << std::endl;
			double *samples_cf64 = (double *)buffs[0];
			for (int32_t i = 0; i < samplesRead; i++){
				std::complex<double> c_double(
					static_cast<double>(buff[i].real())/norm,
					static_cast<double>(buff[i].imag())/norm
					);
				samples_cf64[i * 2] = c_double.real();
				samples_cf64[i * 2 +1] = c_double.imag();
			};
		}
				
	} 
	
	return samplesRead;	
}

		