#pragma once
#include "compiler.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstdint>


struct Value {
    enum class Tag { INT, BOOL, STR } tag;
    int64_t     intVal  = 0;
    bool        boolVal = false;
    std::string strVal;

    static Value fromInt(int64_t v)        { Value x; x.tag = Tag::INT;  x.intVal  = v; return x; }
    static Value fromBool(bool v)          { Value x; x.tag = Tag::BOOL; x.boolVal = v; return x; }
    static Value ofStr(const std::string& s) { Value x; x.tag = Tag::STR;  x.strVal  = s; return x; }

    std::string toString() const {
        switch (tag) {
            case Tag::INT:  return std::to_string(intVal);
            case Tag::BOOL: return boolVal ? "true" : "false";
            case Tag::STR:  return strVal;
        }
        return "";
    }

    
    bool isTruthy() const {
        switch (tag) {
            case Tag::INT:  return intVal != 0;
            case Tag::BOOL: return boolVal;
            case Tag::STR:  return !strVal.empty();
        }
        return false;
    }

    
    bool asBool() const { return isTruthy(); }

    
    int64_t asInt(const std::string& ctx) const {
        if (tag != Tag::INT)
            throw std::runtime_error("Type error: expected int in " + ctx);
        return intVal;
    }

    
    bool equals(const Value& other) const {
        if (tag != other.tag) return false;
        switch (tag) {
            case Tag::INT:  return intVal  == other.intVal;
            case Tag::BOOL: return boolVal == other.boolVal;
            case Tag::STR:  return strVal  == other.strVal;
        }
        return false;
    }
};


class VM {
public:
    static constexpr int MAX_VARS = 4096; 

    VM() : vars_(MAX_VARS, Value::fromInt(0)) {}

    
    void run(const std::vector<Instruction>& code, bool trace = false) {
        stk_.clear();
        int       ip    = 0;
        const int limit = static_cast<int>(code.size());

        while (ip < limit) {
            const Instruction& ins = code[ip];

            if (trace) {
                std::cerr << "[ip=" << std::setw(4) << ip << "] "
                          << std::left << std::setw(14) << opcodeName(ins.op);
                if (ins.op == OpCode::PUSH_STR)
                    std::cerr << " \"" << ins.strOperand << "\"";
                else if (hasIntOperand(ins.op))
                    std::cerr << std::right << " " << ins.operand;
                std::cerr << "\n";
            }

            switch (ins.op) {
                case OpCode::PUSH_INT:
                    push(Value::fromInt(ins.operand));
                    break;

                case OpCode::PUSH_BOOL:
                    push(Value::fromBool(ins.operand != 0));
                    break;

                case OpCode::PUSH_STR:
                    push(Value::ofStr(ins.strOperand));
                    break;

                case OpCode::LOAD:
                    push(vars_[static_cast<size_t>(ins.operand)]);
                    break;

                case OpCode::STORE:
                    vars_[static_cast<size_t>(ins.operand)] = pop();
                    break;

                
                case OpCode::ADD: { auto b = pop(); auto a = pop();
                    push(Value::fromInt(a.asInt("ADD") + b.asInt("ADD"))); break; }

                case OpCode::SUB: { auto b = pop(); auto a = pop();
                    push(Value::fromInt(a.asInt("SUB") - b.asInt("SUB"))); break; }

                case OpCode::MUL: { auto b = pop(); auto a = pop();
                    push(Value::fromInt(a.asInt("MUL") * b.asInt("MUL"))); break; }

                case OpCode::DIV: { auto b = pop(); auto a = pop();
                    if (b.asInt("DIV") == 0)
                        throw std::runtime_error("Division by zero");
                    push(Value::fromInt(a.asInt("DIV") / b.asInt("DIV"))); break; }

                case OpCode::MOD: { auto b = pop(); auto a = pop();
                    if (b.asInt("MOD") == 0)
                        throw std::runtime_error("Modulo by zero");
                    push(Value::fromInt(a.asInt("MOD") % b.asInt("MOD"))); break; }

                case OpCode::EQ: { auto b = pop(); auto a = pop();
                    push(Value::fromBool(a.equals(b))); break; }

                case OpCode::NEQ: { auto b = pop(); auto a = pop();
                    push(Value::fromBool(!a.equals(b))); break; }

                case OpCode::LT: { auto b = pop(); auto a = pop();
                    push(Value::fromBool(a.asInt("LT") < b.asInt("LT"))); break; }

                case OpCode::LE: { auto b = pop(); auto a = pop();
                    push(Value::fromBool(a.asInt("LE") <= b.asInt("LE"))); break; }

                case OpCode::GE: { auto b = pop(); auto a = pop();
                    push(Value::fromBool(a.asInt("GE") >= b.asInt("GE"))); break; }

                
                case OpCode::JMP:
                    ip = static_cast<int>(ins.operand);
                    continue;

                case OpCode::JMP_IF_FALSE: {
                    Value v = pop();
                    if (!v.asBool()) { ip = static_cast<int>(ins.operand); continue; }
                    break;
                }

                case OpCode::PRINT:
                    std::cout << pop().toString() << "\n";
                    break;

                case OpCode::INPUT: {
                    int64_t n = 0;
                    if (!(std::cin >> n))
                        throw std::runtime_error("Failed to read integer from stdin");
                    push(Value::fromInt(n));
                    break;
                }

                case OpCode::POP:
                    pop(); 
                    break;

                case OpCode::HALT:
                    return;

                default:
                    throw std::runtime_error("Unknown opcode at ip=" + std::to_string(ip));
            }
            ++ip;
        }
    }

private:
    std::vector<Value> stk_;  
    std::vector<Value> vars_; 

    void push(Value v) { stk_.push_back(v); }

    Value pop() {
        if (stk_.empty())
            throw std::runtime_error("Stack underflow");
        Value v = stk_.back();
        stk_.pop_back();
        return v;
    }

    
    static const char* opcodeName(OpCode op) {
        switch (op) {
            case OpCode::PUSH_INT:      return "PUSH_INT";
            case OpCode::PUSH_BOOL:     return "PUSH_BOOL";
            case OpCode::PUSH_STR:      return "PUSH_STR";
            case OpCode::LOAD:          return "LOAD";
            case OpCode::STORE:         return "STORE";
            case OpCode::ADD:           return "ADD";
            case OpCode::SUB:           return "SUB";
            case OpCode::MUL:           return "MUL";
            case OpCode::DIV:           return "DIV";
            case OpCode::MOD:           return "MOD";
            case OpCode::EQ:            return "EQ";
            case OpCode::NEQ:           return "NEQ";
            case OpCode::LT:            return "LT";
            case OpCode::LE:            return "LE";
            case OpCode::GE:            return "GE";
            case OpCode::JMP:           return "JMP";
            case OpCode::JMP_IF_FALSE:  return "JMP_IF_FALSE";
            case OpCode::PRINT:         return "PRINT";
            case OpCode::INPUT:         return "INPUT";
            case OpCode::POP:           return "POP";
            case OpCode::HALT:          return "HALT";
            default:                    return "???";
        }
    }

    
    static bool hasIntOperand(OpCode op) {
        switch (op) {
            case OpCode::PUSH_INT:
            case OpCode::PUSH_BOOL:
            case OpCode::LOAD:
            case OpCode::STORE:
            case OpCode::JMP:
            case OpCode::JMP_IF_FALSE:
                return true;
            default:
                return false;
        }
    }
};
