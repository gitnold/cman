#include "semantic_analysis.h"
#include "build.h"
#include "filesystem.h"
#include "parser.h"
#include "style.h"
#include "utils/self_update.h"
#include "utils/config_templates.h"
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

    SemanticAnalyzer::SemanticAnalyzer(const ParsedInput& parser_output) : parsed(parser_output) {
        this->build_ctx = BuildCtx {};
        this->project_ctx = ProjectCtx {};
        this->build_config = BuildConfig {};
    }

    ResultType SemanticAnalyzer::analyze() {
        if (this->parsed.project_name && this->parsed.init_project) {
            this->build_config.project_name = parsed.project_name.value();
            build_config.project_path = "./" + parsed.project_name.value();
            build_config.bin_path = build_config.project_path + "/bin/";

            project_ctx.projectname_set = true;
            build_ctx.bin_path_set = true;

            this->action_list.push_back(ActionType::NEW_PROJECT);
        }

        project_ctx.initialize_git = parsed.init_git ? true : false;
        if (project_ctx.initialize_git) {
            this->action_list.push_back(ActionType::INIT_GIT);
        }

        if (!parsed.language->empty()) {
            if (parsed.language == "c") {
                build_config.language = Lang::CPP;
                build_ctx.lang_set = true;
            } else if (parsed.language == "cpp") {
                build_config.language = Lang::CPP;
                build_ctx.lang_set = true;
            } else {
                // generate an error and suggestion.
                cman::print_message("Unsupported language type! Try --lang c/cpp", ERROR);
                return ResultType::ILLEGAL;
            }
        }

        // TODO: verify build context first
        if (parsed.build_project && build_ctx.validate()) {
            // setup the build config.
            // use defaults as placeholders for now. 
            // NOTE: code below might be unnecessary, pass the build config already created. 
            this->build_config = (BuildConfig) {
                .build = BuildType::SHELL_SCRIPT,
                .bin_path = build_config.bin_path,
                .mode = BuildMode::DEFAULT,
                .project_name = parsed.project_name.value(),
                .project_path = build_config.project_path,
                .language = build_config.language,
            };
            // NOTE: build module already consumes the config struct, might as well pass the struct directly.
            
            this->action_list.push_back(ActionType::BUILD);
            if (parsed.bin_args.has_value() && parsed.run_bin) {
                // FIX: instead of splitting, convert to c string instead.
                build_config.arguments = utils::split_string(parsed.bin_args.value());
            }
            
        }

        if (parsed.init_dir) {
            // initialize the current directory.
            // The initialize_current_dir() function already extracts the current working directory;
            // no further processing needed.
            // Setup an action and exit.
           
            this->action_list.push_back(ActionType::INIT_PROJECT);
            
        }
    }

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
                    initialize_current_dir();
                    break;

                case cman::ActionType::NEW_PROJECT:
                    initialize_newbin_project(this->build_config.project_name);
                    break;

                case cman::ActionType::RUN:
                    cman::build(this->build_config);
                    break;

                default:
                    cman::print_message("Unknown Action requested", DEBUG);
            }
        }
    }
}}
