#include "../include/ast.h"
#include "../include/cli.h"



// cman new <name> --git --lib
// cman -h / cman
// cman new <name>
// cman new <name> --git
// cman init [--git]

//TODO: consinder using an evaluator directly instead of a parse tree.

namespace cmanast {
    inline namespace v1 {
        Ast::Ast(std::vector<cman::Option> options) {
            this->root = nullptr;
            // this->left_child = nullptr;
            // this->right_child = nullptr;
            this->hash = 0;
        }
        Ast::~Ast() {
            //delete this->root;
            delete_tree(this->root);
        }
        
        //new, init and help are the viable roots.
        //new --lib.
        //if root illegal or illegal elsewhere abort the program
        //FIX: dont see the need to construct a whole ast just for four options.
        //HACK: only construct the tree when necessary, for the new case option only.
        //TODO: check if there is a root first.
        void Ast::construct_tree() {
            for (cman::Option option : this->tokens) {
                //FIX: remove repettion below.
                switch (option.type) {
                    case cman::OptionType::ILLEGAL:
                        //abort the program.
                        break;
                    
                    //TODO: find out if structs can have constructors.
                    case cman::OptionType::GIT:
                        if (this->root == nullptr) {
                            this->root = new Node;
                            this->root->type = cmanast::NodeType::ROOT;
                            this->root->value = option;
                        } else {
                            //do sth.#TODO: traverse tree to find empty slot. 
                        }
                        break;

                    case cman::OptionType::HELP:
                        if (this->root == nullptr) {
                            this->root = new Node;
                            this->root->type = cmanast::NodeType::ROOT;
                            this->root->value = option;
                        } else {
                            //raise an error.


                        }
                        break;

                    case cman::OptionType::INIT:
                        //launch project in the current directory.
                        if (this->root ==  nullptr) {
                            this->root = new Node;
                            this->root->type = cmanast::NodeType::ROOT;
                            this->root->value = option;
                        } else {
                            //do sth.
                        }
                        break;

                    case cman::OptionType::NEW:
                        if (this->root == nullptr) {
                            this->root = new Node;
                            this->root->type = cmanast::NodeType::ROOT;
                            this->root->value = option;
                        } else {
                            //FIX: use actual traversal methods i.e bfs, dfs etc.
                            //FIX: logic below is fallible.
                            auto current_node = this->root;
                            while (current_node->left != nullptr) {
                                current_node = current_node->left;
                            }
                            current_node->left = new Node;
                            current_node->left->type = cmanast::NodeType::CHILD;
                            current_node->left->value = option;
                            //TODO: handle rigtht child.
                        }
                        break;
                    
                    case cman::OptionType::LIB:
                        //do sth
                        break;

                    default:
                        //do sth.
                        break;
                };
            }
        }

        int Ast::add_child(Node* node) {
            if (node->type != cmanast::NodeType::ROOT) {
                //do sth
            } else {
                //do sth else.
            }
            return 0;
        }


   
 }
}
