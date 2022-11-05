#ifndef UTILS_H
#define UTILS_H

#include <string>

char* get_command_line_option(int, char **, const std::string&);
bool command_line_option_exists(int, char **, const std::string&);

#endif