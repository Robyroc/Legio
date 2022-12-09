#include "utils.hpp"
#include <algorithm>
#include <string>

char* legio::get_command_line_option(int argc, char** argv, const std::string& option)
{
    char** begin = argv;
    char** end = argv + argc;
    char** itr = std::find(argv, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool legio::command_line_option_exists(int argc, char** argv, const std::string& option)
{
    return std::find(argv, argv + argc, option) != argc + argv;
}
