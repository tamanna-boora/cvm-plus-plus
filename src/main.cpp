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

// disassembler - prints out the bytecode in a readable format
// only used when --debug flag is passed
static const char* opName(OpCode op) {
    switch (op) {
        case OpCode::PUSH_INT:      return "PUSH_INT";
        case OpCode::PUSH_BOOL:     return "PUSH_BOOL";
        case OpCode::LOAD:          return "LOAD";
        case OpCode::STORE:         return "STORE";
        case OpCode::ADD:           return "ADD";
        case OpCode::SUB:           return "SUB";
        case OpCode::MUL:           return "MUL";
        case OpCode::DIV:           return "DIV";
        case OpCode::EQ:            return "EQ";
        case OpCode::LT:            return "LT";
        case OpCode::JMP:           return "JMP";
        case OpCode::JMP_IF_FALSE:  return "JMP_IF_FALSE";
        case OpCode::PRINT:         return "PRINT";
        case OpCode::INPUT:         return "INPUT";
        case OpCode::POP:           return "POP";
        case OpCode::HALT:          return "HALT";
        default:                    return "???";
    }
}

// not all opcodes have a meaningful operand, so only print it when relevant
static bool hasOperand(OpCode op) {
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
        const auto& instr = code[i];
        std::cout << std::setw(4) << i << "  "
                  << std::left << std::setw(16) << opName(instr.op);
        if (hasOperand(instr.op))
            std::cout << std::right << std::setw(6) << instr.operand;
        if (!instr.dbgName.empty())
            std::cout << "  ; " << instr.dbgName;
        std::cout << "\n";
    }
    std::cout << "============================\n\n";
}

// run a source string through the full pipeline: lex -> parse -> compile -> run
static void runSource(const std::string& src, bool debug) {
    Lexer lex(src);
    auto tokens = lex.tokenize();

    Parser parser(std::move(tokens));
    auto ast = parser.parse();

    Compiler compiler;
    auto code = compiler.compile(ast);

    if (debug) disassemble(code);

    VM vm;
    vm.run(code);
}

// file mode: read the whole file and run it
static int runFile(const std::string& path, bool debug) {
    std::ifstream f(path);
    if (!f) {
        std::cerr << "Error: cannot open '" << path << "'\n";
        return 1;
    }
    std::ostringstream oss;
    oss << f.rdbuf();
    try {
        runSource(oss.str(), debug);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

// interactive REPL
static void runREPL() {
    // simple banner - nothing fancy
    std::cout << "\nCVM++ v1.0 - my custom language\n";
    std::cout << "type exit to quit, debug to toggle bytecode, ;; to force run\n\n";

    bool        debug   = false;
    std::string line;
    std::string session;  // accumulated source from all previous runs (gives persistent variables)
    std::string pending;  // lines typed since last run

    while (true) {
        std::cout << (pending.empty() ? "cvm> " : "...  ");
        std::cout.flush();

        if (!std::getline(std::cin, line)) break; // EOF

        // built-in commands
        if (line == "exit" || line == "quit") break;

        if (line == "debug") {
            debug = !debug;
            std::cout << "  [debug " << (debug ? "ON" : "OFF") << "]\n";
            continue;
        }

        if (line == "clear") {
            // reset everything - variables are gone after this
            session.clear();
            pending.clear();
            std::cout << "  [session cleared]\n";
            continue;
        }

        pending += line + "\n";

        // figure out the last non-whitespace character
        char last = '\0';
        for (char c : line) if (!std::isspace(c)) last = c;

        bool forceRun = (line == ";;");
        bool autoRun  = (last == ';' || last == '}');

        if (!forceRun && !autoRun) continue; // keep buffering, not done yet

        // try to compile and run session + new input together
        std::string full = session + pending;
        try {
            runSource(full, debug);
            session = full;  // commit: variables now persist across inputs
            pending.clear();
        } catch (const std::exception& e) {
            std::string msg = e.what();
            // runtime errors (div by zero etc) - discard pending and report
            if (msg.rfind("Runtime", 0) == 0) {
                std::cerr << "  Error: " << msg << "\n";
                pending.clear();
            }
            // lex/parse errors might just mean the input is incomplete, so keep buffering
            // TODO: maybe handle this better later - distinguish real errors from incomplete input
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

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d")
            debug = true;
        else
            path = arg;
    }

    if (path.empty()) {
        std::cerr << "Usage: cvm [script.cvm] [--debug]\n";
        return 1;
    }

    return runFile(path, debug);
}
