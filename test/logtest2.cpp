
//g++ logtest2.cpp -o logtest2 -lspdlog -lfmt

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

/*
spdlog set_pattern
Common Pattern Flags
Flags are prefixed with % and resemble strftime formatting: 

Flag 	    Meaning	Example Output
%v	        The actual log message text	"User logged in"
%n	        Logger's name	"my_logger"
%l	        Full log level	"debug", "info", "warn"
%L	        Short log level	"D", "I", "W"
%t	        Thread ID	"1234"
%P	        Process ID	"5678"
%H:%M:%S	Time (Hours:Minutes:Seconds)	"23:59:59"
%Y-%m-%d	Date (Year-Month-Day)	"2024-03-31"
%^ / %$	    Start/End color range	Colors text between flags
%@	        Source file and line number	"main.cpp:42" (Requires macros)
*/

spdlog::logger* getConsoleLogger() {
    // Create a color multi-threaded logger named "console"
    auto console = spdlog::stdout_color_mt("console");
    
    // Set global or logger-specific log level
   //console->set_level(spdlog::level::debug); 
    
    // Return the raw pointer using .get()
    return console.get();

}

int main() {
    // get the logger
    spdlog::logger* consoleLog = getConsoleLogger();
    consoleLog->set_pattern("[%Y-%m-%d %H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
    // Basic log levels
    consoleLog->info("Welcome to spdlog!");
    consoleLog->error("An error occurred with code: {}", 404);
    consoleLog->warn("Easy padding: {:08d}", 123);
    
    // Changing global log level (default is info)
    consoleLog->set_level(spdlog::level::debug); 
    consoleLog->debug("This message will  be visible");
    consoleLog->trace("This message will not be visible");

    // Customizing the log pattern
    // Pattern: [Hour:Min:Sec] [Logger Name] [Level] Message
    consoleLog->set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
    
    consoleLog->info("Message with a new pattern");
    return 0;
}

#include "spdlog/spdlog.h"

void basic_usage_example()
{
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
    spdlog::info("Welcome to spdlog!");
    spdlog::error("Some error message with arg: {}", 1);
    spdlog::warn("Easy padding in numbers like {:08d}", 12);
    spdlog::critical("Support for int: {0:d};  hex: {0:x} oct: {0:o} bin: {0:b}", 42);
    spdlog::info("Support for floats {:03.2f}", 1.23456);
    spdlog::info("Positional args are {1} {0}..", "too", "supported");
    spdlog::info("{:<30}", "left aligned");
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    spdlog::debug("This message should be displayed..");    
    spdlog::trace("This message shouldn't be displayed..");    
}
