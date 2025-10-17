#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdlib.h>
#include <cmocka.h>
#include "ast/lexer/hbast_lexer.h"
#include "hbapi.h"
#include <string.h>

typedef struct
{
   HB_AST_TOKEN_STREAM * pSnapshot;
   const char * pszModule;
   HB_SIZE nValueRename;
   HB_SIZE nValueMacroSkip;
   HB_SIZE nHelperRename;
} HB_AST_RENAME_FIXTURE;

static void hb_astRenameCollectStats( HB_AST_RENAME_FIXTURE * pFixture )
{
   HB_SIZE nTokenCount = hb_astTokenStreamCount( pFixture->pSnapshot );
   HB_SIZE i;

   for( i = 0; i < nTokenCount; ++i )
   {
      const HB_AST_TOKEN * pTok = hb_astTokenStreamToken( pFixture->pSnapshot, i );

      if( pTok == NULL || pTok->pszLexeme == NULL )
         continue;

      if( pTok->kind == HB_AST_TOKEN_KIND_LITERAL &&
          strcmp( pTok->pszLexeme, "42" ) == 0 )
      {
         if( pTok->pMacroOrigin )
            ++pFixture->nValueMacroSkip;
         else if( pTok->pszModule && strcmp( pTok->pszModule, pFixture->pszModule ) == 0 )
            ++pFixture->nValueRename;
         continue;
      }

      if( ( pTok->kind == HB_AST_TOKEN_KIND_IDENTIFIER ||
            pTok->kind == HB_AST_TOKEN_KIND_KEYWORD ) &&
          hb_stricmp( pTok->pszLexeme, "Helper" ) == 0 &&
          pTok->pMacroOrigin == NULL &&
          pTok->pszModule && strcmp( pTok->pszModule, pFixture->pszModule ) == 0 )
      {
         ++pFixture->nHelperRename;
      }
   }
}

static int hb_astRenameSetup( void ** state )
{
   HB_AST_RENAME_FIXTURE * pFixture = ( HB_AST_RENAME_FIXTURE * ) calloc( 1, sizeof( HB_AST_RENAME_FIXTURE ) );
   HB_AST_LEXER_SOURCE cfg = { "tests/ast/fixture_demo.prg", "tests/ast/fixture_demo.prg", 0, HB_FALSE, HB_TRUE };
   HB_AST_LEXER * pLexer;
   HB_AST_TOKEN token;

   assert_non_null( pFixture );
   pFixture->pszModule = cfg.pszModule;

   pLexer = hb_astLexerNew( &cfg );
   assert_non_null( pLexer );

   while( hb_astLexerNextToken( pLexer, &token ) )
      ;

   pFixture->pSnapshot = hb_astTokenStreamSnapshot( pLexer );
   assert_non_null( pFixture->pSnapshot );

   hb_astLexerFree( pLexer );

   hb_astRenameCollectStats( pFixture );

   *state = pFixture;
   return 0;
}

static int hb_astRenameTeardown( void ** state )
{
   HB_AST_RENAME_FIXTURE * pFixture = ( HB_AST_RENAME_FIXTURE * ) *state;

   if( pFixture )
   {
      if( pFixture->pSnapshot )
         hb_astTokenStreamRelease( pFixture->pSnapshot );
      free( pFixture );
   }
   return 0;
}

static void test_rename_snapshot_available( void ** state )
{
   const HB_AST_RENAME_FIXTURE * pFixture = ( const HB_AST_RENAME_FIXTURE * ) *state;

   assert_non_null( pFixture );
   assert_non_null( pFixture->pSnapshot );
}

static void test_macro_literal_not_renameable( void ** state )
{
   const HB_AST_RENAME_FIXTURE * pFixture = ( const HB_AST_RENAME_FIXTURE * ) *state;

   assert_int_equal( pFixture->nValueRename, 0 );
   assert_int_equal( pFixture->nValueMacroSkip, 1 );
}

static void test_helper_identifier_candidates_exist( void ** state )
{
   const HB_AST_RENAME_FIXTURE * pFixture = ( const HB_AST_RENAME_FIXTURE * ) *state;

   assert_true( pFixture->nHelperRename > 0 );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test_setup_teardown( test_rename_snapshot_available,
                                       hb_astRenameSetup,
                                       hb_astRenameTeardown ),
      cmocka_unit_test_setup_teardown( test_macro_literal_not_renameable,
                                       hb_astRenameSetup,
                                       hb_astRenameTeardown ),
      cmocka_unit_test_setup_teardown( test_helper_identifier_candidates_exist,
                                       hb_astRenameSetup,
                                       hb_astRenameTeardown ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
