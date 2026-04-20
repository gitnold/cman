#ifndef CMAN_AST_BETTER_H
#define CMAN_AST_BETTER_H

#include <vector>

namespace cman {
inline namespace v1 {
    template <typename T>
    struct ActionNode {
        T data;
        unsigned int next_node;
    };

    template<typename U>
    class Arena {
        public:
            using value_type = U;
            using size_type = unsigned int;

            size_type capacity;
            size_type current_size;

            explicit Arena(size_type size);
            ~Arena();

            // Rule of Five
            Arena(const Arena& other);
            Arena& operator=(const Arena& other);
            Arena(Arena&& other) noexcept;
            Arena& operator=(Arena&& other) noexcept;

            U& get_node(size_type node_id);
            const U& get_node(size_type node_id) const;
            
            // Returns the index of the added node
            size_type add_node(const U& data);
            
            // Marks a node as deleted so its slot can be reused
            bool delete_node(size_type node_id);

        private:
            std::vector<size_type> deleted_nodes;
            U* arena;
    };

    template<typename D>
    class ActionList {
        public:
            // Expects a vector of data items (e.g., Options)
            explicit ActionList(const std::vector<D>& options);
            ~ActionList() = default; // Rule of Zero: members handle their own cleanup
            void construct_list();
            ActionNode<D> get_start_node();
            ActionNode<D> get_node(unsigned int node_id);
        
        private:
            const std::vector<D>& options;
            Arena<ActionNode<D>> arena;
            std::vector<unsigned int> node_ids;
    };

}}

#endif // !CMAN_AST_BETTER_H
