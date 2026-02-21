#include <cassert>
#include <cstddef>
#include <string>
#include <utility>

template <typename Type>
class SingleLinkedList {
    
    // Узел списка
    struct Node {
        Node() = default;
        Node(const Type& val, Node* next)
            : value(val)
            , next_node(next) {
        }
        Type value;
        Node* next_node = nullptr;
    };
    
    //итератор
    template <typename ValueType>
    class BasicIterator {
        friend class SingleLinkedList;

        explicit BasicIterator(Node* node) {
            node_ = node;
        }

        public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Type;
        using difference_type = std::ptrdiff_t;
        using pointer = ValueType*;
        using reference = ValueType&;

        BasicIterator() = default;

        BasicIterator(const BasicIterator<Type>& other) noexcept {
            node_ = other.node_;
        }

        BasicIterator& operator=(const BasicIterator& rhs) = default;

        [[nodiscard]] bool operator==(const BasicIterator<const Type>& rhs) const noexcept {
            return node_ == rhs.node_;
        }

        [[nodiscard]] bool operator!=(const BasicIterator<const Type>& rhs) const noexcept {
            return !(rhs == *this);
        }

        [[nodiscard]] bool operator==(const BasicIterator<Type>& rhs) const noexcept {
            if (rhs.node_ == node_) {
                return true;
            }
            return false;
        }

        [[nodiscard]] bool operator!=(const BasicIterator<Type>& rhs) const noexcept {
            return !(rhs == *this);
        }

        BasicIterator& operator++() noexcept {
            assert(node_ != nullptr);
            node_ = node_->next_node;
            return *this;
        }

        BasicIterator operator++(int) noexcept {
            BasicIterator prev = *this;
            ++(*this);
            return prev;
        }

        [[nodiscard]] reference operator*() const noexcept {
            assert(node_ != nullptr);
            return node_->value;
        }

        [[nodiscard]] pointer operator->() const noexcept {
            assert(node_ != nullptr);
            return &(node_->value);
        }

        private:
        Node* node_ = nullptr;
    };

public:

    using value_type = Type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using Iterator = BasicIterator<Type>;
    using ConstIterator = BasicIterator<const Type>;

      SingleLinkedList(std::initializer_list<Type> values) {
        auto it = Iterator(&head_);

        for (const auto& value : values) {
            it = InsertAfter(it, value);
        }
    }

    SingleLinkedList(const SingleLinkedList& other) {
        SingleLinkedList tmp;
    
        auto it = Iterator(&tmp.head_);

        for (const auto& value : other) {
            it = tmp.InsertAfter(it, value);
        }
        swap(tmp);
    }

    SingleLinkedList& operator=(const SingleLinkedList& rhs) {
        if (this != &rhs) {
            SingleLinkedList tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

   
    void swap(SingleLinkedList& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(head_.next_node, other.head_.next_node);
    }

    [[nodiscard]] Iterator begin() noexcept {
        auto it = Iterator(head_.next_node);
        return it;
    }

    [[nodiscard]] Iterator end() noexcept {
        auto it = Iterator(nullptr);
        return it;
    }

     [[nodiscard]] Iterator before_begin() noexcept {
        auto it = Iterator(&head_);
        return it;
    }

     [[nodiscard]] ConstIterator cbefore_begin() const noexcept {
        return ConstIterator{const_cast<Node*>(&head_)};
    }

      [[nodiscard]] ConstIterator before_begin() const noexcept {
        auto it = ConstIterator{const_cast<Node*>(&head_)};
        return it;
    }

    [[nodiscard]] ConstIterator begin() const noexcept {
        auto it = ConstIterator(head_.next_node);
        return it;
    }

    [[nodiscard]] ConstIterator end() const noexcept {
        auto it = ConstIterator(nullptr);
        return it;
    }

     [[nodiscard]] ConstIterator cbegin() const noexcept {
        return begin();
    }

    [[nodiscard]] ConstIterator cend() const noexcept {
        return end();
    }

    SingleLinkedList() : size_(0) {
        head_.next_node = nullptr;
    }

    ~SingleLinkedList() {
        Clear();
    }

    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }

    [[nodiscard]] bool IsEmpty() const noexcept {
        if (head_.next_node == nullptr){
            return true;
        }
        return false;
    }

    void PushFront(const Type& value) {
        head_.next_node = new Node(value, head_.next_node);
        size_++;
    }

    void PushBack(const Type& value) {
        Node* new_node = new Node(value, nullptr);
        if (head_.next_node == nullptr) {
            head_.next_node = new_node; 
        } else {
            Node* current = head_.next_node;
            while (current->next_node != nullptr) {
                current = current->next_node; 
            }
            current->next_node = new_node; 
        }
        size_++;
    }

    void PopFront() noexcept {
        auto temp = head_.next_node;
        head_.next_node = temp->next_node;
        delete temp;
        size_--;
    }

     Iterator InsertAfter(ConstIterator pos, const Type& value) {
        assert(pos.node_ != nullptr);
        Node* n = new Node(value, pos.node_->next_node);
        pos.node_->next_node = n;
        size_++;
        return Iterator(n);
    }

     Iterator EraseAfter(ConstIterator pos) noexcept {
        assert(pos.node_ != nullptr);
        auto temp = pos.node_->next_node;
        pos.node_->next_node = temp->next_node;
        delete temp;
        size_--;
        return Iterator(pos.node_->next_node);
    }

    void Clear() noexcept {
        while (head_.next_node != nullptr) {
            Node* prev = head_.next_node;
            head_.next_node = head_.next_node->next_node;
            delete prev;
        }
        size_ = 0;
    }

private:
    Node head_;
    size_t size_ = 0;
};

template <typename Type>
void swap(SingleLinkedList<Type>& lhs, SingleLinkedList<Type>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename Type>
bool operator==(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
   return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template <typename Type>
bool operator!=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
bool operator<(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    if (lhs.GetSize() != rhs.GetSize()) {
        return lhs.GetSize() < rhs.GetSize();
    }
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
bool operator>=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs < rhs);
}

template <typename Type>
bool operator>(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
bool operator<=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs > rhs);
} 

void Test0() {
    using namespace std;
    {
        const SingleLinkedList<int> empty_int_list;
        assert(empty_int_list.GetSize() == 0u);
        assert(empty_int_list.IsEmpty());
    }

    {
        const SingleLinkedList<string> empty_string_list;
        assert(empty_string_list.GetSize() == 0u);
        assert(empty_string_list.IsEmpty());
    }
}

int main() {
    Test0();
}
