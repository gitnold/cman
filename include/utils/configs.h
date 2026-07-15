#ifndef CMAN_CONFIGS_H
#define CMAN_CONFIGS_H

#include "build.h"
namespace cman {
inline namespace v1 {

    namespace utils {
        bool dump_state_to_json(const BuildConfig& build_config);
    }

}
}

#endif // !CMAN_CONFIGS_H
