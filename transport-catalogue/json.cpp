#include "json.h"

using namespace std;

namespace json {

namespace {

Node LoadNode(std::istream& input);

Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
std::string LoadString(std::istream& input) {

        using namespace std::literals;

        auto it = std::istreambuf_iterator<char>(input);
        auto end = std::istreambuf_iterator<char>();
        std::string s;
        while (true) {
            if (it == end) {
                // Поток закончился до того, как встретили закрывающую кавычку?
                throw ParsingError("String parsing error");
            }
            const char ch = *it;
            if (ch == '"') {
                // Встретили закрывающую кавычку
                ++it;
                break;
            } else if (ch == '\\') {
                // Встретили начало escape-последовательности
                ++it;
                if (it == end) {
                    // Поток завершился сразу после символа обратной косой черты
                    throw ParsingError("String parsing error");
                }
                const char escaped_char = *(it);
                // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                switch (escaped_char) {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        // Встретили неизвестную escape-последовательность
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                }
            } else if (ch == '\n' || ch == '\r') {
                // Строковый литерал внутри- JSON не может прерываться символами \r или \n
                throw ParsingError("Unexpected end of line"s);
            } else {
                // Просто считываем очередной символ и помещаем его в результирующую строку
                s.push_back(ch);
            }
            ++it;
        }

        return s;
}

Node LoadArray(istream& input) {
    Array result;
    char c = ' ';
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (c != ']') {
        throw ParsingError("Array parsing error"s);
    }

    return Node(move(result));
}

Node LoadDict(istream& input) {
    Dict result;

    char c = ' ';
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input);
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }
    if (c != '}') {
        throw ParsingError("Dict parsing error"s);
    }

    return Node(move(result));
}

//Удаление лишних символов в начале
void DeletePrefix(istream& input) {
    char c;
    input >> c;

    while(c == ' ' || c == '\\') {
        if (c == '\\') {
            input >> c;
        }
        input >> c;
    }
    input.putback(c);
}

bool CheckWord(const std::string& str, istream& input) {

    for(size_t i = 0; i < str.size(); ++i) {
        char c = EOF;
        input >> c;
        if (str[i] != c) {
            input.putback(c);
            return false;
        }
    }
    return true;
}

Node LoadNode(istream& input) {
    DeletePrefix(input);
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return Node(LoadString(input));
    } else if (c == 'n') {
        input.putback(c);

        if (CheckWord("null", input)) {
            return Node();
        }

        throw ParsingError("Null parsing error");
    } else if (c == 't' || c == 'f') {
        input.putback(c);

        if (CheckWord("true", input)) {
            return Node{true};
        }
        else if (CheckWord("false", input)) {
            return Node{false};
        }
        throw ParsingError("Bool parsing error");
    }
    else if (isdigit(c) || c == '-' || c =='+'){
        input.putback(c);

        const auto number = LoadNumber(input);
        if (holds_alternative<int>(number)) {
            return Node(get<int>(number));
        }
        else if (holds_alternative<double>(number)) {
            return Node(get<double>(number));
        }
        return Node();
    }
    throw ParsingError("Parsing error");
}

}  // namespace

const Node::Value& Node::GetValue() const {
    return value_;
}

bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}

//Возвращает true, если в Node хранится int либо double.
bool Node::IsDouble() const {
    return holds_alternative<double>(value_) || IsInt();
}

//Возвращает true, если в Node хранится double.
bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}

bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}

bool Node::IsString() const {
    return holds_alternative<std::string>(value_);
}

bool Node::IsNull() const {
    return holds_alternative<std::nullptr_t>(value_);
}

bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}

bool Node::IsMap() const {
    return holds_alternative<Dict>(value_);
}

int Node::AsInt() const {
    if (!IsInt()) {
        throw std::logic_error("");
    }
    return get<int>(value_);
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("");
    }
    return get<bool>(value_);
}

//Возвращает значение типа double, если внутри хранится double либо int.
double Node::AsDouble() const {
    if (!IsDouble()) {
        throw std::logic_error("");
    }
    if (IsInt()) {
        return get<int>(value_);
    }
    return get<double>(value_);
}

const std::string& Node::AsString() const {

    if (!IsString()) {
        throw std::logic_error("");
    }
    return get<std::string>(value_);
}

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("");
    }
    return get<Array>(value_);
}

const Dict& Node::AsMap() const {
    if (!IsMap()) {
        throw std::logic_error("");
    }
    return get<Dict>(value_);
}

bool operator==(const Node& lhs, const Node& rhs) {
    if(lhs.IsArray() && rhs.IsArray()){
        return lhs.AsArray() == rhs.AsArray();
    }
    if(lhs.IsBool() && rhs.IsBool()){
        return lhs.AsBool() == rhs.AsBool();
    }
    if(lhs.IsPureDouble() && rhs.IsPureDouble()){
        return lhs.AsDouble() == rhs.AsDouble();
    }
    if(lhs.IsInt() && rhs.IsInt()){
        return lhs.AsInt() == rhs.AsInt();
    }
    if(lhs.IsMap() && rhs.IsMap()){
        return lhs.AsMap() == rhs.AsMap();
    }
    if(lhs.IsNull() && rhs.IsNull()){
        return true;
    }
    if(lhs.IsString() && rhs.IsString()){
        return lhs.AsString() == rhs.AsString();
    }
    return false;
}
bool operator!=(const Node& lhs, const Node& rhs) {
    return !(lhs == rhs);
}


std::string PrintString(std::string& str) {
    auto it = str.begin();
    auto end = str.end();
    std::string str_result;
    while (true) {
        if (it == end) {
            return str_result;
        }
        const char ch = *it;
        switch (ch) {
            case '\n':
                str_result += "\\n";
                break;
            case '\r':
                str_result += "\\r";
                break;
            case '"':
                str_result += "\\\"";
                break;
            case '\\':
                str_result += "\\\\";
                break;
            default:
                str_result.push_back(ch);
        }
        ++it;
    }

    return str_result;
}
void PrintValue(const int num, std::ostream& out) {
    out << num;
}

void PrintValue(const double num, std::ostream& out) {
    out << num;
}

// Перегрузка функции PrintValue для вывода значений null
void PrintValue(std::nullptr_t, std::ostream& out) {
    out << "null"sv;
}

// Перегрузка функции PrintValue для вывода значений Array
void PrintValue(const Array& nodes, std::ostream& out) {
    out << "[";
    bool isFirst = true;
    for(auto& node : nodes) {
        if (isFirst) {
            isFirst = false;
        }
        else {
            out << ", ";
        }
        PrintNode(node, out);
    }
    out << "]";
}

// Перегрузка функции PrintValue для вывода значений Dict
void PrintValue(const Dict& dict, std::ostream& out) {
    out << "{\n";
    bool isFirst = true;
    for (auto& [key, node] : dict) {
        if (isFirst) {
            isFirst = false;
        }
        else {
            out << "\n, ";
        }
        out << "\"" << key << "\": ";
        PrintNode(node, out);
    }
    out << "}\n";
}

// Перегрузка функции PrintValue для вывода значений bool
void PrintValue(bool bl, std::ostream& out) {
    if (bl) {
        out << "true";
    }
    else {
        out << "false";
    }
}

// Перегрузка функции PrintValue для вывода значений string
void PrintValue(std::string str, std::ostream& out) {
    out << "\"" << PrintString(str) << "\"";
}

void PrintNode(const Node& node, std::ostream& out) {
    std::visit(
        [&out](const auto& value){ PrintValue(value, out); },
        node.GetValue());
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool operator==(const Document& lhs, const Document& rhs) {
    return lhs.GetRoot() == rhs.GetRoot();
}
bool operator!=(const Document& lhs, const Document& rhs) {
    return !(lhs == rhs);
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

}  // namespace json
