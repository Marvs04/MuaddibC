# Muad'dib

A programming language designed for **humans first, machines second.**  
Built with **Flex + Bison + C** for a Compilers & Interpreters course.

---

## Quick Start (Ubuntu / Debian / Linux)

### 1. Install dependencies

```bash
sudo apt update
sudo apt install flex bison gcc make -y
```

### 2. Clone and build

```bash
git clone https://github.com/Marvs04/MuaddibC.git
cd MuaddibC
make
```

You should see:

```
Build complete. Run with: ./muaddib test.mdb
```

### 3. Run the full demo

```bash
./muaddib demo_completo.mdb
```

This runs the complete demonstration covering all compiler phases.  
Errors printed to `stderr` in the demo are **intentional** — they show error detection working.

To see stdout and stderr separately:

```bash
./muaddib demo_completo.mdb 2>errors.log   # only clean output
./muaddib demo_completo.mdb 2>&1            # everything mixed (default)
```

### 4. Other test files

```bash
./muaddib test_scopes.mdb    # nested scopes and shadowing
./muaddib test_bug.mdb       # intentional type error
./muaddib dictionary.mdb     # full language reference with comments
```

---

## The Problem Muad'dib Solves

Most languages were designed for the machine first. The result: cryptic type names, invisible bugs, and code only its author understands.

Muad'dib attacks specific problems at the **syntax level**:

| Problem | Solution |
|---|---|
| `=` means both declare AND assign | `is` declares. `becomes` assigns. They look different because they ARE different. |
| `int`, `bool` mean nothing in plain language | `whole`, `truth`, `decimal` describe what they hold. |
| Division by zero crashes at runtime | `splitby ordefault` forces you to handle it when you write it. |
| `+` used for both math and string concat | `woven` is exclusively for joining text. |
| `!x` is invisible, causes silent bugs | `opposite x` — you cannot miss a full word. |
| No stated intent before logic blocks | `purpose` is required before every section. |
| Comments are unstructured noise | `#why:`, `#todo:`, `#warn:` — comments have categories. |
| Shadowing is completely silent | `vision` blocks emit explicit named warnings. |

---

## Language Reference

### Data Types

| Muad'dib | C equivalent | Example |
|---|---|---|
| `whole` | `int` | `17`, `-3` |
| `decimal` | `double` | `3.14`, `-0.5` |
| `letter` | `char` | `'P'` |
| `word` | `string` | `"Paul"` |
| `truth` | `bool` | `yes` / `no` |

### Syntax

```
// Declaration — creates the variable for the first time
name is TYPE = value;

// Assignment — changes an existing variable
name becomes newValue;

// Arithmetic (precedence: * / before + -)
result becomes a + b * c;

// Safe division — fallback required at write time
result becomes a splitby b ordefault fallback;

// String concatenation
full is word = first woven last;

// Logical negation
isOff is truth = opposite isOn;

// Output
show literal: "text here";
show value:   someNumber;
show word:    someString;
show literal: "label:" then value: someNumber;

// Scope block
vision "block-name"
    // variables here are local to this block
    local is whole = 42;
end;

// Intent statement (required before logic blocks)
purpose "describe what follows";

// Structured comments (all ignored by compiler)
#why:  "design decision"
#todo: "pending task"
#warn: "potential issue"
//     general comment
```

### Variable Naming Rules

- Must start with a **lowercase letter**
- Uses **camelCase** — enforced by the lexer
- Cannot be a reserved keyword

```
// Valid:   userName, userAge, x, totalScore
// Invalid: UserName, user_name, 2fast, is, becomes
```

---

## Project Structure

```
MuaddibC/
├── lexer.l            — Flex lexer: tokenizes source code
├── parser.y           — Bison parser: grammar rules + semantic actions
├── symbols.h          — Symbol table interface
├── symbols.c          — Symbol table: stack of hash tables (djb2)
├── main.c             — Entry point, file/stdin handling
├── Makefile           — Build system
├── demo_completo.mdb  — Full demo covering all compiler phases
├── test_scopes.mdb    — Nested scopes and shadowing
├── test_bug.mdb       — Error detection demo
└── dictionary.mdb     — Complete language reference
```

---

## Build Targets

```bash
make          # build muaddib executable
make test     # build + run test_scopes.mdb
make debug    # build with debug symbols (for gdb)
make clean    # remove all generated files
```

---

## Compiler Architecture

```
Source (.mdb)
    │
    ▼
[Flex — lexer.l]          tokenizes: keywords, literals, identifiers
    │  tokens
    ▼
[Bison — parser.y]        LALR(1) grammar, semantic actions inline
    │  reductions
    ▼
[Symbol Table — symbols.c] stack of hash tables, one per scope level
    │
    ▼
Output (stdout) + Errors (stderr)
```

The compiler uses a **syntax-directed interpreter** model: Bison semantic actions evaluate expressions and execute statements directly during parsing. No separate intermediate representation is generated.

---

## Scope Management

Muad'dib uses **lexical scoping** via `vision` blocks:

```
vision "block-name"
    x is whole = 10;    // local to this block
    // outer variables are visible here
    // declaring a name that exists outside triggers a shadow warning
end;
// x is destroyed here
```

Internally: a **stack of hash tables** (one table per scope, djb2 hash, 64 buckets).  
- `DECLARE` → inserts into top scope only  
- `ASSIGN` → searches top-down, updates closest match  
- `POP` → frees all variables in that scope

---

## Design Decisions

**Why `is` and not `=` for declaration?**  
`=` in most languages does two completely different things. `is` and `becomes` are visually and semantically distinct — you cannot confuse them.

**Why `whole` and not `int`?**  
`int` is short for *integer*, a math term. `whole` means "a whole number" — something anyone understands without CS background.

**Why camelCase enforced in the lexer?**  
The regex `[a-z][a-zA-Z0-9]*` rejects `user_name` and `UserName` at the lexer level. It is not a style guide — it is a compile error.

**Why `splitby ordefault` and not just `/`?**  
Division by zero is a runtime error in every other language. You discover it when the program crashes. `ordefault` forces you to decide the fallback *at the moment of writing*, not after the crash.

**Why `woven` and not `+`?**  
`"hello" + "world"` and `3 + 4` look the same but mean different things. `woven` is exclusively for joining text. No ambiguity.

**Why a stack of hash tables for scopes?**  
A single flat table would require scanning everything to pop a scope. With a stack, `pop_scope()` frees one table. djb2 gives O(1) average lookup. Each scope has 64 buckets — enough for any realistic program at this scale.
