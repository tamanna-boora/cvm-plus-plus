#pragma once
#include "ast.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>

// opcodes for the VM - basically the instruction set of my little CPU
enum class OpCode {
    PUSH_INT,
    PUSH_BOOL,
    LOAD,       // load a variable onto the stack
    STORE,      // pop stack and save into a variable slot
    ADD, SUB, MUL, DIV,
    EQ, LT,
    JMP,
    JMP_IF_FALSE,
    PRINT,
    INPUT,
    POP,        // discard the top of stack (for expression statements)
    HALT
};

// one instruction: the opcode, an optional integer operand, and a debug label
struct Instruction {
    OpCode  op;
    int64_t operand = 0;
    std::string dbgName; // only used when --debug flag is on
};

// walks the AST and emits a flat list of instructions.
// the trickiest part was backpatching - when you emit a jump you don't know
// the target yet, so you emit 0 as a placeholder and fix it later.
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
    std::unordered_map<std::string, int> vars_; // maps variable name -> slot index
    int numVars_ = 0; // how many variables we've declared so far

    // emit an instruction and return its index (needed for backpatching)
    int emit(OpCode op, int64_t operand = 0, const std::string& dbg = "") {
        code_.push_back({op, operand, dbg});
        return static_cast<int>(code_.size()) - 1;
    }

    int currentPos() const { return static_cast<int>(code_.size()); }

    // go back and fix a previously emitted jump target
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

    // recursive tree walk - each node type emits its own instructions
    void emitNode(const std::shared_ptr<ASTNode>& n) {
        switch (n->type) {
            // program and block just process their children in order
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

            case NodeType::Variable:
                emit(OpCode::LOAD, varSlot(n->strVal), "LOAD " + n->strVal);
                break;

            case NodeType::InputExpr:
                emit(OpCode::INPUT, 0, "INPUT");
                break;

            // for binary ops: push both sides then the operation
            case NodeType::BinaryExpr:
                emitNode(n->children[0]);
                emitNode(n->children[1]);
                if      (n->strVal == "+")  emit(OpCode::ADD, 0, "ADD");
                else if (n->strVal == "-")  emit(OpCode::SUB, 0, "SUB");
                else if (n->strVal == "*")  emit(OpCode::MUL, 0, "MUL");
                else if (n->strVal == "/")  emit(OpCode::DIV, 0, "DIV");
                else if (n->strVal == "==") emit(OpCode::EQ,  0, "EQ");
                else if (n->strVal == "<")  emit(OpCode::LT,  0, "LT");
                else throw std::runtime_error("Unknown operator: " + n->strVal);
                break;

            case NodeType::LetDecl: {
                int slot = declareVar(n->strVal);
                emitNode(n->children[0]); // push initial value
                emit(OpCode::STORE, slot, "STORE " + n->strVal);
                break;
            }

            case NodeType::AssignStmt: {
                int slot = varSlot(n->strVal);
                emitNode(n->children[0]); // push new value
                emit(OpCode::STORE, slot, "STORE " + n->strVal);
                break;
            }

            case NodeType::PrintStmt:
                emitNode(n->children[0]);
                emit(OpCode::PRINT, 0, "PRINT");
                break;

            case NodeType::ExprStmt:
                emitNode(n->children[0]);
                emit(OpCode::POP, 0, "POP"); // result not used, throw it away
                break;

            case NodeType::IfStmt: {
                // children[0]=condition, children[1]=then, children[2]=else (optional)
                emitNode(n->children[0]);
                int jifIdx = emit(OpCode::JMP_IF_FALSE, 0, "JMP_IF_FALSE ?"); // placeholder

                emitNode(n->children[1]); // then branch

                if (n->children.size() == 3) {
                    // has else: emit a jump to skip the else, then fill in both targets
                    int jmpIdx = emit(OpCode::JMP, 0, "JMP ?");
                    patch(jifIdx, currentPos()); // false jumps here (start of else)
                    emitNode(n->children[2]);
                    patch(jmpIdx, currentPos()); // then jumps here (past else)
                } else {
                    patch(jifIdx, currentPos()); // false jumps past the then block
                }
                break;
            }

            case NodeType::WhileStmt: {
                // children[0]=condition, children[1]=body
                // this took me a while to get right - you have to remember where the
                // top of the loop is so the JMP at the end can go back to it
                int loopTop = currentPos();
                emitNode(n->children[0]); // check condition each iteration
                int jifIdx = emit(OpCode::JMP_IF_FALSE, 0, "JMP_IF_FALSE ?");
                emitNode(n->children[1]); // loop body
                emit(OpCode::JMP, loopTop, "JMP " + std::to_string(loopTop));
                patch(jifIdx, currentPos()); // exit the loop when condition is false
                break;
            }

            default:
                throw std::runtime_error("Unknown AST node type in compiler");
        }
    }
};
