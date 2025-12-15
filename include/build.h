#ifndef CMAN_BUILD_H
#define CMAN_BUILD_H

#include <expected>
#include <string_view>
#include <filesystem>
#include <unordered_map>

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

    //storage for various build configurations.
    // possible storage overhead.
    struct BuildConfig{
        BuildType build;
        std::string bin_path;
        BuildMode mode;
    };

    //acts like a "main" function for build.cpp taking away build logic from the parser stage.
    // TODO: have sth like a state variable that denotes what build system is set. Integrate with build.c later
    // HACK: consinder having a config json file.
    void build();

    //runs the compiled binary.
    void run(std::string_view project_name);
    void generate_build_sh();
    void compile_bash();
    void compile_make();

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
