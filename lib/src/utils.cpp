#include "utils.hpp"
#include <algorithm>
#include <string>

char* legio::get_command_line_option(int* argc, char*** argv, const std::string& option)
{
    char** begin = *argv;
    char** end = *argv + *argc;
    char** itr = std::find(*argv, end, option);
    if (itr != end && (itr + 1) != end)
    {
        char* value = *(itr + 1);
        while (itr + 2 != end)
        {
            *itr = *(itr + 2);
            itr++;
        }
        *argc -= 2;
        return value;
    }
    return 0;
}

bool legio::command_line_option_exists(int* argc, char*** argv, const std::string& option)
{
    char** itr = std::find(*argv, *argv + *argc, option);
    char** end = *argv + *argc;
    if (itr != end)
    {
        while (itr + 1 != end)
        {
            *itr = *(itr + 1);
            itr++;
        }
        *argc -= 1;
        return true;
    }
    return false;
}
