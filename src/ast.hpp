#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// all the node types the AST can have - basically one per language construct
enum class NodeType {
    Program,     // top-level list of statements
    Block,       // { ... } block
    IntLit,      // integer literal
    BoolLit,     // true or false
    StringLit,   // "hello" string literal
    Variable,    // variable reference
    BinaryExpr,  // a + b, a < b, a != b, etc.
    InputExpr,   // input()
    LetDecl,     // let x = ...
    AssignStmt,  // x = ...
    IfStmt,      // if (...) { } else { }
    WhileStmt,   // while (...) { }
    ForStmt,     // for (init; cond; update) { } - children=[init,cond,update,body]
    PrintStmt,   // print(...)
    ExprStmt     // expression used as a statement (result discarded)
};

// one big struct for all node types - using a union or variant might be
// "cleaner" but this works fine and is way easier to understand
struct ASTNode {
    NodeType    type;
    int64_t     intVal  = 0;
    bool        boolVal = false;
    std::string strVal;  // variable names, operators, string content
    std::vector<std::shared_ptr<ASTNode>> children;

    explicit ASTNode(NodeType t) : type(t) {}
};

// small factory so i don't have to type make_shared<ASTNode> everywhere
inline std::shared_ptr<ASTNode> makeNode(NodeType t) {
    return std::make_shared<ASTNode>(t);
}
