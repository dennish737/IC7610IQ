
//#include <windows.h>
#include <Lmcons.h>

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

// Platform-specific headers and definitions
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketType;
    #define CLOSE_SOCKET closesocket
    #define GET_LAST_ERROR() WSAGetLastError()
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    typedef int SocketType;
    #define CLOSE_SOCKET close
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
	#define GET_LAST_ERROR() errno
#endif

#include <windows.h>
#include "IcomIQPort.hpp"

#include "version.h"


namespace fs = std::filesystem;

//#define DEFAULT_PORT_STR "6789"
#define DEFAULT_PORT 6789
const std::string serialNum ="12001136";
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

void handleClient(SocketType clientSocket) {
    char buffer[IQ_BLOCK_LENGTH] = {0};
	int numSamples = 1024;
	int loop_count = 0;
	int total_samples = 0;
	
	// tbd add setting information
	int samples_read = 0;
	iqPort.iqAsyncStart(vfo);	
	std::vector<std::complex<short>> buf(numSamples);
	printf("Starting write loop \n");
	bool active = true;
	std::this_thread::sleep_for(std::chrono::microseconds(100));
	
	while(active && ((samples_read = iqPort.iqReadBuf( buf.data(), buf.size())) > 0) )
	{
		if ((loop_count % 10000) == 0){
			printf(" total samples = %d\n", total_samples);
			
		}
		//printf("return  iqPort.iqReadBuf(), samples_read = %d\n", samples_read);
		const char* charPtr = reinterpret_cast<const char*>(buf.data());
		//write_complex_data(buf, dataType); //tbd - support different format (currently short interleave

		int sent = send(clientSocket, charPtr, (samples_read * sample_size), 0);
        if (sent == SOCKET_ERROR) {
            // Handle specific errors
#ifdef _WIN32
            int error_code = WSAGetLastError();
            std::cerr << "send() error: " << error_code << std::endl;
            // Common errors: WSAECONNRESET (connection reset by peer), WSAENOTSOCK (invalid socket)
#else // POSIX
            std::cerr << "send() error: " << errno << " (" << strerror(errno) << ")" << std::endl;
            // Common errors: EPIPE (broken pipe), ECONNRESET (connection reset by peer)
#endif
			active = false;
		}

		total_samples += samples_read;
		loop_count++;
	}
	
	// socket is closed, shutdoen thread
	iqPort.iqAsyncStop();
	iqPort.iqClearReadBuf();
    CLOSE_SOCKET(clientSocket);
}

void usage(void)
{
	printf("IC7610TCPServer for the IC7610\n\n");
	printf("Usage:\tIC7610TCPServer.exe -f frequency [-g gain] fillepath \n");
	printf("\t[-f frequency to tune to [Hz]]\n");
	printf("\t[-g gain (default: 10 for auto)]\n");
	printf("\t[-i interval to sample in minutes (default 1)]\n");
	//printf("\t[-t data type S16, CS16, or CF32 - Default CS16\n");
	printf("\t[-v VFO main or sub - Default main\n");
	printf("\tport  - Default port %d \n", DEFAULT_PORT);
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
			/*
            case 't':{
                // optarg points to the argument for -t CS16
				if (strcmp(optarg,"S16") == 0) {
					dataType = "S16";
				} else if (strcmp(optarg, "CS16") == 0){
					dataType = "CS16";
				} else if (strcmp(optarg, "CF32") == 0) {
					dataType = "CF32";
				} else if (strcmp(optarg, "CF64") == 0) {
					dataType = "CF64";
				}
                std::cout << "Option -t specified with value : " << dataType << std::endl;
			break;}
			*/
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
	
	if (frequency == 0) {
		std::cout << "using current rafio frequency" << std::endl;
	} else {
		std::cout << "will set frequency = " << frequency << " Hz." << std::endl;
	}
	
	//std::cout << " dataType = " << dataType << std::endl;
	if (dataType.empty()) {
		//printf(" dataType = empty \n");
		dataType = "CS16";
		std::cout << "using CS16 data format" << std::endl;
	} else {
		std::cout << "using " << dataType << " for data format" << std::endl;
	}
	std::cout << ((vfo == mainVFO)? "main" : "sub") << std::endl;

	if (port == 0)
	{
		port = DEFAULT_PORT;
	}
 
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


	//buffer.reserve(WRBUFFER_SIZE); // Pre-allocate buffer capacity
	
    std::cout << "Server listening on port " << port << "..." << std::endl;
    // 1. Initialize Winsock on Windows

    // 2. Create Socket
    SocketType serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // 3. Bind and Listen
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);
    std::cout << "Server listening on port 8080..." << std::endl;
    // 4. Accept connections and create worker threads
	bool active = true;
    while (active) {
        SocketType client_socket;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        client_socket = accept(serverSocket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << GET_LAST_ERROR() << std::endl;
            continue;
        }

        // Create a detached worker thread to handle the client
        //std::thread (handleClient, client_socket,  vfo, frequency, gain).detach();
        std::thread client_thread(handleClient, client_socket);
        client_thread.detach(); // Detach the thread to run independently
		// Detach the thread to run independently
    }
	
    // Server main loop never exits in this basic example, so cleanup is theoretical
    closesocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;

}	
