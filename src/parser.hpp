#pragma once
#include "lexer.hpp"
#include "ast.hpp"
#include <stdexcept>
#include <string>

// recursive descent parser - each method handles one level of the grammar.
// getting operator precedence right took me a while - the trick is that
// each precedence level calls the one above it, so higher-priority stuff
// binds tighter naturally.
class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens_(std::move(tokens)), idx_(0) {}

    std::shared_ptr<ASTNode> parse() {
        auto prog = makeNode(NodeType::Program);
        while (!check(TokenType::EOF_TOK)) {
            prog->children.push_back(parseStatement());
        }
        return prog;
    }

private:
    std::vector<Token> tokens_;
    size_t idx_; // current position in the token list

    // basic helpers for peeking at and consuming tokens
    const Token& current() const { return tokens_[idx_]; }

    bool check(TokenType t) const { return current().type == t; }

    // look one token ahead without consuming
    bool checkNext(TokenType t) const {
        return (idx_ + 1 < tokens_.size()) && (tokens_[idx_ + 1].type == t);
    }

    bool match(TokenType t) {
        if (check(t)) { ++idx_; return true; }
        return false;
    }

    // consume expects a specific token and throws a readable error if it's wrong
    const Token& consume(TokenType t, const std::string& msg) {
        if (!check(t))
            throw std::runtime_error(msg + " at line " + std::to_string(current().line) +
                                     " (got '" + current().text + "')");
        return tokens_[idx_++];
    }

    // statements
    std::shared_ptr<ASTNode> parseStatement() {
        if (check(TokenType::LET))    return parseLetDecl();
        if (check(TokenType::IF))     return parseIf();
        if (check(TokenType::WHILE))  return parseWhile();
        if (check(TokenType::FOR))    return parseFor();
        if (check(TokenType::PRINT))  return parsePrint();
        if (check(TokenType::LBRACE)) return parseBlock();

        // tricky part: IDENT could be an assignment (x = ...) or just an expression
        // so we peek one token ahead to decide
        if (check(TokenType::IDENT) && checkNext(TokenType::ASSIGN)) {
            std::string name = tokens_[idx_].text;
            idx_ += 2; // consume ident and =
            auto node = makeNode(NodeType::AssignStmt);
            node->strVal = name;
            node->children.push_back(parseExpr());
            consume(TokenType::SEMICOLON, "Expected ';' after assignment");
            return node;
        }

        // otherwise it's just an expression statement (result gets popped)
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

        // else is optional, and else-if just recurses into parseIf
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

    // for (init; cond; update) { body }
    // children layout: [0]=init, [1]=cond, [2]=update, [3]=body
    std::shared_ptr<ASTNode> parseFor() {
        consume(TokenType::FOR, "Expected 'for'");
        consume(TokenType::LPAREN, "Expected '(' after 'for'");

        // init: let decl or assignment/expr - both consume their trailing semicolon
        std::shared_ptr<ASTNode> init;
        if (check(TokenType::LET)) {
            init = parseLetDecl();
        } else {
            init = parseExprOrAssignWithSemi();
        }

        // condition expression then semicolon
        auto cond = parseExpr();
        consume(TokenType::SEMICOLON, "Expected ';' after for condition");

        // update: assignment without trailing semicolon, or expression
        // (no semicolon because ) comes right after)
        std::shared_ptr<ASTNode> update;
        if (check(TokenType::IDENT) && checkNext(TokenType::ASSIGN)) {
            std::string name = tokens_[idx_].text;
            idx_ += 2; // consume ident and =
            auto upd = makeNode(NodeType::AssignStmt);
            upd->strVal = name;
            upd->children.push_back(parseExpr());
            update = upd;
        } else {
            auto upd = makeNode(NodeType::ExprStmt);
            upd->children.push_back(parseExpr());
            update = upd;
        }

        consume(TokenType::RPAREN, "Expected ')' after for update");
        auto body = parseBlock();

        auto node = makeNode(NodeType::ForStmt);
        node->children.push_back(init);    // [0] init
        node->children.push_back(cond);    // [1] condition
        node->children.push_back(update);  // [2] update
        node->children.push_back(body);    // [3] body
        return node;
    }

    // helper for for-init: parses `x = expr;` or `expr;` (with semicolon)
    std::shared_ptr<ASTNode> parseExprOrAssignWithSemi() {
        if (check(TokenType::IDENT) && checkNext(TokenType::ASSIGN)) {
            std::string name = tokens_[idx_].text;
            idx_ += 2;
            auto node = makeNode(NodeType::AssignStmt);
            node->strVal = name;
            node->children.push_back(parseExpr());
            consume(TokenType::SEMICOLON, "Expected ';' in for-init");
            return node;
        }
        auto node = makeNode(NodeType::ExprStmt);
        node->children.push_back(parseExpr());
        consume(TokenType::SEMICOLON, "Expected ';' in for-init");
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

    // expression parsing - precedence levels from lowest to highest:
    // comparison (==, !=, <, <=, >=)  ->  add/sub  ->  mul/div/mod  ->  unary minus  ->  primary
    std::shared_ptr<ASTNode> parseExpr() { return parseComparison(); }

    std::shared_ptr<ASTNode> parseComparison() {
        auto left = parseAddSub();
        while (check(TokenType::EQ_EQ) || check(TokenType::NEQ) ||
               check(TokenType::LESS)  || check(TokenType::LESS_EQ) ||
               check(TokenType::GREATER_EQ))
        {
            std::string op = current().text;
            ++idx_;
            auto right = parseAddSub();
            auto node  = makeNode(NodeType::BinaryExpr);
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
            ++idx_;
            auto right = parseMulDiv();
            auto node  = makeNode(NodeType::BinaryExpr);
            node->strVal = op;
            node->children.push_back(left);
            node->children.push_back(right);
            left = node;
        }
        return left;
    }

    std::shared_ptr<ASTNode> parseMulDiv() {
        auto left = parseUnary();
        while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
            std::string op = current().text;
            ++idx_;
            auto right = parseUnary();
            auto node  = makeNode(NodeType::BinaryExpr);
            node->strVal = op;
            node->children.push_back(left);
            node->children.push_back(right);
            left = node;
        }
        return left;
    }

    std::shared_ptr<ASTNode> parseUnary() {
        if (check(TokenType::MINUS)) {
            ++idx_;
            auto operand = parseUnary();
            // rewrite -x as (0 - x) so the compiler doesn't need a separate NEG opcode
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

    // literals, variables, input(), and parenthesized expressions
    std::shared_ptr<ASTNode> parsePrimary() {
        if (check(TokenType::INT_LIT)) {
            auto node = makeNode(NodeType::IntLit);
            node->intVal = current().intVal;
            ++idx_;
            return node;
        }
        if (check(TokenType::TRUE_LIT)) {
            auto node = makeNode(NodeType::BoolLit);
            node->boolVal = true;
            ++idx_;
            return node;
        }
        if (check(TokenType::FALSE_LIT)) {
            auto node = makeNode(NodeType::BoolLit);
            node->boolVal = false;
            ++idx_;
            return node;
        }
        if (check(TokenType::STRING_LIT)) {
            auto node = makeNode(NodeType::StringLit);
            node->strVal = current().text; // text holds the string content
            ++idx_;
            return node;
        }
        if (check(TokenType::IDENT)) {
            auto node = makeNode(NodeType::Variable);
            node->strVal = current().text;
            ++idx_;
            return node;
        }
        if (check(TokenType::INPUT)) {
            ++idx_;
            consume(TokenType::LPAREN, "Expected '(' after 'input'");
            consume(TokenType::RPAREN, "Expected ')' after 'input('");
            return makeNode(NodeType::InputExpr);
        }
        if (match(TokenType::LPAREN)) {
            auto expr = parseExpr();
            consume(TokenType::RPAREN, "Expected ')' after expression");
            return expr;
        }
        // TODO: maybe handle this better later - right now just throws
        throw std::runtime_error(
            "Unexpected token '" + current().text +
            "' at line " + std::to_string(current().line));
    }
};
