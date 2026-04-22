#ifndef SYMBOLS_H
#define SYMBOLS_H

/*
 * symbols.h — Tabla de Simbolos de Muad'dib (Scope Stack)
 *
 * Arquitectura: pila de tablas hash, una tabla por nivel de scope.
 *   scope_stack[0]  <- scope global  (fondo, siempre existe)
 *   scope_stack[1]  <- vision nivel 1
 *   scope_stack[2]  <- vision nivel 2  <- lookup empieza AQUI (tope)
 *
 * LOOKUP:  empieza en el tope, baja hasta encontrar o no encontrar.
 * DECLARE: siempre inserta en el scope del TOPE.
 * POP:     destruye el scope del tope y todas sus variables.
 *
 * Por que tabla hash y no lista enlazada?
 * Lista enlazada: O(n). Tabla hash: O(1) promedio.
 * Con scopes anidados, lookup se llama muchas veces.
 * La inversion en hash paga inmediatamente.
 *
 * Por que pila de tablas y no una tabla unica con prefijos de scope?
 * pop_scope() con prefijos requeriria escanear TODA la tabla.
 * Con pila de tablas, pop_scope() es: liberar la tabla del tope. O(n_vars).
 */

/* ── CONSTANTES ──────────────────────────────────────────────────────── */

#define HASH_SIZE   64   /* buckets por scope — potencia de 2 para bitmask */
#define MAX_SCOPES  16   /* maxima profundidad de anidamiento */

/* ── TIPOS DE DATOS ──────────────────────────────────────────────────── */

typedef enum {
    DT_WHOLE,    /* entero — equivalente a int    */
    DT_DECIMAL,  /* decimal — equivalente a double */
    DT_LETTER,   /* caracter — equivalente a char  */
    DT_WORD,     /* cadena — equivalente a string  */
    DT_TRUTH     /* booleano — equivalente a bool  */
} DataType;

/*
 * Union: solo un campo es valido a la vez (el que corresponde al DataType).
 * Usa menos memoria que un struct con todos los campos.
 */
typedef union {
    int    ival;   /* DT_WHOLE y DT_TRUTH */
    double dval;   /* DT_DECIMAL */
    char   cval;   /* DT_LETTER */
    char*  sval;   /* DT_WORD */
} Value;

typedef struct Symbol {
    char*         name;         /* nombre de la variable (heap, owned) */
    DataType      type;         /* tipo de dato */
    Value         value;        /* valor actual */
    int           scope_level;  /* nivel de scope donde fue declarada */
    struct Symbol* next;        /* siguiente en la cadena del bucket */
} Symbol;

typedef struct {
    Symbol* buckets[HASH_SIZE]; /* tabla hash */
    int     level;              /* profundidad de scope (0 = global) */
    char*   name;               /* nombre del vision, o "global" */
} ScopeTable;

/* ── OPERACIONES DE SCOPE ────────────────────────────────────────────── */

void scope_init(void);                    /* crea el scope global */
void push_scope(const char* vision_name); /* entra a un bloque vision */
void pop_scope(void);                     /* sale de un bloque vision */
int  current_scope_level(void);           /* retorna profundidad actual */

/* ── OPERACIONES DE SIMBOLOS ─────────────────────────────────────────── */

void    symbol_declare(const char* name, DataType type, Value value);
void    symbol_assign(const char* name, Value value);
Symbol* symbol_lookup(const char* name);
void    symbol_dump(void);
void    symbol_free(void);

#endif /* SYMBOLS_H */
