#include "build.h"
// #include "cli.h"
#include "style.h"
#include "json.hpp"
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <string_view>
#include <sys/stat.h>

//TODO: add build caching??

// finish setting up the file access time hashmaps.
// switch to file content hashing when switching to dependency grqph.


namespace fs = std::filesystem;

//TODO: add make support then focus on `build.zig`-like functionality.
//cmake = command execution no custom logic.
namespace cman {
inline namespace v1 {
    //if nothing has changed then run binary else compile first.
    void run(std::string_view project_name) {

        std::system(std::format("$BIN/{}", project_name).c_str());
    }

    //FIX: possible lifetime issue with using a reference.
    void build(const BuildConfig& config) {

        // try to move into the project root first before trying to build the project.
        try {
            fs::current_path(config.project_path) ;

        } catch (const fs::filesystem_error& e) {
            cman::print_message(e.what(), ERROR);

        }

        switch (config.build) {
            case cman::BuildType::SHELL_SCRIPT:
                if (!fs::exists("build.sh")) generate_build_sh(config);
                compile_bash(config.project_name);
                break;

            case cman::BuildType::BUILD_FILE:
                //compile the file as a standalone binary and run it.
                break;

            case cman::BuildType::CMAKE:
                try {
                    //TODO: try to create the build folder.
                    fs::current_path(config.project_name + "/build/");
                    std::system("cmake"); //TODO: add sane cmake defaults.
                } catch (const fs::filesystem_error& e) {
                    cman::print_message(e.what(), ERROR);
                }
                break;

            case cman::BuildType::MAKE:
                compile_make();
                break;

            default:
                //NOTE: possible dead path below
                print_message("Unknown build type", ERROR);
        }

    }

    void generate_build_sh(const BuildConfig& config) {
        //TODO: add options for standards, release builds, language selection.
        if (fs::exists("./build.sh")) return;
        if (fs::exists("./src/") || (fs::current_path().filename() == config.project_name)) {
            std::ofstream shell_script("build.sh");
            //TODO: have boiler plates sit in utils.

            if (config.language == Lang::CPP) {
                shell_script << "g++";
            } else {
                shell_script << "gcc";
            }

            shell_script << " ./src/*.cpp -o ./bin/" <<  config.project_name << " -Wall -Wextra\n";
            std::system("chmod +x build.sh");
            shell_script.close();

        } else {
            cman::print_message("Cman project format mismatch. Try initializing current directory with 'cman --init'", ERROR);
        }
    }

    //HACK: remove function overhead for cmake and make as they're just command executions??
    bool compile_bash(std::string_view project_name) {
        //TODO: add a flag that shows the current set build system.
        if (fs::exists("./build.sh") && fs::current_path().filename() == project_name) {
            int status = std::system("./build.sh");
            if (status == 0) {
                return true;
            } else return false;

        } else {
            cman::print_message("Not in project root or build.sh missing", ERROR);
            return false;
        }
    }

    void compile_make() {
        //TODO: add make targets support.
        std::system("make");
    }

    //NOTE: function below a possible chokepointn, watch out when profiling.
    //TODO: try to avoid the expensive string copies.
    //TODO: implement file state tracking.
    //FIX: std::unexpected might be unnecessary here.
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
        //this->access_times = json_obj;
    }

    void FileStates::dump_state_to_json() {
        nlohmann::json json_obj;

        //TODO: eliminate loop below.
        for (const auto& pair : this->access_times) {
            json_obj[pair.first] = pair.second.time_since_epoch().count();
        }
    }

}}
