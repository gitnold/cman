#ifndef CMAN_FILESYSTEM_H
#define CMAN_FILESYSTEM_H

//TODO: figure out a better approach.
#include <string_view>
namespace cman {
    inline namespace v1 {
        int initialize_newbin_project(std::string project_name);
        void initialize_git();
        void initialize_current_dir();
        //void initialize_newlib_project();
    }

}

#endif
