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

static void hb_astAssertFixtureDump( size_t argc, const char * const argv[] )
{
   char * actual;
   char * jsonStart;
   char * actualRaw;
   char * expected;
   FILE * capture;
   int savedStdout;
   int iStatus;

   remove( "tests/ast/fixture_demo.c" );
   remove( "fixture_demo.c" );

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

   remove( "tests/ast/fixture_demo.c" );
   remove( "fixture_demo.c" );

   expected = hb_astLoadFile( "tests/ast/fixtures/fixture_demo.ast.json" );
   assert_non_null( expected );
   assert_string_equal( actual, expected );
   free( expected );

   free( actual );
}

static void hb_astCompileFixture_default( void ** state )
{
   const char * argv[] =
   {
      "hb_comp",
      "--ast-trace",
      "--ast-trace-dump=-",
      "tests/ast/fixture_demo.prg"
   };

   HB_SYMBOL_UNUSED( state );

   hb_astAssertFixtureDump( HB_SIZEOFARRAY( argv ), argv );
}

static void hb_astCompileFixture_single_module( void ** state )
{
   const char * argv[] =
   {
      "hb_comp",
      "-m",
      "--ast-trace",
      "--ast-trace-dump=-",
      "tests/ast/fixture_demo.prg"
   };

   HB_SYMBOL_UNUSED( state );

   hb_astAssertFixtureDump( HB_SIZEOFARRAY( argv ), argv );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test( hb_astCompileFixture_default ),
      cmocka_unit_test( hb_astCompileFixture_single_module ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
