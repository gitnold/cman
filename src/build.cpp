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
#include <vector>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace cman {
inline namespace v1 {
    // Check if the current directory is a cman project root.
    bool is_project_root() {
        return fs::exists("./src") && fs::is_directory("./src");
    }

    // Incremental build helper that recompiles only saved/modified source files.
    bool compile_incremental(const BuildConfig& config) {
        if (!is_project_root()) {
            cman::print_message("Not in project root directory! Cman project structure missing ('src/' directory not found).", ERROR);
            return false;
        }

        std::string proj_name = config.project_name.empty() ? fs::current_path().filename().string() : config.project_name;
        
        try {
            fs::create_directories("./bin");
            fs::create_directories("./debug");
        } catch (const fs::filesystem_error& e) {
            cman::print_message(e.what(), ERROR);
            return false;
        }

        FileStates& states = FileStates::instance();
        states.load_json();

        bool headers_modified = false;
        for (const char* dir_path : {"./include", "./src"}) {
            if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
                for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
                    if (entry.is_regular_file()) {
                        auto ext = entry.path().extension();
                        if (ext == ".h" || ext == ".hpp") {
                            auto mod = states.was_modified(entry.path().string());
                            if (mod.has_value() && mod.value()) {
                                headers_modified = true;
                            }
                        }
                    }
                }
            }
        }

        std::vector<fs::path> src_files;
        for (const auto& entry : fs::recursive_directory_iterator("./src")) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension();
                if (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx") {
                    src_files.push_back(entry.path());
                }
            }
        }

        if (src_files.empty()) {
            cman::print_message("No source files found in src/", WARNING);
            return false;
        }

        std::string compiler = (config.language == Lang::CPP) ? "g++" : "gcc";
        std::string flags = (config.language == Lang::CPP) ? "-std=c++23 -Wall -Wextra" : "-std=c17 -Wall -Wextra";
        if (config.mode == BuildMode::RELEASE) {
            flags += " -O3";
        } else if (config.mode == BuildMode::DEBUG) {
            flags += " -g";
        }

        std::vector<std::string> obj_files;
        bool recompiled_any = false;
        bool compile_failed = false;

        for (const auto& src_file : src_files) {
            std::string obj_name = src_file.stem().string() + ".o";
            fs::path obj_path = fs::path("./debug") / obj_name;
            obj_files.push_back(obj_path.string());

            auto mod = states.was_modified(src_file.string());
            bool file_modified = mod.has_value() ? mod.value() : true;
            bool obj_missing = !fs::exists(obj_path);

            if (file_modified || obj_missing || headers_modified) {
                std::string cmd = std::format("{} -c {} -o {} -Iinclude -Isrc {}", compiler, src_file.string(), obj_path.string(), flags);
                cman::print_message(std::format("Compiling {}...", src_file.filename().string()).c_str(), INFO);
                int status = std::system(cmd.c_str());
                if (status != 0) {
                    cman::print_message(std::format("Failed to compile {}", src_file.string()).c_str(), ERROR);
                    compile_failed = true;
                    break;
                }
                states.update_access_time(src_file.string());
                recompiled_any = true;
            }
        }

        if (compile_failed) {
            return false;
        }

        fs::path bin_path = fs::path("./bin") / proj_name;

        if (recompiled_any || !fs::exists(bin_path)) {
            std::string objs_str;
            for (const auto& obj : obj_files) {
                objs_str += obj + " ";
            }
            std::string link_cmd = std::format("{} {} -o {} {}", compiler, objs_str, bin_path.string(), flags);
            cman::print_message(std::format("Linking {}...", bin_path.string()).c_str(), INFO);
            int link_status = std::system(link_cmd.c_str());
            if (link_status != 0) {
                cman::print_message("Linking failed", ERROR);
                return false;
            }
            states.dump_state_to_json();
            cman::print_message("Build successful!", INFO);
        } else {
            cman::print_message("Project up to date, skipping compilation.", INFO);
        }

        return true;
    }

    // if nothing has changed then run binary else compile first.
    void run(std::string_view project_name) {
        if (!is_project_root()) {
            cman::print_message("Not in project root directory! Cman project structure missing ('src/' directory not found).", ERROR);
            return;
        }
        std::system(std::format("./bin/{}", project_name).c_str());
    }

    void build(const BuildConfig& config) {
        // try to move into the project root first before trying to build the project.
        try {
            if (config.project_path.empty()) {
                cman::print_message("Project path is not set, cannot build", ERROR);
                return;
            }
            fs::current_path(config.project_path);

        } catch (const fs::filesystem_error& e) {
            cman::print_message(e.what(), ERROR);
            return;
        }

        if (!is_project_root()) {
            cman::print_message("Not in project root directory! Cman project structure missing ('src/' directory not found).", ERROR);
            return;
        }

        switch (config.build) {
            case cman::BuildType::SHELL_SCRIPT:
                if (!fs::exists("build.sh")) generate_build_sh(config);
                compile_bash(config.project_name);
                break;

            case cman::BuildType::BUILD_FILE:
                compile_incremental(config);
                break;

            case cman::BuildType::CMAKE:
                try {
                    fs::create_directories("./build/");
                    fs::current_path("./build/");
                    std::system("cmake");
                } catch (const fs::filesystem_error& e) {
                    cman::print_message(e.what(), ERROR);
                }
                break;

            case cman::BuildType::MAKE:
                compile_make();
                break;

            default:
                print_message("Unknown build type", ERROR);
        }

    }

    void generate_build_sh(const BuildConfig& config) {
        if (fs::exists("./build.sh")) return;
        if (is_project_root() || (fs::current_path().filename() == config.project_name)) {
            std::ofstream shell_script("build.sh");

            if (config.language == Lang::CPP) {
                shell_script << "g++";
            } else {
                shell_script << "gcc";
            }

            const char* src_ext = (config.language == Lang::CPP) ? "*.cpp" : "*.c";
            shell_script << " ./src/" << src_ext << " -o ./bin/" << config.project_name << " -Wall -Wextra\n";

            std::system("chmod +x build.sh");
            shell_script.close();

        } else {
            cman::print_message("Cman project format mismatch. Try initializing current directory with 'cman --init'", ERROR);
        }
    }

    bool compile_bash(std::string_view project_name) {
        if (!is_project_root()) {
            cman::print_message("Not in project root or src/ directory missing", ERROR);
            return false;
        }

        BuildConfig defaultConfig;
        defaultConfig.project_name = std::string(project_name);
        return compile_incremental(defaultConfig);
    }

    void compile_make() {
        if (!is_project_root()) {
            cman::print_message("Not in project root", ERROR);
            return;
        }
        std::system("make");
    }

    FileStates& FileStates::instance() {
        static FileStates inst;
        return inst;
    }

    std::expected<bool, cman::FsError> FileStates::was_modified(std::string filename) {
        if (!fs::exists(filename)) {
            return std::unexpected(FsError::NOT_FOUND);
        }

        auto last_access = fs::last_write_time(filename);
        auto it = this->access_times.find(filename);
        if (it == this->access_times.end() || last_access != it->second) {
            this->update_access_time(filename);
            return true;
        }
        return false;
    }

    void FileStates::update_access_time(std::string filename) {
        if (fs::exists(filename)) {
            this->access_times.insert_or_assign(filename, fs::last_write_time(filename));
        }
    }

    void FileStates::load_json() {
        if (this->json_file.empty()) {
            this->json_file = "cman.json";
        }
        if (!fs::exists(this->json_file)) {
            return;
        }

        std::ifstream file(this->json_file);
        if (!file.is_open()) {
            return;
        }

        try {
            nlohmann::json json_obj = nlohmann::json::parse(file);
            if (json_obj.contains("access_times") && json_obj["access_times"].is_object()) {
                for (auto& [key, val] : json_obj["access_times"].items()) {
                    if (val.is_number()) {
                        int64_t count_val = val.get<int64_t>();
                        this->access_times[key] = fs::file_time_type(fs::file_time_type::duration(count_val));
                    }
                }
            }
        } catch (...) {
            // TODO: fill this cathc block.
            // Failed parsing json, proceed cleanly
        }
    }

    void FileStates::dump_state_to_json() {
        if (this->json_file.empty()) {
            this->json_file = "cman.json";
        }

        nlohmann::json json_obj;
        if (fs::exists(this->json_file)) {
            std::ifstream file(this->json_file);
            if (file.is_open()) {
                try {
                    json_obj = nlohmann::json::parse(file);
                } catch (...) {
                    json_obj = nlohmann::json::object();
                }
            }
        }

        nlohmann::json times_obj;
        for (const auto& [filename, time_val] : this->access_times) {
            times_obj[filename] = time_val.time_since_epoch().count();
        }
        json_obj["access_times"] = times_obj;

        std::ofstream out(this->json_file);
        if (out.is_open()) {
            out << json_obj.dump(4) << "\n";
        }
    }

}}

