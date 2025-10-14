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

typedef struct _HB_AST_MACRO_TRACE
{
   char *               pszMacroName;
   struct _HB_AST_MACRO_TRACE * pPrev;
} HB_AST_MACRO_TRACE;

struct _HB_AST_LEXER
{
   HB_AST_LEXER_SOURCE source;
   HB_SIZE             nOffset;
   HB_AST_MACRO_TRACE *pTraceTop;
   HB_BOOL             fDirtySnapshot;
};

struct _HB_AST_TOKEN_STREAM
{
   HB_SIZE nTokenCount;
};

static void hb_astLexerTraceClear( HB_AST_LEXER * pLexer );

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

      pLexer->nOffset        = 0;
      pLexer->fDirtySnapshot = HB_TRUE;
   }
}

HB_BOOL hb_astLexerNextToken( HB_AST_LEXER * pLexer, HB_AST_TOKEN * pToken )
{
   if( pLexer == NULL || pToken == NULL )
      return HB_FALSE;

   HB_SYMBOL_UNUSED( pLexer );

   hb_xmemset( pToken, 0, sizeof( *pToken ) );
   pToken->kind     = HB_AST_TOKEN_KIND_EOF;
   pToken->uChannel = ( HB_U16 ) HB_AST_TOKEN_CHANNEL_CODE;

   return HB_FALSE;
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
   }
}
