%{
/*
 * lexer.l — Muad'dib Lexer
 *
 * Convierte el texto fuente en un flujo de tokens para el parser.
 * Usa Flex (Fast Lexical Analyzer Generator).
 *
 * NOTA DE ORDEN CRÍTICO:
 * Las palabras clave DEBEN venir antes de la regla IDENTIFIER.
 * Flex usa longest-match-first; en caso de empate, usa first-rule-first.
 * Listamos keywords primero para garantizar que no sean identificadores.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"

int line_num = 1;
%}

IDENTIFIER  [a-z][a-zA-Z0-9]*
WHOLE_LIT   [0-9]+
DECIMAL_LIT [0-9]+"."[0-9]+
STRING_LIT  \"[^\"]*\"
CHAR_LIT    '.'
WHITESPACE  [ \t\r]+

%%

 /* ESPACIOS EN BLANCO Y SALTOS DE LÍNEA */
{WHITESPACE}    { /* ignorar espacios, tabulaciones */ }
\n              { line_num++; }

 /* COMENTARIOS — descartados en el léxico, nunca llegan al parser */
"//"[^\n]*      { /* comentario general */ }
"#why:"[^\n]*   { /* comentario de decisión de diseño */ }
"#todo:"[^\n]*  { /* comentario de tarea pendiente */ }
"#warn:"[^\n]*  { /* comentario de advertencia */ }

 /* PALABRAS CLAVE — deben ir antes de la regla IDENTIFIER */
"is"        { return IS_TOKEN; }
"becomes"   { return BECOMES; }
"purpose"   { return PURPOSE; }
"show"      { return SHOW; }
"then"      { return THEN; }

 /* PALABRAS CLAVE DE SCOPE */
"vision"    { return VISION; }
"end"       { return END; }

 /* PALABRAS CLAVE DE TIPOS */
"whole"     { return TYPE_WHOLE; }
"decimal"   { return TYPE_DECIMAL; }
"letter"    { return TYPE_LETTER; }
"word"      { return TYPE_WORD; }
"truth"     { return TYPE_TRUTH; }

 /* CALIFICADORES DE SHOW */
"literal"   { return LITERAL; }
"value"     { return VALUE; }

 /* LITERALES BOOLEANOS */
"yes"       { yylval.ival = 1; return YES_VAL; }
"no"        { yylval.ival = 0; return NO_VAL; }

 /* OPERADORES ESPECIALES */
"opposite"  { return OPPOSITE; }
"woven"     { return WOVEN; }
"splitby"   { return SPLITBY; }
"ordefault" { return ORDEFAULT; }

 /* OPERADORES ARITMÉTICOS */
"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MULT; }
"/"         { return DIV; }

 /* PUNTUACIÓN */
"="         { return EQUALS; }
":"         { return COLON; }
";"         { return SEMICOLON; }

 /* LITERALES — DECIMAL antes de WHOLE para que "3.14" no se lea como "3" */
{DECIMAL_LIT}   { yylval.dval = atof(yytext); return DECIMAL_LIT; }
{WHOLE_LIT}     { yylval.ival = atoi(yytext); return WHOLE_LIT; }

{STRING_LIT}    {
                    /* eliminar comillas: "hello" -> hello */
                    yylval.sval = strndup(yytext + 1, yyleng - 2);
                    return STRING_LIT;
                }

{CHAR_LIT}      {
                    /* yytext[0] es la comilla, yytext[1] es el caracter */
                    yylval.cval = yytext[1];
                    return CHAR_LIT;
                }

 /* IDENTIFICADOR — después de todas las palabras clave */
{IDENTIFIER}    {
                    /* strdup: yytext se sobreescribe con cada token */
                    yylval.sval = strdup(yytext);
                    return IDENTIFIER;
                }

 /* CATCH-ALL: caracter desconocido — reportar y continuar */
.               {
                    fprintf(stderr,
                        "[Muad'dib Lexer] Caracter desconocido '%s' en linea %d\n",
                        yytext, line_num);
                }

%%

int yywrap(void) {
    return 1;
}
