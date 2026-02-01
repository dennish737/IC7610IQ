/******************************************
 *
 * IcomCIVPort class include file
 * IcomCIVPort.hpp
 *****************************************/
#ifndef ICOMCIVPORT
#define ICOMCIVPORT

//#include <SoapySDR/Logger.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdint>
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <cstdio> // For FILE and fprintf
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include <libserialport.h> // Assumes you have the serial library set up

#include "version.h"
#include "common.h"



// Define the target Vendor ID and Product ID
#define TARGET_VENDOR_ID 0x10C4 // Replace with your device's Vendor ID, e.g., 0x10C4
#define TARGET_PRODUCT_ID 0xEA60 // Replace with your device's Product ID, e.g., 0xEA60

class IcomCIVPort {
private:
	std::string version_ = VERSION_ICOMCIV_PORT;
	struct sp_port *_civ_port;
	int _baudrate;
	bool _isInitialized = false;
	bool _isOpen = false;
	std::vector<uint8_t> buffer[CIV_READ_BUFFER_SIZE];
	
	void check_sp_result(enum sp_return result, const std::string& message);
public:
	IcomCIVPort (void);
	std::string version() { return version_; };
	std::string findIcomCIVPort();
	void init (std::string port_name, int baudrate = defaultBaudrate);
	~IcomCIVPort(void);
	bool isOpen(void);
	bool isInitialized(void);
	static std::string findIcomCtrlPort();
	int sendCIVCmd( std::vector<uint8_t> cmd);
	int readCIVReply( std::vector<uint8_t>& buffer);	
	int icomCIVCommand( std::vector<uint8_t> cmd, std::vector<uint8_t> &reply);
	// convert an int to bcd
	int read_bcd(int n) { return (10 * ((n & 0xf0) >> 4) + (n & 0x0f));}
	// convert a uint32_ to bcd digit
    int bcd_digit(uint32_t n, int value){return (n / value) % 10;}
	// convert uint32_t to bcddigits
	int bcd_digits(uint32_t n, int value){return 0x10 * bcd_digit(n, 10 * value) | bcd_digit(n, value);}

};
#endif
