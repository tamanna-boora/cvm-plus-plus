#include "lexer.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "vm.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>


static const char* opName(OpCode op) {
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


static void disassemble(const std::vector<Instruction>& code) {
    std::cout << "\n=== Bytecode Disassembly ===\n";
    for (int i = 0; i < static_cast<int>(code.size()); ++i) {
        const auto& ins = code[i];
        std::cout << std::setw(4) << i << "  "
                  << std::left << std::setw(16) << opName(ins.op);
        if (ins.op == OpCode::PUSH_STR) {
            
            std::cout << "  \"" << ins.strOperand << "\"";
        } else if (hasIntOperand(ins.op)) {
            std::cout << std::right << std::setw(6) << ins.operand;
        }
        if (!ins.dbgName.empty())
            std::cout << "  ; " << ins.dbgName;
        std::cout << "\n";
    }
    std::cout << "============================\n\n";
}


static void runSource(const std::string& src, bool debug, bool trace) {
    Lexer lex(src);
    auto tokens = lex.tokenize();

    Parser parser(std::move(tokens));
    auto ast = parser.parse();

    Compiler compiler;
    auto code = compiler.compile(ast);

    if (debug) disassemble(code);

    VM vm;
    vm.run(code, trace);
}


static int runFile(const std::string& path, bool debug, bool trace) {
    std::ifstream f(path);
    if (!f) {
        std::cerr << "Error: cannot open '" << path << "'\n";
        return 1;
    }
    std::ostringstream oss;
    oss << f.rdbuf();
    try {
        runSource(oss.str(), debug, trace);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}


static void runREPL() {
    std::cout << "\nCVM++ v1.0\n";
    std::cout << "commands: exit | debug | trace | clear | ;;\n\n";

    bool        debug   = false;
    bool        trace   = false;
    std::string line;
    std::string session; 
    std::string pending; 

    while (true) {
        std::cout << (pending.empty() ? "cvm> " : "...  ");
        std::cout.flush();

        if (!std::getline(std::cin, line)) break; 

        
        if (line == "exit" || line == "quit") break;

        if (line == "debug") {
            debug = !debug;
            std::cout << "  [debug " << (debug ? "ON" : "OFF") << "]\n";
            continue;
        }

        if (line == "trace") {
            trace = !trace;
            std::cout << "  [trace " << (trace ? "ON" : "OFF") << "]\n";
            continue;
        }

        if (line == "clear") {
            session.clear();
            pending.clear();
            std::cout << "  [session cleared]\n";
            continue;
        }

        pending += line + "\n";

        
        char last = '\0';
        for (char c : line)
            if (!std::isspace(static_cast<unsigned char>(c))) last = c;

        bool forceRun = (line == ";;");
        bool autoRun  = (last == ';' || last == '}');

        if (!forceRun && !autoRun) continue; 

        std::string full = session + pending;
        try {
            runSource(full, debug, trace);
            session = full; 
            pending.clear();
        } catch (const std::exception& e) {
            std::string msg = e.what();
            
            if (msg.rfind("Runtime", 0) == 0) {
                std::cerr << "  Error: " << msg << "\n";
                pending.clear();
            }
            
        }
    }

    std::cout << "\n  Goodbye!\n";
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        runREPL();
        return 0;
    }

    std::string path;
    bool debug = false;
    bool trace = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d")
            debug = true;
        else if (arg == "--trace" || arg == "-t")
            trace = true;
        else
            path = arg;
    }

    if (path.empty()) {
        std::cerr << "Usage: cvm [script.cvm] [--debug] [--trace]\n";
        return 1;
    }

    return runFile(path, debug, trace);
}
