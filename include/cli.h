#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include <string>
#include <vector>

// maybe deprecated.
#define MAX_OPTIONS 3

namespace cman {
inline namespace v1 {
    //consinder namespaces.
    enum class OptionType {
        NEW,
        HELP,       //consinder using a union for help/default.
        INIT,
        GIT,
        LIB,
        RUN,
        BUILD,
        ILLEGAL
    };

    //TODO: try using a union for optional values types.
    struct Option {
        OptionType type;
        std::string value;  //try using unions for non-values params.
    };

    class Config {
        public:
            char** args;
            int num_of_args;
            std::string package_name;
            std::vector<Option> options;
            Config(char** args, int number);
            void parse();
            ~Config();

        private:
            Option make_option(OptionType type, std::string value);
            Option check_arg(const char* arg, char* value);
            
    };
}}
#endif // !CLI_PARSER_H

