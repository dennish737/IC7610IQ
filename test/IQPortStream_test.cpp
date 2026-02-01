

// g++ -I../include IQPortStream_test.cpp ../src/IcomIQPort.cpp -o IQPortStreamTest.exe -L ../libs -lftd3xx
//  ./IQPortStreamTest.exe

#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>
#include <complex>
//#include <libserialport.h>

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

	int freq = iqPort.iqSetFrequency(mainVFO, 1380000);
	test_result(1380000, freq,"Set Sub VFO Frequency : %d -> %s\n");
	total_count += 1;
	printf("Set timeout to 10 Seconds\n");
	int timeout = 5000; // timeout in ms -> 5 seconds
	iqPort.setTimeout(IQ_IN, timeout); 
	// setup buffers
	
	//_rx_stream.allocate_buffers();
	uint32_t total_samples = 0;
	int loop_count = 0;
	//Set up async and buffer
	printf(" Start Async  \n");
	// values for vfo are 0 off, 1 main, 2 for sub. Note everywhere else main is 0 and sub is one
	iqPort.iqAsyncStart(mainVFO);
	//std::this_thread::sleep_for(std::chrono::microseconds(100));
	/******************************************************************/
	auto start = std::chrono::high_resolution_clock::now();

	while(loop_count < 1000)
	{
		
		std::vector<std::complex<short>> buf(512);
		//printf("calling iqPort.iqReadBuf() \n");
		int samples_read = iqPort.iqReadBuf( buf.data(), buf.size());
		//printf(" samples read = %d \n", samples_read);

		//printf("return  iqPort.iqReadBuf(), samples_read = %d\n", samples_read);
		total_samples += samples_read;
		loop_count++;
	}
	auto end = std::chrono::high_resolution_clock::now();
	iqPort.iqAsyncStop();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	/******************************************************/
	double sample_freq = 1920000.0;
	std::cout << "Execution time for 1000 reads: " << duration.count() << " microseconds" << std::endl;
	std::cout << "Total Samples: " << total_samples << std::endl;
	std::cout << "Total Bytes: " << (total_samples * 4) << std::endl;
	std::cout << "Sample rate: " << sample_freq<< " samples per second." << std::endl;
	std::cout << "Sample interval: " << (1.0/sample_freq) << " second per sample." << std::endl;
	std::cout << "Estimated block time: " << ((1.0/sample_freq)*1024) << " seconds." << std::endl;
	std::cout << "Estimated time: " <<((1.0/sample_freq)* total_samples) << " seconds" << std::endl;
	std::cout << "Execution time for 1000 Block reads( " << total_samples << " )samples: "
					<< duration.count() << " microseconds" << std::endl;
;
	
}	
	
	
