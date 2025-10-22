#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "hbapi.h"
#include "hbcomp.h"
#include "hbasttrace.h"

typedef struct
{
   HB_SIZE tokenCount;
   char ** tokenValues;
   HB_SIZE nodeCount;
   HB_COMP_AST_NODE_KIND * nodeKinds;
   HB_COMP_AST_NODE_EVENT_TYPE * nodePhases;
   const char * expectedModule;
   bool sawExpectedModule;
   char * firstTokenModule;
} HB_TRACE_CAPTURE;

static void hb_trace_capture_finish( PHB_COMP pComp, void * cargo )
{
   HB_TRACE_CAPTURE * pCapture = ( HB_TRACE_CAPTURE * ) cargo;
   HB_SIZE i;

   assert_non_null( pComp );
   assert_non_null( pCapture );
   assert_true( hb_compAstTraceIsEnabled( pComp ) );

   pCapture->tokenCount = hb_compAstTraceTokenCount( pComp );
   if( pCapture->tokenCount > 0 )
   {
      pCapture->tokenValues = ( char ** ) calloc( pCapture->tokenCount, sizeof( char * ) );
      assert_non_null( pCapture->tokenValues );

      for( i = 0; i < pCapture->tokenCount; ++i )
      {
         const HB_COMP_AST_TRACE_TOKEN * pToken = hb_compAstTraceToken( pComp, i );
         assert_non_null( pToken );
         if( pToken->value )
         {
            size_t nLen = strlen( pToken->value );
            char * pszCopy = ( char * ) malloc( nLen + 1 );

            assert_non_null( pszCopy );
            memcpy( pszCopy, pToken->value, nLen + 1 );
            pCapture->tokenValues[ i ] = pszCopy;
         }
         if( pToken->module )
         {
            if( pCapture->expectedModule &&
                strcmp( pToken->module, pCapture->expectedModule ) == 0 )
            {
               pCapture->sawExpectedModule = true;
            }
            if( ! pCapture->firstTokenModule )
            {
               size_t nModuleLen = strlen( pToken->module );
               char * pszModule = ( char * ) malloc( nModuleLen + 1 );

               assert_non_null( pszModule );
               memcpy( pszModule, pToken->module, nModuleLen + 1 );
               pCapture->firstTokenModule = pszModule;
            }
         }
      }
   }

   pCapture->nodeCount = hb_compAstTraceNodeCount( pComp );
   if( pCapture->nodeCount > 0 )
   {
      pCapture->nodeKinds = ( HB_COMP_AST_NODE_KIND * ) calloc( pCapture->nodeCount, sizeof( HB_COMP_AST_NODE_KIND ) );
      pCapture->nodePhases = ( HB_COMP_AST_NODE_EVENT_TYPE * ) calloc( pCapture->nodeCount, sizeof( HB_COMP_AST_NODE_EVENT_TYPE ) );
      assert_non_null( pCapture->nodeKinds );
      assert_non_null( pCapture->nodePhases );

      for( i = 0; i < pCapture->nodeCount; ++i )
      {
         const HB_COMP_AST_TRACE_NODE_EVENT * pNode = hb_compAstTraceNode( pComp, i );
         assert_non_null( pNode );
         pCapture->nodeKinds[ i ] = pNode->kind;
         pCapture->nodePhases[ i ] = pNode->phase;
      }
   }
}

static void hb_trace_capture_release( HB_TRACE_CAPTURE * pCapture )
{
   HB_SIZE i;

   if( pCapture->tokenValues )
   {
      for( i = 0; i < pCapture->tokenCount; ++i )
      {
         free( pCapture->tokenValues[ i ] );
      }
      free( pCapture->tokenValues );
   }
   free( pCapture->nodeKinds );
   free( pCapture->nodePhases );
   free( pCapture->firstTokenModule );
   memset( pCapture, 0, sizeof( *pCapture ) );
}

typedef struct
{
   const char * module;
   const char * source;
   const char * sourcePath;
   const char * outputTemp;
   const char * outputFinal;
   const char * requiredToken;
   HB_COMP_AST_NODE_KIND expectedNodeKind;
}
HB_COMPILEBUF_CASE;

static const HB_COMPILEBUF_CASE s_cases[] =
{
   {
      "compilebuf_fixture.prg",
      "FUNCTION Demo()\n"
      "   LOCAL n := 1\n"
      "   RETURN n\n",
      NULL,
      "compilebuf_fixture.c",
      "tests/ast/compilebuf_fixture.c",
      "FUNCTION",
      HB_COMP_AST_NODE_FUNCTION
   },
   {
      "compilebuf_compat_clipper.prg",
      "#pragma -w3\n"
      "#pragma -kh-\n"
      "#pragma -km+\n"
      "#pragma -ko-\n"
      "#include \"error.ch\"\n"
      "#include \"tests/ast/fixture_compat_common.ch\"\n"
      "FUNCTION CompileBufCompatClipper()\n"
      "   LOCAL aLog := {}\n"
      "   LOCAL oBreak := NIL\n"
      "   FixtureCompatLog( aLog, \"compat:CLIPPER\" )\n"
      "   BEGIN SEQUENCE WITH {|oErr| FixtureCompatLog( aLog, \"handler:\" + IIf( oErr == NIL, \"none\", oErr:Description ) ) }\n"
      "      APPLY_NESTED_MACROS( 5 )\n"
      "      oBreak := ErrorNew()\n"
      "      oBreak:Description := \"compilebuf\"\n"
      "      oBreak:GenCode := 9001\n"
      "      BREAK oBreak\n"
      "   RECOVER USING oErr\n"
      "      FixtureCompatLog( aLog, \"recover:\" + IIf( oErr == NIL, \"none\", oErr:Description ) )\n"
      "   ENDSEQUENCE\n"
      "   FixtureCompatLog( aLog, \"compat:CLIPPER:end\" )\n"
      "   RETURN Len( aLog )\n",
      NULL,
      "compilebuf_compat_clipper.c",
      "tests/ast/compilebuf_compat_clipper.c",
      "FUNCTION",
      HB_COMP_AST_NODE_FUNCTION
   },
   {
      "compilebuf_compat_harbour.prg",
      "#pragma -w3\n"
      "#pragma -kh+\n"
      "#pragma -km-\n"
      "#pragma -ko+\n"
      "#include \"error.ch\"\n"
      "#include \"tests/ast/fixture_compat_common.ch\"\n"
      "FUNCTION CompileBufCompatHarbour()\n"
      "   LOCAL aLog := {}\n"
      "   LOCAL oInner := NIL\n"
      "   FixtureCompatLog( aLog, \"compat:HARBOUR\" )\n"
      "   BEGIN SEQUENCE WITH {|oErr| FixtureCompatLog( aLog, \"handler:\" + IIf( oErr == NIL, \"none\", oErr:Description ) ) }\n"
      "      FixtureCompatLog( aLog, \"nested:enter\" )\n"
      "      oInner := ErrorNew()\n"
      "      oInner:Description := \"compilebuf harbour\"\n"
      "      oInner:GenCode := 9101\n"
      "      BREAK oInner\n"
      "   RECOVER USING oErr\n"
      "      FixtureCompatLog( aLog, \"nested:recover:\" + IIf( oErr == NIL, \"none\", oErr:Description ) )\n"
      "   ENDSEQUENCE\n"
      "   FixtureCompatLog( aLog, \"compat:HARBOUR:end\" )\n"
      "   RETURN Len( aLog )\n",
      NULL,
      "compilebuf_compat_harbour.c",
      "tests/ast/compilebuf_compat_harbour.c",
      "FUNCTION",
      HB_COMP_AST_NODE_FUNCTION
   },
   {
      "compilebuf_init_exit.prg",
      "INIT PROCEDURE CompileBufInit()\n"
      "   RETURN\n"
      "\n"
      "EXIT PROCEDURE CompileBufExit()\n"
      "   RETURN\n",
      NULL,
      "compilebuf_init_exit.c",
      "tests/ast/compilebuf_init_exit.c",
      "INIT",
      HB_COMP_AST_NODE_FUNCTION_INIT
   },
   {
      "fixture_inline_real.prg",
      NULL,
      "tests/ast/fixture_inline_real.prg",
      "fixture_inline_real.c",
      "tests/ast/fixture_inline_real.c",
      NULL,
      HB_COMP_AST_NODE_INLINE
   }
};

static char * hb_read_text_file( const char * path )
{
   FILE * fp;
   long size;
   size_t readSize;
   char * buffer;

   assert_non_null( path );

   fp = fopen( path, "rb" );
   assert_non_null( fp );

   assert_int_equal( fseek( fp, 0, SEEK_END ), 0 );
   size = ftell( fp );
   assert_true( size >= 0 );
   assert_int_equal( fseek( fp, 0, SEEK_SET ), 0 );

   buffer = ( char * ) malloc( ( size_t ) size + 1 );
   assert_non_null( buffer );

   readSize = fread( buffer, 1, ( size_t ) size, fp );
   assert_int_equal( readSize, ( size_t ) size );
   buffer[ size ] = '\0';

   fclose( fp );
   return buffer;
}

static void compilebuf_expect_trace( const HB_COMPILEBUF_CASE * pCase,
                                     size_t argc,
                                     const char * const argv[] )
{
   HB_TRACE_CAPTURE capture;
   bool foundToken = false;
   size_t i;
   int iResult;
   const char * source = pCase->source;
   char * loadedSource = NULL;

   memset( &capture, 0, sizeof( capture ) );
   capture.expectedModule = pCase->module;
   capture.sawExpectedModule = false;

   remove( pCase->outputFinal );
   remove( pCase->outputTemp );

   if( !source && pCase->sourcePath )
   {
      loadedSource = hb_read_text_file( pCase->sourcePath );
      source = loadedSource;
   }

   assert_non_null( source );

   iResult = hb_compMainExtModule( ( int ) argc, argv,
                                   NULL, NULL,
                                   pCase->module, source, 0,
                                   NULL, NULL, NULL,
                                   hb_trace_capture_finish, &capture );
   assert_int_equal( iResult, EXIT_SUCCESS );
   assert_true( capture.tokenCount > 0 );
   assert_non_null( capture.tokenValues );
   if( pCase->requiredToken )
   {
      for( i = 0; i < capture.tokenCount; ++i )
      {
         if( capture.tokenValues[ i ] &&
             strcmp( capture.tokenValues[ i ], pCase->requiredToken ) == 0 )
         {
            foundToken = true;
            break;
         }
      }
      assert_true( foundToken );
   }
   assert_true( capture.sawExpectedModule );

   if( rename( pCase->outputTemp, pCase->outputFinal ) != 0 )
   {
      int err = errno;

      fail_msg( "rename(%s,%s) failed: %s",
                pCase->outputTemp, pCase->outputFinal,
                strerror( err ) );
   }

   {
      FILE * fp = fopen( pCase->outputFinal, "r" );
      char buffer[ 64 ];

      assert_non_null( fp );
      assert_non_null( fgets( buffer, sizeof( buffer ), fp ) );
      fclose( fp );
   }

   if( capture.nodeCount > 0 )
   {
      assert_non_null( capture.nodeKinds );
      if( pCase->expectedNodeKind != 0 )
      {
         bool sawExpectedKind = false;

         for( i = 0; i < capture.nodeCount; ++i )
         {
            if( capture.nodeKinds[ i ] == pCase->expectedNodeKind &&
                capture.nodePhases[ i ] == HB_COMP_AST_NODE_EVENT_ENTER )
            {
               sawExpectedKind = true;
               break;
            }
         }
         assert_true( sawExpectedKind );
      }
   }

   hb_trace_capture_release( &capture );
   free( loadedSource );
}

static void compilebuf_generates_trace_events( void ** state )
{
   const char * argv[] = { "hb_comp", "-q2", "-iinclude", "--ast-trace" };
   size_t i;

   HB_SYMBOL_UNUSED( state );

   for( i = 0; i < HB_SIZEOFARRAY( s_cases ); ++i )
   {
      compilebuf_expect_trace( &s_cases[ i ], HB_SIZEOFARRAY( argv ), argv );
   }
}

static void compilebuf_generates_trace_events_single_module( void ** state )
{
   const char * argv[] = { "hb_comp", "-q2", "-m", "-iinclude", "--ast-trace" };
   size_t i;

   HB_SYMBOL_UNUSED( state );

   for( i = 0; i < HB_SIZEOFARRAY( s_cases ); ++i )
   {
      compilebuf_expect_trace( &s_cases[ i ], HB_SIZEOFARRAY( argv ), argv );
   }
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test( compilebuf_generates_trace_events ),
      cmocka_unit_test( compilebuf_generates_trace_events_single_module ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
