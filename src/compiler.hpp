#pragma once
#include "ast.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>


enum class OpCode {
    PUSH_INT,
    PUSH_BOOL,
    PUSH_STR,   
    LOAD,       
    STORE,      
    ADD, SUB, MUL, DIV, MOD,
    EQ, NEQ, LT, LE, GE,
    JMP,
    JMP_IF_FALSE,
    PRINT,
    INPUT,
    POP,        
    HALT
};


struct Instruction {
    OpCode      op;
    int64_t     operand    = 0;
    std::string dbgName;
    std::string strOperand; 
};




class Compiler {
public:
    std::vector<Instruction> compile(const std::shared_ptr<ASTNode>& root) {
        code_.clear();
        vars_.clear();
        numVars_ = 0;
        emitNode(root);
        emit(OpCode::HALT, 0, "HALT");
        return code_;
    }

private:
    std::vector<Instruction> code_;
    std::unordered_map<std::string, int> vars_; 
    int numVars_ = 0; 

    
    int emit(OpCode op, int64_t operand = 0, const std::string& dbg = "") {
        code_.push_back({op, operand, dbg, ""});
        return static_cast<int>(code_.size()) - 1;
    }

    
    int emitStr(const std::string& s) {
        code_.push_back({OpCode::PUSH_STR, 0, "", s});
        return static_cast<int>(code_.size()) - 1;
    }

    int currentPos() const { return static_cast<int>(code_.size()); }

    
    void patch(int idx, int64_t target) { code_[idx].operand = target; }

    int varSlot(const std::string& name) {
        auto it = vars_.find(name);
        if (it != vars_.end()) return it->second;
        throw std::runtime_error("Undefined variable '" + name + "'");
    }

    int declareVar(const std::string& name) {
        if (vars_.count(name))
            throw std::runtime_error("Variable '" + name + "' already declared");
        int slot = numVars_++;
        vars_[name] = slot;
        return slot;
    }

    
    void emitNode(const std::shared_ptr<ASTNode>& n) {
        switch (n->type) {
            
            case NodeType::Program:
            case NodeType::Block:
                for (auto& child : n->children) emitNode(child);
                break;

            case NodeType::IntLit:
                emit(OpCode::PUSH_INT, n->intVal, "PUSH_INT " + std::to_string(n->intVal));
                break;

            case NodeType::BoolLit:
                emit(OpCode::PUSH_BOOL, n->boolVal ? 1 : 0,
                     std::string("PUSH_BOOL ") + (n->boolVal ? "true" : "false"));
                break;

            case NodeType::StringLit:
                emitStr(n->strVal);
                break;

            case NodeType::Variable:
                emit(OpCode::LOAD, varSlot(n->strVal), "LOAD " + n->strVal);
                break;

            case NodeType::InputExpr:
                emit(OpCode::INPUT, 0, "INPUT");
                break;

            
            case NodeType::BinaryExpr:
                emitNode(n->children[0]);
                emitNode(n->children[1]);
                if      (n->strVal == "+")  emit(OpCode::ADD, 0, "ADD");
                else if (n->strVal == "-")  emit(OpCode::SUB, 0, "SUB");
                else if (n->strVal == "*")  emit(OpCode::MUL, 0, "MUL");
                else if (n->strVal == "/")  emit(OpCode::DIV, 0, "DIV");
                else if (n->strVal == "%")  emit(OpCode::MOD, 0, "MOD");
                else if (n->strVal == "==") emit(OpCode::EQ,  0, "EQ");
                else if (n->strVal == "!=") emit(OpCode::NEQ, 0, "NEQ");
                else if (n->strVal == "<")  emit(OpCode::LT,  0, "LT");
                else if (n->strVal == "<=") emit(OpCode::LE,  0, "LE");
                else if (n->strVal == ">=") emit(OpCode::GE,  0, "GE");
                else throw std::runtime_error("Unknown operator: " + n->strVal);
                break;

            case NodeType::LetDecl: {
                int slot = declareVar(n->strVal);
                emitNode(n->children[0]); 
                emit(OpCode::STORE, slot, "STORE " + n->strVal);
                break;
            }

            case NodeType::AssignStmt: {
                int slot = varSlot(n->strVal);
                emitNode(n->children[0]); 
                emit(OpCode::STORE, slot, "STORE " + n->strVal);
                break;
            }

            case NodeType::PrintStmt:
                emitNode(n->children[0]);
                emit(OpCode::PRINT, 0, "PRINT");
                break;

            case NodeType::ExprStmt:
                emitNode(n->children[0]);
                emit(OpCode::POP, 0, "POP"); 
                break;

            case NodeType::IfStmt: {
                
                emitNode(n->children[0]);
                int jifIdx = emit(OpCode::JMP_IF_FALSE, 0, "JMP_IF_FALSE ?");

                emitNode(n->children[1]); 

                if (n->children.size() == 3u) {
                    
                    int jmpIdx = emit(OpCode::JMP, 0, "JMP ?");
                    patch(jifIdx, currentPos()); 
                    emitNode(n->children[2]);
                    patch(jmpIdx, currentPos()); 
                } else {
                    patch(jifIdx, currentPos()); 
                }
                break;
            }

            case NodeType::WhileStmt: {
                
                int loopTop = currentPos();
                emitNode(n->children[0]); 
                int jifIdx = emit(OpCode::JMP_IF_FALSE, 0, "JMP_IF_FALSE ?");
                emitNode(n->children[1]); 
                emit(OpCode::JMP, loopTop, "JMP " + std::to_string(loopTop));
                patch(jifIdx, currentPos()); 
                break;
            }

            case NodeType::ForStmt: {
                
                
                emitNode(n->children[0]); 
                int loopTop = currentPos();
                emitNode(n->children[1]); 
                int jifIdx = emit(OpCode::JMP_IF_FALSE, 0, "JMP_IF_FALSE ?");
                emitNode(n->children[3]); 
                emitNode(n->children[2]); 
                emit(OpCode::JMP, loopTop, "JMP " + std::to_string(loopTop));
                patch(jifIdx, currentPos()); 
                break;
            }

            default:
                throw std::runtime_error("Unknown AST node type in compiler");
        }
    }
};
