
//g++ logtest3.cpp -o logtest3 -lspdlog -lfmt

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h" // Required for color console logging
#include "spdlog/stopwatch.h"


void thread_task(int id) {
    // Access the global logger by name from any thread
    auto logger = spdlog::get("console");
    if (logger) {
        auto start = std::chrono::steady_clock::now();
        logger->info("Thread {} is starting...", id);
        logger->warn("Thread {} is performing a task", id);
        logger->error("Thread {} encountered an error simulation", id);
        auto end = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        logger->info("Thread {}, Duration: {} ms", id, diff.count());   
    }
}

int main() {
    // 1. Create a thread-safe color console logger
    // Use _mt suffix for multi-threaded safety
    auto console = spdlog::stdout_color_mt("console");
    spdlog::stopwatch sw; // Starts timing
    // 2. Optional: Set a custom logging pattern (includes thread ID with %t)
     spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%^%l%$] %v");

    #ifdef DEBUG
        spdlog::set_level(spdlog::level::debug); // Set to debug/trace
    #else
        spdlog::set_level(spdlog::level::info);  // Default for production
    #endif

    try {

        // 3. Launch multiple threads
        std::vector<std::thread> threads;
        for (int i = 1; i <= 5; ++i) {
            threads.emplace_back(thread_task, i);
        }
            
        // Wait for all threads to complete
        for (auto &t : threads) {
            t.join();
        }

        console->info("All threads finished execution.");

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        return 1;
    }

    // Log elapsed time in seconds
    //spdlog::debug("Elapsed {} sec", sw);
    // Log elapsed time with 3 decimal places
    spdlog::debug("Elapsed {:.3} sec", sw);

    return 0;
}
