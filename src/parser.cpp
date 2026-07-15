//NOTE: abort immediately you encounter an illegal token, no need to continue.
#include "parser.h"
#include "build.h"
#include "cli.h"
// #include "filesystem.h"
#include "style.h"
// #include <print>
#include <array>
#include <string>
#include <vector>
#include "semantic_analysis.h"
#include "utils/self_update.h"
#include "utils/help_menu.h"
// #include "build.h"


namespace cman {
inline namespace v1 {
    Parser::Parser(std::vector<cman::Option> options, LexMode mode) {
        this->options = options;  // avoid taking a copy of options
        [[maybe_unused]] ResultType status = get_optiontypes();

        if (mode == LexMode::CLI) {
            this->parse_result.cli_args = true;
        }

    }

    Parser::~Parser() {
        //do nothing.
    }

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
                    if (option.value.empty()) {
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
                    this->parse_result.mode_explicit = true;
                    if (option.value.empty()) {
                        this->parse_result.mode = BuildMode::DEFAULT;
                    } else {
                        if (option.value == "release") {
                            this->parse_result.mode = BuildMode::RELEASE;
                        } else if (option.value == "debug") {
                            this->parse_result.mode = BuildMode::DEBUG;
                        } else {
                            this->parse_result.mode = BuildMode::DEFAULT;
                            print_message("Unknown build mode was passed, using the default mode", INFO);
                        }
                    }
                    break;

                case cman::OptionType::BUILD_TYPE:
                    this->parse_result.build_type_explicit = true;
                    if (option.value.empty()) {
                        this->parse_result.build_type = BuildType::SHELL_SCRIPT;
                    } else {
                        if (option.value == "script") {
                            this->parse_result.build_type = BuildType::SHELL_SCRIPT;
                        } else if (option.value == "make") {
                            this->parse_result.build_type = BuildType::MAKE;
                        } else if (option.value == "cmake") {
                            this->parse_result.build_type = BuildType::CMAKE;
                        } else {
                            print_message("Unknown build type, using shell script", WARNING);
                            this->parse_result.build_type = BuildType::SHELL_SCRIPT;
                        }
                    }
                    break;

                // print version information and exit.
                case OptionType::VERSION:
                    cman::help_menu::print_version_info();
                    return ResultType::OK;
                    break;

                // the update option should short circuit(no need for further computution), no other feasible combination.
                case OptionType::UPDATE:
                    cman::utils::self_update(utils::UpdateMode::SRC);
                    return ResultType::OK;

                case OptionType::CLI_ARGS:
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
        return ResultType::OK;
    }

    //FIX: abort when an illegal option is encountered.
    ResultType Parser::get_optiontypes() {
        for (Option option: this->options) {
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

}}


