#pragma once
#include "compiler.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <cstdint>

// a value is either an int or a bool - i use a tag to track which one
struct Value {
    enum class Tag { INT, BOOL } tag;
    int64_t intVal  = 0;
    bool    boolVal = false;

    static Value fromInt(int64_t v)  { Value x; x.tag = Tag::INT;  x.intVal  = v; return x; }
    static Value fromBool(bool v)    { Value x; x.tag = Tag::BOOL; x.boolVal = v; return x; }

    std::string toString() const {
        if (tag == Tag::BOOL) return boolVal ? "true" : "false";
        return std::to_string(intVal);
    }

    // used by arithmetic ops - throws if someone tries to add a bool
    int64_t asInt(const std::string& ctx) const {
        if (tag != Tag::INT)
            throw std::runtime_error("Type error: expected int in " + ctx);
        return intVal;
    }

    // basically: booleans use their value, integers treat 0 as false
    bool asBool() const {
        if (tag == Tag::BOOL) return boolVal;
        return intVal != 0;
    }
};

// the actual VM - stack-based, variables in a flat array, one big switch loop
class VM {
public:
    static constexpr int MAX_VARS = 4096; // probably more than enough for now

    VM() : vars_(MAX_VARS, Value::fromInt(0)) {}

    void run(const std::vector<Instruction>& code) {
        stk_.clear();
        int ip = 0; // instruction pointer
        const int limit = static_cast<int>(code.size());

        while (ip < limit) {
            const Instruction& instr = code[ip];
            switch (instr.op) {
                case OpCode::PUSH_INT:
                    push(Value::fromInt(instr.operand));
                    break;

                case OpCode::PUSH_BOOL:
                    push(Value::fromBool(instr.operand != 0));
                    break;

                case OpCode::LOAD:
                    push(vars_[static_cast<size_t>(instr.operand)]);
                    break;

                case OpCode::STORE:
                    vars_[static_cast<size_t>(instr.operand)] = pop();
                    break;

                // arithmetic: pop two values, push the result
                case OpCode::ADD: { auto b = pop(); auto a = pop();
                    push(Value::fromInt(a.asInt("ADD") + b.asInt("ADD"))); break; }

                case OpCode::SUB: { auto b = pop(); auto a = pop();
                    push(Value::fromInt(a.asInt("SUB") - b.asInt("SUB"))); break; }

                case OpCode::MUL: { auto b = pop(); auto a = pop();
                    push(Value::fromInt(a.asInt("MUL") * b.asInt("MUL"))); break; }

                case OpCode::DIV: { auto b = pop(); auto a = pop();
                    // had to add this check after getting a segfault... just kidding,
                    // but division by zero would give garbage without it
                    if (b.asInt("DIV") == 0)
                        throw std::runtime_error("Division by zero");
                    push(Value::fromInt(a.asInt("DIV") / b.asInt("DIV"))); break; }

                // equality compares string representations so int 1 != bool true
                case OpCode::EQ: { auto b = pop(); auto a = pop();
                    push(Value::fromBool(a.toString() == b.toString())); break; }

                case OpCode::LT: { auto b = pop(); auto a = pop();
                    push(Value::fromBool(a.asInt("LT") < b.asInt("LT"))); break; }

                // jumps use continue so ip++ at the bottom doesn't happen
                case OpCode::JMP:
                    ip = static_cast<int>(instr.operand);
                    continue;

                case OpCode::JMP_IF_FALSE: {
                    Value v = pop();
                    if (!v.asBool()) { ip = static_cast<int>(instr.operand); continue; }
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
                    pop(); // just discard the top value
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
    std::vector<Value> stk_;              // the value stack
    std::vector<Value> vars_;             // variable storage, indexed by slot number

    void push(Value v) { stk_.push_back(v); }

    Value pop() {
        if (stk_.empty())
            throw std::runtime_error("Stack underflow");
        Value v = stk_.back();
        stk_.pop_back();
        return v;
    }
};
