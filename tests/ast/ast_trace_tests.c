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
   assert_true( hb_compAstTraceIsEnabled( pComp ) );

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

static void pp_events_capture_macro_traces( void ** state )
{
   PHB_COMP pComp = hb_comp_new();
   HB_PP_TRACE_EVENT event;
   const HB_COMP_AST_TRACE_PP_EVENT * pEventCaptured;

   HB_SYMBOL_UNUSED( state );

   assert_non_null( pComp );

   hb_compAstTraceSetEnabled( pComp, HB_TRUE );
   hb_compAstTraceClear( pComp );

   hb_xmemset( &event, 0, sizeof( event ) );
   event.szRuleKind = "macro";
   event.szMacroName = "DBG";
   event.szCallModule = "trace_test.prg";
   event.iCallLine = 5;
   event.iCallColumn = 3;
   event.iCallEndLine = 5;
   event.iCallEndColumn = 12;
   event.nCallOffset = 10;
   event.nCallEndOffset = 20;
   event.nExpansionId = 42;
   event.pszSource = "DBG(1)";
   event.pszResult = "?? 1";

   hb_compAstTracePublishPreprocessorEvent( pComp, &event );

   assert_int_equal( hb_compAstTracePpEventCount( pComp ), 1 );
   pEventCaptured = hb_compAstTracePpEvent( pComp, 0 );
   assert_non_null( pEventCaptured );
   assert_int_equal( pEventCaptured->sequence, 1 );
   assert_string_equal( pEventCaptured->ruleKind, "macro" );
   assert_string_equal( pEventCaptured->macroName, "DBG" );
   assert_string_equal( pEventCaptured->callModule, "trace_test.prg" );
   assert_int_equal( pEventCaptured->callLine, 5 );
   assert_int_equal( pEventCaptured->callColumn, 3 );
   assert_int_equal( pEventCaptured->callEndLine, 5 );
   assert_int_equal( pEventCaptured->callEndColumn, 12 );
   assert_int_equal( pEventCaptured->callOffset, 10 );
   assert_int_equal( pEventCaptured->callEndOffset, 20 );
   assert_int_equal( pEventCaptured->expansionId, 42 );
   assert_string_equal( pEventCaptured->source, "DBG(1)" );
   assert_string_equal( pEventCaptured->result, "?? 1" );

   hb_compAstTraceClear( pComp );
   assert_int_equal( hb_compAstTracePpEventCount( pComp ), 0 );

   hb_comp_free( pComp );
}

static void cli_toggle_controls_trace( void ** state )
{
   PHB_COMP pComp = hb_comp_new();
   const char * argvEnable[] = { "hb_comp", "--ast-trace" };
   const char * argvDisable[] = { "hb_comp", "--no-ast-trace" };

   HB_SYMBOL_UNUSED( state );

   assert_non_null( pComp );

   hb_compChkCommandLine( pComp, HB_SIZEOFARRAY( argvEnable ), argvEnable );
   assert_true( hb_compAstTraceIsEnabled( pComp ) );

   hb_compChkCommandLine( pComp, HB_SIZEOFARRAY( argvDisable ), argvDisable );
   assert_false( hb_compAstTraceIsEnabled( pComp ) );

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

static void expression_nodes_capture_reductions( void ** state )
{
   PHB_COMP pComp = hb_comp_new();

   HB_SYMBOL_UNUSED( state );

   assert_non_null( pComp );

   hb_compAstTraceSetEnabled( pComp, HB_TRUE );
   hb_compAstTraceClear( pComp );

#define ASSERT_EXPR_EVENT(nodeKind, label) \
   do { \
      hb_compAstTraceClear( pComp ); \
      HB_SIZE _traceId = hb_compAstTraceNodeEnterName( pComp, ( nodeKind ), ( label ), hb_compAstTraceLastTokenId( pComp ) ); \
      hb_compAstTraceNodeLeaveById( pComp, ( nodeKind ), _traceId ); \
      assert_int_equal( hb_compAstTraceNodeCount( pComp ), 2 ); \
      const HB_COMP_AST_TRACE_NODE_EVENT * _enter = hb_compAstTraceNode( pComp, 0 ); \
      const HB_COMP_AST_TRACE_NODE_EVENT * _leave = hb_compAstTraceNode( pComp, 1 ); \
      assert_non_null( _enter ); \
      assert_non_null( _leave ); \
      assert_true( _enter->kind == ( nodeKind ) ); \
      assert_string_equal( _enter->name, ( label ) ); \
      assert_true( _leave->phase == HB_COMP_AST_NODE_EVENT_LEAVE ); \
      assert_true( _leave->id == _enter->id ); \
   } while( 0 )

   ASSERT_EXPR_EVENT( HB_COMP_AST_NODE_EXPR_MATH, "+" );
   ASSERT_EXPR_EVENT( HB_COMP_AST_NODE_EXPR_BOOL, "AND" );
   ASSERT_EXPR_EVENT( HB_COMP_AST_NODE_EXPR_RELATION, ">=" );
   ASSERT_EXPR_EVENT( HB_COMP_AST_NODE_EXPR_ASSIGN, ":=" );
   ASSERT_EXPR_EVENT( HB_COMP_AST_NODE_EXPR_INPLACE, "+=" );
   ASSERT_EXPR_EVENT( HB_COMP_AST_NODE_EXPR_UNARY, "PRE++" );

#undef ASSERT_EXPR_EVENT

   hb_comp_free( pComp );
}

static void class_and_member_events_capture_names( void ** state )
{
   PHB_COMP pComp = hb_comp_new();
   HB_PP_TOKEN token;
   HB_HCLASS klass;
   HB_HDECLARED method;
   HB_HDECLARED data;
   const HB_COMP_AST_TRACE_NODE_EVENT * pEvent;

   HB_SYMBOL_UNUSED( state );

   assert_non_null( pComp );

   hb_compAstTraceSetEnabled( pComp, HB_TRUE );
   hb_compAstTraceClear( pComp );

   hb_xmemset( &token, 0, sizeof( token ) );
   token.value = "CLASS";
   token.len = 5;

   hb_compAstTracePublishToken( pComp, &token );

   hb_xmemset( &klass, 0, sizeof( klass ) );
   klass.szName = "SampleClass";

   hb_compAstTraceNodeEnter( pComp, HB_COMP_AST_NODE_CLASS, &klass,
                             hb_compAstTraceLastTokenId( pComp ) );
   hb_compAstTraceNodeLeave( pComp, HB_COMP_AST_NODE_CLASS, &klass );

   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 2 );
   pEvent = hb_compAstTraceNode( pComp, 0 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->kind, HB_COMP_AST_NODE_CLASS );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_ENTER );
   assert_string_equal( pEvent->name, "SampleClass" );

   pEvent = hb_compAstTraceNode( pComp, 1 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_LEAVE );
   assert_string_equal( pEvent->name, "SampleClass" );

   hb_compAstTraceClear( pComp );

   token.value = "METHOD";
   token.len = 6;
   hb_compAstTracePublishToken( pComp, &token );

   hb_xmemset( &method, 0, sizeof( method ) );
   method.szName = "MethodName";

   hb_compAstTraceNodeEnter( pComp, HB_COMP_AST_NODE_CLASS_METHOD, &method,
                             hb_compAstTraceLastTokenId( pComp ) );
   hb_compAstTraceNodeLeave( pComp, HB_COMP_AST_NODE_CLASS_METHOD, &method );

   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 2 );
   pEvent = hb_compAstTraceNode( pComp, 0 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->kind, HB_COMP_AST_NODE_CLASS_METHOD );
   assert_string_equal( pEvent->name, "MethodName" );

   pEvent = hb_compAstTraceNode( pComp, 1 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_LEAVE );
   assert_string_equal( pEvent->name, "MethodName" );

   hb_compAstTraceClear( pComp );

   token.value = "DATA";
   token.len = 4;
   hb_compAstTracePublishToken( pComp, &token );

   hb_xmemset( &data, 0, sizeof( data ) );
   data.szName = "DataMember";

   hb_compAstTraceNodeEnter( pComp, HB_COMP_AST_NODE_CLASS_DATA, &data,
                             hb_compAstTraceLastTokenId( pComp ) );
   hb_compAstTraceNodeLeave( pComp, HB_COMP_AST_NODE_CLASS_DATA, &data );

   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 2 );
   pEvent = hb_compAstTraceNode( pComp, 0 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->kind, HB_COMP_AST_NODE_CLASS_DATA );
   assert_string_equal( pEvent->name, "DataMember" );

   pEvent = hb_compAstTraceNode( pComp, 1 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_LEAVE );
   assert_string_equal( pEvent->name, "DataMember" );

   hb_comp_free( pComp );
}

static void statement_stack_records_nested_nodes( void ** state )
{
   PHB_COMP pComp = hb_comp_new();
   HB_PP_TOKEN tokenIf;
   HB_PP_TOKEN tokenWhile;
    HB_PP_TOKEN tokenBlock;
   const HB_COMP_AST_TRACE_NODE_EVENT * pEvent;

   HB_SYMBOL_UNUSED( state );

   assert_non_null( pComp );

   hb_compAstTraceSetEnabled( pComp, HB_TRUE );
   hb_compAstTraceClear( pComp );

   hb_xmemset( &tokenIf, 0, sizeof( tokenIf ) );
   tokenIf.value = "IF";
   tokenIf.len = 2;

   hb_compAstTracePublishToken( pComp, &tokenIf );
   hb_compAstTraceNodeEnterStack( pComp, HB_COMP_AST_NODE_STATEMENT_IF,
                                  hb_compAstTraceLastTokenId( pComp ) );

   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 1 );

   hb_xmemset( &tokenWhile, 0, sizeof( tokenWhile ) );
   tokenWhile.value = "WHILE";
   tokenWhile.len = 5;

   hb_compAstTracePublishToken( pComp, &tokenWhile );
   hb_compAstTraceNodeEnterStack( pComp, HB_COMP_AST_NODE_STATEMENT_WHILE,
                                  hb_compAstTraceLastTokenId( pComp ) );

   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 2 );

   hb_compAstTracePublishToken( pComp, &tokenWhile );
   hb_compAstTraceNodeLeaveStack( pComp, HB_COMP_AST_NODE_STATEMENT_WHILE );

   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 3 );

   hb_compAstTracePublishToken( pComp, &tokenIf );
   hb_compAstTraceNodeLeaveStack( pComp, HB_COMP_AST_NODE_STATEMENT_IF );

   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 4 );

   pEvent = hb_compAstTraceNode( pComp, 0 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->kind, HB_COMP_AST_NODE_STATEMENT_IF );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_ENTER );

   pEvent = hb_compAstTraceNode( pComp, 1 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->kind, HB_COMP_AST_NODE_STATEMENT_WHILE );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_ENTER );

   pEvent = hb_compAstTraceNode( pComp, 2 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->kind, HB_COMP_AST_NODE_STATEMENT_WHILE );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_LEAVE );
   assert_null( pEvent->name );

   pEvent = hb_compAstTraceNode( pComp, 3 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->kind, HB_COMP_AST_NODE_STATEMENT_IF );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_LEAVE );
   assert_null( pEvent->name );

   hb_compAstTraceClear( pComp );

   hb_xmemset( &tokenBlock, 0, sizeof( tokenBlock ) );
   tokenBlock.value = "{||";
   tokenBlock.len = 3;

   hb_compAstTracePublishToken( pComp, &tokenBlock );
   hb_compAstTraceNodeEnterStack( pComp, HB_COMP_AST_NODE_CODEBLOCK,
                                  hb_compAstTraceLastTokenId( pComp ) );
   hb_compAstTracePublishToken( pComp, &tokenBlock );
   hb_compAstTraceNodeLeaveStack( pComp, HB_COMP_AST_NODE_CODEBLOCK );

   assert_int_equal( hb_compAstTraceNodeCount( pComp ), 2 );

   pEvent = hb_compAstTraceNode( pComp, 0 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->kind, HB_COMP_AST_NODE_CODEBLOCK );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_ENTER );

   pEvent = hb_compAstTraceNode( pComp, 1 );
   assert_non_null( pEvent );
   assert_int_equal( pEvent->kind, HB_COMP_AST_NODE_CODEBLOCK );
   assert_int_equal( pEvent->phase, HB_COMP_AST_NODE_EVENT_LEAVE );
   assert_null( pEvent->name );

   hb_comp_free( pComp );
}

int main( void )
{
   const struct CMUnitTest tests[] = {
      cmocka_unit_test( traceinfo_lifetime_balances ),
      cmocka_unit_test( disabled_state_blocks_events ),
      cmocka_unit_test( token_and_boundary_events_capture_metadata ),
      cmocka_unit_test( pp_events_capture_macro_traces ),
      cmocka_unit_test( cli_toggle_controls_trace ),
      cmocka_unit_test( expression_nodes_capture_reductions ),
      cmocka_unit_test( class_and_member_events_capture_names ),
      cmocka_unit_test( statement_stack_records_nested_nodes ),
      cmocka_unit_test( node_events_pair_enter_and_leave ),
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
