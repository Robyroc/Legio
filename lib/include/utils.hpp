#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

namespace legio {

char* get_command_line_option(int, char**, const std::string&);
bool command_line_option_exists(int, char**, const std::string&);

/*
template <typename, typename = void>
constexpr bool is_type_complete_v = false;

template <typename T>
constexpr bool is_type_complete_v<T, std::void_t<decltype(sizeof(T))>> = true;
*/

}  // namespace legio

#endif