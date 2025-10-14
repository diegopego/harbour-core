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

char * hb_astTokenStreamSerializeMacrosJson( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE * pnLength )
{
   HB_AST_JSON_BUFFER buffer;
   HB_SIZE nTraceCount, i;

   if( pStream == NULL )
      return NULL;

   hb_astJsonBufferInit( &buffer );
   hb_astJsonBufferAddCStr( &buffer, "{\"macros\":{\"expansions\":[" );

   nTraceCount = hb_astTokenStreamMacroTraceCount( pStream );

   for( i = 0; i < nTraceCount; ++i )
   {
      const void * pTrace = hb_astTokenStreamMacroTrace( pStream, i );
      HB_AST_SOURCE_RANGE callRange;
      const void * pParent;

      if( i > 0 )
         hb_astJsonBufferAddChar( &buffer, ',' );

      hb_astJsonBufferAddChar( &buffer, '{' );
      hb_astJsonBufferAddCStr( &buffer, "\"expansion_id\":" );
      hb_astJsonBufferAddUnsigned( &buffer, hb_astMacroTraceId( pTrace ) );

      hb_astJsonBufferAddCStr( &buffer, ",\"macro_name\":" );
      hb_astJsonAddEscapedString( &buffer, hb_astMacroTraceName( pTrace ) );

      hb_astJsonBufferAddCStr( &buffer, ",\"call_module\":" );
      hb_astJsonAddEscapedString( &buffer, hb_astMacroTraceCallModule( pTrace ) );

      hb_astJsonBufferAddCStr( &buffer, ",\"depth\":" );
      hb_astJsonBufferAddUnsigned( &buffer, hb_astMacroTraceDepth( pTrace ) );

      pParent = hb_astMacroTraceParent( pTrace );
      if( pParent )
      {
         hb_astJsonBufferAddCStr( &buffer, ",\"parent_expansion_id\":" );
         hb_astJsonBufferAddUnsigned( &buffer, hb_astMacroTraceId( pParent ) );
      }

      callRange = hb_astMacroTraceCallRange( pTrace );
      hb_astJsonBufferAddCStr( &buffer, ",\"call_range\":{" );

      hb_astJsonBufferAddCStr( &buffer, "\"start\":{" );
      hb_astJsonBufferAddCStr( &buffer, "\"line\":" );
      hb_astJsonBufferAddUnsigned( &buffer, callRange.start.nLine );
      hb_astJsonBufferAddCStr( &buffer, ",\"column\":" );
      hb_astJsonBufferAddUnsigned( &buffer, callRange.start.nColumn );
      hb_astJsonBufferAddCStr( &buffer, ",\"offset\":" );
      hb_astJsonBufferAddUnsigned( &buffer, callRange.start.nOffset );
      hb_astJsonBufferAddChar( &buffer, '}' );

      hb_astJsonBufferAddCStr( &buffer, ",\"end\":{" );
      hb_astJsonBufferAddCStr( &buffer, "\"line\":" );
      hb_astJsonBufferAddUnsigned( &buffer, callRange.end.nLine );
      hb_astJsonBufferAddCStr( &buffer, ",\"column\":" );
      hb_astJsonBufferAddUnsigned( &buffer, callRange.end.nColumn );
      hb_astJsonBufferAddCStr( &buffer, ",\"offset\":" );
      hb_astJsonBufferAddUnsigned( &buffer, callRange.end.nOffset );
      hb_astJsonBufferAddChar( &buffer, '}' );

      hb_astJsonBufferAddChar( &buffer, '}' ); /* close call_range */

      hb_astJsonBufferAddChar( &buffer, '}' );
   }

   hb_astJsonBufferAddCStr( &buffer, "]}}" );
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
