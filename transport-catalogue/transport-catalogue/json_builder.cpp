#include "json_builder.h"

namespace json {

Builder::KeyContext Builder::Key(std::string key) {
    if (nodes_stack_.empty() || !Top()->IsDict()) {
        throw std::logic_error("Key can only be set in a dictionary");
    }
    auto& dict = const_cast<Dict&>(Top()->AsDict());
    dict[key] = Node(key);  // Временное значение
    Push(&dict[key]);
    return Builder::KeyContext(*this);
}

Builder& Builder::EndDict() {
    if (nodes_stack_.empty() || !Top()->IsDict()) {
        throw std::logic_error("EndDict can only be called on a dictionary");
    }
    Pop();
    return *this;
}

Builder& Builder::EndArray() {
    if (nodes_stack_.empty() || !Top()->IsArray()) {
        throw std::logic_error("EndArray can only be called on an array");
    }
    Pop();
    return *this;
}

Builder::DictItemContext Builder::StartDict() {
    return StartContainer<Dict>();
}

Builder::ArrayItemContext Builder::StartArray() {
    return StartContainer<Array>();
}

template <typename Container>
auto Builder::StartContainer() -> typename std::conditional<
    std::is_same<Container, Dict>::value,
    Builder::DictItemContext,
    Builder::ArrayItemContext
>::type
{
    if (nodes_stack_.empty() && root_.IsNull()) {
        root_ = Container{};
        Push(&root_);
        return typename std::conditional<
            std::is_same<Container, Dict>::value,
            Builder::DictItemContext,
            Builder::ArrayItemContext
        >::type(*this);
    }

    if (!nodes_stack_.empty() && Top()->IsArray()) {
        auto& array = const_cast<Array&>(Top()->AsArray());
        array.emplace_back(Container{});
        Push(&array.back());
    }
    else if (!nodes_stack_.empty() && Top()->IsString()) {
        auto key = Top()->AsString();
        Pop();
        
        if (nodes_stack_.empty() || !Top()->IsDict()) {
            throw std::logic_error("Key must be followed by a value in a dictionary");
        }
        
        auto& dict = const_cast<Dict&>(Top()->AsDict());
        dict[key] = Container{};
        Push(&dict[key]);
    }
    else {
        throw std::logic_error("StartContainer can only be called in an array, after a key, or at the root level");
    }

    return typename std::conditional<
        std::is_same<Container, Dict>::value,
        Builder::DictItemContext,
        Builder::ArrayItemContext
    >::type(*this);
}

template Builder::DictItemContext Builder::StartContainer<Dict>();
template Builder::ArrayItemContext Builder::StartContainer<Array>();

Builder& Builder::Value(Node::Value value) {
    if (nodes_stack_.empty()) {
        if (!root_.IsNull()) {
            throw std::logic_error("Value can only be set once at the root level");
        }
        root_ = Node(std::move(value));
        return *this;
    }

    if (Top()->IsArray()) {
        auto& array = const_cast<Array&>(Top()->AsArray());
        array.emplace_back(Node(std::move(value)));
    } 
    else if (Top()->IsString()) {
        auto key = Top()->AsString();
        Pop();
        if (nodes_stack_.empty() || !Top()->IsDict()) {
            throw std::logic_error("Key must be followed by a value in a dictionary");
        }
        auto& dict = const_cast<Dict&>(Top()->AsDict());
        dict[key] = Node(std::move(value));
    }
    else {
        throw std::logic_error("Value can only be set in an array, after a key, or at the root level");
    }
    return *this;
}

Node Builder::Build() {
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Cannot build: some containers are not closed");
    }
    if (root_.IsNull()) {
        throw std::logic_error("Cannot build: root is null");
    }
    return std::move(root_);
}

Node* Builder::Top() {
    return nodes_stack_.top();
}

void Builder::Pop() {
    nodes_stack_.pop();
}

void Builder::Push(Node* node) {
    nodes_stack_.push(node);
}

// Реализации методов контекстных классов
Builder::KeyContext Builder::DictItemContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

Builder& Builder::DictItemContext::EndDict() {
    return builder_.EndDict();
}

Builder::DictItemContext Builder::KeyContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return Builder::DictItemContext(builder_);
}

Builder::DictItemContext Builder::KeyContext::StartDict() {
    return builder_.StartDict();
}

Builder::ArrayItemContext Builder::KeyContext::StartArray() {
    return builder_.StartArray();
}

Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return Builder::ArrayItemContext(builder_);
}

Builder::DictItemContext Builder::ArrayItemContext::StartDict() {
    return builder_.StartDict();
}

Builder::ArrayItemContext Builder::ArrayItemContext::StartArray() {
    return builder_.StartArray();
}

Builder& Builder::ArrayItemContext::EndArray() {
    return builder_.EndArray();
}

} // namespace json
