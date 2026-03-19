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
     *   userName is word = "Paul"
     *   userName is word = "Leto"   <- should be an error
     * We catch it here and report it with a clear message.
     */
    if (symbol_lookup(name) != NULL) {
        fprintf(stderr,
            "[Muad'dib] Error: Variable '%s' was already declared. "
            "Use 'becomes' to change its value.\n", name);
        if (type == DT_WORD && value.sval != NULL) {
            free(value.sval);
        }
        return;
    }

    /*
     * WHY malloc for the symbol itself?
     * We're building a heap-allocated linked list. Each node lives
     * until we explicitly free it. Stack allocation would be wrong
     * here — the node needs to outlive the function call.
     */
    Symbol* sym = (Symbol*)malloc(sizeof(Symbol));
    if (sym == NULL) {
        fprintf(stderr, "[Muad'dib] Fatal: out of memory.\n");
        exit(1);
    }

    /*
     * WHY strdup the name?
     * The 'name' pointer comes from yytext (via the parser's yylval.sval).
     * That memory is managed by the caller — it might be freed or
     * reused. We need our OWN copy that we control.
     */
    sym->name  = strdup(name);
    sym->type  = type;
    sym->value = value;

    /*
     * WHY insert at the HEAD and not the TAIL?
     * Inserting at the head is O(1) — just update one pointer.
     * Inserting at the tail is O(n) — we'd have to walk the whole list.
     * For lookup, we walk the list either way. Head insertion is faster
     * and we don't need alphabetical order or insertion order.
     */
    sym->next  = table_head;
    table_head = sym;
}

/* ── symbol_assign ─────────────────────────────────────────────────────── */

void symbol_assign(const char* name, Value value) {
    Symbol* sym = symbol_lookup(name);

    /*
     * WHY error on assign-before-declare?
     * Muad'dib's rule: you MUST declare with 'is' before you can
     * change with 'becomes'. This catches a common beginner mistake
     * (using a variable that was never created) at compile time,
     * not at some mysterious runtime moment.
     */
    if (sym == NULL) {
        fprintf(stderr,
            "[Muad'dib] Error: Variable '%s' was never declared. "
            "Use 'is' to declare it first.\n", name);
        return;
    }

    /*
     * WHY free the old sval before assigning new one?
     * If the variable is a DT_WORD (string), its value.sval is a
     * heap-allocated string we strdup'd during declaration.
     * Assigning a new string without freeing the old one is a
     * memory leak. We clean up before we replace.
     */
    if (sym->type == DT_WORD && sym->value.sval != NULL) {
        free(sym->value.sval);
    }

    sym->value = value;
}

/* ── symbol_lookup ─────────────────────────────────────────────────────── */

Symbol* symbol_lookup(const char* name) {
    /*
     * WHY linear search?
     * See symbols.h for the full reasoning. Short version: our programs
     * are small. O(n) through 10-20 variables is negligible.
     * A hash table would be O(1) but adds complexity we don't need yet.
     */
    Symbol* current = table_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;  /* not found */
}

/* ── symbol_dump ───────────────────────────────────────────────────────── */

void symbol_dump(void) {
    /*
     * WHY have a dump function?
     * For debugging. When something goes wrong with variable storage,
     * calling symbol_dump() shows us exactly what's in the table.
     * It costs nothing if we don't call it in production.
     */
    printf("\n=== Muad'dib Symbol Table ===\n");
    Symbol* current = table_head;
    if (current == NULL) {
        printf("  (empty)\n");
    }
    while (current != NULL) {
        printf("  [%s] type=", current->name);
        switch (current->type) {
            case DT_WHOLE:   printf("whole   value=%d\n",   current->value.ival); break;
            case DT_DECIMAL: printf("decimal value=%.2f\n", current->value.dval); break;
            case DT_LETTER:  printf("letter  value='%c'\n", current->value.cval); break;
            case DT_WORD:    printf("word    value=\"%s\"\n",current->value.sval); break;
            case DT_TRUTH:   printf("truth   value=%s\n",
                                current->value.ival ? "yes" : "no");             break;
        }
        current = current->next;
    }
    printf("=============================\n\n");
}

/* ── symbol_free ───────────────────────────────────────────────────────── */

void symbol_free(void) {
    /*
     * WHY free in a separate function and not at exit?
     * Technically the OS reclaims all memory when the process ends.
     * BUT: calling symbol_free() explicitly means tools like Valgrind
     * won't report false memory leak warnings. It also shows that we
     * THOUGHT about memory management — which matters in a compiler class.
     */
    Symbol* current = table_head;
    while (current != NULL) {
        Symbol* next = current->next;
        free(current->name);
        if (current->type == DT_WORD && current->value.sval != NULL) {
            free(current->value.sval);
        }
        free(current);
        current = next;
    }
    table_head = NULL;
}
