#include "ast/lexer/hbast_lexer.h"
#include "hbapi.h"
#include <stdio.h>
#include <string.h>

static int report_failure( const char * pszMessage )
{
   fprintf( stderr, "test_rename: %s\n", pszMessage );
   return 1;
}

int main( void )
{
   HB_AST_LEXER_SOURCE cfg = { "tests/ast/demo.prg", "tests/ast/demo.prg", 0, HB_FALSE, HB_TRUE };
   HB_AST_LEXER * pLexer = hb_astLexerNew( &cfg );
   HB_AST_TOKEN token;
   HB_AST_TOKEN_STREAM * pSnapshot;
   HB_SIZE nValueRename = 0;
   HB_SIZE nValueMacroSkip = 0;
   HB_SIZE nHelperRename = 0;
   HB_SIZE i;
   int rc = 0;

   if( pLexer == NULL )
      return report_failure( "unable to create lexer" );

   while( hb_astLexerNextToken( pLexer, &token ) )
      ;

   pSnapshot = hb_astTokenStreamSnapshot( pLexer );
   if( pSnapshot == NULL )
   {
      rc = report_failure( "failed to snapshot token stream" );
   }
   else
   {
      HB_SIZE nTokenCount = hb_astTokenStreamCount( pSnapshot );

      for( i = 0; i < nTokenCount; ++i )
      {
         const HB_AST_TOKEN * pTok = hb_astTokenStreamToken( pSnapshot, i );

      if( pTok == NULL )
         continue;

      if( pTok->kind == HB_AST_TOKEN_KIND_LITERAL &&
          pTok->pszLexeme && strcmp( pTok->pszLexeme, "42" ) == 0 )
      {
         if( pTok->pMacroOrigin )
            ++nValueMacroSkip;
         else if( pTok->pszModule && strcmp( pTok->pszModule, cfg.pszModule ) == 0 )
            ++nValueRename;
         continue;
      }

      if( pTok->kind != HB_AST_TOKEN_KIND_IDENTIFIER &&
          pTok->kind != HB_AST_TOKEN_KIND_KEYWORD )
         continue;

      if( pTok->pszLexeme && hb_stricmp( pTok->pszLexeme, "Helper" ) == 0 )
      {
         if( pTok->pMacroOrigin == NULL &&
             pTok->pszModule && strcmp( pTok->pszModule, cfg.pszModule ) == 0 )
         {
            ++nHelperRename;
         }
      }
      }

      if( nValueRename != 0 )
         rc = report_failure( "macro expansion should not be renameable" );
      else if( nValueMacroSkip != 1 )
         rc = report_failure( "expected exactly one VALUE macro skip" );
      else if( nHelperRename == 0 )
         rc = report_failure( "expected at least one Helper rename candidate" );

      hb_astTokenStreamRelease( pSnapshot );
   }

   hb_astLexerFree( pLexer );
   return rc;
}
