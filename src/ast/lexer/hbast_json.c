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
   char buffer[ 32 ];

   hb_snprintf( buffer, sizeof( buffer ), "%lu", ( unsigned long ) nValue );
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

static void hb_astJsonAppendMacroSection( HB_AST_JSON_BUFFER * pBuf, const HB_AST_TOKEN_STREAM * pStream )
{
   HB_SIZE nTraceCount = hb_astTokenStreamMacroTraceCount( pStream );
   HB_SIZE i;

   hb_astJsonBufferAddCStr( pBuf, "\"macros\":{\"expansions\":[" );

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
      if( pParent )
      {
         HB_SIZE nParentId = hb_astMacroTraceId( pParent );
         if( nParentId == HB_SIZE_MAX )
            nParentId = nId;
         hb_astJsonBufferAddCStr( pBuf, ",\"parent_expansion_id\":" );
         hb_astJsonBufferAddUnsigned( pBuf, nParentId );
      }

      callRange = hb_astMacroTraceCallRange( pTrace );
      hb_astJsonBufferAddCStr( pBuf, ",\"call_range\":" );
      hb_astJsonAppendRange( pBuf, &callRange );

      hb_astJsonBufferAddChar( pBuf, '}' );
   }

    hb_astJsonBufferAddChar( pBuf, ']' );
    hb_astJsonBufferAddChar( pBuf, '}' );
}

char * hb_astTokenStreamSerializeMacrosJson( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE * pnLength )
{
   HB_AST_JSON_BUFFER buffer;

   if( pStream == NULL )
      return NULL;

   hb_astJsonBufferInit( &buffer );
   hb_astJsonBufferAddChar( &buffer, '{' );
   hb_astJsonAppendMacroSection( &buffer, pStream );
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

char * hb_astTokenStreamSerializeSnapshotJson( const HB_AST_TOKEN_STREAM * pStream, const char * pszSourcePath, HB_SIZE * pnLength )
{
   HB_AST_JSON_BUFFER buffer;
   HB_SIZE nTokenCount, i;

   if( pStream == NULL )
      return NULL;

   hb_astJsonBufferInit( &buffer );

   hb_astJsonBufferAddCStr( &buffer, "{\"format_version\":\"0.0.1\",\"schema_revision\":1,\"generator\":{\"name\":\"hbast\",\"version\":\"0.0.1\"},\"files\":[{\"file_id\":1,\"path\":" );
   hb_astJsonAddEscapedString( &buffer, pszSourcePath );
   hb_astJsonBufferAddCStr( &buffer, ",\"hash\":\"\",\"ast\":{\"root\":0,\"nodes\":[],\"token_stream\":{\"tokens\":[" );

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

   hb_astJsonBufferAddCStr( &buffer, "]}}," );
   hb_astJsonAppendMacroSection( &buffer, pStream );
   hb_astJsonBufferAddChar( &buffer, '}' );   /* close file entry */
   hb_astJsonBufferAddChar( &buffer, ']' );   /* close files array */
   hb_astJsonBufferAddChar( &buffer, '}' );   /* close root */
   hb_astJsonBufferAddChar( &buffer, '\0' );

   if( pnLength )
      *pnLength = buffer.nLen - 1;

   return buffer.pszData;
}

void hb_astTokenStreamSerializeSnapshotJsonFree( char * pszJson )
{
   hb_astTokenStreamSerializeMacrosJsonFree( pszJson );
}

HB_BOOL hb_astTokenStreamWriteSnapshotJson( const HB_AST_TOKEN_STREAM * pStream, const char * pszSourcePath, const char * pszPath )
{
   char * pszJson;
   HB_SIZE nLen;
   HB_FHANDLE hFile;
   HB_BOOL fResult = HB_FALSE;

   if( pszPath == NULL )
      return HB_FALSE;

   pszJson = hb_astTokenStreamSerializeSnapshotJson( pStream, pszSourcePath, &nLen );
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
