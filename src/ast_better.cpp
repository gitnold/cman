#include "ast_better.h"
#include <iostream>
#include <new>

// TODO: meaning of explicit and const in a function context
#include <print>
#include <stdexcept>
#include <algorithm> // for std::copy
#include <utility>   // for std::exchange, std::move


namespace cman {
inline namespace v1 {

    // -------------------------------------------------------------------------
    // Arena Implementation
    // -------------------------------------------------------------------------

    template<typename U>
    Arena<U>::Arena(size_type size) : capacity(size), current_size(0), arena(nullptr) {
        if (size > 0) {
            try {
                this->arena = new U[size](); // Default initialize
            } catch (const std::bad_alloc& e) {
                std::cerr << "Arena allocation failed: " << e.what() << std::endl;
                throw;
            }
        }
    }

    template<typename U>
    Arena<U>::~Arena() {
        delete[] this->arena;
    }

    // Copy Constructor
    template<typename U>
    Arena<U>::Arena(const Arena& other)
        : capacity(other.capacity),
          current_size(other.current_size),
          deleted_nodes(other.deleted_nodes) {
        if (other.capacity > 0) {
            this->arena = new U[other.capacity];
            // Deep copy of the data
            // Note: This copies potentially uninitialized/unused slots too if U is trivial.
            // For complex types, only active slots should ideally be copied, but tracking them requires more state.
            // Assuming U is copyable.
            std::copy(other.arena, other.arena + other.capacity, this->arena);
        } else {
            this->arena = nullptr;
        }
    }

    // Copy Assignment Operator
    template<typename U>
    Arena<U>& Arena<U>::operator=(const Arena& other) {
        if (this != &other) {
            U* new_arena = nullptr;
            if (other.capacity > 0) {
                new_arena = new U[other.capacity];
                std::copy(other.arena, other.arena + other.capacity, new_arena);
            }
            
            delete[] this->arena;
            
            this->arena = new_arena;
            this->capacity = other.capacity;
            this->current_size = other.current_size;
            this->deleted_nodes = other.deleted_nodes;
        }
        return *this;
    }

    // Move Constructor
    template<typename U>
    Arena<U>::Arena(Arena&& other) noexcept
        : capacity(std::exchange(other.capacity, 0)),
          current_size(std::exchange(other.current_size, 0)),
          deleted_nodes(std::move(other.deleted_nodes)),
          arena(std::exchange(other.arena, nullptr)) {
    }

    // Move Assignment Operator
    template<typename U>
    Arena<U>& Arena<U>::operator=(Arena&& other) noexcept {
        if (this != &other) {
            delete[] this->arena;
            
            this->capacity = std::exchange(other.capacity, 0);
            this->current_size = std::exchange(other.current_size, 0);
            this->deleted_nodes = std::move(other.deleted_nodes);
            this->arena = std::exchange(other.arena, nullptr);
        }
        return *this;
    }

    template<typename U>
    U& Arena<U>::get_node(size_type node_id) {
        if (node_id >= current_size) {
            throw std::out_of_range("Arena::get_node: Invalid node_id");
        }
        return arena[node_id];
    }

    template<typename U>
    const U& Arena<U>::get_node(size_type node_id) const {
        if (node_id >= current_size) {
            throw std::out_of_range("Arena::get_node: Invalid node_id");
        }
        return arena[node_id];
    }

    template<typename U>
    typename Arena<U>::size_type Arena<U>::add_node(const U& data) {
        // BUG FIX: Original code did not assign 'data' when reusing a deleted node ID.
        if (!deleted_nodes.empty()) {
            size_type id = deleted_nodes.back();
            deleted_nodes.pop_back();
            arena[id] = data; // Fixed: Data assignment added.
            return id;
        }

        if (current_size < capacity) {
            arena[current_size] = data;
            return current_size++;
        }

        // IMPROVEMENT: Auto-resize instead of returning 0 or failing silently.
        // If capacity is 0, start with 8. Else double it.
        size_type new_capacity = (capacity == 0) ? 8 : capacity * 2;
        U* new_arena = new U[new_capacity]; // May throw bad_alloc
        
        // Move existing data
        if (arena) {
            // usage of std::move for potentially move-only types
            for (size_type i = 0; i < current_size; ++i) {
                new_arena[i] = std::move(arena[i]);
            }
            delete[] arena;
        }
        
        arena = new_arena;
        capacity = new_capacity;
        
        arena[current_size] = data;
        return current_size++;
    }

    template<typename U>
    bool Arena<U>::delete_node(size_type node_id) {
        if (node_id < current_size) {
            // BUG FIX: Original code assigned '0' (int) to U, which might not be valid for all types.
            // this->arena[node_id] = 0; 
            
            // Instead, we can just destruct it if needed, or rely on it being overwritten later.
            // For POD types, nothing needed. For classes, maybe reset to default?
            // arena[node_id] = U(); 
            
            deleted_nodes.push_back(node_id);
            return true;
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // ActionList Implementation
    // -------------------------------------------------------------------------

    template<typename D>
    ActionList<D>::ActionList(const std::vector<D>& options) 
        : arena(options.size() + 5) { // Pre-allocate with some slack
        
        // BUG FIX: Original code used a temporary Arena assigned to a reference member.
        // Here 'arena' is a member object initialized in the initializer list.
        
        this->options = options;
        
        for (const auto& option : options) {
            // BUG OBSERVATION: The original code initialized next_node with current_size.
            // Since current_size is the index of the node *being* added, next_node pointed to itself.
            // Intention likely was to link them, but without context, 0 (or MAX_UINT) is safer than self-loop 
            // unless self-loop implies "end of chain".
            // We will initialize next_node to 0 (null-ish) for now.
            
            ActionNode<D> node;
            node.data = option;
            node.next_node = 0; 
            
            unsigned int node_id = this->arena.add_node(node);
            this->node_ids.push_back(node_id);
        }
    }
    
    template<typename D>
    void ActionList<D>::construct_list() {
        ActionNode<D> previous;
        
        // TODO: check copy assignemt related logic bugs.
        // FIX: traversal logic below.
        for (auto option : this->options) {
            previous = this->arena.add_node(option);
        }
    }

    template<typename U>
    ActionNode<U> ActionList<U>::get_start_node() {
        return this->arena.get_node(0);
    }

    template<typename U>
    ActionNode<U> ActionList<U>::get_node(unsigned int node_id) {
        return this->arena.get_node(node_id);
    }
}}
