#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "ast/hbast_builder.h"
#include "ast/lexer/hbast_lexer.h"
#include "hbapi.h"
#include <string.h>
#include <ctype.h>
#include <stdint.h>

static HB_AST_TOKEN_STREAM * hb_astCreateSnapshot( const char * pszModule )
{
   HB_AST_LEXER_SOURCE cfg = { pszModule, pszModule, 0, HB_FALSE, HB_TRUE };
   HB_AST_LEXER * pLexer = hb_astLexerNew( &cfg );
   HB_AST_TOKEN token;
   HB_AST_TOKEN_STREAM * pStream;

   assert_non_null( pLexer );

   while( hb_astLexerNextToken( pLexer, &token ) )
      ;

   pStream = hb_astTokenStreamSnapshot( pLexer );
   hb_astLexerFree( pLexer );
   assert_non_null( pStream );
   return pStream;
}

static const HB_AST_SYMBOL_INFO * hb_astFindSymbolByName( const HB_AST_BUILD_RESULT * pResult, const char * pszName )
{
   HB_SIZE i;

   for( i = 0; i < pResult->nSymbolCount; ++i )
   {
      if( pResult->pSymbols[ i ].pszName && hb_stricmp( pResult->pSymbols[ i ].pszName, pszName ) == 0 )
         return &pResult->pSymbols[ i ];
   }
   return NULL;
}

static const HB_AST_SYMBOL_INFO * hb_astFindSymbolByQualifiedName( const HB_AST_BUILD_RESULT * pResult,
                                                                   const char * pszQualifiedName )
{
   HB_SIZE i;

   for( i = 0; i < pResult->nSymbolCount; ++i )
   {
      if( pResult->pSymbols[ i ].pszQualifiedName &&
          hb_stricmp( pResult->pSymbols[ i ].pszQualifiedName, pszQualifiedName ) == 0 )
         return &pResult->pSymbols[ i ];
   }

   return NULL;
}

static const HB_AST_NODE_INFO * hb_astFindNodeBySymbolName( const HB_AST_BUILD_RESULT * pResult, const char * pszName )
{
   const HB_AST_SYMBOL_INFO * pSymbol = hb_astFindSymbolByName( pResult, pszName );
   HB_SIZE i;

   if( pSymbol == NULL )
      return NULL;

   for( i = 0; i < pResult->nNodeCount; ++i )
   {
      const HB_AST_NODE_INFO * pNode = &pResult->pNodes[ i ];

      if( pNode->symbolId == pSymbol->symbolId )
         return pNode;
   }
   return NULL;
}

static const HB_AST_NODE_INFO * hb_astFindChildByKind( const HB_AST_BUILD_RESULT * pResult,
                                                       HB_SIZE nParentId,
                                                       const char * pszKind )
{
   HB_SIZE i;

   for( i = 0; i < pResult->nNodeCount; ++i )
   {
      const HB_AST_NODE_INFO * pNode = &pResult->pNodes[ i ];

      if( pNode->parentId == nParentId &&
          pNode->pszKind &&
          hb_stricmp( pNode->pszKind, pszKind ) == 0 )
      {
         return pNode;
      }
   }

   return NULL;
}

static HB_U64 hb_astTestHashLower( HB_U64 hash, const char * pszValue )
{
   const unsigned char * pch = ( const unsigned char * ) ( pszValue ? pszValue : "" );

   while( *pch )
   {
      hash ^= ( HB_U64 ) tolower( *pch++ );
      hash *= UINT64_C( 1099511628211 );
   }

   return hash;
}

static HB_SIZE hb_astTestComputeSymbolId( const char * pszModule,
                                          const char * pszKind,
                                          const char * pszQualifiedName )
{
   HB_U64 hash = UINT64_C( 14695981039346656037 );
   HB_SIZE result;

   hash = hb_astTestHashLower( hash, pszModule );
   hash = hb_astTestHashLower( hash, "::" );
   hash = hb_astTestHashLower( hash, pszKind );
   hash = hb_astTestHashLower( hash, "::" );
   hash = hb_astTestHashLower( hash, pszQualifiedName );

   hash &= ( HB_U64 ) HB_SIZE_MAX;
   result = ( HB_SIZE ) hash;

   if( result == HB_SIZE_MAX )
      --result;
   if( result == 0 )
      result = 1;

   return result;
}

static void test_ast_builder_demo_module( void ** state )
{
   HB_AST_TOKEN_STREAM * pStream = hb_astCreateSnapshot( "tests/ast/fixture_demo.prg" );
   HB_AST_BUILD_RESULT result;
   const HB_AST_NODE_INFO * pDemoNode;
   const HB_AST_NODE_INFO * pOuterNode;
   const HB_AST_NODE_INFO * pInnerNode;
   const HB_AST_NODE_INFO * pExportNode;
   const HB_AST_SYMBOL_INFO * pVar;

   ( void ) state;

   assert_true( hb_astBuildFromStream( pStream, "tests/ast/fixture_demo.prg", &result ) );
   assert_int_equal( result.nRootId, 0 );
   assert_true( result.nNodeCount >= 12 );
   assert_true( result.nSymbolCount >= 7 );

   pDemoNode = hb_astFindNodeBySymbolName( &result, "Demo" );
   assert_non_null( pDemoNode );
   assert_int_equal( pDemoNode->parentId, result.nRootId );
   assert_non_null( hb_astFindChildByKind( &result, pDemoNode->id, "LocalDecl" ) );
   assert_non_null( hb_astFindChildByKind( &result, pDemoNode->id, "ReturnStmt" ) );

   pOuterNode = hb_astFindNodeBySymbolName( &result, "Outer" );
   assert_non_null( pOuterNode );
   assert_int_equal( pOuterNode->parentId, result.nRootId );
   assert_non_null( hb_astFindChildByKind( &result, pOuterNode->id, "LocalDecl" ) );
   assert_non_null( hb_astFindChildByKind( &result, pOuterNode->id, "ReturnStmt" ) );

   pInnerNode = hb_astFindNodeBySymbolName( &result, "InnerProc" );
   assert_non_null( pInnerNode );
   assert_int_equal( pInnerNode->parentId, result.nRootId );
   assert_non_null( hb_astFindChildByKind( &result, pInnerNode->id, "LocalDecl" ) );
   assert_non_null( hb_astFindChildByKind( &result, pInnerNode->id, "ReturnStmt" ) );

   pExportNode = hb_astFindNodeBySymbolName( &result, "Exported" );
   assert_non_null( pExportNode );
   assert_int_equal( pExportNode->parentId, result.nRootId );
   assert_null( hb_astFindChildByKind( &result, pExportNode->id, "LocalDecl" ) );
   assert_non_null( hb_astFindChildByKind( &result, pExportNode->id, "ReturnStmt" ) );

   pVar = hb_astFindSymbolByName( &result, "n" );
   assert_non_null( pVar );
   assert_int_equal( pVar->nDeclarationCount, 1 );
   assert_true( pVar->nReferenceCount >= 1 );

   pVar = hb_astFindSymbolByName( &result, "cName" );
   assert_non_null( pVar );
   assert_int_equal( pVar->nDeclarationCount, 1 );
   assert_true( pVar->nReferenceCount >= 1 );

   pVar = hb_astFindSymbolByName( &result, "nCount" );
   assert_non_null( pVar );
   assert_int_equal( pVar->nDeclarationCount, 1 );
   assert_true( pVar->nReferenceCount >= 1 );

   assert_null( hb_astFindSymbolByName( &result, "IncludedProc" ) );

   hb_astBuildResultRelease( &result );
   hb_astTokenStreamRelease( pStream );
}

static void test_ast_builder_helpers_module( void ** state )
{
   HB_AST_TOKEN_STREAM * pStream = hb_astCreateSnapshot( "tests/ast/fixture_demo.prg" );
   HB_AST_BUILD_RESULT result;
   const HB_AST_NODE_INFO * pHelperNode;

   ( void ) state;

   assert_true( hb_astBuildFromStream( pStream, "tests/ast/fixture_helpers.ch", &result ) );
   assert_int_equal( result.nRootId, 0 );
   assert_true( result.nNodeCount >= 3 );

   pHelperNode = hb_astFindNodeBySymbolName( &result, "Helper" );
   assert_non_null( pHelperNode );
   assert_non_null( hb_astFindChildByKind( &result, pHelperNode->id, "ReturnStmt" ) );

   assert_null( hb_astFindSymbolByName( &result, "Demo" ) );

   hb_astBuildResultRelease( &result );
   hb_astTokenStreamRelease( pStream );
}

static void test_ast_builder_extrahelpers_module( void ** state )
{
   HB_AST_TOKEN_STREAM * pStream = hb_astCreateSnapshot( "tests/ast/fixture_demo.prg" );
   HB_AST_BUILD_RESULT result;
   const HB_AST_NODE_INFO * pIncludedNode;

   ( void ) state;

   assert_true( hb_astBuildFromStream( pStream, "tests/ast/fixture_extrahelpers.ch", &result ) );
   assert_int_equal( result.nRootId, 0 );
   assert_true( result.nNodeCount >= 3 );

   pIncludedNode = hb_astFindNodeBySymbolName( &result, "IncludedProc" );
   assert_non_null( pIncludedNode );
   assert_non_null( hb_astFindChildByKind( &result, pIncludedNode->id, "ReturnStmt" ) );

   assert_null( hb_astFindSymbolByName( &result, "INLINE_HELPER" ) );

   hb_astBuildResultRelease( &result );
   hb_astTokenStreamRelease( pStream );
}

static void test_ast_builder_symbol_qualification( void ** state )
{
   HB_AST_TOKEN_STREAM * pStream = hb_astCreateSnapshot( "tests/ast/fixture_demo.prg" );
   HB_AST_BUILD_RESULT result;
   const HB_AST_SYMBOL_INFO * pFunction;
   const HB_AST_SYMBOL_INFO * pLocalDemo;
   const HB_AST_SYMBOL_INFO * pLocalOuter;
   const HB_AST_SYMBOL_INFO * pLocalInner;

   ( void ) state;

   assert_true( hb_astBuildFromStream( pStream, "tests/ast/fixture_demo.prg", &result ) );

   pFunction = hb_astFindSymbolByQualifiedName( &result, "Demo" );
   assert_non_null( pFunction );
   assert_string_equal( pFunction->pszName, "Demo" );

   pLocalDemo = hb_astFindSymbolByQualifiedName( &result, "Demo::n" );
   assert_non_null( pLocalDemo );
   assert_string_equal( pLocalDemo->pszName, "n" );

   pLocalOuter = hb_astFindSymbolByQualifiedName( &result, "Outer::cName" );
   assert_non_null( pLocalOuter );
   assert_string_equal( pLocalOuter->pszName, "cName" );

   pLocalInner = hb_astFindSymbolByQualifiedName( &result, "InnerProc::nCount" );
   assert_non_null( pLocalInner );
   assert_string_equal( pLocalInner->pszName, "nCount" );

   hb_astBuildResultRelease( &result );
   hb_astTokenStreamRelease( pStream );
}

static void test_ast_builder_symbol_id_stability( void ** state )
{
   HB_AST_TOKEN_STREAM * pStream = hb_astCreateSnapshot( "tests/ast/fixture_demo.prg" );
   HB_AST_BUILD_RESULT result;
   HB_SIZE i;
   const char * pszModule = "tests/ast/fixture_demo.prg";

   ( void ) state;

   assert_true( hb_astBuildFromStream( pStream, pszModule, &result ) );

  for( i = 0; i < result.nSymbolCount; ++i )
  {
      const HB_AST_SYMBOL_INFO * pSymbol = &result.pSymbols[ i ];
      const char * pszKind = pSymbol->pszKind ? pSymbol->pszKind : "Symbol";
      const char * pszQualifier = pSymbol->pszQualifiedName ? pSymbol->pszQualifiedName : "";
      HB_SIZE expected = hb_astTestComputeSymbolId( pszModule, pszKind, pszQualifier );

      assert_int_equal( pSymbol->symbolId, expected );
  }

   hb_astBuildResultRelease( &result );
   hb_astTokenStreamRelease( pStream );
}

static void test_ast_builder_serialization_snapshot( void ** state )
{
   HB_AST_TOKEN_STREAM * pStream = hb_astCreateSnapshot( "tests/ast/fixture_demo.prg" );
   HB_SIZE nJsonLen = 0;
   HB_SIZE nCborLen = 0;
   char * pszJson;
   HB_BYTE * pCbor;
   char * pszSymbols;
   HB_SIZE nSymbolsLen = 0;

   ( void ) state;

   pszJson = hb_astTokenStreamSerializeSnapshotJson( pStream, "tests/ast/fixture_demo.prg", &nJsonLen );
   assert_non_null( pszJson );
   assert_true( nJsonLen > 0 );
   assert_non_null( strstr( pszJson, "\"FunctionDecl\"" ) );
   hb_astTokenStreamSerializeSnapshotJsonFree( pszJson );

   pCbor = hb_astTokenStreamSerializeSnapshotCbor( pStream, "tests/ast/fixture_demo.prg", &nCborLen );
   assert_non_null( pCbor );
   assert_true( nCborLen > 0 );
   hb_astTokenStreamSerializeSnapshotCborFree( pCbor );

   pszSymbols = hb_astTokenStreamSerializeSymbolsJson( pStream, "tests/ast/fixture_demo.prg", &nSymbolsLen );
   assert_non_null( pszSymbols );
   assert_true( nSymbolsLen > 0 );
   assert_non_null( strstr( pszSymbols, "\"name\":\"Demo\"" ) );
   assert_non_null( strstr( pszSymbols, "\"name\":\"Outer\"" ) );
   assert_non_null( strstr( pszSymbols, "\"name\":\"Exported\"" ) );
   hb_astTokenStreamSerializeSymbolsJsonFree( pszSymbols );

   hb_astTokenStreamRelease( pStream );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test( test_ast_builder_demo_module ),
      cmocka_unit_test( test_ast_builder_helpers_module ),
      cmocka_unit_test( test_ast_builder_extrahelpers_module ),
      cmocka_unit_test( test_ast_builder_symbol_qualification ),
      cmocka_unit_test( test_ast_builder_symbol_id_stability ),
      cmocka_unit_test( test_ast_builder_serialization_snapshot ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
