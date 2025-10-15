#include "ast/lexer/hbast_lexer.h"
#include "hbapi.h"
#include "hbapifs.h"
#include <string.h>

typedef struct _HB_AST_JSON_BUFFER
{
   char *  pszData;
   HB_SIZE nLen;
   HB_SIZE nCapacity;
} HB_AST_JSON_BUFFER;

typedef struct _HB_AST_NODE_SERIAL
{
   HB_SIZE id;
   char *  pszKind;
   char *  pszStableId;
   HB_AST_SOURCE_RANGE range;
   HB_SIZE parentId;
   HB_SIZE * pChildren;
   HB_SIZE nChildCount;
   HB_SIZE nChildCapacity;
   HB_SIZE * pTokens;
   HB_SIZE nTokenCount;
   HB_SIZE nTokenCapacity;
} HB_AST_NODE_SERIAL;

typedef struct _HB_AST_NODE_LIST
{
   HB_AST_NODE_SERIAL * pNodes;
   HB_SIZE nCount;
   HB_SIZE nCapacity;
   HB_SIZE nRootId;
} HB_AST_NODE_LIST;

static void hb_astJsonBufferInit( HB_AST_JSON_BUFFER * pBuf )
{
   pBuf->nCapacity = 256;
   pBuf->nLen = 0;
   pBuf->pszData = ( char * ) hb_xgrab( pBuf->nCapacity );
}

static void hb_astJsonBufferEnsure( HB_AST_JSON_BUFFER * pBuf, HB_SIZE nExtra )
{
   HB_SIZE nNeeded = pBuf->nLen + nExtra;

   if( nNeeded > pBuf->nCapacity )
   {
      while( pBuf->nCapacity < nNeeded )
         pBuf->nCapacity <<= 1;
      pBuf->pszData = ( char * ) hb_xrealloc( pBuf->pszData, pBuf->nCapacity );
   }
}

static void hb_astJsonBufferAddChar( HB_AST_JSON_BUFFER * pBuf, char c )
{
   hb_astJsonBufferEnsure( pBuf, 1 );
   pBuf->pszData[ pBuf->nLen++ ] = c;
}

static void hb_astJsonBufferAddCStr( HB_AST_JSON_BUFFER * pBuf, const char * pszValue )
{
   HB_SIZE nLen = strlen( pszValue );
   hb_astJsonBufferEnsure( pBuf, nLen );
   memcpy( pBuf->pszData + pBuf->nLen, pszValue, nLen );
   pBuf->nLen += nLen;
}

static void hb_astJsonBufferAddUnsigned( HB_AST_JSON_BUFFER * pBuf, HB_SIZE nValue )
{
   char buffer[ 64 ];

   hb_snprintf( buffer, sizeof( buffer ), "%" HB_PFS "u", nValue );
   hb_astJsonBufferAddCStr( pBuf, buffer );
}

static void hb_astJsonAddEscapedString( HB_AST_JSON_BUFFER * pBuf, const char * pszValue )
{
   const unsigned char * pch;

   if( pszValue == NULL )
   {
      hb_astJsonBufferAddCStr( pBuf, "null" );
      return;
   }

   hb_astJsonBufferAddChar( pBuf, '"' );
   pch = ( const unsigned char * ) pszValue;
   while( *pch )
   {
      unsigned char ch = *pch++;

      switch( ch )
      {
         case '\"':
         case '\\':
            hb_astJsonBufferAddChar( pBuf, '\\' );
            hb_astJsonBufferAddChar( pBuf, ( char ) ch );
            break;

         case '\b':
            hb_astJsonBufferAddCStr( pBuf, "\\b" );
            break;

         case '\f':
            hb_astJsonBufferAddCStr( pBuf, "\\f" );
            break;

         case '\n':
            hb_astJsonBufferAddCStr( pBuf, "\\n" );
            break;

         case '\r':
            hb_astJsonBufferAddCStr( pBuf, "\\r" );
            break;

         case '\t':
            hb_astJsonBufferAddCStr( pBuf, "\\t" );
            break;

         default:
            if( ch < 0x20 )
            {
               char escape[ 7 ];
               hb_snprintf( escape, sizeof( escape ), "\\u%04x", ch );
               hb_astJsonBufferAddCStr( pBuf, escape );
            }
            else
               hb_astJsonBufferAddChar( pBuf, ( char ) ch );
            break;
      }
   }
   hb_astJsonBufferAddChar( pBuf, '"' );
}

static void hb_astJsonAppendCoord( HB_AST_JSON_BUFFER * pBuf, const HB_AST_SOURCE_COORD * pCoord )
{
   hb_astJsonBufferAddChar( pBuf, '{' );
   hb_astJsonBufferAddCStr( pBuf, "\"line\":" );
   hb_astJsonBufferAddUnsigned( pBuf, pCoord->nLine );
   hb_astJsonBufferAddCStr( pBuf, ",\"column\":" );
   hb_astJsonBufferAddUnsigned( pBuf, pCoord->nColumn );
   hb_astJsonBufferAddCStr( pBuf, ",\"offset\":" );
   hb_astJsonBufferAddUnsigned( pBuf, pCoord->nOffset );
   hb_astJsonBufferAddChar( pBuf, '}' );
}

static void hb_astJsonAppendRange( HB_AST_JSON_BUFFER * pBuf, const HB_AST_SOURCE_RANGE * pRange )
{
   hb_astJsonBufferAddChar( pBuf, '{' );
   hb_astJsonBufferAddCStr( pBuf, "\"start\":" );
   hb_astJsonAppendCoord( pBuf, &pRange->start );
   hb_astJsonBufferAddCStr( pBuf, ",\"end\":" );
   hb_astJsonAppendCoord( pBuf, &pRange->end );
   hb_astJsonBufferAddChar( pBuf, '}' );
}

static void hb_astNodeListInit( HB_AST_NODE_LIST * pList )
{
   hb_xmemset( pList, 0, sizeof( *pList ) );
}

static void hb_astNodeRelease( HB_AST_NODE_SERIAL * pNode )
{
   if( pNode->pszKind )
      hb_xfree( pNode->pszKind );
   if( pNode->pszStableId )
      hb_xfree( pNode->pszStableId );
   if( pNode->pChildren )
      hb_xfree( pNode->pChildren );
   if( pNode->pTokens )
      hb_xfree( pNode->pTokens );
}

static void hb_astNodeListRelease( HB_AST_NODE_LIST * pList )
{
   if( pList->pNodes )
   {
      HB_SIZE i;

      for( i = 0; i < pList->nCount; ++i )
         hb_astNodeRelease( &pList->pNodes[ i ] );

      hb_xfree( pList->pNodes );
   }

   hb_xmemset( pList, 0, sizeof( *pList ) );
}

static HB_AST_NODE_SERIAL * hb_astNodeListAdd( HB_AST_NODE_LIST * pList, HB_SIZE nId, const char * pszKind )
{
   HB_AST_NODE_SERIAL * pNode;

   if( pList->nCount == pList->nCapacity )
   {
      HB_SIZE nNewCap = pList->nCapacity == 0 ? 8 : pList->nCapacity << 1;

      if( pList->pNodes )
         pList->pNodes = ( HB_AST_NODE_SERIAL * ) hb_xrealloc( pList->pNodes, nNewCap * sizeof( HB_AST_NODE_SERIAL ) );
      else
         pList->pNodes = ( HB_AST_NODE_SERIAL * ) hb_xgrab( nNewCap * sizeof( HB_AST_NODE_SERIAL ) );

      hb_xmemset( pList->pNodes + pList->nCapacity, 0,
                  ( nNewCap - pList->nCapacity ) * sizeof( HB_AST_NODE_SERIAL ) );

      pList->nCapacity = nNewCap;
   }

   pNode = &pList->pNodes[ pList->nCount++ ];
   hb_xmemset( pNode, 0, sizeof( HB_AST_NODE_SERIAL ) );
   pNode->id = nId;
   pNode->parentId = HB_SIZE_MAX;
   if( pszKind )
      pNode->pszKind = hb_strdup( pszKind );

   return pNode;
}

static void hb_astNodeAddChild( HB_AST_NODE_SERIAL * pNode, HB_SIZE nChildId )
{
   if( pNode == NULL )
      return;

   if( pNode->nChildCount == pNode->nChildCapacity )
   {
      HB_SIZE nNewCap = pNode->nChildCapacity == 0 ? 4 : pNode->nChildCapacity << 1;

      if( pNode->pChildren )
         pNode->pChildren = ( HB_SIZE * ) hb_xrealloc( pNode->pChildren, nNewCap * sizeof( HB_SIZE ) );
      else
         pNode->pChildren = ( HB_SIZE * ) hb_xgrab( nNewCap * sizeof( HB_SIZE ) );

      pNode->nChildCapacity = nNewCap;
   }

   pNode->pChildren[ pNode->nChildCount++ ] = nChildId;
}

static void hb_astNodeAddToken( HB_AST_NODE_SERIAL * pNode, HB_SIZE nTokenId )
{
   if( pNode == NULL )
      return;

   if( pNode->nTokenCount == pNode->nTokenCapacity )
   {
      HB_SIZE nNewCap = pNode->nTokenCapacity == 0 ? 8 : pNode->nTokenCapacity << 1;

      if( pNode->pTokens )
         pNode->pTokens = ( HB_SIZE * ) hb_xrealloc( pNode->pTokens, nNewCap * sizeof( HB_SIZE ) );
      else
         pNode->pTokens = ( HB_SIZE * ) hb_xgrab( nNewCap * sizeof( HB_SIZE ) );

      pNode->nTokenCapacity = nNewCap;
   }

   pNode->pTokens[ pNode->nTokenCount++ ] = nTokenId;
}

static HB_BOOL hb_astModulesEqual( const char * pszA, const char * pszB )
{
   if( pszA == NULL || pszB == NULL )
      return HB_FALSE;

   return hb_stricmp( pszA, pszB ) == 0;
}

static HB_BOOL hb_astTokenBelongsToModule( const HB_AST_TOKEN * pToken, const char * pszModule )
{
   if( pszModule == NULL || *pszModule == '\0' )
      return HB_TRUE;

   if( pToken->pszModule && hb_astModulesEqual( pToken->pszModule, pszModule ) )
      return HB_TRUE;

   if( pToken->pMacroOrigin )
   {
      const char * pszCallModule = hb_astMacroTraceCallModule( pToken->pMacroOrigin );
      if( pszCallModule && hb_astModulesEqual( pszCallModule, pszModule ) )
         return HB_TRUE;
   }

   return HB_FALSE;
}

static void hb_astNodeSetRangeFromIndices( HB_AST_NODE_SERIAL * pNode,
                                           const HB_AST_TOKEN_STREAM * pStream,
                                           HB_SIZE nStart,
                                           HB_SIZE nEnd )
{
   const HB_AST_TOKEN * pStart;
   const HB_AST_TOKEN * pEnd;

   if( pNode == NULL || pStream == NULL || nStart >= hb_astTokenStreamCount( pStream ) ||
       nEnd >= hb_astTokenStreamCount( pStream ) || nStart > nEnd )
   {
      hb_xmemset( &pNode->range, 0, sizeof( pNode->range ) );
      return;
   }

   pStart = hb_astTokenStreamToken( pStream, nStart );
   pEnd   = hb_astTokenStreamToken( pStream, nEnd );

   if( pStart )
      pNode->range.start = pStart->original.start;
   else
      hb_xmemset( &pNode->range.start, 0, sizeof( pNode->range.start ) );

   if( pEnd )
      pNode->range.end = pEnd->original.end;
   else
      hb_xmemset( &pNode->range.end, 0, sizeof( pNode->range.end ) );
}

static void hb_astNodeAssignStableId( HB_AST_NODE_SERIAL * pNode, const char * pszModule )
{
   char buffer[ 512 ];
   const char * pszKind = pNode->pszKind ? pNode->pszKind : "Node";
   HB_SIZE nOffset = pNode->range.start.nOffset;

   hb_snprintf( buffer, sizeof( buffer ), "%s:%" HB_PFS "u:%s@%08" HB_PFS "u",
                pszModule ? pszModule : "", nOffset, pszKind, pNode->id );

   if( pNode->pszStableId )
      hb_xfree( pNode->pszStableId );
   pNode->pszStableId = hb_strdup( buffer );
}

static HB_BOOL hb_astIsKeyword( const HB_AST_TOKEN * pToken, const char * pszKeyword )
{
   return pToken &&
          pToken->kind == HB_AST_TOKEN_KIND_KEYWORD &&
          pToken->pszLexeme &&
          hb_stricmp( pToken->pszLexeme, pszKeyword ) == 0;
}

static void hb_astBuildNodes( const HB_AST_TOKEN_STREAM * pStream,
                              const char * pszModule,
                              HB_AST_NODE_LIST * pNodes )
{
   HB_AST_NODE_SERIAL * pRoot;
   HB_SIZE nTokenCount;
   HB_SIZE nFirstIdx = HB_SIZE_MAX;
   HB_SIZE nLastIdx  = 0;
   HB_SIZE nNextId = 1;
   HB_SIZE i;

   hb_astNodeListInit( pNodes );

   pRoot = hb_astNodeListAdd( pNodes, 0, "File" );
   pNodes->nRootId = pRoot->id;

   if( pStream == NULL )
   {
      hb_astNodeAssignStableId( pRoot, pszModule );
      return;
   }

   nTokenCount = hb_astTokenStreamCount( pStream );

   for( i = 0; i < nTokenCount; ++i )
   {
      const HB_AST_TOKEN * pToken = hb_astTokenStreamToken( pStream, i );

      if( pToken && hb_astTokenBelongsToModule( pToken, pszModule ) )
      {
         if( nFirstIdx == HB_SIZE_MAX )
            nFirstIdx = i;
         nLastIdx = i;
         hb_astNodeAddToken( pRoot, pToken->id.uHash );
      }
   }

   if( nFirstIdx != HB_SIZE_MAX )
      hb_astNodeSetRangeFromIndices( pRoot, pStream, nFirstIdx, nLastIdx );
   else
      hb_xmemset( &pRoot->range, 0, sizeof( pRoot->range ) );

   hb_astNodeAssignStableId( pRoot, pszModule );

   if( nFirstIdx == HB_SIZE_MAX )
      return;

   i = nFirstIdx;
   while( i <= nLastIdx && i < nTokenCount )
   {
      const HB_AST_TOKEN * pToken = hb_astTokenStreamToken( pStream, i );

      if( pToken == NULL || ! hb_astTokenBelongsToModule( pToken, pszModule ) )
      {
         ++i;
         continue;
      }

      if( hb_astIsKeyword( pToken, "PROC" ) || hb_astIsKeyword( pToken, "FUNCTION" ) )
      {
         HB_AST_NODE_SERIAL * pProcNode;
         const char * pszKind = hb_astIsKeyword( pToken, "PROC" ) ? "ProcDecl" : "FunctionDecl";
         HB_SIZE nStart = i;
         HB_SIZE nHeaderEnd = i;
         HB_SIZE nEnd = nLastIdx;
         HB_SIZE j;

         for( j = i; j <= nLastIdx && j < nTokenCount; ++j )
         {
            const HB_AST_TOKEN * pTemp = hb_astTokenStreamToken( pStream, j );
            if( pTemp && pTemp->kind == HB_AST_TOKEN_KIND_NEWLINE &&
                hb_astTokenBelongsToModule( pTemp, pszModule ) )
            {
               nHeaderEnd = j;
               break;
            }
         }

         for( j = i + 1; j <= nLastIdx && j < nTokenCount; ++j )
         {
            const HB_AST_TOKEN * pTemp = hb_astTokenStreamToken( pStream, j );
            if( pTemp && hb_astTokenBelongsToModule( pTemp, pszModule ) &&
                ( hb_astIsKeyword( pTemp, "PROC" ) || hb_astIsKeyword( pTemp, "FUNCTION" ) ) )
            {
               nEnd = j > 0 ? j - 1 : j;
               break;
            }
         }

         if( nEnd < nStart )
            nEnd = nStart;

         pProcNode = hb_astNodeListAdd( pNodes, nNextId++, pszKind );
         pProcNode->parentId = pRoot->id;
         hb_astNodeAddChild( pRoot, pProcNode->id );

         for( j = nStart; j <= nEnd && j < nTokenCount; ++j )
         {
            const HB_AST_TOKEN * pTemp = hb_astTokenStreamToken( pStream, j );
            if( pTemp && hb_astTokenBelongsToModule( pTemp, pszModule ) )
               hb_astNodeAddToken( pProcNode, pTemp->id.uHash );
         }

         hb_astNodeSetRangeFromIndices( pProcNode, pStream, nStart, nEnd );
         hb_astNodeAssignStableId( pProcNode, pszModule );

         if( nHeaderEnd < nEnd )
         {
            HB_SIZE nBodyIdx = nHeaderEnd + 1;

            while( nBodyIdx <= nEnd && nBodyIdx < nTokenCount )
            {
               const HB_AST_TOKEN * pBodyTok = hb_astTokenStreamToken( pStream, nBodyIdx );

               if( pBodyTok == NULL || ! hb_astTokenBelongsToModule( pBodyTok, pszModule ) )
               {
                  ++nBodyIdx;
                  continue;
               }

               if( hb_astIsKeyword( pBodyTok, "LOCAL" ) || hb_astIsKeyword( pBodyTok, "RETURN" ) )
               {
                  const char * pszStmtKind = hb_astIsKeyword( pBodyTok, "LOCAL" ) ?
                                             "LocalDecl" : "ReturnStmt";
                  HB_AST_NODE_SERIAL * pStmtNode;
                  HB_SIZE nStmtStart = nBodyIdx;
                  HB_SIZE nStmtEnd = nBodyIdx;
                  HB_SIZE nScan;

                  for( nScan = nBodyIdx; nScan <= nEnd && nScan < nTokenCount; ++nScan )
                  {
                     const HB_AST_TOKEN * pScanTok = hb_astTokenStreamToken( pStream, nScan );

                     if( pScanTok && pScanTok->kind == HB_AST_TOKEN_KIND_NEWLINE &&
                         hb_astTokenBelongsToModule( pScanTok, pszModule ) )
                     {
                        nStmtEnd = nScan;
                        break;
                     }
                  }

                  if( nStmtEnd < nStmtStart )
                     nStmtEnd = nStmtStart;
                  if( nStmtEnd > nEnd )
                     nStmtEnd = nEnd;

                  pStmtNode = hb_astNodeListAdd( pNodes, nNextId++, pszStmtKind );
                  pStmtNode->parentId = pProcNode->id;
                  hb_astNodeAddChild( pProcNode, pStmtNode->id );

                  for( nScan = nStmtStart; nScan <= nStmtEnd && nScan < nTokenCount; ++nScan )
                  {
                     const HB_AST_TOKEN * pStmtTok = hb_astTokenStreamToken( pStream, nScan );

                     if( pStmtTok && hb_astTokenBelongsToModule( pStmtTok, pszModule ) )
                        hb_astNodeAddToken( pStmtNode, pStmtTok->id.uHash );
                     else if( pStmtTok && pStmtTok->pMacroOrigin )
                     {
                        const char * pszCall = hb_astMacroTraceCallModule( pStmtTok->pMacroOrigin );
                        if( pszCall && pszModule && hb_astModulesEqual( pszCall, pszModule ) )
                           hb_astNodeAddToken( pStmtNode, pStmtTok->id.uHash );
                     }
                  }

                  hb_astNodeSetRangeFromIndices( pStmtNode, pStream, nStmtStart, nStmtEnd );
                  hb_astNodeAssignStableId( pStmtNode, pszModule );

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
}

static void hb_astJsonAppendNodes( HB_AST_JSON_BUFFER * pBuf, const HB_AST_NODE_LIST * pNodes )
{
   HB_SIZE i, j;

   hb_astJsonBufferAddCStr( pBuf, "\"nodes\":[" );
   for( i = 0; i < pNodes->nCount; ++i )
   {
      const HB_AST_NODE_SERIAL * pNode = &pNodes->pNodes[ i ];

      if( i > 0 )
         hb_astJsonBufferAddChar( pBuf, ',' );

      hb_astJsonBufferAddChar( pBuf, '{' );
      hb_astJsonBufferAddCStr( pBuf, "\"id\":" );
      hb_astJsonBufferAddUnsigned( pBuf, pNode->id );

      hb_astJsonBufferAddCStr( pBuf, ",\"kind\":" );
      hb_astJsonAddEscapedString( pBuf, pNode->pszKind );

      hb_astJsonBufferAddCStr( pBuf, ",\"range\":" );
      hb_astJsonAppendRange( pBuf, &pNode->range );

      hb_astJsonBufferAddCStr( pBuf, ",\"stable_id\":" );
      hb_astJsonAddEscapedString( pBuf, pNode->pszStableId );

      hb_astJsonBufferAddCStr( pBuf, ",\"parent\":" );
      if( pNode->parentId == HB_SIZE_MAX )
         hb_astJsonBufferAddCStr( pBuf, "null" );
      else
         hb_astJsonBufferAddUnsigned( pBuf, pNode->parentId );

      hb_astJsonBufferAddCStr( pBuf, ",\"children\":[" );
      for( j = 0; j < pNode->nChildCount; ++j )
      {
         if( j > 0 )
            hb_astJsonBufferAddChar( pBuf, ',' );
         hb_astJsonBufferAddUnsigned( pBuf, pNode->pChildren[ j ] );
      }
      hb_astJsonBufferAddChar( pBuf, ']' );

      hb_astJsonBufferAddCStr( pBuf, ",\"tokens\":[" );
      for( j = 0; j < pNode->nTokenCount; ++j )
      {
         if( j > 0 )
            hb_astJsonBufferAddChar( pBuf, ',' );
         hb_astJsonBufferAddUnsigned( pBuf, pNode->pTokens[ j ] );
      }
      hb_astJsonBufferAddChar( pBuf, ']' );

      hb_astJsonBufferAddChar( pBuf, '}' );
   }
   hb_astJsonBufferAddChar( pBuf, ']' );
}

static void hb_astJsonAppendMacrosObject( HB_AST_JSON_BUFFER * pBuf, const HB_AST_TOKEN_STREAM * pStream )
{
   HB_SIZE nTraceCount = hb_astTokenStreamMacroTraceCount( pStream );
   HB_SIZE i;

   hb_astJsonBufferAddCStr( pBuf, "\"expansions\":[" );

   for( i = 0; i < nTraceCount; ++i )
   {
      const void * pTrace = hb_astTokenStreamMacroTrace( pStream, i );
      HB_AST_SOURCE_RANGE callRange;
      const void * pParent;
      HB_SIZE nId = hb_astMacroTraceId( pTrace );

      if( nId == HB_SIZE_MAX )
         nId = i;

      if( i > 0 )
         hb_astJsonBufferAddChar( pBuf, ',' );

      hb_astJsonBufferAddChar( pBuf, '{' );
      hb_astJsonBufferAddCStr( pBuf, "\"expansion_id\":" );
      hb_astJsonBufferAddUnsigned( pBuf, nId );

      hb_astJsonBufferAddCStr( pBuf, ",\"macro_name\":" );
      hb_astJsonAddEscapedString( pBuf, hb_astMacroTraceName( pTrace ) );

      hb_astJsonBufferAddCStr( pBuf, ",\"call_module\":" );
      hb_astJsonAddEscapedString( pBuf, hb_astMacroTraceCallModule( pTrace ) );

      hb_astJsonBufferAddCStr( pBuf, ",\"depth\":" );
      hb_astJsonBufferAddUnsigned( pBuf, hb_astMacroTraceDepth( pTrace ) );

      pParent = hb_astMacroTraceParent( pTrace );
      hb_astJsonBufferAddCStr( pBuf, ",\"parent_expansion_id\":" );
      if( pParent )
      {
         HB_SIZE nParentId = hb_astMacroTraceId( pParent );
         if( nParentId == HB_SIZE_MAX )
            nParentId = nId;
         hb_astJsonBufferAddUnsigned( pBuf, nParentId );
      }
      else
         hb_astJsonBufferAddCStr( pBuf, "null" );

      callRange = hb_astMacroTraceCallRange( pTrace );
      hb_astJsonBufferAddCStr( pBuf, ",\"call_range\":" );
      hb_astJsonAppendRange( pBuf, &callRange );

      hb_astJsonBufferAddChar( pBuf, '}' );
   }

   hb_astJsonBufferAddChar( pBuf, ']' );
}

char * hb_astTokenStreamSerializeMacrosJson( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE * pnLength )
{
   HB_AST_JSON_BUFFER buffer;

   if( pStream == NULL )
      return NULL;

   hb_astJsonBufferInit( &buffer );
   hb_astJsonBufferAddChar( &buffer, '{' );
   hb_astJsonAppendMacrosObject( &buffer, pStream );
   hb_astJsonBufferAddChar( &buffer, '}' );
   hb_astJsonBufferAddChar( &buffer, '\0' );

   if( pnLength )
      *pnLength = buffer.nLen - 1;

   return buffer.pszData;
}

void hb_astTokenStreamSerializeMacrosJsonFree( char * pszJson )
{
   if( pszJson )
      hb_xfree( pszJson );
}

HB_BOOL hb_astTokenStreamWriteMacrosJson( const HB_AST_TOKEN_STREAM * pStream, const char * pszPath )
{
   char * pszJson;
   HB_SIZE nLen;
   HB_FHANDLE hFile;
   HB_BOOL fResult = HB_FALSE;

   if( pszPath == NULL )
      return HB_FALSE;

   pszJson = hb_astTokenStreamSerializeMacrosJson( pStream, &nLen );
   if( pszJson == NULL )
      return HB_FALSE;

   hFile = hb_fsCreate( pszPath, FC_NORMAL );
   if( hFile != FS_ERROR )
   {
      HB_FOFFSET nWritten = hb_fsWriteLarge( hFile, pszJson, ( HB_FOFFSET ) nLen );

      if( nWritten == ( HB_FOFFSET ) nLen )
         fResult = HB_TRUE;
      hb_fsClose( hFile );
   }

   hb_astTokenStreamSerializeMacrosJsonFree( pszJson );
   return fResult;
}

char * hb_astTokenStreamSerializeSnapshotJson( const HB_AST_TOKEN_STREAM * pStream,
                                               const char * pszModule,
                                               HB_SIZE * pnLength )
{
   HB_AST_JSON_BUFFER buffer;
   HB_AST_NODE_LIST nodes;
   HB_SIZE nTokenCount, i;

   if( pStream == NULL )
      return NULL;

   hb_astJsonBufferInit( &buffer );
   hb_astBuildNodes( pStream, pszModule, &nodes );

   hb_astJsonBufferAddChar( &buffer, '{' );
   hb_astJsonBufferAddCStr( &buffer, "\"root\":" );
   hb_astJsonBufferAddUnsigned( &buffer, nodes.nRootId );
   hb_astJsonBufferAddChar( &buffer, ',' );
   hb_astJsonAppendNodes( &buffer, &nodes );
   hb_astJsonBufferAddChar( &buffer, ',' );

   hb_astJsonBufferAddCStr( &buffer, "\"token_stream\":{\"tokens\":[" );

   nTokenCount = hb_astTokenStreamCount( pStream );
   for( i = 0; i < nTokenCount; ++i )
   {
      const HB_AST_TOKEN * pToken = hb_astTokenStreamToken( pStream, i );
      const void * pTrace = pToken ? pToken->pMacroOrigin : NULL;

      if( i > 0 )
         hb_astJsonBufferAddChar( &buffer, ',' );

      hb_astJsonBufferAddChar( &buffer, '{' );
      hb_astJsonBufferAddCStr( &buffer, "\"id\":" );
      hb_astJsonBufferAddUnsigned( &buffer, pToken->id.uHash );

      hb_astJsonBufferAddCStr( &buffer, ",\"kind\":" );
      hb_astJsonBufferAddUnsigned( &buffer, pToken->kind );

      hb_astJsonBufferAddCStr( &buffer, ",\"pp_type\":" );
      hb_astJsonBufferAddUnsigned( &buffer, pToken->uPPType );

      hb_astJsonBufferAddCStr( &buffer, ",\"channel\":" );
      hb_astJsonBufferAddUnsigned( &buffer, pToken->uChannel );

      hb_astJsonBufferAddCStr( &buffer, ",\"macro_depth\":" );
      hb_astJsonBufferAddUnsigned( &buffer, pToken->id.nMacroDepth );

      hb_astJsonBufferAddCStr( &buffer, ",\"lexeme\":" );
      hb_astJsonAddEscapedString( &buffer, pToken->pszLexeme );

      hb_astJsonBufferAddCStr( &buffer, ",\"length\":" );
      hb_astJsonBufferAddUnsigned( &buffer, pToken->nLexemeLength );

      hb_astJsonBufferAddCStr( &buffer, ",\"module\":" );
      hb_astJsonAddEscapedString( &buffer, pToken->pszModule );

      hb_astJsonBufferAddCStr( &buffer, ",\"original\":" );
      hb_astJsonAppendRange( &buffer, &pToken->original );

      hb_astJsonBufferAddCStr( &buffer, ",\"expanded\":" );
      hb_astJsonAppendRange( &buffer, &pToken->expanded );

      hb_astJsonBufferAddCStr( &buffer, ",\"macro_expansion_id\":" );
      if( pTrace )
      {
         HB_SIZE nId = hb_astMacroTraceId( pTrace );
         if( nId == HB_SIZE_MAX )
            nId = i;
         hb_astJsonBufferAddUnsigned( &buffer, nId );
      }
      else
         hb_astJsonBufferAddCStr( &buffer, "null" );

      hb_astJsonBufferAddChar( &buffer, '}' );
   }

   hb_astJsonBufferAddCStr( &buffer, "]}" );
   hb_astJsonBufferAddChar( &buffer, '}' );
   hb_astJsonBufferAddChar( &buffer, '\0' );

   if( pnLength )
      *pnLength = buffer.nLen - 1;

   hb_astNodeListRelease( &nodes );

   return buffer.pszData;
}

void hb_astTokenStreamSerializeSnapshotJsonFree( char * pszJson )
{
   if( pszJson )
      hb_xfree( pszJson );
}

HB_BOOL hb_astTokenStreamWriteSnapshotJson( const HB_AST_TOKEN_STREAM * pStream,
                                            const char * pszModule,
                                            const char * pszPath )
{
   char * pszJson;
   HB_SIZE nLen;
   HB_FHANDLE hFile;
   HB_BOOL fResult = HB_FALSE;

   if( pszPath == NULL )
      return HB_FALSE;

   pszJson = hb_astTokenStreamSerializeSnapshotJson( pStream, pszModule, &nLen );
   if( pszJson == NULL )
      return HB_FALSE;

   hFile = hb_fsCreate( pszPath, FC_NORMAL );
   if( hFile != FS_ERROR )
   {
      HB_FOFFSET nWritten = hb_fsWriteLarge( hFile, pszJson, ( HB_FOFFSET ) nLen );

      if( nWritten == ( HB_FOFFSET ) nLen )
         fResult = HB_TRUE;
      hb_fsClose( hFile );
   }

   hb_astTokenStreamSerializeSnapshotJsonFree( pszJson );
   return fResult;
}

typedef struct _HB_AST_CBOR_BUFFER
{
   HB_BYTE * pData;
   HB_SIZE   nLen;
   HB_SIZE   nCapacity;
} HB_AST_CBOR_BUFFER;

static void hb_astCborBufferInit( HB_AST_CBOR_BUFFER * pBuf )
{
   pBuf->nCapacity = 256;
   pBuf->nLen = 0;
   pBuf->pData = ( HB_BYTE * ) hb_xgrab( pBuf->nCapacity );
}

static void hb_astCborBufferEnsure( HB_AST_CBOR_BUFFER * pBuf, HB_SIZE nExtra )
{
   HB_SIZE nNeeded = pBuf->nLen + nExtra;

   if( nNeeded > pBuf->nCapacity )
   {
      while( pBuf->nCapacity < nNeeded )
         pBuf->nCapacity <<= 1;
      pBuf->pData = ( HB_BYTE * ) hb_xrealloc( pBuf->pData, pBuf->nCapacity );
   }
}

static void hb_astCborBufferAddByte( HB_AST_CBOR_BUFFER * pBuf, HB_BYTE value )
{
   hb_astCborBufferEnsure( pBuf, 1 );
   pBuf->pData[ pBuf->nLen++ ] = value;
}

static void hb_astCborBufferAddData( HB_AST_CBOR_BUFFER * pBuf, const HB_BYTE * pData, HB_SIZE nLen )
{
   hb_astCborBufferEnsure( pBuf, nLen );
   memcpy( pBuf->pData + pBuf->nLen, pData, nLen );
   pBuf->nLen += nLen;
}

static void hb_astCborAppendTypeLength( HB_AST_CBOR_BUFFER * pBuf, HB_U8 major, HB_U64 value )
{
   if( value <= 23 )
   {
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | ( HB_U8 ) value ) );
   }
   else if( value <= 0xFF )
   {
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | 24 ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) value );
   }
   else if( value <= 0xFFFF )
   {
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | 25 ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 8 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( value & 0xFF ) );
   }
   else if( value <= 0xFFFFFFFFULL )
   {
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | 26 ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 24 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 16 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 8 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( value & 0xFF ) );
   }
   else
   {
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | 27 ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 56 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 48 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 40 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 32 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 24 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 16 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( ( value >> 8 ) & 0xFF ) );
      hb_astCborBufferAddByte( pBuf, ( HB_BYTE ) ( value & 0xFF ) );
   }
}

static void hb_astCborEncodeUnsigned( HB_AST_CBOR_BUFFER * pBuf, HB_SIZE value )
{
   hb_astCborAppendTypeLength( pBuf, 0, ( HB_U64 ) value );
}

static void hb_astCborEncodeText( HB_AST_CBOR_BUFFER * pBuf, const char * pszValue )
{
   if( pszValue == NULL )
   {
      hb_astCborBufferAddByte( pBuf, 0xF6 ); /* null */
      return;
   }

   HB_SIZE nLen = strlen( pszValue );
   hb_astCborAppendTypeLength( pBuf, 3, nLen );
   hb_astCborBufferAddData( pBuf, ( const HB_BYTE * ) pszValue, nLen );
}

static void hb_astCborEncodeArrayStart( HB_AST_CBOR_BUFFER * pBuf, HB_SIZE nItems )
{
   hb_astCborAppendTypeLength( pBuf, 4, ( HB_U64 ) nItems );
}

static void hb_astCborEncodeMapStart( HB_AST_CBOR_BUFFER * pBuf, HB_SIZE nPairs )
{
   hb_astCborAppendTypeLength( pBuf, 5, ( HB_U64 ) nPairs );
}

static void hb_astCborEncodeNull( HB_AST_CBOR_BUFFER * pBuf )
{
   hb_astCborBufferAddByte( pBuf, 0xF6 );
}

static void hb_astCborEncodeRange( HB_AST_CBOR_BUFFER * pBuf, const HB_AST_SOURCE_RANGE * pRange )
{
   hb_astCborEncodeMapStart( pBuf, 2 );
   hb_astCborEncodeText( pBuf, "start" );
   hb_astCborEncodeMapStart( pBuf, 3 );
   hb_astCborEncodeText( pBuf, "line" );
   hb_astCborEncodeUnsigned( pBuf, pRange->start.nLine );
   hb_astCborEncodeText( pBuf, "column" );
   hb_astCborEncodeUnsigned( pBuf, pRange->start.nColumn );
   hb_astCborEncodeText( pBuf, "offset" );
   hb_astCborEncodeUnsigned( pBuf, pRange->start.nOffset );

   hb_astCborEncodeText( pBuf, "end" );
   hb_astCborEncodeMapStart( pBuf, 3 );
   hb_astCborEncodeText( pBuf, "line" );
   hb_astCborEncodeUnsigned( pBuf, pRange->end.nLine );
   hb_astCborEncodeText( pBuf, "column" );
   hb_astCborEncodeUnsigned( pBuf, pRange->end.nColumn );
   hb_astCborEncodeText( pBuf, "offset" );
   hb_astCborEncodeUnsigned( pBuf, pRange->end.nOffset );
}

static void hb_astCborEncodeNodes( HB_AST_CBOR_BUFFER * pBuf, const HB_AST_NODE_LIST * pNodes )
{
   HB_SIZE i, j;

   hb_astCborEncodeArrayStart( pBuf, pNodes->nCount );
   for( i = 0; i < pNodes->nCount; ++i )
   {
      const HB_AST_NODE_SERIAL * pNode = &pNodes->pNodes[ i ];

      hb_astCborEncodeMapStart( pBuf, 7 );
      hb_astCborEncodeText( pBuf, "id" );
      hb_astCborEncodeUnsigned( pBuf, pNode->id );

      hb_astCborEncodeText( pBuf, "kind" );
      hb_astCborEncodeText( pBuf, pNode->pszKind );

      hb_astCborEncodeText( pBuf, "range" );
      hb_astCborEncodeRange( pBuf, &pNode->range );

      hb_astCborEncodeText( pBuf, "stable_id" );
      hb_astCborEncodeText( pBuf, pNode->pszStableId );

      hb_astCborEncodeText( pBuf, "parent" );
      if( pNode->parentId == HB_SIZE_MAX )
         hb_astCborEncodeNull( pBuf );
      else
         hb_astCborEncodeUnsigned( pBuf, pNode->parentId );

      hb_astCborEncodeText( pBuf, "children" );
      hb_astCborEncodeArrayStart( pBuf, pNode->nChildCount );
      for( j = 0; j < pNode->nChildCount; ++j )
         hb_astCborEncodeUnsigned( pBuf, pNode->pChildren[ j ] );

      hb_astCborEncodeText( pBuf, "tokens" );
      hb_astCborEncodeArrayStart( pBuf, pNode->nTokenCount );
      for( j = 0; j < pNode->nTokenCount; ++j )
         hb_astCborEncodeUnsigned( pBuf, pNode->pTokens[ j ] );
   }
}

static void hb_astCborEncodeTokenStream( HB_AST_CBOR_BUFFER * pBuf, const HB_AST_TOKEN_STREAM * pStream )
{
   HB_SIZE nTokenCount, i;

   hb_astCborEncodeMapStart( pBuf, 1 );
   hb_astCborEncodeText( pBuf, "tokens" );

   nTokenCount = hb_astTokenStreamCount( pStream );
   hb_astCborEncodeArrayStart( pBuf, nTokenCount );

   for( i = 0; i < nTokenCount; ++i )
   {
      const HB_AST_TOKEN * pToken = hb_astTokenStreamToken( pStream, i );
      const void * pTrace = pToken ? pToken->pMacroOrigin : NULL;

      hb_astCborEncodeMapStart( pBuf, 11 );

      hb_astCborEncodeText( pBuf, "id" );
      hb_astCborEncodeUnsigned( pBuf, pToken->id.uHash );

      hb_astCborEncodeText( pBuf, "kind" );
      hb_astCborEncodeUnsigned( pBuf, pToken->kind );

      hb_astCborEncodeText( pBuf, "pp_type" );
      hb_astCborEncodeUnsigned( pBuf, pToken->uPPType );

      hb_astCborEncodeText( pBuf, "channel" );
      hb_astCborEncodeUnsigned( pBuf, pToken->uChannel );

      hb_astCborEncodeText( pBuf, "macro_depth" );
      hb_astCborEncodeUnsigned( pBuf, pToken->id.nMacroDepth );

      hb_astCborEncodeText( pBuf, "lexeme" );
      hb_astCborEncodeText( pBuf, pToken->pszLexeme );

      hb_astCborEncodeText( pBuf, "length" );
      hb_astCborEncodeUnsigned( pBuf, pToken->nLexemeLength );

      hb_astCborEncodeText( pBuf, "module" );
      hb_astCborEncodeText( pBuf, pToken->pszModule );

      hb_astCborEncodeText( pBuf, "original" );
      hb_astCborEncodeRange( pBuf, &pToken->original );

      hb_astCborEncodeText( pBuf, "expanded" );
      hb_astCborEncodeRange( pBuf, &pToken->expanded );

      hb_astCborEncodeText( pBuf, "macro_expansion_id" );
      if( pTrace )
      {
         HB_SIZE nId = hb_astMacroTraceId( pTrace );
         if( nId == HB_SIZE_MAX )
            nId = i;
         hb_astCborEncodeUnsigned( pBuf, nId );
      }
      else
         hb_astCborEncodeNull( pBuf );
   }
}

static void hb_astCborEncodeMacros( HB_AST_CBOR_BUFFER * pBuf, const HB_AST_TOKEN_STREAM * pStream )
{
   HB_SIZE nTraceCount = hb_astTokenStreamMacroTraceCount( pStream );
   HB_SIZE i;

   hb_astCborEncodeMapStart( pBuf, 1 );
   hb_astCborEncodeText( pBuf, "expansions" );
   hb_astCborEncodeArrayStart( pBuf, nTraceCount );

   for( i = 0; i < nTraceCount; ++i )
   {
      const void * pTrace = hb_astTokenStreamMacroTrace( pStream, i );
      HB_AST_SOURCE_RANGE callRange;
      const void * pParent;
      HB_SIZE nId = hb_astMacroTraceId( pTrace );

      if( nId == HB_SIZE_MAX )
         nId = i;

      hb_astCborEncodeMapStart( pBuf, 6 );

      hb_astCborEncodeText( pBuf, "expansion_id" );
      hb_astCborEncodeUnsigned( pBuf, nId );

      hb_astCborEncodeText( pBuf, "macro_name" );
      hb_astCborEncodeText( pBuf, hb_astMacroTraceName( pTrace ) );

      hb_astCborEncodeText( pBuf, "call_module" );
      hb_astCborEncodeText( pBuf, hb_astMacroTraceCallModule( pTrace ) );

      hb_astCborEncodeText( pBuf, "depth" );
      hb_astCborEncodeUnsigned( pBuf, hb_astMacroTraceDepth( pTrace ) );

      hb_astCborEncodeText( pBuf, "parent_expansion_id" );
      pParent = hb_astMacroTraceParent( pTrace );
      if( pParent )
      {
         HB_SIZE nParentId = hb_astMacroTraceId( pParent );
         if( nParentId == HB_SIZE_MAX )
            nParentId = nId;
         hb_astCborEncodeUnsigned( pBuf, nParentId );
      }
      else
         hb_astCborEncodeNull( pBuf );

      hb_astCborEncodeText( pBuf, "call_range" );
      callRange = hb_astMacroTraceCallRange( pTrace );
      hb_astCborEncodeRange( pBuf, &callRange );
   }
}

HB_BYTE * hb_astTokenStreamSerializeSnapshotCbor( const HB_AST_TOKEN_STREAM * pStream,
                                                  const char * pszModule,
                                                  HB_SIZE * pnLength )
{
   HB_AST_CBOR_BUFFER buffer;
   HB_AST_NODE_LIST nodes;

   if( pStream == NULL )
      return NULL;

   hb_astCborBufferInit( &buffer );
   hb_astBuildNodes( pStream, pszModule, &nodes );

   hb_astCborEncodeMapStart( &buffer, 3 );
   hb_astCborEncodeText( &buffer, "root" );
   hb_astCborEncodeUnsigned( &buffer, nodes.nRootId );
   hb_astCborEncodeText( &buffer, "nodes" );
   hb_astCborEncodeNodes( &buffer, &nodes );
   hb_astCborEncodeText( &buffer, "token_stream" );
   hb_astCborEncodeTokenStream( &buffer, pStream );

   hb_astNodeListRelease( &nodes );

   if( pnLength )
      *pnLength = buffer.nLen;

   return buffer.pData;
}

void hb_astTokenStreamSerializeSnapshotCborFree( HB_BYTE * pBuffer )
{
   if( pBuffer )
      hb_xfree( pBuffer );
}

HB_BOOL hb_astTokenStreamWriteSnapshotCbor( const HB_AST_TOKEN_STREAM * pStream,
                                            const char * pszModule,
                                            const char * pszPath )
{
   HB_BYTE * pBuffer;
   HB_SIZE nLen;
   HB_FHANDLE hFile;
   HB_BOOL fResult = HB_FALSE;

   if( pszPath == NULL )
      return HB_FALSE;

   pBuffer = hb_astTokenStreamSerializeSnapshotCbor( pStream, pszModule, &nLen );
   if( pBuffer == NULL )
      return HB_FALSE;

   hFile = hb_fsCreate( pszPath, FC_NORMAL );
   if( hFile != FS_ERROR )
   {
      HB_FOFFSET nWritten = hb_fsWriteLarge( hFile, pBuffer, ( HB_FOFFSET ) nLen );
      if( nWritten == ( HB_FOFFSET ) nLen )
         fResult = HB_TRUE;
      hb_fsClose( hFile );
   }

   hb_astTokenStreamSerializeSnapshotCborFree( pBuffer );
   return fResult;
}

HB_BYTE * hb_astTokenStreamSerializeMacrosCbor( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE * pnLength )
{
   HB_AST_CBOR_BUFFER buffer;

   if( pStream == NULL )
      return NULL;

   hb_astCborBufferInit( &buffer );
   hb_astCborEncodeMacros( &buffer, pStream );

   if( pnLength )
      *pnLength = buffer.nLen;

   return buffer.pData;
}

void hb_astTokenStreamSerializeMacrosCborFree( HB_BYTE * pBuffer )
{
   if( pBuffer )
      hb_xfree( pBuffer );
}

HB_BOOL hb_astTokenStreamWriteMacrosCbor( const HB_AST_TOKEN_STREAM * pStream, const char * pszPath )
{
   HB_BYTE * pBuffer;
   HB_SIZE nLen;
   HB_FHANDLE hFile;
   HB_BOOL fResult = HB_FALSE;

   if( pszPath == NULL )
      return HB_FALSE;

   pBuffer = hb_astTokenStreamSerializeMacrosCbor( pStream, &nLen );
   if( pBuffer == NULL )
      return HB_FALSE;

   hFile = hb_fsCreate( pszPath, FC_NORMAL );
   if( hFile != FS_ERROR )
   {
      HB_FOFFSET nWritten = hb_fsWriteLarge( hFile, pBuffer, ( HB_FOFFSET ) nLen );
      if( nWritten == ( HB_FOFFSET ) nLen )
         fResult = HB_TRUE;
      hb_fsClose( hFile );
   }

   hb_astTokenStreamSerializeMacrosCborFree( pBuffer );
   return fResult;
}
