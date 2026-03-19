# Muad'dib


A programming language designed for **humans first, machines second**.

Built with Flex (lexer) + Bison/Yacc (parser) + C as part of a Compilers & Interpreters course.

---

## The Problem Muad'dib Solves

Most programming languages were designed thinking about the machine first and the programmer second. The result: cryptic type names (`int`, `bool`, `char`), invisible bugs (`!x` vs `x`), and code that only its author understands.

Muad'dib attacks specific, real problems at the **syntax level**:

| Problem | Muad'dib's solution |
|---|---|
| `=` means both declare AND assign | `is` declares, `becomes` assigns — they look different because they ARE different |
| `int`, `bool` mean nothing in plain language | `whole`, `truth`, `decimal` describe what they hold |
| Division by zero crashes at runtime | `splitby ordefault` forces you to handle it when you write it |
| `+` is used for both math and string concat | `woven` is exclusively for joining text |
| `!x` is invisible, causes silent bugs | `opposite x` — you cannot miss a full word |
| Code has no structure or stated intent | `purpose` is a required statement before every logic block |
| Comments are unstructured noise | `#why:`, `#todo:`, `#warn:` — comments have categories |

---

## Quick Look

```
#why: "program to register a new user"

purpose "define user identity";
userName is word    = "Paul";
userAge  is whole   = 17;
isActive is truth   = no;

purpose "update identity after login";
userName becomes "Muad'dib";
isActive becomes yes;

purpose "show user info";
show literal: "Name:"   then word:  userName;
show literal: "Age:"    then value: userAge;
show literal: "Active:" then value: isActive;

purpose "combine name and title";
userTitle is word = "Atreides";
fullName  is word = userName woven userTitle;
show literal: "Full name:" then word: fullName;

purpose "safe division example";
level is decimal = 0.0;
level becomes userAge splitby 5 ordefault 0.0;
show literal: "Level:" then value: level;
```

---

## Language Reference

### Data Types

| Muad'dib | Equivalent | Example |
|---|---|---|
| `whole` | `int` | `17`, `-3` |
| `decimal` | `double` | `3.14`, `-0.5` |
| `letter` | `char` | `'P'` |
| `word` | `string` | `"Paul"` |
| `truth` | `bool` | `yes` / `no` |

### Declaration vs Assignment

```
// Declaration — creates the variable for the first time
name is TYPE = value;

// Assignment — changes an existing variable
name becomes newValue;
```

### Print (show)

```
show literal: "text here";
show value:   numericVariable;
show word:    wordVariable;

// Chaining with 'then'
show literal: "Name:" then word: userName;
```

### Comments

```
// general comment
#why:  "explains a design decision"
#todo: "marks something to do later"
#warn: "flags something dangerous"
```

### Operators

```
// Arithmetic
a + b       // addition
a - b       // subtraction
a * b       // multiplication
a / b       // division (use splitby for safety)

// Safe division
result becomes a splitby b ordefault fallback;

// String concatenation
full is word = first woven last;

// Logical negation
isOff is truth = opposite isOn;
```

### Variable Naming Rules

- Must start with a **lowercase letter**
- Uses **camelCase** — enforced by the lexer
- No underscores, no hyphens, no spaces
- Cannot be a reserved keyword

```
// Valid
userName, userAge, isActive, x, totalScore

// Invalid
UserName, user_name, 2fast, is, becomes
```

---

## Files

```
muaddib/
├── lexer.l         — Flex lexer: tokenizes Muad'dib source code
├── parser.y        — Bison parser: grammar rules and semantic actions
├── symbols.h       — Symbol table interface
├── symbols.c       — Symbol table implementation (linked list)
├── main.c          — Entry point, file/stdin handling
├── Makefile        — Build system with make, make test, make clean
└── test.mdb        — Test program exercising every language feature
```

---

## Build & Run

**Requirements:** `flex`, `bison`, `gcc`, `make`

```bash
# Build
make

# Run a .mdb file
./muaddib test.mdb

# Run all tests
make test

# Clean generated files
make clean
```

---

## Design Decisions (The Why)

Every decision in this language has a reason. Here are the most important ones:

**Why `is` and not `=` for declaration?**
Because `=` in most languages does two completely different things depending on context. `int x = 5` declares. `x = 10` assigns. They look almost identical. `is` and `becomes` are visually and semantically distinct — you cannot confuse them.

**Why `whole` and not `int`?**
`int` is short for *integer*, a math term. `whole` means "a whole number" — something any person understands without knowing math terminology. The type name should describe what it holds, not its historical origin.

**Why camelCase enforced in the lexer?**
So no one ever debates it. The regex `[a-z][a-zA-Z0-9]*` rejects `user_name` and `UserName` at the lexer level. The rule is not a style guide — it's a compile error.

**Why `splitby ordefault` and not just `/`?**
Division by zero is a runtime error in every other language. You discover it when the program crashes, not when you write it. `ordefault` forces you to decide the fallback *at the moment of writing*, not after the crash.

**Why `woven` and not `+`?**
`"hello" + "world"` and `3 + 4` look the same but mean different things. In JavaScript, `"3" + 3 = "33"`. The `+` operator is overloaded with confusing behavior. `woven` is exclusively for joining text. There is no ambiguity.

**Why `#why:`, `#todo:`, `#warn:` and not just `//`?**
Because comments are for future humans, not for the compiler. Knowing that a comment is a *reason*, a *task*, or a *warning* is completely different information. Categorizing them makes any codebase more readable, even at this scale.

---

## Course Context

This project was built for a Compilers & Interpreters course. The implementation is limited to what was covered in class:

- Flex for lexical analysis
- Bison/Yacc for parsing
- C for symbol table and helper functions
- No control flow (if, while) in this version

---

*"A beginning is a very delicate time."* — Dune

