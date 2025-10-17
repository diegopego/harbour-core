#include "ast/lexer/hbast_lexer.h"
#include "hbapifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _HB_AST_PAYLOAD_CBOR_BUFFER
{
   HB_BYTE * pData;
   HB_SIZE   nLen;
   HB_SIZE   nCapacity;
} HB_AST_PAYLOAD_CBOR_BUFFER;

static void hb_astCliCborInit( HB_AST_PAYLOAD_CBOR_BUFFER * pBuf )
{
   pBuf->nCapacity = 256;
   pBuf->nLen = 0;
   pBuf->pData = ( HB_BYTE * ) hb_xgrab( pBuf->nCapacity );
}

static void hb_astCliCborEnsure( HB_AST_PAYLOAD_CBOR_BUFFER * pBuf, HB_SIZE nExtra )
{
   HB_SIZE nNeeded = pBuf->nLen + nExtra;

   if( nNeeded > pBuf->nCapacity )
   {
      while( pBuf->nCapacity < nNeeded )
         pBuf->nCapacity <<= 1;
      pBuf->pData = ( HB_BYTE * ) hb_xrealloc( pBuf->pData, pBuf->nCapacity );
   }
}

static void hb_astCliCborAddByte( HB_AST_PAYLOAD_CBOR_BUFFER * pBuf, HB_BYTE value )
{
   hb_astCliCborEnsure( pBuf, 1 );
   pBuf->pData[ pBuf->nLen++ ] = value;
}

static void hb_astCliCborAddData( HB_AST_PAYLOAD_CBOR_BUFFER * pBuf, const HB_BYTE * pData, HB_SIZE nLen )
{
   hb_astCliCborEnsure( pBuf, nLen );
   memcpy( pBuf->pData + pBuf->nLen, pData, nLen );
   pBuf->nLen += nLen;
}

static void hb_astCliCborAppendTypeLength( HB_AST_PAYLOAD_CBOR_BUFFER * pBuf, HB_U8 major, HB_U64 value )
{
   if( value <= 23 )
   {
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | ( HB_U8 ) value ) );
   }
   else if( value <= 0xFF )
   {
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | 24 ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) value );
   }
   else if( value <= 0xFFFF )
   {
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | 25 ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 8 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( value & 0xFF ) );
   }
   else if( value <= 0xFFFFFFFFULL )
   {
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | 26 ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 24 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 16 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 8 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( value & 0xFF ) );
   }
   else
   {
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( major << 5 ) | 27 ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 56 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 48 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 40 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 32 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 24 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 16 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( ( value >> 8 ) & 0xFF ) );
      hb_astCliCborAddByte( pBuf, ( HB_BYTE ) ( value & 0xFF ) );
   }
}

static void hb_astCliCborEncodeUnsigned( HB_AST_PAYLOAD_CBOR_BUFFER * pBuf, HB_SIZE value )
{
   hb_astCliCborAppendTypeLength( pBuf, 0, ( HB_U64 ) value );
}

static void hb_astCliCborEncodeText( HB_AST_PAYLOAD_CBOR_BUFFER * pBuf, const char * pszValue )
{
   if( pszValue == NULL )
   {
      hb_astCliCborAddByte( pBuf, 0xF6 );
      return;
   }

   HB_SIZE nLen = strlen( pszValue );
   hb_astCliCborAppendTypeLength( pBuf, 3, nLen );
   hb_astCliCborAddData( pBuf, ( const HB_BYTE * ) pszValue, nLen );
}

static void hb_astCliCborEncodeArrayStart( HB_AST_PAYLOAD_CBOR_BUFFER * pBuf, HB_SIZE nItems )
{
   hb_astCliCborAppendTypeLength( pBuf, 4, ( HB_U64 ) nItems );
}

static void hb_astCliCborEncodeMapStart( HB_AST_PAYLOAD_CBOR_BUFFER * pBuf, HB_SIZE nPairs )
{
   hb_astCliCborAppendTypeLength( pBuf, 5, ( HB_U64 ) nPairs );
}

static HB_BYTE * hb_astBuildPayloadCbor( const HB_AST_TOKEN_STREAM * pStream,
                                         const char * pszModule,
                                         const char * pszInputPath,
                                         HB_SIZE * pnLength )
{
   HB_AST_PAYLOAD_CBOR_BUFFER buffer;
   HB_BYTE * pAst = NULL;
   HB_BYTE * pMacros = NULL;
   HB_BYTE * pSymbols = NULL;
   HB_SIZE nAstLen = 0, nMacrosLen = 0, nSymbolsLen = 0;
   HB_BOOL fOk = HB_TRUE;

   if( pStream == NULL )
      return NULL;

   hb_xmemset( &buffer, 0, sizeof( buffer ) );

   pAst = hb_astTokenStreamSerializeSnapshotCbor( pStream, pszModule, &nAstLen );
   pMacros = hb_astTokenStreamSerializeMacrosCbor( pStream, &nMacrosLen );
   pSymbols = hb_astTokenStreamSerializeSymbolsCbor( pStream, pszModule, &nSymbolsLen );

   if( pAst == NULL || pMacros == NULL || pSymbols == NULL )
   {
      fOk = HB_FALSE;
   }
   else
   {
      hb_astCliCborInit( &buffer );

      hb_astCliCborEncodeMapStart( &buffer, 6 );

      hb_astCliCborEncodeText( &buffer, "format_version" );
      hb_astCliCborEncodeText( &buffer, "0.1.0" );

      hb_astCliCborEncodeText( &buffer, "schema_revision" );
      hb_astCliCborEncodeUnsigned( &buffer, 1 );

      hb_astCliCborEncodeText( &buffer, "generator" );
      hb_astCliCborEncodeMapStart( &buffer, 2 );
      hb_astCliCborEncodeText( &buffer, "name" );
      hb_astCliCborEncodeText( &buffer, "hbast" );
      hb_astCliCborEncodeText( &buffer, "version" );
      hb_astCliCborEncodeText( &buffer, "0.1.0" );

      hb_astCliCborEncodeText( &buffer, "project" );
      hb_astCliCborEncodeMapStart( &buffer, 3 );
      hb_astCliCborEncodeText( &buffer, "root" );
      hb_astCliCborEncodeText( &buffer, "" );
      hb_astCliCborEncodeText( &buffer, "dialect" );
      hb_astCliCborEncodeText( &buffer, "harbour" );
      hb_astCliCborEncodeText( &buffer, "flags" );
      hb_astCliCborEncodeArrayStart( &buffer, 0 );

      hb_astCliCborEncodeText( &buffer, "files" );
      hb_astCliCborEncodeArrayStart( &buffer, 1 );
      hb_astCliCborEncodeMapStart( &buffer, 5 );

      hb_astCliCborEncodeText( &buffer, "file_id" );
      hb_astCliCborEncodeUnsigned( &buffer, 1 );

      hb_astCliCborEncodeText( &buffer, "path" );
      hb_astCliCborEncodeText( &buffer, pszInputPath );

      hb_astCliCborEncodeText( &buffer, "hash" );
      hb_astCliCborEncodeText( &buffer, "" );

      hb_astCliCborEncodeText( &buffer, "ast" );
      hb_astCliCborAddData( &buffer, pAst, nAstLen );

      hb_astCliCborEncodeText( &buffer, "macros" );
      hb_astCliCborAddData( &buffer, pMacros, nMacrosLen );

      hb_astCliCborEncodeText( &buffer, "symbols" );
      hb_astCliCborAddData( &buffer, pSymbols, nSymbolsLen );
   }

   if( pAst )
      hb_astTokenStreamSerializeSnapshotCborFree( pAst );
   if( pMacros )
      hb_astTokenStreamSerializeMacrosCborFree( pMacros );
   if( pSymbols )
      hb_astTokenStreamSerializeSymbolsCborFree( pSymbols );

   if( ! fOk )
   {
      if( buffer.pData )
         hb_xfree( buffer.pData );
      return NULL;
   }

   if( pnLength )
      *pnLength = buffer.nLen;
   return buffer.pData;
}

static void hb_astUsage( void )
{
   fprintf( stderr, "usage: hbast <input.prg> [-o <output.json>] [-b <output.cbor>]\n" );
}

static void hb_astJsonPrintEscaped( FILE * stream, const char * pszValue )
{
   const unsigned char * pch;

   if( pszValue == NULL )
   {
      fputs( "null", stream );
      return;
   }

   fputc( '"', stream );
   pch = ( const unsigned char * ) pszValue;
   while( *pch )
   {
      unsigned char ch = *pch++;

      switch( ch )
      {
         case '"':
         case '\\':
            fputc( '\\', stream );
            fputc( ch, stream );
            break;

         case '\b': fputs( "\\b", stream ); break;
         case '\f': fputs( "\\f", stream ); break;
         case '\n': fputs( "\\n", stream ); break;
         case '\r': fputs( "\\r", stream ); break;
         case '\t': fputs( "\\t", stream ); break;

         default:
            if( ch < 0x20 )
               fprintf( stream, "\\u%04x", ch );
            else
               fputc( ch, stream );
            break;
      }
   }
   fputc( '"', stream );
}

int main( int argc, char * argv[] )
{
   const char * pszInput = NULL;
   const char * pszOutput = NULL;
   const char * pszOutputCbor = NULL;
   int i;
   int rc = 0;
   HB_AST_LEXER * pLexer;
   HB_AST_TOKEN_STREAM * pStream;
   HB_AST_TOKEN token;
   HB_AST_LEXER_SOURCE cfg;

   if( argc < 2 )
   {
      hb_astUsage();
      return 1;
   }

   for( i = 1; i < argc; ++i )
   {
      if( strcmp( argv[ i ], "-o" ) == 0 )
      {
         if( i + 1 < argc )
            pszOutput = argv[ ++i ];
         else
         {
            hb_astUsage();
            return 1;
         }
      }
      else if( strcmp( argv[ i ], "-b" ) == 0 )
      {
         if( i + 1 < argc )
            pszOutputCbor = argv[ ++i ];
         else
         {
            hb_astUsage();
            return 1;
         }
      }
      else if( pszInput == NULL )
         pszInput = argv[ i ];
      else
      {
         hb_astUsage();
         return 1;
      }
   }

   if( pszInput == NULL )
   {
      hb_astUsage();
      return 1;
   }

   cfg.pszModule = pszInput;
   cfg.pszBuffer = pszInput;
   cfg.nLength   = 0;
   cfg.fOwnBuffer = HB_FALSE;
   cfg.fFromFile  = HB_TRUE;

   pLexer = hb_astLexerNew( &cfg );
   if( pLexer == NULL )
   {
      fprintf( stderr, "hbast: unable to create lexer for '%s'\n", pszInput );
      return 1;
   }

   while( hb_astLexerNextToken( pLexer, &token ) )
      ;

   pStream = hb_astTokenStreamSnapshot( pLexer );
   if( pStream == NULL )
   {
      fprintf( stderr, "hbast: failed to snapshot token stream\n" );
      hb_astLexerFree( pLexer );
      return 1;
   }

   {
      HB_SIZE nAstLen = 0;
      HB_SIZE nMacrosLen = 0;
      HB_SIZE nSymbolsLen = 0;
      char * pszAstJson = hb_astTokenStreamSerializeSnapshotJson( pStream, cfg.pszModule, &nAstLen );
      char * pszMacrosJson = hb_astTokenStreamSerializeMacrosJson( pStream, &nMacrosLen );
      char * pszSymbolsJson = hb_astTokenStreamSerializeSymbolsJson( pStream, cfg.pszModule, &nSymbolsLen );
      FILE * hOut = stdout;

      if( pszAstJson == NULL || pszMacrosJson == NULL || pszSymbolsJson == NULL )
      {
         fprintf( stderr, "hbast: serialization error\n" );
         rc = 1;
      }
      else
      {
         if( pszOutput )
         {
            hOut = fopen( pszOutput, "wb" );
            if( hOut == NULL )
            {
               fprintf( stderr, "hbast: failed to write '%s'\n", pszOutput );
               rc = 1;
            }
         }

         if( rc == 0 )
         {
            fputs( "{\"format_version\":\"0.1.0\",\"schema_revision\":1,"
                   "\"generator\":{\"name\":\"hbast\",\"version\":\"0.1.0\"},"
                   "\"project\":{\"root\":\"\",\"dialect\":\"harbour\",\"flags\":[]},"
                   "\"files\":[{\"file_id\":1,\"path\":", hOut );
            hb_astJsonPrintEscaped( hOut, pszInput );
            fputs( ",\"hash\":\"\",\"ast\":", hOut );
            if( nAstLen > 0 )
               fwrite( pszAstJson, 1, nAstLen, hOut );
            else
               fputs( "{}", hOut );
            fputs( ",\"macros\":", hOut );
            if( nMacrosLen > 0 )
               fwrite( pszMacrosJson, 1, nMacrosLen, hOut );
            else
               fputs( "{}", hOut );
            fputs( "}", hOut ); /* close file entry */
            fputs( "],\"symbols\":", hOut );
            if( nSymbolsLen > 0 )
               fwrite( pszSymbolsJson, 1, nSymbolsLen, hOut );
            else
               fputs( "[]", hOut );
            fputs( "}", hOut );
            if( hOut == stdout )
               fputc( '\n', hOut );
         }

         if( pszOutput && hOut )
            fclose( hOut );
      }

      hb_astTokenStreamSerializeSnapshotJsonFree( pszAstJson );
      hb_astTokenStreamSerializeMacrosJsonFree( pszMacrosJson );
      hb_astTokenStreamSerializeSymbolsJsonFree( pszSymbolsJson );
   }

   if( rc == 0 && pszOutputCbor )
   {
      HB_SIZE nPayloadLen = 0;
      HB_BYTE * pPayload = hb_astBuildPayloadCbor( pStream, cfg.pszModule, pszInput, &nPayloadLen );

      if( pPayload == NULL || nPayloadLen == 0 )
      {
         fprintf( stderr, "hbast: failed to serialize CBOR payload\n" );
         rc = 1;
      }
      else
      {
         FILE * hBin = fopen( pszOutputCbor, "wb" );

         if( hBin == NULL )
         {
            fprintf( stderr, "hbast: failed to write '%s'\n", pszOutputCbor );
            rc = 1;
         }
         else
         {
            if( fwrite( pPayload, 1, nPayloadLen, hBin ) != nPayloadLen )
            {
               fprintf( stderr, "hbast: failed to fully write '%s'\n", pszOutputCbor );
               rc = 1;
            }
            fclose( hBin );
         }
      }

      if( pPayload )
         hb_xfree( pPayload );
   }

   hb_astTokenStreamRelease( pStream );
   hb_astLexerFree( pLexer );

   return rc;
}
