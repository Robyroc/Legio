#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

namespace legio {

char* get_command_line_option(int, char**, const std::string&);
bool command_line_option_exists(int, char**, const std::string&);

}  // namespace legio

#endif