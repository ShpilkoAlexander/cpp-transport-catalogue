#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <regex>
#include <algorithm>

namespace json {

class Node;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;

using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

void DeletePrefix(std::istream& input);

//Проверка на равенство слова из потока
bool CheckWord(const std::string& std, std::istream& input) ;

using Number = std::variant<int, double>;

Number LoadNumber(std::istream& input);

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует иexспользовать после считывания открывающего символа ":
std::string LoadString(std::istream& input);

class Node {
public:

    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    Node() = default;

    template <typename Value>
    Node(Value value) {
        value_ = value;
    }


    const Value& GetValue() const;

    bool IsInt() const;
    bool IsDouble() const; //Возвращает true, если в Node хранится int либо double.
    bool IsPureDouble() const; //Возвращает true, если в Node хранится double.
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const; //Возвращает значение типа double, если внутри хранится double либо int.
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

private:
    Value value_;
};

bool operator==(const Node& lhs, const Node& rhs);
bool operator!=(const Node& lhs, const Node& rhs);

// Шаблон, подходящий для вывода double и int
template <typename Value>
void PrintValue(const Value& value, std::ostream& out) {
    out << &value;
}

// Вывод строки с экранированием
std::string PrintString(std::string& str);

void PrintValue(const int num, std::ostream& out);

void PrintValue(const double num, std::ostream& out);

// Перегрузка функции PrintValue для вывода значений null
void PrintValue(std::nullptr_t, std::ostream& out);

// Перегрузка функции PrintValue для вывода значений Array
void PrintValue(const Array& nodes, std::ostream& out);
// Перегрузка функции PrintValue для вывода значений Dict
void PrintValue(const Dict& dict, std::ostream& out);

// Перегрузка функции PrintValue для вывода значений bool
void PrintValue(bool bl, std::ostream& out);

// Перегрузка функции PrintValue для вывода значений string
void PrintValue(std::string str, std::ostream& out);

void PrintNode(const Node& node, std::ostream& out);

class Document {
public:
    Document(Node root);

    const Node& GetRoot() const;

private:
    Node root_;
};

bool operator==(const Document& lhs, const Document& rhs);
bool operator!=(const Document& lhs, const Document& rhs);

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json
