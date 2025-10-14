// tests/ast/smoke.c
#include "ast/lexer/hbast_lexer.h"

int main( void )
{
   const char source[] = "PROC Demo()\nRETURN\n";

   HB_AST_LEXER_SOURCE cfg = { "demo.prg", source, sizeof( source ) - 1, HB_FALSE };
   HB_AST_LEXER *lex = hb_astLexerNew( &cfg );
   HB_AST_TOKEN tok;

   while( hb_astLexerNextToken( lex, &tok ) )  /* hoje sai imediatamente */
   {
      /* imprimir tok.kind, tok.original etc. quando houver implementação real */
   }

   hb_astLexerFree( lex );
   return 0;
}
