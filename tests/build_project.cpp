#include "build.h"
#include "style.h"
#include <print>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace cman { inline namespace v1 {
    void print_message(const char *message, MessageType type) {
        (void)type;
        std::println("[TEST LOG] {}", message);
    }
}}

#include "../src/build.cpp"

namespace fs = std::filesystem;

bool test_filestates_modified_and_json() {
    std::string test_file = "dummy_test_file.txt";
    std::ofstream out(test_file);
    out << "initial content\n";
    out.close();

    cman::FileStates& states = cman::FileStates::instance();

    // 1. Initial check (untracked / modified)
    auto mod1 = states.was_modified(test_file);
    if (!mod1.has_value() || !mod1.value()) {
        std::println("FAIL: Expected was_modified to return true for new file");
        fs::remove(test_file);
        return false;
    }

    // 2. Second check immediately (unmodified)
    auto mod2 = states.was_modified(test_file);
    if (!mod2.has_value() || mod2.value()) {
        std::println("FAIL: Expected was_modified to return false for unchanged file");
        fs::remove(test_file);
        return false;
    }

    // 3. Modify file content after delay to ensure filesystem timestamp update
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::ofstream out2(test_file, std::ios::app);
    out2 << "added content\n";
    out2.close();

    auto mod3 = states.was_modified(test_file);
    if (!mod3.has_value() || !mod3.value()) {
        std::println("FAIL: Expected was_modified to return true for modified file");
        fs::remove(test_file);
        return false;
    }

    // 4. Persistence test to cman.json
    states.dump_state_to_json();
    if (!fs::exists("cman.json")) {
        std::println("FAIL: cman.json was not created");
        fs::remove(test_file);
        return false;
    }

    states.load_json();
    auto mod4 = states.was_modified(test_file);
    if (!mod4.has_value() || mod4.value()) {
        std::println("FAIL: Expected was_modified to return false after loading persisted state from cman.json");
        fs::remove(test_file);
        if (fs::exists("cman.json")) fs::remove("cman.json");
        return false;
    }

    fs::remove(test_file);
    if (fs::exists("cman.json")) fs::remove("cman.json");
    return true;
}

int main() {
    std::println("Testing FileStates and build functionality...");
    bool pass = test_filestates_modified_and_json();
    if (pass) {
        std::println("Test : FileStates & cman.json tracking passed !!!!");
        return 0;
    } else {
        std::println("Test : FileStates & cman.json tracking failed !!!!");
        return 1;
    }
}


