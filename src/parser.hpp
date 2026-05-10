#pragma once
#include "lexer.hpp"
#include "ast.hpp"
#include <stdexcept>
#include <string>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens_(std::move(tokens)), pos_(0) {}

    std::shared_ptr<ASTNode> parse() {
        auto prog = makeNode(NodeType::Program);
        while (!check(TokenType::EOF_TOK)) {
            prog->children.push_back(parseStatement());
        }
        return prog;
    }

private:
    std::vector<Token> tokens_;
    size_t pos_;

    // ── token helpers ──────────────────────────────────────────────────────
    const Token& current() const { return tokens_[pos_]; }

    bool check(TokenType t) const { return current().type == t; }

    bool match(TokenType t) {
        if (check(t)) { ++pos_; return true; }
        return false;
    }

    const Token& consume(TokenType t, const std::string& msg) {
        if (!check(t))
            throw std::runtime_error(msg + " at line " + std::to_string(current().line) +
                                     " (got '" + current().text + "')");
        return tokens_[pos_++];
    }

    // ── statements ─────────────────────────────────────────────────────────
    std::shared_ptr<ASTNode> parseStatement() {
        if (check(TokenType::LET))   return parseLetDecl();
        if (check(TokenType::IF))    return parseIf();
        if (check(TokenType::WHILE)) return parseWhile();
        if (check(TokenType::PRINT)) return parsePrint();
        if (check(TokenType::LBRACE)) return parseBlock();

        // assignment  or  expression statement
        if (check(TokenType::IDENT)) {
            size_t saved = pos_;
            std::string name = tokens_[pos_].text;
            ++pos_;
            if (match(TokenType::ASSIGN)) {
                auto node = makeNode(NodeType::AssignStmt);
                node->strVal = name;
                node->children.push_back(parseExpr());
                consume(TokenType::SEMICOLON, "Expected ';' after assignment");
                return node;
            }
            pos_ = saved; // back-track
        }

        auto node = makeNode(NodeType::ExprStmt);
        node->children.push_back(parseExpr());
        consume(TokenType::SEMICOLON, "Expected ';' after expression");
        return node;
    }

    std::shared_ptr<ASTNode> parseBlock() {
        consume(TokenType::LBRACE, "Expected '{'");
        auto block = makeNode(NodeType::Block);
        while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK))
            block->children.push_back(parseStatement());
        consume(TokenType::RBRACE, "Expected '}'");
        return block;
    }

    std::shared_ptr<ASTNode> parseLetDecl() {
        consume(TokenType::LET, "Expected 'let'");
        const Token& name = consume(TokenType::IDENT, "Expected variable name after 'let'");
        consume(TokenType::ASSIGN, "Expected '=' after variable name");
        auto node = makeNode(NodeType::LetDecl);
        node->strVal = name.text;
        node->children.push_back(parseExpr());
        consume(TokenType::SEMICOLON, "Expected ';' after let declaration");
        return node;
    }

    std::shared_ptr<ASTNode> parseIf() {
        consume(TokenType::IF, "Expected 'if'");
        consume(TokenType::LPAREN, "Expected '(' after 'if'");
        auto cond = parseExpr();
        consume(TokenType::RPAREN, "Expected ')' after if condition");
        auto thenBranch = parseBlock();

        auto node = makeNode(NodeType::IfStmt);
        node->children.push_back(cond);
        node->children.push_back(thenBranch);

        if (match(TokenType::ELSE)) {
            if (check(TokenType::IF))
                node->children.push_back(parseIf());
            else
                node->children.push_back(parseBlock());
        }
        return node;
    }

    std::shared_ptr<ASTNode> parseWhile() {
        consume(TokenType::WHILE, "Expected 'while'");
        consume(TokenType::LPAREN, "Expected '(' after 'while'");
        auto cond = parseExpr();
        consume(TokenType::RPAREN, "Expected ')' after while condition");
        auto body = parseBlock();

        auto node = makeNode(NodeType::WhileStmt);
        node->children.push_back(cond);
        node->children.push_back(body);
        return node;
    }

    std::shared_ptr<ASTNode> parsePrint() {
        consume(TokenType::PRINT, "Expected 'print'");
        consume(TokenType::LPAREN, "Expected '(' after 'print'");
        auto node = makeNode(NodeType::PrintStmt);
        node->children.push_back(parseExpr());
        consume(TokenType::RPAREN, "Expected ')' after print argument");
        consume(TokenType::SEMICOLON, "Expected ';' after print(...)");
        return node;
    }

    // ── expressions (precedence: comparison < add/sub < mul/div < unary < primary) ──
    std::shared_ptr<ASTNode> parseExpr() { return parseComparison(); }

    std::shared_ptr<ASTNode> parseComparison() {
        auto left = parseAddSub();
        while (check(TokenType::EQ_EQ) || check(TokenType::LESS)) {
            std::string op = current().text;
            ++pos_;
            auto right = parseAddSub();
            auto node = makeNode(NodeType::BinaryExpr);
            node->strVal = op;
            node->children.push_back(left);
            node->children.push_back(right);
            left = node;
        }
        return left;
    }

    std::shared_ptr<ASTNode> parseAddSub() {
        auto left = parseMulDiv();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            std::string op = current().text;
            ++pos_;
            auto right = parseMulDiv();
            auto node = makeNode(NodeType::BinaryExpr);
            node->strVal = op;
            node->children.push_back(left);
            node->children.push_back(right);
            left = node;
        }
        return left;
    }

    std::shared_ptr<ASTNode> parseMulDiv() {
        auto left = parseUnary();
        while (check(TokenType::STAR) || check(TokenType::SLASH)) {
            std::string op = current().text;
            ++pos_;
            auto right = parseUnary();
            auto node = makeNode(NodeType::BinaryExpr);
            node->strVal = op;
            node->children.push_back(left);
            node->children.push_back(right);
            left = node;
        }
        return left;
    }

    std::shared_ptr<ASTNode> parseUnary() {
        if (check(TokenType::MINUS)) {
            ++pos_;
            auto operand = parseUnary();
            // fold into  (0 - operand)
            auto zero = makeNode(NodeType::IntLit);
            zero->intVal = 0;
            auto node = makeNode(NodeType::BinaryExpr);
            node->strVal = "-";
            node->children.push_back(zero);
            node->children.push_back(operand);
            return node;
        }
        return parsePrimary();
    }

    std::shared_ptr<ASTNode> parsePrimary() {
        if (check(TokenType::INT_LIT)) {
            auto node = makeNode(NodeType::IntLit);
            node->intVal = current().intVal;
            ++pos_;
            return node;
        }
        if (check(TokenType::TRUE_LIT)) {
            auto node = makeNode(NodeType::BoolLit);
            node->boolVal = true;
            ++pos_;
            return node;
        }
        if (check(TokenType::FALSE_LIT)) {
            auto node = makeNode(NodeType::BoolLit);
            node->boolVal = false;
            ++pos_;
            return node;
        }
        if (check(TokenType::IDENT)) {
            auto node = makeNode(NodeType::Variable);
            node->strVal = current().text;
            ++pos_;
            return node;
        }
        if (check(TokenType::INPUT)) {
            ++pos_;
            consume(TokenType::LPAREN, "Expected '(' after 'input'");
            consume(TokenType::RPAREN, "Expected ')' after 'input('");
            return makeNode(NodeType::InputExpr);
        }
        if (match(TokenType::LPAREN)) {
            auto expr = parseExpr();
            consume(TokenType::RPAREN, "Expected ')' after expression");
            return expr;
        }
        throw std::runtime_error(
            "Unexpected token '" + current().text +
            "' at line " + std::to_string(current().line));
    }
};
