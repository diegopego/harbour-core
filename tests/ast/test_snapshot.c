// tests/ast/test_snapshot.c
// tests/ast/snapshot is our fast “sanity probe” that the lexer + snapshot infrastructure is preserving macro-expansion metadata correctly. It instantiates the lexer on tests/ast/demo.prg, consumes the token stream, and then:
// Asserts that the literal coming from the VALUE macro carries pMacroOrigin with the right name, caller module and source range.
// Takes a snapshot and enumerates the recorded macro graph, checking that there’s exactly one expansion, with depth/ID/ranges matching the call site.
// If anything in that pipeline regresses—PP trace wiring, snapshot cloning, helper APIs—the test exits with a failure message. Because our downstream tooling (rename, verify, serializer) depends on that macro graph being trustworthy, this test is our early warning that the data feeding those agents is still sane.
#include "ast/lexer/hbast_lexer.h"
#include <stdio.h>
#include <string.h>

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

         hb_astTokenStreamRelease( pSnapshot );
      }
   }

   hb_astLexerFree( pLexer );
   return rc;
}
