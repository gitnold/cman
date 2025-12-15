#ifndef CMAN_AST_H
#define CMAN_AST_H

#include <vector>
#include <string>
#include "../include/cli.h"

namespace cman {
inline namespace v1 {
    struct Node {
        std::string data;
        Node* left_child;
        Node* right_child;
    };

    class Arena {
        public:
            int capacity;
            int size;
            Arena();
            ~Arena();
            bool add_node(); //returns a node id which is its position in the arena.
            bool delete_node(uint node_id);
        private:
            // keep track of deleted nodes to pack data for max storage utilization.
            std::vector<uint> deleted_nodes;
            Node* arena;

    };

    // ast class has a pointer to the root.
    // nodes store pointers but actual data is in the arena.
    // allocate the arena b4 trying to construct the tree.
    class Ast{
        public:
            Ast(std::vector<Option> options);
            ~Ast();
            // construct the tree from the given list of tokens
            void construct_tree();
            // walk the tree and produce an actionable result.
            void parse();
        private:
            std::vector<Option> tokens;
    };
}}


#endif // !CMAN_AST_H
