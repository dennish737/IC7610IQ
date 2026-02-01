//
// g++ -I../include SimpleExample.cpp  -o SimpleExample.exe 
//
#include <iostream>


#include <string>
#include <args.hxx> // Include the args header

int main(int argc, char** argv)
{
    args::ArgumentParser parser("This is a test program.", "This is an optional extra description.");
    args::HelpFlag help(parser, "help", "Display this help menu.", {'h', "help"});
    args::Flag extract(parser, "extract", "Extract the data.", {'e', "extract"});
    args::ValueFlag<std::string> input_file(parser, "file", "The input file name.", {'i', "input"});

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (const args::Help& e)
    {
        std::cout << parser;
        return 0;
    }
    catch (const args::ParseError& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (const args::ValidationError& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    if (extract) {
        std::cout << "Extraction flag set." << std::endl;
    }

    if (input_file) {
        std::cout << "Input file: " << args::get(input_file) << std::endl;
    }

    return 0;
}

/*
% ./test
% ./test -h
 ./test {OPTIONS} 
 
   This is a test program. 
 
 OPTIONS:
 
     -h, --help         Display this help menu 
 
   This goes after the options. 
% 
*/