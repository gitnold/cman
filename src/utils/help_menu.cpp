#include "utils/help_menu.h"

namespace cman {
inline namespace v1 {
    namespace help_menu {
        std::unordered_map<std::string, std::string> help_menu {
            {"--help", "print the help manu and quit"},
            {"--git", "initialize a git repository in the current directory"},
            {"--build [build type ]", "Build the current project using the specified build type"},
            {"--run", "Run the binary if no changes are made, if changes are present the project is compiled first"},  // NOTE: needs a config file for persistent choice storage. local configs overrride the global config.
            {"--new [project name]", "Initialize a new project under the directory named [project name]"},
            {"--lang [c/cpp]", "Specify a language to be used in the project"}
        };
        
        void print_whole_help();
    }
}}

