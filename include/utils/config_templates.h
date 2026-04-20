#ifndef CMAN_BOILERPLATES_H
#define CMAN_BOILERPLATES_H

#include <string>
#include <string_view>
#include <vector>
#include "build.h"

// TODO: consinder switching to file templates stored somewhere

namespace cman {
inline namespace v1 {
    namespace utils {
        
        inline std::string hello_world(std::string_view project_name, cman::Lang language) {
            switch (language) {
                case cman::Lang::C:
                    return ("#include <stdio.h>\n"
                           "int main(void) {\n"
                           "\tprintf(\"Hello from " + std::string(project_name) + "\\n\");\n"
                           "\treturn 0;\n"
                           "}");
                
                case cman::Lang::CPP:
                    return ("#include <iostream>\n"
                           "int main() {\n"
                           "\tstd::cout << \"Hello from " + std::string(project_name) + "\" << std::endl;\n"
                           "\treturn 0;\n"
                           "}");
                
                default:
                    return ""; 
            }
        }

        std::vector<std::string> split_string(std::string);
        
    } // namespace utils
} // inline namespace v1
} // namespace cman

#endif // CMAN_BOILERPLATES_H
