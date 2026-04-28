

// g++ -I../include IQPortStream_disk_wr.cpp ../src/IcomIQPort.cpp -o IQPortStream_disk_wr.exe -lspdlog -lfmt -lboost_filesystem-mt -lws2_32 -L ../libs -lftd3xx
//  ./IQPortStream_disk_wr.exe

#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>
#include <complex>
#include <string>
#include <vector>

// boost
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

// logging functions
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>// Required for color console logging
#include <spdlog/fmt/ranges.h> // Required for vector support
#include <spdlog/fmt/bin_to_hex.h> // format vector of bytes to hex values


// splog stopwatch for timing
//#include "spdlog/stopwatch.h"

//#include <libserialport.h>

#ifndef _WINSOCKAPI_
    #include <windows.h>
#endif


#include "BufferPool.hpp"
#include "IcomIQPort.hpp"

const uint8_t mainVFO = 0x00;
const uint8_t subVFO = 0x01;

#define NUM_BUFFERS 8
#define BUFFER_SIZE  16384
#define SYS_PAGE_SIZE  4096

BufferPool* poolPointer;

namespace asio = boost::asio;

std::string fileName = "IQStreamTestDatq.data";


struct ASYNC_CONTEXT : OVERLAPPED {
	UINT id;
	HANDLE ftHandle;
    Buffer* buffer;          // Data buffer for this request
	void (*on_complete)(ASYNC_CONTEXT*, DWORD, DWORD);

    ASYNC_CONTEXT(HANDLE devHandle) {
        memset(this, 0, sizeof(OVERLAPPED));
		FT_InitializeOverlapped(devHandle, this);
        hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // Manual reset
		ftHandle = devHandle;
    };
    ~ASYNC_CONTEXT() { CloseHandle(hEvent); }
};

// 2. Callback function
void OnReadComplete(ASYNC_CONTEXT* ov, DWORD errorCode, DWORD bytesTransferred) {
	FT_STATUS  status;
    std::cout << "Read completed. Bytes: " << bytesTransferred << " Error: " << errorCode << std::endl;
	auto end = std::chrono::steady_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	UINT currentIdx = ov->id;
	Buffer* currentBuffer = ov->buffer;
	consoleLog->info("Read completed, bufferId = {}, number of bytes = {}",currentBuffer->bufferId, currentBuffer->totalUsed);
    // Process your IQ data here (e.g., DSP, FFT, or save to disk)

	// acquire new Buffer
	ov->buffer = poolPointer->acquire();
	consoleLog->info(" New bufferId {}", ov->buffer->bufferId);
	consoleLog->info(" Releasing bufferId {}", currentBuffer->bufferId);
	//poolPointer->release(currentBuffer);
	// write the data
    consoleLog->info("Duration: {} ms", diff.count()); 


    // 5. Re-queue the buffer immediately to keep the stream alive
	uint8_t* buf_ptr = ov->buffer->bufferPointer;
	ULONG* bytesTransferredPtr = (ULONG*)&(ov->buffer->totalUsed);

	status = FT_ReadPipeEx(ov->ftHandle, IQ_IN, buf_ptr, BUFFER_SIZE, bytesTransferredPtr, ov);
	consoleLog->info("Context({})status = {}", currentIdx, status);
}

// Callback function to handle completion
void OnWriteComplete(ASYNC_CONTEXT* ov, const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (!ec) {
        std::cout << "Successfully wrote " << bytes_transferred << " bytes." << std::endl;
		Buffer* currentBuffer = ov->buffer;
		consoleLog->info(" Releasing bufferId {}", currentBuffer->bufferId);
		poolPointer->release(currentBuffer);

    } else {
        std::cerr << "Error: " << ec.message() << std::endl;
    }
}


int pass_count = 0;
int fail_count = 0;
int total_count = 0;
std::chrono::steady_clock::time_point start;

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

// 2. Callback function
void OnReadComplete(ASYNC_CONTEXT* ov, DWORD errorCode, DWORD bytesTransferred) {
	FT_STATUS  status;
    std::cout << "Read completed. Bytes: " << bytesTransferred << " Error: " << errorCode << std::endl;
	auto end = std::chrono::steady_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	UINT currentIdx = ov->id;
	Buffer* currentBuffer = ov->buffer;
	consoleLog->info("Read completed, bufferId = {}, number of bytes = {}",currentBuffer->bufferId, currentBuffer->totalUsed);
    // Process your IQ data here (e.g., DSP, FFT, or save to disk)

	// acquire new Buffer
	ov->buffer = poolPointer->acquire();
	consoleLog->info(" New bufferId {}", ov->buffer->bufferId);
	consoleLog->info(" Releasing bufferId {}", currentBuffer->bufferId);
	poolPointer->release(currentBuffer);
    consoleLog->info("Duration: {} ms", diff.count()); 


    // 5. Re-queue the buffer immediately to keep the stream alive
	uint8_t* buf_ptr = ov->buffer->bufferPointer;
	ULONG* bytesTransferredPtr = (ULONG*)&(ov->buffer->totalUsed);

	status = FT_ReadPipeEx(ov->ftHandle, IQ_IN, buf_ptr, BUFFER_SIZE, bytesTransferredPtr, ov);
	consoleLog->info("Context({})status = {}", currentIdx, status);
}

size_t asioWriteToDisk( asio::stream_file dataFile, Buffer* bufPtr )
{

	return 0;
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

	consoleLog->info("Starting IQPortStream_disk_wr.");

	consoleLog->info("create boost asio context");
	//asio::io_context ctx;
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
	consoleLog->info("Alocating buffer pool of {} buffers with size = {}", NUM_BUFFERS, BUFFER_SIZE);
	poolPointer = new BufferPool(NUM_BUFFERS, BUFFER_SIZE);

	
	// Wrap Handle in Boost.Asio
	//ost::asio::io_context io_ctx;
	//ost::asio::windows::stream_handle ftdi_stream(io_ctx, (HANDLE)ftHandle);

	// We always want to have a read pending on the device. To do that we will queue two overlapped
	// reads. When a read completes,
	//	1) acquire a new buffer
	//	2) queue the buffer for processing
	//	3) queue a new read.
	consoleLog->info("Setup boost info");
	ASYNC_CONTEXT ov[2] { ASYNC_CONTEXT(ftHandle), ASYNC_CONTEXT(ftHandle)};
	//boost::asio::windows::stream_handle ftdi_stream(io_ctx, (HANDLE)ftHandle);
	//auto stream_ = std::make_unique<FtStream>(io_context, (HANDLE)ftHandle);
	consoleLog->info("Initializeing Context");

    for (int i = 0; i < 2; i++) {
		ov[i].buffer = poolPointer->acquire();
		ov[i].id = i;
		ov[i].on_complete = OnReadComplete;
        // Initialize overlapped structure for D3XX use
		//hEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
		//ov[i].hEvent = hEvents[i];
        //FT_InitializeOverlapped(ftHandle, &ov[i].overlapped);
        //ov[i].overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

	// 3 open the file
    //asio::stream_file file(ctx, fileName, 
    //    asio::stream_file::write_only | asio::stream_file::create | asio::stream_file::truncate);
	
	// 4. Queue the initial reads
	consoleLog->info("Queueing First Read");
	// ULONG bytesTransferred = 0;
	// ASYNC_CONTEXT* ctx = reinterpret_cast<ASYNC_CONTEXT*>(lpOverlapped);
	// OVERLAPPED* lovp = reinterpret_cast<LPOVERLAPPED *>&&ov[i];
	consoleLog->info(" queueing bufferId {}", ov[0].buffer->bufferId);
	uint8_t* buf_ptr = ov[0].buffer->bufferPointer;
	ULONG* bytesTransferredPtr = &ov[0].buffer->totalUsed;
	//OVERLAPPED* startLOPV  = (&ov[0].overlapped);
	status = FT_ReadPipeEx(ftHandle, IQ_IN, buf_ptr, BUFFER_SIZE, bytesTransferredPtr, &ov[0]);
	consoleLog->info("Context({}) status = {}", 0, status);

	consoleLog->info("Queueing Second Read");
	consoleLog->info(" queueing bufferId {}", ov[1].buffer->bufferId);
	buf_ptr = ov[1].buffer->bufferPointer;
	bytesTransferredPtr = &ov[1].buffer->totalUsed;
	//startLOPV  = (&ov[1].overlapped);
	status = FT_ReadPipeEx(ftHandle, IQ_IN, buf_ptr, BUFFER_SIZE, bytesTransferredPtr, &ov[1]);
	consoleLog->info("Context({})status = {}", 1, status);

    int currentIdx = 0;
    bool streaming = true;
	int streamCount = 20;

	consoleLog->info("starting stream");
	if (!iqPort.isIQDataEnabled()){
		iqPort.enableIQData(mainVFO);
	}
    while (streaming && streamCount > 0) {
		start = std::chrono::steady_clock::now();
		consoleLog->info("Read Start");
        // 4. Wait for the current buffer to complete
		//LPOVERLAPPED*  lpov = (&ov[currentIdx].overlapped);
		ULONG* num_bytes =  (&ov[currentIdx].buffer->totalUsed);
		consoleLog->info( "Calling FT_GetOverlappedResult");
        //status = FT_GetOverlappedResult(ftHandle,&ov[currentIdx], num_bytes, TRUE);
		DWORD waitResult = WaitForSingleObject(ov[currentIdx].hEvent, INFINITE);
       
        //if (status == FT_OK) {
		if (waitResult == WAIT_OBJECT_0) {
			consoleLog->info(" process data");
			consoleLog->info( "Completion Status {}", status);
			consoleLog->info( " Number of bytes {}", ov[currentIdx].buffer->totalUsed);
			ov[currentIdx].on_complete(&ov[currentIdx], status, ov[currentIdx].buffer->totalUsed);

        }
		else {
			consoleLog->error("Read Failed: error = {}", status);
		}

        // Toggle to the other buffer
		//ResetEvent(ov[currentIdx].hEvent);
        currentIdx = (currentIdx + 1) % 2;
		consoleLog->info("Loop Count = {}",streamCount);
		streamCount--;
    }

	consoleLog->info("Cleanup");
	iqPort.disableIQData();
    // 6. Cleanup
    for (int i = 0; i < 2; i++) {
        FT_ReleaseOverlapped(ftHandle, &ov[i]);
        CloseHandle(ov[i].hEvent);

    }
	
    iqPort.close();

    return 0;



	
}	
	
		
	