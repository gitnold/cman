#include "cli.h"
#include <string>
#include <string_view>
#include <vector>

namespace cman {
inline namespace v1 {
    Config::Config(char** args,int number){
        this->args = args;
        this->num_of_args = number;
        this->package_name = "Cman";

        //this->options {}; //TODO: how to declare an empty vector without initializing

    }
    
    Config::~Config() {
        
    }

    void Config::parse() {
        // std::vector<char*> args {this->args};
        for (int i = 1; i < this->num_of_args; i++) {
            //TODO: rectify the null read on the last iteration for the second parameter to check_arg()
            //FIX: skip first arg as its the program itself.
            //TODO: return the options to the user on instantiating the cli parser.
            //FIX: logic below causes segfaults why??
            
            this->options.push_back(check_arg(this->args[i], this->args[i+1]));
            const char* current = this->args[i];
            char* next = (i + 1 < this->num_of_args) ? this->args[i + 1] : nullptr;

            // Must be an option (starts with '-')
            if (current[0] == '-') {
                // paired: current has a following value that isn’t another option
                if (next && next[0] != '-') {
                    this->options.push_back(check_arg(current, next));
                    ++i; // skip value
                }
                // standalone: just the flag
                else {
                    this->options.push_back(check_arg(current, nullptr));
                }
            }
        }
    }

    Option Config::check_arg(const char* arg,  char* value) {
        //TODO: add support for -v or --version
        //TODO: add support for -- for passing cli options to the resulting binary.
        //Find whether a trie is more efficient for the giant if else toggle below.
        std::string value_new;
        if (value == nullptr) {
            value_new = "";
        } else {
            value_new = value;
        }

        std::string_view option {arg};

        if (option.compare("-h") || option.compare("--help")) {
            return make_option(OptionType::HELP, value_new);

        } else if (option.compare("-v") || option.compare("--version")) {
            return make_option(OptionType::HELP, value_new);

        } else if (option.compare("--git") == 0) {
            return make_option(OptionType::GIT, value_new);

        } else if (option.compare("--init") == 0) {
            //check that value is not null.
            return make_option(OptionType::INIT, value_new);
        } else if (option.compare("--run") == 0) {
            return make_option(OptionType::RUN, value_new);
            
        } else if (option.compare("--build") == 0) {
            return make_option(OptionType::BUILD, value_new);

        } else if (option.compare("--new") == 0) {
            return make_option(OptionType::NEW, value_new);

        } else if (option.compare("--lang") == 0) {
            return make_option(OptionType::LANGUAGE, value_new);

        } else if (option.compare("--update")) {
            return make_option(OptionType::UPDATE, value_new);
        
        } else if (option.compare("--mode")){
            return make_option(OptionType::MODE, value_new);
        } else if (option.compare("--")) {
            return make_option(OptionType::CLI_ARGS, value_new);

        } else if (option.empty() == true) {
            return make_option(OptionType::HELP, "print help");

        } else {
            return make_option(OptionType::ILLEGAL, "Unknown option passed!!");
        
        }
    }

    Option Config::make_option(OptionType type, std::string value) {
        switch (type) {
            case cman::OptionType::GIT:
                return (Option) {
                    .type = OptionType::GIT,
                    .value = value
                };
                break;

            case cman::OptionType::HELP:
                return (Option) {
                    .type = OptionType::HELP,
                    .value = value
                };
                break;

            case cman::OptionType::INIT:
                return (Option) {
                    .type = OptionType::INIT,
                    .value = value
                };
                break;

            case cman::OptionType::ILLEGAL:
            //TODO: design custom error values.
                return (Option) {
                    .type = OptionType::ILLEGAL,
                    .value = value
                };
                break;

            case cman::OptionType::NEW:
                return (Option) {
                    .type = OptionType::NEW,
                    .value = value
                };
                break;

            case cman::OptionType::LIB:
                return (Option) {
                    .type = OptionType::LIB,
                    .value = value
                };
                break;

            case cman::OptionType::RUN:
                return (Option) {
                    .type = OptionType::RUN,
                    .value = value
                };
                break;

            case cman::OptionType::BUILD:
                return (Option) {
                    .type = OptionType::BUILD,
                    .value = value
                };
                break;

            case cman::OptionType::LANGUAGE:
                return (Option) {
                    .type = OptionType::LANGUAGE,
                    .value = value
                };
                break;
            
            case cman::OptionType::UPDATE:
                return (Option) {
                    .type = OptionType::UPDATE,
                    .value = value
                };
                break;
            

            case cman::OptionType::MODE:
                return (Option) {
                    .type = OptionType::MODE,
                    .value = value
                };
                break;
            
            case cman::OptionType::VERSION:
                return (Option) {
                    .type = OptionType::VERSION,
                    .value = value
                };
                break;

            case cman::OptionType::CLI_ARGS:
                return (Option) {
                    .type = OptionType::CLI_ARGS,
                    .value = value
                };
                break;

            default:
                //TODO: find a way to propagate errors.
                return (Option) {
                    .type = OptionType::ILLEGAL,
                    .value = "Unrecognized option encountered!!"
                };
        }
    }
}}
