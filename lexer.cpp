#include "lexer.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <map>
#include <unordered_map>

using namespace std;

namespace parse
{

bool operator==(const Token &lhs, const Token &rhs)
{
    using namespace token_type;

    if (lhs.index() != rhs.index())
    {
        return false;
    }

#define VALUED_COMPARE(type)                                                                                 \
    {                                                                                                        \
        auto lhs_ptr = lhs.TryAs<type>();                                                                    \
        auto rhs_ptr = rhs.TryAs<type>();                                                                    \
        if (lhs_ptr && rhs_ptr)                                                                              \
            return lhs_ptr->value == rhs_ptr->value;                                                         \
    }

    VALUED_COMPARE(Number);
    VALUED_COMPARE(Id);
    VALUED_COMPARE(String);
    VALUED_COMPARE(Char);

#undef VALUED_COMPARE

    return true;
}

bool operator!=(const Token &lhs, const Token &rhs)
{
    return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &os, const Token &rhs)
{
    using namespace token_type;

#define VALUED_OUTPUT(type)                                                                                  \
    if (auto p = rhs.TryAs<type>())                                                                          \
        return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type)                                                                                \
    if (rhs.Is<type>())                                                                                      \
        return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

void Lexer::ProcessKeyWord(const std::string &word)
{
    const std::map<std::string, Token> KEYWORD_TO_TOKEN = {
        {"class", Token(token_type::Class{})}, {"return", Token(token_type::Return{})},
        {"if", Token(token_type::If{})},       {"else", Token(token_type::Else{})},
        {"def", Token(token_type::Def{})},     {"print", Token(token_type::Print{})},
        {"or", Token(token_type::Or{})},       {"None", Token(token_type::None{})},
        {"not", Token(token_type::Not{})},     {"and", Token(token_type::And{})},
        {"True", Token(token_type::True{})},   {"False", Token(token_type::False{})}};

    auto iter = KEYWORD_TO_TOKEN.find(word);
    if (iter != KEYWORD_TO_TOKEN.end())
    {
        tokens_.push_back(iter->second);
    }
    else
    {
        std::runtime_error("Failed to process key word");
    }
}

void Lexer::ProcessWord(std::istream &input)
{
    std::string word;
    while (std::isalpha(input.peek()) || std::isdigit(input.peek()) || input.peek() == '_')
    {
        word.push_back(static_cast<char>(input.get()));
    }
    if (IsKeyWord(word))
    {
        ProcessKeyWord(std::move(word));
    }
    else
    {
        tokens_.push_back(Token(token_type::Id{std::move(word)}));
    }
}

void Lexer::ProcessIndent(std::istream &input)
{
    size_t space_count = 0;
    while (input && static_cast<char>(input.peek()) == ' ')
    {
        input.get();
        space_count++;
    }
    if (input.peek() == '\n')
    {
        return;
    }
    const size_t indent = space_count / 2;
    if (indent > prev_indent)
    {
        for (size_t i = 0; i < indent - prev_indent; i++)
        {
            tokens_.push_back(Token(token_type::Indent{}));
        }
        prev_indent = indent;
    }
    else if (indent < prev_indent)
    {
        for (size_t i = 0; i < prev_indent - indent; i++)
        {
            tokens_.push_back(Token(token_type::Dedent{}));
        }
        prev_indent = indent;
    }
}

bool Lexer::IsComparisonOperator(char left, char right) const
{
    const std::string oper{left, right};

    return (oper == ">=" || oper == "==" || oper == "<=" || oper == "!=");
}

void Lexer::ProcessComparisonOperator(char left, char right)
{
    const std::string oper{left, right};

    if (oper == ">=")
    {
        tokens_.push_back(Token(token_type::GreaterOrEq{}));
    }
    else if (oper == "<=")
    {
        tokens_.push_back(Token(token_type::LessOrEq{}));
    }
    else if (oper == "==")
    {
        tokens_.push_back(Token(token_type::Eq{}));
    }
    else if (oper == "!=")
    {
        tokens_.push_back(Token(token_type::NotEq{}));
    }
}

void Lexer::ProcessSymbol(char c)
{
    tokens_.push_back(Token(token_type::Char{c}));
}

void Lexer::ProcessNextLine()
{
    if (!tokens_.empty() && !tokens_.back().Is<token_type::Newline>())
    {
        tokens_.push_back(Token(token_type::Newline{}));
    }
}

void Lexer::ProcessNum(std::istream &input)
{
    std::string num_str;
    while (std::isdigit(input.peek()))
    {
        num_str.push_back(static_cast<char>(input.get()));
    }

    int result = std::stoi(num_str);
    tokens_.push_back(Token(token_type::Number{result}));
}

void Lexer::IgnoreComment(std::istream &input)
{
    do
    {
        input.ignore(1024, '\n');
        if (!input.eof())
        {
            input.unget();
        }
    } while (input.peek() != '\n' && !input.eof());
}

void Lexer::ProcessString(std::istream &input)
{
    char delim = static_cast<char>(input.get());
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true)
    {
        if (it == end)
        {
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == delim)
        {
            ++it;
            break;
        }
        else if (ch == '\\')
        {
            ++it;
            if (it == end)
            {
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            switch (escaped_char)
            {
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
            case '\'':
                s.push_back('\'');
                break;
            case '\\':
                s.push_back('\\');
                break;
            default:
                throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        }
        else if (ch == '\n' || ch == '\r')
        {
            throw ParsingError("Unexpected end of line"s);
        }
        else
        {
            s.push_back(ch);
        }
        ++it;
    }
    tokens_.push_back(Token(token_type::String{std::move(s)}));
}

Lexer::Lexer(std::istream &input)
{
    ProcessAllTokens(input);
}

void Lexer::ProcessAllTokens(std::istream &input)
{
    ProcessIndent(input);
    while (input.good())
    {
        ProcessNextToken(input);
    }
    if (!tokens_.empty() && !tokens_.back().Is<token_type::Dedent>() &&
        !tokens_.back().Is<token_type::Newline>())
    {
        ProcessNextLine();
    }
    tokens_.push_back(Token(token_type::Eof{}));
}

void Lexer::ProcessNextToken(std::istream &input)
{
    char c;
    c = static_cast<char>(input.get());
    if (c == '\n')
    {
        ProcessNextLine();
        ProcessIndent(input);
    }
    else if (c == '"' || c == '\'')
    {
        input.putback(c);
        ProcessString(input);
    }
    else if (c == '_' || std::isalpha(c))
    {
        input.putback(c);
        ProcessWord(input);
    }
    else if (std::isdigit(c))
    {
        input.putback(c);
        ProcessNum(input);
    }
    else if (c == '#')
    {
        IgnoreComment(input);
    }
    else if (std::ispunct(c))
    {
        char second_symbol = static_cast<char>(input.get());

        if (IsComparisonOperator(c, second_symbol))
        {
            ProcessComparisonOperator(c, second_symbol);
        }
        else
        {
            input.putback(second_symbol);
            ProcessSymbol(c);
        }
    }
}

bool Lexer::IsKeyWord(const std::string &word) const
{
    const static std::set<std::string> USING_KEYWORDS = {"class", "return", "if",  "else", "def",  "print",
                                                         "or",    "None",   "and", "not",  "True", "False"};

    return USING_KEYWORDS.count(word);
}

const Token &Lexer::CurrentToken() const
{
    if (current_pos < tokens_.size())
    {
        return tokens_[current_pos];
    }
    else
    {
        const static Token EOF_TOKEN = Token(token_type::Eof{});
        return EOF_TOKEN;
    }
}

Token Lexer::NextToken()
{
    if (current_pos < tokens_.size())
    {
        current_pos++;
        return CurrentToken();
    }
    else
    {
        return Token(token_type::Eof{});
    }
}

} // namespace parse