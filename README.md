# CVM++ — Stack-based Virtual Machine & Compiler

A complete, self-contained language implementation in C++17: lexer → parser → compiler → bytecode VM.

---

## Build

### Make (Linux / macOS / MinGW on Windows)
```bash
make          # produces ./cvm
make clean
```

### CMake
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Requires: g++ with C++17 support (GCC 7+, Clang 5+) or MSVC 2017+. No external dependencies.

---

## Usage

**Run a script**
```bash
./cvm examples/hello.cvm
./cvm examples/fibonacci.cvm --debug   # show bytecode disassembly first
```

**Interactive REPL**
```bash
./cvm
```

REPL commands:
| Command | Effect |
|---------|--------|
| `exit`  | Quit the REPL |
| `debug` | Toggle bytecode disassembly |
| `;;`    | Force-execute the current buffer |

Lines ending with `;` or `}` are auto-executed.

---

## Language Reference

### Data Types
| Type    | Literals        |
|---------|-----------------|
| Integer | `0`, `42`, `-7` |
| Boolean | `true`, `false` |

### Operators
| Operator | Description    | Precedence |
|----------|----------------|-----------|
| `==`     | Equality       | lowest     |
| `<`      | Less-than      | lowest     |
| `+`      | Addition       | medium     |
| `-`      | Subtraction    | medium     |
| `*`      | Multiplication | high       |
| `/`      | Division       | high       |
| `-` (unary) | Negation    | highest    |

### Statements
```
let x = expr;              // variable declaration
x = expr;                  // assignment
if (cond) { ... }          // conditional
if (cond) { ... } else { ... }
while (cond) { ... }       // loop
print(expr);               // output
{ ... }                    // block (scoped)
// comment text            // single-line comment
```

### Built-in Functions
| Call        | Description                        |
|-------------|------------------------------------|
| `print(e)`  | Print value followed by newline    |
| `input()`   | Read an integer from stdin         |

### Example Programs

**hello.cvm**
```cvm
let x = 6 * 7;
print(x);        // 42
print(true);
```

**fibonacci.cvm**
```cvm
let a = 0; let b = 1; let count = 0;
while (count < 15) {
    print(a);
    let tmp = a + b;
    a = b; b = tmp;
    count = count + 1;
}
```

**factorial.cvm**
```cvm
let n = 10; let result = 1; let i = 1;
while (i < n + 1) { result = result * i; i = i + 1; }
print(result);   // 3628800
```

---

## ISA — Instruction Set

| Opcode          | Operand       | Stack effect       | Description                       |
|-----------------|---------------|--------------------|-----------------------------------|
| `PUSH_INT`      | `int64`       | `-- val`           | Push integer literal              |
| `PUSH_BOOL`     | `0` or `1`    | `-- val`           | Push boolean literal              |
| `LOAD`          | slot index    | `-- val`           | Load variable from slot           |
| `STORE`         | slot index    | `val --`           | Pop and store to slot             |
| `ADD`           | —             | `a b -- (a+b)`     | Integer addition                  |
| `SUB`           | —             | `a b -- (a-b)`     | Integer subtraction               |
| `MUL`           | —             | `a b -- (a*b)`     | Integer multiplication            |
| `DIV`           | —             | `a b -- (a/b)`     | Integer division (error on 0)     |
| `EQ`            | —             | `a b -- bool`      | Equality comparison               |
| `LT`            | —             | `a b -- bool`      | Less-than comparison              |
| `JMP`           | target        | —                  | Unconditional jump                |
| `JMP_IF_FALSE`  | target        | `cond --`          | Jump if top-of-stack is falsy     |
| `PRINT`         | —             | `val --`           | Print value + newline             |
| `INPUT`         | —             | `-- val`           | Read integer from stdin           |
| `POP`           | —             | `val --`           | Discard top of stack              |
| `HALT`          | —             | —                  | Stop execution                    |

---

## Architecture

```
Source code (.cvm)
       │
       ▼
  ┌─────────┐
  │  Lexer  │  (lexer.hpp)
  │         │  Converts raw text → vector<Token>
  └────┬────┘  Handles whitespace, comments, keywords
       │
       ▼
  ┌─────────┐
  │  Parser │  (parser.hpp + ast.hpp)
  │         │  Recursive-descent, produces AST
  └────┬────┘  Operator precedence via call hierarchy
       │
       ▼
  ┌──────────┐
  │ Compiler │  (compiler.hpp)
  │          │  AST → vector<Instruction>
  └────┬─────┘  Variable slot map, backpatching jumps
       │
       ▼
  ┌────┐
  │ VM │  (vm.hpp)
  │    │  Stack-based execution engine
  └────┘  4096-slot variable array, typed Value union
```

### Module Details

| File           | Role |
|----------------|------|
| `lexer.hpp`    | Tokenizer: character stream → token stream. Single-pass, throws on unknown chars. |
| `ast.hpp`      | AST node definition: `ASTNode` struct + `NodeType` enum + `makeNode()` factory. |
| `parser.hpp`   | Recursive-descent parser building an AST. Precedence encoded in call depth. |
| `compiler.hpp` | Tree-walk code generator. Uses backpatching for control-flow jumps. |
| `vm.hpp`       | Bytecode interpreter. `Value` carries a type tag (INT/BOOL) and the raw data. |
| `main.cpp`     | CLI entry point: file mode + REPL with auto-exec heuristic. |

---

## REPL Tips

- Declare variables and then use them across multiple lines — state persists within a single buffer execution, but each auto-exec resets the VM state (variables start fresh).
- Use `;;` to manually flush a multi-line block that doesn't end cleanly with `;` or `}`.
- Toggle `debug` to inspect the generated bytecode for any expression.

---

## Suggested Extensions

| Feature      | How to add |
|--------------|------------|
| `>=`, `!=`   | Extend lexer tokens + parser comparison + compiler ops |
| `%` modulo   | Add `MOD` opcode, wire through parser and compiler |
| String type  | Add `STR_LIT` token, `Value::Tag::STR`, `PUSH_STR` opcode |
| Functions    | Add `CALL`/`RET` opcodes, stack frames, scope chain |
| Arrays       | `PUSH_ARRAY`, `INDEX_GET`, `INDEX_SET` opcodes |
| `for` loop   | Desugar to `while` in the parser |
| `break`/`continue` | Emit placeholder JMPs, backpatch at while end |
| File `import` | Pre-process source, prepend imported file tokens |
