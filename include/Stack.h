#ifndef STACK_H
#define STACK_H

#include "LinkedList.h"

template<class T>
class Stack {
public:
    void push(const T& value) { items_.insertHead(value); }
    bool pop() noexcept { return items_.removeHead(); }
    const T* peek() const noexcept { return items_.front(); }
    bool isEmpty() const noexcept { return items_.isEmpty(); }
    void clear() noexcept { items_.clear(); }

private:
    LinkedList<T> items_;
};

#endif
