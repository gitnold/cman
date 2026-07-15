#include "semantic_analysis.h"
#include "build.h"
#include "filesystem.h"
#include "parser.h"
#include "style.h"
#include "utils/config_templates.h"
#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <vector>


// TODO: create an action list of sorts.

namespace cman {
inline namespace v1 {
    namespace utils {
        std::vector<std::string> split_string(std::string str) {
            std::stringstream stream(str);
            std::string word;
            std::vector<std::string> words;

            while (stream >> word) {
                words.push_back(word);
            }

            return words;
        }
    }

    bool BuildCtx::validate() {
        return this->buildtype_set && this->bin_path_set && this->lang_set && this->project_folder_exists;
    }

    bool ProjectCtx::validate() {
        return this->projectname_set;
    }

    SemanticAnalyzer::SemanticAnalyzer(const ParsedInput& parser_output, std::optional<json> g_config, std::optional<json> l_config) : parsed(parser_output) {
        this->build_ctx = BuildCtx {};
        this->project_ctx = ProjectCtx {};
        this->build_config = BuildConfig {};

        // FIX: use moves instead of copies??
        this->g_config = g_config;
        this->l_config = l_config;
    }
    
    // NOTE: parsing mostly required for first time runs, else previous files or build commands already there!!
    ResultType SemanticAnalyzer::analyze() {
        // NOTE: code below unreachable if user doesnt initialize a project.
        if (this->parsed.project_name.has_value() && this->parsed.init_project) {
            this->build_config.project_name = parsed.project_name.value();
            build_config.project_path = "./" + parsed.project_name.value();
            build_config.bin_path = build_config.project_path + "/bin/";

            project_ctx.projectname_set = true;
            build_ctx.bin_path_set = true;

            this->action_list.push_back(ActionType::NEW_PROJECT);
        }

        // check whether to initialize git or not.
        project_ctx.initialize_git = parsed.init_git ? true : false;
        if (project_ctx.initialize_git) {
            this->action_list.push_back(ActionType::INIT_GIT);
        }

        // if build/run is requested but no project was specified, use the current directory name
        if ((parsed.build_project || parsed.run_bin) && build_config.project_name.empty()) {
            build_config.project_name = fs::current_path().filename().string();
        }
        if (build_config.project_path.empty()) {
            build_config.project_path = ".";
        }

        // FIXED: added a has_value check to prevent UB.
        if (parsed.language.has_value() && !parsed.language->empty()) {
            std::string lang = *parsed.language;

            // convert to lowercase.
            for (auto& ch: lang) ch = std::tolower(ch);

            if (lang == "c") {
                build_config.language = Lang::C;
                build_ctx.lang_set = true;
            } else if (lang == "cpp" || lang == "c++" || lang == "cxx") {
                build_config.language = Lang::CPP;
                build_ctx.lang_set = true;
            } else {
                // generate an error and suggestion.
                cman::print_message("Unsupported language type! Try --lang c/cpp/cxx/c++", ERROR);
                return ResultType::ILLEGAL;
            }
        }

        // TODO: verify build context first
        // FIX: check below fails as some values are not yet set at this point.
        if (parsed.build_project || parsed.run_bin) {
            if (!build_ctx.lang_set) {
                print_message("--build/--run requires --lang c or --lang cpp", ERROR);
                return ResultType::ILLEGAL_FORMAT;
            }

            if (!build_ctx.buildtype_set) {
                build_config.build = BuildType::SHELL_SCRIPT;
            }
            if (!build_ctx.buildmode_set) {
                build_config.mode = BuildMode::DEFAULT;
            }

            if (parsed.build_type_explicit) {
                this->build_config.build = parsed.build_type;
                build_ctx.buildtype_set = true;
            }
            if (parsed.mode_explicit) {
                this->build_config.mode = parsed.mode;
                build_ctx.buildmode_set = true;
            }

            // --build or --run requires atleast --lang and --mode set, check this before building the build struct.
            // NOTE: build module already consumes the config struct, might as well pass the struct directly.

            this->action_list.push_back(ActionType::BUILD);
            // push the run action to the action list.
            if (parsed.run_bin) {
                this->action_list.push_back(ActionType::RUN);
            }

            if (parsed.bin_args.has_value() && parsed.run_bin) {
                // FIX: instead of splitting, convert to c string instead.
                build_config.arguments = utils::split_string(parsed.bin_args.value());
            }

        }

        if (parsed.init_dir && parsed.init_project) {
            print_message("Cannot initialize a project and directory at once,choose on action", ERROR);
            return ResultType::ILLEGAL_FORMAT;
        }

        if (parsed.init_dir) {
            // initialize the current directory.
            // The initialize_current_dir() function already extracts the current working directory;
            // no further processing needed.
            // Setup an action and exit.

            this->action_list.push_back(ActionType::INIT_PROJECT);

        }

        return ResultType::OK;
    }


    ResultType SemanticAnalyzer::analyze_configs() {
        // precedence: cli > local > global.
        // options to override: buildmode, buildtype, language
        // global config serves as the base, local config overrides it,
        // and analyze() later overrides with explicit CLI args.

        auto apply_config = [this](const json& cfg) {
            if (cfg.contains("language") && cfg["language"].is_string()) {
                std::string lang = cfg["language"];
                for (auto& ch : lang) ch = std::tolower(ch);
                if (lang == "c") {
                    build_config.language = Lang::C;
                    build_ctx.lang_set = true;
                } else if (lang == "cpp" || lang == "c++" || lang == "cxx") {
                    build_config.language = Lang::CPP;
                    build_ctx.lang_set = true;
                }
            }

            if (cfg.contains("build mode") && cfg["build mode"].is_string()) {
                std::string mode = cfg["build mode"];
                for (auto& ch : mode) ch = std::tolower(ch);
                if (mode == "release") {
                    build_config.mode = BuildMode::RELEASE;
                    build_ctx.buildmode_set = true;
                } else if (mode == "debug") {
                    build_config.mode = BuildMode::DEBUG;
                    build_ctx.buildmode_set = true;
                } else if (mode == "default") {
                    build_config.mode = BuildMode::DEFAULT;
                    build_ctx.buildmode_set = true;
                }
            }

            if (cfg.contains("build type") && cfg["build type"].is_string()) {
                std::string type = cfg["build type"];
                for (auto& ch : type) ch = std::tolower(ch);
                if (type == "script" || type == "shell script") {
                    build_config.build = BuildType::SHELL_SCRIPT;
                    build_ctx.buildtype_set = true;
                } else if (type == "make") {
                    build_config.build = BuildType::MAKE;
                    build_ctx.buildtype_set = true;
                } else if (type == "cmake") {
                    build_config.build = BuildType::CMAKE;
                    build_ctx.buildtype_set = true;
                }
            }

            if (cfg.contains("bin path") && cfg["bin path"].is_string()) {
                build_config.bin_path = cfg["bin path"];
                build_ctx.bin_path_set = true;
            }

            if (cfg.contains("project path") && cfg["project path"].is_string()) {
                build_config.project_path = cfg["project path"];
            }

            if (cfg.contains("project name") && cfg["project name"].is_string()) {
                build_config.project_name = cfg["project name"];
                project_ctx.projectname_set = true;
            }

            if (cfg.contains("build command") && cfg["build command"].is_string()) {
                build_config.build_command = cfg["build command"];
            }

            if (cfg.contains("tasks") && cfg["tasks"].is_array()) {
                std::vector<std::string> tasks;
                for (const auto& task : cfg["tasks"]) {
                    if (task.is_string()) {
                        tasks.push_back(task);
                    }
                }
                if (!tasks.empty()) {
                    build_config.tasks = tasks;
                }
            }
        };

        if (g_config.has_value() && !g_config->is_null()) {
            apply_config(g_config.value());
        }

        if (l_config.has_value() && !l_config->is_null()) {
            apply_config(l_config.value());
        }

        return ResultType::OK;
    }


    // TODO: remove all side effects from the parser, have a short-circuit flag or switch mechanism to exit the loop or set in the parse input
    // e.g cman --new bin -h should only print the help menu as cman does not support submenus yet.
    // cman <...options> --update should ideally update after doing everything or ask whether to use the newer version to be installed.
    void SemanticAnalyzer::execute() {
        for (ActionType action : this->action_list) {
            switch (action) {
                case ActionType::BUILD:
                    cman::build(this->build_config);
                    break;

                // NOTE: action today might be redundant as update should short circuit in the parser stage.
                case ActionType::UPDATE_CMAN:
                    //cman::utils::self_update(utils::UpdateMode::SRC);
                    break;

                case cman::ActionType::INIT_GIT:
                    initialize_git();
                    break;

                case cman::ActionType::INIT_PROJECT:
                    initialize_current_dir(this->build_config);
                    break;

                case cman::ActionType::NEW_PROJECT:
                    cman::initialize_newbin_project(this->build_config.project_name, this->build_config);
                    break;

                case cman::ActionType::RUN:
                    // TODO: wire this to the filestates tracker to have conditional builds otherwise --run is useless??.
                    cman::run(this->build_config.project_name);
                    break;

                default:
                    cman::print_message("Unknown Action requested", DEBUG);
            }
        }
    }
}}
