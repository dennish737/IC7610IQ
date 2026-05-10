

// g++ -I../include SigMFFileWriter.cpp ../src/IcomIQPort.cpp -o SigMFFileWriter.exe  -lspdlog -lfmt -lboost_filesystem-mt -lws2_32 -L ../libs -lftd3xx
// g++ -I../include -DDEBUG SigMFFileWriter.cpp ../src/IcomIQPort.cpp -o SigMFFileWriter.exe  -lspdlog -lfmt -lboost_filesystem-mt -lws2_32 -L ../libs -lftd3xx
//  ./SigMFFileWriter.exe



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
#include "nlohmann/json.hpp"

#include <unistd.h>
#include <cstdlib> // For exit()
#include <filesystem>
#include <iomanip>

#include <locale>
#include <codecvt> // Required for std::codecvt_utf8

// logging functions
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>// Required for color console logging
#include <spdlog/fmt/ranges.h> // Required for vector support
#include <spdlog/fmt/bin_to_hex.h> // format vector of bytes to hex values

#include "IcomIQPort.hpp"
#include "IcomIQStream.hpp"

#include <windows.h>
#include "version.h"



namespace fs = std::filesystem;

#define DEFAULT_PORT_STR "1234"

//const uint8_t mainVFO = 0x00;
//const uint8_t subVFO = 0x01;

int pass_count = 0;
int fail_count = 0;
int total_count = 0;

std::shared_ptr<spdlog::logger> consoleLog;

/*
size_t sample_size = 4; // bytes
size_t num_samples_per_buff = 1024; // samples per read block
size_t num_buffers = 16;  //number of buffers
*/

using json = nlohmann::json;

struct capture_params {
	std::string startTime;
	int frequency;	
	int RFGain;
	std::string Antenna;			// ANT1, ANT2, RX
	int PreampStatus;
	uint8_t AttenuatorSettings;
	std::string DIGI_SEL;			// On or Off
	std::string IPPlus;				// On or Off
	int duration;					// duration in minutes
	size_t totalSamples;
	float executionTime;
};
	
	void to_json(json& j, const capture_params& params) {
		j = json{{"start_time", params.startTime}, {"frequency", params.frequency}, 
			{"rf_gain", params.RFGain},
			{"antenna", params.Antenna},
			{"preamp", params.PreampStatus},
			{"attenuator", params.AttenuatorSettings},
			{"DIGI_SEL", params.DIGI_SEL},
			{"IPPlus", params.IPPlus},
			{"duration", params.duration},
			{"total_samples", params.totalSamples},
			{"execution_time", params.executionTime}};			
};
	

struct IC7610Parameters {
	std::string date;
	std::string datatype;
	int	sample_rate;
	std::string hw;
	std::string	VFO;				// main or sub

	std::string author;
	std::string software;
	std::string version;
	std::string dataFile;
	std::vector<capture_params> captures; 
};
	
	// Define the to_json function for the outer struct
	void to_json(json& j, const IC7610Parameters& ic7610) {
    j = json{{"date", ic7610.date},
			{"datatype", ic7610.datatype}, 
			{"sample_rate", ic7610.sample_rate}, 
			{"hw", ic7610.hw},
			{"vfo", ic7610.VFO},

			{"author", ic7610.author},
			{"software", ic7610.software},
			{"version", ic7610.version},
			{"datafile", ic7610.dataFile},
			{"capatures", ic7610.captures}		
			};
};

IC7610Parameters metaParameters;

// Function to convert wstring to string (encoded in UTF-8)
std::string wstring_to_utf8(const std::wstring& wstr) {
    // Setup converter
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    // Use the converter to_bytes function
    return converter.to_bytes(wstr);
}

std::string getUsername() {
    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (::GetUserNameW(username, &username_len)) {
        // Construct std::wstring from buffer and its actual length
		std::wstring name = std::wstring(username, username_len - 1);
		
        return wstring_to_utf8( name); 
    }
    return "Unknown"; // Return "Unknown" or handle the error as needed
}

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



std::string getMetaFileName(std::string fileName)
{
	fs::path f = fileName;
	std::string new_extension = ".sigmf-meta";
	f.replace_extension(new_extension);
	return f.string();
}

void print_vector(std::vector<uint8_t> v)
{
	for (int i = 0; i < v.size(); i++) 
	{
		printf("%02X ", v[i]);
	}
	printf("\n");	
}

void getMetaData( std::string datatype, int	sample_rate, uint8_t vfo, std::string fileName){
	fs::path p = fileName;
	fs::path filename_with_ext = p.filename();
	metaParameters.date = getDate();
	metaParameters.datatype = datatype + ((isLittleEndian)? " le" : " be");
	metaParameters.sample_rate = sample_rate;
	metaParameters.hw = "Icom IC7610";
	metaParameters.VFO = (vfo)? "sub" : "main";
	metaParameters.author = getUsername();
	metaParameters.software  = "SigMF_IC7610_Writter";
	metaParameters.version = VERSION_SIGMFFILEWRITTER;
	metaParameters.dataFile = filename_with_ext.string();
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
	printf("SigMFFileWrite for the IC7610. Version %s\n\n", VERSION_SIGMFFILEWRITTER);
	printf("Usage:\tSigMFFileWrite.exe -f frequency [-g gain] fillepath \n");
	printf("\t[-f frequency to tune to [Hz]]\n");
	printf("\t[-g gain (default: 10 for auto)]\n");
	printf("\t[-i interval to sample in minutes (default 1)]\n");
	printf("\t[-t data type S16, CS16, or CF32 - Default CS16\n");
	printf("\t[-v VFO main or sub - Default main\n");
	printf("\tfilepath  - required\n");

	exit(1);
}

/*
void writeMetaData(std::string fileName) 
{
	
}
*/

/**
 * @brief Initializes and returns a console logger.
 * @param name The unique name for the logger.
 * @return std::shared_ptr<spdlog::logger> Pointer to the created logger.
 */
std::shared_ptr<spdlog::logger> getlogger(const std::string& name = " main_logger") {
    // Check if the logger already exists in the global registry to avoid duplicates
    auto main_logger= spdlog::get(name);
    
    if (!main_logger) {
        // 1. Create a color multi-threaded console logger named "mainconsoleLog"
        main_logger = spdlog::stdout_color_mt(name);
		//spdlog::register_logger(main_logger);

        // 2. Set a custom pattern (e.g., [Timestamp] [LoggerName] [Level] Message)
       main_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

#ifdef DEBUG
        // 3. Set the log level to debug for this specific logger
        // Note: You must also ensure SPDLOG_ACTIVE_LEVEL is set if using macros, if DEBUG define
		main_logger->info("Setting logger to debug mode. Logger name = {}", "main_logger");
        main_logger->set_level(spdlog::level::debug);
#endif

        // 4. Set this as the default logger so spdlog::info/debug calls use it
        spdlog::set_default_logger(main_logger);
		
    }
    return main_logger;
}

int main(int argc, char *argv[]) {
    int c;
	int gain  = INT_MIN;
	int frequency = 0;
	std::string filename;
	std::string dataType;
	uint8_t vfo = mainVFO;
	int interval = 1;	//sample interval in minutes
    const char *options = ":f:g:t:v:h";

	 // 1. Create a thread-safe color console logger
    // Use _mt surffix for multi-threaded safety
    consoleLog = getlogger();

    consoleLog->info("create boost asio context");
    asio::io_context ctx;


	consoleLog->info("Process Options");
	
    while ((c = getopt(argc, argv, options)) != -1) {
        switch (c) {

            case 'f':{
                // optarg points to the argument for -f
				frequency = (uint32_t)atof(optarg);
				consoleLog->debug("Option -f (frequency) specified with value: {}", frequency);
			break;}
            case 'g':{
                // optarg points to the argument for -f
				gain = (int)(atof(optarg) * 10);
				consoleLog->debug("Option -g (gain) specified with value: {}", gain);
			break; }
            case 'i':{
				
                // optarg points to the argument for -f
				interval = (int)(atof(optarg));
				consoleLog->debug("Option -i (interval) specified with value: {} min, {} secinds", interval, (interval*60));
			break; }
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
				consoleLog->debug("Option -t (dataTpye) specified with value: {}", dataType);
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
				consoleLog->debug("Option -v (vfo) specified with value: {}", vfoName);
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
		// missing file name
		consoleLog->error("output file path required!");
		usage();
	} else if ((argc - optind) > 1) {	
		consoleLog->error("only one ouput file path is allowed!");
		usage();		
	} else {
		filename = argv[optind];
		consoleLog->debug("Output file path: {}:",filename);
	}
	
	if (frequency == 0) {
		consoleLog->debug("Using current radio frequency");
	} else {
		consoleLog->debug("will set frequency to: {} Hz", frequency);
	}
	

	if (dataType.empty()) {
		dataType = "CS16";
		consoleLog->debug("Datatype not defined using default : {}",dataType);
	} else {
		consoleLog->debug("Using dataType: {}", dataType);
	}

	consoleLog->debug("vfo: {}", ((vfo == mainVFO)? "main" : "sub"));

	// end of option proce3ssing


	std::vector<uint8_t> command = {0x1a, 0x0b}; // iq Enable 	
	
	consoleLog->info("Find, Connect and Activate Highspeed USB Port : ");
	std::string deviceSerialNum = IcomIQPort::getDeviceSerialNum();
	consoleLog->info("USB Device Serial Number: {}", deviceSerialNum);
    consoleLog->info("Init ftdi to disk  output: {}", filename);
    // 1. Open device (Example using index 0)

    // 2. Start the async manager
    consoleLog->info("Init ftdi to disk  output: {}", filename);
	auto writer = std::make_shared<IcomIQPortStream>(ctx, deviceSerialNum, filename);

    consoleLog->info("Start vfo: {}", ((vfo == mainVFO)?"mainVFO" : "subVFO"));



	
	// get radio parameters
	if (frequency > 0) {	
		int freq = writer->iqSetFrequency(vfo, frequency);		
	} else {
		frequency = writer->iqGetFrequency(vfo);
	}
	// set gain 
	if (gain == INT_MIN) {
		gain = writer->iqGetRFGain(vfo);
		SPDLOG_LOGGER_DEBUG(consoleLog, "Icom7610stream using current gain {} db", ((float)gain/10.0)  );
	} else {
		SPDLOG_LOGGER_DEBUG(consoleLog, "Icom7610stream setting gain to {}} db", ((float)gain/10.0)  );
		// tbd db to gain converter
		gain = writer->iqGetRFGain(vfo);
	}	
	consoleLog->info( "Set Icom7610stream port timeout to 10 sec");
	int timeout = 10000; // timeout in ms -> 5 seconds
	writer->setTimeout(IQ_IN, timeout); 

	int loop_count = 0;
	int numSamples = 1024;
	int total_samples = 0;
	//buffer.reserve(WRBUFFER_SIZE); // Pre-allocate buffer capacity
	consoleLog->info(" getting meta data");
	getMetaData( dataType, 1920000, vfo, filename);
	// Note we want to allow multiple capture n the future, currently only a single capture is allowed 
	// capture parameters
	consoleLog->info("get capture parameters");
	capture_params cap_data;
	cap_data.frequency = writer->iqGetFrequency(vfo);
	cap_data.RFGain = writer->iqGetRFGain(vfo);
	cap_data.Antenna = "RX";
	cap_data.PreampStatus = writer->iqGetPreAmpStatus(vfo);
	cap_data.AttenuatorSettings = writer->iqGetAttenuatorSettings(vfo);
	cap_data.DIGI_SEL = writer->iqGetDIGI_SEL_Status(vfo)? "On" : "Off";
	cap_data.IPPlus = writer->iqGetIP_Status(vfo)? "On" : "Off";
	cap_data.duration = interval;
	cap_data.startTime = getDateTime();
	
	const std::chrono::minutes duration(interval);
	auto start_time = std::chrono::steady_clock::now();
	auto start = std::chrono::high_resolution_clock::now();


	// start the pipe:
	consoleLog->info("Starting ReadPipe");
    writer->start(vfo);

    // 3. Run for 30 seconds
    //ctx.run_for(std::chrono::seconds(300));
	unsigned int waitTime = interval*60;
	consoleLog->info("waiting for {} seconds.", waitTime);
    ctx.run_for(std::chrono::seconds(interval*60));
    consoleLog->info("Total Bytes read = {}", writer->getReadTotal());
    consoleLog->info("Total Bytes written = {}", writer->getWriteTotal());
	
	cap_data.totalSamples = writer->getReadTotal()/4;
	cap_data.executionTime = interval * 60;
	
	// add the capture dataset to the metaParameters.
	metaParameters.captures.push_back(cap_data);
	
	json j_object = metaParameters;
	
	std::string metaFile = getMetaFileName(filename);
	std::ofstream output_file(metaFile);
    if (output_file.is_open()) {
        output_file << std::setw(4) << j_object << std::endl; // std::setw(4) adds indentation for pretty printing
        output_file.close();
        std::cout << "Successfully wrote JSON to order_data.json" << std::endl;
    } else {
        std::cerr << "Error opening file for writing!" << std::endl;
    }	
	std::cout << j_object.dump(4) << std::endl;
	/******************************************************/
	
	//double sample_freq = 1920000.0;
	//std::cout << "Execution time for 1000 reads: " << duration.count() << " microseconds" << std::endl;
	//std::cout << "Total Samples: " << total_samples << std::endl;
	//std::cout << "Total Bytes: " << (total_samples * 4) << std::endl;
	//std::cout << "Sample rate: " << sample_freq<< " samples per second." << std::endl;
	//std::cout << "Sample interval: " << (1.0/sample_freq) << " second per sample." << std::endl;
	//std::cout << "Estimated block time: " << ((1.0/sample_freq)*1024) << " seconds." << std::endl;
	//std::cout << "Estimated time: " <<((1.0/sample_freq)* total_samples) << " seconds" << std::endl;
	//std::cout << "Execution time for " <<  numSamples << " Blocks read( " << total_samples << " )samples: "
	//				<< time_duration.count() << " microseconds" << std::endl;


}	
