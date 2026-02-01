

// g++ -I../include WriteToFile.cpp ../src/IcomIQPort.cpp -o ../test_bin/WriteToFile.exe -L ../libs -lftd3xx
//  ./WriteToFile.exe

#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <thread>
#include <atomic>
#include <fstream>
#include <string>
#include <complex>
//#include <libserialport.h>



#include <atomic>
#include <fstream>
#include <string>


#include "IcomIQPort.hpp"

const uint8_t mainVFO = 0x00;
const uint8_t subVFO = 0x01;

int pass_count = 0;
int fail_count = 0;
int total_count = 0;



size_t sample_size = 4; // bytes
size_t num_samples_per_buff = 1024; // samples per read block
size_t num_buffers = 16;  //number of buffers


const std::string& filename("./asciicomplexwrite.data");
std::ofstream ofs;
// Define a large buffer size
const size_t WRBUFFER_SIZE = 64 * 1024; // 64 KB

std::string buffer;
//buffer.reserve(WRBUFFER_SIZE); // Pre-allocate buffer capacity

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


	
void write_complex_data(const std::vector<std::complex<short>>& data)
{

    for (const auto& val : data) {
        // Use snprintf to format the complex number into a temporary char array
        char temp_str[64]; // Sufficiently large for two short values and formatting
        int len = snprintf(temp_str, sizeof(temp_str), "%hd %hd\n", val.real(), val.imag());
        
        // Append to string buffer
        if (len > 0) {
            if (buffer.size() + len >= WRBUFFER_SIZE) {
                // Buffer full, write to file
                ofs.write(buffer.data(), buffer.size());
                buffer.clear();
            }
            buffer.append(temp_str, len);
        }
    }
}

int main() {
    ofs.open(filename, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return(1);
    }
    // Disable synchronization with C standard streams and turn off automatic flushing for speed
    ofs.sync_with_stdio(false);
    ofs.tie(nullptr);
	
	printf("Enable IQ channels\n");
	std::vector<uint8_t> command = {0x1a, 0x0b}; // iq Enable 
	
    //sp_return status;
	
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

	int loop_count = 0;
	int numSamples = 3000;
	int total_samples = 0;
	auto start = std::chrono::high_resolution_clock::now();
	buffer.reserve(WRBUFFER_SIZE); // Pre-allocate buffer capacity
	iqPort.iqAsyncStart(mainVFO);
	while(loop_count < numSamples)
	{
		
		std::vector<std::complex<short>> buf(1024);
		//printf("calling iqPort.iqReadBuf() \n");
		int samples_read = iqPort.iqReadBuf( buf.data(), buf.size());
		//printf(" samples read = %d \n", samples_read);

		//printf("return  iqPort.iqReadBuf(), samples_read = %d\n", samples_read);
		write_complex_data(buf);
		total_samples += samples_read;
		loop_count++;
	}
	printf("loop complete\n");
	auto end = std::chrono::high_resolution_clock::now();
	iqPort.iqAsyncStop();
	int remaining = iqPort.iqGetSizeOfAvailableData();
	std::vector<std::complex<short>> buf(remaining);
	int samples_read = iqPort.iqReadBuf( buf.data(), buf.size());
	write_complex_data(buf);
	total_samples += samples_read;
	iqPort.iqClearReadBuf();
	ofs.flush();
	ofs.close();
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
	std::cout << "Execution time for " <<  numSamples << " Block reads( " << total_samples << " )samples: "
					<< duration.count() << " microseconds" << std::endl;
;
	
}	
