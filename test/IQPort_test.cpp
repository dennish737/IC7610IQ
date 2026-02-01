#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>
#include <complex>

#include <libserialport.h>

#include "IcomIQPort.hpp"

const uint8_t mainVFO = 0x00;
const uint8_t subVFO = 0x01;

int pass_count = 0;
int fail_count = 0;
int total_count = 0;



size_t sample_size = 4; // bytes
size_t num_samples_per_buff = 1024; // samples per read block
size_t num_buffers = 16;  //number of buffers


void test_result( int expected, int results, const char* message)
{
	if (expected == results)
	{
		pass_count +=1;
		printf(message, results, "passed");
	}
	else 
	{ 
		fail_count +=1;
		printf(message, results, "failed");	
	}
}

void print_vector(std::vector<uint8_t> v)
{
	for (int i = 0; i < v.size(); i++) 
	{
		printf("%02X ", v[i]);
	}
	printf("\n");	
}

int main() {
	printf("Enable IQ channels\n");
	std::vector<uint8_t> command = {0x1a, 0x0b}; // iq Enable 
	
    sp_return status;
	
	printf("Find, Connect and Activate Highspeed USB Port : ");
	std::string deviceSerialNum = IcomIQPort::getDeviceSerialNum();
	IcomIQPort iqPort;
	iqPort.init(deviceSerialNum) ;
	if (iqPort.isOpen())
	{
		printf("iqPort is open \n");
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = iqPort.icomIQCommand(command, reply);

		if (received_bytes > 0) {
			printf(" pass. result = %d\n", received_bytes);
			//print_vector(reply);
		} else
		{
			printf(" failed. \n");
			printf("test aborted \n");
			exit(1);
		}
	} else {
		printf("Port Not Open - abort\n");
		exit(2);
	}
	total_count += 1;
	pass_count += 1;
	// get chip and and device seascriptor
	std::string chipDescriptor = iqPort.iqGetChipConfiguration();
	std::cout << "--Chip Description: ";
	std::cout << chipDescriptor << std::endl;
	std::string deviceDescription = iqPort.iqGetDevicveDescriptor();
	std::cout << "--Device Descriptor: ";
	std::cout << deviceDescription << std::endl;
	
	std::cout << "RF Gain:     " << iqPort.iqGetRFGain(mainVFO) << std::endl;
	std::cout << "Pre Amp:     " << iqPort.iqGetPreAmpStatus(mainVFO) << std::endl;
	std::cout << "Atentuation: " << iqPort.iqGetAttenuatorSettings(mainVFO) << std::endl;;
	std::cout << "Antenna:     " << iqPort.iqGetAntenna(mainVFO) << std::endl;;
	std::cout << "Frequency:   " << iqPort.iqGetFrequency(mainVFO) << std::endl;
	std::cout << "DIGI_SEL:    " << (iqPort.iqGetDIGI_SEL_Status(mainVFO)? "TRUE" : "FALSE") << std::endl;
	std::cout << "IPPLUS:      " << (iqPort.iqGetIP_Status(mainVFO)? "TRUE" : "FALSE") << std::endl;

	
	int freq = iqPort.iqSetFrequency(mainVFO, 1380000);
	test_result(1380000, freq,"Set Main VFO Frequency : %d -> %s\n");
	total_count += 1;
	printf("Set timeout to 10 Seconds\n");
	int timeout = 5000; // timeout in ms -> 5 seconds
	iqPort.setTimeout(IQ_IN, timeout); 
	
	// enable iq port and read data
	std::vector<std::complex<short>> buffer(1024);
	iqPort.enableIQData(mainVFO);
	for (int i = 0; i < 100; i++)
	{
		int r = iqPort.readIQData(buffer.data(), buffer.size());
		if ( r > 0) {
			 std::cout << "readIQdata samples read = " << r << " , samples requested = " << buffer.size() << 
						((r == buffer.size()) ? " Passed." : " Failed.") << std::endl;
		} else {
			std::cout << "readIQData failed: " << r << std::endl;
			break;
		}
	}
	return (0);
						
}