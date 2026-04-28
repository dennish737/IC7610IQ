// g++ -I../include boostwrapper.cpp ../src/IcomIQPort.cpp -o boostwrapper.exe -lspdlog -lfmt -lboost_filesystem-mt -lws2_32 -L ../libs -lftd3xx
// ./boostwrapper.exe

#include <boost/asio.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <windows.h>
#include <ftd3xx.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>// Required for color console logging
#include <spdlog/fmt/ranges.h> // Required for vector support
#include <spdlog/fmt/bin_to_hex.h> // format vector of bytes to hex values

#ifndef _WINSOCKAPI_
    #include <windows.h>
#endif

#include "IcomIQPort.hpp"

const uint8_t mainVFO = 0x00;
const uint8_t subVFO = 0x01;

#define NUM_BUFFERS 8
#define BUFFER_SIZE  16384
#define SYS_PAGE_SIZE  4096

// Use a custom context structure to pass extra data
struct D3XXOverlappedContext {
    OVERLAPPED overlapped;
    int pipeId;
    std::vector<uint8_t> buffer; // Buffer used for read/write
    // Add any other user-defined data here
    ULONG bytesTransfered;
};


int main() {
    std::string deviceSerialNum = IcomIQPort::getDeviceSerialNum();

	FT_HANDLE ftHandle;
    FT_STATUS ftStatus;
    FT_DEVICE_LIST_INFO_NODE nodes[16];
    DWORD count;
    int res;
	
	printf("USB Device Serial Number: %s", deviceSerialNum);
	IcomIQPort iqPort;
	iqPort.init(deviceSerialNum, true);
	if (!iqPort.isOpen())
	{
        printf("falied to open device Aborting \n");
        exit(1);
    }

    ftHandle = iqPort.getHandle();

    printf("IC7610IQ Port is open\n");

    //auto stream_ptr = std::make_shared<boost::asio::windows::stream_handle>(io_context, ftHandle);

    // Wrap the handle for Boost Asio
    //boost::asio::io_context io_ctx;
    //boost::asio::windows::stream_handle ftdi_stream(io_ctx, ftHandle);

    // 3. Using FT_ReadPipeEx and FT_GetOverlappedResults
    //When using FT_ReadPipeEx, it returns FT_IO_PENDING immediately. You then use FT_GetOverlappedResult 
    //(or FT_W32_GetOverlappedResult) in the completion handler. 

    // Example: Asynchronous Read using FT_ReadPipeEx
    auto context = std::make_shared<D3XXOverlappedContext>();
    context->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    context->pipeId = 0x84; // Example Pipe ID
    context->buffer.resize(131072);

    // Initiate async read
    ftStatus = FT_ReadPipeEx(ftHandle, 
                            context->pipeId, 
                            context->buffer.data(), 
                            context->buffer.size(), 
                            &context->bytesTransfered,
                            &context->overlapped);
                            

    if (ftStatus != FT_IO_PENDING && ftStatus != FT_OK) {
        // Handle error
        printf("IO error");
        CloseHandle(context->overlapped.hEvent);
    }
    printf("Queued status = %d\n", ftStatus);
    iqPort.enableIQData(0);

    // Associate the event with asio to handle completion
    boost::asio::io_context io_ctx;
    auto waiter = std::make_shared<boost::asio::windows::object_handle>(io_ctx, context->overlapped.hEvent);
    waiter->async_wait([context, waiter, ftHandle](const boost::system::error_code& ec) {
        if (!ec) {
            DWORD bytesTransferred = 0;
            // Retrieve result
            BOOL success = FT_GetOverlappedResult(ftHandle, 
                                                    &context->overlapped, 
                                                    &bytesTransferred, 
                                                    FALSE);
            if (success) {
                std::cout << "Read " << bytesTransferred << " bytes from pipe " << context->pipeId << std::endl;
            }
        } else {
            std::cout << "Error" << ec;
        }
        // Cleanup
        CloseHandle(context->overlapped.hEvent);
    });

    io_ctx.run(); // Run the Asio event loop

}


