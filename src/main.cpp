#include "../include/cli.h"
#include "../include/parser.h"
#include "../include/style.h"
#include <cstdlib>
#include <print>
#include <string_view>

//TODO: modify the helper message below.
//TODO: add support for git initialization only i.e cman --git

namespace cman {
inline namespace v1 {
    void add_cli_option(std::string_view option) {
        std::println("{}\t{}{}",GREEN, option, RESET);
    }   
}}

void cman::print_help() {
    //FIX: consinder creating a helper fucntion.
    printf("%sWelcome to C package manager ^_^%s\n", BLUE, RESET);
    printf("%sUsage :%s\n", BLUE, RESET);
    cman::add_cli_option("--new <project_name>  ::  create a new project in the new directory 'project_name'");
    cman::add_cli_option("--init                ::  organize current working directory");
    cman::add_cli_option("--git                 ::  start a local repository in the project root");
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

//FIX: everything seems to initialize a new binary project contrary to expected behaviour.
int main(int argc, char** argv) {

    //launch the cli argument parser.
    auto cli = cman::Config(argv, argc);
    cli.parse();
    
    //launch the evaluator.
    auto parser = cman::Parser(cli.options);


    return EXIT_SUCCESS; 
}

//CHECKLIST.
//NOTE: git initialization works
//NOTE: cman --new <project name> works -- not anymore hahaha
//NOTE: hashing seems to work as intended.
//-h flag is fucking bonkers!!!! - does its own shit.

