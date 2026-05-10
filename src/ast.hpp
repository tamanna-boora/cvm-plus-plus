#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

enum class NodeType {
    Program,
    Block,
    IntLit,
    BoolLit,
    Variable,
    BinaryExpr,
    InputExpr,
    LetDecl,
    AssignStmt,
    IfStmt,
    WhileStmt,
    PrintStmt,
    ExprStmt
};

struct ASTNode {
    NodeType        type;
    int64_t         intVal  = 0;
    bool            boolVal = false;
    std::string     strVal;
    std::vector<std::shared_ptr<ASTNode>> children;

    explicit ASTNode(NodeType t) : type(t) {}
};

inline std::shared_ptr<ASTNode> makeNode(NodeType t) {
    return std::make_shared<ASTNode>(t);
}
