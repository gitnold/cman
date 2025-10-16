//NOTE: abort immediately you encounter an illegal token, no need to continue.
#include "../include/parser.h"
#include "../include/filesystem.h"
#include "../include/style.h"
// #include <print>
#include <string>
#include <vector>

//FIX: fis the hashing its non-deterministic.

namespace cman {
inline namespace v1 {
    Parser::Parser(std::vector<cman::Option> options) {
        //TODO: save the project name somewhere.
        this->options = options;
        ResultType status = get_optiontypes();
        if (status == ResultType::OK) {
            construct_hash();
            evaluate();
        }
        //TODO: show that an illegal token was found and posssibly what it was.
        //TODO: find a way to use the returned error values.
    }


    Parser::~Parser() {
        //do nothing.
    }
    
    //FIX: abort when an illegal option is encountered.
    ResultType Parser::get_optiontypes() {
        for (Option option: this->options) {
            //HACK: design a custom viewer fucntion to visualize the pipeline.
            //debug line below
            // std::println("Option type->{} : option value->{}",static_cast<int>(option.type), option.value);

            if (option.type != cman::OptionType::ILLEGAL) {
                this->tokens.push_back(option.type);
                if (option.type == cman::OptionType::NEW) {
                    this->project_name = option.value;
                }
            } else {
                return ResultType::ILLEGAL;
            }
        }
        return ResultType::OK;
    }
    
    //TODO: do sth like a result type for this function for error handling.
    ResultType Parser::construct_hash() {
        std::string hash_str;

        if (this->tokens.empty()) {
            return ResultType::EMPTY_TOKEN;

        }
        //HACK: try byte arrays for speed up.
        for (OptionType token: this->tokens) {
            hash_str.append(std::to_string(static_cast<int>(token)));
            // hash += static_cast<int>(token);
        }
        this->hash = {.value = hash_str};
        return ResultType::OK;
    }

    //NOTE: find a better altenative to branching, like a hashmap.
    ResultType Parser::evaluate() {
        
        // std::println("Hash value {}", this->hash.value);
        
        if (this->hash.value.compare("03") == 0) {
            cman::initialize_newbin_project(this->project_name);
            cman::initialize_git();

        } else if (this->hash.value.compare("04") == 0) {
            return ResultType::UNIMPLEMENTED;

        } else if (this->hash.value.compare("034") == 0) {
            return ResultType::UNIMPLEMENTED;

        } else if (this->hash.value.compare("23") == 0) {
            cman::initialize_current_dir();
            //FIX: overload or fix git + init variants where project is current directory.
            cman::initialize_git();

        } else if (this->hash.value.compare("1") == 0) {
            //print help.
            cman::print_message("Printing help", INFO);
            cman::print_help();

        } else if (this->hash.value.compare("2") == 0) {
            cman::initialize_current_dir();

        } else if (this->hash.value.compare("0") == 0) {
            cman::initialize_newbin_project(this->project_name);

        } else {
            return ResultType::ILLEGAL_FORMAT;
        }

        return ResultType::OK;
    }
    
}}
