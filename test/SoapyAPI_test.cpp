
// g++ SoapyAPI_test.cpp -o SoapyAPI_test.exe -lSoapySDR
// ./SoapyAPI_test.exe


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
    std::streambuf* originalCerrBuffer = std::cerr.rdbuf();

    // 2. Open a file for logging
    std::ofstream logFile("soapysdr_log.txt");

    // 3. Check if the file opened successfully
    if (!logFile.is_open()) {
        std::cerr << "Error: Could not open log file." << std::endl;
        return 1;
    }
	
	// 4. Redirect std::cerr to the log file
    std::cerr.rdbuf(logFile.rdbuf());
	
	//SoapySDR::setLogLevel(SOAPY_SDR_INFO);
	
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
    SoapySDR::Device* device = SoapySDR::Device::make(results[0]);
    if (!device) {
        std::cerr << "Failed to open device." << std::endl;
        return 1;
    }

    std::cout << "\nDevice opened successfully!" << std::endl;

	const size_t channel = 0;
	const int direction = SOAPY_SDR_RX;  // For receive direction	
	
	// get list of antenn configs
	std::vector<std::string> antennas = device->listAntennas(direction, channel);
	for (auto antenna : antennas) {
		std::cout << " Antenna: " << antenna << std::endl;
	}	
	std::cout << "Calling getAntenna()" << std::endl;
	std::string antenna_str = device->getAntenna(direction, channel);
	std::cout << antenna_str << std::endl;
		// getFrequency
	double frequency;
	std::cout << "Calling device->getFrequency(direction, channel)" << std::endl;
	frequency = device->getFrequency(direction, channel);
	std::cout << "Frequency: "  << frequency << std::endl;
	
		// getFrequency
	std::cout << "Calling device->getFrequency(direction, channel, RF)" << std::endl;
	frequency = device->getFrequency(direction, channel, "RF");
	std::cout << "Frequency: "  << frequency << std::endl;
	
	std::vector<std::string> formats = device->getStreamFormats(direction, channel);
	for (auto format : formats) {
		std::cout << " Format: " << format << std::endl;
	}
	
	double fullscale;
	std::string nativeFormat =  device->getNativeStreamFormat(direction, channel, fullscale);
	std::cout << " Native Format: " << nativeFormat << " , fullscale = " << fullscale << std::endl;
	
	SoapySDR::ArgInfoList stream_args = device->getStreamArgsInfo(direction, channel);
	for (auto arg : stream_args ) {
		std::cout << "key:" << arg.key << std::endl;
		std::cout << "Name: " << arg.name << std::endl;
		std::cout << "Description: " << arg.description << std::endl;
		std::cout << "Value: " << arg.value << std::endl;
		std::cout << "-----------------" << std::endl;	
	};	
	    // Release the device
    SoapySDR::Device::unmake(device);
    std::cout << "Device released." << std::endl;

    // 6. Restore the original cerr buffer (important!)
    std::cerr.rdbuf(originalCerrBuffer);

    // 7. Close the log file
    logFile.close();

    std::cout << "SoapySDR log messages redirected to soapysdr_log.txt" << std::endl;


    return 0;
}