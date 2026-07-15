#include "cli.h"
#include "parser.h"
#include "style.h"
#include <cstdlib>
#include <print>
#include <string_view>
#include "semantic_analysis.h"


namespace cman {
inline namespace v1 {
    void add_cli_option(std::string_view option) {
        std::println("{}\t{}{}",GREEN, option, RESET);
    }
}}

void cman::print_help() {
    printf("%sWelcome to C package manager ^_^%s\n", BLUE, RESET);
    printf("%sUsage :%s\n", BLUE, RESET);
    cman::add_cli_option("--new <project_name>  ::  create a new project in the new directory 'project_name'");
    cman::add_cli_option("--init                ::  organize current working directory");
    cman::add_cli_option("--git                 ::  start a local repository in the project root");
    cman::add_cli_option("--build               ::  build the current project");
    cman::add_cli_option("--run                 ::  run built binary, builds the projects if any changes were made");
}

void cman::print_message(const char *message, cman::MessageType type) {
    switch (type) {
        case cman::INFO:
            printf("%s[INFO]  %s%s\n",GREEN, message, RESET);
            break;
        case cman::WARNING:
            printf("%s[WARNING] %s%s\n",YELLOW, message, RESET);
            break;
        case cman::ERROR:
            printf("%s[ERROR]  %s%s\n",RED, message, RESET);
            break;
        case cman::DEBUG:
            printf("%s[DEBUG] %s%s\n", BLUE, message, RESET);
            break;
        default:
            printf("%sWrite a default fallback%s\n",RED, RESET);
    }
}
//TODO: how cargo detects file changes.

int main(int argc, char** argv) {
    // TODO: make the lex mode dynamic.

    std::vector<std::string> args(argv, argv + argc);


    auto lex_mode = cman::LexMode::CLI;
    auto cli = cman::Config(args, lex_mode);
    cli.parse();

    //launch the evaluator.
    auto parser = cman::Parser(cli.options, lex_mode);

    //if parser returns a non-okay status, return failure, leave details to the semantic analysis.
    // NOTE: pissible logic bug below.
    if (parser.parse() != cman::ResultType::OK) {
        return EXIT_FAILURE;
    }

    /// fixed confgi files missing.
    std::optional<nlohmann::json> g_config = cli.global_config.is_null() ? std::nullopt : std::optional<nlohmann::json>(cli.global_config);
    std::optional<nlohmann::json> l_config = cli.local_config.is_null() ? std::nullopt : std::optional<nlohmann::json>(cli.local_config);
    cman::SemanticAnalyzer semantic_analyzer = cman::SemanticAnalyzer(parser.parse_result, g_config, l_config);
    if (semantic_analyzer.analyze_configs() == cman::ResultType::OK &&
        semantic_analyzer.analyze() == cman::ResultType::OK) {
        /// -----------------------------------------
        semantic_analyzer.execute();
    }
    return EXIT_SUCCESS;
}

// TODO: IMPORTANT: switch to an action tree asap.
//TODO: add detection to ensure some operations are ran in the correct location e.g project root.
//TODO: do i need a rest api for the cman registry.
//NOTE: allow sb to configure paths to use
//NOTE: support for compile_commands.json

//NOTE: consinder having animations e.g for progress tracking.
// add 'bear' support or use it as dependency.
// add support for self update and self install
