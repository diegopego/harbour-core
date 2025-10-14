/*
 * Harbour AST incremental lexer (prototype skeleton)
 *
 * Copyright 2024 Harbour Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.txt.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA (or visit https://www.gnu.org/licenses/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

#include "ast/lexer/hbast_lexer.h"
#include "hbpp.h"
#include "hbapifs.h"
#include <string.h>

static const char s_astNewlineLexeme[] = "\n";

typedef struct _HB_AST_MACRO_TRACE
{
   char *               pszMacroName;
   struct _HB_AST_MACRO_TRACE * pPrev;
} HB_AST_MACRO_TRACE;

struct _HB_AST_LEXER
{
   HB_AST_LEXER_SOURCE source;
   PHB_PP_STATE        pPP;
   HB_AST_SOURCE_COORD cursor;
   HB_BOOL             fPrevCR;
   HB_U32              nTokenIndex;
   HB_AST_MACRO_TRACE *pTraceTop;
   HB_BOOL             fDirtySnapshot;
   HB_SIZE             nMacroDepth;
};

struct _HB_AST_TOKEN_STREAM
{
   HB_SIZE nTokenCount;
};

static void hb_astLexerTraceClear( HB_AST_LEXER * pLexer );
static void hb_astLexerResetCursor( HB_AST_LEXER * pLexer );
static void hb_astLexerAdvanceSpaces( HB_AST_LEXER * pLexer, HB_SIZE nSpaces );
static void hb_astLexerAdvanceByLexeme( HB_AST_LEXER * pLexer, const char * pszLexeme, HB_SIZE nLen );
static HB_AST_TOKEN_KIND hb_astClassifyToken( HB_USHORT uType );
static HB_U16 hb_astDetermineChannel( HB_USHORT uType );
static void hb_astAdvanceChar( HB_AST_LEXER * pLexer, char c );
static PHB_PP_STATE hb_astCreatePP( const HB_AST_LEXER_SOURCE * pSource );

HB_AST_LEXER * hb_astLexerNew( const HB_AST_LEXER_SOURCE * pSource )
{
   HB_AST_LEXER * pLexer = ( HB_AST_LEXER * ) hb_xgrabz( sizeof( HB_AST_LEXER ) );

   if( pSource )
      hb_astLexerReset( pLexer, pSource );

   return pLexer;
}

void hb_astLexerFree( HB_AST_LEXER * pLexer )
{
   if( pLexer )
   {
      hb_astLexerTraceClear( pLexer );

      if( pLexer->pPP )
      {
         hb_pp_free( pLexer->pPP );
         pLexer->pPP = NULL;
      }

      if( pLexer->source.fOwnBuffer && pLexer->source.pszBuffer )
      {
         hb_xfree( ( void * ) pLexer->source.pszBuffer );
      }

      hb_xfree( pLexer );
   }
}

void hb_astLexerReset( HB_AST_LEXER * pLexer, const HB_AST_LEXER_SOURCE * pSource )
{
   if( pLexer )
   {
      hb_astLexerTraceClear( pLexer );

      if( pLexer->pPP )
      {
         hb_pp_free( pLexer->pPP );
         pLexer->pPP = NULL;
      }

      if( pLexer->source.fOwnBuffer && pLexer->source.pszBuffer && pLexer->source.pszBuffer != ( pSource ? pSource->pszBuffer : NULL ) )
      {
         hb_xfree( ( void * ) pLexer->source.pszBuffer );
      }

      if( pSource )
      {
         pLexer->source = *pSource;
      }
      else
      {
         hb_xmemset( &pLexer->source, 0, sizeof( pLexer->source ) );
      }

      hb_astLexerResetCursor( pLexer );
      pLexer->nTokenIndex    = 0;
      pLexer->fDirtySnapshot = HB_TRUE;

      if( pSource )
      {
         pLexer->pPP = hb_astCreatePP( &pLexer->source );
      }
   }
}

HB_BOOL hb_astLexerNextToken( HB_AST_LEXER * pLexer, HB_AST_TOKEN * pToken )
{
   if( pLexer == NULL || pToken == NULL )
      return HB_FALSE;

   hb_xmemset( pToken, 0, sizeof( *pToken ) );

   if( pLexer->pPP == NULL )
   {
      pToken->kind     = HB_AST_TOKEN_KIND_EOF;
      pToken->uChannel = ( HB_U16 ) HB_AST_TOKEN_CHANNEL_CODE;
      return HB_FALSE;
   }

   PHB_PP_TOKEN pSrcToken = hb_pp_tokenGet( pLexer->pPP );

   if( pSrcToken == NULL )
   {
      pToken->kind     = HB_AST_TOKEN_KIND_EOF;
      pToken->uChannel = ( HB_U16 ) HB_AST_TOKEN_CHANNEL_CODE;
      return HB_FALSE;
   }

   HB_USHORT uType = HB_PP_TOKEN_TYPE( pSrcToken->type );
   HB_SIZE nSpaces = ( HB_SIZE ) pSrcToken->spaces;

   if( uType == HB_PP_TOKEN_EOL )
      nSpaces = 0;

   if( nSpaces > 0 )
      hb_astLexerAdvanceSpaces( pLexer, nSpaces );

   HB_AST_SOURCE_COORD start = pLexer->cursor;
   if( pSrcToken->iLine > 0 )
   {
      start.nLine = ( HB_SIZE ) pSrcToken->iLine;
      pLexer->cursor.nLine = start.nLine;
   }
   HB_SIZE nLen = pSrcToken->len;
   const char * pszLexeme = pSrcToken->value;

   if( uType == HB_PP_TOKEN_EOL )
   {
      hb_astAdvanceChar( pLexer, '\n' );
      pszLexeme = s_astNewlineLexeme;
      if( nLen == 0 )
         nLen = 1;
   }
   else if( pszLexeme && nLen > 0 )
      hb_astLexerAdvanceByLexeme( pLexer, pszLexeme, nLen );

   HB_AST_SOURCE_COORD end = pLexer->cursor;

   pToken->id.uHash       = ++pLexer->nTokenIndex;
   pToken->id.nMacroDepth = pLexer->nMacroDepth;
   pToken->original.start = start;
   pToken->original.end   = end;
   pToken->expanded       = pToken->original;
   pToken->nLexemeLength  = nLen;

   pToken->uPPType     = uType;
   pToken->kind        = hb_astClassifyToken( uType );
   pToken->uChannel    = hb_astDetermineChannel( uType );
   pToken->pszLexeme   = pszLexeme ? pszLexeme : "";
   pToken->pMacroOrigin = pSrcToken;
   if( pSrcToken->szModule )
      pToken->pszModule = pSrcToken->szModule;
   else if( pLexer->source.pszModule )
      pToken->pszModule = pLexer->source.pszModule;
   else if( pLexer->source.fFromFile && pLexer->source.pszBuffer )
      pToken->pszModule = pLexer->source.pszBuffer;
   else
      pToken->pszModule = "<buffer>";

   return HB_TRUE;
}

HB_BOOL hb_astLexerMacroTracePush( HB_AST_LEXER * pLexer, const char * szMacroName )
{
   HB_AST_MACRO_TRACE * pEntry;

   if( pLexer == NULL || szMacroName == NULL )
      return HB_FALSE;

   pEntry = ( HB_AST_MACRO_TRACE * ) hb_xgrabz( sizeof( HB_AST_MACRO_TRACE ) );
   pEntry->pszMacroName = hb_strdup( szMacroName );
   pEntry->pPrev        = pLexer->pTraceTop;
   pLexer->pTraceTop    = pEntry;
   pLexer->nMacroDepth++;
   pLexer->fDirtySnapshot = HB_TRUE;

   return HB_TRUE;
}

void hb_astLexerMacroTracePop( HB_AST_LEXER * pLexer )
{
   HB_AST_MACRO_TRACE * pEntry;

   if( pLexer == NULL || pLexer->pTraceTop == NULL )
      return;

   pEntry             = pLexer->pTraceTop;
   pLexer->pTraceTop  = pEntry->pPrev;
   if( pLexer->nMacroDepth > 0 )
      pLexer->nMacroDepth--;
   pLexer->fDirtySnapshot = HB_TRUE;
   hb_xfree( pEntry->pszMacroName );
   hb_xfree( pEntry );
}

HB_AST_TOKEN_STREAM * hb_astTokenStreamSnapshot( const HB_AST_LEXER * pLexer )
{
   HB_AST_TOKEN_STREAM * pStream;

   if( pLexer == NULL )
      return NULL;

   pStream = ( HB_AST_TOKEN_STREAM * ) hb_xgrabz( sizeof( HB_AST_TOKEN_STREAM ) );
   HB_SYMBOL_UNUSED( pLexer );

   return pStream;
}

void hb_astTokenStreamRelease( HB_AST_TOKEN_STREAM * pStream )
{
   if( pStream )
   {
      hb_xfree( pStream );
   }
}

static void hb_astLexerTraceClear( HB_AST_LEXER * pLexer )
{
   if( pLexer )
   {
      while( pLexer->pTraceTop )
      {
         hb_astLexerMacroTracePop( pLexer );
      }
      pLexer->nMacroDepth = 0;
   }
}

static PHB_PP_STATE hb_astCreatePP( const HB_AST_LEXER_SOURCE * pSource )
{
   PHB_PP_STATE pPP = hb_pp_new();

   if( pPP == NULL )
      return NULL;

   hb_pp_init( pPP, HB_TRUE, HB_FALSE, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
   hb_pp_setStdRules( pPP );
   hb_pp_initDynDefines( pPP, HB_TRUE );
   hb_pp_setStdBase( pPP );

   if( pSource )
   {
      if( pSource->fFromFile )
      {
         if( pSource->pszBuffer && pSource->pszBuffer[ 0 ] )
         {
            PHB_FNAME pFName = hb_fsFNameSplit( pSource->pszBuffer );

            if( pFName )
            {
               if( pFName->szPath && pFName->szPath[ 0 ] )
                  hb_pp_addSearchPath( pPP, pFName->szPath, HB_FALSE );

               hb_xfree( pFName );
            }
         }

         if( ! pSource->pszBuffer || ! hb_pp_inFile( pPP, pSource->pszBuffer, HB_TRUE, NULL, HB_TRUE ) )
         {
            hb_pp_free( pPP );
            return NULL;
         }
      }
      else if( pSource->pszBuffer )
      {
         HB_SIZE nLen = pSource->nLength;

         if( nLen == 0 )
            nLen = ( HB_SIZE ) strlen( pSource->pszBuffer );

         hb_pp_inBuffer( pPP,
                         pSource->pszModule ? pSource->pszModule : "buffer",
                         pSource->pszBuffer,
                         nLen,
                         1 );
      }
   }

   return pPP;
}

static void hb_astLexerResetCursor( HB_AST_LEXER * pLexer )
{
   if( pLexer )
   {
      pLexer->cursor.nLine   = 1;
      pLexer->cursor.nColumn = 1;
      pLexer->cursor.nOffset = 0;
      pLexer->fPrevCR        = HB_FALSE;
   }
}

static void hb_astLexerAdvanceSpaces( HB_AST_LEXER * pLexer, HB_SIZE nSpaces )
{
   while( pLexer && nSpaces-- > 0 )
      hb_astAdvanceChar( pLexer, ' ' );
}

static void hb_astLexerAdvanceByLexeme( HB_AST_LEXER * pLexer, const char * pszLexeme, HB_SIZE nLen )
{
    if( pLexer == NULL || pszLexeme == NULL || nLen == 0 )
       return;

    HB_SIZE i;
    for( i = 0; i < nLen; ++i )
    {
       hb_astAdvanceChar( pLexer, pszLexeme[ i ] );
    }
}

static HB_AST_TOKEN_KIND hb_astClassifyToken( HB_USHORT uType )
{
   switch( uType )
   {
      case HB_PP_TOKEN_KEYWORD:
         return HB_AST_TOKEN_KIND_KEYWORD;

      case HB_PP_TOKEN_STRING:
      case HB_PP_TOKEN_NUMBER:
      case HB_PP_TOKEN_DATE:
      case HB_PP_TOKEN_TIMESTAMP:
      case HB_PP_TOKEN_LOGICAL:
      case HB_PP_TOKEN_TEXT:
         return HB_AST_TOKEN_KIND_LITERAL;

      case HB_PP_TOKEN_MACROVAR:
      case HB_PP_TOKEN_MACROTEXT:
         return HB_AST_TOKEN_KIND_MACRO;

      case HB_PP_TOKEN_DIRECTIVE:
      case HB_PP_TOKEN_HASH:
         return HB_AST_TOKEN_KIND_DIRECTIVE;

      case HB_PP_TOKEN_EOL:
         return HB_AST_TOKEN_KIND_NEWLINE;

      case HB_PP_TOKEN_LEFT_PB:
      case HB_PP_TOKEN_RIGHT_PB:
      case HB_PP_TOKEN_LEFT_SB:
      case HB_PP_TOKEN_RIGHT_SB:
      case HB_PP_TOKEN_LEFT_CB:
      case HB_PP_TOKEN_RIGHT_CB:
      case HB_PP_TOKEN_ASSIGN:
      case HB_PP_TOKEN_PLUSEQ:
      case HB_PP_TOKEN_MINUSEQ:
      case HB_PP_TOKEN_MULTEQ:
      case HB_PP_TOKEN_DIVEQ:
      case HB_PP_TOKEN_MODEQ:
      case HB_PP_TOKEN_EXPEQ:
      case HB_PP_TOKEN_INC:
      case HB_PP_TOKEN_DEC:
      case HB_PP_TOKEN_NOT:
      case HB_PP_TOKEN_OR:
      case HB_PP_TOKEN_AND:
      case HB_PP_TOKEN_EQUAL:
      case HB_PP_TOKEN_EQ:
      case HB_PP_TOKEN_LT:
      case HB_PP_TOKEN_GT:
      case HB_PP_TOKEN_LE:
      case HB_PP_TOKEN_GE:
      case HB_PP_TOKEN_NE:
      case HB_PP_TOKEN_IN:
      case HB_PP_TOKEN_PLUS:
      case HB_PP_TOKEN_MINUS:
      case HB_PP_TOKEN_MULT:
      case HB_PP_TOKEN_DIV:
      case HB_PP_TOKEN_MOD:
      case HB_PP_TOKEN_POWER:
      case HB_PP_TOKEN_EPSILON:
      case HB_PP_TOKEN_SHIFTL:
      case HB_PP_TOKEN_SHIFTR:
      case HB_PP_TOKEN_BITXOR:
      case HB_PP_TOKEN_SEND:
      case HB_PP_TOKEN_ALIAS:
      case HB_PP_TOKEN_AMPERSAND:
      case HB_PP_TOKEN_REFERENCE:
         return HB_AST_TOKEN_KIND_OPERATOR;

      case HB_PP_TOKEN_COMMA:
      case HB_PP_TOKEN_DOT:
      case HB_PP_TOKEN_EOC:
      case HB_PP_TOKEN_PIPE:
      case HB_PP_TOKEN_BACKSLASH:
         return HB_AST_TOKEN_KIND_PUNCTUATION;

      default:
         return HB_AST_TOKEN_KIND_IDENTIFIER;
   }
}

static HB_U16 hb_astDetermineChannel( HB_USHORT uType )
{
   switch( uType )
   {
      case HB_PP_TOKEN_DIRECTIVE:
      case HB_PP_TOKEN_HASH:
         return ( HB_U16 ) HB_AST_TOKEN_CHANNEL_DIRECTIVE;

      case HB_PP_TOKEN_EOL:
         return ( HB_U16 ) HB_AST_TOKEN_CHANNEL_TRIVIA;

      default:
         return ( HB_U16 ) HB_AST_TOKEN_CHANNEL_CODE;
   }
}

static void hb_astAdvanceChar( HB_AST_LEXER * pLexer, char c )
{
   if( pLexer == NULL )
      return;

   pLexer->cursor.nOffset++;

   if( c == '\r' )
   {
      pLexer->cursor.nLine++;
      pLexer->cursor.nColumn = 1;
      pLexer->fPrevCR = HB_TRUE;
   }
   else if( c == '\n' )
   {
      if( ! pLexer->fPrevCR )
         pLexer->cursor.nLine++;
      pLexer->cursor.nColumn = 1;
      pLexer->fPrevCR = HB_FALSE;
   }
   else
   {
      pLexer->cursor.nColumn++;
      pLexer->fPrevCR = HB_FALSE;
   }
}
