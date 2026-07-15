#include "utils/self_update.h"
#include "build.h"
#include "style.h"
#include "json.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>


// global config needed for version, source paths sync, persistent state etc.


namespace cman {
inline namespace v1 {
    namespace utils {
        
        namespace fs = std::filesystem;
        
        void self_update(UpdateMode update_type) {
            switch (update_type) {
                // binary downloads require a few security guarantees first.
                case UpdateMode::BIN:
                    download_binary();
                    break;
                
                case UpdateMode::SRC:
                    if (compile_cman()){
                        cman::print_message("Cman updated to version _ successfully", INFO);
                    } else {
                        cman::print_message("Failed to update cman", ERROR);
                    }
                    break;

                default:
                    cman::print_message("No update type Specified", DEBUG);
            }
        }  
        
                
        // compile cman from source and install it, should support various build systems.
        bool compile_cman() {
            try {
                std::string cman_path = expand_path(UpdateConfig.cman_src_path + "/cman/");
                if (fs::exists(cman_path)) {
                    fs::current_path(cman_path);
                    std::system("git pull");
                } else {
                    std::string src_path = expand_path(UpdateConfig.cman_src_path);
                    if (!fs::exists(src_path)) {
                        fs::create_directories(src_path);
                    }
                    fs::current_path(src_path);
                    std::system(("git clone " + github_repo).c_str());
                    if (fs::exists(cman_path)) {
                        fs::current_path(cman_path);
                    } else {
                        cman::print_message("Failed to clone cman repository", ERROR);
                        return false;
                    }
                }
            } catch (const fs::filesystem_error& e) {
                cman::print_message(e.what(), ERROR);
                return false;
            }
            
            cman::compile_bash("cman");
            return update_binary(expand_path(UpdateConfig.cman_src_path) + "bin/cman");
        }
        
        // updates the binary used by the system; downloads and puts it in path etc.
        // gets binary from a path and copies, downloads it from upstream otherwise(overload the function to handle such cases).
        // only support raw binary downloads when signing, checksums and other security features are supported.
        [[deprecated("Not really just unused for now!")]]
        bool download_binary() {
            return false;
        }        

        bool update_binary(std::string bin_src_path) {
            // copy the bin to .local bin on linux or symlink(if need be)
            // find windows equivalent.
            
            // find a way of supporting custom paths for the destination
            #if defined (__linux__)
                try {
                    fs::copy(expand_path(bin_src_path), expand_path(LINUX_BIN_PATH));
                    return true;
                } catch (const fs::filesystem_error& e) {
                    cman::print_message(e.what(), ERROR);
                    return false;
                }
            #elif defined(_WIN32)
                try {
                    fs::copy(expand_path(bin_src_path), expand_path(WINDOWS_BIN_PATH));
                    return true;
                } catch (const fs::filesystem_error& e) {
                    cman::print_message(e.what(), ERROR);
                    return false;
                }
            #endif
            
            // possibly add mac support.
            
            return false;
        }
    }
}}
