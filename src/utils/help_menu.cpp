#include "utils/help_menu.h"
#include "style.h"
#include <print>

namespace cman {
inline namespace v1 {
    namespace help_menu {
                
        void print_whole_help() {
            // preamble.
            cman::print_message("Welcome to 'Cman`s' help menu (^_^)\n\nUsage  :: \n", INFO);
            
            // iterate the help menu table and dump everything to stdout.
            for (const auto& [_, help] : help_menu) {
                std::println("\t{} :: {}", help.flag, help.descr);
            }
        }
        // TODO: read on terminal tables.
        void print_version_info() {
            std::println("cman v0.1.0");
        }
    }
}}

