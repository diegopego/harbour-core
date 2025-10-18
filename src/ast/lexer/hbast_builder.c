#include "ast/hbast_builder.h"
#include "hbapi.h"
#include "hbapifs.h"
#include "hbdefs.h"
#include <ctype.h>
#include <stdint.h>
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
   HB_SIZE rootId;
} HB_AST_BUILD_STATE;

static void hb_astBuilderStateInit( HB_AST_BUILD_STATE * pState )
{
   hb_xmemset( pState, 0, sizeof( HB_AST_BUILD_STATE ) );
   pState->rootId = HB_SIZE_MAX;
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
                                                  HB_SIZE nEnd );
static void hb_astBuilderAssignStableId( HB_AST_NODE_INTERNAL * pNode, const char * pszModule );

static HB_U64 hb_astBuilderHashLower( HB_U64 hash, const char * pszValue )
{
   const unsigned char * pch = ( const unsigned char * ) ( pszValue ? pszValue : "" );

   while( *pch )
   {
      hash ^= ( HB_U64 ) tolower( *pch++ );
      hash *= UINT64_C( 1099511628211 );
   }

   return hash;
}

static HB_SIZE hb_astBuilderComputeSymbolId( const char * pszModule,
                                             const char * pszQualifiedName,
                                             const char * pszKind )
{
   HB_U64 hash = UINT64_C( 14695981039346656037 );
   HB_SIZE result;

   hash = hb_astBuilderHashLower( hash, pszModule );
   hash = hb_astBuilderHashLower( hash, "::" );
   hash = hb_astBuilderHashLower( hash, pszKind );
   hash = hb_astBuilderHashLower( hash, "::" );
   hash = hb_astBuilderHashLower( hash, pszQualifiedName );

   hash &= ( HB_U64 ) HB_SIZE_MAX;
   result = ( HB_SIZE ) hash;

   if( result == HB_SIZE_MAX )
      result--;
   if( result == 0 )
      result = 1;

   return result;
}

static HB_SIZE * hb_astBuilderCollectModuleIndices( const HB_AST_TOKEN_STREAM * pStream,
                                                    const char * pszModule,
                                                    HB_SIZE * pnCount )
{
   HB_SIZE nTokenCount = hb_astTokenStreamCount( pStream );
   HB_SIZE nCount = 0;
   HB_SIZE * pIndices;
   HB_SIZE i;

   if( pnCount == NULL )
      return NULL;

   if( nTokenCount == 0 )
   {
      *pnCount = 0;
      return NULL;
   }

   pIndices = ( HB_SIZE * ) hb_xgrab( nTokenCount * sizeof( HB_SIZE ) );

   for( i = 0; i < nTokenCount; ++i )
   {
      const HB_AST_TOKEN * pToken = hb_astBuilderTokenByIndex( pStream, i );

      if( pToken && hb_astBuilderTokenBelongsToModule( pToken, pszModule ) )
         pIndices[ nCount++ ] = i;
   }

   if( nCount == 0 )
   {
      hb_xfree( pIndices );
      pIndices = NULL;
   }
   else if( nCount < nTokenCount )
      pIndices = ( HB_SIZE * ) hb_xrealloc( pIndices, nCount * sizeof( HB_SIZE ) );

   *pnCount = nCount;
   return pIndices;
}

static HB_BOOL hb_astBuilderTokenIsFunctionStart( const HB_AST_TOKEN_STREAM * pStream,
                                                  const HB_SIZE * pIndices,
                                                  HB_SIZE nCount,
                                                  HB_SIZE pos )
{
   const HB_AST_TOKEN * pToken;

   if( pIndices == NULL || pos >= nCount )
      return HB_FALSE;

   pToken = hb_astBuilderTokenByIndex( pStream, pIndices[ pos ] );
   if( pToken == NULL || pToken->kind != HB_AST_TOKEN_KIND_KEYWORD )
      return HB_FALSE;

   if( hb_astBuilderIsKeyword( pToken, "PROC" ) ||
       hb_astBuilderIsKeyword( pToken, "PROCEDURE" ) ||
       hb_astBuilderIsKeyword( pToken, "FUNCTION" ) )
   {
      return HB_TRUE;
   }

   if( hb_astBuilderIsKeyword( pToken, "STATIC" ) )
   {
      HB_SIZE lookahead = pos + 1;

      while( lookahead < nCount )
      {
         const HB_AST_TOKEN * pNext = hb_astBuilderTokenByIndex( pStream, pIndices[ lookahead ] );

         if( pNext == NULL )
         {
            ++lookahead;
            continue;
         }

         if( pNext->kind == HB_AST_TOKEN_KIND_NEWLINE )
            break;

         if( pNext->kind == HB_AST_TOKEN_KIND_KEYWORD &&
             ( hb_astBuilderIsKeyword( pNext, "PROC" ) ||
               hb_astBuilderIsKeyword( pNext, "PROCEDURE" ) ||
               hb_astBuilderIsKeyword( pNext, "FUNCTION" ) ) )
         {
            return HB_TRUE;
         }

         ++lookahead;
      }
   }

   return HB_FALSE;
}

static HB_SIZE hb_astBuilderFindNextFunctionStart( const HB_AST_TOKEN_STREAM * pStream,
                                                   const HB_SIZE * pIndices,
                                                   HB_SIZE nCount,
                                                   HB_SIZE startPos )
{
   HB_SIZE pos;

   for( pos = startPos; pos < nCount; ++pos )
   {
      if( hb_astBuilderTokenIsFunctionStart( pStream, pIndices, nCount, pos ) )
         return pos;
   }

   return nCount;
}

static HB_AST_NODE_INTERNAL * hb_astBuilderCreateNodeForRange( HB_AST_BUILD_STATE * pState,
                                                               const char * pszKind,
                                                               HB_SIZE parentId,
                                                               const HB_AST_TOKEN_STREAM * pStream,
                                                               const HB_SIZE * pIndices,
                                                               HB_SIZE nStartPos,
                                                               HB_SIZE nEndPos,
                                                               const char * pszModule )
{
   HB_AST_NODE_INTERNAL * pNode;
   HB_AST_NODE_INTERNAL * pParent;
   HB_SIZE idx;

   if( pState == NULL || pIndices == NULL || nStartPos > nEndPos )
      return NULL;

   pNode = hb_astBuilderAddNodeInternal( pState, pszKind );
   if( pNode == NULL )
      return NULL;

   pNode->info.parentId = parentId;

   pParent = hb_astBuilderLookupNode( pState, parentId );
   if( pParent )
      hb_astBuilderNodeAddChild( pParent, pNode->info.id );

   for( idx = nStartPos; idx <= nEndPos; ++idx )
   {
      const HB_AST_TOKEN * pTok = hb_astBuilderTokenByIndex( pStream, pIndices[ idx ] );

      if( pTok && hb_astBuilderTokenBelongsToModule( pTok, pszModule ) )
         hb_astBuilderNodeAddToken( pNode, pTok->id.uHash );
   }

   hb_astBuilderNodeSetRangeFromIndices( pNode, pStream, pIndices[ nStartPos ], pIndices[ nEndPos ] );
   hb_astBuilderAssignStableId( pNode, pszModule );

   return pNode;
}

static HB_SIZE hb_astBuilderFindStatementEnd( const HB_AST_TOKEN_STREAM * pStream,
                                              const HB_SIZE * pIndices,
                                              HB_SIZE pos,
                                              HB_SIZE limitEnd )
{
   HB_SIZE idx = pos;

   while( idx < limitEnd )
   {
      const HB_AST_TOKEN * pTok = hb_astBuilderTokenByIndex( pStream, pIndices[ idx ] );

      if( pTok && pTok->kind == HB_AST_TOKEN_KIND_NEWLINE )
         return idx;

      ++idx;
   }

   return limitEnd;
}

static void hb_astBuilderParseFunctionBody( HB_AST_BUILD_STATE * pState,
                                            HB_AST_NODE_INTERNAL * pFunctionNode,
                                            const HB_AST_TOKEN_STREAM * pStream,
                                            const char * pszModule,
                                            const HB_SIZE * pIndices,
                                            HB_SIZE bodyStartPos,
                                            HB_SIZE endPos )
{
   HB_SIZE pos = bodyStartPos;

   if( pState == NULL || pFunctionNode == NULL || pIndices == NULL )
      return;

   while( pos <= endPos )
   {
      const HB_AST_TOKEN * pTok = hb_astBuilderTokenByIndex( pStream, pIndices[ pos ] );

      if( pTok && pTok->kind == HB_AST_TOKEN_KIND_KEYWORD )
      {
         if( hb_astBuilderIsKeyword( pTok, "LOCAL" ) )
         {
            HB_SIZE stmtEnd = hb_astBuilderFindStatementEnd( pStream, pIndices, pos, endPos );
            hb_astBuilderCreateNodeForRange( pState,
                                             "LocalDecl",
                                             pFunctionNode->info.id,
                                             pStream,
                                             pIndices,
                                             pos,
                                             stmtEnd,
                                             pszModule );
            pos = stmtEnd + 1;
            continue;
         }
         else if( hb_astBuilderIsKeyword( pTok, "RETURN" ) )
         {
            HB_SIZE stmtEnd = hb_astBuilderFindStatementEnd( pStream, pIndices, pos, endPos );
            hb_astBuilderCreateNodeForRange( pState,
                                             "ReturnStmt",
                                             pFunctionNode->info.id,
                                             pStream,
                                             pIndices,
                                             pos,
                                             stmtEnd,
                                             pszModule );
            pos = stmtEnd + 1;
            continue;
         }
      }

      ++pos;
   }
}

static HB_SIZE hb_astBuilderParseFunction( HB_AST_BUILD_STATE * pState,
                                           const HB_AST_TOKEN_STREAM * pStream,
                                           const char * pszModule,
                                           const HB_SIZE * pIndices,
                                           HB_SIZE nCount,
                                           HB_SIZE startPos )
{
   HB_SIZE funcKeywordPos = startPos;
   HB_SIZE scan;
   HB_SIZE endPos;
   HB_AST_NODE_INTERNAL * pFunctionNode;
   const HB_AST_TOKEN * pKeywordTok;
   const char * pszNodeKind = "ProcDecl";
   HB_SIZE nextStart;
   HB_SIZE headerEndPos;
   HB_SIZE bodyStartPos;

   if( pState == NULL || pIndices == NULL || startPos >= nCount )
      return startPos + 1;

   for( scan = startPos; scan < nCount; ++scan )
   {
      const HB_AST_TOKEN * pTok = hb_astBuilderTokenByIndex( pStream, pIndices[ scan ] );

      if( pTok == NULL )
         continue;

      if( pTok->kind == HB_AST_TOKEN_KIND_NEWLINE )
         break;

      if( pTok->kind == HB_AST_TOKEN_KIND_KEYWORD &&
          ( hb_astBuilderIsKeyword( pTok, "PROC" ) ||
            hb_astBuilderIsKeyword( pTok, "PROCEDURE" ) ||
            hb_astBuilderIsKeyword( pTok, "FUNCTION" ) ) )
      {
         funcKeywordPos = scan;
         break;
      }
   }

   pKeywordTok = hb_astBuilderTokenByIndex( pStream, pIndices[ funcKeywordPos ] );
   if( pKeywordTok && hb_astBuilderIsKeyword( pKeywordTok, "FUNCTION" ) )
      pszNodeKind = "FunctionDecl";

   nextStart = hb_astBuilderFindNextFunctionStart( pStream, pIndices, nCount, funcKeywordPos + 1 );
   if( nextStart < nCount && nextStart > startPos )
      endPos = nextStart - 1;
   else
      endPos = nCount == 0 ? 0 : nCount - 1;

   if( endPos < startPos )
      endPos = startPos;

   pFunctionNode = hb_astBuilderCreateNodeForRange( pState,
                                                    pszNodeKind,
                                                    pState->rootId,
                                                    pStream,
                                                    pIndices,
                                                    startPos,
                                                    endPos,
                                                    pszModule );
   if( pFunctionNode == NULL )
      return endPos + 1;

   headerEndPos = endPos;
   for( scan = funcKeywordPos; scan <= endPos; ++scan )
   {
      const HB_AST_TOKEN * pTok = hb_astBuilderTokenByIndex( pStream, pIndices[ scan ] );

      if( pTok && pTok->kind == HB_AST_TOKEN_KIND_NEWLINE )
      {
         headerEndPos = scan;
         break;
      }
   }

   if( headerEndPos < endPos )
      bodyStartPos = headerEndPos + 1;
   else
      bodyStartPos = endPos + 1;

   if( bodyStartPos <= endPos )
      hb_astBuilderParseFunctionBody( pState,
                                      pFunctionNode,
                                      pStream,
                                      pszModule,
                                      pIndices,
                                      bodyStartPos,
                                      endPos );

   return endPos + 1;
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
                                                                const char * pszName,
                                                                const char * pszQualifiedName,
                                                                const char * pszModule )
{
   HB_AST_SYMBOL_INTERNAL * pSymbol;
   const char * pszQual;
   const char * pszSymbolKind;

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

   pszQual = pszQualifiedName ? pszQualifiedName : pszName;
   pszSymbolKind = pszKind ? pszKind : "Symbol";

   pSymbol->info.symbolId = hb_astBuilderComputeSymbolId( pszModule,
                                                          pszQual ? pszQual : "",
                                                          pszSymbolKind );
   pSymbol->info.pszKind = pszSymbolKind ? hb_strdup( pszSymbolKind ) : NULL;
   pSymbol->info.pszName = pszName ? hb_strdup( pszName ) : NULL;
   pSymbol->info.pszQualifiedName = pszQual ? hb_strdup( pszQual ) : NULL;

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
   HB_SIZE * pIndices = NULL;
   HB_SIZE nModuleTokenCount = 0;
   HB_SIZE idx;

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

   pIndices = hb_astBuilderCollectModuleIndices( pStream, pszModule, &nModuleTokenCount );

   if( pIndices && nModuleTokenCount > 0 )
   {
      for( idx = 0; idx < nModuleTokenCount; ++idx )
      {
         const HB_AST_TOKEN * pTok = hb_astBuilderTokenByIndex( pStream, pIndices[ idx ] );

         if( pTok && hb_astBuilderTokenBelongsToModule( pTok, pszModule ) )
            hb_astBuilderNodeAddToken( pRoot, pTok->id.uHash );
      }

      hb_astBuilderNodeSetRangeFromIndices( pRoot,
                                            pStream,
                                            pIndices[ 0 ],
                                            pIndices[ nModuleTokenCount - 1 ] );
   }
   else
      hb_xmemset( &pRoot->info.range, 0, sizeof( HB_AST_SOURCE_RANGE ) );

   hb_astBuilderAssignStableId( pRoot, pszModule );

   if( pIndices == NULL || nModuleTokenCount == 0 )
   {
      if( pIndices )
         hb_xfree( pIndices );
      return HB_TRUE;
   }

   idx = 0;
   while( idx < nModuleTokenCount )
   {
      if( hb_astBuilderTokenIsFunctionStart( pStream, pIndices, nModuleTokenCount, idx ) )
         idx = hb_astBuilderParseFunction( pState,
                                           pStream,
                                           pszModule,
                                           pIndices,
                                           nModuleTokenCount,
                                           idx );
      else
         ++idx;
   }

   hb_xfree( pIndices );

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
         char szQualifier[ 512 ];

         if( pszName == NULL && hb_stricmp( pNode->info.pszKind, "ProcDecl" ) == 0 )
            pszName = hb_astBuilderExtractIdentifier( pStream, pNode, "PROCEDURE" );

         if( pszName )
         {
            HB_AST_SYMBOL_INTERNAL * pSymbol;

            hb_snprintf( szQualifier, sizeof( szQualifier ), "%s", pszName );

            pSymbol = hb_astBuilderAddSymbolInternal( pState,
                                                      "Function",
                                                      pszName,
                                                      szQualifier,
                                                      pszModule );

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
         char szQualifier[ 512 ];

         if( pszName )
         {
            HB_AST_SYMBOL_INTERNAL * pSymbol;
            HB_AST_NODE_INTERNAL * pParentNode = hb_astBuilderLookupNode( pState, pNode->info.parentId );
            const char * pszParentName = NULL;

            if( pParentNode && pParentNode->info.pszKind )
            {
               const char * pszParentKeyword = hb_stricmp( pParentNode->info.pszKind, "FunctionDecl" ) == 0 ?
                                               "FUNCTION" : "PROC";

               pszParentName = hb_astBuilderExtractIdentifier( pStream, pParentNode, pszParentKeyword );
               if( pszParentName == NULL && hb_stricmp( pParentNode->info.pszKind, "ProcDecl" ) == 0 )
                  pszParentName = hb_astBuilderExtractIdentifier( pStream, pParentNode, "PROCEDURE" );
            }

            if( pszParentName )
               hb_snprintf( szQualifier, sizeof( szQualifier ), "%s::%s", pszParentName, pszName );
            else
               hb_snprintf( szQualifier, sizeof( szQualifier ), "%s", pszName );

            pSymbol = hb_astBuilderAddSymbolInternal( pState,
                                                      "Variable",
                                                      pszName,
                                                      szQualifier,
                                                      pszModule );

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
