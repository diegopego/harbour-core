#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>
#include "ast/lexer/hbast_lexer.h"
#include "hbapi.h"

typedef struct
{
   HB_AST_TOKEN_STREAM * pSnapshot;
   const char * pszModule;
} HB_AST_SNAPSHOT_FIXTURE;

static const char * HB_AST_MACRO_JSON_PATH = "tests/ast/out_macro.json";
static const char * HB_AST_SNAPSHOT_JSON_PATH = "tests/ast/out_snapshot.json";
static const char * HB_AST_MACRO_CBOR_PATH = "tests/ast/out_macro.cbor";
static const char * HB_AST_SNAPSHOT_CBOR_PATH = "tests/ast/out_snapshot.cbor";

static void hb_astSnapshotCleanupFiles( void )
{
   remove( HB_AST_MACRO_JSON_PATH );
   remove( HB_AST_SNAPSHOT_JSON_PATH );
   remove( HB_AST_MACRO_CBOR_PATH );
   remove( HB_AST_SNAPSHOT_CBOR_PATH );
}

static int hb_astSnapshotSetup( void ** state )
{
   HB_AST_SNAPSHOT_FIXTURE * pFixture = ( HB_AST_SNAPSHOT_FIXTURE * ) calloc( 1, sizeof( HB_AST_SNAPSHOT_FIXTURE ) );
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

   hb_astSnapshotCleanupFiles();

   *state = pFixture;
    return 0;
}

static int hb_astSnapshotTeardown( void ** state )
{
   HB_AST_SNAPSHOT_FIXTURE * pFixture = ( HB_AST_SNAPSHOT_FIXTURE * ) *state;

   if( pFixture )
   {
      if( pFixture->pSnapshot )
         hb_astTokenStreamRelease( pFixture->pSnapshot );
      free( pFixture );
   }

   hb_astSnapshotCleanupFiles();
   return 0;
}

static const HB_AST_TOKEN * hb_astSnapshotFindLiteral( const HB_AST_SNAPSHOT_FIXTURE * pFixture, const char * pszLexeme )
{
   HB_SIZE nTokenCount = hb_astTokenStreamCount( pFixture->pSnapshot );
   HB_SIZE i;

   for( i = 0; i < nTokenCount; ++i )
   {
      const HB_AST_TOKEN * pTok = hb_astTokenStreamToken( pFixture->pSnapshot, i );

      if( pTok &&
          pTok->kind == HB_AST_TOKEN_KIND_LITERAL &&
          pTok->pszLexeme &&
          strcmp( pTok->pszLexeme, pszLexeme ) == 0 )
      {
         return pTok;
      }
   }

   return NULL;
}

static void hb_astAssertContains( const char * pszHaystack, const char * pszNeedle )
{
   assert_non_null( pszHaystack );
   assert_non_null( pszNeedle );
   assert_non_null( strstr( pszHaystack, pszNeedle ) );
}

static void test_snapshot_macro_literal_metadata( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   const HB_AST_TOKEN * pLiteral = hb_astSnapshotFindLiteral( pFixture, "42" );
   const void * pTrace;
   HB_AST_SOURCE_RANGE callRange;

   assert_non_null( pLiteral );
   assert_non_null( pLiteral->pMacroOrigin );
   assert_true( pLiteral->id.nMacroDepth > 0 );

   pTrace = pLiteral->pMacroOrigin;

   assert_non_null( hb_astMacroTraceName( pTrace ) );
   assert_string_equal( hb_astMacroTraceName( pTrace ), "VALUE" );

   assert_non_null( hb_astMacroTraceCallModule( pTrace ) );
   assert_string_equal( hb_astMacroTraceCallModule( pTrace ), pFixture->pszModule );

   callRange = hb_astMacroTraceCallRange( pTrace );
   assert_true( callRange.start.nLine == 5 && callRange.end.nLine == 5 );
   assert_true( callRange.start.nColumn == 15 && callRange.end.nColumn == 20 );
}

static void test_snapshot_macro_trace_collection( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   HB_SIZE nTraceCount = hb_astTokenStreamMacroTraceCount( pFixture->pSnapshot );
   const void * pTrace;
   HB_AST_SOURCE_RANGE callRange;

   assert_int_equal( ( int ) nTraceCount, 1 );

   pTrace = hb_astTokenStreamMacroTrace( pFixture->pSnapshot, 0 );
   assert_non_null( pTrace );
   assert_true( hb_astMacroTraceId( pTrace ) != HB_SIZE_MAX );
   assert_int_equal( ( int ) hb_astMacroTraceDepth( pTrace ), 1 );
   assert_non_null( hb_astMacroTraceName( pTrace ) );
   assert_string_equal( hb_astMacroTraceName( pTrace ), "VALUE" );
   assert_null( hb_astMacroTraceParent( pTrace ) );

   callRange = hb_astMacroTraceCallRange( pTrace );
   assert_true( callRange.start.nLine == 5 && callRange.end.nLine == 5 );
   assert_true( callRange.start.nColumn == 15 && callRange.end.nColumn == 20 );
}

static void test_macro_graph_json_serialization( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   HB_SIZE nJsonLen = 0;
   char * pszJson = hb_astTokenStreamSerializeMacrosJson( pFixture->pSnapshot, &nJsonLen );

   assert_non_null( pszJson );
   assert_true( nJsonLen > 0 );
   hb_astAssertContains( pszJson, "\"expansions\"" );
   hb_astAssertContains( pszJson, "\"macro_name\":\"VALUE\"" );
   hb_astAssertContains( pszJson, "\"expansion_id\":0" );
   hb_astAssertContains( pszJson, "\"call_module\":\"tests/ast/fixture_demo.prg\"" );

   hb_astTokenStreamSerializeMacrosJsonFree( pszJson );
}

static void test_macro_graph_json_file_roundtrip( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   FILE * pFile;
   char buffer[ 512 ];
   size_t nRead;

   assert_true( hb_astTokenStreamWriteMacrosJson( pFixture->pSnapshot, HB_AST_MACRO_JSON_PATH ) );

   pFile = fopen( HB_AST_MACRO_JSON_PATH, "rb" );
   assert_non_null( pFile );

   nRead = fread( buffer, 1, sizeof( buffer ) - 1, pFile );
   buffer[ nRead ] = '\0';
   fclose( pFile );

   hb_astAssertContains( buffer, "\"expansions\"" );
   hb_astAssertContains( buffer, "\"macro_name\":\"VALUE\"" );
}

static void test_snapshot_json_serialization( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   HB_SIZE nJsonLen = 0;
   char * pszJson = hb_astTokenStreamSerializeSnapshotJson( pFixture->pSnapshot,
                                                            pFixture->pszModule,
                                                            &nJsonLen );

   assert_non_null( pszJson );
   assert_true( nJsonLen > 0 );
   hb_astAssertContains( pszJson, "\"nodes\"" );
   hb_astAssertContains( pszJson, "\"token_stream\"" );
   hb_astAssertContains( pszJson, "\"ProcDecl\"" );
   hb_astAssertContains( pszJson, "\"FunctionDecl\"" );

   hb_astTokenStreamSerializeSnapshotJsonFree( pszJson );
}

static void test_snapshot_json_file_roundtrip( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   FILE * pSnap;
   long nSnapSize;
   char * pszBuffer = NULL;

   assert_true( hb_astTokenStreamWriteSnapshotJson( pFixture->pSnapshot,
                                                   pFixture->pszModule,
                                                   HB_AST_SNAPSHOT_JSON_PATH ) );

   pSnap = fopen( HB_AST_SNAPSHOT_JSON_PATH, "rb" );
   assert_non_null( pSnap );

   assert_int_equal( fseek( pSnap, 0, SEEK_END ), 0 );
   nSnapSize = ftell( pSnap );
   assert_true( nSnapSize > 0 && nSnapSize < 65536 );
   assert_int_equal( fseek( pSnap, 0, SEEK_SET ), 0 );

   pszBuffer = ( char * ) malloc( ( size_t ) nSnapSize + 1 );
   assert_non_null( pszBuffer );

   if( pszBuffer )
   {
      size_t nRead = fread( pszBuffer, 1, ( size_t ) nSnapSize, pSnap );
      pszBuffer[ nRead ] = '\0';

      hb_astAssertContains( pszBuffer, "\"token_stream\"" );
      hb_astAssertContains( pszBuffer, "\"nodes\"" );
      hb_astAssertContains( pszBuffer, "\"FunctionDecl\"" );

      free( pszBuffer );
   }

   fclose( pSnap );
}

static void test_symbols_json_serialization( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   HB_SIZE nJsonLen = 0;
   char * pszJson = hb_astTokenStreamSerializeSymbolsJson( pFixture->pSnapshot,
                                                           pFixture->pszModule,
                                                           &nJsonLen );

   assert_non_null( pszJson );
   assert_true( nJsonLen > 0 );
   hb_astAssertContains( pszJson, "\"symbol_id\"" );
   hb_astAssertContains( pszJson, "\"name\":\"Demo\"" );

   hb_astTokenStreamSerializeSymbolsJsonFree( pszJson );
}

static void test_macro_graph_cbor_serialization( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   HB_SIZE nCborLen = 0;
   HB_BYTE * pCbor = hb_astTokenStreamSerializeMacrosCbor( pFixture->pSnapshot, &nCborLen );

   assert_non_null( pCbor );
   assert_true( nCborLen > 0 );
   assert_true( ( pCbor[ 0 ] & 0xE0 ) == 0xA0 );

   hb_astTokenStreamSerializeMacrosCborFree( pCbor );
}

static void test_snapshot_cbor_serialization( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   HB_SIZE nCborLen = 0;
   HB_BYTE * pCbor = hb_astTokenStreamSerializeSnapshotCbor( pFixture->pSnapshot,
                                                             pFixture->pszModule,
                                                             &nCborLen );

   assert_non_null( pCbor );
   assert_true( nCborLen > 0 );
   assert_true( ( pCbor[ 0 ] & 0xE0 ) == 0xA0 );

   hb_astTokenStreamSerializeSnapshotCborFree( pCbor );
}

static void test_symbols_cbor_serialization( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   HB_SIZE nCborLen = 0;
   HB_BYTE * pCbor = hb_astTokenStreamSerializeSymbolsCbor( pFixture->pSnapshot,
                                                            pFixture->pszModule,
                                                            &nCborLen );

   assert_non_null( pCbor );
   assert_true( nCborLen > 0 );
   assert_true( ( pCbor[ 0 ] & 0xE0 ) == 0x80 );

   hb_astTokenStreamSerializeSymbolsCborFree( pCbor );
}

static void test_macro_graph_cbor_file_roundtrip( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   FILE * pFile;
   int c;

   assert_true( hb_astTokenStreamWriteMacrosCbor( pFixture->pSnapshot, HB_AST_MACRO_CBOR_PATH ) );

   pFile = fopen( HB_AST_MACRO_CBOR_PATH, "rb" );
   assert_non_null( pFile );

   c = fgetc( pFile );
   assert_true( c != EOF );

   fclose( pFile );
}

static void test_snapshot_cbor_file_roundtrip( void ** state )
{
   const HB_AST_SNAPSHOT_FIXTURE * pFixture = ( const HB_AST_SNAPSHOT_FIXTURE * ) *state;
   FILE * pFile;
   int c;

   assert_true( hb_astTokenStreamWriteSnapshotCbor( pFixture->pSnapshot,
                                                   pFixture->pszModule,
                                                   HB_AST_SNAPSHOT_CBOR_PATH ) );

   pFile = fopen( HB_AST_SNAPSHOT_CBOR_PATH, "rb" );
   assert_non_null( pFile );

   c = fgetc( pFile );
   assert_true( c != EOF );

   fclose( pFile );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test_setup_teardown( test_snapshot_macro_literal_metadata,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_snapshot_macro_trace_collection,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_macro_graph_json_serialization,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_macro_graph_json_file_roundtrip,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_snapshot_json_serialization,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_snapshot_json_file_roundtrip,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_symbols_json_serialization,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_macro_graph_cbor_serialization,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_snapshot_cbor_serialization,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_symbols_cbor_serialization,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_macro_graph_cbor_file_roundtrip,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
      cmocka_unit_test_setup_teardown( test_snapshot_cbor_file_roundtrip,
                                       hb_astSnapshotSetup,
                                       hb_astSnapshotTeardown ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
