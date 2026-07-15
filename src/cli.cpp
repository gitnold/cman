#include "cli.h"
#include "build.h"
#include "json.hpp"
#include "style.h"
#include "utils/self_update.h"
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace cman {
inline namespace v1 {
    Config::Config(std::vector<std::string> args, [[maybe_unused]] LexMode mode){
        this->args = args;
        this->num_of_args = args.size();
        this->package_name = "Cman";

        //TODO:  if theres no cli args use the json configs.
        //TODO:  cli args might be provided but insufficient, requiring config consolidation. have the user explicity  enable config parsing.
        // parse() is called from main.cpp — avoid double-parsing.
        this->parse_config();

        //this->options {}; //TODO: how to declare an empty vector without initializing

    }

    Config::~Config() {

    }

    void Config::parse() {
        // TODO: check whether the args array is 1 indicating its empty and print the help menu.
        
        for (int i = 1; i < this->num_of_args; i++) {

            const char* current = this->args[i].c_str();
            const char* next = (i + 1 < this->num_of_args) ? this->args[i + 1].c_str() : nullptr;

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

    Option Config::check_arg(const char* arg,  const char* value) {
        //Find whether a trie is more efficient for the giant if else toggle below or a hashset instead O(1) lookups.
        std::string value_new;
        if (value == nullptr) {
            value_new = "";
        } else {
            value_new = value;
        }

        std::string_view option {arg};

        // NOTE: compare() returns zero on match, non-zero on mismatch, values must be checked against zero
        //      - to avoid truthy values trap.

        if (option.compare("-h") == 0 || option.compare("--help") == 0) {
            return make_option(OptionType::HELP, value_new);

        } else if (option.compare("-v") == 0 || option.compare("--version") == 0) {
            return make_option(OptionType::VERSION, value_new);

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

        } else if (option.compare("--update") == 0) {
            return make_option(OptionType::UPDATE, value_new);

        } else if (option.compare("--mode") == 0){
            return make_option(OptionType::MODE, value_new);

        }else if (option.compare("--type") == 0){
            return make_option(OptionType::BUILD_TYPE, value_new);

        // arguments after -- are passed to the resulting binary/build
        } else if (option.compare("--") == 0) {
            return make_option(OptionType::CLI_ARGS, value_new);

        } else if (option.empty() == true) {
            return make_option(OptionType::HELP, "print help");

        } else {
            return make_option(OptionType::ILLEGAL, "Unknown option passed!!");

        }
    }


    Option Config::check_arg_map(const char* arg, const char* value) {
        auto option = known_options.find(arg);

        // if the option is not found check whether the user passed a short form of the option.
        // if not a shorthand then it is an illegal token. 
        if (option == known_options.end()) {
            std::string short_arg {arg};

            if (short_arg.compare("-h") == 0) {
                option = known_options.find("--help");
        
            } else if (short_arg.compare("-v") == 0) {
                option = known_options.find("--version");
            
            } else {
                return Option(OptionType::ILLEGAL, "Unknown option encountered");
            }
    
        }

        option->second.value = value;
        return option->second;
    
    }
    
    bool Config::parse_config() {
        using json = nlohmann::json;

        //FIX: boolean returns insufficient, find a more informative type. Need to know what exactly failed.   

        // check if the global config file exists. if not try to create it.
        if (fs::exists(expand_path(cman::utils::GLOBAL_CONFIG_FILE))) {
            // expand the tilde path correctly.
            std::ifstream f (expand_path(cman::utils::GLOBAL_CONFIG_FILE));
            this->global_config = json::parse(f);

        } else {
            cman::print_message("Failed to open global config file!! Skipping.....", ERROR);
        }
        
        // check for a local config. might need project context and project traversal.. 
        fs::path local_config; 
        auto result = find_local_config(fs::current_path());
        
        if (!result.has_value()) {
            cman::print_message("Local config file not found!! Skipping.....", WARNING);
            return false;
        
        } else {
            local_config = result.value();
            std::ifstream f (local_config.string());
            this->local_config = json::parse(f);
        }

        return true;
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

            case cman::OptionType::BUILD_TYPE:
                return (Option) {
                    .type = OptionType::BUILD_TYPE,
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

    // iterate over the parent folders looking for the target files.
    std::optional<fs::path> find_local_config(fs::path start) {
        start = fs::absolute(start);

        while (!start.empty()) {
            fs::path target = start / "cman.json";
            
            if (fs::exists(target)) return target;

            fs::path parent = start.parent_path();

            // add a check to ensure if we,re at the root we stop moving up.
            if (parent == start) break;

            start = parent;
        }

        return std::nullopt;
    }
}}

#ifdef CMAN_TESTS

namespace tests {
    bool test_lexer() {
        // test the lexers parsing logic.
        return true;
    }

}

#endif // CMAN_TESTS
