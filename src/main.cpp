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
static void printBanner() {
    std::cout <<
        "  ____   __   __ __  __     _     _\n"
        " / ___| \\ \\ / /|  \\/  |_  _| |_  | |_\n"
        "| |      \\ V / | |\\/| | || |  _+ |  _|\n"
        "|  ___    | |  | |  | | || | |_  | |_\n"
        " \\____|   |_|  |_|  |_|\\__/ \\__|  \\__|\n"
        "\n"
        " CVM++ v1.0  —  Stack-based VM & Compiler\n"
        " Commands: exit | debug | ;; (force exec)\n"
        " Statements auto-execute on ';' or '}'\n\n";
}

static void replLoop() {
    printBanner();
    std::string buffer;
    bool debug = false;

    auto execBuffer = [&]() {
        if (buffer.empty()) return;
        try {
            runSource(buffer, debug);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
        buffer.clear();
    };

    while (true) {
        std::cout << (buffer.empty() ? "cvm> " : "  .. ");
        std::cout.flush();
        std::string line;
        if (!std::getline(std::cin, line)) break;

        // meta-commands
        if (line == "exit") break;
        if (line == "debug") {
            debug = !debug;
            std::cout << "Debug mode " << (debug ? "ON" : "OFF") << "\n";
            continue;
        }
        if (line == ";;") { execBuffer(); continue; }

        buffer += line + "\n";

        // auto-execute heuristic: last non-space char is ';' or '}'
        std::string trimmed = line;
        size_t last = trimmed.find_last_not_of(" \t\r\n");
        if (last != std::string::npos) {
            char ch = trimmed[last];
            if (ch == ';' || ch == '}') execBuffer();
        }
    }
    std::cout << "Bye!\n";
}

// ── entry point ───────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc == 1) {
        replLoop();
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
