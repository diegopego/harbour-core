#include "ast/hbast_builder.h"
#include "hbapi.h"
#include "hbapifs.h"
#include "hbdefs.h"
#include <string.h>

typedef struct
{
   HB_AST_NODE_INFO info;
   HB_SIZE childCapacity;
   HB_SIZE tokenCapacity;
} HB_AST_NODE_INTERNAL;

typedef struct
{
   HB_AST_SYMBOL_INFO info;
   HB_SIZE declCapacity;
   HB_SIZE refCapacity;
} HB_AST_SYMBOL_INTERNAL;

typedef struct
{
   HB_AST_NODE_INTERNAL * pNodes;
   HB_SIZE nNodeCount;
   HB_SIZE nNodeCapacity;
   HB_AST_SYMBOL_INTERNAL * pSymbols;
   HB_SIZE nSymbolCount;
   HB_SIZE nSymbolCapacity;
   HB_SIZE nextNodeId;
   HB_SIZE nextSymbolId;
   HB_SIZE rootId;
} HB_AST_BUILD_STATE;

static void hb_astBuilderStateInit( HB_AST_BUILD_STATE * pState )
{
   hb_xmemset( pState, 0, sizeof( HB_AST_BUILD_STATE ) );
   pState->rootId = HB_SIZE_MAX;
   pState->nextSymbolId = 5000;
}

static void hb_astBuilderFreeNodeInternal( HB_AST_NODE_INTERNAL * pNode )
{
   if( pNode->info.pszKind )
      hb_xfree( pNode->info.pszKind );
   if( pNode->info.pszStableId )
      hb_xfree( pNode->info.pszStableId );
   if( pNode->info.pChildren )
      hb_xfree( pNode->info.pChildren );
   if( pNode->info.pTokens )
      hb_xfree( pNode->info.pTokens );
}

static void hb_astBuilderFreeSymbolInternal( HB_AST_SYMBOL_INTERNAL * pSymbol )
{
   if( pSymbol->info.pszKind )
      hb_xfree( pSymbol->info.pszKind );
   if( pSymbol->info.pszName )
      hb_xfree( pSymbol->info.pszName );
   if( pSymbol->info.pszQualifiedName )
      hb_xfree( pSymbol->info.pszQualifiedName );
   if( pSymbol->info.pDeclarations )
      hb_xfree( pSymbol->info.pDeclarations );
   if( pSymbol->info.pReferences )
      hb_xfree( pSymbol->info.pReferences );
}

static void hb_astBuilderStateRelease( HB_AST_BUILD_STATE * pState )
{
   if( pState->pNodes )
   {
      HB_SIZE i;

      for( i = 0; i < pState->nNodeCount; ++i )
         hb_astBuilderFreeNodeInternal( &pState->pNodes[ i ] );

      hb_xfree( pState->pNodes );
   }

   if( pState->pSymbols )
   {
      HB_SIZE i;

      for( i = 0; i < pState->nSymbolCount; ++i )
         hb_astBuilderFreeSymbolInternal( &pState->pSymbols[ i ] );

      hb_xfree( pState->pSymbols );
   }

   hb_astBuilderStateInit( pState );
}

static HB_AST_NODE_INTERNAL * hb_astBuilderAddNodeInternal( HB_AST_BUILD_STATE * pState, const char * pszKind )
{
   HB_AST_NODE_INTERNAL * pNode;

   if( pState->nNodeCount == pState->nNodeCapacity )
   {
      HB_SIZE nNewCap = pState->nNodeCapacity == 0 ? 8 : pState->nNodeCapacity << 1;

      if( pState->pNodes )
         pState->pNodes = ( HB_AST_NODE_INTERNAL * ) hb_xrealloc( pState->pNodes, nNewCap * sizeof( HB_AST_NODE_INTERNAL ) );
      else
         pState->pNodes = ( HB_AST_NODE_INTERNAL * ) hb_xgrab( nNewCap * sizeof( HB_AST_NODE_INTERNAL ) );

      hb_xmemset( pState->pNodes + pState->nNodeCapacity, 0,
                  ( nNewCap - pState->nNodeCapacity ) * sizeof( HB_AST_NODE_INTERNAL ) );

      pState->nNodeCapacity = nNewCap;
   }

   pNode = &pState->pNodes[ pState->nNodeCount++ ];
   hb_xmemset( pNode, 0, sizeof( HB_AST_NODE_INTERNAL ) );

   pNode->info.id = pState->nextNodeId++;
   pNode->info.pszKind = pszKind ? hb_strdup( pszKind ) : NULL;
   pNode->info.parentId = HB_SIZE_MAX;
   pNode->info.symbolId = HB_SIZE_MAX;

   return pNode;
}

static HB_AST_NODE_INTERNAL * hb_astBuilderLookupNode( HB_AST_BUILD_STATE * pState, HB_SIZE id )
{
   if( id == HB_SIZE_MAX || id >= pState->nNodeCount )
      return NULL;
   return &pState->pNodes[ id ];
}

static void hb_astBuilderNodeAddChild( HB_AST_NODE_INTERNAL * pParent, HB_SIZE childId )
{
   if( pParent == NULL )
      return;

   if( pParent->childCapacity == pParent->info.nChildCount )
   {
      HB_SIZE nNewCap = pParent->childCapacity == 0 ? 4 : pParent->childCapacity << 1;

      if( pParent->info.pChildren )
         pParent->info.pChildren = ( HB_SIZE * ) hb_xrealloc( pParent->info.pChildren, nNewCap * sizeof( HB_SIZE ) );
      else
         pParent->info.pChildren = ( HB_SIZE * ) hb_xgrab( nNewCap * sizeof( HB_SIZE ) );

      pParent->childCapacity = nNewCap;
   }

  pParent->info.pChildren[ pParent->info.nChildCount++ ] = childId;
}

static void hb_astBuilderNodeAddToken( HB_AST_NODE_INTERNAL * pNode, HB_SIZE tokenId )
{
   if( pNode == NULL )
      return;

   if( pNode->tokenCapacity == pNode->info.nTokenCount )
   {
      HB_SIZE nNewCap = pNode->tokenCapacity == 0 ? 8 : pNode->tokenCapacity << 1;

      if( pNode->info.pTokens )
         pNode->info.pTokens = ( HB_SIZE * ) hb_xrealloc( pNode->info.pTokens, nNewCap * sizeof( HB_SIZE ) );
      else
         pNode->info.pTokens = ( HB_SIZE * ) hb_xgrab( nNewCap * sizeof( HB_SIZE ) );

      pNode->tokenCapacity = nNewCap;
   }

   pNode->info.pTokens[ pNode->info.nTokenCount++ ] = tokenId;
}

static HB_BOOL hb_astBuilderModulesEqual( const char * pszA, const char * pszB )
{
   if( pszA == NULL || pszB == NULL )
      return HB_FALSE;

   return hb_stricmp( pszA, pszB ) == 0;
}

static HB_BOOL hb_astBuilderTokenBelongsToModule( const HB_AST_TOKEN * pToken, const char * pszModule )
{
   if( pszModule == NULL || *pszModule == '\0' )
      return HB_TRUE;

   if( pToken->pszModule && hb_astBuilderModulesEqual( pToken->pszModule, pszModule ) )
      return HB_TRUE;

   if( pToken->pMacroOrigin )
   {
      const char * pszCallModule = hb_astMacroTraceCallModule( pToken->pMacroOrigin );
      if( pszCallModule && hb_astBuilderModulesEqual( pszCallModule, pszModule ) )
         return HB_TRUE;
   }

   return HB_FALSE;
}

static HB_BOOL hb_astBuilderIsKeyword( const HB_AST_TOKEN * pToken, const char * pszKeyword )
{
   return pToken &&
          pToken->kind == HB_AST_TOKEN_KIND_KEYWORD &&
          pToken->pszLexeme &&
          hb_stricmp( pToken->pszLexeme, pszKeyword ) == 0;
}

static const HB_AST_TOKEN * hb_astBuilderTokenByIndex( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE index )
{
   return hb_astTokenStreamToken( pStream, index );
}

static const HB_AST_TOKEN * hb_astBuilderTokenById( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE tokenId )
{
   HB_SIZE i, count = hb_astTokenStreamCount( pStream );

   for( i = 0; i < count; ++i )
   {
      const HB_AST_TOKEN * pToken = hb_astTokenStreamToken( pStream, i );

      if( pToken && pToken->id.uHash == tokenId )
         return pToken;
   }

   return NULL;
}

static void hb_astBuilderNodeSetRangeFromIndices( HB_AST_NODE_INTERNAL * pNode,
                                                  const HB_AST_TOKEN_STREAM * pStream,
                                                  HB_SIZE nStart,
                                                  HB_SIZE nEnd )
{
   const HB_AST_TOKEN * pStart = hb_astBuilderTokenByIndex( pStream, nStart );
   const HB_AST_TOKEN * pEnd   = hb_astBuilderTokenByIndex( pStream, nEnd );

   if( pStart )
      pNode->info.range.start = pStart->original.start;
   else
      hb_xmemset( &pNode->info.range.start, 0, sizeof( HB_AST_SOURCE_COORD ) );

   if( pEnd )
      pNode->info.range.end = pEnd->original.end;
   else
      hb_xmemset( &pNode->info.range.end, 0, sizeof( HB_AST_SOURCE_COORD ) );
}

static void hb_astBuilderAssignStableId( HB_AST_NODE_INTERNAL * pNode, const char * pszModule )
{
   char buffer[ 512 ];
   const char * pszKind = pNode->info.pszKind ? pNode->info.pszKind : "Node";
   HB_SIZE nOffset = pNode->info.range.start.nOffset;

   hb_snprintf( buffer, sizeof( buffer ), "%s:%" HB_PFS "u:%s@%08" HB_PFS "u",
                pszModule ? pszModule : "",
                nOffset,
                pszKind,
                pNode->info.id );

   if( pNode->info.pszStableId )
      hb_xfree( pNode->info.pszStableId );
   pNode->info.pszStableId = hb_strdup( buffer );
}

static const char * hb_astBuilderExtractIdentifier( const HB_AST_TOKEN_STREAM * pStream,
                                                    const HB_AST_NODE_INTERNAL * pNode,
                                                    const char * pszKeyword )
{
   HB_SIZE i;
   HB_BOOL fSeenKeyword = HB_FALSE;

   for( i = 0; i < pNode->info.nTokenCount; ++i )
   {
      HB_SIZE tokenId = pNode->info.pTokens[ i ];
      const HB_AST_TOKEN * pToken = hb_astBuilderTokenById( pStream, tokenId );

      if( pToken == NULL )
         continue;

      if( hb_astBuilderIsKeyword( pToken, pszKeyword ) )
      {
         fSeenKeyword = HB_TRUE;
         continue;
      }

      if( fSeenKeyword &&
          ( pToken->kind == HB_AST_TOKEN_KIND_IDENTIFIER ||
            pToken->kind == HB_AST_TOKEN_KIND_KEYWORD ) &&
          pToken->pszLexeme )
      {
         return pToken->pszLexeme;
      }
   }

   return NULL;
}

static void hb_astBuilderTrimNodeArrays( HB_AST_NODE_INTERNAL * pNode )
{
   if( pNode->info.pChildren && pNode->childCapacity > pNode->info.nChildCount )
   {
      if( pNode->info.nChildCount == 0 )
      {
         hb_xfree( pNode->info.pChildren );
         pNode->info.pChildren = NULL;
      }
      else
         pNode->info.pChildren = ( HB_SIZE * ) hb_xrealloc( pNode->info.pChildren, pNode->info.nChildCount * sizeof( HB_SIZE ) );
   }
   if( pNode->info.pTokens && pNode->tokenCapacity > pNode->info.nTokenCount )
   {
      if( pNode->info.nTokenCount == 0 )
      {
         hb_xfree( pNode->info.pTokens );
         pNode->info.pTokens = NULL;
      }
      else
         pNode->info.pTokens = ( HB_SIZE * ) hb_xrealloc( pNode->info.pTokens, pNode->info.nTokenCount * sizeof( HB_SIZE ) );
   }
}

static void hb_astBuilderSymbolAddDeclaration( HB_AST_SYMBOL_INTERNAL * pSymbol, HB_SIZE nodeId )
{
   if( pSymbol->declCapacity == pSymbol->info.nDeclarationCount )
   {
      HB_SIZE nNewCap = pSymbol->declCapacity == 0 ? 2 : pSymbol->declCapacity << 1;

      if( pSymbol->info.pDeclarations )
         pSymbol->info.pDeclarations = ( HB_SIZE * ) hb_xrealloc( pSymbol->info.pDeclarations, nNewCap * sizeof( HB_SIZE ) );
      else
         pSymbol->info.pDeclarations = ( HB_SIZE * ) hb_xgrab( nNewCap * sizeof( HB_SIZE ) );

      pSymbol->declCapacity = nNewCap;
   }

   pSymbol->info.pDeclarations[ pSymbol->info.nDeclarationCount++ ] = nodeId;
}

static HB_BOOL hb_astBuilderSymbolHasReference( const HB_AST_SYMBOL_INTERNAL * pSymbol, HB_SIZE nodeId )
{
   HB_SIZE i;

   for( i = 0; i < pSymbol->info.nReferenceCount; ++i )
   {
      if( pSymbol->info.pReferences[ i ] == nodeId )
         return HB_TRUE;
   }
   return HB_FALSE;
}

static void hb_astBuilderSymbolAddReference( HB_AST_SYMBOL_INTERNAL * pSymbol, HB_SIZE nodeId )
{
   if( hb_astBuilderSymbolHasReference( pSymbol, nodeId ) )
      return;

   if( pSymbol->refCapacity == pSymbol->info.nReferenceCount )
   {
      HB_SIZE nNewCap = pSymbol->refCapacity == 0 ? 4 : pSymbol->refCapacity << 1;

      if( pSymbol->info.pReferences )
         pSymbol->info.pReferences = ( HB_SIZE * ) hb_xrealloc( pSymbol->info.pReferences, nNewCap * sizeof( HB_SIZE ) );
      else
         pSymbol->info.pReferences = ( HB_SIZE * ) hb_xgrab( nNewCap * sizeof( HB_SIZE ) );

      pSymbol->refCapacity = nNewCap;
   }

   pSymbol->info.pReferences[ pSymbol->info.nReferenceCount++ ] = nodeId;
}

static void hb_astBuilderTrimSymbolArrays( HB_AST_SYMBOL_INTERNAL * pSymbol )
{
   if( pSymbol->info.pDeclarations && pSymbol->declCapacity > pSymbol->info.nDeclarationCount )
   {
      if( pSymbol->info.nDeclarationCount == 0 )
      {
         hb_xfree( pSymbol->info.pDeclarations );
         pSymbol->info.pDeclarations = NULL;
      }
      else
         pSymbol->info.pDeclarations = ( HB_SIZE * ) hb_xrealloc( pSymbol->info.pDeclarations,
                                                                  pSymbol->info.nDeclarationCount * sizeof( HB_SIZE ) );
   }

   if( pSymbol->info.pReferences && pSymbol->refCapacity > pSymbol->info.nReferenceCount )
   {
      if( pSymbol->info.nReferenceCount == 0 )
      {
         hb_xfree( pSymbol->info.pReferences );
         pSymbol->info.pReferences = NULL;
      }
      else
         pSymbol->info.pReferences = ( HB_SIZE * ) hb_xrealloc( pSymbol->info.pReferences,
                                                                pSymbol->info.nReferenceCount * sizeof( HB_SIZE ) );
   }
}

static HB_BOOL hb_astBuilderNodeIsDescendantOf( const HB_AST_BUILD_STATE * pState, HB_SIZE nodeId, HB_SIZE ancestorId )
{
   while( nodeId != HB_SIZE_MAX )
   {
      const HB_AST_NODE_INTERNAL * pNode;

      if( nodeId >= pState->nNodeCount )
         break;

      pNode = &pState->pNodes[ nodeId ];

      if( pNode->info.parentId == ancestorId )
         return HB_TRUE;

      nodeId = pNode->info.parentId;
   }
   return HB_FALSE;
}

static HB_BOOL hb_astBuilderNodeContainsIdentifier( const HB_AST_NODE_INTERNAL * pNode,
                                                    const HB_AST_TOKEN_STREAM * pStream,
                                                    const char * pszModule,
                                                    const char * pszName )
{
   HB_SIZE i;

   for( i = 0; i < pNode->info.nTokenCount; ++i )
   {
      HB_SIZE tokenId = pNode->info.pTokens[ i ];
      const HB_AST_TOKEN * pToken = hb_astBuilderTokenById( pStream, tokenId );

      if( pToken == NULL )
         continue;

      if( ( pToken->kind == HB_AST_TOKEN_KIND_IDENTIFIER ||
            pToken->kind == HB_AST_TOKEN_KIND_KEYWORD ) &&
          pToken->pszLexeme &&
          hb_astBuilderTokenBelongsToModule( pToken, pszModule ) &&
          hb_stricmp( pToken->pszLexeme, pszName ) == 0 )
      {
         return HB_TRUE;
      }
   }

   return HB_FALSE;
}

static HB_AST_SYMBOL_INTERNAL * hb_astBuilderAddSymbolInternal( HB_AST_BUILD_STATE * pState,
                                                                const char * pszKind,
                                                                const char * pszName )
{
   HB_AST_SYMBOL_INTERNAL * pSymbol;

   if( pState->nSymbolCount == pState->nSymbolCapacity )
   {
      HB_SIZE nNewCap = pState->nSymbolCapacity == 0 ? 4 : pState->nSymbolCapacity << 1;

      if( pState->pSymbols )
         pState->pSymbols = ( HB_AST_SYMBOL_INTERNAL * ) hb_xrealloc( pState->pSymbols,
                                                                      nNewCap * sizeof( HB_AST_SYMBOL_INTERNAL ) );
      else
         pState->pSymbols = ( HB_AST_SYMBOL_INTERNAL * ) hb_xgrab( nNewCap * sizeof( HB_AST_SYMBOL_INTERNAL ) );

      hb_xmemset( pState->pSymbols + pState->nSymbolCapacity, 0,
                  ( nNewCap - pState->nSymbolCapacity ) * sizeof( HB_AST_SYMBOL_INTERNAL ) );

      pState->nSymbolCapacity = nNewCap;
   }

   pSymbol = &pState->pSymbols[ pState->nSymbolCount++ ];
   hb_xmemset( pSymbol, 0, sizeof( HB_AST_SYMBOL_INTERNAL ) );

   pSymbol->info.symbolId = pState->nextSymbolId++;
   pSymbol->info.pszKind = pszKind ? hb_strdup( pszKind ) : NULL;
   pSymbol->info.pszName = pszName ? hb_strdup( pszName ) : NULL;
   pSymbol->info.pszQualifiedName = pszName ? hb_strdup( pszName ) : NULL;

   return pSymbol;
}

static void hb_astBuilderCollectReferences( HB_AST_BUILD_STATE * pState,
                                            HB_AST_SYMBOL_INTERNAL * pSymbol,
                                            const HB_AST_TOKEN_STREAM * pStream,
                                            const char * pszModule,
                                            HB_SIZE declarationNodeId,
                                            HB_BOOL fLimitToAncestor )
{
   HB_SIZE i;
   const char * pszName = pSymbol->info.pszName;
   HB_SIZE ancestorId = HB_SIZE_MAX;

   if( fLimitToAncestor )
   {
      HB_AST_NODE_INTERNAL * pDeclNode = hb_astBuilderLookupNode( pState, declarationNodeId );

      if( pDeclNode )
         ancestorId = pDeclNode->info.parentId;
   }

   if( pszName == NULL )
      return;

   for( i = 0; i < pState->nNodeCount; ++i )
   {
      HB_AST_NODE_INTERNAL * pNode = &pState->pNodes[ i ];

      if( pNode->info.id == declarationNodeId )
         continue;

      if( pNode->info.id == pState->rootId )
         continue;

      if( fLimitToAncestor && ancestorId != HB_SIZE_MAX )
      {
         if( ! hb_astBuilderNodeIsDescendantOf( pState, pNode->info.id, ancestorId ) )
            continue;
      }

      if( hb_astBuilderNodeContainsIdentifier( pNode, pStream, pszModule, pszName ) )
         hb_astBuilderSymbolAddReference( pSymbol, pNode->info.id );
   }
}

static HB_BOOL hb_astBuilderPopulateNodes( HB_AST_BUILD_STATE * pState,
                                           const HB_AST_TOKEN_STREAM * pStream,
                                           const char * pszModule )
{
   HB_AST_NODE_INTERNAL * pRoot;
   HB_SIZE nTokenCount;
   HB_SIZE nFirstIdx = HB_SIZE_MAX;
   HB_SIZE nLastIdx = 0;
   HB_SIZE i;
   HB_SIZE nProcId = HB_SIZE_MAX;
   HB_AST_NODE_INTERNAL * pRootNode = NULL;
   HB_AST_NODE_INTERNAL * pParentNode;

   hb_astBuilderStateInit( pState );

   pRoot = hb_astBuilderAddNodeInternal( pState, "File" );
   if( pRoot == NULL )
      return HB_FALSE;
   pState->rootId = pRoot->info.id;

   if( pStream == NULL )
   {
      hb_astBuilderAssignStableId( pRoot, pszModule );
      return HB_TRUE;
   }

   nTokenCount = hb_astTokenStreamCount( pStream );

   for( i = 0; i < nTokenCount; ++i )
   {
      const HB_AST_TOKEN * pToken = hb_astBuilderTokenByIndex( pStream, i );

      if( pToken && hb_astBuilderTokenBelongsToModule( pToken, pszModule ) )
      {
         if( nFirstIdx == HB_SIZE_MAX )
            nFirstIdx = i;
         nLastIdx = i;
         hb_astBuilderNodeAddToken( pRoot, pToken->id.uHash );
      }
   }

   if( nFirstIdx != HB_SIZE_MAX )
      hb_astBuilderNodeSetRangeFromIndices( pRoot, pStream, nFirstIdx, nLastIdx );
   else
      hb_xmemset( &pRoot->info.range, 0, sizeof( HB_AST_SOURCE_RANGE ) );

   hb_astBuilderAssignStableId( pRoot, pszModule );

   if( nFirstIdx == HB_SIZE_MAX )
      return HB_TRUE;

   i = nFirstIdx;
   while( i <= nLastIdx && i < nTokenCount )
   {
      const HB_AST_TOKEN * pToken = hb_astBuilderTokenByIndex( pStream, i );

      if( pToken == NULL || ! hb_astBuilderTokenBelongsToModule( pToken, pszModule ) )
      {
         ++i;
         continue;
      }

      if( hb_astBuilderIsKeyword( pToken, "PROC" ) || hb_astBuilderIsKeyword( pToken, "FUNCTION" ) )
      {
         HB_AST_NODE_INTERNAL * pProcNode;
         const char * pszKind = hb_astBuilderIsKeyword( pToken, "PROC" ) ? "ProcDecl" : "FunctionDecl";
         HB_SIZE nStart = i;
         HB_SIZE nHeaderEnd = i;
         HB_SIZE nEnd = nLastIdx;
         HB_SIZE j;

         for( j = i; j <= nLastIdx && j < nTokenCount; ++j )
         {
            const HB_AST_TOKEN * pTemp = hb_astBuilderTokenByIndex( pStream, j );
            if( pTemp && pTemp->kind == HB_AST_TOKEN_KIND_NEWLINE &&
                hb_astBuilderTokenBelongsToModule( pTemp, pszModule ) )
            {
               nHeaderEnd = j;
               break;
            }
         }

         for( j = i + 1; j <= nLastIdx && j < nTokenCount; ++j )
         {
            const HB_AST_TOKEN * pTemp = hb_astBuilderTokenByIndex( pStream, j );
            if( pTemp && hb_astBuilderTokenBelongsToModule( pTemp, pszModule ) &&
                ( hb_astBuilderIsKeyword( pTemp, "PROC" ) || hb_astBuilderIsKeyword( pTemp, "FUNCTION" ) ) )
            {
               nEnd = j > 0 ? j - 1 : j;
               break;
            }
         }

         if( nEnd < nStart )
            nEnd = nStart;

         pProcNode = hb_astBuilderAddNodeInternal( pState, pszKind );
         if( pProcNode == NULL )
            return HB_FALSE;

         nProcId = pProcNode->info.id;

         pProcNode->info.parentId = pState->rootId;

         pRootNode = hb_astBuilderLookupNode( pState, pState->rootId );
         if( pRootNode )
            hb_astBuilderNodeAddChild( pRootNode, nProcId );

         for( j = nStart; j <= nEnd && j < nTokenCount; ++j )
         {
            const HB_AST_TOKEN * pTemp = hb_astBuilderTokenByIndex( pStream, j );
            if( pTemp && hb_astBuilderTokenBelongsToModule( pTemp, pszModule ) )
               hb_astBuilderNodeAddToken( pProcNode, pTemp->id.uHash );
         }

         hb_astBuilderNodeSetRangeFromIndices( pProcNode, pStream, nStart, nEnd );
         hb_astBuilderAssignStableId( pProcNode, pszModule );

         if( nHeaderEnd < nEnd )
         {
            HB_SIZE nBodyIdx = nHeaderEnd + 1;

            while( nBodyIdx <= nEnd && nBodyIdx < nTokenCount )
            {
               const HB_AST_TOKEN * pBodyTok = hb_astBuilderTokenByIndex( pStream, nBodyIdx );

               if( pBodyTok == NULL || ! hb_astBuilderTokenBelongsToModule( pBodyTok, pszModule ) )
               {
                  ++nBodyIdx;
                  continue;
               }

               if( hb_astBuilderIsKeyword( pBodyTok, "LOCAL" ) || hb_astBuilderIsKeyword( pBodyTok, "RETURN" ) )
               {
                  const char * pszStmtKind = hb_astBuilderIsKeyword( pBodyTok, "LOCAL" ) ?
                                             "LocalDecl" : "ReturnStmt";
                  HB_AST_NODE_INTERNAL * pStmtNode;
                  HB_SIZE nStmtStart = nBodyIdx;
                  HB_SIZE nStmtEnd = nBodyIdx;
                  HB_SIZE nScan;

                  for( nScan = nBodyIdx; nScan <= nEnd && nScan < nTokenCount; ++nScan )
                  {
                     const HB_AST_TOKEN * pScanTok = hb_astBuilderTokenByIndex( pStream, nScan );

                     if( pScanTok && pScanTok->kind == HB_AST_TOKEN_KIND_NEWLINE &&
                         hb_astBuilderTokenBelongsToModule( pScanTok, pszModule ) )
                     {
                        nStmtEnd = nScan;
                        break;
                     }
                  }

                  if( nStmtEnd < nStmtStart )
                     nStmtEnd = nStmtStart;
                  if( nStmtEnd > nEnd )
                     nStmtEnd = nEnd;

                  pStmtNode = hb_astBuilderAddNodeInternal( pState, pszStmtKind );
                  if( pStmtNode == NULL )
                     return HB_FALSE;

                  pStmtNode->info.parentId = nProcId;

                  pParentNode = hb_astBuilderLookupNode( pState, nProcId );
                  if( pParentNode )
                     hb_astBuilderNodeAddChild( pParentNode, pStmtNode->info.id );

                  for( nScan = nStmtStart; nScan <= nStmtEnd && nScan < nTokenCount; ++nScan )
                  {
                     const HB_AST_TOKEN * pStmtTok = hb_astBuilderTokenByIndex( pStream, nScan );

                     if( pStmtTok && hb_astBuilderTokenBelongsToModule( pStmtTok, pszModule ) )
                        hb_astBuilderNodeAddToken( pStmtNode, pStmtTok->id.uHash );
                     else if( pStmtTok && pStmtTok->pMacroOrigin )
                     {
                        const char * pszCall = hb_astMacroTraceCallModule( pStmtTok->pMacroOrigin );
                        if( pszCall && pszModule && hb_astBuilderModulesEqual( pszCall, pszModule ) )
                           hb_astBuilderNodeAddToken( pStmtNode, pStmtTok->id.uHash );
                     }
                  }

                  hb_astBuilderNodeSetRangeFromIndices( pStmtNode, pStream, nStmtStart, nStmtEnd );
                  hb_astBuilderAssignStableId( pStmtNode, pszModule );

                  nBodyIdx = nStmtEnd + 1;
                  continue;
               }

               ++nBodyIdx;
            }
         }

         i = ( nEnd < nLastIdx ) ? nEnd + 1 : nLastIdx + 1;
         continue;
      }

      ++i;
   }

   return HB_TRUE;
}

static HB_BOOL hb_astBuilderPopulateSymbols( HB_AST_BUILD_STATE * pState,
                                             const HB_AST_TOKEN_STREAM * pStream,
                                             const char * pszModule )
{
   HB_SIZE i;

   for( i = 0; i < pState->nNodeCount; ++i )
   {
      HB_AST_NODE_INTERNAL * pNode = &pState->pNodes[ i ];

      if( pNode->info.pszKind == NULL )
         continue;

      if( hb_stricmp( pNode->info.pszKind, "ProcDecl" ) == 0 ||
          hb_stricmp( pNode->info.pszKind, "FunctionDecl" ) == 0 )
      {
         const char * pszKeyword = hb_stricmp( pNode->info.pszKind, "ProcDecl" ) == 0 ? "PROC" : "FUNCTION";
         const char * pszName = hb_astBuilderExtractIdentifier( pStream, pNode, pszKeyword );

         if( pszName )
         {
            HB_AST_SYMBOL_INTERNAL * pSymbol = hb_astBuilderAddSymbolInternal( pState, "Function", pszName );

            if( pSymbol == NULL )
               return HB_FALSE;

            hb_astBuilderSymbolAddDeclaration( pSymbol, pNode->info.id );
            pNode->info.symbolId = pSymbol->info.symbolId;

            hb_astBuilderCollectReferences( pState, pSymbol, pStream, pszModule, pNode->info.id, HB_FALSE );
         }
      }
      else if( hb_stricmp( pNode->info.pszKind, "LocalDecl" ) == 0 )
      {
         const char * pszName = hb_astBuilderExtractIdentifier( pStream, pNode, "LOCAL" );

         if( pszName )
         {
            HB_AST_SYMBOL_INTERNAL * pSymbol = hb_astBuilderAddSymbolInternal( pState, "Variable", pszName );

            if( pSymbol == NULL )
               return HB_FALSE;

            hb_astBuilderSymbolAddDeclaration( pSymbol, pNode->info.id );
            pNode->info.symbolId = pSymbol->info.symbolId;

            hb_astBuilderCollectReferences( pState, pSymbol, pStream, pszModule, pNode->info.id, HB_TRUE );
         }
      }
   }

   return HB_TRUE;
}

static HB_BOOL hb_astBuilderFinalize( HB_AST_BUILD_STATE * pState, HB_AST_BUILD_RESULT * pResult )
{
   HB_SIZE i;

   hb_xmemset( pResult, 0, sizeof( HB_AST_BUILD_RESULT ) );

   if( pState->nNodeCount )
   {
      pResult->pNodes = ( HB_AST_NODE_INFO * ) hb_xgrab( pState->nNodeCount * sizeof( HB_AST_NODE_INFO ) );

      for( i = 0; i < pState->nNodeCount; ++i )
      {
         HB_AST_NODE_INTERNAL * pNode = &pState->pNodes[ i ];

         hb_astBuilderTrimNodeArrays( pNode );
         pResult->pNodes[ i ] = pNode->info;

         /* Prevent double free during state release */
         pNode->info.pszKind = NULL;
         pNode->info.pszStableId = NULL;
         pNode->info.pChildren = NULL;
         pNode->info.pTokens = NULL;
      }
   }

   pResult->nNodeCount = pState->nNodeCount;
   pResult->nRootId = pState->rootId;

   if( pState->nSymbolCount )
   {
      pResult->pSymbols = ( HB_AST_SYMBOL_INFO * ) hb_xgrab( pState->nSymbolCount * sizeof( HB_AST_SYMBOL_INFO ) );

      for( i = 0; i < pState->nSymbolCount; ++i )
      {
         HB_AST_SYMBOL_INTERNAL * pSymbol = &pState->pSymbols[ i ];

         hb_astBuilderTrimSymbolArrays( pSymbol );
         pResult->pSymbols[ i ] = pSymbol->info;

         pSymbol->info.pszKind = NULL;
         pSymbol->info.pszName = NULL;
         pSymbol->info.pszQualifiedName = NULL;
         pSymbol->info.pDeclarations = NULL;
         pSymbol->info.pReferences = NULL;
      }
   }

   pResult->nSymbolCount = pState->nSymbolCount;

   return HB_TRUE;
}

HB_BOOL hb_astBuildFromStream( const HB_AST_TOKEN_STREAM * pStream,
                               const char * pszModule,
                               HB_AST_BUILD_RESULT * pResult )
{
   HB_AST_BUILD_STATE state;
   HB_BOOL fStatus = HB_FALSE;

   if( pResult == NULL )
      return HB_FALSE;

   hb_astBuilderStateInit( &state );

   if( hb_astBuilderPopulateNodes( &state, pStream, pszModule ) &&
       hb_astBuilderPopulateSymbols( &state, pStream, pszModule ) &&
       hb_astBuilderFinalize( &state, pResult ) )
   {
      fStatus = HB_TRUE;
   }

   hb_astBuilderStateRelease( &state );
   return fStatus;
}

void hb_astBuildResultRelease( HB_AST_BUILD_RESULT * pResult )
{
   HB_SIZE i;

   if( pResult == NULL )
      return;

   if( pResult->pNodes )
   {
      for( i = 0; i < pResult->nNodeCount; ++i )
      {
         HB_AST_NODE_INFO * pNode = &pResult->pNodes[ i ];

         if( pNode->pszKind )
            hb_xfree( pNode->pszKind );
         if( pNode->pszStableId )
            hb_xfree( pNode->pszStableId );
         if( pNode->pChildren )
            hb_xfree( pNode->pChildren );
         if( pNode->pTokens )
            hb_xfree( pNode->pTokens );
      }

      hb_xfree( pResult->pNodes );
   }

   if( pResult->pSymbols )
   {
      for( i = 0; i < pResult->nSymbolCount; ++i )
      {
         HB_AST_SYMBOL_INFO * pSymbol = &pResult->pSymbols[ i ];

         if( pSymbol->pszKind )
            hb_xfree( pSymbol->pszKind );
         if( pSymbol->pszName )
            hb_xfree( pSymbol->pszName );
         if( pSymbol->pszQualifiedName )
            hb_xfree( pSymbol->pszQualifiedName );
         if( pSymbol->pDeclarations )
            hb_xfree( pSymbol->pDeclarations );
         if( pSymbol->pReferences )
            hb_xfree( pSymbol->pReferences );
      }

      hb_xfree( pResult->pSymbols );
   }

   hb_xmemset( pResult, 0, sizeof( HB_AST_BUILD_RESULT ) );
}
