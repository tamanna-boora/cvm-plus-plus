# CVM++

A small programming language I built in C++17. It has a lexer, a 
parser, a compiler, and a virtual machine. No external libraries. 
You write code in .cvm files and run them with ./cvm.

---

## How it works

The code goes through four steps:

**Lexer** — reads the source text and breaks it into tokens. A token 
is a single meaningful piece like a number, a keyword, or an operator.

**Parser** — takes the tokens and builds a tree that represents the 
structure of the program. I used recursive descent, where each grammar 
rule is its own function. Operator precedence is handled by the order 
in which these functions call each other.

**Compiler** — walks the tree and produces a list of bytecode 
instructions. The tricky part was compiling loops. When you emit a 
jump instruction, you do not know the target address yet because the 
loop body has not been compiled. The fix is to write a placeholder, 
compile the body, then go back and fill in the real address. This is 
called backpatching.

**VM** — executes the bytecode on a stack. Each instruction pushes or 
pops values. Variables are stored in a flat array of 4096 slots, with 
names mapped to slot numbers at compile time.

---

## What the language supports

- integers, booleans, strings
- operators: + - * / %
- comparisons: == != < <= >=
- let, if, else, while, for
- print() and input()
- single line comments with //

---

## Build

```bash
g++ -std=c++17 -I src -o cvm src/main.cpp
```

Tested on Windows with MSYS2 ucrt64.

---

## Usage

```bash
./cvm script.cvm           
./cvm script.cvm --debug   
./cvm script.cvm --trace   
./cvm                      
```

--debug prints the compiled bytecode before running.
--trace prints each instruction as the VM executes it.
The REPL keeps variables alive between lines.

---

## Examples
```
fibonacci.cvm   first 15 Fibonacci numbers
factorial.cvm   10 factorial = 3628800
fizzbuzz.cvm    FizzBuzz using %
gcd.cvm         GCD of 48 and 18 = 6
primes.cvm      all primes up to 50
for_loop.cvm    squares 1 to 5 using for
for_sum.cvm     sum 1 to 100 = 5050
tamanna.cvm     sum of squares 1 to 10 = 385
```
---

## What I found difficult

**Backpatching** — loops need a jump instruction whose target is not 
known yet. I emit a zero, compile the body, then patch the address 
once I know it.

**REPL state** — the first version forgot all variables between inputs. 
I fixed this by storing all previously run source in a string and 
recompiling everything from scratch on each new input.

**Operator precedence** — each precedence level is its own parsing 
function that calls the next one. Lower priority operators sit at the 
top, higher priority ones go deeper.

---

## Possible additions

- functions with their own call stack
- arrays
- break and continue in loops
