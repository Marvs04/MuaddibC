%{
%{
/*
 * lexer.l - Muad'dib Lexer (v2: adds vision/end for scope blocks)
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

 /* WHITESPACE & NEWLINES */
{WHITESPACE}            { }
\n                      { line_num++; }

 /* COMMENTS */
"//"[^\n]*              { }
"#why:"[^\n]*           { }
"#todo:"[^\n]*          { }
"#warn:"[^\n]*          { }

 /* KEYWORDS - must come before IDENTIFIER rule */
"is"                    { return IS_TOKEN; }
"becomes"               { return BECOMES; }
"purpose"               { return PURPOSE; }
"show"                  { return SHOW; }
"then"                  { return THEN; }

 /* SCOPE KEYWORDS */
 /* WHY vision/end and not { } ?
    Braces are symbols — they carry no meaning.
    vision announces: "I am entering a new layer of reality."
    end announces: "This layer is over. Its variables are gone."
    The words make the concept visible, not just syntactic noise. */
"vision"                { return VISION; }
"end"                   { return END; }

 /* TYPE KEYWORDS */
"whole"                 { return TYPE_WHOLE; }
"decimal"               { return TYPE_DECIMAL; }
"letter"                { return TYPE_LETTER; }
"word"                  { return TYPE_WORD; }
"truth"                 { return TYPE_TRUTH; }

 /* SHOW QUALIFIERS */
"literal"               { return LITERAL; }
"value"                 { return VALUE; }

 /* BOOLEAN LITERALS */
"yes"                   { yylval.ival = 1; return YES_VAL; }
"no"                    { yylval.ival = 0; return NO_VAL; }

 /* SPECIAL OPERATORS */
"opposite"              { return OPPOSITE; }
"woven"                 { return WOVEN; }
"splitby"               { return SPLITBY; }
"ordefault"             { return ORDEFAULT; }

 /* ARITHMETIC */
"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return MULT; }
"/"                     { return DIV; }

 /* PUNCTUATION */
"="                     { return EQUALS; }
":"                     { return COLON; }
";"                     { return SEMICOLON; }

 /* LITERALS */
{DECIMAL_LIT}           { yylval.dval = atof(yytext); return DECIMAL_LIT; }
{WHOLE_LIT}             { yylval.ival = atoi(yytext); return WHOLE_LIT; }

{STRING_LIT}            {
                            yylval.sval = strndup(yytext + 1, yyleng - 2);
                            return STRING_LIT;
                        }

{CHAR_LIT}              {
                            yylval.cval = yytext[1];
                            return CHAR_LIT;
                        }

 /* IDENTIFIER - after all keywords */
{IDENTIFIER}            { yylval.sval = strdup(yytext); return IDENTIFIER; }

 /* CATCH-ALL */
.                       {
                            fprintf(stderr,
                                "[Muad'dib] Unknown character '%s' at line %d\n",
                                yytext, line_num);
                        }

%%

int yywrap(void) {
    return 1;
}
                    line_num++;
                    /*
                     * WHY increment here and not in WHITESPACE?
                     * Because \n is a newline, not just whitespace.
                     * We need the count for error reporting.
                     * Whitespace (spaces, tabs) doesn't change the line.
                     */
                }

    /* ── COMMENT RULES ───────────────────────────────────────────────────────── */

    /*
  * WHY match comments in the LEXER and not the PARSER?
  * Comments are not part of the grammar — they carry no semantic meaning
  * for the program's execution. The cleanest place to discard them is
  * right here in the lexer, before they ever reach the parser.
  *
  * WHY use [^\n]* to consume comment content?
  * Because a comment ends at the newline. [^\n]* means "everything
  * that is not a newline" — exactly what we want for single-line comments.
  *
  * WHY handle #why:, #todo:, #warn: separately from //?
  * In the future, a linter or documentation tool could scan for these
  * specific comment types and extract structured information. By giving
  * them their own lexer rules, we leave that door open. For now, all
  * comments are discarded — but the STRUCTURE is preserved in the code.
  */
"//"[^\n]*      { /* general comment — ignore */ }
"#why:"[^\n]*   { /* design decision comment — ignore at runtime */ }
"#todo:"[^\n]*  { /* pending task comment — ignore at runtime */ }
"#warn:"[^\n]*  { /* warning comment — ignore at runtime */ }

    /* ── KEYWORDS ───────────────────────────────────────────────────────── */

    /*
  * WHY return token constants and not strings?
  * The parser (Bison) works with integer token codes, not strings.
  * Each token defined in parser.y with %token gets a unique integer.
  * Returning IS_TOKEN tells the parser "I saw the keyword 'is'".
  * Returning a string would require string comparison — slower and fragile.
  */

"is"        { return IS_TOKEN; }
"becomes"   { return BECOMES; }
"purpose"   { return PURPOSE; }
"show"      { return SHOW; }
"then"      { return THEN; }

 /* ── TYPE KEYWORDS ──────────────────────────────────────────────────── */
"whole"     { return TYPE_WHOLE; }
"decimal"   { return TYPE_DECIMAL; }
"letter"    { return TYPE_LETTER; }
"word"      { return TYPE_WORD; }
"truth"     { return TYPE_TRUTH; }

    /* ── SHOW QUALIFIER KEYWORDS ────────────────────────────────────────── */
    /*
  * WHY are "literal", "value", "word" keywords?
  * In the grammar, "show literal: ..." and "show value: ..." are
  * distinct constructs. The parser needs to KNOW which kind of show
  * it's dealing with. Making them tokens (not just the IDENTIFIER
  * "literal") means the parser can have separate grammar rules for each.
  * This is cleaner than checking the string value of an identifier.
  *
  * NOTE: "word" is both a TYPE keyword and a show qualifier. The same
  * token TYPE_WORD is reused — the parser's grammar handles the context.
  */
"literal"   { return LITERAL; }
"value"     { return VALUE; }

    /* ── BOOLEAN VALUE KEYWORDS ─────────────────────────────────────────── */
    /*
  * WHY return YES_VAL / NO_VAL with yylval instead of just the token?
  * Because the parser needs to know the actual boolean value to store
  * it in the symbol table. yylval.ival carries the integer (1 or 0).
  */
"yes"       { yylval.ival = 1; return YES_VAL; }
"no"        { yylval.ival = 0; return NO_VAL; }

    /* ── SPECIAL OPERATORS ──────────────────────────────────────────────── */
"opposite"  { return OPPOSITE; }
"woven"     { return WOVEN; }
"splitby"   { return SPLITBY; }
"ordefault" { return ORDEFAULT; }

    /* ── ARITHMETIC OPERATORS ───────────────────────────────────────────── */
    /*
  * WHY return single character tokens with return *yytext?
  * For simple single-character tokens like +, -, *, /,
  * we can return the ASCII value of the character itself.
  * Bison allows this — a token can be its own character code.
  * This avoids defining separate token names for simple symbols.
  */
"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MULT; }
"/"         { return DIV; }

    /* ── PUNCTUATION ────────────────────────────────────────────────────── */
"="         { return EQUALS; }
":"         { return COLON; }
";"         { return SEMICOLON; }

    /* ── LITERALS ────────────────────────────────────────────────────────── */

{DECIMAL_LIT}   {
                    /*
                     * WHY DECIMAL before WHOLE?
                     * "3.14" would partially match WHOLE_LIT as "3",
                     * then fail on the dot. By listing DECIMAL_LIT first,
                     * Flex tries the longer decimal pattern first.
                     * Actually Flex uses longest match automatically, but
                     * listing decimal first makes intent explicit.
                     */
                    yylval.dval = atof(yytext);
                    return DECIMAL_LIT;
                }

{WHOLE_LIT}     {
                    yylval.ival = atoi(yytext);
                    return WHOLE_LIT;
                }

{STRING_LIT}    {
                    /*
                     * WHY copy yytext+1 and cut the last char?
                     * yytext contains the quotes: "hello"
                     * We want just: hello
                     * yytext+1 skips the opening quote.
                     * yyleng-2 removes both quotes from the length.
                     * We strdup to own the memory — yytext is overwritten
                     * on the next token.
                     */
                    yylval.sval = strndup(yytext + 1, yyleng - 2);
                    return STRING_LIT;
                }

{CHAR_LIT}      {
                    /*
                     * WHY yytext[1] and not yytext[0]?
                     * yytext[0] is the opening single quote.
                     * yytext[1] is the actual character we want.
                     */
                    yylval.cval = yytext[1];
                    return CHAR_LIT;
                }

 /* ── IDENTIFIER ─────────────────────────────────────────────────────── */

{IDENTIFIER}    {
                    /*
                     * WHY strdup here?
                     * yytext points to Flex's internal buffer, which gets
                     * overwritten with every new token. If we stored
                     * yytext directly, we'd lose the name. strdup gives us
                     * our own copy on the heap that persists.
                     */
                    yylval.sval = strdup(yytext);
                    return IDENTIFIER;
                }

    /* ── CATCH-ALL ERROR RULE ───────────────────────────────────────────── */

.               {
                    /*
                     * WHY have a catch-all rule?
                     * If Flex encounters a character that matches nothing,
                     * its DEFAULT behavior is to print it and move on.
                     * That's silent and confusing. Instead, we explicitly
                     * report an error with the line number.
                     * The dot . matches any single character not matched above.
                     */
                    fprintf(stderr,
                        "[Muad'dib Lexer] Unknown character '%s' at line %d\n",
                        yytext, line_num);
                }

%%

/*
 * ── USER CODE ────────────────────────────────────────────────────────────
 *
 * WHY define yywrap here?
 * Flex calls yywrap() when it reaches the end of input to ask:
 * "is there more input from another file?"
 * Returning 1 means "no, we're done". This is required — without it,
 * the linker will complain about a missing symbol.
 * An alternative is to use %option noyywrap in the definitions section,
 * but defining it explicitly is more transparent about what's happening.
 */
int yywrap(void) {
    return 1;
}
