// test project creation.
#include <array>
#include <print>
#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

///fixes
constexpr std::string_view CMAN_BIN_PATH {"bin/dev/cman-default"};
constexpr std::array<std::string_view, 4> paths_to_check {
    "src/",
    "bin/",
    "include/",
    "debug/"
};
/// end of fix

bool verify_project_structure() {
    auto current_path = fs::current_path();
    auto test_project_path {current_path / "test_project"};

    bool main_file_exists = fs::exists(test_project_path / "src/main.cpp") || fs::exists(test_project_path / "src/main.c");

    return std::all_of(paths_to_check.begin(), paths_to_check.end(), [&test_project_path, main_file_exists](const fs::path& p) {
       return fs::exists(test_project_path / p) && main_file_exists;
    });

}

int main() {
    auto current_path = fs::current_path();
    auto test_project_path = current_path / "test_project";

    if (fs::exists(test_project_path)) {
        fs::remove_all(test_project_path);
    }

    auto command = std::string(CMAN_BIN_PATH) + " --new test_project --lang c --git";
    std::system(command.c_str());

    // things to check for.
    // 1. whether the .git folder exists or return value of git status.
    // 2. whether the folder has the folder structure and the main file is equal to sth else.
    // 3. whether a c or cpp file was created.

    bool test_result = verify_project_structure();
    if (test_result) {
        std::println("Test : Test project creation passed !!!!");
    } else {
        std::println("Test : Test project creation failed !!!!");
    }

    if (fs::exists(test_project_path)) {
        fs::remove_all(test_project_path);
    }

    return test_result ? 0 : 1;
}
