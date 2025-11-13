#pragma once

#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace Json {

class Node : std::variant<std::vector<Node>,
                        std::map<std::string, Node>,
                        int,
                        double,
                        bool,
                        std::string>
{
public:
    using variant::variant;

    enum class Type {
        ARRAY,
        MAP,
        INT,
        STRING,
        BOOL,
        COUNT,
        DOUBLE
    };

    Type getType() const {
        if(std::holds_alternative<std::vector<Node>>(*this)) {
            return Type::ARRAY;
        }
        else if(std::holds_alternative<std::map<std::string, Node>>(*this)) {
            return Type::MAP;
        }
        else if(std::holds_alternative<int>(*this)) {
            return Type::INT;
        }
        else if(std::holds_alternative<std::string>(*this)) {
            return Type::STRING;
        }
        else if(std::holds_alternative<bool>(*this)) {
            return Type::BOOL;
        }
        else if(std::holds_alternative<double>(*this)) {
            return Type::DOUBLE;
        }
        return Type::COUNT;
    }

    const auto& AsArray() const
    {
        return std::get<std::vector<Node>>(*this);
    }
    auto& AsArray()
    {
        return std::get<std::vector<Node>>(*this);
    }
    const auto& AsMap() const
    {
        return std::get<std::map<std::string, Node>>(*this);
    }
    auto& AsMap()
    {
        return std::get<std::map<std::string, Node>>(*this);
    }
    int AsInt() const
    {
        return std::get<int>(*this);
    }
    const auto& AsString() const
    {
        return std::get<std::string>(*this);
    }
    const auto& AsDouble() const
    {
        return std::get<double>(*this);
    }
    bool AsBool() const
    {
      return std::get<bool>(*this);
    }
};

class Document
{
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

private:
    Node root;
};

Document Load(std::istream& input);

}