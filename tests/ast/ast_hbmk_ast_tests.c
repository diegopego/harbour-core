#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "hbapi.h"
#include "hbcomp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#if defined( _WIN32 )
   #include <direct.h>
   #define HB_GETCWD _getcwd
#else
   #include <unistd.h>
   #define HB_GETCWD getcwd
#endif

#ifndef PATH_MAX
   #define PATH_MAX 4096
#endif

static const char * hb_astRootPrefix( void )
{
   static char prefix[ PATH_MAX * 2 ];
   static int initialized = 0;

   if( !initialized )
   {
      if( HB_GETCWD( prefix, sizeof( prefix ) - 2 ) != NULL )
      {
         size_t len;

         for( len = 0; prefix[ len ] != '\0'; ++len )
         {
            if( prefix[ len ] == '\\' )
               prefix[ len ] = '/';
         }

         len = strlen( prefix );
         if( len > 0 && prefix[ len - 1 ] != '/' )
         {
            prefix[ len ] = '/';
            prefix[ len + 1 ] = '\0';
         }
      }
      else
         prefix[ 0 ] = '\0';

      initialized = 1;
   }

   return prefix;
}

static void hb_astStripRootPrefix( char * text )
{
   const char * prefix = hb_astRootPrefix();
   size_t prefixLen = strlen( prefix );
   char * pos;

   if( prefixLen == 0 )
      return;

   while( ( pos = strstr( text, prefix ) ) != NULL )
   {
      memmove( pos, pos + prefixLen, strlen( pos + prefixLen ) + 1 );
   }
}

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

static char * hb_astCaptureFixtureDump( const char * prgPath, HB_BOOL fSingleModule )
{
   char command[ PATH_MAX * 2 ];
   FILE * pipe;
   char buffer[ 512 ];
   char * output = NULL;
   size_t total = 0;
   int status;
   char * jsonStart;
   char * actual;

   if( fSingleModule )
      snprintf( command, sizeof( command ),
                "bin/linux/gcc/hbmk2 -w3 -q -incpath=include "
                "-prgflag=-m -prgflag=--ast-trace -prgflag=--ast-trace-dump=- \"%s\" 2>&1",
                prgPath );
   else
      snprintf( command, sizeof( command ),
                "bin/linux/gcc/hbmk2 -w3 -q -incpath=include "
                "-prgflag=--ast-trace -prgflag=--ast-trace-dump=- \"%s\" 2>&1",
                prgPath );

   pipe = popen( command, "r" );
   assert_non_null( pipe );

   while( fgets( buffer, sizeof( buffer ), pipe ) != NULL )
   {
      size_t len = strlen( buffer );
      char * tmp = ( char * ) realloc( output, total + len + 1 );

      assert_non_null( tmp );
      output = tmp;
      memcpy( output + total, buffer, len );
      total += len;
      output[ total ] = '\0';
   }

   status = pclose( pipe );
#if defined( WIFEXITED ) && defined( WEXITSTATUS )
   assert_true( WIFEXITED( status ) );
   assert_int_equal( WEXITSTATUS( status ), 0 );
#else
   assert_int_equal( status, 0 );
#endif

   assert_non_null( output );

   jsonStart = strchr( output, '{' );
   assert_non_null( jsonStart );
   actual = ( char * ) malloc( strlen( jsonStart ) + 1 );
   assert_non_null( actual );
   memcpy( actual, jsonStart, strlen( jsonStart ) + 1 );

   hb_astStripRootPrefix( actual );

   free( output );

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
   { "tests/ast/fixture_macro_expansion.prg", "tests/ast/fixtures/fixture_macro_expansion.ast.json" },
   { "tests/ast/fixture_inline_real.prg", "tests/ast/fixtures/fixture_inline_real.ast.json" }
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
