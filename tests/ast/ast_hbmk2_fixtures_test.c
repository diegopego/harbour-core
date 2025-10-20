#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <glob.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

static void hbmk2_verify_command( const char * pszCommand )
{
   FILE * pPipe;
   char szLine[ 512 ];
   char * pszOutput = NULL;
   size_t nOutput = 0;
   bool fSawWarning = false;
   int iStatus;
   int iExitCode = -1;

   assert_non_null( pszCommand );

   pPipe = popen( pszCommand, "r" );
   assert_non_null( pPipe );

   while( fgets( szLine, sizeof( szLine ), pPipe ) != NULL )
   {
      size_t nLen = strlen( szLine );
      char * pszNew;

      if( strstr( szLine, "Warning W" ) != NULL )
         fSawWarning = true;

      pszNew = ( char * ) realloc( pszOutput, nOutput + nLen + 1 );
      assert_non_null( pszNew );
      pszOutput = pszNew;
      memcpy( pszOutput + nOutput, szLine, nLen );
      nOutput += nLen;
      pszOutput[ nOutput ] = '\0';
   }

   iStatus = pclose( pPipe );

#if defined( WIFEXITED ) && defined( WEXITSTATUS )
   if( WIFEXITED( iStatus ) )
      iExitCode = WEXITSTATUS( iStatus );
   else
      iExitCode = -1;
#else
   iExitCode = iStatus;
#endif

   if( pszOutput == NULL )
   {
      pszOutput = ( char * ) calloc( 1, sizeof( char ) );
      assert_non_null( pszOutput );
   }

   if( iExitCode != 0 || fSawWarning )
   {
      fail_msg( "%s failed (exit=%d, warnings=%d)\n%s",
                pszCommand, iExitCode, fSawWarning ? 1 : 0, pszOutput ? pszOutput : "" );
   }

   free( pszOutput );
}

static void hbmk2_verify_fixture( const char * pszFixture )
{
   char szCommand[ 512 ];

   assert_non_null( pszFixture );

   assert_true( snprintf( szCommand, sizeof( szCommand ),
                          "hbmk2 -w3 %s 2>&1", pszFixture ) > 0 );

   hbmk2_verify_command( szCommand );
}

static void hbmk2_compile_fixtures( void ** state )
{
   const char * const szPatterns[] =
   {
      "tests/ast/*.prg",
      "tests/ast/*/*.prg",
      "tests/ast/*/*/*.prg"
   };
   glob_t globbuf;
   size_t i;
   int rc;

   ( void ) state;

   memset( &globbuf, 0, sizeof( globbuf ) );

   for( i = 0; i < sizeof( szPatterns ) / sizeof( szPatterns[ 0 ] ); ++i )
   {
      rc = glob( szPatterns[ i ],
                 i == 0 ? 0 : GLOB_APPEND,
                 NULL,
                 &globbuf );
      assert_true( rc == 0 || rc == GLOB_NOMATCH );
   }

   assert_true( globbuf.gl_pathc > 0 );

   for( i = 0; i < globbuf.gl_pathc; ++i )
   {
      if( strstr( globbuf.gl_pathv[ i ], "/ref_project/" ) != NULL )
         continue;
      hbmk2_verify_fixture( globbuf.gl_pathv[ i ] );
   }

   globfree( &globbuf );
}

static void hbmk2_compile_ref_project( void ** state )
{
   ( void ) state;

   hbmk2_verify_command( "hbmk2 -w3 tests/ast/ref_project/main.prg tests/ast/ref_project/support.prg 2>&1" );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test( hbmk2_compile_fixtures ),
      cmocka_unit_test( hbmk2_compile_ref_project ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
