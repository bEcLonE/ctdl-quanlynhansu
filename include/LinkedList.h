#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <cstddef>
#include <type_traits>
#include <utility>

template<class T> class LinkedList;

template<class T>
class Node {
    friend class LinkedList<T>;
    explicit Node(const T& value) : value_(value) {}
    T value_;
    Node* next_ = nullptr;
};

template<class T>
class LinkedList {
public:
    LinkedList() = default;

    LinkedList(const LinkedList& other) {
        LinkedList copy;
        other.traverse([&](const T& value) { copy.insertTail(value); });
        swap(copy);
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            LinkedList copy(other);
            swap(copy);
        }
        return *this;
    }

    ~LinkedList() { clear(); }

    T& insertHead(const T& value) {
        auto* node = new Node<T>(value);
        node->next_ = head_;
        head_ = node;
        if (!tail_) {
            tail_ = node;
        }
        ++size_;
        return node->value_;
    }

    T& insertTail(const T& value) {
        auto* node = new Node<T>(value);
        if (tail_) {
            tail_->next_ = node;
        } else {
            head_ = node;
        }
        tail_ = node;
        ++size_;
        return node->value_;
    }

    bool removeHead() noexcept {
        if (!head_) {
            return false;
        }
        unlink(nullptr, head_);
        return true;
    }

    template<class Predicate>
    bool removeByKey(Predicate matches) {
        Node<T>* previous = nullptr;
        for (auto* node = head_; node; node = node->next_) {
            if (matches(node->value_)) {
                unlink(previous, node);
                return true;
            }
            previous = node;
        }
        return false;
    }

    template<class Predicate>
    void removeMatching(Predicate matches) {
        Node<T>* previous = nullptr;
        auto* node = head_;
        while (node) {
            auto* next = node->next_;
            if (matches(node->value_)) {
                unlink(previous, node);
            } else {
                previous = node;
            }
            node = next;
        }
    }

    template<class Predicate>
    const T* find(Predicate matches) const {
        for (const auto* node = head_; node; node = node->next_) {
            if (matches(node->value_)) {
                return &node->value_;
            }
        }
        return nullptr;
    }

    T* front() noexcept { return head_ ? &head_->value_ : nullptr; }
    const T* front() const noexcept { return head_ ? &head_->value_ : nullptr; }

    template<class Visitor>
    void traverse(Visitor visit) const {
        for (const auto* node = head_; node; node = node->next_) {
            visit(node->value_);
        }
    }

    template<class Visitor>
    void traverseMutable(Visitor visit) {
        for (auto* node = head_; node; node = node->next_) {
            visit(node->value_);
        }
    }

    std::size_t getSize() const noexcept { return size_; }
    bool isEmpty() const noexcept { return !head_; }

    void clear() noexcept {
        while (removeHead()) {
        }
    }

    // less must not throw while the links are being rearranged.
    template<class Less>
    void sort(Less less) {
        static_assert(std::is_nothrow_invocable_r_v<bool, const Less&, const T&, const T&>,
                      "LinkedList::sort requires a non-throwing comparator");
        head_ = mergeSort(head_, less);
        tail_ = head_;
        while (tail_ && tail_->next_) {
            tail_ = tail_->next_;
        }
    }

private:
    void swap(LinkedList& other) noexcept {
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
        std::swap(size_, other.size_);
    }

    void unlink(Node<T>* previous, Node<T>* node) noexcept {
        if (previous) {
            previous->next_ = node->next_;
        } else {
            head_ = node->next_;
        }
        if (node == tail_) {
            tail_ = previous;
        }
        delete node;
        --size_;
    }

    template<class Less>
    static Node<T>* mergeSort(Node<T>* head, const Less& less) {
        if (!head || !head->next_) {
            return head;
        }
        auto* middle = head;
        for (auto* fast = head->next_; fast && fast->next_; fast = fast->next_->next_) {
            middle = middle->next_;
        }
        auto* right = middle->next_;
        middle->next_ = nullptr;
        auto* left = mergeSort(head, less);
        right = mergeSort(right, less);
        Node<T>* result = nullptr;
        auto** next = &result;
        while (left && right) {
            auto*& selected = less(std::as_const(right->value_), std::as_const(left->value_))
                ? right : left;
            *next = selected;
            selected = selected->next_;
            next = &(*next)->next_;
        }
        *next = left ? left : right;
        return result;
    }

    Node<T>* head_ = nullptr;
    Node<T>* tail_ = nullptr;
    std::size_t size_ = 0;
};

#endif
