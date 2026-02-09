// g++ -I../include Commands.cpp  -o Commands.exe
//

#include <args.hxx>
int main(int argc, char **argv)
{
    args::ArgumentParser p("git-like parser");
    args::Group commands(p, "commands");
    args::Command add(commands, "add", "add file contents to the index");
    args::Command commit(commands, "commit", "record changes to the repository");
    args::Group arguments(p, "arguments", args::Group::Validators::DontCare, args::Options::Global);
    args::ValueFlag<std::string> gitdir(arguments, "path", "", {"git-dir"});
    args::HelpFlag h(arguments, "help", "help", {'h', "help"});
    args::PositionalList<std::string> pathsList(arguments, "paths", "files to commit");
 
    try
    {
        p.ParseCLI(argc, argv);
        if (add)
        {
            std::cout << "Add";
        }
        else
        {
            std::cout << "Commit";
        }
 
        for (auto &&path : pathsList)
        {
            std::cout << ' ' << path;
        }
 
        std::cout << std::endl;
    }
    catch (args::Help)
    {
        std::cout << p;
    }
    catch (args::Error& e)
    {
        std::cerr << e.what() << std::endl << p;
        return 1;
    }
    return 0;
}

/*
% ./test -h
  ./test COMMAND [paths...] {OPTIONS}
 
    git-like parser
 
  OPTIONS:
 
      commands
        add                               add file contents to the index
        commit                            record changes to the repository
      arguments
        --git-dir=[path]
        -h, --help                        help
        paths...                          files
      "--" can be used to terminate flag options and force all following
      arguments to be treated as positional options
 
% ./test add 1 2
Add 1 2
*/
