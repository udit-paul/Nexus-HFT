#pragma once

#include <cassert>

namespace nexus {

// Forward declaration
struct OrderNode;

// An intrusive doubly-linked list of OrderNodes
class IntrusiveList {
public:
    IntrusiveList() : head_(nullptr), tail_(nullptr) {}

    void push_back(OrderNode* node);
    void remove(OrderNode* node);
    
    OrderNode* head() const { return head_; }
    OrderNode* tail() const { return tail_; }
    bool empty() const { return head_ == nullptr; }

private:
    OrderNode* head_;
    OrderNode* tail_;
};

} // namespace nexus
