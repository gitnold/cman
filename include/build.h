#ifndef CMAN_BUILD_H
#define CMAN_BUILD_H

// #include "cli.h"
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace cman {
inline namespace v1 {
    enum class FileState {
        MODIFIED,
        ACCESSED,
        NOT_MODIFIED
    };
    enum class FsError{
        NOT_FOUND,
        PERMISSION_DENIED
    };

    enum class BuildType {
        SHELL_SCRIPT,
        MAKE,
        CMAKE,
        BUILD_FILE
    };
    //below maybe unnecessary.
    enum class BuildMode {
        DEBUG,
        RELEASE,
        DEFAULT
    };
    enum class Lang {
        C,
        CPP
    };

    //storage for various build configurations.
    // possible storage overhead.
    // make fields like BuildType private but expose various constructors that set them under the hood.
    // have separate bin paths based on buildmode i.e targets
    //
    // TODO: capture project path on initialization for use in various stuff.
    struct BuildConfig{
        BuildType build;
        std::string bin_path;
        BuildMode mode;
        std::string project_name;
        std::string project_path;
        Lang language;
        std::vector<std::string> arguments;
        std::string build_command;
        std::optional<std::vector<std::string>> tasks;
    };

    // global state to handle build details
    // TODO: remove inline def below, opt for references.
    // inline BuildConfig Config;

    //acts like a "main" function for build.cpp taking away build logic from the parser stage.
    // TODO: have sth like a state variable that denotes what build system is set. Integrate with build.c later
    // HACK: consinder having a config json file.
    void build(const BuildConfig& config);

    //runs the compiled binary.
    void run(std::string_view project_name);
    void generate_build_sh(const BuildConfig& config);
    bool compile_bash(std::string_view project_name);
    void compile_make();
    std::string generate_build_command(const BuildConfig& build_config);

    //TODO: find the best place to store the file path.
    class FileStates {
        public:
            static FileStates& instance();
            std::expected<bool, FsError> was_modified(std::string filename);
            void update_access_time(std::string filename);
            void load_json();
            void dump_state_to_json();
        private:
            std::string json_file;
            FileStates() = default;
            std::unordered_map<std::string, fs::file_time_type> access_times;
    };
}}


#endif
