#ifndef CMAN_PARSER_H
#define CMAN_PARSER_H

#include "cli.h"
#include <vector>


namespace cman {
    inline namespace v1 {
        struct Hash {
            std::string value;
        };
        enum class ErrorType {
            ILLEGAL,
            EMPTY_TOKEN,
            ILLEGAL_FORMAT,
            UNIMPLEMENTED,
            OK
        };
        //TODO: repetitive logic below.
        class Parser {
            public:
                //HACK: have a hashmap where the key is optiontype to the option struct?
                std::vector<cman::Option> options;
                std::vector<cman::OptionType> tokens;
                std::string project_name;
                Hash hash;
                Hash parse();
                Parser(std::vector<cman::Option> options);
                ~Parser();

            private:
                ErrorType get_optiontypes();
                ErrorType construct_hash();
                ErrorType evaluate();
        };
    }
}


#endif // !CMAN_PARSER_H

