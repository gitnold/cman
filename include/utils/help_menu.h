#ifndef CMAN_HELP_MENU_H
#define CMAN_HELP_MENU_H


#include "cli.h"
#include <string>
#include <string_view>
#include <unordered_map>


namespace cman {
inline namespace v1 {
    namespace help_menu {

        struct HelpEntry {
            std::string flag;
            std::string descr;
        };
        
        inline std::unordered_map<OptionType, HelpEntry> help_menu = {
            {OptionType::HELP, {"--help", "print the help manu and quit"}},
            {OptionType::GIT, {"--git", "initialize a git repository in the current directory"}},
            {OptionType::BUILD, {"--build [build type ]", "Build the current project using the specified build type"}},
            {OptionType::RUN, {"--run", "Run the binary if no changes are made, if changes are present the project is compiled first"}},  // NOTE: needs a config file for persistent choice storage. local configs overrride the global config.
            {OptionType::NEW, {"--new [project name]", "Initialize a new project under the directory named [project name]"}},
            {OptionType::LANGUAGE, {"--lang [c/cpp]", "Specify a language to be used in the project"}}
        };

        // prints the whole help menu.
        void print_whole_help();

        // takes an option type as a key and the wrong value put in and prints a helpful message.
        //FIX: move to the semantic analyzer. 
        void print_help_tip(std::string option, std::string_view error);
        void print_version_info();
    }
}}

#endif // !CMAN_HELP_MENU_H
