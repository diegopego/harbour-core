#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
         if( pToken->module && ! pCapture->firstTokenModule )
         {
            size_t nModuleLen = strlen( pToken->module );
            char * pszModule = ( char * ) malloc( nModuleLen + 1 );

            assert_non_null( pszModule );
            memcpy( pszModule, pToken->module, nModuleLen + 1 );
            pCapture->firstTokenModule = pszModule;
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

static void compilebuf_generates_trace_events( void ** state )
{
   static const char * const s_szSource =
      "FUNCTION Demo()\n"
      "   LOCAL n := 1\n"
      "   RETURN n\n";
   static const char * const s_szModule = "compilebuf_fixture.prg";
   static const char * const s_szOutputTemp = "compilebuf_fixture.c";
   static const char * const s_szOutput = "tests/ast/compilebuf_fixture.c";
   const char * argv[] = { "hb_comp", "-q2", "--ast-trace" };
   HB_TRACE_CAPTURE capture;
   int iResult;

   HB_SYMBOL_UNUSED( state );

   memset( &capture, 0, sizeof( capture ) );

   remove( s_szOutput );
   remove( s_szOutputTemp );

   iResult = hb_compMainExtModule( HB_SIZEOFARRAY( argv ), argv,
                                   NULL, NULL,
                                   s_szModule, s_szSource, 0,
                                   NULL, NULL, NULL,
                                   hb_trace_capture_finish, &capture );
   assert_int_equal( iResult, EXIT_SUCCESS );
   assert_true( capture.tokenCount > 0 );
   assert_non_null( capture.tokenValues );
   assert_string_equal( capture.tokenValues[ 0 ], "FUNCTION" );
   assert_non_null( capture.firstTokenModule );
   assert_string_equal( capture.firstTokenModule, s_szModule );

   assert_int_equal( rename( s_szOutputTemp, s_szOutput ), 0 );

   {
      FILE * fp = fopen( s_szOutput, "r" );
      char buffer[ 64 ];

      assert_non_null( fp );
      assert_non_null( fgets( buffer, sizeof( buffer ), fp ) );
      fclose( fp );
   }

   if( capture.nodeCount > 0 )
   {
      assert_non_null( capture.nodeKinds );
      assert_int_equal( capture.nodeKinds[ 0 ], HB_COMP_AST_NODE_FUNCTION );
      assert_int_equal( capture.nodePhases[ 0 ], HB_COMP_AST_NODE_EVENT_ENTER );
   }

   hb_trace_capture_release( &capture );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test( compilebuf_generates_trace_events ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
