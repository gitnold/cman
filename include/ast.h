#ifndef CMAN_AST_H
#define CMAN_AST_H

#include <sys/types.h>
#include <vector>

// TODO: switch to an action tree(linked list) instead of an ast tree.
// read on initializer lists.

namespace cman {
inline namespace v1 {
    template <typename T>
    struct ActionNode {
        T data;
        uint next_node;
    };

    template<typename U>
    class Arena {
        public:
            int capacity;
            int current_size;
            Arena(uint size);
            ~Arena();
            U get_node(uint node_id);
            uint add_node(U data); //returns a node id which is its position in the arena.
            bool delete_node(uint node_id);
        private:
            // keep track of deleted nodes positions to pack data for max storage utilization.
            std::vector<uint> deleted_nodes;
            U* arena;
            //TODO: rely on destructor vs custom delete function.
    };

    // ast class has a pointer to the root.
    // nodes store pointers but actual data is in the arena.
    // allocate the arena b4 trying to construct the tree.
    template<typename D>
    class ActionList{
        public:
            ActionList(std::vector<D> options);
            ~ActionList();
            Arena<ActionNode<D>>& get_arena();
            // construct the tree from the given list of tokens
            // walk the tree and produce an actionable result.
            void parse();
        private:
            Arena<ActionNode<D>>& arena;
            std::vector<uint> node_ids;
            
    };
}}


#endif // !CMAN_AST_H
