#pragma once

#include "json.h"

namespace json
{

    class KeyContext;
    class DictItemContext;
    class ArrayItemContext;


    class Builder
    {
    public:

        Builder();

        virtual DictItemContext StartDict();
        virtual ArrayItemContext StartArray();

        virtual Builder& EndDict();
        virtual Builder& EndArray();

        virtual KeyContext Key(std::string key);
        Builder& Value(Node value);

        virtual Node Build();

    private:

        Node root_;
        std::vector<Node*> nodes_stack_;
    };


    class KeyContext final : Builder
    {
    public:

        KeyContext(Builder&& builder);

        DictItemContext Value(Node value);

        ArrayItemContext StartArray() override;

        DictItemContext StartDict() override;

    private:

        Builder& builder_;
    };


    class DictItemContext final : Builder
    {
    public:

        DictItemContext(Builder&& builder);

        KeyContext Key(std::string key) override;

        Builder& EndDict() override;

    private:

        Builder& builder_;
    };

    class ArrayItemContext final : Builder
    {
    public:

        ArrayItemContext(Builder&& builder);

        ArrayItemContext& Value(Node value);

        DictItemContext StartDict() override;

        ArrayItemContext StartArray() override;

        Builder& EndArray() override;

    private:

        Builder& builder_;
    };

}
