//NOTE: abort immediately you encounter an illegal token, no need to continue.
#include "parser.h"
#include "build.h"
#include "cli.h"
#include "filesystem.h"
#include "style.h"
// #include <print>
#include <string>
#include <vector>
#include "semantic_analysis.h"
#include "utils/self_update.h"
#include "utils/help_menu.h"
// #include "build.h"

//FIX: fis the hashing its non-deterministic.

namespace cman {
inline namespace v1 {
    Parser::Parser(std::vector<cman::Option> options) {
        //TODO: save the project name somewhere.
        this->options = options;  // avoid taking a copy of options
        ResultType status = get_optiontypes();
        if (status == ResultType::OK) {
            construct_hash();
            evaluate();
        }
        //TODO: show that an illegal token was found and posssibly what it was.
        //TODO: find a way to use the returned error values.
    }



    Parser::~Parser() {
        //do nothing.
    }
    
    // TODO: check code correctness.
    ResultType Parser::parse() {
        
        // build context.
        BuildCtx build_context;
        ProjectCtx project_context;

        
        for (Option option : this->options) {
            switch (option.type) {
                case OptionType::HELP:
                    this->parse_result.has_help = true;
                    cman::print_help();
                    return ResultType::OK;
                    break;

                case OptionType::NEW:
                    // project context set.
                    if (option.value.empty() == 0) {
                        cman::print_message("--new cannot have an empty project name", ERROR);
                        return ResultType::ILLEGAL_FORMAT;
                    }
                    this->parse_result.project_name = option.value;
                    this->parse_result.init_project = true;
                    
                    project_context.projectname_set = true;
                    build_context.bin_path_set = true; // add support for overriding this later.
                    break;

                case OptionType::GIT:
                    this->parse_result.init_git = true;
                    project_context.initialize_git = true;
                    break;

                case OptionType::BUILD:
                    // make sure mandatory config options are set before dispatching a build.
                    if (option.value.empty()) {
                        cman::print_message("--build option must take a value! Try --build script/build/make/cmake", ERROR);
                        return ResultType::ILLEGAL;
                    }
                    this->parse_result.build_project = true;
                    break;

                case OptionType::RUN:
                    this->parse_result.build_project = true;
                    this->parse_result.run_bin = true;
                    break;

                case OptionType::LANGUAGE:
                    if (option.value.empty()) {
                        cman::print_message("--lang <language e.g  c/cpp> cannot have an empty value", ERROR);
                        return ResultType::EMPTY_TOKEN;
                    } 

                    build_context.lang_set = true;

                    this->parse_result.language = option.value;
                    break;
                
                case OptionType::INIT:
                    // project context set.
                    this->parse_result.init_dir = true; 
                    break;

                case OptionType::MODE:
                    if (option.value.empty()) {
                        // use the default mode.
                    } else {

                    }

                // print version information and exit.
                case cman::OptionType::VERSION:
                    cman::help_menu::print_version_info();
                    return ResultType::OK;
                    break;
                
                // the update option should short circuit(no need for further computution), no other feasible combination.
                case cman::OptionType::UPDATE:
                    cman::utils::self_update(utils::UpdateMode::SRC);
                    return ResultType::OK;
                
                case cman::OptionType::CLI_ARGS:
                    if (!parse_result.run_bin) {
                        cman::print_message("commands line arguments for the built binary should be put after the --run option", ERROR);
                        return ResultType::ILLEGAL_FORMAT; 
                    }
                    
                    if (!option.value.empty()) {
                        // process provided arguments.
                        // NOTE: only trigger this if the --run is used.
                        parse_result.bin_args = option.value;
                    } else {
                        cman::print_message("-- option was used but no commands were passed. Commands are space separated", WARNING);
                        cman::print_message("Try removing `--` or pass arguments instead e.g -- -f 6", INFO);
                    }
                    break;
                default:
                    cman::print_message("Illegal token found!!", ERROR);
                    cman::print_help();
                    return ResultType::ILLEGAL;
            }
        }
    }

    //FIX: abort when an illegal option is encountered.
    ResultType Parser::get_optiontypes() {
        for (Option option: this->options) {
            //HACK: design a custom viewer fucntion to visualize the pipeline.
            //debug line below
            // std::println("Option type->{} : option value->{}",static_cast<int>(option.type), option.value);

            if (option.type != cman::OptionType::ILLEGAL) {
                this->tokens.push_back(option.type);
                if (option.type == cman::OptionType::NEW) {
                    this->project_name = option.value;
                }
            } else {
                return ResultType::ILLEGAL;
            }
        }
        return ResultType::OK;
    }

    //TODO: do sth like a result type for this function for error handling.
    [[deprecated("function no longer used")]]
    ResultType Parser::construct_hash() {
        std::string hash_str;

        if (this->tokens.empty()) {
            return ResultType::EMPTY_TOKEN;

        }
        //HACK: try byte arrays for speed up.
        for (OptionType token: this->tokens) {
            hash_str.append(std::to_string(static_cast<int>(token)));
            // hash += static_cast<int>(token);
        }
        this->hash = {.value = hash_str};
        return ResultType::OK;
    }

    //NOTE: find a better altenative to branching, like a hashmap.
    ResultType Parser::evaluate() {

        // std::println("Hash value {}", this->hash.value);
        // FIX: for `--init` options set project_name to current directory.

        if (this->hash.value.compare("03") == 0) {
            cman::initialize_newbin_project(this->project_name);
            cman::initialize_git();

        } else if (this->hash.value.compare("04") == 0) {
            return ResultType::UNIMPLEMENTED;

        } else if (this->hash.value.compare("034") == 0) {
            return ResultType::UNIMPLEMENTED;

        } else if (this->hash.value.compare("23") == 0) {
            cman::initialize_current_dir();
            //FIX: overload or fix git + init variants where project is current directory.
            cman::initialize_git();

        } else if (this->hash.value.compare("1") == 0 || this->hash.value.empty()) {
            //print help.
            cman::print_message("Printing help", INFO);
            cman::print_help();

        } else if (this->hash.value.compare("2") == 0) {
            cman::initialize_current_dir();

        } else if (this->hash.value.compare("0") == 0) {
            cman::initialize_newbin_project(this->project_name);

        // --run option.
        } else if (this->hash.value.compare("5") == 0) {
            //cman::run(this->project_name);

        //--build option
        } else if (this->hash.value.compare("6") == 0) {
            //cman::build();
        } else if (this->hash.value.compare("7") == 0) {
        
        } else {
            return ResultType::ILLEGAL_FORMAT;
        }

        return ResultType::OK;
    }

}}
