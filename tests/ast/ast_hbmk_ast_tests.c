#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "hbapi.h"
#include "hbcomp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined( _WIN32 )
   #include <io.h>
   #define HB_DUP   _dup
   #define HB_DUP2  _dup2
   #define HB_CLOSE _close
#else
   #include <unistd.h>
   #define HB_DUP   dup
   #define HB_DUP2  dup2
   #define HB_CLOSE close
#endif

static char * hb_astLoadFile( const char * path )
{
   FILE * fp = fopen( path, "rb" );
   size_t nLen;
   char * buffer;

   assert_non_null( fp );
   assert_return_code( fseek( fp, 0, SEEK_END ), 0 );
   nLen = ( size_t ) ftell( fp );
   rewind( fp );

   buffer = ( char * ) malloc( nLen + 1 );
   assert_non_null( buffer );
   if( nLen )
   {
      size_t nRead = fread( buffer, 1, nLen, fp );
      assert_int_equal( nRead, nLen );
   }
   buffer[ nLen ] = '\0';
   fclose( fp );
   return buffer;
}

static char * hb_astReadStream( FILE * fp )
{
   size_t nLen;
   char * buffer;

   assert_return_code( fseek( fp, 0, SEEK_END ), 0 );
   nLen = ( size_t ) ftell( fp );
   assert_return_code( fseek( fp, 0, SEEK_SET ), 0 );

   buffer = ( char * ) malloc( nLen + 1 );
   assert_non_null( buffer );
   if( nLen )
   {
      size_t nRead = fread( buffer, 1, nLen, fp );
      assert_int_equal( nRead, nLen );
   }
   buffer[ nLen ] = '\0';
   return buffer;
}

static void hb_astCleanupArtifacts( const char * prgPath )
{
   char base[256];
   char * pExt;
   const char * pszBase = strrchr( prgPath, '/' );
   char staged[512];
   char local[256];

   if( pszBase )
      pszBase++;
   else
      pszBase = prgPath;

   hb_strncpy( base, pszBase, sizeof( base ) - 1 );
   pExt = strrchr( base, '.' );
   if( pExt )
      *pExt = '\0';

   hb_snprintf( local, sizeof( local ), "%s.c", base );
   hb_snprintf( staged, sizeof( staged ), "tests/ast/%s.c", base );

   remove( local );
   remove( staged );
}

static char * hb_astCaptureFixtureDump( const char * prgPath, HB_BOOL fSingleModule )
{
   const char * argv[6];
   size_t argc = 0;
   char * actualRaw;
   char * actual;
   char * jsonStart;
   FILE * capture;
   int savedStdout;
   int iStatus;

   argv[ argc++ ] = "hb_comp";
   if( fSingleModule )
      argv[ argc++ ] = "-m";
   argv[ argc++ ] = "-iinclude";
   argv[ argc++ ] = "--ast-trace";
   argv[ argc++ ] = "--ast-trace-dump=-";
   argv[ argc++ ] = prgPath;

   hb_astCleanupArtifacts( prgPath );

   capture = tmpfile();
   assert_non_null( capture );
   fflush( stdout );
   savedStdout = HB_DUP( fileno( stdout ) );
   assert_true( savedStdout >= 0 );
   assert_true( HB_DUP2( fileno( capture ), fileno( stdout ) ) >= 0 );

   iStatus = hb_compMainExtModule( ( int ) argc, argv,
                                   NULL, NULL,
                                   NULL, NULL, 0,
                                   NULL, NULL, NULL, NULL, NULL );

   fflush( stdout );
   assert_true( HB_DUP2( savedStdout, fileno( stdout ) ) >= 0 );
   HB_CLOSE( savedStdout );

   if( iStatus != EXIT_SUCCESS )
   {
      char * dump = hb_astReadStream( capture );

      fprintf( stderr, "%s", dump );
      free( dump );
      fclose( capture );
      hb_astCleanupArtifacts( prgPath );
      fail_msg( "hb_compMainExtModule() returned %d", iStatus );
   }

   actualRaw = hb_astReadStream( capture );
   fclose( capture );

   jsonStart = strchr( actualRaw, '{' );
   assert_non_null( jsonStart );
   actual = ( char * ) malloc( strlen( jsonStart ) + 1 );
   assert_non_null( actual );
   memcpy( actual, jsonStart, strlen( jsonStart ) + 1 );
   free( actualRaw );

   hb_astCleanupArtifacts( prgPath );

    /* expected will be handled by callers */
   return actual;
}

static void hb_astAssertFixtureDump( const char * prgPath, const char * expectedPath, HB_BOOL fSingleModule )
{
   char * actual = hb_astCaptureFixtureDump( prgPath, fSingleModule );
   char * expected;

   expected = hb_astLoadFile( expectedPath );
   assert_non_null( expected );
   assert_string_equal( actual, expected );
   free( expected );

   free( actual );
}

typedef struct
{
   const char * prgPath;
   const char * expectedJson;
}
HB_AST_FIXTURE;

static const HB_AST_FIXTURE s_cases[] =
{
   { "tests/ast/fixture_demo.prg", "tests/ast/fixtures/fixture_demo.ast.json" },
   { "tests/ast/fixture_blocks.prg", "tests/ast/fixtures/fixture_blocks.ast.json" },
   { "tests/ast/fixture_ppdirectives.prg", "tests/ast/fixtures/fixture_ppdirectives.ast.json" },
   { "tests/ast/fixture_statements.prg", "tests/ast/fixtures/fixture_statements.ast.json" },
   { "tests/ast/fixture_expressions.prg", "tests/ast/fixtures/fixture_expressions.ast.json" },
   { "tests/ast/fixture_includes.prg", "tests/ast/fixtures/fixture_includes.ast.json" },
   { "tests/ast/fixture_compat_clipper.prg", "tests/ast/fixtures/fixture_compat_clipper.ast.json" },
   { "tests/ast/fixture_compat_harbour.prg", "tests/ast/fixtures/fixture_compat_harbour.ast.json" },
   { "tests/ast/fixture_macro_expansion.prg", "tests/ast/fixtures/fixture_macro_expansion.ast.json" }
};

static void hb_astCompileFixture_default( void ** state )
{
   size_t i;

   HB_SYMBOL_UNUSED( state );

   for( i = 0; i < HB_SIZEOFARRAY( s_cases ); ++i )
   {
      hb_astAssertFixtureDump( s_cases[ i ].prgPath, s_cases[ i ].expectedJson, HB_FALSE );
   }
}

static void hb_astCompileFixture_single_module( void ** state )
{
   HB_SYMBOL_UNUSED( state );

   for( size_t i = 0; i < HB_SIZEOFARRAY( s_cases ); ++i )
   {
      hb_astAssertFixtureDump( s_cases[ i ].prgPath, s_cases[ i ].expectedJson, HB_TRUE );
   }
}

static void hb_astFixture_macro_expansion_metadata( void ** state )
{
   /* fixture_macro_expansion.prg expands TOP()->MIDDLE()->LEAF() so the literal 42
      arrives via nested macros; the JSON must surface non-zero expansion ancestry. */
   char * json;
   const char * value42;
   const char * cursor;
   unsigned long expansionId;
   unsigned long expansionParentId;
   unsigned long expansionDepth;

   HB_SYMBOL_UNUSED( state );

   json = hb_astCaptureFixtureDump( "tests/ast/fixture_macro_expansion.prg", HB_FALSE );
   assert_non_null( json );

   value42 = strstr( json, "\"value\":\"42\"" );
   assert_non_null( value42 );

   cursor = strstr( value42, "\"expansionId\":" );
   assert_non_null( cursor );
   expansionId = strtoul( cursor + strlen( "\"expansionId\":" ), NULL, 10 );
   assert_true( expansionId > 0 );

   cursor = strstr( value42, "\"expansionParentId\":" );
   assert_non_null( cursor );
   expansionParentId = strtoul( cursor + strlen( "\"expansionParentId\":" ), NULL, 10 );
   assert_true( expansionParentId > 0 );

   cursor = strstr( value42, "\"expansionDepth\":" );
   assert_non_null( cursor );
   expansionDepth = strtoul( cursor + strlen( "\"expansionDepth\":" ), NULL, 10 );
   assert_true( expansionDepth > 0 );

   free( json );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test( hb_astCompileFixture_default ),
      cmocka_unit_test( hb_astCompileFixture_single_module ),
      cmocka_unit_test( hb_astFixture_macro_expansion_metadata ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
