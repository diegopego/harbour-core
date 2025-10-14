// tests/ast/smoke.c
#include "ast/lexer/hbast_lexer.h"
#include <stdio.h>

/*
 * Debug output format
 *
 * Example line:
 *   [   1] kind=2 pp=21 span=(3:1:0 -> 3:9:8) module=tests/ast/helpers.ch text="FUNCTION"
 *
 * - Sequence (`[   1]`): local counter emitted by this harness.
 * - `kind`: token category (`HB_AST_TOKEN_KIND_*`). Values: 0 unknown, 1 eof, 2 keyword, 3 identifier,
 *   4 literal, 5 operator, 6 punctuation, 7 directive, 8 macro, 9 newline.
 * - `pp`: raw preprocessor token type (`HB_PP_TOKEN_*`). E.g. 21 KEYWORD, 41 STRING, 42 NUMBER, etc.
 * - `span`: `line:column:offset` start → end (exclusive). Columns are 1-based; the offset is the byte position
 *   in the originating module (exclusive end).
 * - `module`: source file captured by the preprocessor; includes and macro expansions carry their original module.
 * - `text`: lexeme as emitted by the PP after macro/include processing.
 */

int main( void )
{
   HB_AST_LEXER_SOURCE cfg = { "demo.prg", "tests/ast/demo.prg", 0, HB_FALSE, HB_TRUE };
   HB_AST_LEXER *lex = hb_astLexerNew( &cfg );
   HB_AST_TOKEN tok;

   while( hb_astLexerNextToken( lex, &tok ) )  /* hoje sai imediatamente */
   {
      printf( "[%4u] kind=%d pp=%u span=(%lu:%lu:%lu -> %lu:%lu:%lu) module=%s text=\"%.*s\"\n",
              ( unsigned ) tok.id.uHash,
              ( int ) tok.kind,
              ( unsigned ) tok.uPPType,
              ( unsigned long ) tok.original.start.nLine,
              ( unsigned long ) tok.original.start.nColumn,
              ( unsigned long ) tok.original.start.nOffset,
              ( unsigned long ) tok.original.end.nLine,
              ( unsigned long ) tok.original.end.nColumn,
              ( unsigned long ) tok.original.end.nOffset,
              tok.pszModule ? tok.pszModule : "<none>",
              ( int ) tok.nLexemeLength,
              tok.pszLexeme ? tok.pszLexeme : "" );
   }

   puts( "[done] EOF reached" );

   hb_astLexerFree( lex );
   return 0;
}
