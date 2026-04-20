#ifndef CMAN_SELF_UPDATE_H
#define CMAN_SELF_UPDATE_H

#include <string>

namespace cman {
inline namespace v1 {
    namespace utils {
        enum class UpdateMode {
            SRC,
            BIN
        };

        // self update struct info.
        struct SelfUpdate {
            std::string cman_bin_path;
            std::string cman_src_path;
            std::string current_version;
            std::string target_version;
            UpdateMode update_type;
        };
        //TODO: store the target version somewhere for reference.
        const std::string LINUX_BIN_PATH {"~/.local/bin/"};
        const std::string WINDOWS_BIN_PATH {"windows equivalent"};
        const std::string CONFIG_FILE {"~/.comfig/cman/cman.json"};
        
        inline const std::string github_repo {"https://github.com/gitnold/cman.git"};
        inline SelfUpdate UpdateConfig{
            .cman_bin_path = "~/local/bin/",
            .cman_src_path = "~/Applications/",
            .current_version = "0.1",
            .target_version = "",
            .update_type = UpdateMode::SRC
        };
        void self_update(UpdateMode update_type);
        // void check_version();
        bool compile_cman();
        
        // possible security issue below.
        bool download_binary();
        bool update_binary(std::string bin_src_path);
    }
}}

#endif // !CMAN_SELF_UPDATE_H
