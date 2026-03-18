#ifndef SYMBOLS_H
#define SYMBOLS_H

/*
 * symbols.h — Muad'dib Symbol Table
 *
 * WHY A SYMBOL TABLE?
 * When the parser sees "userName becomes 'Paul'", it needs to:
 *   1. Find the variable "userName"
 *   2. Check its type (is it a word? can it hold a string?)
 *   3. Update its value
 *
 * A symbol table is the data structure that stores all declared
 * variables: their name, type, and current value.
 *
 * WHY A LINKED LIST and NOT an array?
 * We don't know ahead of time how many variables the program will
 * declare. A fixed-size array would either waste memory or overflow.
 * A linked list grows as needed. For a language at this scale, the
 * performance difference is irrelevant — we'll never have thousands
 * of variables in a single program of this scope.
 *
 * WHY NOT a hash table?
 * A hash table would give O(1) lookup instead of O(n). But:
 *   1. We're in a compiler class, not building production software
 *   2. The programs we run are small — O(n) is fine
 *   3. A linked list is simpler to implement and easier to explain
 *   4. Adding a hash table later is a well-defined upgrade path
 *
 * "Premature optimization is the root of all evil." — Knuth
 */

/* ── TYPE ENUM ─────────────────────────────────────────────────────────── */

/*
 * WHY an enum and NOT #define constants?
 * #define gives us: #define TYPE_WHOLE 0
 * enum gives us:    typedef enum { TYPE_WHOLE, ... } DataType;
 *
 * The enum is safer because:
 *   - The compiler knows the valid range of values
 *   - It shows up with its name in debuggers (gdb shows TYPE_WHOLE, not 0)
 *   - It groups related constants visually and semantically
 */
typedef enum {
    DT_WHOLE,    /* int equivalent   */
    DT_DECIMAL,  /* double equivalent */
    DT_LETTER,   /* char equivalent  */
    DT_WORD,     /* string equivalent */
    DT_TRUTH     /* bool equivalent  */
} DataType;

/* ── VALUE UNION ───────────────────────────────────────────────────────── */

/*
 * WHY a union and NOT separate fields?
 * A variable can only be ONE type at a time. If we used a struct with
 * all fields (int ival; double dval; char cval; char* sval; int bval),
 * we'd waste memory — most fields would always be unused.
 *
 * A union uses the SAME memory for all fields. The active field is
 * whichever one matches the DataType. We use less memory and the
 * intent is clear: exactly one of these is valid at any time.
 */
typedef union {
    int    ival;   /* used when type is DT_WHOLE or DT_TRUTH */
    double dval;   /* used when type is DT_DECIMAL */
    char   cval;   /* used when type is DT_LETTER */
    char*  sval;   /* used when type is DT_WORD */
} Value;

/* ── SYMBOL STRUCT ─────────────────────────────────────────────────────── */

/*
 * WHY is 'next' a pointer to Symbol and NOT to void*?
 * A linked list node needs to point to the NEXT node of the same type.
 * void* would work but requires a cast everywhere — that's error-prone.
 * struct Symbol* is type-safe: the compiler catches mistakes.
 *
 * NOTE: We use 'struct Symbol' instead of 'Symbol' here because the
 * typedef hasn't been completed yet at the point of declaration.
 * This is a C forward-reference pattern.
 */
typedef struct Symbol {
    char*         name;    /* the variable name (heap-allocated, owned) */
    DataType      type;    /* which type this variable holds */
    Value         value;   /* the current value */
    struct Symbol* next;   /* next symbol in the linked list */
} Symbol;

/* ── FUNCTION DECLARATIONS ─────────────────────────────────────────────── */

/*
 * WHY declare functions in the header and define in .c?
 * The header is the PUBLIC INTERFACE. Any file that #includes symbols.h
 * knows what functions are available and what they expect.
 * The .c file has the IMPLEMENTATION — the details that callers don't
 * need to know about.
 * This separation is what makes C codebases maintainable.
 */

/* declare a new variable — fails if name already exists */
void   symbol_declare(const char* name, DataType type, Value value);

/* update an existing variable — fails if name doesn't exist */
void   symbol_assign(const char* name, Value value);

/* look up a variable by name — returns NULL if not found */
Symbol* symbol_lookup(const char* name);

/* print all variables (for debugging) */
void   symbol_dump(void);

/* free all memory used by the symbol table */
void   symbol_free(void);

#endif /* SYMBOLS_H */
