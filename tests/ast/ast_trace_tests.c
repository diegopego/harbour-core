#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#include "hbapi.h"
#include "hbcomp.h"
#include "hbasttrace.h"
#include "hbpp.h"

static void traceinfo_lifetime_balances( void ** state )
{
   PHB_COMP pComp;
   int i;

   HB_SYMBOL_UNUSED( state );

   pComp = hb_comp_new();
   assert_non_null( pComp );

   assert_false( hb_compAstTraceIsEnabled( pComp ) );
   hb_compAstTraceSetEnabled( pComp, HB_TRUE );
   assert_true( hb_compAstTraceIsEnabled( pComp ) );

   for( i = 0; i < 64; ++i )
   {
      PHB_PP_TRACEINFO pInfo = ( PHB_PP_TRACEINFO ) hb_xgrabz( sizeof( HB_PP_TRACEINFO ) );

      pInfo->nRefCount = 1;

      hb_compAstTraceRetainInfo( pComp, pInfo );
      hb_compAstTraceRetainInfo( pComp, pInfo );
      assert_int_equal( hb_compAstTraceOutstandingTraceinfo( pComp ), 2 );

      hb_compAstTraceReleaseInfo( pComp, pInfo );
      hb_compAstTraceReleaseInfo( pComp, pInfo );
      hb_pp_traceinfoRelease( pInfo );

      assert_int_equal( hb_compAstTraceOutstandingTraceinfo( pComp ), 0 );
   }

   hb_compAstTraceSetEnabled( pComp, HB_FALSE );
   assert_false( hb_compAstTraceIsEnabled( pComp ) );

   hb_comp_free( pComp );
}

static void disabled_state_blocks_events( void ** state )
{
   PHB_COMP pComp = hb_comp_new();
   HB_PP_TOKEN token;

   HB_SYMBOL_UNUSED( state );

   assert_non_null( pComp );

   hb_xmemset( &token, 0, sizeof( token ) );
   token.value = "ignored";
   token.len = 7;

   hb_compAstTraceSetEnabled( pComp, HB_FALSE );
   hb_compAstTraceClear( pComp );

   hb_compAstTracePublishToken( pComp, &token );
   hb_compAstTracePublishBoundary( pComp, ';', 0 );
   hb_compAstTraceNodeEnter( pComp, HB_COMP_AST_NODE_FUNCTION, NULL, 0 );
   hb_compAstTraceNodeLeave( pComp, HB_COMP_AST_NODE_FUNCTION, NULL );

   assert_int_equal( hb_compAstTraceTokenCount( pComp ), 0 );
   assert_int_equal( hb_compAstTraceBoundaryCount( pComp ), 0 );
   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 0 );

   hb_comp_free( pComp );
}

static void token_and_boundary_events_capture_metadata( void ** state )
{
   PHB_COMP pComp = hb_comp_new();
   HB_PP_TOKEN token;
   const HB_COMP_AST_TRACE_TOKEN * pCaptured;
   const HB_COMP_AST_TRACE_BOUNDARY * pBoundary;

   HB_SYMBOL_UNUSED( state );

   assert_non_null( pComp );

   hb_xmemset( &token, 0, sizeof( token ) );
   token.value = "PROCEDURE";
   token.len = 9;
   token.szModule = "trace_test.prg";
   token.iLine = 42;
   token.iColumn = 7;
   token.nOffset = 128;

   hb_compAstTraceSetEnabled( pComp, HB_TRUE );
   hb_compAstTraceClear( pComp );

   hb_compAstTracePublishToken( pComp, &token );
   hb_compAstTracePublishBoundary( pComp, ';', 1 );

   assert_int_equal( hb_compAstTraceTokenCount( pComp ), 1 );
   assert_int_equal( hb_compAstTraceBoundaryCount( pComp ), 1 );

   pCaptured = hb_compAstTraceToken( pComp, 0 );
   assert_non_null( pCaptured );
   assert_int_equal( pCaptured->sequence, 1 );
   assert_int_equal( pCaptured->id, 1 );
   assert_string_equal( pCaptured->value, "PROCEDURE" );
   assert_string_equal( pCaptured->module, "trace_test.prg" );
   assert_int_equal( pCaptured->line, 42 );
   assert_int_equal( pCaptured->column, 7 );
   assert_int_equal( pCaptured->offset, 128 );

   pBoundary = hb_compAstTraceBoundary( pComp, 0 );
   assert_non_null( pBoundary );
   assert_int_equal( pBoundary->sequence, 2 );
   assert_int_equal( pBoundary->tokenId, pCaptured->id );
   assert_int_equal( pBoundary->code, ';' );
   assert_int_equal( pBoundary->lexState, 1 );

   hb_comp_free( pComp );
}

static void node_events_pair_enter_and_leave( void ** state )
{
   PHB_COMP pComp = hb_comp_new();
   HB_PP_TOKEN token;
   HB_HFUNC func;
   const HB_COMP_AST_TRACE_NODE_EVENT * pEnter;
   const HB_COMP_AST_TRACE_NODE_EVENT * pLeave;

   HB_SYMBOL_UNUSED( state );

   assert_non_null( pComp );

   hb_xmemset( &token, 0, sizeof( token ) );
   token.value = "FUNCTION";
   token.len = 8;

   hb_xmemset( &func, 0, sizeof( func ) );
   func.szName = "Demo";

   hb_compAstTraceSetEnabled( pComp, HB_TRUE );
   hb_compAstTraceClear( pComp );

   hb_compAstTracePublishToken( pComp, &token );
   hb_compAstTraceNodeEnter( pComp, HB_COMP_AST_NODE_FUNCTION, &func,
                             hb_compAstTraceLastTokenId( pComp ) );
   hb_compAstTraceNodeLeave( pComp, HB_COMP_AST_NODE_FUNCTION, &func );

   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 2 );

   pEnter = hb_compAstTraceNode( pComp, 0 );
   pLeave = hb_compAstTraceNode( pComp, 1 );
   assert_non_null( pEnter );
   assert_non_null( pLeave );

   assert_int_equal( pEnter->kind, HB_COMP_AST_NODE_FUNCTION );
   assert_int_equal( pEnter->phase, HB_COMP_AST_NODE_EVENT_ENTER );
   assert_int_equal( pEnter->tokenId, hb_compAstTraceLastTokenId( pComp ) );
   assert_string_equal( pEnter->name, "Demo" );

   assert_int_equal( pLeave->kind, HB_COMP_AST_NODE_FUNCTION );
   assert_int_equal( pLeave->phase, HB_COMP_AST_NODE_EVENT_LEAVE );
   assert_int_equal( pLeave->id, pEnter->id );
   assert_string_equal( pLeave->name, "Demo" );

   hb_comp_free( pComp );
}

int main( void )
{
   const struct CMUnitTest tests[] = {
      cmocka_unit_test( traceinfo_lifetime_balances ),
      cmocka_unit_test( disabled_state_blocks_events ),
      cmocka_unit_test( token_and_boundary_events_capture_metadata ),
      cmocka_unit_test( node_events_pair_enter_and_leave ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
