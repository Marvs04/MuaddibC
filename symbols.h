#ifndef SYMBOLS_H
#define SYMBOLS_H

/*
 * symbols.h — Muad'dib Symbol Table (v2: Scope Stack)
 *
 * WHY REWRITE THE SYMBOL TABLE?
 * v1 was a single linked list — one flat scope.
 * v2 is a STACK OF HASH TABLES — one table per scope level.
 * The metaphor fits Muad'dib perfectly:
 * Paul sees multiple layers of time simultaneously.
 * Each layer (scope) has its own variables.
 * The closest layer always wins — that is shadowing.
 *
 * STRUCTURE:
 *   scope_stack[0]  ← global scope  (bottom, always exists)
 *   scope_stack[1]  ← vision level 1
 *   scope_stack[2]  ← vision level 2  ← lookup starts HERE (top)
 * LOOKUP: starts at top, walks DOWN until found or not found.
 * DECLARE: always inserts into the TOP scope only.
 * POP: destroys the top scope and everything in it.
 * WHY A HASH TABLE per scope and NOT a linked list per scope?
 * Linked list lookup = O(n). Hash table lookup = O(1) average.
 * With scopes we call lookup MANY times (every variable reference).
 * The hash table investment pays off immediately.
 * Also — a pro would expect a hash table here. We deliver.
 *
 * WHY NOT one big hash table with scope prefixes like "scope2_userName"?
 * Because pop_scope() would require scanning the ENTIRE table to find
 * and delete all variables from that scope. With a stack of tables,
 * pop_scope() is just: free the top table. O(1) structural operation.
 */

/* ── CONSTANTS ─────────────────────────────────────────────────────────── */

/*
 * WHY 64 buckets and NOT 256 or 16?
 * 64 is a sweet spot for our use case:
 *   - Programs in this language have ~10-30 variables per scope
 *   - 64 buckets means low collision probability for that range
 *   - 256 would waste memory, 16 would cause too many collisions
 *   - Must be a power of 2 so we can use bitmasking: hash & (SIZE-1)
 *     instead of the slower modulo: hash % SIZE
 */
#define HASH_SIZE    64

/*
 * WHY 16 max scopes?
 * 16 levels of nesting is already deeply unreasonable code.
 * If someone nests 17 visions deep, they have bigger problems.
 * A fixed-size stack avoids dynamic allocation complexity
 * while being more than enough for any real program.
#define MAX_SCOPES   16

/* ── DATA TYPES ────────────────────────────────────────────────────────── */

typedef enum {
    DT_WHOLE,
    DT_DECIMAL,
    DT_LETTER,
    DT_WORD,
    DT_TRUTH
} DataType;

typedef union {
    int    ival;
    double dval;
    char   cval;
    char*  sval;
} Value;

/*
 * WHY add 'scope_level' to Symbol?
 * So error messages and the shadow warning can say:
 * "hides declaration at scope level 1"
 * Without it, we'd have no way to report WHERE the outer variable lives.
 */
typedef struct Symbol {
    char*         name;
    DataType      type;
    Value         value;
    int           scope_level;  /* which scope level declared this */
    struct Symbol* next;         /* next in hash bucket chain */
/*
 * WHY a struct for ScopeTable instead of Symbol**?
 * A struct lets us add metadata to each scope level.
 * Right now we store 'level' and 'name' (the vision name).
 * In the future we could add variable count, memory offset, etc.
 * A raw Symbol** would not allow that extension cleanly.
typedef struct {
    Symbol* buckets[HASH_SIZE];  /* the hash table */
    int     level;               /* scope depth (0 = global) */
    char*   name;                /* vision name, or "global" */
} ScopeTable;

/* ── SCOPE OPERATIONS ──────────────────────────────────────────────────── */

/* called at program start — creates global scope */
void scope_init(void);

/* called when entering a vision block */
void push_scope(const char* vision_name);

/* called when leaving a vision block */
/* current scope depth (0 = global) */
int  current_scope_level(void);

/* ── SYMBOL OPERATIONS ─────────────────────────────────────────────────── */

/* declare in CURRENT scope only — errors if already in current scope */
void    symbol_declare(const char* name, DataType type, Value value);

/* assign — finds variable in any scope, updates it */
void    symbol_assign(const char* name, Value value);

/* lookup — searches from top scope down, returns first match */
Symbol* symbol_lookup(const char* name);

/* dump all scopes for debugging */
void    symbol_dump(void);

/* free everything */
void    symbol_free(void);

#endif /* SYMBOLS_H */
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
