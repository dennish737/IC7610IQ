
#include "SoapyIC7610SDR.hpp"
#include "IcomCIVPort.hpp"
#include "IcomIQPort.hpp"
#include "ftd3xx.h"
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Logger.h>

#include <cstdio>

#include <memory>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <iterator>
#include <algorithm>
#include <thread>

#include <libserialport.h>
#include <unistd.h> // For usleep

#pragma comment(lib, "setupapi.lib")

SoapyIC7610SDR::SoapyIC7610SDR(const SoapySDR::Kwargs &args)
{
	// arguments:
	// driver=SoapyIC7610SDR 		-> driver name


	// vfo=[MAIN | SUB]				-> Main or Sub vfo (default MAIN)
	// ipplus=[<yes>|<no>]			-> Intermodulation Performance Plus (default no)
	// digi_sel=[<yes>|<no>]		-> DIGI-SEL (default-no)
	//enum sp_return result;
	
	// stream parameters
	// bufflen - buffer length (bufferLength), set in IC7610SDR_Streaming.cpp ->setupStream
	// buffers - number of buffers (numBuffers), set in IC7610SDR_Streaming.cpp ->setupStream 
	// asyncBuffs - number of async buffers (asyncBuffs), set in IC7610SDR_Streaming.cpp ->setupStream
	
	int baudrate = 115200;
	
    SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::SoapyIC7610SDR");

	
    if (args.count("label") != 0) SoapySDR_logf(SOAPY_SDR_INFO, "Opening %s...", args.at("label").c_str());

	/*
	_rx_stream.setIQBlockSize(IQ_BLOCK_LENGTH);
	
    if (args.count("iqbufflen") != 0)
    {
        try
        {
            int bufferLength_in = std::stoi(args.at("iqbufflen"));
            if (bufferLength_in > 0)
            {
                _rx_stream.setIQBlockSize(bufferLength_in);
            }
        }
        catch (const std::invalid_argument &){}
    }
	
	_rx_stream.setNumIQBlocks( NUM_BLOCKS);
	    if (args.count("iqblocks") != 0)
    {
        try
        {
            int numBuffers_in = std::stoi(args.at("iqblocks"));
            if (numBuffers_in > 0)
            {
                _rx_stream.setNumIQBlocks(numBuffers_in);
            }
        }
        catch (const std::invalid_argument &){}
    }
    SoapySDR_logf(SOAPY_SDR_DEBUG, "IC7610-SDR Using %d IQ blocks", _rx_stream.getNumIQBlocks());
	*/
	
	_ipplus = false;
	if (args.count("ipplus") != 0)
	{
		// assume we set to true
		_ipplus = true;
	}
	
	_digi_sel = false;
	if (args.count("digi_sel") != 0)
	{
		// if flag is present assume true
		_digi_sel = true;
	}

	_rx_stream._vfo = 0x00;
	if (args.count("vfo") != 0)
	{
		if (args.at("vfo") == "MAIN")
		{
			_rx_stream._vfo = 0x00;
		} else if (args.at("vfo") == "SUB")
		{
			_rx_stream._vfo = 0x01;
		} else
		{
			_rx_stream._vfo = 0x00; // error conditon - TBD add log and define error response
		}
	};
	

    if (_iq_port && args.count("serial") == 0) throw std::runtime_error("No SoapyIC7610SDR IQ devices not defined!");
	if (args.count("serial") != 0)
	{
		serial = args.at("serial");
	}
	 		
	// Open the high speed usb data port

	iqPort.init(serial);
	if (iqPort.isOpen())
    {
		// test iqport
		//iqPort.iqSetBufferSize(_rx_stream.getIQBlockSize());
		std::vector<uint8_t> cmd = {0x1a, 0x0b};
		std::vector<uint8_t> reply;
		iqPort.icomIQCommand(cmd, reply);
	}
	
	if (iqPort.isOpen())
	{
		iqPort.iqSetDIGI_SEL_Status(_rx_stream._vfo, _digi_sel);
		iqPort.iqSetIP_Status(_rx_stream._vfo, _ipplus);
	}
	
	// Initialize from system.
	// get frequency, gain and attenuation and antenna setting
	_rx_stream.rfGain = iqPort.iqGetRFGain( _rx_stream._vfo);
	//printf("RF Gaind = %d \n",gain);
	_rx_stream.attenuation = iqPort.iqGetAttenuatorSettings(_rx_stream._vfo);
	//printf("Attenuation = %d\n",atten);
	_rx_stream.antenna = iqPort.iqGetAntenna(_rx_stream._vfo);
	//printf("Antenna : %s \n", antenna.c_str());
	_rx_stream.frequency = (double) iqPort.iqGetFrequency(_rx_stream._vfo);
	//printf("Main Frequency = %d\n", freq);

	
	
}

SoapyIC7610SDR::~SoapyIC7610SDR()
{
    SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::~SoapyIC7610SDR");
	// Close and free the port (unreachable in this infinite loop but good practice for proper exit)
	
	iqPort.close();
	//civPort.~IcomCIVPort();
	// Close and free the port (unreachable in this infinite loop but good practice for proper exit)

}

/*******************************************************************
 * Identification API
 ******************************************************************/
// virtual std::string SoapySDR::Device::getDriverKey	(	void 		)	const
// 		A key that uniquely identifies the device driver. 
//		This key identifies the underlying implementation. 
//		Serveral variants of a product may share a driver.
// The key must be the same value as the SoapySDR::Registry name value
std::string SoapyIC7610SDR::getDriverKey(void) const
{
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::getDriverKey");
	
    return "SoapyIC7610SDR";
}

// virtual std::string SoapySDR::Device::getHardwareKey	(	void 		)	const
//		A key that uniquely identifies the hardware. This key should be meaningful 
//		to the user to optimize for the underlying hardware.
std::string SoapyIC7610SDR::getHardwareKey(void) const
{
	SoapySDR_logf(SOAPY_SDR_INFO, "getHardwareKey called");
    return "IC-7610";
}

// virtual Kwargs SoapySDR::Device::getHardwareInfo	(	void 		)	const
//		Query a dictionary of available device information. 
//		This dictionary can any number of values like vendor name, product name, 
//		revisions, serials... This information can be displayed to the user to 
//		help identify the instantiated device.
SoapySDR::Kwargs SoapyIC7610SDR::getHardwareInfo(void) const
{
    //key/value pairs for any useful information
    //this also gets printed in --probe
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::getHardwareInfo");
    SoapySDR::Kwargs args;

    args["origin"] = "add to github SoapyIC7610SDR";
    args["serial"] = serial;
	args["vfo"]= (_rx_stream._vfo == 1)?"SUB" : "MAIN";
	args["digi_sel"]= (_digi_sel)?"on" : "off";
	args["ipplus"]= (_ipplus)?"on" : "off";

    return args;
}

/*******************************************************************
 * Channels API
 ******************************************************************/
//	virtual size_t SoapySDR::Device::getNumChannels	(const int 	direction)	const
//			Get a number of channels given the streaming direction
// a Channel is the number of outputs alloweed at one time. Forthe IC7610, we have 1 IQ and 1 Aduio
size_t SoapyIC7610SDR::getNumChannels(const int dir) const
{
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::getNumChannels");
    return (dir == SOAPY_SDR_RX) ? 1 : 0;
}

//	virtual bool SoapySDR::Device::getFullDuplex(	
//		const int 	direction,
//		const size_t 	channel 
//	)	
//		Find out if the specified channel is full or half duplex.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//		Returns
//			true for full duplex, false for half duplex	
bool SoapyIC7610SDR::getFullDuplex(const int direction, const size_t channel) const
{
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::getFullDuplex");
	(void) direction;
	(void) channel;
	
    return false;
}

/*******************************************************************
 * Antenna API
 ******************************************************************/

// virtual std::vector<std::string> SoapySDR::Device::listAntennas	(
//			const int 	direction,
//			const size_t 	channel ) const
//		Get a list of available antennas to select on a given chain.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//		Returns
//			a list of available antenna names	
std::vector<std::string> SoapyIC7610SDR::listAntennas(const int direction, const size_t channel) const
{
     if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
     }
	(void) channel;
	std::vector<std::string> antennas;

	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::listAntennas");
	
	antennas.push_back("ANT1_OFF"); // ANT1 OFF
	antennas.push_back("ANT1_ON");	// ANT1 ON
	antennas.push_back("ANT2_OFF");	// ANT2 Off
	antennas.push_back("ANT2_ON");	// ANT2_ON
	antennas.push_back("RX_ON"); 	// TODO: research RX, should we use default or actual
	antennas.push_back("RX_OFF");
	antennas.push_back("RX");
    return antennas;
}

//	virtual std::string SoapySDR::Device::getAntenna(
//			const int 	direction,
//			const size_t 	channel  )const
//		Get the selected antenna on a chain.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//		Returns
//			the name of an available antenna
std::string SoapyIC7610SDR::getAntenna(const int direction, const size_t channel) const
{
	// direction should be SOAPY_SDR_RX. SOAPY_SDR_TX is not supported by device
	// channel shold always be zero (0), since only one channel is supported
	// TODO Add test for direction, and determine error
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::getAntenna");
     if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
     }
	fprintf(stderr, "getAntena called. Antenna is %s \n", _rx_stream.antenna.c_str());
	(void) channel;
	return _rx_stream.antenna;

}

// virtual void SoapySDR::Device::setAntenna	(
//			const int 	direction,
//			const size_t 	channel,
//			const std::string &name )
//		Set the selected antenna on a chain.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//			name	the name of an available antenna
void SoapyIC7610SDR::setAntenna(const int direction, const size_t channel, const std::string &name)
{
	// channel should always be zero (0), since only one channel is supported
	// direction should be SOAPY_SDR_RX. SOAPY_SDR_TX is not supported by device
     if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
     }
	(void) channel;
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::setAntenna");
	SoapySDR::Kwargs antArgs = {
		{"ANT1_ON", "0"},
		{"ANT2_ON", "1"},
		{"RX_ON", "4"},
		{"ANT1_OFF", "2"},
		{"ANT2_OFF", "3"},
		{"RX_OFF", "5"},
		{"ANT1", "0"},
		{"ANT2", "1"},
		{"RX", "4"}
	};
	int intValue=0;
	bool status = true;
	
	fprintf(stderr, "setAntenna Called. name = %s", name.c_str());

	if (antArgs.count(name) != 0)
	{
		const std::string& stringValue = antArgs.at(name);
		try {
			// Convert the string value to an integer
			intValue = std::stoi(stringValue);

		} catch (const std::invalid_argument& e) {
			std::cerr << "Error: Invalid argument for std::stoi: " << e.what() << std::endl;
		} catch (const std::out_of_range& e) {
			std::cerr << "Error: Value out of range for std::stoi: " << e.what() << std::endl;
		}
		if (intValue > 3){	
			// this is an RX antenna don not change anything
			_rx_stream.antenna = name;
			return;
		};
		if( intValue > 1)
		{
			intValue -= 2;
			status = false;
		}
		int atnReturn = iqPort.iqSetAntenna(_rx_stream._vfo, intValue, status);
		if (atnReturn == 0)
		{
			_rx_stream.antenna = name;
		}
		
	} else
	{
		std::cerr << "Error: Invalid argument for antenna: " << std::endl;
	}
}

/*******************************************************************
 * Gain API
 ******************************************************************/

std::vector<std::string> SoapyIC7610SDR::listGains( const int direction, const size_t channel ) const
{
	if (direction != SOAPY_SDR_RX) {
		throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
	}
	(void) channel;
	std::vector<std::string> options;
	if ( direction == SOAPY_SDR_RX )
	{
	// in gr-osmosdr/lib/soapy/ soapy_sink_c.cc and soapy_source_c.cc expect if_gain at front and bb_gain at back
		options.push_back( "PAMP1" );						// RX: rf_gain ~12dB gain
		options.push_back( "PAMP2" );						// RX: rf_gain ~16dB gain
		options.push_back( "AMP" );
		//options.push_back( "RFGAIN" );
		// IP+ Function: An additional feature that can be turned on/off
	    // to reduce ADC distortion
		// Low Noise/Weak Signals: Use Preamp 2 on quieter bands (like 50 MHz) or for 
		// very weak signals to bring them above the noise floor.	
		// Strong Signal Conditions: Avoid preamps on busy lower HF bands (like 40m/20m) 
		// where they can overload the receiver; instead, use the internal attenuator 
		//(ATT) or manual RF gain.		
	}


	return(options);
	/*
	 * list available gain elements,
	 * the functions below have a "name" parameter
	 */
}

void SoapyIC7610SDR::setGainMode( const int direction, const size_t channel, const bool automatic )
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
     }
	(void) channel;
	/* enable AGC if the hardware supports it, or remove this function */
	(void) automatic;
}


bool SoapyIC7610SDR::getGainMode( const int direction, const size_t channel ) const
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	return(false);
	/* ditto for the AGC */
}

void SoapyIC7610SDR::setGain(const int direction, const size_t channel, const double value)
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	//TBD
}
void SoapyIC7610SDR::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	//TBD
	(void) value;
	(void) name;
}
double SoapyIC7610SDR::getGain(const int direction, const size_t channel, const std::string &name) const
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	//std::lock_guard<std::mutex> lock(_device_mutex);
	double gain = 0.0;	
	if ( direction == SOAPY_SDR_RX and name == "PAMP1" )
	{
		gain = 0.0; //tbd
	}else if ( direction == SOAPY_SDR_RX and name == "PAMP2" )
	{
		gain = 0.0; //tbd
	}else if ( direction == SOAPY_SDR_RX and name == "AMP" )
	{
		gain = 0.0; //tbd
	}
	return gain;
}

SoapySDR::Range SoapyIC7610SDR::getGainRange( const int direction, const size_t channel, const std::string &name ) const
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void)channel;
	if (direction == SOAPY_SDR_RX and name == "AMP" )
		return(SoapySDR::Range( 0, IC7610_RX_AMP_MAX_DB,  IC7610_RX_AMP_MAX_DB) );
	if ( direction == SOAPY_SDR_RX and name == "PAMP1" )
		return(SoapySDR::Range( 0, IC7610_RX_PAMP1_MAX_DB , IC7610_RX_PAMP1_MAX_DB ) );
	if ( direction == SOAPY_SDR_RX and name == "PAMP2")
		return(SoapySDR::Range( 0, IC7610_RX_PAMP1_MAX_DB ,IC7610_RX_PAMP1_MAX_DB ) );

	return(SoapySDR::Range( 0, 0 ) );
}

/*******************************************************************
 * Frequency API
 ******************************************************************/
 
 //	virtual void SoapySDR::Device::setFrequency	(
//			const int 	direction,
//			const size_t 	channel,
//			const std::string & 	name,
//			const double 	frequency,
//			const Kwargs & 	args = Kwargs() )
//		Tune the center frequency of the specified element.
//			For RX, this specifies the down-conversion frequency.
//			For TX, this specifies the up-conversion frequency.
//			Recommended names used to represent tunable components:
//
//				"CORR" - freq error correction in PPM
//				"RF" - frequency of the RF frontend
//				"BB" - frequency of the baseband DSP
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//			name	the name of a tunable element
//			frequency	the center frequency in Hz
//			args	optional tuner arguments	
void SoapyIC7610SDR::setFrequency(
        const int direction,
        const size_t channel,
        const std::string &name,
        const double frequency,
        const SoapySDR::Kwargs &args)
{

	(void) channel;
	
	int result;
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::setFrequency");
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
    if (name == "RF")
    {
		printf("Channel = %lld \n",channel);
        uint32_t f = frequency;
        SoapySDR_logf(SOAPY_SDR_INFO, "Setting center freq: %d", f);
		if (iqPort.isOpen())
		{
			result = iqPort.iqSetFrequency(_rx_stream._vfo, f);
			if (result > 0)
			{
				_rx_stream.frequency = frequency;
			} else {
				// else do not change frequency and log error
				SoapySDR_logf(SOAPY_SDR_ERROR, "Setting center freq: %d failed", f);
			}
		}
		
    }
    else if (name == "CORR")
    {
        /*
        int r = rtlsdr_set_freq_correction(dev, (int)frequency);
        if (r == -2)
        {
            return; // CORR didn't actually change, we are done
        }
        if (r != 0)
        {
            throw std::runtime_error("setFrequencyCorrection failed");
        }
        ppm = rtlsdr_get_freq_correction(dev);
        */
    }
    else
        SoapySDR_logf(SOAPY_SDR_INFO, "setFrequency: unknown name %s", name.c_str());
}

//	virtual double SoapySDR::Device::getFrequency	(
//			const int 	direction,
//			const size_t 	channel)	const
//		Get the frequency of a tunable element in the chain.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//			name	the name of a tunable element
//		Returns
//			the tunable element's frequency in Hz
double SoapyIC7610SDR::getFrequency(const int direction, const size_t channel) const
{
	
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::getFrequency");
	return _rx_stream.frequency;
	
}


//	virtual double SoapySDR::Device::getFrequency	(
//			const int 	direction,
//			const size_t 	channel,
//			const std::string & name )	const
//		Get the frequency of a tunable element in the chain.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//			name	the name of a tunable element
//		Returns
//			the tunable element's frequency in Hz
double SoapyIC7610SDR::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::getFrequency");
	SoapySDR_logf(SOAPY_SDR_INFO, "getFrequency called, Direction: %d, Channel: %d, name: %s", 
				direction, channel, name.c_str());
    if (name == "RF")
    {
		return (double) _rx_stream.frequency;
	}   
    else if (name == "CORR")
    {
        return (double) 1; // TODO ppm;
    }
    else
	{
        SoapySDR_logf(SOAPY_SDR_INFO, "getFrequency: unknown name %s", name.c_str());
	}
    return 0;
}

SoapySDR::ArgInfoList SoapyIC7610SDR::getFrequencyArgsInfo(const int direction, const size_t channel) const
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	SoapySDR::ArgInfoList freqArgs;
	// TODO: frequency arguments
	return freqArgs;
}

//	virtual std::vector<std::string> SoapySDR::Device::listFrequencies	(
//			const int 	direction,
//			const size_t 	channel )	const
//		List available tunable elements in the chain. Elements should be in order RF to baseband.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel
//		Returns
//			a list of tunable elements by name
std::vector<std::string> SoapyIC7610SDR::listFrequencies(const int direction, const size_t channel) const
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	std::vector<std::string> names;
	if ( direction == SOAPY_SDR_RX )
	{
		SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::listFrequencies");
		
		names.push_back("RF");
		//names.push_back("TX");
		//names.push_back("CORR");
	}
    return names;
}

//	virtual RangeList SoapySDR::Device::getFrequencyRange	(
//			const int 	direction,
//			const size_t 	channel,
//			const std::string & 	name ) const
//		Get the range of tunable values for the specified element.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//			name	the name of a tunable element
//		Returns
//			a list of frequency ranges in Hz
SoapySDR::RangeList SoapyIC7610SDR::getFrequencyRange(
        const int direction,
        const size_t channel,
        const std::string &name) const
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::getFrequencyRange");
	SoapySDR::RangeList results;
	if ( direction == SOAPY_SDR_RX )
	{
		if (name == "RF")
		{
			//results.push_back(SoapySDR::Range(30000, 60000000));
			results.push_back(SoapySDR::Range(0, 60000000));
		}
	}
    if (name == "TX")
    {
        results.push_back(SoapySDR::Range(135700, 137800));
        results.push_back(SoapySDR::Range(1800000, 29700000)); // TODO: fill in bands
        results.push_back(SoapySDR::Range(50000000, 54000000));
    }
    if (name == "CORR")
    {
        // TODO results.push_back(SoapySDR::Range(-1000, 1000));
    }
    return results;
}

/*******************************************************************
 * Sample Rate API
 ******************************************************************/

//	virtual void SoapySDR::Device::setSampleRate(
//			const int 	direction,
//			const size_t 	channel,
//			const double 	rate )	
//		Set the baseband sample rate of the chain.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//			rate	the sample rate in samples per second
void SoapyIC7610SDR::setSampleRate(const int direction, const size_t channel, const double rate)
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	(void)rate;
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::setSampleRate");
    return;
}

//	virtual double SoapySDR::Device::getSampleRate	(
//			const int 	direction,
//			const size_t 	channel )	
//		Get the baseband sample rate of the chain.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//		Returns
//			the sample rate in samples per second
//			Note 7610 Sample rate is constant at 1.92MHz
double SoapyIC7610SDR::getSampleRate(const int direction, const size_t channel) const
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::getSampleRate");
    return 1920000;
}


//	virtual RangeList SoapySDR::Device::getSampleRateRange	(
//			const int 	direction,
//			const size_t 	channel ) const
//		Get the range of possible baseband sample rates.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//		Returns
//			a list of sample rate ranges in samples per second

//std::vector<double> SoapyIC7610SDR::getSampleRateRange(const int direction, const size_t channel) const

//	virtual RangeList SoapySDR::Device::listSampleRates	(
//			const int 	direction,
//			const size_t 	channel ) const
//		Get the range of possible baseband sample rates.
//		Parameters
//			direction	the channel direction RX or TX
//			channel	an available channel on the device
//		Returns
//			a list of sample rate ranges in samples per second
std::vector<double> SoapyIC7610SDR::listSampleRates(const int direction, const size_t channel) const
{
    if (direction != SOAPY_SDR_RX) {
         throw std::runtime_error("IC7610-SDR is RX only, use SOAPY_SDR_RX");
    }
	(void) channel;
	SoapySDR_logf(SOAPY_SDR_INFO, "SoapyIC7610SDR::listSampleRates");
    std::vector<double> results;
    results.push_back(1920000);
    return results;
}
