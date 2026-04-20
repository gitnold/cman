#ifndef CMAN_HELP_MENU_H
#define CMAN_HELP_MENU_H


#include <string>
#include <string_view>
#include <unordered_map>
namespace cman {
inline namespace v1 {
    namespace help_menu {
        // prints the whole help menu.
        void print_whole_help();
        
        // takes an option type as a key and the wrong value put in and prints a helpful message.
        void print_help_tip(std::string option, std::string_view error);
        void print_version_info();
    }
}}
#endif // !CMAN_HELP_MENU_H
