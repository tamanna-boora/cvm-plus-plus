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

// ── disassembler ──────────────────────────────────────────────────────────────
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

// ── run a complete source string ──────────────────────────────────────────────
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

// ── file mode ─────────────────────────────────────────────────────────────────
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

// ── REPL ──────────────────────────────────────────────────────────────────────
static void runREPL() {
    std::cout << R"(
 ██████╗██╗   ██╗███╗   ███╗  ██╗  ██╗
██╔════╝██║   ██║████╗ ████║  ██║  ██║
██║     ██║   ██║██╔████╔██║  ███████║
██║     ╚██╗ ██╔╝██║╚██╔╝██║  ╚════██║
╚██████╗ ╚████╔╝ ██║ ╚═╝ ██║       ██║
 ╚═════╝  ╚═══╝  ╚═╝     ╚═╝       ╚═╝
)";
    std::cout << "  CVM++ REPL v1.0\n";
    std::cout << "  Commands: exit | debug | clear | ;; (force run)\n\n";

    bool        debug   = false;
    std::string line;
    std::string session;  // ALL committed source — gives persistent variables
    std::string pending;  // lines typed since last successful run

    while (true) {
        std::cout << (pending.empty() ? "cvm> " : "...  ");
        std::cout.flush();

        if (!std::getline(std::cin, line)) break;  // EOF (Ctrl+D / Ctrl+Z)

        // ── built-in commands ─────────────────────────────────────────────
        if (line == "exit" || line == "quit") break;

        if (line == "debug") {
            debug = !debug;
            std::cout << "  [debug " << (debug ? "ON" : "OFF") << "]\n";
            continue;
        }

        if (line == "clear") {
            session.clear();
            pending.clear();
            std::cout << "  [session cleared]\n";
            continue;
        }

        // ── accumulate input ──────────────────────────────────────────────
        pending += line + "\n";

        // Determine last non-whitespace character
        char last = '\0';
        for (char c : line) if (!std::isspace(c)) last = c;

        bool forceRun = (line == ";;");
        bool autoRun  = (last == ';' || last == '}');

        if (!forceRun && !autoRun) continue;  // keep buffering

        // ── try to compile & run session + pending ────────────────────────
        std::string full = session + pending;
        try {
            runSource(full, debug);
            session = full;   // commit: variables now persist
            pending.clear();
        } catch (const std::exception& e) {
            std::string msg = e.what();
            // Runtime errors (div/0, underflow) → discard pending, keep session
            if (msg.rfind("Runtime", 0) == 0) {
                std::cerr << "  Error: " << msg << "\n";
                pending.clear();
            }
            // Lex/parse errors → could be incomplete input, keep buffering
            // (user will see nothing — they can type more or use ;; to force)
        }
    }

    std::cout << "\n  Goodbye!\n";
}

// ── entry point ───────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // No arguments → interactive REPL
    if (argc == 1) {
        runREPL();   // ← was wrongly called replLoop() before
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