// tests/ast/test_snapshot.c
// tests/ast/snapshot is our fast “sanity probe” that the lexer + snapshot infrastructure
// is preserving macro-expansion metadata correctly and that the serialized payload matches
// the documented schema.
#include "ast/lexer/hbast_lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int report_failure( const char * pszMessage )
{
   fprintf( stderr, "test_snapshot: %s\n", pszMessage );
   return 1;
}

static int report_failure_fmt( const char * pszMessage, const char * pszDetail )
{
   fprintf( stderr, "test_snapshot: %s (%s)\n", pszMessage, pszDetail );
   return 1;
}

int main( void )
{
   HB_AST_LEXER_SOURCE cfg = { "demo.prg", "tests/ast/demo.prg", 0, HB_FALSE, HB_TRUE };
   HB_AST_LEXER * pLexer = hb_astLexerNew( &cfg );
   HB_AST_TOKEN token;
   int fSawMacroLiteral = 0;
   int rc = 0;

   if( pLexer == NULL )
      return report_failure( "unable to create lexer" );

   while( hb_astLexerNextToken( pLexer, &token ) )
   {
      if( token.kind == HB_AST_TOKEN_KIND_LITERAL &&
          token.nLexemeLength == 2 &&
          strncmp( token.pszLexeme, "42", 2 ) == 0 )
      {
         const void * pTrace = token.pMacroOrigin;
         HB_AST_SOURCE_RANGE callRange;

         if( pTrace == NULL )
         {
            rc = report_failure( "literal token lacks macro trace" );
            break;
         }

         if( token.id.nMacroDepth == 0 )
         {
            rc = report_failure( "literal token reports zero macro depth" );
            break;
         }

         if( hb_astMacroTraceName( pTrace ) == NULL ||
             strcmp( hb_astMacroTraceName( pTrace ), "VALUE" ) != 0 )
         {
            rc = report_failure( "unexpected macro name for literal token" );
            break;
         }

         if( hb_astMacroTraceCallModule( pTrace ) == NULL ||
             strcmp( hb_astMacroTraceCallModule( pTrace ), "tests/ast/demo.prg" ) != 0 )
         {
            rc = report_failure( "unexpected call module for literal token" );
            break;
         }

         callRange = hb_astMacroTraceCallRange( pTrace );
         if( callRange.start.nLine != 4 ||
             callRange.end.nLine != 4 ||
             callRange.start.nColumn != 15 ||
             callRange.end.nColumn != 20 )
         {
            rc = report_failure( "unexpected call range for literal token" );
            break;
         }

         fSawMacroLiteral = 1;
      }
   }

    if( rc == 0 && ! fSawMacroLiteral )
      rc = report_failure( "macro literal not found" );

   if( rc == 0 )
   {
      HB_AST_TOKEN_STREAM * pSnapshot = hb_astTokenStreamSnapshot( pLexer );

      if( pSnapshot == NULL )
         rc = report_failure( "failed to snapshot token stream" );
      else
      {
         HB_SIZE nTraceCount = hb_astTokenStreamMacroTraceCount( pSnapshot );

         if( nTraceCount != 1 )
            rc = report_failure_fmt( "unexpected macro trace count", nTraceCount ? "!= 1" : "0" );
         else
         {
            const void * pTrace = hb_astTokenStreamMacroTrace( pSnapshot, 0 );
            HB_AST_SOURCE_RANGE callRange;

            if( pTrace == NULL )
               rc = report_failure( "snapshot macro trace missing" );
            else if( hb_astMacroTraceId( pTrace ) == HB_SIZE_MAX )
               rc = report_failure( "snapshot macro trace lacks stable id" );
            else if( hb_astMacroTraceDepth( pTrace ) != 1 )
               rc = report_failure( "snapshot macro trace depth unexpected" );
            else if( hb_astMacroTraceName( pTrace ) == NULL ||
                     strcmp( hb_astMacroTraceName( pTrace ), "VALUE" ) != 0 )
               rc = report_failure( "snapshot macro trace macro name mismatch" );
            else if( hb_astMacroTraceParent( pTrace ) != NULL )
               rc = report_failure( "snapshot macro trace should not have parent" );
            else
            {
               callRange = hb_astMacroTraceCallRange( pTrace );
               if( callRange.start.nLine != 4 ||
                   callRange.end.nLine != 4 ||
                   callRange.start.nColumn != 15 ||
                   callRange.end.nColumn != 20 )
               {
                  rc = report_failure( "snapshot call range mismatch" );
               }
            }
         }

         if( rc == 0 )
         {
            HB_SIZE nJsonLen = 0;
            char * pszJson = hb_astTokenStreamSerializeMacrosJson( pSnapshot, &nJsonLen );

            if( pszJson == NULL )
               rc = report_failure( "failed to serialize macro graph to json" );
            else if( nJsonLen == 0 )
               rc = report_failure( "macro graph json has zero length" );
            else if( strstr( pszJson, "\"macro_name\":\"VALUE\"" ) == NULL )
               rc = report_failure( "macro graph json missing macro name" );
            else if( strstr( pszJson, "\"expansion_id\":0" ) == NULL )
               rc = report_failure( "macro graph json missing expansion id" );
            else if( strstr( pszJson, "\"call_module\":\"tests/ast/demo.prg\"" ) == NULL )
               rc = report_failure( "macro graph json missing call module" );

            hb_astTokenStreamSerializeMacrosJsonFree( pszJson );
         }

         if( rc == 0 )
         {
            const char * pszPath = "tests/ast/out_macro.json";
            FILE * pFile;
            char buffer[ 512 ];
            size_t nRead;

            if( ! hb_astTokenStreamWriteMacrosJson( pSnapshot, pszPath ) )
               rc = report_failure( "failed to write macro graph json file" );
            else
            {
               pFile = fopen( pszPath, "rb" );
               if( pFile == NULL )
                  rc = report_failure( "unable to reopen macro graph json file" );
               else
               {
                  nRead = fread( buffer, 1, sizeof( buffer ) - 1, pFile );
                  buffer[ nRead ] = '\0';
                  fclose( pFile );

                  if( strstr( buffer, "\"expansions\"" ) == NULL )
                     rc = report_failure( "macro graph file missing expansions array" );
                  else if( strstr( buffer, "\"macro_name\":\"VALUE\"" ) == NULL )
                     rc = report_failure( "macro graph file missing macro name" );
               }
            }

            remove( pszPath );
         }

         if( rc == 0 )
         {
            HB_SIZE nFullLen = 0;
            char * pszFull = hb_astTokenStreamSerializeSnapshotJson( pSnapshot, cfg.pszBuffer, &nFullLen );

            if( pszFull == NULL )
               rc = report_failure( "failed to serialize snapshot json" );
            else if( nFullLen == 0 )
               rc = report_failure( "snapshot json has zero length" );
            else if( strstr( pszFull, "\"format_version\"") == NULL )
               rc = report_failure( "snapshot json missing format_version" );
            else if( strstr( pszFull, "\"tokens\"") == NULL )
               rc = report_failure( "snapshot json missing tokens array" );
            else if( strstr( pszFull, "\"macros\":{\"expansions\"") == NULL )
               rc = report_failure( "snapshot json missing macro section" );

            hb_astTokenStreamSerializeSnapshotJsonFree( pszFull );
         }

         hb_astTokenStreamRelease( pSnapshot );
      }
   }

   hb_astLexerFree( pLexer );
   return rc;
}
