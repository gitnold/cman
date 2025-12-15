#include "../include/build.h"
#include "../include/style.h"
#include "../lib/json.hpp"
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <string_view>
#include <sys/stat.h>

//TODO: add incremental compilations and caching.
//NOTE: incremental compilation abit tricky focus on detecting changes first.



//TODO: a hashmap of filenames to last access time.
//TODO:

namespace fs = std::filesystem;
//TODO: add make support then focus on `build.zig`-like functionality.
//cmake = command execution no custom logic.
namespace cman {
inline namespace v1 {
    //if nothing has changed then run binary else compile first. 
    void run(std::string_view project_name) {
        // define the build directory path using the build conficuration methods.
        std::system(std::format("$BIN/{}", project_name).c_str());
    }

    void generate_build_sh(std::string_view project_name) {
        //TODO: check if youre in the project root first.
        //add options for standards, release builds, language selection.
        if (fs::exists("./src/") || (fs::current_path().filename() == project_name)) {
            std::ofstream shell_script("build.sh");
            shell_script << "g++ ./src/*.cpp -o ./bin/" <<  project_name << " -Wall -Wextra\n";
            std::system("chmod +x build.sh");
            shell_script.close();

        } else {
            cman::print_message("Cman project format mismatch. Try initializing current directory", ERROR);
        }
    }

    //HACK: remove function overhead for cmake and make as they're just command executions.
    void compile_bash() {
        //TODO: check if build.sh or Makefile exists in the first place.
        //TODO: add a flag that shows the current set build system.
        std::system("./build.sh");
    }

    void compile_make() {
        std::system("make");
    }

    //NOTE: function below a possible chokepointn, watch out when profiling.
    //TODO: try to avoid the expensive string copies.
    std::expected<bool, cman::FsError> FileStates::was_modified(std::string filename) {

        if(fs::exists(filename)) {
            auto last_access = fs::last_write_time(filename);

            //FIX: try the .at() method inorder to handle cases where the file is not in the map.
            //above may eliminate the outer if scope checking for the file exists operation.
            if (last_access != this->access_times[filename]) {
                update_access_time(filename);
                return true;
            }
            return false;
        } else {
            return std::unexpected(FsError::NOT_FOUND);
        }
    }

    void FileStates::update_access_time(std::string filename) {
        this->access_times.insert({filename, fs::last_write_time(filename)});
    }

    //FIX: construct the hashmap from json correctly.
    void FileStates::load_json() {
        nlohmann::json json_obj;
        this->access_times = json_obj;
    }

    void FileStates::dump_state_to_json() {
        nlohmann::json json_obj;
        
        //TODO: eliminate loop below.
        for (const auto& pair : this->access_times) {
            json_obj[pair.first] = pair.second.time_since_epoch().count();
        }
    }

}}
