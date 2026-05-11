# CVM++

A tiny programming language I built from scratch in C++17 for the IIT Guwahati Coding Club Even Semester Project. It has a lexer, a recursive-descent parser, a bytecode compiler, and a stack-based virtual machine — all header-only, no external dependencies.

## How it works

You write `.cvm` files. The lexer turns them into tokens, the parser builds an AST, the compiler walks the tree and emits bytecode, and the VM executes it on a stack. Variables live in a flat array of 4096 slots indexed by integers (names are resolved at compile time).

Supported: integers, booleans (`true`/`false`), `+` `-` `*` `/` `==` `<`, `let`, `if/else`, `while`, `print()`, `input()`, and `//` comments.

## Build

You need g++ with C++17 support. On Windows I used MSYS2 + ucrt64.

```bash
# with make (add C:\msys64\ucrt64\bin to PATH first on Windows)
make

# or just directly
g++ -std=c++17 -Wall -Wextra -O2 -I src -o cvm src/main.cpp
```

## Run

```bash
./cvm examples/hello.cvm
./cvm examples/fibonacci.cvm --debug   # shows bytecode before running
./cvm                                   # starts the interactive REPL
```

In the REPL, lines ending with `;` or `}` auto-execute. Use `debug` to toggle bytecode, `;;` to force-run the buffer, `clear` to wipe the session, `exit` to quit. Variables persist across lines within the same session.

## What I learned

**Backpatching** was the hardest part. When you're compiling a `while` loop, you emit the condition check and then a `JMP_IF_FALSE` — but you don't know where to jump yet because the body hasn't been compiled. So you emit a placeholder `0`, compile the body, then go back and overwrite the `0` with the real address. I spent a long time confused about why loops were jumping to wrong places before this clicked.

**REPL persistent state** was also tricky. The naive approach re-runs everything from scratch each time, which means variables disappear between inputs. My fix is to keep a `session` string of all previously committed source. On each run, I prepend that to the new input and run the whole thing. A bit wasteful but it works.

**Operator precedence** took a few tries to get right. The key insight is that each precedence level is its own function — `parseExpr` calls `parseComparison`, which calls `parseAddSub`, which calls `parseMulDiv`, and so on. Lower levels call higher ones, so higher-priority operators bind more tightly. Once I saw why that works, the rest was straightforward.

## Possible extensions

- `>=` and `!=` operators (just add tokens and opcodes)
- `%` modulo
- String type with a `PUSH_STR` opcode
- Functions using `CALL`/`RET` opcodes and a call stack
- `break` and `continue` in loops (emit placeholder jumps, backpatch at loop end)

## Status
All features verified and tested:
- fibonacci, factorial, fizzbuzz, gcd, primes all passing
- debug mode and trace mode working

