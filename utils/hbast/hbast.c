#include "ast/lexer/hbast_lexer.h"
#include "hbapifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void hb_astUsage( void )
{
   fprintf( stderr, "usage: hbast <input.prg> [-o <output.json>]\n" );
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
      HB_SIZE nLen = 0;
      char * pszJson = hb_astTokenStreamSerializeSnapshotJson( pStream, &nLen );
      FILE * hOut = stdout;

      if( pszJson == NULL )
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
            fputs( "{\"format_version\":\"0.0.1\",\"schema_revision\":1,"
                   "\"generator\":{\"name\":\"hbast\",\"version\":\"0.0.1\"},"
                   "\"files\":[{\"file_id\":1,\"path\":", hOut );
            hb_astJsonPrintEscaped( hOut, pszInput );
            fputs( ",\"hash\":\"\",\"ast\":{\"root\":0,\"nodes\":[],", hOut );
            if( nLen > 0 )
               fwrite( pszJson, 1, nLen, hOut );
            fputs( "}}]}", hOut );
            if( hOut == stdout )
               fputc( '\n', hOut );
         }

         if( pszOutput && hOut )
            fclose( hOut );

         hb_astTokenStreamSerializeSnapshotJsonFree( pszJson );
      }
   }

   hb_astTokenStreamRelease( pStream );
   hb_astLexerFree( pLexer );

   return rc;
}
