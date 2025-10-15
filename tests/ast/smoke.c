// tests/ast/smoke.c
#include "ast/lexer/hbast_lexer.h"
#include <stdio.h>

/*
 * Debug output format
 *
 * Example line:
 *   [   1] kind=2 pp=21 span=(3:1:0 -> 3:9:8) module=tests/ast/helpers.ch text="FUNCTION"
 *
 * - Sequence (`[   1]`): local counter emitted by this harness.
 * - `kind`: token category (`HB_AST_TOKEN_KIND_*`). Values: 0 unknown, 1 eof, 2 keyword, 3 identifier,
 *   4 literal, 5 operator, 6 punctuation, 7 directive, 8 macro, 9 newline.
 * - `pp`: raw preprocessor token type (`HB_PP_TOKEN_*`). E.g. 21 KEYWORD, 41 STRING, 42 NUMBER, etc.
 * - `span`: `line:column:offset` start → end (exclusive). Columns are 1-based; the offset is the byte position
 *   in the originating module (exclusive end).
 * - `module`: source file captured by the preprocessor; includes and macro expansions carry their original module.
 * - `text`: lexeme as emitted by the PP after macro/include processing.
 */

int main( void )
{
   HB_AST_LEXER_SOURCE cfg = { "demo.prg", "demo.prg", 0, HB_FALSE, HB_TRUE };
   HB_AST_LEXER *lex = hb_astLexerNew( &cfg );
   HB_AST_TOKEN tok;

   while( hb_astLexerNextToken( lex, &tok ) )  /* hoje sai imediatamente */
   {
      printf( "[%4u] kind=%d pp=%u span=(%lu:%lu:%lu -> %lu:%lu:%lu) module=%s text=\"%.*s\"",
              ( unsigned ) tok.id.uHash,
              ( int ) tok.kind,
              ( unsigned ) tok.uPPType,
              ( unsigned long ) tok.original.start.nLine,
              ( unsigned long ) tok.original.start.nColumn,
              ( unsigned long ) tok.original.start.nOffset,
              ( unsigned long ) tok.original.end.nLine,
              ( unsigned long ) tok.original.end.nColumn,
              ( unsigned long ) tok.original.end.nOffset,
              tok.pszModule ? tok.pszModule : "<none>",
              ( int ) tok.nLexemeLength,
              tok.pszLexeme ? tok.pszLexeme : "" );

      if( tok.pMacroOrigin )
      {
         HB_AST_SOURCE_RANGE call = hb_astMacroTraceCallRange( tok.pMacroOrigin );
         const char * pszCallModule = hb_astMacroTraceCallModule( tok.pMacroOrigin );
         const char * pszMacroName = hb_astMacroTraceName( tok.pMacroOrigin );
         HB_SIZE nDepth = hb_astMacroTraceDepth( tok.pMacroOrigin );
         HB_SIZE nMacroId = hb_astMacroTraceId( tok.pMacroOrigin );

         if( nMacroId != HB_SIZE_MAX )
            printf( " macroId=%lu", ( unsigned long ) nMacroId );

         printf( " macroDepth=%lu macro=\"%s\" caller=%s call=(%lu:%lu:%lu -> %lu:%lu:%lu)",
                 ( unsigned long ) nDepth,
                 pszMacroName ? pszMacroName : "<anon>",
                 pszCallModule ? pszCallModule : "<unknown>",
                 ( unsigned long ) call.start.nLine,
                 ( unsigned long ) call.start.nColumn,
                 ( unsigned long ) call.start.nOffset,
                 ( unsigned long ) call.end.nLine,
                 ( unsigned long ) call.end.nColumn,
                 ( unsigned long ) call.end.nOffset );
      }

      putchar( '\n' );
   }

   puts( "[done] EOF reached" );

   {
      HB_AST_TOKEN_STREAM * snapshot = hb_astTokenStreamSnapshot( lex );

      if( snapshot )
      {
         HB_SIZE nCount = hb_astTokenStreamMacroTraceCount( snapshot );
         HB_SIZE i;

         for( i = 0; i < nCount; ++i )
         {
            const void * pTrace = hb_astTokenStreamMacroTrace( snapshot, i );
            HB_AST_SOURCE_RANGE call = hb_astMacroTraceCallRange( pTrace );
            const char * pszCallModule = hb_astMacroTraceCallModule( pTrace );
            const char * pszMacroName = hb_astMacroTraceName( pTrace );

            printf( "[macro %3lu] id=%lu depth=%lu macro=\"%s\" caller=%s call=(%lu:%lu:%lu -> %lu:%lu:%lu)\n",
                    ( unsigned long ) i,
                    ( unsigned long ) hb_astMacroTraceId( pTrace ),
                    ( unsigned long ) hb_astMacroTraceDepth( pTrace ),
                    pszMacroName ? pszMacroName : "<anon>",
                    pszCallModule ? pszCallModule : "<unknown>",
                    ( unsigned long ) call.start.nLine,
                    ( unsigned long ) call.start.nColumn,
                    ( unsigned long ) call.start.nOffset,
                    ( unsigned long ) call.end.nLine,
                    ( unsigned long ) call.end.nColumn,
                    ( unsigned long ) call.end.nOffset );
         }

         hb_astTokenStreamRelease( snapshot );
      }
   }

   hb_astLexerFree( lex );
   return 0;
}
