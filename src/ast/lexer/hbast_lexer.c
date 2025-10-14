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

#define HB_AST_LEXER_HISTORY_GROWTH 64

typedef struct _HB_AST_MACRO_TRACE_INFO
{
   HB_SIZE nRefCount;
   char *  pszMacroName;
   char *  pszCallModule;
   HB_AST_SOURCE_RANGE callRange;
   HB_SIZE nMacroDepth;
   struct _HB_AST_MACRO_TRACE_INFO * pParent;
   HB_SIZE nExpansionId;
} HB_AST_MACRO_TRACE_INFO;

typedef struct _HB_AST_MACRO_TRACE_MAP
{
   const HB_AST_MACRO_TRACE_INFO * pSource;
   HB_AST_MACRO_TRACE_INFO * pClone;
} HB_AST_MACRO_TRACE_MAP;

typedef struct _HB_AST_TOKEN_ENTRY
{
   HB_AST_TOKEN token;
   char *       pszLexemeOwned;
   char *       pszModuleOwned;
   HB_AST_MACRO_TRACE_INFO * pMacroTrace;
} HB_AST_TOKEN_ENTRY;

typedef struct _HB_AST_TOKEN_STREAM_ENTRY
{
   HB_AST_TOKEN token;
   char *       pszLexemeOwned;
   char *       pszModuleOwned;
   HB_AST_MACRO_TRACE_INFO * pMacroTrace;
} HB_AST_TOKEN_STREAM_ENTRY;

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
   HB_BOOL             fSkipLineDirective;
   HB_AST_TOKEN_ENTRY *pHistory;
   HB_SIZE             nHistoryCount;
   HB_SIZE             nHistoryCapacity;
   const HB_PP_TRACEINFO * pLastPPTrace;
   HB_AST_MACRO_TRACE_INFO * pLastASTTrace;
};

struct _HB_AST_TOKEN_STREAM
{
   HB_SIZE nTokenCount;
   HB_AST_TOKEN_STREAM_ENTRY * pEntries;
   HB_AST_MACRO_TRACE_INFO ** pMacroTraces;
   HB_SIZE nMacroTraceCount;
};

static void hb_astLexerTraceClear( HB_AST_LEXER * pLexer );
static void hb_astLexerResetCursor( HB_AST_LEXER * pLexer );
static void hb_astLexerAdvanceSpaces( HB_AST_LEXER * pLexer, HB_SIZE nSpaces );
static void hb_astLexerAdvanceByLexeme( HB_AST_LEXER * pLexer, const char * pszLexeme, HB_SIZE nLen );
static HB_AST_TOKEN_KIND hb_astClassifyToken( HB_USHORT uType );
static HB_U16 hb_astDetermineChannel( HB_USHORT uType );
static void hb_astAdvanceChar( HB_AST_LEXER * pLexer, char c );
static PHB_PP_STATE hb_astCreatePP( const HB_AST_LEXER_SOURCE * pSource );
static void hb_astLexerHistoryReset( HB_AST_LEXER * pLexer );
static void hb_astLexerHistoryEnsureCapacity( HB_AST_LEXER * pLexer, HB_SIZE nExtra );
static void hb_astLexerHistoryStore( HB_AST_LEXER * pLexer, HB_AST_TOKEN * pToken, const PHB_PP_TRACEINFO pTraceInfo );
static HB_AST_MACRO_TRACE_INFO * hb_astMacroTraceFromPP( HB_AST_LEXER * pLexer, const PHB_PP_TRACEINFO pTrace );
static HB_AST_MACRO_TRACE_INFO * hb_astMacroTraceRetain( HB_AST_MACRO_TRACE_INFO * pTrace );
static void hb_astMacroTraceRelease( HB_AST_MACRO_TRACE_INFO * pTrace );
static HB_AST_MACRO_TRACE_INFO * hb_astMacroTraceClone( const HB_AST_MACRO_TRACE_INFO * pTrace,
                                                        HB_AST_MACRO_TRACE_MAP ** ppMap,
                                                        HB_SIZE * pnCount,
                                                        HB_SIZE * pnCapacity );

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
      hb_astLexerHistoryReset( pLexer );

      if( pLexer->pPP )
      {
         hb_pp_free( pLexer->pPP );
         pLexer->pPP = NULL;
      }

      if( pLexer->source.fOwnBuffer && pLexer->source.pszBuffer )
      {
         hb_xfree( ( void * ) pLexer->source.pszBuffer );
      }

      if( pLexer->pLastASTTrace )
      {
         hb_astMacroTraceRelease( pLexer->pLastASTTrace );
         pLexer->pLastASTTrace = NULL;
      }
      pLexer->pLastPPTrace = NULL;

      if( pLexer->pHistory )
      {
         hb_xfree( pLexer->pHistory );
         pLexer->pHistory = NULL;
      }
      pLexer->nHistoryCapacity = 0;
      pLexer->nHistoryCount = 0;

      hb_xfree( pLexer );
   }
}

void hb_astLexerReset( HB_AST_LEXER * pLexer, const HB_AST_LEXER_SOURCE * pSource )
{
   if( pLexer )
   {
      hb_astLexerTraceClear( pLexer );
      hb_astLexerHistoryReset( pLexer );

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
      pLexer->fSkipLineDirective = HB_FALSE;
      if( pLexer->pLastASTTrace )
      {
         hb_astMacroTraceRelease( pLexer->pLastASTTrace );
         pLexer->pLastASTTrace = NULL;
      }
      pLexer->pLastPPTrace = NULL;

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

   PHB_PP_TOKEN pSrcToken = NULL;

   for( ;; )
   {
      pSrcToken = hb_pp_tokenGet( pLexer->pPP );

      if( pSrcToken == NULL )
      {
         pToken->kind     = HB_AST_TOKEN_KIND_EOF;
         pToken->uChannel = ( HB_U16 ) HB_AST_TOKEN_CHANNEL_CODE;
         return HB_FALSE;
      }

      HB_USHORT uTypePeek = HB_PP_TOKEN_TYPE( pSrcToken->type );

      if( pLexer->fSkipLineDirective )
      {
         if( uTypePeek == HB_PP_TOKEN_EOL )
            pLexer->fSkipLineDirective = HB_FALSE;
         continue;
      }

      if( ( uTypePeek == HB_PP_TOKEN_DIRECTIVE || uTypePeek == HB_PP_TOKEN_HASH ) &&
          pSrcToken->value && pSrcToken->len == 1 && pSrcToken->value[ 0 ] == '#' )
      {
         pLexer->fSkipLineDirective = HB_TRUE;
         continue;
      }

      break;
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
   if( pSrcToken->iColumn > 0 )
   {
      start.nColumn = ( HB_SIZE ) pSrcToken->iColumn;
      pLexer->cursor.nColumn = start.nColumn;
   }
   if( pSrcToken->nOffset != ( HB_SIZE ) -1 )
   {
      start.nOffset = pSrcToken->nOffset;
      pLexer->cursor.nOffset = start.nOffset;
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

   if( pSrcToken->iEndColumn > 0 )
   {
      end.nColumn = ( HB_SIZE ) pSrcToken->iEndColumn;
      pLexer->cursor.nColumn = end.nColumn;
   }
   else if( pSrcToken->iColumn > 0 && nLen > 0 )
   {
      end.nColumn = ( HB_SIZE ) ( pSrcToken->iColumn + ( int ) nLen );
      pLexer->cursor.nColumn = end.nColumn;
   }
   if( pSrcToken->nEndOffset != ( HB_SIZE ) -1 )
   {
      end.nOffset = pSrcToken->nEndOffset;
      pLexer->cursor.nOffset = end.nOffset;
   }
   else if( pSrcToken->nOffset != ( HB_SIZE ) -1 && nLen > 0 )
   {
      end.nOffset = pSrcToken->nOffset + nLen;
      pLexer->cursor.nOffset = end.nOffset;
   }

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
   pToken->pMacroOrigin = NULL;
   if( pSrcToken->szModule )
      pToken->pszModule = pSrcToken->szModule;
   else if( pLexer->source.pszModule )
      pToken->pszModule = pLexer->source.pszModule;
   else if( pLexer->source.fFromFile && pLexer->source.pszBuffer )
      pToken->pszModule = pLexer->source.pszBuffer;
   else
      pToken->pszModule = "<buffer>";

   hb_astLexerHistoryStore( pLexer, pToken, pSrcToken ? pSrcToken->pTraceInfo : NULL );

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
   HB_AST_MACRO_TRACE_MAP * pTraceMap = NULL;
   HB_SIZE nTraceMapCount = 0;
   HB_SIZE nTraceMapCapacity = 0;
   HB_SIZE i = 0;

   if( pLexer == NULL )
      return NULL;

   pStream = ( HB_AST_TOKEN_STREAM * ) hb_xgrabz( sizeof( HB_AST_TOKEN_STREAM ) );

   if( pLexer->nHistoryCount > 0 && pLexer->pHistory )
   {
      pStream->nTokenCount = pLexer->nHistoryCount;
      pStream->pEntries = ( HB_AST_TOKEN_STREAM_ENTRY * ) hb_xgrabz( pStream->nTokenCount * sizeof( HB_AST_TOKEN_STREAM_ENTRY ) );

      for( i = 0; i < pStream->nTokenCount; ++i )
      {
         const HB_AST_TOKEN_ENTRY * pSrc = &pLexer->pHistory[ i ];
         HB_AST_TOKEN_STREAM_ENTRY * pDst = &pStream->pEntries[ i ];
         HB_AST_MACRO_TRACE_INFO * pTraceExport;

         pDst->token = pSrc->token;

         if( pSrc->token.pszLexeme )
         {
            if( pSrc->token.nLexemeLength > 0 )
            {
               HB_SIZE nCopy = pSrc->token.nLexemeLength;
               pDst->pszLexemeOwned = ( char * ) hb_xgrab( nCopy + 1 );
               memcpy( pDst->pszLexemeOwned, pSrc->token.pszLexeme, nCopy );
               pDst->pszLexemeOwned[ nCopy ] = '\0';
            }
            else
               pDst->pszLexemeOwned = hb_strdup( pSrc->token.pszLexeme );
         }
         else
            pDst->pszLexemeOwned = hb_strdup( "" );

         pDst->token.pszLexeme = pDst->pszLexemeOwned;

         if( pSrc->token.pszModule )
            pDst->pszModuleOwned = hb_strdup( pSrc->token.pszModule );
         else
            pDst->pszModuleOwned = NULL;

         pDst->token.pszModule = pDst->pszModuleOwned;

         pTraceExport = hb_astMacroTraceClone( pSrc->pMacroTrace, &pTraceMap, &nTraceMapCount, &nTraceMapCapacity );
         pDst->pMacroTrace = pTraceExport;
         pDst->token.pMacroOrigin = pTraceExport;
         if( pTraceExport )
            pDst->token.id.nMacroDepth = pTraceExport->nMacroDepth;
      }
   }

   if( nTraceMapCount > 0 )
   {
      pStream->pMacroTraces = ( HB_AST_MACRO_TRACE_INFO ** ) hb_xgrab( nTraceMapCount * sizeof( HB_AST_MACRO_TRACE_INFO * ) );
      for( i = 0; i < nTraceMapCount; ++i )
      {
         HB_AST_MACRO_TRACE_INFO * pTrace = pTraceMap[ i ].pClone;
         pTrace->nExpansionId = i;
         pStream->pMacroTraces[ i ] = pTrace;
      }
      pStream->nMacroTraceCount = nTraceMapCount;
   }
   else
   {
      pStream->pMacroTraces = NULL;
      pStream->nMacroTraceCount = 0;
   }

   if( pTraceMap )
      hb_xfree( pTraceMap );

   ( ( HB_AST_LEXER * ) pLexer )->fDirtySnapshot = HB_FALSE;

   return pStream;
}

void hb_astTokenStreamRelease( HB_AST_TOKEN_STREAM * pStream )
{
   if( pStream )
   {
      if( pStream->pEntries )
      {
         HB_SIZE i;

         for( i = 0; i < pStream->nTokenCount; ++i )
         {
            HB_AST_TOKEN_STREAM_ENTRY * pEntry = &pStream->pEntries[ i ];

            if( pEntry->pszLexemeOwned )
               hb_xfree( pEntry->pszLexemeOwned );
            if( pEntry->pszModuleOwned )
               hb_xfree( pEntry->pszModuleOwned );
         }

         hb_xfree( pStream->pEntries );
      }

      if( pStream->pMacroTraces )
      {
         HB_SIZE i;

         for( i = 0; i < pStream->nMacroTraceCount; ++i )
            hb_astMacroTraceRelease( pStream->pMacroTraces[ i ] );

         hb_xfree( pStream->pMacroTraces );
      }

      hb_xfree( pStream );
   }
}

HB_SIZE hb_astTokenStreamCount( const HB_AST_TOKEN_STREAM * pStream )
{
   return pStream ? pStream->nTokenCount : 0;
}

const HB_AST_TOKEN * hb_astTokenStreamToken( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE nIndex )
{
   if( pStream == NULL || pStream->pEntries == NULL || nIndex >= pStream->nTokenCount )
      return NULL;

   return &pStream->pEntries[ nIndex ].token;
}

HB_SIZE hb_astTokenStreamMacroTraceCount( const HB_AST_TOKEN_STREAM * pStream )
{
   return pStream ? pStream->nMacroTraceCount : 0;
}

const void * hb_astTokenStreamMacroTrace( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE nIndex )
{
   if( pStream == NULL || pStream->pMacroTraces == NULL || nIndex >= pStream->nMacroTraceCount )
      return NULL;

   return pStream->pMacroTraces[ nIndex ];
}

const char * hb_astMacroTraceName( const void * pMacroTrace )
{
   const HB_AST_MACRO_TRACE_INFO * pInfo = ( const HB_AST_MACRO_TRACE_INFO * ) pMacroTrace;

   return pInfo ? pInfo->pszMacroName : NULL;
}

const char * hb_astMacroTraceCallModule( const void * pMacroTrace )
{
   const HB_AST_MACRO_TRACE_INFO * pInfo = ( const HB_AST_MACRO_TRACE_INFO * ) pMacroTrace;

   return pInfo ? pInfo->pszCallModule : NULL;
}

HB_AST_SOURCE_RANGE hb_astMacroTraceCallRange( const void * pMacroTrace )
{
   HB_AST_SOURCE_RANGE range;
   const HB_AST_MACRO_TRACE_INFO * pInfo = ( const HB_AST_MACRO_TRACE_INFO * ) pMacroTrace;

   hb_xmemset( &range, 0, sizeof( range ) );
   if( pInfo )
      range = pInfo->callRange;

   return range;
}

HB_SIZE hb_astMacroTraceDepth( const void * pMacroTrace )
{
   const HB_AST_MACRO_TRACE_INFO * pInfo = ( const HB_AST_MACRO_TRACE_INFO * ) pMacroTrace;

   return pInfo ? pInfo->nMacroDepth : 0;
}

const void * hb_astMacroTraceParent( const void * pMacroTrace )
{
   const HB_AST_MACRO_TRACE_INFO * pInfo = ( const HB_AST_MACRO_TRACE_INFO * ) pMacroTrace;

   return pInfo ? pInfo->pParent : NULL;
}

HB_SIZE hb_astMacroTraceId( const void * pMacroTrace )
{
   const HB_AST_MACRO_TRACE_INFO * pInfo = ( const HB_AST_MACRO_TRACE_INFO * ) pMacroTrace;

   return pInfo ? pInfo->nExpansionId : HB_SIZE_MAX;
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

static void hb_astLexerHistoryReset( HB_AST_LEXER * pLexer )
{
   HB_SIZE i;

   if( pLexer == NULL )
      return;

   if( pLexer->pHistory )
   {
      for( i = 0; i < pLexer->nHistoryCount; ++i )
      {
         HB_AST_TOKEN_ENTRY * pEntry = &pLexer->pHistory[ i ];

         if( pEntry->pszLexemeOwned )
         {
            hb_xfree( pEntry->pszLexemeOwned );
            pEntry->pszLexemeOwned = NULL;
         }

         if( pEntry->pszModuleOwned )
         {
            hb_xfree( pEntry->pszModuleOwned );
            pEntry->pszModuleOwned = NULL;
         }

         if( pEntry->pMacroTrace )
         {
            hb_astMacroTraceRelease( pEntry->pMacroTrace );
            pEntry->pMacroTrace = NULL;
         }

         hb_xmemset( &pEntry->token, 0, sizeof( HB_AST_TOKEN ) );
      }
   }

   pLexer->nHistoryCount = 0;
   if( pLexer->pLastASTTrace )
   {
      hb_astMacroTraceRelease( pLexer->pLastASTTrace );
      pLexer->pLastASTTrace = NULL;
   }
   pLexer->pLastPPTrace = NULL;
}

static void hb_astLexerHistoryEnsureCapacity( HB_AST_LEXER * pLexer, HB_SIZE nExtra )
{
   HB_SIZE nNeeded, nCapacity;

   if( pLexer == NULL )
      return;

   nNeeded = pLexer->nHistoryCount + nExtra;
   if( nNeeded <= pLexer->nHistoryCapacity )
      return;

   nCapacity = pLexer->nHistoryCapacity;
   if( nCapacity == 0 )
      nCapacity = HB_AST_LEXER_HISTORY_GROWTH;

   while( nCapacity < nNeeded )
   {
      if( nCapacity < HB_AST_LEXER_HISTORY_GROWTH )
         nCapacity = HB_AST_LEXER_HISTORY_GROWTH;
      else
         nCapacity *= 2;
   }

   pLexer->pHistory = ( HB_AST_TOKEN_ENTRY * ) hb_xrealloc( pLexer->pHistory, nCapacity * sizeof( HB_AST_TOKEN_ENTRY ) );
   hb_xmemset( pLexer->pHistory + pLexer->nHistoryCapacity, 0, ( nCapacity - pLexer->nHistoryCapacity ) * sizeof( HB_AST_TOKEN_ENTRY ) );
   pLexer->nHistoryCapacity = nCapacity;
}

static void hb_astLexerHistoryStore( HB_AST_LEXER * pLexer, HB_AST_TOKEN * pToken, const PHB_PP_TRACEINFO pTraceInfo )
{
   HB_AST_TOKEN_ENTRY * pEntry;
   HB_SIZE nCopyLen;

   if( pLexer == NULL || pToken == NULL )
      return;

   hb_astLexerHistoryEnsureCapacity( pLexer, 1 );

   pEntry = &pLexer->pHistory[ pLexer->nHistoryCount++ ];
   memcpy( &pEntry->token, pToken, sizeof( HB_AST_TOKEN ) );

   pEntry->pszLexemeOwned = NULL;
   pEntry->pszModuleOwned = NULL;

   if( pToken->pszLexeme )
   {
      if( pToken->nLexemeLength > 0 )
      {
         nCopyLen = pToken->nLexemeLength;
         pEntry->pszLexemeOwned = ( char * ) hb_xgrab( nCopyLen + 1 );
         memcpy( pEntry->pszLexemeOwned, pToken->pszLexeme, nCopyLen );
         pEntry->pszLexemeOwned[ nCopyLen ] = '\0';
      }
      else
         pEntry->pszLexemeOwned = hb_strdup( pToken->pszLexeme );
   }
   else
      pEntry->pszLexemeOwned = hb_strdup( "" );

   pEntry->token.pszLexeme = pEntry->pszLexemeOwned;

   if( pToken->pszModule )
      pEntry->pszModuleOwned = hb_strdup( pToken->pszModule );

   pEntry->token.pszModule = pEntry->pszModuleOwned ? pEntry->pszModuleOwned : NULL;
   pEntry->pMacroTrace = hb_astMacroTraceFromPP( pLexer, pTraceInfo );
   pEntry->token.pMacroOrigin = pEntry->pMacroTrace;
   if( pEntry->pMacroTrace )
      pEntry->token.id.nMacroDepth = ( HB_U32 ) pEntry->pMacroTrace->nMacroDepth;
   else
      pEntry->token.id.nMacroDepth = 0;

   *pToken = pEntry->token;
   pLexer->fDirtySnapshot = HB_TRUE;
}

static HB_AST_MACRO_TRACE_INFO * hb_astMacroTraceRetain( HB_AST_MACRO_TRACE_INFO * pTrace )
{
   if( pTrace )
      ++pTrace->nRefCount;
   return pTrace;
}

static void hb_astMacroTraceRelease( HB_AST_MACRO_TRACE_INFO * pTrace )
{
   if( pTrace && --pTrace->nRefCount == 0 )
   {
      if( pTrace->pszMacroName )
         hb_xfree( pTrace->pszMacroName );
      if( pTrace->pszCallModule )
         hb_xfree( pTrace->pszCallModule );
      if( pTrace->pParent )
         hb_astMacroTraceRelease( pTrace->pParent );
      hb_xfree( pTrace );
   }
}

static HB_AST_MACRO_TRACE_INFO * hb_astMacroTraceFromPP( HB_AST_LEXER * pLexer, const PHB_PP_TRACEINFO pTrace )
{
   HB_AST_MACRO_TRACE_INFO * pResult;

   if( pTrace == NULL )
      return NULL;

   if( pLexer && pLexer->pLastPPTrace == pTrace && pLexer->pLastASTTrace )
      return hb_astMacroTraceRetain( pLexer->pLastASTTrace );

   pResult = ( HB_AST_MACRO_TRACE_INFO * ) hb_xgrabz( sizeof( HB_AST_MACRO_TRACE_INFO ) );
   pResult->nRefCount = 1;

   if( pTrace->pszMacroName )
      pResult->pszMacroName = hb_strdup( pTrace->pszMacroName );
   if( pTrace->pszCallModule )
      pResult->pszCallModule = hb_strdup( pTrace->pszCallModule );

   if( pTrace->iCallLine > 0 )
      pResult->callRange.start.nLine = ( HB_SIZE ) pTrace->iCallLine;
   if( pTrace->iCallColumn > 0 )
      pResult->callRange.start.nColumn = ( HB_SIZE ) pTrace->iCallColumn;
   if( pTrace->nCallOffset != ( HB_SIZE ) -1 )
      pResult->callRange.start.nOffset = pTrace->nCallOffset;

   if( pTrace->iCallEndLine > 0 )
      pResult->callRange.end.nLine = ( HB_SIZE ) pTrace->iCallEndLine;
   else if( pResult->callRange.end.nLine == 0 )
      pResult->callRange.end.nLine = pResult->callRange.start.nLine;

   if( pTrace->iCallEndColumn > 0 )
      pResult->callRange.end.nColumn = ( HB_SIZE ) pTrace->iCallEndColumn;
   else if( pResult->callRange.end.nColumn == 0 )
      pResult->callRange.end.nColumn = pResult->callRange.start.nColumn;

   if( pTrace->nCallEndOffset != ( HB_SIZE ) -1 )
      pResult->callRange.end.nOffset = pTrace->nCallEndOffset;
   else if( pTrace->nCallOffset != ( HB_SIZE ) -1 )
      pResult->callRange.end.nOffset = pTrace->nCallOffset;

   if( pTrace->pParent )
   {
      pResult->pParent = hb_astMacroTraceFromPP( pLexer, pTrace->pParent );
      if( pResult->pParent )
         pResult->nMacroDepth = pResult->pParent->nMacroDepth + 1;
      else
         pResult->nMacroDepth = 1;
   }
   else
      pResult->nMacroDepth = 1;

   pResult->nExpansionId = HB_SIZE_MAX;

   if( pLexer )
   {
      if( pLexer->pLastASTTrace )
         hb_astMacroTraceRelease( pLexer->pLastASTTrace );
      pLexer->pLastPPTrace = pTrace;
      pLexer->pLastASTTrace = hb_astMacroTraceRetain( pResult );
   }

   return pResult;
}

static HB_AST_MACRO_TRACE_INFO * hb_astMacroTraceClone( const HB_AST_MACRO_TRACE_INFO * pTrace,
                                                        HB_AST_MACRO_TRACE_MAP ** ppMap,
                                                        HB_SIZE * pnCount,
                                                        HB_SIZE * pnCapacity )
{
   HB_AST_MACRO_TRACE_MAP * pMap;
   HB_SIZE nCount, nCapacity, i;
   HB_AST_MACRO_TRACE_INFO * pClone;

   if( pTrace == NULL )
      return NULL;

   pMap = *ppMap;
   nCount = *pnCount;
   nCapacity = *pnCapacity;

   for( i = 0; i < nCount; ++i )
   {
      if( pMap[ i ].pSource == pTrace )
         return pMap[ i ].pClone;
   }

   pClone = ( HB_AST_MACRO_TRACE_INFO * ) hb_xgrabz( sizeof( HB_AST_MACRO_TRACE_INFO ) );
   pClone->nRefCount = 1;

   if( pTrace->pszMacroName )
      pClone->pszMacroName = hb_strdup( pTrace->pszMacroName );
   if( pTrace->pszCallModule )
      pClone->pszCallModule = hb_strdup( pTrace->pszCallModule );

   pClone->callRange = pTrace->callRange;
   if( pClone->callRange.end.nLine == 0 )
      pClone->callRange.end.nLine = pClone->callRange.start.nLine;
   if( pClone->callRange.end.nColumn == 0 )
      pClone->callRange.end.nColumn = pClone->callRange.start.nColumn;
   if( pClone->callRange.end.nOffset == 0 && pClone->callRange.start.nOffset != 0 )
      pClone->callRange.end.nOffset = pClone->callRange.start.nOffset;

   pClone->nMacroDepth = pTrace->nMacroDepth;
   pClone->nExpansionId = HB_SIZE_MAX;

   pClone->pParent = hb_astMacroTraceClone( pTrace->pParent, ppMap, pnCount, pnCapacity );
   if( pClone->pParent )
      hb_astMacroTraceRetain( pClone->pParent );

   if( nCount == nCapacity )
   {
      nCapacity = nCapacity ? nCapacity * 2 : 16;
      pMap = ( HB_AST_MACRO_TRACE_MAP * ) hb_xrealloc( pMap, nCapacity * sizeof( HB_AST_MACRO_TRACE_MAP ) );
      *ppMap = pMap;
      *pnCapacity = nCapacity;
   }

   pMap[ nCount ].pSource = pTrace;
   pMap[ nCount ].pClone  = pClone;
   *pnCount = nCount + 1;

   return pClone;
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
