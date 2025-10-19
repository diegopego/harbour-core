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

static void hb_astCompileFixture( void ** state )
{
   const char * argv[] =
   {
      "hb_comp",
      "--ast-trace",
      "--ast-trace-dump=-",
      "tests/ast/fixture_demo.prg"
   };
   char * actualRaw;
   char * actual;
   char * jsonStart;
   char * expected;
   FILE * capture;
   int savedStdout;

   HB_SYMBOL_UNUSED( state );

   remove( "tests/ast/fixture_demo.c" );
   remove( "fixture_demo.c" );

   capture = tmpfile();
   assert_non_null( capture );
   fflush( stdout );
   savedStdout = HB_DUP( fileno( stdout ) );
   assert_true( savedStdout >= 0 );
   assert_true( HB_DUP2( fileno( capture ), fileno( stdout ) ) >= 0 );

   {
      int iStatus = hb_compMainExtModule( HB_SIZEOFARRAY( argv ), argv,
                                          NULL, NULL,
                                          NULL, NULL, 0,
                                          NULL, NULL, NULL, NULL, NULL );

      if( iStatus != EXIT_SUCCESS )
      {
         char * dump;

         fflush( stdout );
         assert_true( HB_DUP2( savedStdout, fileno( stdout ) ) >= 0 );
         HB_CLOSE( savedStdout );
         dump = hb_astReadStream( capture );
         fprintf( stderr, "%s", dump );
         free( dump );
         fail_msg( "hb_compMainExtModule() returned %d", iStatus );
      }
   }

   fflush( stdout );
   assert_true( HB_DUP2( savedStdout, fileno( stdout ) ) >= 0 );
   HB_CLOSE( savedStdout );

   actualRaw = hb_astReadStream( capture );
   fclose( capture );

   jsonStart = strchr( actualRaw, '{' );
   assert_non_null( jsonStart );
   actual = ( char * ) malloc( strlen( jsonStart ) + 1 );
   assert_non_null( actual );
   memcpy( actual, jsonStart, strlen( jsonStart ) + 1 );
   free( actualRaw );

   expected = hb_astLoadFile( "tests/ast/fixtures/fixture_demo.ast.json" );
   assert_string_equal( actual, expected );

   free( expected );
   free( actual );

   remove( "tests/ast/fixture_demo.c" );
   remove( "fixture_demo.c" );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test( hb_astCompileFixture ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
