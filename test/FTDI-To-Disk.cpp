

// g++ -I../include FTDI-To-Disk.cpp ../src/IcomIQPort.cpp -o FTDI-To-Disk.exe -lspdlog -lfmt -lboost_filesystem-mt -lws2_32 -L ../libs -lftd3xx
// g++ -I../include -DDEBUG FTDI-To-Disk.cpp ../src/IcomIQPort.cpp -o FTDI-To-Disk.exe -lspdlog -lfmt -lboost_filesystem-mt -lws2_32 -L ../libs -lftd3xx
//  ./FTDI-To-Disk.exe

#include <iostream>
#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>


// logging functions
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>// Required for color console logging
#include <spdlog/fmt/ranges.h> // Required for vector support
#include <spdlog/fmt/bin_to_hex.h> // format vector of bytes to hex values

#include "BufferPool.hpp"
#include "IcomIqPort.hpp"
#include "ftd3xx.h"
#include "IcomIQStream.hpp"

#ifndef WINSOCK_API
    #include <windows.h>
#endif

using boost::asio::windows::object_handle;

namespace asio = boost::asio;

//const uint8_t mainVFO = 0x00;
//const uint8_t subVFO = 0x01;





std::shared_ptr<spdlog::logger> consoleLog;


/**
 * @brief Initializes and returns a console logger.
 * @param name The unique name for the logger.
 * @return std::shared_ptr<spdlog::logger> Pointer to the created logger.
 */
std::shared_ptr<spdlog::logger> getlogger(const std::string& name = "main_logger") {
    // Check if the logger already exists in the global registry to avoid duplicates
    auto main_logger = spdlog::get(name);
    
    if (!main_logger) {
        // 1. Create a color multi-threaded console logger named "main_logger"
        main_logger = spdlog::stdout_color_mt(name);

        // 2. Set the log level to debug for this specific logger
        // Note: You must also ensure SPDLOG_ACTIVE_LEVEL is set if using macros, if DEBUG define
#ifdef DEBUG
        main_logger->set_level(spdlog::level::debug);
#endif

        // 3. Set a custom pattern (e.g., [Timestamp] [LoggerName] [Level] Message)
        main_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

        // 4. Set this as the default logger so spdlog::info/debug calls use it
        spdlog::set_default_logger(main_logger);
    }
    return main_logger;
}



int main() {

    // 1. Create a thread-safe color console logger
    // Use _mt surffix for multi-threaded safety
    consoleLog = getlogger();
	// 2. Optional: Set a custom logging pattern (includes thread ID with %t)


	consoleLog->info("Starting IQPortStream_disk_wr.");

    consoleLog->info("create boost asio context");
    asio::io_context ctx;

	consoleLog->info("Find, Connect and Activate Highspeed USB Port : ");
	std::string deviceSerialNum = IcomIQPort::getDeviceSerialNum();
	consoleLog->info("USB Device Serial Number: {}", deviceSerialNum);

    
    // 1. Open device (Example using index 0)

    // 2. Start the async manager
    consoleLog->info("Init ftdi to disk  output: {}", "output.bin");
    auto writer = std::make_shared<IcomIQPortStream>(ctx, deviceSerialNum, "output.bin");
    uint8_t targetVFO  = mainVFO;
    consoleLog->info("Start vfo: {}", ((targetVFO == mainVFO)?"mainVFO" : "subVFO"));

    writer->start(targetVFO);

    // 3. Run for 30 seconds
    //ctx.run_for(std::chrono::seconds(300));
    ctx.run_for(std::chrono::seconds(30));
    consoleLog->info("Total Bytes read = {}", writer->getReadTotal());
    consoleLog->info("Total Bytes written = {}", writer->getWriteTotal());
    return 0;
}
