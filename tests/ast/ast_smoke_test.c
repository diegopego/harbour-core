// tests/ast/ast_smoke_test.c
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "ast/lexer/hbast_lexer.h"
#include "hbapi.h"
#include <string.h>

static const char * HB_AST_SMOKE_MODULE = "tests/ast/fixture_demo.prg";

static HB_AST_TOKEN_STREAM * hb_astSmokeCreateSnapshot( void )
{
   HB_AST_LEXER_SOURCE cfg = { HB_AST_SMOKE_MODULE, HB_AST_SMOKE_MODULE, 0, HB_FALSE, HB_TRUE };
   HB_AST_LEXER * pLexer = hb_astLexerNew( &cfg );
   HB_AST_TOKEN token;
   HB_AST_TOKEN_STREAM * pSnapshot;

   assert_non_null( pLexer );

   while( hb_astLexerNextToken( pLexer, &token ) )
      ;

   pSnapshot = hb_astTokenStreamSnapshot( pLexer );
   hb_astLexerFree( pLexer );

   return pSnapshot;
}

static void test_smoke_lexer_emits_tokens_with_macro_info( void ** state )
{
   HB_AST_LEXER_SOURCE cfg = { HB_AST_SMOKE_MODULE, HB_AST_SMOKE_MODULE, 0, HB_FALSE, HB_TRUE };
   HB_AST_LEXER * pLexer = hb_astLexerNew( &cfg );
   HB_AST_TOKEN token;
   HB_SIZE nTokenCount = 0;
   int fSawProcKeyword = 0;
   int fSawMacroOrigin = 0;

   ( void ) state;

   assert_non_null( pLexer );

   while( hb_astLexerNextToken( pLexer, &token ) )
   {
      ++nTokenCount;
      assert_true( token.kind != HB_AST_TOKEN_KIND_UNKNOWN );

      if( token.kind == HB_AST_TOKEN_KIND_KEYWORD &&
          token.pszLexeme &&
          hb_stricmp( token.pszLexeme, "PROC" ) == 0 )
      {
         fSawProcKeyword = 1;
      }

      if( token.pMacroOrigin != NULL )
         fSawMacroOrigin = 1;
   }

   assert_true( nTokenCount > 0 );
   assert_true( fSawProcKeyword );
   assert_true( fSawMacroOrigin );

   hb_astLexerFree( pLexer );
}

static void test_smoke_snapshot_contains_tokens_and_traces( void ** state )
{
   HB_AST_TOKEN_STREAM * pSnapshot = hb_astSmokeCreateSnapshot();
   HB_SIZE nTokenCount;
   HB_SIZE nTraceCount;

   ( void ) state;

   assert_non_null( pSnapshot );

   nTokenCount = hb_astTokenStreamCount( pSnapshot );
   nTraceCount = hb_astTokenStreamMacroTraceCount( pSnapshot );

   assert_true( nTokenCount > 0 );
   assert_true( nTraceCount > 0 );

   hb_astTokenStreamRelease( pSnapshot );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test( test_smoke_lexer_emits_tokens_with_macro_info ),
      cmocka_unit_test( test_smoke_snapshot_contains_tokens_and_traces ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
