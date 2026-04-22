/*
 * symbols.c — Implementacion de la Tabla de Simbolos de Muad'dib
 *
 * Implementa una pila de tablas hash para scoping lexico.
 * Cada nivel de scope tiene su propia tabla hash (64 buckets, hash djb2).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbols.h"

/* ── LA PILA ─────────────────────────────────────────────────────────── */

/*
 * Array estatico de MAX_SCOPES: mas simple, rapido, y para 16 niveles
 * el costo de memoria es insignificante. top == -1 significa pila vacia.
 */
static ScopeTable stack[MAX_SCOPES];
static int        top = -1;

/* ── FUNCION HASH ────────────────────────────────────────────────────── */

/*
 * djb2: h = h * 33 + c
 * Distribuye nombres similares en buckets diferentes.
 * & (HASH_SIZE-1) reemplaza % HASH_SIZE — mas rapido cuando size es potencia de 2.
 */
static unsigned int hash(const char* key) {
    unsigned long h = 5381;
    int c;
    while ((c = *key++)) {
        h = ((h << 5) + h) + c;
    }
    return (unsigned int)(h & (HASH_SIZE - 1));
}

/* ── OPERACIONES DE SCOPE ────────────────────────────────────────────── */

void scope_init(void) {
    top = 0;
    memset(stack[0].buckets, 0, sizeof(stack[0].buckets));
    stack[0].level = 0;
    stack[0].name  = strdup("global");
    printf("[Muad'dib] Scope global abierto.\n");
}

void push_scope(const char* vision_name) {
    if (top >= MAX_SCOPES - 1) {
        fprintf(stderr,
            "[Muad'dib] Error: profundidad maxima de scope (%d) excedida.\n",
            MAX_SCOPES);
        return;
    }
    top++;
    memset(stack[top].buckets, 0, sizeof(stack[top].buckets));
    stack[top].level = top;
    stack[top].name  = strdup(vision_name);
    printf("[Muad'dib] Vision '%s' abierta en nivel de scope %d.\n",
           vision_name, top);
}

void pop_scope(void) {
    if (top <= 0) {
        fprintf(stderr, "[Muad'dib] Error: no se puede cerrar el scope global.\n");
        return;
    }

    printf("[Muad'dib] Vision '%s' (nivel %d) cerrada. Variables destruidas:\n",
           stack[top].name, top);

    for (int i = 0; i < HASH_SIZE; i++) {
        Symbol* sym = stack[top].buckets[i];
        while (sym != NULL) {
            Symbol* next = sym->next;
            printf("           x '%s' (nivel %d) — eliminada.\n",
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
    printf("[Muad'dib] Regresando a nivel %d ('%s').\n",
           top, stack[top].name);
}

int current_scope_level(void) {
    return top;
}

/* ── OPERACIONES DE SIMBOLOS ─────────────────────────────────────────── */

void symbol_declare(const char* name, DataType type, Value value) {
    unsigned int bucket = hash(name);

    /* Verificar duplicado solo en el scope ACTUAL */
    Symbol* sym = stack[top].buckets[bucket];
    while (sym != NULL) {
        if (strcmp(sym->name, name) == 0) {
            fprintf(stderr,
                "[Muad'dib] Error: '%s' ya fue declarado en este scope "
                "(nivel %d). Usa 'becomes' para cambiar su valor.\n",
                name, top);
            if (type == DT_WORD && value.sval != NULL) {
                free(value.sval);
            }
            return;
        }
        sym = sym->next;
    }

    /* Advertencia de shadowing — verificar scopes externos */
    for (int level = top - 1; level >= 0; level--) {
        Symbol* outer = stack[level].buckets[bucket];
        while (outer != NULL) {
            if (strcmp(outer->name, name) == 0) {
                fprintf(stderr,
                    "[Muad'dib] Shadow detectado: '%s' en vision '%s' (nivel %d)\n"
                    "           oculta la declaracion en scope nivel %d ('%s').\n",
                    name, stack[top].name, top, level, stack[level].name);
                break;
            }
            outer = outer->next;
        }
    }

    /* Insertar al inicio de la cadena del bucket */
    Symbol* new_sym = (Symbol*)malloc(sizeof(Symbol));
    if (new_sym == NULL) {
        fprintf(stderr, "[Muad'dib] Fatal: sin memoria.\n");
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
    unsigned int bucket = hash(name);
    /*
     * becomes modifica una variable EXISTENTE donde sea que viva.
     * Busqueda top-down: si hay shadow local, se actualiza ese.
     * Si no, se busca hacia abajo hasta encontrarla.
     */
    for (int level = top; level >= 0; level--) {
        Symbol* sym = stack[level].buckets[bucket];
        while (sym != NULL) {
            if (strcmp(sym->name, name) == 0) {
                if (sym->type == DT_WORD && sym->value.sval != NULL
                    && value.sval != sym->value.sval) {
                    free(sym->value.sval);
                }
                sym->value = value;
                return;
            }
            sym = sym->next;
        }
    }
    fprintf(stderr,
        "[Muad'dib] Error: '%s' nunca fue declarado. Usa 'is' primero.\n",
        name);
}

Symbol* symbol_lookup(const char* name) {
    unsigned int bucket = hash(name);
    /* Busqueda top-down: el scope mas interno siempre gana */
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

/* ── DEBUG DUMP ──────────────────────────────────────────────────────── */

void symbol_dump(void) {
    printf("\n+======= Muad'dib Scope Stack ========+\n");
    for (int level = top; level >= 0; level--) {
        printf("|  Nivel %d — vision: '%s'%s\n",
               level, stack[level].name,
               level == top ? " <- actual" : "");
        int found = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            Symbol* sym = stack[level].buckets[i];
            while (sym != NULL) {
                printf("|    %-20s tipo=%d  scope=%d\n",
                       sym->name, sym->type, sym->scope_level);
                found = 1;
                sym = sym->next;
            }
        }
        if (!found) printf("|    (scope vacio)\n");
        if (level > 0) printf("|  ------------------------------------\n");
    }
    printf("+=====================================+\n\n");
}

/* ── LIBERAR MEMORIA ─────────────────────────────────────────────────── */

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
        if (stack[top].name != NULL) {
            free(stack[top].name);
        }
        top--;
    }
}
