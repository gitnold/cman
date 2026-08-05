#include <cstdlib>
#include <string>
#include <string_view>
#include <format>
#include "style.h"

namespace cman {
inline namespace v1 {
    namespace utils {
        void generate_compilation_db(std::string_view build_command) {
            // check if bear is installed and inform the user.
            // attach the link, no support for autodownload currently, maybe add support later.
            auto command = std::string{std::format("bear -- {}", build_command)}; 
            if (std::system(command.c_str()) == 0) {
                cman::print_message("Compilation database generated successfully!!", INFO);
            } else {
                cman::print_message("bear run failed! Make sure bear is installed! Download at https://github.com/rizsotto/bear", ERROR);
            }
        }
    }
}
}
