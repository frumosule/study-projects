#pragma once

#include "json.h"
#include <stack>
#include <string>
#include <stdexcept>

namespace json {

class Builder {

private:
    class KeyContext;
    class DictItemContext; 
    class ArrayItemContext;

public:
    Builder() = default;

    // Основные методы Builder
    KeyContext Key(std::string key);
    Builder& EndDict();
    Builder& EndArray();
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& Value(Node::Value value);
    Node Build();    

private:
    Node root_;
    std::stack<Node*> nodes_stack_; 

    // Базовый класс контекста со всеми методами Builder
    class BaseContext {
    public:
        explicit BaseContext(Builder& builder) : builder_(builder) {}

        // Все методы Builder
        KeyContext Key(std::string key);
        Builder& EndDict();
        Builder& EndArray();
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& Value(Node::Value value);
        Node Build();

    protected:
        Builder& builder_;
    };

    // Контекст для работы с элементами словаря
    class DictItemContext : public BaseContext {
    public:
        using BaseContext::BaseContext;

        KeyContext Key(std::string key);
        Builder& EndDict();

        // Запрещаем недопустимые методы
        Builder& Value(Node::Value) = delete;
        Builder& EndArray() = delete;
        DictItemContext StartDict() = delete;
        ArrayItemContext StartArray() = delete;
    };

    // Контекст для работы с ключами
    class KeyContext : public BaseContext {
    public:
        using BaseContext::BaseContext;

        DictItemContext Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();

        // Запрещаем недопустимые методы
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
        KeyContext Key(std::string) = delete;
    };

    // Контекст для работы с элементами массива
    class ArrayItemContext : public BaseContext {
    public:
        using BaseContext::BaseContext;

        ArrayItemContext Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndArray();

        // Запрещаем недопустимые методы
        Builder& EndDict() = delete;
        KeyContext Key(std::string) = delete;
    };

    // Вспомогательные методы
    Node* Top();
    void Pop();
    void Push(Node* node);

    // Шаблонный метод для создания контейнеров
    template <typename Container>
    auto StartContainer() -> typename std::conditional<
        std::is_same<Container, Dict>::value,
        DictItemContext,
        ArrayItemContext
    >::type;
};

} // namespace json
