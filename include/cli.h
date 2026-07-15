#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "json.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// maybe deprecated.
#define MAX_OPTIONS 3

namespace fs = std::filesystem;

// NOTE: have a unified/flexible config style that allows cross referencing.
// allow more features via the config files, add supoort for manual cli arguments 
// overrides later.

namespace cman {
inline namespace v1 {
    //consinder namespaces.
    enum class OptionType {
        NEW,
        HELP,       //consinder using a union for help/default.
        INIT,
        GIT,
        LIB,
        RUN,
        BUILD,
        LANGUAGE,
        UPDATE,
        MODE,
        VERSION,
        CLI_ARGS,
        BUILD_TYPE,
        ILLEGAL
    };

    struct Option {
        OptionType type;
        std::string value;  //try using unions for non-values params.
    };

    enum class LexMode {
        CLI,
        JSON
    };

    // TODO: consinder changing the value to std::optional.
    inline  std::unordered_map<std::string, Option> known_options = {
      {"--new", {OptionType::NEW, ""}},
      {"--help", {OptionType::HELP, ""}},
      {"--init", {OptionType::INIT, ""}},
      {"--git", {OptionType::GIT, ""}},
      {"--lib", {OptionType::LIB, ""}},
      {"--run",{OptionType::RUN, ""}},
      {"--build", {OptionType::BUILD, ""}},
      {"--lang", {OptionType::LANGUAGE, ""}},
      {"--update", {OptionType::UPDATE, ""}},
      {"--mode", {OptionType::MODE, ""}},
      {"--version", {OptionType::VERSION, ""}},
      {"--", {OptionType::CLI_ARGS,""}},
    };

    std::optional<fs::path> find_local_config(fs::path start);

    class Config {
        public:
            std::vector<std::string> args;
            int num_of_args;
            std::string package_name;
            nlohmann::json global_config;
            nlohmann::json local_config;
            std::vector<Option> options;

            Config(std::vector<std::string> args, LexMode mode);
            void parse();
            ~Config();

        private:
            bool parse_config();
            Option make_option(OptionType type, std::string value);
            Option check_arg(const char* arg, const char* value);
            Option check_arg_map(const char* arg, const char* value);

    };
}}
#endif // !CLI_PARSER_H
