%{
/*
 * parser.y — Muad'dib Language Parser (Bison/Yacc)
 *
 * Bison lee este archivo y genera parser.tab.c y parser.tab.h.
 * parser.tab.h contiene las definiciones de tokens que lexer.l necesita.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbols.h"

extern int line_num;
extern int yylex(void);
void yyerror(const char* msg);

/*
 * Helper para división segura: si b == 0, retorna fallback en vez de crashear.
 * El constructor splitby/ordefault llama a esto en tiempo de ejecución.
 */
double safe_divide(double a, double b, double fallback) {
    if (b == 0.0) {
        fprintf(stderr,
            "[Muad'dib] Advertencia: division por cero detectada. "
            "Usando valor ordefault.\n");
        return fallback;
    }
    return a / b;
}
%}

/*
 * ── DECLARACIONES BISON ──────────────────────────────────────────────────
 */

%union {
    int    ival;
    double dval;
    char   cval;
    char*  sval;
}

/* tokens sin valor */
%token IS_TOKEN BECOMES PURPOSE SHOW THEN
%token OPPOSITE WOVEN SPLITBY ORDEFAULT
%token LITERAL VALUE
%token VISION END

/* tokens de tipo */
%token TYPE_WHOLE TYPE_DECIMAL TYPE_LETTER TYPE_WORD TYPE_TRUTH

/* tokens con valor */
%token <ival> YES_VAL NO_VAL
%token <ival> WHOLE_LIT
%token <dval> DECIMAL_LIT
%token <cval> CHAR_LIT
%token <sval> STRING_LIT IDENTIFIER

/* puntuación */
%token EQUALS COLON SEMICOLON PLUS MINUS MULT DIV

/* tipos de no-terminales */
%type <dval> expr
%type <sval> word_expr

/* precedencia: menor a mayor (MULT/DIV ligan más fuerte) */
%left PLUS MINUS
%left MULT DIV

%%

/*
 * scope_init() en la regla program garantiza que el scope global
 * exista antes de que cualquier sentencia se ejecute.
 */
program
    : { scope_init(); } statement_list
    | { scope_init(); }
    ;

statement_list
    : statement_list statement
    | statement
    ;

statement
    : purpose_stmt
    | declaration_stmt
    | assignment_stmt
    | show_stmt
    | vision_stmt
    ;

/* ── PURPOSE ──────────────────────────────────────────────────────────── */

purpose_stmt
    : PURPOSE STRING_LIT SEMICOLON
        {
            printf("\n--- %s ---\n", $2);
            free($2);
        }
    ;

/* ── VISION BLOCK ─────────────────────────────────────────────────────── */

/*
 * push_scope() es una mid-rule action: se ejecuta DESPUÉS de leer
 * VISION STRING_LIT pero ANTES de procesar statement_list.
 * Esto garantiza que el scope esté abierto antes de cualquier sentencia interior.
 */
vision_stmt
    : VISION STRING_LIT { push_scope($2); free($2); }
      statement_list
      END SEMICOLON     { pop_scope(); }
    ;

/* ── DECLARACIONES ─────────────────────────────────────────────────────── */

declaration_stmt
    : IDENTIFIER IS_TOKEN TYPE_WHOLE EQUALS WHOLE_LIT SEMICOLON
        {
            Value v; v.ival = $5;
            symbol_declare($1, DT_WHOLE, v);
            free($1);
        }
    | IDENTIFIER IS_TOKEN TYPE_WHOLE EQUALS expr SEMICOLON
        {
            Value v; v.ival = (int)$5;
            symbol_declare($1, DT_WHOLE, v);
            free($1);
        }
    | IDENTIFIER IS_TOKEN TYPE_DECIMAL EQUALS DECIMAL_LIT SEMICOLON
        {
            Value v; v.dval = $5;
            symbol_declare($1, DT_DECIMAL, v);
            free($1);
        }
    | IDENTIFIER IS_TOKEN TYPE_DECIMAL EQUALS expr SEMICOLON
        {
            Value v; v.dval = $5;
            symbol_declare($1, DT_DECIMAL, v);
            free($1);
        }
    | IDENTIFIER IS_TOKEN TYPE_LETTER EQUALS CHAR_LIT SEMICOLON
        {
            Value v; v.cval = $5;
            symbol_declare($1, DT_LETTER, v);
            free($1);
        }
    | IDENTIFIER IS_TOKEN TYPE_WORD EQUALS STRING_LIT SEMICOLON
        {
            Value v; v.sval = $5;
            symbol_declare($1, DT_WORD, v);
            free($1);
        }
    | IDENTIFIER IS_TOKEN TYPE_WORD EQUALS word_expr SEMICOLON
        {
            Value v; v.sval = $5;
            symbol_declare($1, DT_WORD, v);
            free($1);
        }
    | IDENTIFIER IS_TOKEN TYPE_TRUTH EQUALS YES_VAL SEMICOLON
        {
            Value v; v.ival = $5;
            symbol_declare($1, DT_TRUTH, v);
            free($1);
        }
    | IDENTIFIER IS_TOKEN TYPE_TRUTH EQUALS NO_VAL SEMICOLON
        {
            Value v; v.ival = $5;
            symbol_declare($1, DT_TRUTH, v);
            free($1);
        }
    ;

/* ── ASIGNACIONES ──────────────────────────────────────────────────────── */

assignment_stmt
    : IDENTIFIER BECOMES expr SEMICOLON
        {
            Symbol* sym = symbol_lookup($1);
            if (sym == NULL) {
                fprintf(stderr, "[Muad'dib] Error: '%s' no fue declarado.\n", $1);
            } else if (sym->type == DT_WHOLE) {
                Value v; v.ival = (int)$3;
                symbol_assign($1, v);
            } else if (sym->type == DT_DECIMAL) {
                Value v; v.dval = $3;
                symbol_assign($1, v);
            } else {
                fprintf(stderr, "[Muad'dib] Error: tipo incompatible para '%s'.\n", $1);
            }
            free($1);
        }
    | IDENTIFIER BECOMES STRING_LIT SEMICOLON
        {
            Symbol* sym = symbol_lookup($1);
            if (sym == NULL) {
                fprintf(stderr, "[Muad'dib] Error: '%s' no fue declarado.\n", $1);
                free($3);
            } else if (sym->type == DT_WORD) {
                Value v; v.sval = $3;
                symbol_assign($1, v);
            } else {
                fprintf(stderr, "[Muad'dib] Error: tipo incompatible para '%s'.\n", $1);
                free($3);
            }
            free($1);
        }
    | IDENTIFIER BECOMES word_expr SEMICOLON
        {
            Symbol* sym = symbol_lookup($1);
            if (sym != NULL && sym->type == DT_WORD) {
                Value v; v.sval = $3;
                symbol_assign($1, v);
            } else {
                fprintf(stderr, "[Muad'dib] Error: tipo incompatible para '%s'.\n", $1);
                free($3);
            }
            free($1);
        }
    | IDENTIFIER BECOMES YES_VAL SEMICOLON
        {
            Symbol* sym = symbol_lookup($1);
            if (sym == NULL) {
                fprintf(stderr, "[Muad'dib] Error: '%s' no fue declarado.\n", $1);
            } else if (sym->type == DT_TRUTH) {
                Value v; v.ival = $3;
                symbol_assign($1, v);
            } else {
                fprintf(stderr, "[Muad'dib] Error: '%s' no es una variable truth.\n", $1);
            }
            free($1);
        }
    | IDENTIFIER BECOMES NO_VAL SEMICOLON
        {
            Symbol* sym = symbol_lookup($1);
            if (sym == NULL) {
                fprintf(stderr, "[Muad'dib] Error: '%s' no fue declarado.\n", $1);
            } else if (sym->type == DT_TRUTH) {
                Value v; v.ival = $3;
                symbol_assign($1, v);
            } else {
                fprintf(stderr, "[Muad'dib] Error: '%s' no es una variable truth.\n", $1);
            }
            free($1);
        }
    | IDENTIFIER BECOMES OPPOSITE IDENTIFIER SEMICOLON
        {
            Symbol* dst = symbol_lookup($1);
            Symbol* src = symbol_lookup($4);
            if (dst == NULL) {
                fprintf(stderr, "[Muad'dib] Error: '%s' no fue declarado.\n", $1);
            } else if (dst->type != DT_TRUTH) {
                fprintf(stderr, "[Muad'dib] Error: '%s' no es una variable truth.\n", $1);
            } else if (src == NULL || src->type != DT_TRUTH) {
                fprintf(stderr, "[Muad'dib] Error: 'opposite' requiere una variable truth.\n");
            } else {
                Value v; v.ival = !src->value.ival;
                symbol_assign($1, v);
            }
            free($1); free($4);
        }
    | IDENTIFIER BECOMES expr SPLITBY expr ORDEFAULT expr SEMICOLON
        {
            Symbol* sym = symbol_lookup($1);
            if (sym == NULL) {
                fprintf(stderr, "[Muad'dib] Error: '%s' no fue declarado.\n", $1);
            } else {
                double result = safe_divide($3, $5, $7);
                Value v;
                if (sym->type == DT_WHOLE)        { v.ival = (int)result; }
                else if (sym->type == DT_DECIMAL)  { v.dval = result; }
                else {
                    fprintf(stderr,
                        "[Muad'dib] Error: splitby solo puede asignarse a whole o decimal.\n");
                    free($1);
                    return;
                }
                symbol_assign($1, v);
            }
            free($1);
        }
    ;

/* ── EXPRESIONES ───────────────────────────────────────────────────────── */

/*
 * expr retorna double para manejar tanto whole como decimal con una sola regla.
 * Al almacenar en variable whole, se castea con (int).
 */
expr
    : WHOLE_LIT             { $$ = (double)$1; }
    | DECIMAL_LIT           { $$ = $1; }
    | IDENTIFIER
        {
            Symbol* sym = symbol_lookup($1);
            if (sym == NULL) {
                fprintf(stderr, "[Muad'dib] Error: Variable no declarada '%s'.\n", $1);
                $$ = 0.0;
            } else if (sym->type == DT_WHOLE) {
                $$ = (double)sym->value.ival;
            } else if (sym->type == DT_DECIMAL) {
                $$ = sym->value.dval;
            } else {
                fprintf(stderr, "[Muad'dib] Error: '%s' no es una variable numerica.\n", $1);
                $$ = 0.0;
            }
            free($1);
        }
    | expr PLUS  expr       { $$ = $1 + $3; }
    | expr MINUS expr       { $$ = $1 - $3; }
    | expr MULT  expr       { $$ = $1 * $3; }
    | expr DIV   expr
        {
            if ($3 == 0.0) {
                fprintf(stderr,
                    "[Muad'dib] Advertencia: division por cero. "
                    "Considera usar 'splitby ordefault'.\n");
                $$ = 0.0;
            } else {
                $$ = $1 / $3;
            }
        }
    ;

/* ── EXPRESIONES DE TEXTO ──────────────────────────────────────────────── */

/*
 * word_expr es una regla separada para operaciones sobre strings.
 * woven (concatenación) es la única operación soportada actualmente.
 */
word_expr
    : IDENTIFIER WOVEN IDENTIFIER
        {
            Symbol* left  = symbol_lookup($1);
            Symbol* right = symbol_lookup($3);
            if (left == NULL || right == NULL ||
                left->type != DT_WORD || right->type != DT_WORD) {
                fprintf(stderr,
                    "[Muad'dib] Error: 'woven' requiere dos variables word.\n");
                $$ = strdup("");
            } else {
                size_t len = strlen(left->value.sval)
                           + strlen(right->value.sval) + 2;
                char* result = (char*)malloc(len);
                strcpy(result, left->value.sval);
                strcat(result, " ");
                strcat(result, right->value.sval);
                $$ = result;
            }
            free($1); free($3);
        }
    | IDENTIFIER WOVEN STRING_LIT
        {
            Symbol* left = symbol_lookup($1);
            if (left == NULL || left->type != DT_WORD) {
                fprintf(stderr,
                    "[Muad'dib] Error: el lado izquierdo de 'woven' debe ser una variable word.\n");
                $$ = strdup("");
            } else {
                size_t len = strlen(left->value.sval) + strlen($3) + 2;
                char* result = (char*)malloc(len);
                strcpy(result, left->value.sval);
                strcat(result, " ");
                strcat(result, $3);
                $$ = result;
            }
            free($1); free($3);
        }
    ;

/* ── SHOW ──────────────────────────────────────────────────────────────── */

show_stmt
    : SHOW LITERAL COLON STRING_LIT SEMICOLON
        {
            printf("%s\n", $4);
            free($4);
        }
    | SHOW LITERAL COLON STRING_LIT THEN
        {
            printf("%s ", $4);
            free($4);
        }
      show_chain SEMICOLON
    | SHOW VALUE COLON IDENTIFIER SEMICOLON
        {
            Symbol* sym = symbol_lookup($4);
            if (sym == NULL) {
                fprintf(stderr, "[Muad'dib] Error: variable no declarada '%s'.\n", $4);
            } else if (sym->type == DT_WHOLE) {
                printf("%d\n", sym->value.ival);
            } else if (sym->type == DT_DECIMAL) {
                printf("%.2f\n", sym->value.dval);
            } else if (sym->type == DT_TRUTH) {
                printf("%s\n", sym->value.ival ? "yes" : "no");
            } else {
                fprintf(stderr, "[Muad'dib] Error: 'show value:' requiere variable numerica.\n");
            }
            free($4);
        }
    | SHOW TYPE_WORD COLON IDENTIFIER SEMICOLON
        {
            Symbol* sym = symbol_lookup($4);
            if (sym == NULL) {
                fprintf(stderr, "[Muad'dib] Error: variable no declarada '%s'.\n", $4);
            } else if (sym->type == DT_WORD) {
                printf("%s\n", sym->value.sval);
            } else {
                fprintf(stderr, "[Muad'dib] Error: 'show word:' requiere variable word.\n");
            }
            free($4);
        }
    ;

show_chain
    : VALUE COLON IDENTIFIER
        {
            Symbol* sym = symbol_lookup($3);
            if (sym != NULL) {
                if (sym->type == DT_WHOLE)        printf("%d\n",   sym->value.ival);
                else if (sym->type == DT_DECIMAL)  printf("%.2f\n", sym->value.dval);
                else if (sym->type == DT_TRUTH)    printf("%s\n",   sym->value.ival ? "yes" : "no");
            }
            free($3);
        }
    | TYPE_WORD COLON IDENTIFIER
        {
            Symbol* sym = symbol_lookup($3);
            if (sym != NULL && sym->type == DT_WORD) {
                printf("%s\n", sym->value.sval);
            }
            free($3);
        }
    | LITERAL COLON STRING_LIT
        {
            printf("%s\n", $3);
            free($3);
        }
    ;

%%

void yyerror(const char* msg) {
    fprintf(stderr,
        "[Muad'dib Parser] %s en linea %d\n", msg, line_num);
}
