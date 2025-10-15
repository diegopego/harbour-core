#include "ast/lexer/hbast_lexer.h"
#include "hbapifs.h"
#include "hbapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void hbrenameUsage( void )
{
   fprintf( stderr, "usage: hbrename <input.prg> <old> <new>\n" );
}

static HB_BOOL hbrenameTokenBelongsToModule( const HB_AST_TOKEN * pToken, const char * pszModule )
{
   if( pszModule == NULL || *pszModule == '\0' )
      return HB_TRUE;

   if( pToken->pszModule && hb_stricmp( pToken->pszModule, pszModule ) == 0 )
      return HB_TRUE;

   if( pToken->pMacroOrigin )
   {
      const char * pszCallModule = hb_astMacroTraceCallModule( pToken->pMacroOrigin );
      if( pszCallModule && hb_stricmp( pszCallModule, pszModule ) == 0 )
         return HB_TRUE;
   }

   return HB_FALSE;
}

int main( int argc, char * argv[] )
{
   const char * pszInput;
   const char * pszOld;
   const char * pszNew;
   HB_AST_LEXER_SOURCE cfg;
   HB_AST_LEXER * pLexer;
   HB_AST_TOKEN_STREAM * pStream;
   HB_AST_TOKEN token;
   HB_SIZE nRenameCount = 0;
   HB_SIZE nSkipMacro = 0;
   HB_SIZE nSkipModule = 0;
   HB_SIZE i;

   if( argc != 4 )
   {
      hbrenameUsage();
      return 1;
   }

   pszInput = argv[ 1 ];
   pszOld   = argv[ 2 ];
   pszNew   = argv[ 3 ];

   cfg.pszModule  = pszInput;
   cfg.pszBuffer  = pszInput;
   cfg.nLength    = 0;
   cfg.fOwnBuffer = HB_FALSE;
   cfg.fFromFile  = HB_TRUE;

   pLexer = hb_astLexerNew( &cfg );
   if( pLexer == NULL )
   {
      fprintf( stderr, "hbrename: unable to create lexer for '%s'\n", pszInput );
      return 1;
   }

   while( hb_astLexerNextToken( pLexer, &token ) )
      ;

   pStream = hb_astTokenStreamSnapshot( pLexer );
   if( pStream == NULL )
   {
      fprintf( stderr, "hbrename: failed to snapshot token stream\n" );
      hb_astLexerFree( pLexer );
      return 1;
   }

   for( i = 0; i < hb_astTokenStreamCount( pStream ); ++i )
   {
      const HB_AST_TOKEN * pTok = hb_astTokenStreamToken( pStream, i );
      const void * pTrace = pTok ? pTok->pMacroOrigin : NULL;

      if( pTok == NULL )
         continue;

      if( pTok->kind != HB_AST_TOKEN_KIND_IDENTIFIER &&
          pTok->kind != HB_AST_TOKEN_KIND_KEYWORD )
         continue;

      if( pTok->pszLexeme == NULL || hb_stricmp( pTok->pszLexeme, pszOld ) != 0 )
         continue;

      if( ! hbrenameTokenBelongsToModule( pTok, cfg.pszModule ) )
      {
         ++nSkipModule;
         continue;
      }

      if( pTrace )
      {
         HB_AST_SOURCE_RANGE callRange = hb_astMacroTraceCallRange( pTrace );
         const char * pszMacroName = hb_astMacroTraceName( pTrace );
         const char * pszCallModule = hb_astMacroTraceCallModule( pTrace );
         HB_SIZE nId = hb_astMacroTraceId( pTrace );

         printf( "skip macro expansion: macro=%s id=%" HB_PFS "u call=%s:%" HB_PFS "u:%" HB_PFS "u token=%s\n",
                 pszMacroName ? pszMacroName : "<unknown>",
                 ( nId == HB_SIZE_MAX ) ? 0 : nId,
                 pszCallModule ? pszCallModule : cfg.pszModule,
                 callRange.start.nLine,
                 callRange.start.nColumn,
                 pTok->pszLexeme ? pTok->pszLexeme : "<anon>" );
         ++nSkipMacro;
         continue;
      }

      printf( "rename %s:%" HB_PFS "u:%" HB_PFS "u -> %s\n",
              pTok->pszModule ? pTok->pszModule : cfg.pszModule,
              pTok->original.start.nLine,
              pTok->original.start.nColumn,
              pszNew );
      ++nRenameCount;
   }

   for( i = 0; i < hb_astTokenStreamMacroTraceCount( pStream ); ++i )
   {
      const void * pTrace = hb_astTokenStreamMacroTrace( pStream, i );
      const char * pszMacroName = hb_astMacroTraceName( pTrace );

      if( pszMacroName && hb_stricmp( pszMacroName, pszOld ) == 0 )
      {
         HB_AST_SOURCE_RANGE callRange = hb_astMacroTraceCallRange( pTrace );
         const char * pszCallModule = hb_astMacroTraceCallModule( pTrace );
         HB_SIZE nId = hb_astMacroTraceId( pTrace );

         printf( "skip macro expansion: macro=%s id=%" HB_PFS "u call=%s:%" HB_PFS "u:%" HB_PFS "u\n",
                 pszMacroName,
                 ( nId == HB_SIZE_MAX ) ? 0 : nId,
                 pszCallModule ? pszCallModule : cfg.pszModule,
                 callRange.start.nLine,
                 callRange.start.nColumn );
         ++nSkipMacro;
      }
   }

   printf( "summary: %" HB_PFS "u candidate(s), %" HB_PFS "u skipped (macro), %" HB_PFS "u skipped (module mismatch)\n",
           nRenameCount, nSkipMacro, nSkipModule );

   hb_astTokenStreamRelease( pStream );
   hb_astLexerFree( pLexer );

   return 0;
}
