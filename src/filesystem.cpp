#include "../include/filesystem.h"
#include <array>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>
#include "../include/style.h"

//TODO: add robust error handling.
//TODO: add checks to prevent reinitialization of existing projects.

//NOTE: add support fpr reorganizing current projects.

namespace fs = std::filesystem;

namespace cman {
inline namespace v1 {
    void initialize_git() {
        //TODO: hide this error or format for the user.
        int check_git = std::system("git --version");
        if (WEXITSTATUS(check_git) == 0) {
            int git_repo = system("git init");
            std::string git_ignore {"bin\ndebug\nassets\n"};
            
            //TODO: wrap logic below in a try block.
            std::ofstream gitfile (".gitignore");
    
            if (!gitfile) {
                print_message("Error opening file", WARNING);
                return;
            }

            //writing to file.
            gitfile << git_ignore << std::endl; 
            gitfile.close();

            print_message("Successfully written to .gitignore", DEBUG);
        }

    }
    
    //TODO: overload function below or add a switch case to avoid code repetition.
    int initialize_newbin_project(std::string project_name) {
        std::array<std::string, 4> dirs { "bin", "include", "src", "debug"};
        std::error_code err;
        
        //FIX: check if the directories exist.
        print_message("Creating a new binary project", DEBUG);
        for (std::string dir : dirs) {
            fs::create_directories("./" + project_name + "/" + dir, err);
            if (err.value() != 0) {
                print_message(err.message().c_str(), ERROR);
                return EXIT_FAILURE;
            } else {
                std::string message {"Creating " + dir + " directory..."};
                print_message(message.c_str(), INFO);
            }
        }
        //TODO: generate a main.cpp file and put some boilerplate code.
        return EXIT_SUCCESS;
    }

    void initialize_current_dir() {
        //get the current working directory.
        std::string current_dir = fs::current_path().filename();
        int status = initialize_newbin_project(current_dir);
        if (status == EXIT_FAILURE) return;

        print_message("Modyfying current directory", DEBUG);
        
        //TODO: try to detect bash build scripts, makefiles and cmake files.
        try {
            for (const auto& entry : fs::directory_iterator(current_dir)) {
                //TODO: add a loading animation for process below.
                if (entry.is_regular_file()) {
                    fs::path extension = entry.path().extension();
                    auto perms = entry.status().permissions();

                    if (extension == ".h") {
                        //copy files to include directory.
                        fs::copy(entry.path(), "./include/");

                    //TODO: add support for other c++ file extensions
                    } else if (extension == ".cpp" || extension == ".c") {
                        //copy files to src directory..
                        fs::copy(entry.path(), "./src/");
                    
                    } else if ((perms & fs::perms::others_exec) != fs::perms::none ||
                            (perms & fs::perms::group_exec) != fs::perms::none ||
                            (perms & fs::perms::owner_exec) != fs::perms::none){
                        //copy binary files to bin directory..
                        fs::copy(entry.path(), "./bin/");
                    } else {
                        continue;  //NOTE: this might be unnecessary.
                    }
                }
            }
        } catch (const fs::filesystem_error& e) {
            print_message(e.what(), ERROR); 
        }
    }


}}
