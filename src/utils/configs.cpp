#include "utils/configs.h"
#include "build.h"
#include "json.hpp"
#include "style.h"

namespace cman {
inline namespace v1 {
    namespace utils {
        bool dump_state_to_json(const BuildConfig& build_config) {
            using json = nlohmann::json;
            json config_values;

            // setup the json fields
            config_values["project name"] = build_config.project_name;
            config_values["version"] = 0.1;
            config_values["bin path"] = "";
            config_values["config path"] = "";
            config_values["project path"] = "";
            

            if (build_config.language == Lang::CPP) {
                config_values["language"] = "cpp";
            } else {
                config_values["language"] = "c";
            }

            // check the compilation mode.
            if (build_config.mode == BuildMode::DEBUG) {
                config_values["build mode"] = "debug";
            } else if (build_config.mode == BuildMode::DEFAULT) {
                config_values["build mode"] = "default";
            } else if (build_config.mode == BuildMode::RELEASE) {
                config_values["build mode"] = "release";
            }

            // check the build system selected.
            if (build_config.build == BuildType::BUILD_FILE) {
                config_values["build type"] = "build file";
            } else if (build_config.build == BuildType::CMAKE) {
                config_values["build type"] = "cmake";
            } else if (build_config.build == BuildType::MAKE) {
                config_values["build type"] = "make";
            } else if (build_config.build == BuildType::SHELL_SCRIPT) {
                config_values["build type"] = "shell script";
            }
            
            //
            config_values["build command"] =  build_config.build_command;
            
            // FIX: possible issues with skipping setting the values, try setting defaults maybe???
            if (build_config.tasks.has_value()) {
                config_values["tasks"] = build_config.tasks.value();
            } else {
                cman::print_message("No tasks set, skipping.... [possibly add tasks manually in cman.json]", INFO);
            }

            // NOTE: return values might be redundant??
            return true;
        }
    }
}}
