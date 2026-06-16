#ifndef CMAN_PARSER_H
#define CMAN_PARSER_H

#include "build.h"
#include "cli.h"
#include <optional>
#include <string>
#include <vector>


namespace cman {
    inline namespace v1 {
        struct Hash {
            std::string value;
        };

        // TODO: change this to a type that can incorporate and propagate the error found to the end user.
        enum class ResultType {
            ILLEGAL,
            EMPTY_TOKEN,
            ILLEGAL_FORMAT,
            UNIMPLEMENTED,
            OK
        };

        struct ParsedInput {
            bool has_help = false;
            std::optional<std::string> project_name;
            std::optional<std::string> language;
            BuildMode mode = BuildMode::DEFAULT;
            BuildType build_type = BuildType::SHELL_SCRIPT;  // cman defaults to using shell scripts.
            bool init_dir = false;
            bool init_git = false;
            bool init_project = false;
            bool build_project = false;
            bool run_bin = false;
            bool cli_args = false;
            std::optional<std::string> bin_args;

        };

        //TODO: repetitive logic below.
        class Parser {
            public:
                //HACK: have a hashmap where the key is optiontype to the option struct?
                std::vector<cman::Option> options;
                std::vector<cman::OptionType> tokens;
                ParsedInput parse_result;
                std::string project_name;
                Hash hash;

                // TODO: take in the context as a const to avoid modifying it.
                ResultType parse();
                Parser(std::vector<cman::Option> options, LexMode mode);
                ~Parser();

            private:
                ResultType get_optiontypes();
        };
    }
}


#endif // !CMAN_PARSER_H
