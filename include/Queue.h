#ifndef QUEUE_H
#define QUEUE_H

#include "LinkedList.h"

template<class T>
class Queue {
public:
    void enqueue(const T& value) { items_.insertTail(value); }
    bool dequeue() noexcept { return items_.removeHead(); }
    const T* front() const noexcept { return items_.front(); }
    bool isEmpty() const noexcept { return items_.isEmpty(); }
    void clear() noexcept { items_.clear(); }
    template<class Predicate>
    void removeMatching(Predicate matches) { items_.removeMatching(matches); }

private:
    LinkedList<T> items_;
};

#endif
