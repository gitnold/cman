#ifndef CMAN_AST_H
#define CMAN_AST_H

#include "../include/cli.h"
#include <vector>


//FIX: do i really need a new namespace or can i add to an existing one.
namespace cmanast {
    inline namespace v1 {
        enum class NodeType {
            ROOT,
            CHILD
        };
        struct Node {
            NodeType type;
            cman::Option value;
            Node* left;
            Node* right;
        };
        class Ast {
            public:
                Ast(std::vector<cman::Option> options);
                ~Ast();
                Node* root;
                //Node* left_child;
                //Node* right_child;
                int hash;
                void construct_tree();
                int add_child(Node* node);
                int parse_tree();
                int delete_tree(Node* root);
            private:
                std::vector<cman::Option> tokens;

        };
    }
}
    
#endif
