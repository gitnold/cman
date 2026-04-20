#include "ast.h"
// #include "cli.h"
#include <iostream>
#include <new>
#include <sys/types.h>
#include <vector>
// #include <vector>

#define DATA(d) template<typename d>

namespace cman {
inline namespace v1 {
    template<typename U>
    Arena<U>::Arena(uint size) {
        this->capacity = size;
        this->current_size = 0;
        try {
            // try to allocate memory for the arena.
            this->arena = new U[size]();
        } catch (std::bad_alloc) {
            std::cerr << "Allocation failed" << std::endl;
        }
    }
  
    template<typename U>
    Arena<U>::~Arena(){
        // deallocate the arena.
        delete [] this->arena;
    }

    // add a node to the arena.
    template<typename U>
    uint Arena<U>::add_node(U data) {
        if (this->current_size < this->capacity) {
            uint nodeid = this->current_size;

            if (this->deleted_nodes.empty()) {
                // ensure arena has space else reallocate. 
                this->arena[this->current_size] = data;
                this->current_size++;
                return nodeid;
            } else {
                uint id = this->deleted_nodes.back();
                this->deleted_nodes.pop_back();
                return id;
            }
        } else {
            //copy existing elements, resize the arena? or just switch to a freaking hashmap or vector.
            // investigate possible indices changes.
            return 0;
        }
    }
    
    template<typename U>
    bool Arena<U>::delete_node(uint node_id) {
        if (this->current_size > 0) {
            // is there need of setting the index below to zero or you can just overwrite it in the next write.
            //FIX: possible operator issue and 
            this->arena[node_id] = 0;
            this->deleted_nodes.push_back(node_id);
            return true;
     } else {
            return false;
        }
    }

    template<typename D>
    ActionList<D>::ActionList(std::vector<Option> options) {
        //FIX: check if theres any copy constructor related possible memory leaks.
        this->arena = Arena<ActionNode<Option>>(options.size());

        for (auto option : options) {
            // store indices as they are more reallocation friendly.
            uint node_id = this->arena.add_node((ActionNode<Option>){option, this->arena.current_size});
            this->node_ids.push_back(node_id);
        }
    }
    
}}
