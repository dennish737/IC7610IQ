




#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Logger.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <map>
#include <iomanip>


int main() {
	// log to a file
    // 1. Store the original cerr buffer
	/*
    std::streambuf* originalCerrBuffer = std::cerr.rdbuf();

    // 2. Open a file for logging
    std::ofstream logFile("soapystream_log.txt");

    // 3. Check if the file opened successfully
    if (!logFile.is_open()) {
        std::cerr << "Error: Could not open log file." << std::endl;
        return 1;
    }
	
	// 4. Redirect std::cerr to the log file
    std::cerr.rdbuf(logFile.rdbuf());
	
	SoapySDR::setLogLevel(SOAPY_SDR_INFO);
	*/
    // Discover available SDR devices
    std::map<std::string, std::string> args;
	std::cout << "SoapySDR::Devices called" << std::endl;
    std::vector<SoapySDR::Kwargs> results = SoapySDR::Device::enumerate(args);

    if (results.empty()) {
        std::cerr << "No SDR devices found." << std::endl;
        return 1;
    }

    std::cout << "Found " << results.size() << " SDR devices:" << std::endl;
	
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "  Device " << i << " parameters:" << std::endl;
        for (const auto& pair : results[i]) {
            std::cout << "    " << pair.first << " = " << pair.second << std::endl;
        }
    }

    // Open the first available device
	std::cout << "Open the first available device" << std::endl;
    SoapySDR::Device* sdr = SoapySDR::Device::make(results[0]);
    if (!sdr) {
        std::cerr << "Failed to open device." << std::endl;
        return 1;
    }

    std::cout << "\nDevice opened successfully!" << std::endl;

	const size_t channel = 0;
	const int direction = SOAPY_SDR_RX;  // For receive direction	
	const std::string format = SOAPY_SDR_CF32;
	// set up the stream
	SoapySDR::Stream * rxStream;
	try {
		rxStream = sdr->setupStream(
				direction, SOAPY_SDR_CF32, {0}, {} );
		std::cout << "RX stream set up successfully" << std::endl;
	} catch (const std::exception &e) {
		std::cerr << "Failed to set up stream: " << e.what() << std::endl;
		SoapySDR::Device::unmake(sdr);
		return (1);
	}
	int mtuSize = sdr->getStreamMTU(rxStream);
	std::cout << "MTU Size = " << mtuSize << std::endl;
	// setup buffer
    const size_t numSamples = 1024;
    std::vector<std::complex<float>> buff(numSamples);
    void *buffs[] = {buff.data()}; // Array of pointers for multi-channel support
		
    // Start continuous streaming (numElems = 0 for infinite, or a specific value for a burst)
    try {
        sdr->activateStream(rxStream, 0, 0, 0);
        std::cout << "Stream activated" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Failed to activate stream: " << e.what() << std::endl;
        sdr->closeStream(rxStream);
        SoapySDR::Device::unmake(sdr);
        return 1;
    }
	
	size_t numberOfReads = 100;
    for (int i = 0; i < numberOfReads; ++i) {
        int flags;
        long long timeNs;
		std::cout << "readStream called" << std::endl;
        int ret = sdr->readStream(rxStream, buffs, numSamples, flags, timeNs, 100000); // 100ms timeout
		std::cout << "readStream called" << std::endl;
        if (ret > 0) {
            std::cout << "Read " << ret << " samples. Time: " << timeNs << " ns" << std::endl;
            // Process samples in the 'buff' vector here
        } else if (ret == SOAPY_SDR_TIMEOUT) {
            std::cerr << "Timeout!" << std::endl;
        } else if (ret == SOAPY_SDR_OVERFLOW) {
            std::cerr << "Overflow!" << std::endl;
        } else if (ret == SOAPY_SDR_CORRUPTION) {
            std::cerr << "Corruption!" << std::endl;
        } else {
            std::cerr << "Error reading stream: " << ret << std::endl;
        }
    }

	sdr->deactivateStream(rxStream, 0, 0);
	sdr->closeStream(rxStream);
	std::cout << "Stream closed" << std::endl;
	
	SoapySDR::Device::unmake(sdr);
	std::cout << "Device released" << std::endl;
	/*
    // 6. Restore the original cerr buffer (important!)
    std::cerr.rdbuf(originalCerrBuffer);

    // 7. Close the log file
    logFile.close();

    std::cout << "SoapySDR log messages redirected to soapysdr_log.txt" << std::endl;
	*/
	return 0;
}