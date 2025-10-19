#include "hbapi.h"
#include "hbcompdf.h"
#include "hbpp.h"
#include "hbasttrace.h"

#include <string.h>

#define HB_AST_TRACE_INITIAL_CAPACITY 32

typedef struct _HB_COMP_AST_TRACE_NODE_LINK
{
   const void * handle;
   HB_SIZE      id;
} HB_COMP_AST_TRACE_NODE_LINK;

typedef struct _HB_COMP_AST_TRACE
{
   HB_BOOL                      fEnabled;
   HB_SIZE                      nTraceinfoRetained;
   HB_SIZE                      nTraceinfoReleased;
   HB_SIZE                      nNextTokenId;
   HB_SIZE                      nNextSequence;
   HB_SIZE                      nTokenCount;
   HB_SIZE                      nTokenCapacity;
   HB_COMP_AST_TRACE_TOKEN *    pTokens;
   HB_SIZE                      nBoundaryCount;
   HB_SIZE                      nBoundaryCapacity;
   HB_COMP_AST_TRACE_BOUNDARY * pBoundaries;
   HB_SIZE                      nLastTokenId;
   HB_SIZE                      nNodeCount;
   HB_SIZE                      nNodeCapacity;
   HB_COMP_AST_TRACE_NODE_EVENT * pNodes;
   HB_SIZE                      nNextNodeId;
   HB_SIZE                      nNodeLinkCount;
   HB_SIZE                      nNodeLinkCapacity;
   HB_COMP_AST_TRACE_NODE_LINK * pNodeLinks;
   HB_SIZE                      nPpEventCount;
   HB_SIZE                      nPpEventCapacity;
   HB_COMP_AST_TRACE_PP_EVENT * pPpEvents;
} HB_COMP_AST_TRACE;

static HB_COMP_AST_TRACE * hb_compAstTraceState( PHB_COMP pComp )
{
   return pComp ? ( HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;
}

static void hb_compAstTracePpSink( void * cargo, const HB_PP_TRACE_EVENT * pEvent )
{
   hb_compAstTracePublishPreprocessorEvent( ( PHB_COMP ) cargo, pEvent );
}

static void hb_compAstTraceEnsureTokenCapacity( HB_COMP_AST_TRACE * pTrace, HB_SIZE nExtra )
{
   HB_SIZE nRequired;

   if( ! pTrace )
      return;

   nRequired = pTrace->nTokenCount + nExtra;
   if( nRequired > pTrace->nTokenCapacity )
   {
      HB_SIZE nNewCap = pTrace->nTokenCapacity ? pTrace->nTokenCapacity : HB_AST_TRACE_INITIAL_CAPACITY;

      while( nNewCap < nRequired )
         nNewCap <<= 1;

      pTrace->pTokens = ( HB_COMP_AST_TRACE_TOKEN * ) hb_xrealloc( pTrace->pTokens,
                                                                   nNewCap * sizeof( HB_COMP_AST_TRACE_TOKEN ) );
      hb_xmemset( pTrace->pTokens + pTrace->nTokenCapacity, 0,
                  ( nNewCap - pTrace->nTokenCapacity ) * sizeof( HB_COMP_AST_TRACE_TOKEN ) );
      pTrace->nTokenCapacity = nNewCap;
   }
}

static void hb_compAstTraceEnsureBoundaryCapacity( HB_COMP_AST_TRACE * pTrace, HB_SIZE nExtra )
{
   HB_SIZE nRequired;

   if( ! pTrace )
      return;

   nRequired = pTrace->nBoundaryCount + nExtra;
   if( nRequired > pTrace->nBoundaryCapacity )
   {
      HB_SIZE nNewCap = pTrace->nBoundaryCapacity ? pTrace->nBoundaryCapacity : HB_AST_TRACE_INITIAL_CAPACITY;

      while( nNewCap < nRequired )
         nNewCap <<= 1;

      pTrace->pBoundaries = ( HB_COMP_AST_TRACE_BOUNDARY * ) hb_xrealloc( pTrace->pBoundaries,
                                                                          nNewCap * sizeof( HB_COMP_AST_TRACE_BOUNDARY ) );
      hb_xmemset( pTrace->pBoundaries + pTrace->nBoundaryCapacity, 0,
                  ( nNewCap - pTrace->nBoundaryCapacity ) * sizeof( HB_COMP_AST_TRACE_BOUNDARY ) );
      pTrace->nBoundaryCapacity = nNewCap;
   }
}

static void hb_compAstTraceEnsurePpEventCapacity( HB_COMP_AST_TRACE * pTrace, HB_SIZE nExtra )
{
   HB_SIZE nRequired;

   if( ! pTrace )
      return;

   nRequired = pTrace->nPpEventCount + nExtra;
   if( nRequired > pTrace->nPpEventCapacity )
   {
      HB_SIZE nNewCap = pTrace->nPpEventCapacity ? pTrace->nPpEventCapacity : HB_AST_TRACE_INITIAL_CAPACITY;

      while( nNewCap < nRequired )
         nNewCap <<= 1;

      pTrace->pPpEvents = ( HB_COMP_AST_TRACE_PP_EVENT * ) hb_xrealloc( pTrace->pPpEvents,
                                                                        nNewCap * sizeof( HB_COMP_AST_TRACE_PP_EVENT ) );
      hb_xmemset( pTrace->pPpEvents + pTrace->nPpEventCapacity, 0,
                  ( nNewCap - pTrace->nPpEventCapacity ) * sizeof( HB_COMP_AST_TRACE_PP_EVENT ) );
      pTrace->nPpEventCapacity = nNewCap;
   }
}

static void hb_compAstTraceClearTokens( PHB_COMP pComp, HB_COMP_AST_TRACE * pTrace )
{
   HB_SIZE i;

   if( ! pTrace )
      return;

   for( i = 0; i < pTrace->nTokenCount; ++i )
   {
      HB_COMP_AST_TRACE_TOKEN * pToken = &pTrace->pTokens[ i ];

      if( pToken->traceInfo )
      {
         hb_compAstTraceReleaseInfo( pComp, ( PHB_PP_TRACEINFO ) pToken->traceInfo );
         pToken->traceInfo = NULL;
      }
      if( pToken->value )
      {
         hb_xfree( ( void * ) pToken->value );
         pToken->value = NULL;
      }
      if( pToken->module )
      {
         hb_xfree( ( void * ) pToken->module );
         pToken->module = NULL;
      }
   }

   pTrace->nTokenCount = 0;
   pTrace->nLastTokenId = 0;
}

static void hb_compAstTraceClearBoundaries( HB_COMP_AST_TRACE * pTrace )
{
   if( pTrace )
      pTrace->nBoundaryCount = 0;
}

static void hb_compAstTraceClearPpEvents( PHB_COMP pComp, HB_COMP_AST_TRACE * pTrace )
{
   HB_SIZE i;

   if( ! pTrace )
      return;

   for( i = 0; i < pTrace->nPpEventCount; ++i )
   {
      HB_COMP_AST_TRACE_PP_EVENT * pEvent = &pTrace->pPpEvents[ i ];

      if( pEvent->ruleKind )
      {
         hb_xfree( ( void * ) pEvent->ruleKind );
         pEvent->ruleKind = NULL;
      }
      if( pEvent->macroName )
      {
         hb_xfree( ( void * ) pEvent->macroName );
         pEvent->macroName = NULL;
      }
      if( pEvent->callModule )
      {
         hb_xfree( ( void * ) pEvent->callModule );
         pEvent->callModule = NULL;
      }
      if( pEvent->source )
      {
         hb_xfree( ( void * ) pEvent->source );
         pEvent->source = NULL;
      }
      if( pEvent->result )
      {
         hb_xfree( ( void * ) pEvent->result );
         pEvent->result = NULL;
      }
      if( pEvent->traceInfo )
      {
         hb_compAstTraceReleaseInfo( pComp, ( PHB_PP_TRACEINFO ) pEvent->traceInfo );
         pEvent->traceInfo = NULL;
      }
   }

   pTrace->nPpEventCount = 0;
}

static void hb_compAstTraceClearNodes( HB_COMP_AST_TRACE * pTrace )
{
   HB_SIZE i;

   if( ! pTrace )
      return;

   for( i = 0; i < pTrace->nNodeCount; ++i )
   {
      HB_COMP_AST_TRACE_NODE_EVENT * pEvent = &pTrace->pNodes[ i ];

      if( pEvent->name )
      {
         hb_xfree( ( void * ) pEvent->name );
         pEvent->name = NULL;
      }
   }

   pTrace->nNodeCount = 0;
   pTrace->nNodeLinkCount = 0;
   pTrace->nNextNodeId = 0;
}

static void hb_compAstTraceEnsureNodeCapacity( HB_COMP_AST_TRACE * pTrace, HB_SIZE nExtra )
{
   HB_SIZE nRequired;

   if( ! pTrace )
      return;

   nRequired = pTrace->nNodeCount + nExtra;
   if( nRequired > pTrace->nNodeCapacity )
   {
      HB_SIZE nNewCap = pTrace->nNodeCapacity ? pTrace->nNodeCapacity : HB_AST_TRACE_INITIAL_CAPACITY;

      while( nNewCap < nRequired )
         nNewCap <<= 1;

      pTrace->pNodes = ( HB_COMP_AST_TRACE_NODE_EVENT * ) hb_xrealloc( pTrace->pNodes,
                                                                       nNewCap * sizeof( HB_COMP_AST_TRACE_NODE_EVENT ) );
      hb_xmemset( pTrace->pNodes + pTrace->nNodeCapacity, 0,
                  ( nNewCap - pTrace->nNodeCapacity ) * sizeof( HB_COMP_AST_TRACE_NODE_EVENT ) );
      pTrace->nNodeCapacity = nNewCap;
   }
}

static void hb_compAstTraceEnsureNodeLinkCapacity( HB_COMP_AST_TRACE * pTrace, HB_SIZE nExtra )
{
   HB_SIZE nRequired;

   if( ! pTrace )
      return;

   nRequired = pTrace->nNodeLinkCount + nExtra;
   if( nRequired > pTrace->nNodeLinkCapacity )
   {
      HB_SIZE nNewCap = pTrace->nNodeLinkCapacity ? pTrace->nNodeLinkCapacity : HB_AST_TRACE_INITIAL_CAPACITY;

      while( nNewCap < nRequired )
         nNewCap <<= 1;

      pTrace->pNodeLinks = ( HB_COMP_AST_TRACE_NODE_LINK * ) hb_xrealloc( pTrace->pNodeLinks,
                                                                          nNewCap * sizeof( HB_COMP_AST_TRACE_NODE_LINK ) );
      hb_xmemset( pTrace->pNodeLinks + pTrace->nNodeLinkCapacity, 0,
                  ( nNewCap - pTrace->nNodeLinkCapacity ) * sizeof( HB_COMP_AST_TRACE_NODE_LINK ) );
      pTrace->nNodeLinkCapacity = nNewCap;
   }
}

static HB_SIZE hb_compAstTraceLookupNodeId( HB_COMP_AST_TRACE * pTrace, const void * handle )
{
   HB_SIZE i;

   if( ! pTrace || ! handle )
      return 0;

   for( i = 0; i < pTrace->nNodeLinkCount; ++i )
   {
      if( pTrace->pNodeLinks[ i ].handle == handle )
         return pTrace->pNodeLinks[ i ].id;
   }

   return 0;
}

static void hb_compAstTraceAddNodeLink( HB_COMP_AST_TRACE * pTrace, const void * handle, HB_SIZE id )
{
   if( ! pTrace || ! handle || id == 0 )
      return;

   hb_compAstTraceEnsureNodeLinkCapacity( pTrace, 1 );
   pTrace->pNodeLinks[ pTrace->nNodeLinkCount ].handle = handle;
   pTrace->pNodeLinks[ pTrace->nNodeLinkCount ].id = id;
   ++pTrace->nNodeLinkCount;
}

static void hb_compAstTraceRemoveNodeLink( HB_COMP_AST_TRACE * pTrace, const void * handle )
{
   HB_SIZE i;

   if( ! pTrace || ! handle )
      return;

   for( i = 0; i < pTrace->nNodeLinkCount; ++i )
   {
      if( pTrace->pNodeLinks[ i ].handle == handle )
      {
         if( i != pTrace->nNodeLinkCount - 1 )
            pTrace->pNodeLinks[ i ] = pTrace->pNodeLinks[ pTrace->nNodeLinkCount - 1 ];
         --pTrace->nNodeLinkCount;
         return;
      }
   }
}

void hb_compAstTraceInit( PHB_COMP pComp )
{
   HB_COMP_AST_TRACE * pTrace;

   if( ! pComp )
      return;

   pTrace = hb_compAstTraceState( pComp );
   if( ! pTrace )
   {
      pTrace = ( HB_COMP_AST_TRACE * ) hb_xgrabz( sizeof( HB_COMP_AST_TRACE ) );
      pComp->pAstTrace = ( struct _HB_COMP_AST_TRACE * ) pTrace;
   }

   pComp->fAstTraceEnabled = HB_FALSE;

   if( pComp->pLex && pComp->pLex->pPP )
      hb_pp_setTraceCallback( pComp->pLex->pPP, hb_compAstTracePpSink, pComp );
}

void hb_compAstTraceShutdown( PHB_COMP pComp )
{
   HB_COMP_AST_TRACE * pTrace;

   if( ! pComp )
      return;

   if( pComp->pLex && pComp->pLex->pPP )
      hb_pp_setTraceCallback( pComp->pLex->pPP, NULL, NULL );

   pTrace = hb_compAstTraceState( pComp );
   if( pTrace )
   {
      hb_compAstTraceClearTokens( pComp, pTrace );
      hb_compAstTraceClearBoundaries( pTrace );
      hb_compAstTraceClearNodes( pTrace );
      hb_compAstTraceClearPpEvents( pComp, pTrace );
      if( pTrace->pTokens )
      {
         hb_xfree( pTrace->pTokens );
         pTrace->pTokens = NULL;
      }
      if( pTrace->pBoundaries )
      {
         hb_xfree( pTrace->pBoundaries );
         pTrace->pBoundaries = NULL;
      }
      if( pTrace->pNodes )
      {
         hb_xfree( pTrace->pNodes );
         pTrace->pNodes = NULL;
      }
      if( pTrace->pNodeLinks )
      {
         hb_xfree( pTrace->pNodeLinks );
         pTrace->pNodeLinks = NULL;
      }
      if( pTrace->pPpEvents )
      {
         hb_xfree( pTrace->pPpEvents );
         pTrace->pPpEvents = NULL;
      }
      pTrace->nTokenCapacity = 0;
      pTrace->nBoundaryCapacity = 0;
      pTrace->nNodeCapacity = 0;
      pTrace->nNodeLinkCapacity = 0;
      pTrace->nPpEventCapacity = 0;
      pTrace->nNextTokenId = 0;
      pTrace->nNextSequence = 0;
      pComp->pAstTrace = NULL;
      pComp->fAstTraceEnabled = HB_FALSE;
      hb_xfree( pTrace );
   }
}

void hb_compAstTraceSetEnabled( PHB_COMP pComp, HB_BOOL fEnabled )
{
   HB_COMP_AST_TRACE * pTrace = hb_compAstTraceState( pComp );

   if( pTrace )
   {
      if( pTrace->fEnabled != fEnabled )
         hb_compAstTraceClear( pComp );
      pTrace->fEnabled = fEnabled;
   }
   if( pComp )
      pComp->fAstTraceEnabled = fEnabled;
}

HB_BOOL hb_compAstTraceIsEnabled( const HB_COMP * pComp )
{
   return pComp ? pComp->fAstTraceEnabled : HB_FALSE;
}

void hb_compAstTraceRetainInfo( PHB_COMP pComp, PHB_PP_TRACEINFO pTraceInfo )
{
   HB_COMP_AST_TRACE * pTrace;

   if( ! pComp || ! pTraceInfo )
      return;

   pTrace = hb_compAstTraceState( pComp );
   if( pTrace )
   {
      hb_pp_traceinfoRetain( pTraceInfo );
      ++pTrace->nTraceinfoRetained;
   }
}

void hb_compAstTraceReleaseInfo( PHB_COMP pComp, PHB_PP_TRACEINFO pTraceInfo )
{
   HB_COMP_AST_TRACE * pTrace;

   if( ! pComp || ! pTraceInfo )
      return;

   pTrace = hb_compAstTraceState( pComp );
   if( pTrace )
   {
      hb_pp_traceinfoRelease( pTraceInfo );
      ++pTrace->nTraceinfoReleased;
   }
}

HB_SIZE hb_compAstTraceOutstandingTraceinfo( const HB_COMP * pComp )
{
   const HB_COMP_AST_TRACE * pTrace;

   if( ! pComp )
      return 0;

   pTrace = ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace;
   if( ! pTrace )
      return 0;

   if( pTrace->nTraceinfoRetained >= pTrace->nTraceinfoReleased )
      return pTrace->nTraceinfoRetained - pTrace->nTraceinfoReleased;

   return 0;
}

void hb_compAstTracePublishToken( PHB_COMP pComp, const PHB_PP_TOKEN pToken )
{
   HB_COMP_AST_TRACE * pTrace;
   HB_COMP_AST_TRACE_TOKEN * pTarget;

   if( ! pComp || ! pToken )
      return;

   pTrace = hb_compAstTraceState( pComp );
   if( ! pTrace || ! pTrace->fEnabled )
      return;

   hb_compAstTraceEnsureTokenCapacity( pTrace, 1 );
   pTarget = &pTrace->pTokens[ pTrace->nTokenCount ];
   hb_xmemset( pTarget, 0, sizeof( HB_COMP_AST_TRACE_TOKEN ) );

   pTarget->id = ++pTrace->nNextTokenId;
   pTarget->sequence = ++pTrace->nNextSequence;
   pTarget->spaces = pToken->spaces;
   pTarget->length = pToken->len;
   pTarget->type = pToken->type;
   pTarget->markerIndex = pToken->index;
   if( pToken->value )
   {
      char * pValue = ( char * ) hb_xgrab( pToken->len + 1 );

      if( pToken->len )
         memcpy( pValue, pToken->value, pToken->len );
      pValue[ pToken->len ] = '\0';
      pTarget->value = pValue;
   }
   if( pToken->szModule )
      pTarget->module = hb_strdup( pToken->szModule );
   pTarget->line = pToken->iLine;
   pTarget->column = pToken->iColumn;
   pTarget->endColumn = pToken->iEndColumn;
   pTarget->offset = pToken->nOffset;
   pTarget->endOffset = pToken->nEndOffset;
   if( pToken->pTraceInfo )
   {
      hb_compAstTraceRetainInfo( pComp, pToken->pTraceInfo );
      pTarget->traceInfo = pToken->pTraceInfo;
   }

   ++pTrace->nTokenCount;
   pTrace->nLastTokenId = pTarget->id;
}

void hb_compAstTracePublishBoundary( PHB_COMP pComp, int code, int lexState )
{
   HB_COMP_AST_TRACE * pTrace;
   HB_COMP_AST_TRACE_BOUNDARY * pTarget;

   if( ! pComp )
      return;

   pTrace = hb_compAstTraceState( pComp );
   if( ! pTrace || ! pTrace->fEnabled )
      return;

   hb_compAstTraceEnsureBoundaryCapacity( pTrace, 1 );
   pTarget = &pTrace->pBoundaries[ pTrace->nBoundaryCount ];
   hb_xmemset( pTarget, 0, sizeof( HB_COMP_AST_TRACE_BOUNDARY ) );

   pTarget->sequence = ++pTrace->nNextSequence;
   pTarget->tokenId = pTrace->nLastTokenId;
   pTarget->code = code;
   pTarget->lexState = lexState;

   ++pTrace->nBoundaryCount;
}

void hb_compAstTracePublishPreprocessorEvent( PHB_COMP pComp, const HB_PP_TRACE_EVENT * pEvent )
{
   HB_COMP_AST_TRACE * pTrace;
   HB_COMP_AST_TRACE_PP_EVENT * pTarget;

   if( ! pComp || ! pEvent )
      return;

   pTrace = hb_compAstTraceState( pComp );
   if( ! pTrace || ! pTrace->fEnabled )
      return;

   hb_compAstTraceEnsurePpEventCapacity( pTrace, 1 );
   pTarget = &pTrace->pPpEvents[ pTrace->nPpEventCount ];
   hb_xmemset( pTarget, 0, sizeof( HB_COMP_AST_TRACE_PP_EVENT ) );

   pTarget->sequence = ++pTrace->nNextSequence;

   if( pEvent->szRuleKind && pEvent->szRuleKind[ 0 ] != '\0' )
      pTarget->ruleKind = hb_strdup( pEvent->szRuleKind );
   if( pEvent->szMacroName && pEvent->szMacroName[ 0 ] != '\0' )
      pTarget->macroName = hb_strdup( pEvent->szMacroName );
   if( pEvent->szCallModule && pEvent->szCallModule[ 0 ] != '\0' )
      pTarget->callModule = hb_strdup( pEvent->szCallModule );
   if( pEvent->pszSource && pEvent->pszSource[ 0 ] != '\0' )
      pTarget->source = hb_strdup( pEvent->pszSource );
   if( pEvent->pszResult && pEvent->pszResult[ 0 ] != '\0' )
      pTarget->result = hb_strdup( pEvent->pszResult );

   pTarget->callLine = pEvent->iCallLine;
   pTarget->callColumn = pEvent->iCallColumn;
   pTarget->callEndLine = pEvent->iCallEndLine;
   pTarget->callEndColumn = pEvent->iCallEndColumn;
   pTarget->callOffset = pEvent->nCallOffset;
   pTarget->callEndOffset = pEvent->nCallEndOffset;
   pTarget->expansionId = pEvent->nExpansionId;

   if( pEvent->pTraceInfo )
   {
      hb_compAstTraceRetainInfo( pComp, ( PHB_PP_TRACEINFO ) pEvent->pTraceInfo );
      pTarget->traceInfo = pEvent->pTraceInfo;
   }

   ++pTrace->nPpEventCount;
}

void hb_compAstTraceNodeEnter( PHB_COMP pComp, HB_COMP_AST_NODE_KIND kind, const void * handle, HB_SIZE tokenId )
{
   HB_COMP_AST_TRACE * pTrace;
   HB_COMP_AST_TRACE_NODE_EVENT * pEvent;

   if( ! pComp )
      return;

   pTrace = hb_compAstTraceState( pComp );
   if( ! pTrace || ! pTrace->fEnabled )
      return;

   hb_compAstTraceEnsureNodeCapacity( pTrace, 1 );
   pEvent = &pTrace->pNodes[ pTrace->nNodeCount ];
   hb_xmemset( pEvent, 0, sizeof( HB_COMP_AST_TRACE_NODE_EVENT ) );

   pEvent->id = ++pTrace->nNextNodeId;
   pEvent->sequence = ++pTrace->nNextSequence;
   pEvent->kind = kind;
   pEvent->phase = HB_COMP_AST_NODE_EVENT_ENTER;
   pEvent->tokenId = tokenId;
   pEvent->handle = handle;

   if( handle && kind == HB_COMP_AST_NODE_FUNCTION )
   {
      const HB_HFUNC * pFunc = ( const HB_HFUNC * ) handle;

      if( pFunc->szName )
         pEvent->name = hb_strdup( pFunc->szName );
   }

   ++pTrace->nNodeCount;
   hb_compAstTraceAddNodeLink( pTrace, handle, pEvent->id );
}

void hb_compAstTraceNodeLeave( PHB_COMP pComp, HB_COMP_AST_NODE_KIND kind, const void * handle )
{
   HB_COMP_AST_TRACE * pTrace;
   HB_COMP_AST_TRACE_NODE_EVENT * pEvent;
   HB_SIZE nodeId;

   if( ! pComp )
      return;

   pTrace = hb_compAstTraceState( pComp );
   if( ! pTrace || ! pTrace->fEnabled )
      return;

   nodeId = hb_compAstTraceLookupNodeId( pTrace, handle );
   if( nodeId == 0 )
      return;

   hb_compAstTraceEnsureNodeCapacity( pTrace, 1 );
   pEvent = &pTrace->pNodes[ pTrace->nNodeCount ];
   hb_xmemset( pEvent, 0, sizeof( HB_COMP_AST_TRACE_NODE_EVENT ) );

   pEvent->id = nodeId;
   pEvent->sequence = ++pTrace->nNextSequence;
   pEvent->kind = kind;
   pEvent->phase = HB_COMP_AST_NODE_EVENT_LEAVE;
   pEvent->tokenId = pTrace->nLastTokenId;
   pEvent->handle = handle;

   if( handle && kind == HB_COMP_AST_NODE_FUNCTION )
   {
      const HB_HFUNC * pFunc = ( const HB_HFUNC * ) handle;

      if( pFunc->szName )
         pEvent->name = hb_strdup( pFunc->szName );
   }

   ++pTrace->nNodeCount;
   hb_compAstTraceRemoveNodeLink( pTrace, handle );
}

HB_SIZE hb_compAstTraceTokenCount( const HB_COMP * pComp )
{
   const HB_COMP_AST_TRACE * pTrace = pComp ? ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;

   return pTrace ? pTrace->nTokenCount : 0;
}

const HB_COMP_AST_TRACE_TOKEN * hb_compAstTraceToken( const HB_COMP * pComp, HB_SIZE index )
{
   const HB_COMP_AST_TRACE * pTrace = pComp ? ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;

   if( ! pTrace || index >= pTrace->nTokenCount )
      return NULL;

   return &pTrace->pTokens[ index ];
}

HB_SIZE hb_compAstTraceBoundaryCount( const HB_COMP * pComp )
{
   const HB_COMP_AST_TRACE * pTrace = pComp ? ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;

   return pTrace ? pTrace->nBoundaryCount : 0;
}

const HB_COMP_AST_TRACE_BOUNDARY * hb_compAstTraceBoundary( const HB_COMP * pComp, HB_SIZE index )
{
   const HB_COMP_AST_TRACE * pTrace = pComp ? ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;

   if( ! pTrace || index >= pTrace->nBoundaryCount )
      return NULL;

   return &pTrace->pBoundaries[ index ];
}

HB_SIZE hb_compAstTracePpEventCount( const HB_COMP * pComp )
{
   const HB_COMP_AST_TRACE * pTrace = pComp ? ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;

   return pTrace ? pTrace->nPpEventCount : 0;
}

const HB_COMP_AST_TRACE_PP_EVENT * hb_compAstTracePpEvent( const HB_COMP * pComp, HB_SIZE index )
{
   const HB_COMP_AST_TRACE * pTrace = pComp ? ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;

   if( ! pTrace || index >= pTrace->nPpEventCount )
      return NULL;

   return &pTrace->pPpEvents[ index ];
}

HB_SIZE hb_compAstTraceNodeCount( const HB_COMP * pComp )
{
   const HB_COMP_AST_TRACE * pTrace = pComp ? ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;

   return pTrace ? pTrace->nNodeCount : 0;
}

const HB_COMP_AST_TRACE_NODE_EVENT * hb_compAstTraceNode( const HB_COMP * pComp, HB_SIZE index )
{
   const HB_COMP_AST_TRACE * pTrace = pComp ? ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;

   if( ! pTrace || index >= pTrace->nNodeCount )
      return NULL;

   return &pTrace->pNodes[ index ];
}

HB_SIZE hb_compAstTraceLastTokenId( const HB_COMP * pComp )
{
   const HB_COMP_AST_TRACE * pTrace = pComp ? ( const HB_COMP_AST_TRACE * ) pComp->pAstTrace : NULL;

   return pTrace ? pTrace->nLastTokenId : 0;
}

void hb_compAstTraceClear( PHB_COMP pComp )
{
   HB_COMP_AST_TRACE * pTrace = hb_compAstTraceState( pComp );

   if( ! pTrace )
      return;

   hb_compAstTraceClearTokens( pComp, pTrace );
   hb_compAstTraceClearBoundaries( pTrace );
   hb_compAstTraceClearNodes( pTrace );
   hb_compAstTraceClearPpEvents( pComp, pTrace );
   pTrace->nNextTokenId = 0;
   pTrace->nNextSequence = 0;
   pTrace->nLastTokenId = 0;
}
