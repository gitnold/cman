#ifndef CMAN_PARSER_H
#define CMAN_PARSER_H

#include "cli.h"
#include <optional>
#include <string>
#include <vector>


namespace cman {
    inline namespace v1 {
        struct Hash {
            std::string value;
        };
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
            bool init_dir = false;
            bool init_git = false;
            bool init_project = false;
            bool build_project = false;
            bool run_bin = false;
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
                Parser(std::vector<cman::Option> options);
                ~Parser();

            private:
                ResultType get_optiontypes();
                ResultType construct_hash();
                ResultType evaluate();
        };
    }
}


#endif // !CMAN_PARSER_H

