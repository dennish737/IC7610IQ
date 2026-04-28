

// g++ logtest1.cpp -o logtest1 -lspdlog -lfmt

#include "spdlog/spdlog.h"

int main() {
    // Basic log levels
    spdlog::info("Welcome to spdlog!");
    spdlog::error("An error occurred with code: {}", 404);
    spdlog::warn("Easy padding: {:08d}", 123);
    
    // Changing global log level (default is info)
    spdlog::set_level(spdlog::level::debug); 
    spdlog::debug("This will now be visible");

    // Customizing the log pattern
    // Pattern: [Hour:Min:Sec] [Logger Name] [Level] Message
    spdlog::set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
    spdlog::info("Message with a new pattern");
}

