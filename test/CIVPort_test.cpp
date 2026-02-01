#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <thread>
#include <libserialport.h>

#include "IcomCIVPort.hpp"


/**
 * @brief Reads an Icom CI-V command response from the serial port.
 * 
 * @param port A pointer to an open sp_port structure.
 * @return A vector of bytes containing the full CI-V response packet (including preamble and EOM), 
 *         or an empty vector if an error occurs or no valid response is received within timeout.
 */

// Example usage:
int main() {
	std::vector <uint8_t> command = { 0x03}; 
	
    sp_return status;
	std::string port_name = IcomCIVPort::findIcomCtrlPort();
    //const char* port_name = "COM4"; // Change to your serial port name (e.g., "/dev/ttyUSB0" on Linux/MSYS2)
	IcomCIVPort port ;
	port.init(port_name);
	fprintf(stdout, "Test if the port is open\n");
	if (port.isOpen()){
		fprintf(stdout, "port is open\n");
		std::vector<uint8_t> reply;
		int received_bytes = port.icomCIVCommand(command, reply);

		if (received_bytes > 0) {
			printf("Received %d bytes: ", received_bytes);
			for (int i = 0; i < received_bytes; i++) {
				printf("%02X ", reply[i]);
			}
			printf("\n");
		}
	}

	
    return 0;
}
