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
            std::string cman_path {UpdateConfig.cman_src_path + "/cman/"};
            if (fs::exists(cman_path)) {
                fs::current_path(cman_path);

                // TODO: do some error handling.
                std::system("git pull");

            } else {
                // move to the location first
                // FIX: get status of shell commands first. 
                fs::current_path(UpdateConfig.cman_src_path);
                std::system(("git clone " + github_repo).c_str());
                fs::current_path(cman_path);
            }
            
            cman::compile_bash();
            return update_binary(UpdateConfig.cman_src_path + "bin/cman");
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
                fs::copy(bin_src_path, LINUX_BIN_PATH); 
                return true;
            #elif defined(_WIN32)
                fs::copy(bin_src_path, WINDOWS_BIN_PATH); 
                return true;
            #endif
            
            // possibly add mac support.
            
            return false;
        }
    }
}}
