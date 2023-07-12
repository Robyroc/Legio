#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

namespace legio {

// Gets the command line option following the one specified in the last parameter
// If it cannot find any, it will return 0
// Both the returned value and the last parameter are removed from argv
char* get_command_line_option(int*, char***, const std::string&);

// Checks if the last parameter is part of argv
// If so, it removes it
bool command_line_option_exists(int*, char***, const std::string&);

/*
template <typename, typename = void>
constexpr bool is_type_complete_v = false;

template <typename T>
constexpr bool is_type_complete_v<T, std::void_t<decltype(sizeof(T))>> = true;
*/

}  // namespace legio

#endif