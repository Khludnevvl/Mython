#pragma once

#include <deque>
#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace parse
{
namespace token_type
{
struct Number
{
    int value = 0;
};

struct Id
{
    std::string value = 0;
};

struct Char
{
    char value = ' ';
};

struct String
{
    std::string value;
};

struct Class
{
}; // Лексема «class»
struct Return
{
}; // Лексема «return»
struct If
{
}; // Лексема «if»
struct Else
{
}; // Лексема «else»
struct Def
{
}; // Лексема «def»
struct Newline
{
}; // Лексема «конец строки»
struct Print
{
}; // Лексема «print»
struct Indent
{
}; // Лексема «увеличение отступа», соответствует двум пробелам
struct Dedent
{
}; // Лексема «уменьшение отступа»
struct Eof
{
}; // Лексема «конец файла»
struct And
{
}; // Лексема «and»
struct Or
{
}; // Лексема «or»
struct Not
{
}; // Лексема «not»
struct Eq
{
}; // Лексема «==»
struct NotEq
{
}; // Лексема «!=»
struct LessOrEq
{
}; // Лексема «<=»
struct GreaterOrEq
{
}; // Лексема «>=»
struct None
{
}; // Лексема «None»
struct True
{
}; // Лексема «True»
struct False
{
}; // Лексема «False»
} // namespace token_type

using TokenBase =
    std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String, token_type::Class,
                 token_type::Return, token_type::If, token_type::Else, token_type::Def, token_type::Newline,
                 token_type::Print, token_type::Indent, token_type::Dedent, token_type::And, token_type::Or,
                 token_type::Not, token_type::Eq, token_type::NotEq, token_type::LessOrEq,
                 token_type::GreaterOrEq, token_type::None, token_type::True, token_type::False,
                 token_type::Eof>;

struct Token : TokenBase
{
    using TokenBase::TokenBase;

    template <typename T>
    [[nodiscard]] bool Is() const
    {
        return std::holds_alternative<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T &As() const
    {
        return std::get<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T *TryAs() const
    {
        return std::get_if<T>(this);
    }
};

bool operator==(const Token &lhs, const Token &rhs);
bool operator!=(const Token &lhs, const Token &rhs);

std::ostream &operator<<(std::ostream &os, const Token &rhs);

class LexerError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class ParsingError : public std::runtime_error
{
public:
    using runtime_error::runtime_error;
};

class Lexer
{
public:
    explicit Lexer(std::istream &input);

    // Возвращает ссылку на текущий токен или token_type::Eof, если поток
    // токенов закончился
    [[nodiscard]] const Token &CurrentToken() const;

    // Возвращает следующий токен, либо token_type::Eof, если поток токенов
    // закончился
    Token NextToken();

    // Если текущий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T &Expect() const;

    // Метод проверяет, что текущий токен имеет тип T, а сам токен содержит
    // значение value. В противном случае метод выбрасывает исключение
    // LexerError
    template <typename T, typename U>
    void Expect(const U &value) const;

    // Если следующий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T &ExpectNext();

    // Метод проверяет, что следующий токен имеет тип T, а сам токен содержит
    // значение value. В противном случае метод выбрасывает исключение
    // LexerError
    template <typename T, typename U>
    void ExpectNext(const U &value);

private:
    void ProcessAllTokens(std::istream &input);
    void ProcessNextToken(std::istream &input);
    void ProcessIndent(std::istream &input);
    void ProcessNextLine();
    void ProcessNum(std::istream &input);
    void ProcessSymbol(char c);
    void ProcessComparisonOperator(char left, char right);
    void ProcessWord(std::istream &input);
    void ProcessKeyWord(const std::string &word);
    void ProcessString(std::istream &input);
    void IgnoreComment(std::istream &input);

    bool IsComparisonOperator(char left, char right) const;
    bool IsKeyWord(const std::string &word) const;

private:
    std::deque<Token> tokens_;
    size_t prev_indent = 0;
    size_t current_pos = 0;
};
template <typename T>
const T &Lexer::Expect() const
{
    using namespace std::literals;
    auto token = CurrentToken().TryAs<T>();

    if (token != nullptr)
    {
        return *token;
    }
    throw LexerError("Incorect type in lexer::Expect()"s);
}

template <typename T, typename U>
void Lexer::Expect(const U &value) const
{
    using namespace std::literals;
    auto token = CurrentToken().TryAs<T>();
    if (token != nullptr && token->value == value)
    {
        return;
    }
    throw LexerError("Incorect type or value in lexer::Expect(value)"s);
}

template <typename T>
const T &Lexer::ExpectNext()
{
    using namespace std::literals;
    NextToken();
    auto token = CurrentToken().TryAs<T>();
    if (token != nullptr)
    {
        const T &result = *token;
        return result;
    }
    throw LexerError("Incorect type in lexer::ExpectNext()"s);
}

template <typename T, typename U>
void Lexer::ExpectNext(const U &value)
{
    using namespace std::literals;
    auto token = NextToken();
    auto token_ptr = token.TryAs<T>();
    if (token_ptr != nullptr && token_ptr->value == value)
    {
        return;
    }
    throw LexerError("Incorect type or value lexer::ExpectNext()"s);
}

} // namespace parse