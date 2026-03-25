/*
 * symbols.c — Muad'dib Symbol Table Implementation
 *
 * This file implements the symbol table as a singly-linked list.
 * See symbols.h for the reasoning behind every design choice.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbols.h"

/*
 * WHY a single global pointer and NOT passing the table around?
 * In a simple single-pass compiler like ours, there is exactly ONE
 * symbol table for the entire program. Making it global avoids passing
 * it as a parameter to every function that touches variables.
 *
 * In a more complex compiler with scopes (functions, blocks), we'd
 * use a STACK of symbol tables — one per scope. But that's beyond
 * our current scope (pun intended).
 */
static Symbol* table_head = NULL;

/* ── symbol_declare ────────────────────────────────────────────────────── */

void symbol_declare(const char* name, DataType type, Value value) {
    /*
     * WHY check for duplicates BEFORE inserting?
     * Muad'dib requires 'is' for first declaration. If someone writes:
    /*
     * symbols.c — Muad'dib Symbol Table Implementation (v2: Scope Stack)
     *
     * Each scope = one hash table.
     * Scopes live in a fixed-size stack array.
     * Top of stack = current scope = where new variables go.
     */

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "symbols.h"

    /* ── THE STACK ─────────────────────────────────────────────────────────── */

    /*
     * WHY a static array and NOT malloc'd stack nodes?
     * A static array of MAX_SCOPES is simpler, faster, and for 16 levels
     * costs almost nothing in memory. Dynamic allocation would add
     * complexity (malloc, free, null checks) for zero practical benefit
     * at this scale. We know the upper bound. We use it.
     */
    static ScopeTable stack[MAX_SCOPES];
    static int        top = -1;   /* -1 = empty, 0 = global scope active */

    /* ── HASH FUNCTION ─────────────────────────────────────────────────────── */

    /*
     * WHY djb2 and NOT a simpler hash like sum-of-chars?
     * Sum-of-chars: "abc" and "bca" and "cab" all hash to the same bucket.
     * That causes unnecessary collisions on names like userName/nameUser.
     * djb2 uses multiplication (hash * 33) which spreads similar strings
     * across different buckets. It's simple, fast, and well-tested.
     * Named after Daniel J. Bernstein — it's been standard since the 90s.
     *
     * WHY & (HASH_SIZE - 1) and NOT % HASH_SIZE?
     * HASH_SIZE is 64 = 2^6. So HASH_SIZE-1 = 0b00111111.
     * Bitwise AND with that mask is equivalent to % 64 but faster —
     * no division instruction needed. Only works when size is power of 2.
     * That's exactly why we chose 64.
     */
    static unsigned int hash(const char* key) {
        unsigned long h = 5381;
        int c;
        while ((c = *key++)) {
            h = ((h << 5) + h) + c;  /* h * 33 + c */
        }
        return (unsigned int)(h & (HASH_SIZE - 1));
    }

    /* ── SCOPE OPERATIONS ──────────────────────────────────────────────────── */

    void scope_init(void) {
        top = 0;
        memset(stack[0].buckets, 0, sizeof(stack[0].buckets));
        stack[0].level = 0;
        stack[0].name  = strdup("global");
        printf("[Muad'dib] Global scope opened.\n");
    }

    void push_scope(const char* vision_name) {
        /*
         * WHY check MAX_SCOPES here?
         * Without this check, writing past the array is undefined behavior.
         * The program would corrupt memory silently. Loud error > silent crash.
         */
        if (top >= MAX_SCOPES - 1) {
            fprintf(stderr,
                "[Muad'dib] Error: maximum scope depth (%d) exceeded. "
                "No vision goes this deep.\n", MAX_SCOPES);
            return;
        }
        top++;
        memset(stack[top].buckets, 0, sizeof(stack[top].buckets));
        stack[top].level = top;
        stack[top].name  = strdup(vision_name);

        printf("[Muad'dib] Vision '%s' opened at scope level %d.\n",
               vision_name, top);
    }

    void pop_scope(void) {
        if (top <= 0) {
            fprintf(stderr,
                "[Muad'dib] Error: cannot close global scope.\n");
            return;
        }

        /*
         * WHY free every symbol in every bucket?
         * When a scope closes, its variables cease to exist.
         * In a real compiler this is where stack frame memory gets reclaimed.
         * We simulate that by freeing the heap memory.
         * This is the MEMORY ANALYSIS the assignment asks about:
         * variables don't "move" when a scope closes — they are destroyed.
         */
        printf("[Muad'dib] Vision '%s' (level %d) closed. "
               "Variables in this scope are destroyed:\n",
               stack[top].name, top);

        for (int i = 0; i < HASH_SIZE; i++) {
            Symbol* sym = stack[top].buckets[i];
            while (sym != NULL) {
                Symbol* next = sym->next;
                printf("           ✗ '%s' (scope level %d) — gone.\n",
                       sym->name, sym->scope_level);
                free(sym->name);
                if (sym->type == DT_WORD && sym->value.sval != NULL) {
                    free(sym->value.sval);
                }
                free(sym);
                sym = next;
            }
        }

        free(stack[top].name);
        top--;

        printf("[Muad'dib] Back to scope level %d ('%s').\n",
               top, stack[top].name);
    }

    int current_scope_level(void) {
        return top;
    }

    /* ── SYMBOL OPERATIONS ─────────────────────────────────────────────────── */

    void symbol_declare(const char* name, DataType type, Value value) {
        /*
         * COLLISION CHECK — only within the CURRENT scope.
         * Declaring the same name in an outer scope is shadowing (allowed).
         * Declaring the same name TWICE in the same scope is an error.
         *
         * WHY only check current scope and not all scopes?
         * Because shadowing is intentional and valid. The assignment
         * specifically asks us to support it. We check current scope
         * for duplicates, and warn (not error) when shadowing occurs.
         */
        unsigned int bucket = hash(name);
        Symbol* sym = stack[top].buckets[bucket];
        while (sym != NULL) {
            if (strcmp(sym->name, name) == 0) {
                fprintf(stderr,
                    "[Muad'dib] Error: '%s' already declared in this scope "
                    "(level %d). Use 'becomes' to change its value.\n",
                    name, top);
                return;
            }
            sym = sym->next;
        }

        /*
         * SHADOW WARNING — check outer scopes.
         * If a variable with the same name exists in any outer scope,
         * we warn. This is the key feature: shadowing is not silent.
         * In C, shadowing is invisible. In Muad'dib, it announces itself.
         */
        for (int level = top - 1; level >= 0; level--) {
            Symbol* outer = stack[level].buckets[bucket];
            while (outer != NULL) {
                if (strcmp(outer->name, name) == 0) {
                    fprintf(stderr,
                        "[Muad'dib] Shadow detected: '%s' in vision '%s' (level %d)\n"
                        "           hides outer declaration at scope level %d ('%s').\n",
                        name, stack[top].name, top, level, stack[level].name);
                    break;
                }
                outer = outer->next;
            }
        }

        /* Insert at HEAD of bucket chain */
        Symbol* new_sym = (Symbol*)malloc(sizeof(Symbol));
        if (new_sym == NULL) {
            fprintf(stderr, "[Muad'dib] Fatal: out of memory.\n");
            exit(1);
        }
        new_sym->name        = strdup(name);
        new_sym->type        = type;
        new_sym->value       = value;
        new_sym->scope_level = top;
        new_sym->next        = stack[top].buckets[bucket];
        stack[top].buckets[bucket] = new_sym;
    }

    void symbol_assign(const char* name, Value value) {
        /*
         * WHY search ALL scopes for assignment?
         * becomes modifies an EXISTING variable wherever it lives.
         * If userName was declared at global scope and we're inside a vision,
         * becomes userName = "Paul" should update the global one.
         * We search top-down: if there's a local shadow, we update that.
         * If not, we go deeper until we find it.
         */
        unsigned int bucket = hash(name);
        for (int level = top; level >= 0; level--) {
            Symbol* sym = stack[level].buckets[bucket];
            while (sym != NULL) {
                if (strcmp(sym->name, name) == 0) {
                    /* Free old value if DT_WORD */
                    if (sym->type == DT_WORD && sym->value.sval != NULL && value.sval != sym->value.sval) {
                        free(sym->value.sval);
                    }
                    sym->value = value;
                    return;
                }
                sym = sym->next;
            }
        }
        fprintf(stderr,
<<<<<<< HEAD
            "[Muad'dib] Error: '%s' was never declared. Use 'is' first.\n",
            name);
=======
            "[Muad'dib] Error: Variable '%s' was already declared. "
            "Use 'becomes' to change its value.\n", name);
        if (type == DT_WORD && value.sval != NULL) {
            free(value.sval);
        }
        return;
>>>>>>> 2ae2cac6cde61bc3cf55c25bfe30c65febdca8e1
    }

    Symbol* symbol_lookup(const char* name) {
        /*
         * WHY top-down search?
         * This is the core of scope resolution.
         * The innermost (most recent) declaration always wins.
         * We start at the current scope and work outward.
         * First match = correct answer. No further search needed.
         *
         * This is how every language with lexical scoping works:
         * JavaScript, Python, C, Java. We implement the same rule.
         */
        unsigned int bucket = hash(name);
        for (int level = top; level >= 0; level--) {
            Symbol* sym = stack[level].buckets[bucket];
            while (sym != NULL) {
                if (strcmp(sym->name, name) == 0) {
                    return sym;
                }
                sym = sym->next;
            }
        }
        return NULL;
    }

    /* ── DEBUG DUMP ────────────────────────────────────────────────────────── */

    void symbol_dump(void) {
        printf("\n╔══ Muad'dib Scope Stack ══════════════════════╗\n");
        for (int level = top; level >= 0; level--) {
            printf("║  Level %d — vision: '%s'%s\n",
                   level, stack[level].name,
                   level == top ? " ← current" : "");
            int found = 0;
            for (int i = 0; i < HASH_SIZE; i++) {
                Symbol* sym = stack[level].buckets[i];
                while (sym != NULL) {
                    printf("║    %s (type %d, scope %d)\n", sym->name, sym->type, sym->scope_level);
                    found = 1;
                    sym = sym->next;
                }
            }
            if (!found) printf("║    (empty scope)\n");
            if (level > 0) printf("║  ───────────────────────────────────────────\n");
        }
        printf("╚═════════════════════════════════════════════╝\n\n");
    }

    /* ── FREE ALL ──────────────────────────────────────────────────────────── */

    void symbol_free(void) {
        while (top >= 0) {
            for (int i = 0; i < HASH_SIZE; i++) {
                Symbol* sym = stack[top].buckets[i];
                while (sym != NULL) {
                    Symbol* next = sym->next;
                    free(sym->name);
                    if (sym->type == DT_WORD && sym->value.sval != NULL) {
                        free(sym->value.sval);
                    }
                    free(sym);
                    sym = next;
                }
            }
            free(stack[top].name);
            top--;
        }
    }
