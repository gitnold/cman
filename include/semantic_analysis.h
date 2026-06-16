#ifndef CMAN_SEMANTIC_ANALYSIS_H
#define CMAN_SEMANTIC_ANALYSIS_H

#include "build.h"
#include "json.hpp"
#include "parser.h"
#include <optional>
#include <vector>


// semantic context.
// before triggering a build : lang must be set, paths must be defined, BuildType must be set.
// before initialization: project name must be set.
// git cannot be done in empty directory.
namespace cman {
inline namespace v1 {
    enum class ActionType {
        BUILD,
        RUN,
        INIT_GIT,
        NEW_PROJECT,
        INIT_PROJECT,
        UPDATE_CMAN
    };
    
    
    
    struct BuildCtx {
        bool lang_set = false;
        bool bin_path_set = false;
        bool buildtype_set = false;
        bool project_folder_exists = false;
        bool buildmode_set = false;
        explicit operator bool() const;
        bool validate();
    };
    
    struct ProjectCtx {
        bool projectname_set = false;
        bool initialize_git = false;
        explicit operator bool() const;
        bool validate();
    };

    class SemanticAnalyzer {
        using json = nlohmann::json; 

        BuildConfig build_config;
        BuildCtx build_ctx;
        ProjectCtx project_ctx;
        std::optional<json> g_config;
        std::optional<json> l_config;
        std::vector<ActionType> action_list;

        public:
            const ParsedInput& parsed;
            SemanticAnalyzer(const ParsedInput& parser_output, std::optional<json> g_config, std::optional<json> l_config);
            ResultType analyze();
            ResultType analyze_configs();
            void execute();
    };
    
}}

#endif
