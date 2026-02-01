



// g++ -I../include IC7610zmqPush.cpp ../src/IcomIQPort.cpp -o IC7610zmqPush.exe -L ../libs -lftd3xx -lws2_32 -lzmq
//  ./IC7610TCPWritter.exe


#include <zmq.hpp>
#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <thread>
#include <atomic>
#include <fstream>
#include <string>
#include <complex>
#include <cstring>
#include <cerrno>



#include <unistd.h>
#include <cstdlib> // For exit()
#include <filesystem>
#include <iomanip>

#include <locale>
#include <codecvt> // Required for std::codecvt_utf8



//#include <windows.h>
#include "IcomIQPort.hpp"

#include "version.h"


const uint8_t mainVFO = 0x00;
const uint8_t subVFO = 0x01;

size_t sample_size = 4; // bytes
size_t num_samples_per_buff = 1024; // samples per read block
size_t num_buffers = 16;  //number of buffers

int gain  = INT_MIN;
int frequency = 0;
int port =0;
std::string dataType;
uint8_t vfo = mainVFO;
IcomIQPort iqPort;
std::string buffer;

std::string getDate() {
	std::time_t now = std::time(nullptr);
    std::tm* tm_struct = std::localtime(&now);
	
    char buffer[32]; // Buffer to hold the formatted time string
    // Format the time and store it in the buffer
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm_struct);

    std::string formatted_date(buffer);
    return formatted_date;	
}

std::string getDateTime() {
	std::time_t now = std::time(nullptr);
    std::tm* tm_struct = std::localtime(&now);
	
    char buffer[80]; // Buffer to hold the formatted time string
    // Format the time and store it in the buffer
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_struct);

    std::string formatted_time(buffer);
    return formatted_time;	
}

bool isLittleEndian() {
    // A 16-bit integer with a distinct byte pattern
    short temp = 0x1234; 
    // A char pointer pointing to the first byte of the integer
    char* tempChar = reinterpret_cast<char*>(&temp);

    // If the first byte (lowest address) holds the least significant byte (0x34),
    // the system is little-endian. Otherwise, it is big-endian (0x12).
    if (tempChar[0] == 0x34) {
        return true; // Little-Endian
    } else {
        return false; // Big-Endian
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

/*	
void write_complex_data(const std::vector<std::complex<short>>& data, std::string format)
{
	const float norm = 32767.0f;
	
	if (format == "S16") {
		for (const auto& val : data) {
			const char* dataPtr = reinterpret_cast<const char*>(&val);
			size_t len = sizeof(std::complex<short>);
			if (len > 0) {
				if (buffer.size() + len >= WRBUFFER_SIZE) {
					// Buffer full, write to file
					ofs.write(buffer.data(), buffer.size());
					buffer.clear();
				}				
				buffer.append( dataPtr, len);
			}
		}
	} else if (format == "CS16") {		
		for (const auto& val : data) {
			const char* dataPtr = reinterpret_cast<const char*>(&val);
			size_t len = sizeof(std::complex<short>);
			if (len > 0) {
				if (buffer.size() + len >= WRBUFFER_SIZE) {
					// Buffer full, write to file
					ofs.write(buffer.data(), buffer.size());
					buffer.clear();
				}				
				buffer.append( dataPtr, len);
			}
		}
	} else if (format == "CF32"	){
		int for_count =0;
		for (const auto& val : data) {

			std::complex<float> c_float(
					static_cast<float>(val.real())/norm,
					static_cast<float>(val.imag())/norm
					);
					
			float magnitude = std::abs(c_float);
			if (magnitude > 2.0) {
				std::cout << "val real = " << val.real() << " ,val imag = " << val.imag() << std::endl;
				std::cout << "c_float real = " << c_float.real() << " ,c_float imag = " << c_float.imag() << std::endl;
				std::cout << "magnitude = " << magnitude << std::endl;
			}
			
			const char* dataPtr = reinterpret_cast<const char*>(&c_float);
			size_t len = sizeof(std::complex<float>);
			if (len > 0) {
				if (buffer.size() + len >= WRBUFFER_SIZE) {
					// Buffer full, write to file
					ofs.write(buffer.data(), buffer.size());
					buffer.clear();
				}				
				buffer.append( dataPtr, len);	
			}
		}
	} else if (format == "CF64"	){
		for (const auto& val : data) {
			//std::cout << "real = " << val.real() << " ,imag = " << val.imag() << std::endl;
			std::complex<double> c_double(
					static_cast<double>(val.real())/norm,
					static_cast<double>(val.imag())/norm
					);
			//std::cout << "real = " << re << " ,imag = " << im << std::endl;
			const char* dataPtr = reinterpret_cast<const char*>(&c_double);
			size_t len = sizeof(std::complex<double>);
			if (len > 0) {
				if (buffer.size() + len >= WRBUFFER_SIZE) {
					// Buffer full, write to file
					ofs.write(buffer.data(), buffer.size());
					buffer.clear();
				}				
				buffer.append( dataPtr, len);	
			}
		}
	}
			
}
*/


void usage(void)
{
	printf("IC7610zmqPush.exe for the IC7610\n\n");
	printf("Usage:\t./IC7610zmqPush.exe -f frequency [-g gain] [-d <type>] [-v vfo \n");
	printf("\t[-f frequency to tune to [Hz]]\n");
	printf("\t[-g gain (default: 10 for auto)]\n");

	printf("\t[-t data type S16, CS16, or CF32 - Default CS16\n");
	printf("\t[-v VFO main or sub - Default main\n");
	exit(1);
}


int main(int argc, char *argv[]) {
	
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;
#endif

    int c;

	int interval = 1;	//sample interval in minutes
    const char *options = ":f:g:t:v:h";
	
    while ((c = getopt(argc, argv, options)) != -1) {
        switch (c) {

            case 'f':{
                // optarg points to the argument for -f
				frequency = (uint32_t)atof(optarg);
                std::cout << "Option -f specified with value: " << frequency << std::endl;
			break;}
            case 'g':{
                // optarg points to the argument for -g
				gain = (int)(atof(optarg) * 10);
                std::cout << "Option -g specified with value : " << gain << " in dB/10"<< std::endl;
			break; }

            case 't':{
                // optarg points to the argument for -t CS16
				if (strcmp(optarg, "CS16") == 0){
					dataType = "CS16";
				} else if (strcmp(optarg, "CF32") == 0) {
					dataType = "CF32";
				} else if (strcmp(optarg, "CF64") == 0) {
					dataType = "CF64";
				} else {
					dataType = "CS16";
				}
                std::cout << "Option -t specified with value : " << dataType << std::endl;
			break;}
            case 'v': {
                // optarg points to the argument for -v
				std::string vfoName = optarg;
				if (vfoName == "main" || vfoName == "Main" || vfoName == "MAIN"){
					vfo = mainVFO;
				} else if ( vfoName == "sub" || vfoName == "Sub" || vfoName == "SUB") {
					vfo = subVFO;
				} else {
					vfo = mainVFO;
				}
                std::cout << "Option -v specified with value : " << vfoName << std::endl;
			break; }
            case 'h': {
                usage();
                // You might print usage info and exit here
			break; }
            case ':': // Missing argument case (due to leading ':' in optstring)
                std::cerr << "Option -" << static_cast<char>(optopt) << " requires an argument\n";
				usage();
                break;
            case '?': {// Unrecognized option case
                std::cerr << "Unrecognized option: - " << static_cast<char>(optopt) << std::endl;
				usage();
			break; }
            case -1: {// End of options (handled by the while loop condition)
				break; }
            default: {
                std::cerr << "Error during option parsing\n";
				usage();
				break;}
        }
    }

	/*
    //Process non-option arguments (operands)
	if (optind == argc) {
		// missing Port notify and cntinue
		std::cout << "port was not define, using default: "<< DEFAULT_PORT << std::endl;
		port = DEFAULT_PORT;
	} else if ((argc - optind) > 1) {	
		std::cout << "only a single output port is allowed!" << std::endl;
		usage();		
	} else {
		port = (int)(atof(optarg));
		std::cout << "Port: " << port << "\n";
	}
	*/
	
	if (frequency == 0) {
		std::cout << "using current rafio frequency" << std::endl;
	} else {
		std::cout << "will set frequency = " << frequency << " Hz." << std::endl;
	}
	
	std::cout << " dataType = " << dataType << std::endl;
	if (dataType.empty()) {
		printf(" dataType = empty \n");
		dataType = "CS16";
		std::cout << "using CS16 data format" << std::endl;
	} else {
		std::cout << "using " << dataType << " for data format" << std::endl;
	}
	std::cout << ((vfo == mainVFO)? "main" : "sub") << std::endl;


 
	printf("Open IQ Port\n");
	std::vector<uint8_t> command = {0x1a, 0x0b}; // iq Enable 
		

	printf("Find, Connect and Activate Highspeed USB Port : ");
	std::string deviceSerialNum = IcomIQPort::getDeviceSerialNum();
	
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
	
	// get radio parameters
	if (frequency > 0) {	
		int freq = iqPort.iqSetFrequency(vfo, frequency);		
	} else {
		frequency;
	}
	// set gain 
	if (gain == INT_MIN) {
		std::cout << "using current radio gain (10.0 dB)" << std::endl;
		gain = iqPort.iqGetRFGain(vfo);
	} else {
		std::cout << "will set radio gain to " << ((float)gain/10.0) << " dB" <<std::endl;
		// tbd db to gain converter
		gain = iqPort.iqGetRFGain(vfo);
	}	
	
	printf("Set timeout to 10 Seconds\n");
	int timeout = 5000; // timeout in ms -> 5 seconds
	iqPort.setTimeout(IQ_IN, timeout); 

	int loop_count = 0;
	int numSamples = 1024;
	int total_samples = 0;
	
	// calculate  buffer size Each call returns 1024 ca16 samples requiring 4096 bytes (1024*4). For CF32, we need 1024* 8 = 8192 bytes
	// to hold 1024 samples. Finally CF64, we require 1024* (sizeof(double) + sizeof(double) to hold 1024 samples.
	int multiplier = sample_size;
	if (dataType == "CF32") 
	{ multiplier = sample_size * 2 ; } 
	else if (dataType == "CF64") 
	{ multiplier = sample_size * 4 ; };

	//size_t buffer_size = 1024 * multiple;
	
	//buffer.reserve(buffer_size); // Pre-allocate buffer capacity for send
    // 1. ZMQ Setup
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PUSH);
    socket.bind("tcp://*:5555"); // Bind for GNURadio to connect
	
	// start the async reader	
	iqPort.iqAsyncStart(vfo);
	std::vector<std::complex<short>> buf(numSamples); // buffer to read into
	int samples_read = 0;
	while((samples_read = iqPort.iqReadBuf( buf.data(), buf.size())) > 0) 
	{
		
		zmq::message_t message(buf.size() * multiplier);
		memcpy(message.data(), buf.data(), buf.size() * multiplier);
		socket.send(message, zmq::send_flags::none);
		if ((loop_count % 10000) == 0){
			printf(" total samples = %d\n", total_samples);
		}

		// tbd integrate write_complex to allow for different types
		//write_complex_data(buf, dataType);
		total_samples += samples_read;
		loop_count++;
	}	

    return 0;

}	
