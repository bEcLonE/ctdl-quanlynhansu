#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <memory>
#include <string>
#include <string_view>
#include <utility>

template<class T>
class AvlTree {
    class Entry {
    public:
        Entry(std::string_view key, const T& value) : key_(key), value_(value) {}
        std::string key_;
        T value_;
        int height_ = 1;
        std::unique_ptr<Entry> left_;
        std::unique_ptr<Entry> right_;
    };
    using Link = std::unique_ptr<Entry>;

public:
    bool insert(std::string_view key, const T& value) {
        return insert(root_, key, value);
    }

    const T* search(std::string_view key) const noexcept {
        auto* entry = root_.get();
        while (entry && key != entry->key_) {
            entry = key < entry->key_ ? entry->left_.get() : entry->right_.get();
        }
        return entry ? &entry->value_ : nullptr;
    }

    bool remove(std::string_view key) noexcept { return remove(root_, key); }

    template<class Visitor>
    void inorder(Visitor visit) const { traverse(root_.get(), visit, Order::In); }

    template<class Visitor>
    void preorder(Visitor visit) const { traverse(root_.get(), visit, Order::Pre); }

    template<class Visitor>
    void postorder(Visitor visit) const { traverse(root_.get(), visit, Order::Post); }

    void clear() noexcept { root_.reset(); }

private:
    enum class Order { In, Pre, Post };

    static int height(const Link& entry) noexcept { return entry ? entry->height_ : 0; }

    static void refresh(Entry& entry) noexcept {
        const int left = height(entry.left_);
        const int right = height(entry.right_);
        entry.height_ = 1 + (left > right ? left : right);
    }

    static int balance(const Link& entry) noexcept {
        return height(entry->left_) - height(entry->right_);
    }

    static void rotateLeft(Link& root) noexcept {
        auto pivot = std::move(root->right_);
        root->right_ = std::move(pivot->left_);
        refresh(*root);
        pivot->left_ = std::move(root);
        refresh(*pivot);
        root = std::move(pivot);
    }

    static void rotateRight(Link& root) noexcept {
        auto pivot = std::move(root->left_);
        root->left_ = std::move(pivot->right_);
        refresh(*root);
        pivot->right_ = std::move(root);
        refresh(*pivot);
        root = std::move(pivot);
    }

    static void rebalance(Link& root) noexcept {
        if (!root) {
            return;
        }
        refresh(*root);
        if (balance(root) > 1) {
            if (balance(root->left_) < 0) {
                rotateLeft(root->left_);
            }
            rotateRight(root);
        } else if (balance(root) < -1) {
            if (balance(root->right_) > 0) {
                rotateRight(root->right_);
            }
            rotateLeft(root);
        }
    }

    static bool insert(Link& root, std::string_view key, const T& value) {
        if (!root) {
            root = std::make_unique<Entry>(key, value);
            return true;
        }
        if (key == root->key_) {
            return false;
        }
        const bool inserted = insert(key < root->key_ ? root->left_ : root->right_, key, value);
        rebalance(root);
        return inserted;
    }

    static Link detachMin(Link& root) noexcept {
        if (!root->left_) {
            auto minimum = std::move(root);
            root = std::move(minimum->right_);
            return minimum;
        }
        auto minimum = detachMin(root->left_);
        rebalance(root);
        return minimum;
    }

    static bool remove(Link& root, std::string_view key) noexcept {
        if (!root) {
            return false;
        }
        if (key != root->key_) {
            const bool removed = remove(key < root->key_ ? root->left_ : root->right_, key);
            rebalance(root);
            return removed;
        }
        auto old = std::move(root);
        if (!old->right_) {
            root = std::move(old->left_);
        } else {
            root = detachMin(old->right_);
            root->left_ = std::move(old->left_);
            root->right_ = std::move(old->right_);
        }
        rebalance(root);
        return true;
    }

    template<class Visitor>
    static void traverse(const Entry* entry, Visitor& visit, Order order) {
        if (!entry) {
            return;
        }
        if (order == Order::Pre) {
            visit(entry->key_, entry->value_);
        }
        traverse(entry->left_.get(), visit, order);
        if (order == Order::In) {
            visit(entry->key_, entry->value_);
        }
        traverse(entry->right_.get(), visit, order);
        if (order == Order::Post) {
            visit(entry->key_, entry->value_);
        }
    }

    Link root_;
};

#endif
