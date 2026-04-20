#ifndef CMAN_LINKED_LIST_H
#define CMAN_LINKED_LIST_H

namespace cman {
    template <typename T>
    struct Node {
        // use a pointer or reference??
        Node* next;
        T data;
    };
    struct LinkedList {

    };
}

#endif // !CMAN_LINKED_LIST_H

