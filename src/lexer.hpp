#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <cstdint>

// all the token types i need - one per keyword, operator, or literal kind
enum class TokenType {
    INT_LIT, TRUE_LIT, FALSE_LIT,
    IDENT,
    LET, IF, ELSE, WHILE, PRINT, INPUT,
    PLUS, MINUS, STAR, SLASH,
    EQ_EQ, LESS, ASSIGN,
    LPAREN, RPAREN, LBRACE, RBRACE,
    SEMICOLON,
    EOF_TOK
};

// each token stores what type it is, the raw text, integer value (for INT_LIT),
// and what line it's on (useful for error messages)
struct Token {
    TokenType type;
    std::string text;
    int64_t intVal;
    int line;

    Token(TokenType t, std::string tx, int64_t v, int ln)
        : type(t), text(std::move(tx)), intVal(v), line(ln) {}
};

// the lexer takes the whole source as a string and produces a flat list of tokens
class Lexer {
public:
    explicit Lexer(std::string src) : src_(std::move(src)), pos_(0), line_(1) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        // keep going until we consume the whole input
        while (true) {
            skipWhitespaceAndComments();
            if (pos_ >= src_.size()) {
                tokens.emplace_back(TokenType::EOF_TOK, "", 0, line_);
                break;
            }
            char c = src_[pos_];
            if (std::isdigit(static_cast<unsigned char>(c))) {
                tokens.push_back(readInt());
            } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                tokens.push_back(readIdent());
            } else {
                tokens.push_back(readSymbol());
            }
        }
        return tokens;
    }

private:
    std::string src_;
    size_t pos_;
    int line_;

    // look ahead without consuming - offset=0 means current char
    char peek(size_t offset = 0) const {
        size_t idx = pos_ + offset;
        return (idx < src_.size()) ? src_[idx] : '\0';
    }

    char advance() {
        char c = src_[pos_++];
        if (c == '\n') ++line_; // had to add this check after getting wrong line numbers
        return c;
    }

    // skip blank lines, spaces, tabs, and // line comments
    void skipWhitespaceAndComments() {
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (std::isspace(static_cast<unsigned char>(c))) {
                advance();
            } else if (c == '/' && peek(1) == '/') {
                // skip everything until end of line
                while (pos_ < src_.size() && src_[pos_] != '\n')
                    ++pos_;
            } else {
                break;
            }
        }
    }

    // just grab digits and parse the number
    Token readInt() {
        int startLine = line_;
        size_t start = pos_;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_])))
            ++pos_;
        std::string text = src_.substr(start, pos_ - start);
        int64_t val = std::stoll(text);
        return Token(TokenType::INT_LIT, text, val, startLine);
    }

    // read word, then check if it's a keyword or just an identifier
    Token readIdent() {
        int startLine = line_;
        size_t start = pos_;
        while (pos_ < src_.size() &&
               (std::isalnum(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_'))
            ++pos_;
        std::string text = src_.substr(start, pos_ - start);

        // keywords - not sure if there's a cleaner way but this is readable
        if (text == "let")   return Token(TokenType::LET,       text, 0, startLine);
        if (text == "if")    return Token(TokenType::IF,        text, 0, startLine);
        if (text == "else")  return Token(TokenType::ELSE,      text, 0, startLine);
        if (text == "while") return Token(TokenType::WHILE,     text, 0, startLine);
        if (text == "print") return Token(TokenType::PRINT,     text, 0, startLine);
        if (text == "input") return Token(TokenType::INPUT,     text, 0, startLine);
        if (text == "true")  return Token(TokenType::TRUE_LIT,  text, 0, startLine);
        if (text == "false") return Token(TokenType::FALSE_LIT, text, 0, startLine);

        return Token(TokenType::IDENT, text, 0, startLine);
    }

    // handle single-char symbols and the one two-char case (==)
    Token readSymbol() {
        int startLine = line_;
        char c = advance();
        switch (c) {
            case '+': return Token(TokenType::PLUS,      "+", 0, startLine);
            case '-': return Token(TokenType::MINUS,     "-", 0, startLine);
            case '*': return Token(TokenType::STAR,      "*", 0, startLine);
            case '/': return Token(TokenType::SLASH,     "/", 0, startLine);
            case '<': return Token(TokenType::LESS,      "<", 0, startLine);
            case '(': return Token(TokenType::LPAREN,    "(", 0, startLine);
            case ')': return Token(TokenType::RPAREN,    ")", 0, startLine);
            case '{': return Token(TokenType::LBRACE,    "{", 0, startLine);
            case '}': return Token(TokenType::RBRACE,    "}", 0, startLine);
            case ';': return Token(TokenType::SEMICOLON, ";", 0, startLine);
            case '=':
                // peek ahead - is it == or just =?
                if (pos_ < src_.size() && src_[pos_] == '=') {
                    ++pos_;
                    return Token(TokenType::EQ_EQ, "==", 0, startLine);
                }
                return Token(TokenType::ASSIGN, "=", 0, startLine);
            default:
                throw std::runtime_error(
                    "Unknown character '" + std::string(1, c) +
                    "' at line " + std::to_string(startLine));
        }
    }
};
