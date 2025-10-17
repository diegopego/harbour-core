#include "ast/hbast_builder.h"
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

static void hb_astJsonAppendNodes( HB_AST_JSON_BUFFER * pBuf, const HB_AST_BUILD_RESULT * pBuild )
{
   HB_SIZE i, j;

   hb_astJsonBufferAddCStr( pBuf, "\"nodes\":[" );
   for( i = 0; i < pBuild->nNodeCount; ++i )
   {
      const HB_AST_NODE_INFO * pNode = &pBuild->pNodes[ i ];

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

      hb_astJsonBufferAddCStr( pBuf, ",\"symbol\":" );
      if( pNode->symbolId == HB_SIZE_MAX )
         hb_astJsonBufferAddCStr( pBuf, "null" );
      else
         hb_astJsonBufferAddUnsigned( pBuf, pNode->symbolId );

      hb_astJsonBufferAddChar( pBuf, '}' );
   }
   hb_astJsonBufferAddChar( pBuf, ']' );
}

static void hb_astJsonAppendTokenStream( HB_AST_JSON_BUFFER * pBuf, const HB_AST_TOKEN_STREAM * pStream )
{
   HB_SIZE nTokenCount, i;

   hb_astJsonBufferAddCStr( pBuf, "\"token_stream\":{\"tokens\":[" );

   nTokenCount = hb_astTokenStreamCount( pStream );
   for( i = 0; i < nTokenCount; ++i )
   {
      const HB_AST_TOKEN * pToken = hb_astTokenStreamToken( pStream, i );
      const void * pTrace = pToken ? pToken->pMacroOrigin : NULL;

      if( i > 0 )
         hb_astJsonBufferAddChar( pBuf, ',' );

      hb_astJsonBufferAddChar( pBuf, '{' );
      hb_astJsonBufferAddCStr( pBuf, "\"id\":" );
      hb_astJsonBufferAddUnsigned( pBuf, pToken->id.uHash );

      hb_astJsonBufferAddCStr( pBuf, ",\"kind\":" );
      hb_astJsonBufferAddUnsigned( pBuf, pToken->kind );

      hb_astJsonBufferAddCStr( pBuf, ",\"pp_type\":" );
      hb_astJsonBufferAddUnsigned( pBuf, pToken->uPPType );

      hb_astJsonBufferAddCStr( pBuf, ",\"channel\":" );
      hb_astJsonBufferAddUnsigned( pBuf, pToken->uChannel );

      hb_astJsonBufferAddCStr( pBuf, ",\"macro_depth\":" );
      hb_astJsonBufferAddUnsigned( pBuf, pToken->id.nMacroDepth );

      hb_astJsonBufferAddCStr( pBuf, ",\"lexeme\":" );
      hb_astJsonAddEscapedString( pBuf, pToken->pszLexeme );

      hb_astJsonBufferAddCStr( pBuf, ",\"length\":" );
      hb_astJsonBufferAddUnsigned( pBuf, pToken->nLexemeLength );

      hb_astJsonBufferAddCStr( pBuf, ",\"module\":" );
      hb_astJsonAddEscapedString( pBuf, pToken->pszModule );

      hb_astJsonBufferAddCStr( pBuf, ",\"original\":" );
      hb_astJsonAppendRange( pBuf, &pToken->original );

      hb_astJsonBufferAddCStr( pBuf, ",\"expanded\":" );
      hb_astJsonAppendRange( pBuf, &pToken->expanded );

      hb_astJsonBufferAddCStr( pBuf, ",\"macro_expansion_id\":" );
      if( pTrace )
      {
         HB_SIZE nId = hb_astMacroTraceId( pTrace );
         if( nId == HB_SIZE_MAX )
            nId = i;
         hb_astJsonBufferAddUnsigned( pBuf, nId );
      }
      else
         hb_astJsonBufferAddCStr( pBuf, "null" );

      hb_astJsonBufferAddChar( pBuf, '}' );
   }

   hb_astJsonBufferAddCStr( pBuf, "]}" );
}

static void hb_astJsonWriteSymbolsArray( HB_AST_JSON_BUFFER * pBuf, const HB_AST_BUILD_RESULT * pBuild )
{
   HB_SIZE i, j;

   hb_astJsonBufferAddChar( pBuf, '[' );
   for( i = 0; i < pBuild->nSymbolCount; ++i )
   {
      const HB_AST_SYMBOL_INFO * pSymbol = &pBuild->pSymbols[ i ];

      if( i > 0 )
         hb_astJsonBufferAddChar( pBuf, ',' );

      hb_astJsonBufferAddChar( pBuf, '{' );
      hb_astJsonBufferAddCStr( pBuf, "\"symbol_id\":" );
      hb_astJsonBufferAddUnsigned( pBuf, pSymbol->symbolId );

      hb_astJsonBufferAddCStr( pBuf, ",\"kind\":" );
      hb_astJsonAddEscapedString( pBuf, pSymbol->pszKind );

      hb_astJsonBufferAddCStr( pBuf, ",\"name\":" );
      hb_astJsonAddEscapedString( pBuf, pSymbol->pszName );

      hb_astJsonBufferAddCStr( pBuf, ",\"qualified_name\":" );
      hb_astJsonAddEscapedString( pBuf, pSymbol->pszQualifiedName );

      hb_astJsonBufferAddCStr( pBuf, ",\"declarations\":[" );
      for( j = 0; j < pSymbol->nDeclarationCount; ++j )
      {
         if( j > 0 )
            hb_astJsonBufferAddChar( pBuf, ',' );
         hb_astJsonBufferAddUnsigned( pBuf, pSymbol->pDeclarations[ j ] );
      }
      hb_astJsonBufferAddChar( pBuf, ']' );

      hb_astJsonBufferAddCStr( pBuf, ",\"references\":[" );
      for( j = 0; j < pSymbol->nReferenceCount; ++j )
      {
         if( j > 0 )
            hb_astJsonBufferAddChar( pBuf, ',' );
         hb_astJsonBufferAddUnsigned( pBuf, pSymbol->pReferences[ j ] );
      }
      hb_astJsonBufferAddChar( pBuf, ']' );

      hb_astJsonBufferAddCStr( pBuf, ",\"scope\":{\"parent\":0,\"kind\":\"Module\"}" );
      hb_astJsonBufferAddCStr( pBuf, ",\"annotations\":[]" );
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
   HB_AST_BUILD_RESULT build;

   if( pStream == NULL )
      return NULL;

   if( ! hb_astBuildFromStream( pStream, pszModule, &build ) )
      return NULL;

   hb_astJsonBufferInit( &buffer );

   hb_astJsonBufferAddChar( &buffer, '{' );
   hb_astJsonBufferAddCStr( &buffer, "\"root\":" );
   hb_astJsonBufferAddUnsigned( &buffer, build.nRootId );
   hb_astJsonBufferAddChar( &buffer, ',' );
   hb_astJsonAppendNodes( &buffer, &build );
   hb_astJsonBufferAddChar( &buffer, ',' );

   hb_astJsonAppendTokenStream( &buffer, pStream );
   hb_astJsonBufferAddChar( &buffer, '}' );
   hb_astJsonBufferAddChar( &buffer, '\0' );

   if( pnLength )
      *pnLength = buffer.nLen - 1;

   hb_astBuildResultRelease( &build );

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

char * hb_astTokenStreamSerializeSymbolsJson( const HB_AST_TOKEN_STREAM * pStream,
                                              const char * pszModule,
                                              HB_SIZE * pnLength )
{
   HB_AST_BUILD_RESULT build;
   HB_AST_JSON_BUFFER buffer;
   char * pszResult = NULL;

   if( pStream == NULL )
      return NULL;

   if( ! hb_astBuildFromStream( pStream, pszModule, &build ) )
      return NULL;

   hb_astJsonBufferInit( &buffer );
   hb_astJsonWriteSymbolsArray( &buffer, &build );
   hb_astJsonBufferAddChar( &buffer, '\0' );

   if( pnLength )
      *pnLength = buffer.nLen - 1;

   pszResult = buffer.pszData;
   hb_astBuildResultRelease( &build );
   return pszResult;
}

void hb_astTokenStreamSerializeSymbolsJsonFree( char * pszJson )
{
   if( pszJson )
      hb_xfree( pszJson );
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

static void hb_astCborEncodeTokenStream( HB_AST_CBOR_BUFFER * pBuf, const HB_AST_TOKEN_STREAM * pStream )
{
   HB_SIZE nTokenCount = hb_astTokenStreamCount( pStream );
   HB_SIZE i;

   hb_astCborEncodeMapStart( pBuf, 1 );
   hb_astCborEncodeText( pBuf, "tokens" );
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

static void hb_astCborEncodeNodes( HB_AST_CBOR_BUFFER * pBuf, const HB_AST_BUILD_RESULT * pBuild )
{
   HB_SIZE i, j;

   hb_astCborEncodeArrayStart( pBuf, pBuild->nNodeCount );
   for( i = 0; i < pBuild->nNodeCount; ++i )
   {
      const HB_AST_NODE_INFO * pNode = &pBuild->pNodes[ i ];

      hb_astCborEncodeMapStart( pBuf, 8 );
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

      hb_astCborEncodeText( pBuf, "symbol" );
      if( pNode->symbolId == HB_SIZE_MAX )
         hb_astCborEncodeNull( pBuf );
      else
         hb_astCborEncodeUnsigned( pBuf, pNode->symbolId );
   }
}

static void hb_astCborEncodeSymbols( HB_AST_CBOR_BUFFER * pBuf, const HB_AST_BUILD_RESULT * pBuild )
{
   HB_SIZE i, j;

   hb_astCborEncodeArrayStart( pBuf, pBuild->nSymbolCount );
   for( i = 0; i < pBuild->nSymbolCount; ++i )
   {
      const HB_AST_SYMBOL_INFO * pSymbol = &pBuild->pSymbols[ i ];

      hb_astCborEncodeMapStart( pBuf, 8 );

      hb_astCborEncodeText( pBuf, "symbol_id" );
      hb_astCborEncodeUnsigned( pBuf, pSymbol->symbolId );

      hb_astCborEncodeText( pBuf, "kind" );
      hb_astCborEncodeText( pBuf, pSymbol->pszKind );

      hb_astCborEncodeText( pBuf, "name" );
      hb_astCborEncodeText( pBuf, pSymbol->pszName );

      hb_astCborEncodeText( pBuf, "qualified_name" );
      hb_astCborEncodeText( pBuf, pSymbol->pszQualifiedName );

      hb_astCborEncodeText( pBuf, "declarations" );
      hb_astCborEncodeArrayStart( pBuf, pSymbol->nDeclarationCount );
      for( j = 0; j < pSymbol->nDeclarationCount; ++j )
         hb_astCborEncodeUnsigned( pBuf, pSymbol->pDeclarations[ j ] );

      hb_astCborEncodeText( pBuf, "references" );
      hb_astCborEncodeArrayStart( pBuf, pSymbol->nReferenceCount );
      for( j = 0; j < pSymbol->nReferenceCount; ++j )
         hb_astCborEncodeUnsigned( pBuf, pSymbol->pReferences[ j ] );

      hb_astCborEncodeText( pBuf, "scope" );
      hb_astCborEncodeMapStart( pBuf, 2 );
      hb_astCborEncodeText( pBuf, "parent" );
      hb_astCborEncodeUnsigned( pBuf, 0 );
      hb_astCborEncodeText( pBuf, "kind" );
      hb_astCborEncodeText( pBuf, "Module" );

      hb_astCborEncodeText( pBuf, "annotations" );
      hb_astCborEncodeArrayStart( pBuf, 0 );
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
   HB_AST_BUILD_RESULT build;
   HB_BYTE * pResult = NULL;

   if( pStream == NULL )
      return NULL;

   if( ! hb_astBuildFromStream( pStream, pszModule, &build ) )
      return NULL;

   hb_astCborBufferInit( &buffer );

   hb_astCborEncodeMapStart( &buffer, 3 );
   hb_astCborEncodeText( &buffer, "root" );
   hb_astCborEncodeUnsigned( &buffer, build.nRootId );
   hb_astCborEncodeText( &buffer, "nodes" );
   hb_astCborEncodeNodes( &buffer, &build );
   hb_astCborEncodeText( &buffer, "token_stream" );
   hb_astCborEncodeTokenStream( &buffer, pStream );

   if( pnLength )
      *pnLength = buffer.nLen;

   pResult = buffer.pData;
   hb_astBuildResultRelease( &build );
   return pResult;
}

void hb_astTokenStreamSerializeSnapshotCborFree( HB_BYTE * pBuffer )
{
   if( pBuffer )
      hb_xfree( pBuffer );
}

HB_BYTE * hb_astTokenStreamSerializeSymbolsCbor( const HB_AST_TOKEN_STREAM * pStream,
                                                 const char * pszModule,
                                                 HB_SIZE * pnLength )
{
   HB_AST_BUILD_RESULT build;
   HB_AST_CBOR_BUFFER buffer;
   HB_BYTE * pResult = NULL;

   if( pStream == NULL )
      return NULL;

   if( ! hb_astBuildFromStream( pStream, pszModule, &build ) )
      return NULL;

   hb_astCborBufferInit( &buffer );
   hb_astCborEncodeSymbols( &buffer, &build );

   if( pnLength )
      *pnLength = buffer.nLen;

   pResult = buffer.pData;
   hb_astBuildResultRelease( &build );
   return pResult;
}

void hb_astTokenStreamSerializeSymbolsCborFree( HB_BYTE * pBuffer )
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
