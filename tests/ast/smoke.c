// tests/ast/smoke.c
#include "ast/lexer/hbast_lexer.h"
#include <stdio.h>

int main( void )
{
   HB_AST_LEXER_SOURCE cfg = { "demo.prg", "tests/ast/demo.prg", 0, HB_FALSE, HB_TRUE };
   HB_AST_LEXER *lex = hb_astLexerNew( &cfg );
   HB_AST_TOKEN tok;

   while( hb_astLexerNextToken( lex, &tok ) )  /* hoje sai imediatamente */
   {
      printf( "[%4u] kind=%d pp=%u span=(%lu:%lu -> %lu:%lu) module=%s text=\"%.*s\"\n",
              ( unsigned ) tok.id.uHash,
              ( int ) tok.kind,
              ( unsigned ) tok.uPPType,
              ( unsigned long ) tok.original.start.nLine,
              ( unsigned long ) tok.original.start.nColumn,
              ( unsigned long ) tok.original.end.nLine,
              ( unsigned long ) tok.original.end.nColumn,
              tok.pszModule ? tok.pszModule : "<none>",
              ( int ) tok.nLexemeLength,
              tok.pszLexeme ? tok.pszLexeme : "" );
   }

   puts( "[done] EOF reached" );

   hb_astLexerFree( lex );
   return 0;
}
