

// g++ -I../include PipeStreamTest1.cpp ../src/IcomIQPort.cpp -o PipeStreamTest1.exe -lspdlog -lfmt -L ../libs -lftd3xx
//  ./Asynctest1.exe.exe

#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>
#include <complex>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>// Required for color console logging
#include <spdlog/fmt/ranges.h> // Required for vector support
#include <spdlog/fmt/bin_to_hex.h> // format vector of bytes to hex values


// splog stopwatch for timing
//#include "spdlog/stopwatch.h"

//#include <libserialport.h>

#include "IcomIQPort.hpp"

const uint8_t mainVFO = 0x00;
const uint8_t subVFO = 0x01;

#define NUM_BUFFERS 8
#define BUFFER_SIZE  16384
#define SYS_PAGE_SIZE  4096


// 2 The Completion Callback
/*
void CALLBACK ReadCompletionRoutine(DWORD dwErrorCode, DWORD dwBytesTransferred, LPOVERLAPPED lpOverlapped) {
    // Cast back to your custom structure
    iqASYNC_CONTEXT* ctx = reinterpret_cast<iqASYNC_CONTEXT*>(lpOverlapped);

    if (dwErrorCode == 0 && dwBytesTransferred > 0) {
        // Process the data in ctx->buffer
        // ...
        
		// get next buffer
        // Re-queue the next read immediately to maintain the stream
        FT_ReadPipeAsync(ctx->contextport., ctx->pipeID, ctx->buffer, ctx->bufferSize, 
                          &ctx->overlapped, ReadCompletionRoutine);
    } else {
        // Handle errors or EOF
    }
}
*/
int pass_count = 0;
int fail_count = 0;
int total_count = 0;



size_t sample_size = 4; // bytes
size_t num_samples_per_buff = 1024; // samples per read block
size_t num_buffers = 16;  //number of buffers

spdlog::logger* consoleLog;


spdlog::logger* getConsoleLogger() {
    // Create a color multi-threaded logger named "console"
    auto console = spdlog::stdout_color_mt("console");
    
    // Set global or logger-specific log level
   //consoleLog->set_level(spdlog::level::debug); 
    
    // Return the raw pointer using .get()
    return console.get();

}

void test_result( int expected, int results, const char* message)
{
	if (expected == results)
	{
		pass_count +=1;
		consoleLog->info(message, results, "passed");
	}
	else 
	{ 
		fail_count +=1;
		consoleLog->error(message, results, + "failed");
	}
}

void print_vector(std::vector<uint8_t> v)
{
	spdlog::info("Binary data: {}", spdlog::to_hex(v));

}

int main() {
    // 1. Create a thread-safe color console logger
    // Use _mt suffix for multi-threaded safety
    consoleLog = getConsoleLogger();
	// 2. Optional: Set a custom logging pattern (includes thread ID with %t)
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%^%l%$] %v");
	// set the log level default is info
    #ifdef DEBUG
        spdlog::set_level(spdlog::level::debug); // Set to debug/trace
    #else
        spdlog::set_level(spdlog::level::info);  // Default for production
    #endif

	consoleLog->info("Starting Asynctest1.");
	consoleLog->info("Enable IQ channels");
	std::vector<uint8_t> command = {0x1a, 0x0b}; // iq Enable 
		
	consoleLog->info("Find, Connect and Activate Highspeed USB Port : ");
	std::string deviceSerialNum = IcomIQPort::getDeviceSerialNum();
	consoleLog->info("USB Device Serial Number: {}", deviceSerialNum);
	IcomIQPort iqPort;
	iqPort.init(deviceSerialNum) ;
	if (iqPort.isOpen())
	{
		consoleLog->info("iqPort is open");
		int received_bytes = 0;
		std::vector<uint8_t> reply;
		received_bytes = iqPort.icomIQCommand(command, reply);

		if (received_bytes > 0) {
			consoleLog->info(" pass. result = {}", received_bytes);
			//print_vector(reply);
		} else
		{
			consoleLog->error("An error occurred with code: {}", received_bytes);
			consoleLog->error(" failed. \n");
			consoleLog->error("test aborted \n");
			exit(1);
		}
	} else {
		consoleLog->error("Port Not Open - abort");
		exit(2);
	}
	total_count += 1;
	pass_count += 1;

	// set center frequency
	int freq = iqPort.iqSetFrequency(mainVFO, 1380000);
	test_result(1380000, freq,"Set Main_VFO VFO Frequency: {} -> {}");
	total_count += 1;
	consoleLog->info("Set timeout to 10 Seconds\n");
	int timeout = 5000; // timeout in ms -> 5 seconds
	iqPort.setTimeout(IQ_IN, timeout); 

	// ... inside your streaming thread ...
	FT_HANDLE ftHandle =iqPort.getHandle();
    FT_STATUS status;


		// allocate buffers and overlapped
	// we need 2 buffers alligned on page bounraries
	consoleLog->info("Alocating buffers size = {}", BUFFER_SIZE);
	uint8_t* buffers[2];
	buffers[0] = (uint8_t*)_aligned_malloc(BUFFER_SIZE, SYS_PAGE_SIZE);
	buffers[1] = (uint8_t*)_aligned_malloc(BUFFER_SIZE, SYS_PAGE_SIZE);

	OVERLAPPED ov[2] = { 0 };
	HANDLE hEvents[2];

	consoleLog->info("Initializeing Context");

    for (int i = 0; i < 2; i++) {
        // Initialize overlapped structure for D3XX use
		//hEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
		//ov[i].hEvent = hEvents[i];
        FT_InitializeOverlapped(ftHandle, &ov[i]);
        ov[i].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

	
	// 3. Queue the initial reads
	consoleLog->info("Queueing First Read");
	ULONG bytesTransferred = 0;
	status = FT_ReadPipeEx(ftHandle, IQ_IN, buffers[0], BUFFER_SIZE, &bytesTransferred, &ov[0]);
	consoleLog->info("Context({}) status = {}", 0, status);
	status = FT_ReadPipeEx(ftHandle, IQ_IN, buffers[1], BUFFER_SIZE, &bytesTransferred, &ov[1]);
	consoleLog->info("Context({})status = {}", 1, status);

    int currentIdx = 0;
    bool streaming = true;
	int streamCount = 20;

	consoleLog->info("starting stream");
	if (!iqPort.isIQDataEnabled()){
		iqPort.enableIQData(mainVFO);
	}
    while (streaming && streamCount > 0) {
		auto start = std::chrono::steady_clock::now();
        // 4. Wait for the current buffer to complete
        status = FT_GetOverlappedResult(ftHandle, &ov[currentIdx], &bytesTransferred, TRUE);
        
        if (status == FT_OK) {
			auto end = std::chrono::steady_clock::now();
            // Process your IQ data here (e.g., DSP, FFT, or save to disk)
            // ProcessData(buffers[currentIdx], bytesTransferred);
            consoleLog->info("Read completed, number of bytes = {}", bytesTransferred);
			auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        	consoleLog->info("Duration: {} ms", diff.count()); 

            // 5. Re-queue the buffer immediately to keep the stream alive
            FT_ReadPipeEx(ftHandle, IQ_IN, buffers[currentIdx], BUFFER_SIZE, &bytesTransferred, &ov[currentIdx]);

        }
		else {
			consoleLog->error("Read Failed: error = {}", status);
		}

        // Toggle to the other buffer
        currentIdx = (currentIdx + 1) % 2;
		streamCount--;
    }

	consoleLog->info("Cleanup");
	iqPort.disableIQData();
    // 6. Cleanup
    for (int i = 0; i < 2; i++) {
        FT_ReleaseOverlapped(ftHandle, &ov[i]);
        CloseHandle(ov[i].hEvent);
        _aligned_free(buffers[i]);
    }
	
    iqPort.close();

    return 0;



	
}	
	
	
