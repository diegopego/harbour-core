/*
 * AST dump (-x switch) in Harbour compiler
 *
 * While compiling, exports as a JSON file the facts a source analysis /
 * refactoring tool needs: the token stream with exact source positions
 * (surviving preprocessor rule rewrites through match markers), every
 * declaration with its resolved scope, every variable reference with its
 * access mode, function calls, message sends, control block events and
 * the expression tree of each statement.
 *
 * All logic lives in this file; the rest of the compiler only carries
 * one-line hook calls gated by HB_COMP_PARAM->fAst, so the compiler
 * behaviour and its outputs are byte-identical when the switch is off.
 *
 * Copyright 2026 Diego Oliveira Pego <diego@audisoft.com.br>
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

#include "hbcomp.h"

#define HB_AST_SCHEMA         "ast-1"
#define HB_AST_ALLOC_BASE     64

/* one consumed token: position captured by the preprocessor's tracking
   (see hb_pp_trackPos()) at the moment the compiler pulled it */
typedef struct
{
   char *    szText;
   HB_SIZE   nLen;
   int       iLine;           /* physical source line, 0 = unknown */
   int       iCol;            /* 0-based byte column, -1 = no source column */
   HB_USHORT type;            /* HB_PP_TOKEN_TYPE() value */
   char      cProv;           /* 's' main source, 'i' include, 'n' synthesized */
} HB_ASTTOKEN, * PHB_ASTTOKEN;

/* birth record of an expression node: the pointer is only ever looked up
   while the node is alive (during hb_compAstStatement() serialization of
   the still-live tree), so entries of freed nodes are never reached */
typedef struct
{
   const void * pKey;         /* PHB_EXPR at birth */
   int          iLine;
   HB_SIZE      nBirthTok;    /* token counter value at birth */
} HB_ASTNODE, * PHB_ASTNODE;

typedef struct
{
   PHB_HFUNC pFunc;
   int       iLine;
} HB_ASTFUNC, * PHB_ASTFUNC;

typedef struct
{
   const char * szSym;        /* interned identifier */
   PHB_HFUNC    pFunc;        /* resolved owner function */
   int          iLine;
   int          iScope;       /* HB_VS_* value */
   int          iAccess;      /* 'r' read, 'w' write, 'x' by ref, 'u' use */
   HB_BOOL      fBlock;       /* reference made inside a codeblock body */
} HB_ASTUSE, * PHB_ASTUSE;

typedef struct
{
   const char * szSym;
   PHB_HFUNC    pFunc;
   int          iLine;
   HB_BOOL      fBlock;
} HB_ASTCALL, * PHB_ASTCALL;

/* control block event emitted by the grammar actions */
typedef struct
{
   PHB_HFUNC pFunc;
   int       iLine;
   HB_SIZE   nTok;
   char      cKind;           /* 'i'f 'w'hile 'f'or 'c'ase 's'witch 'q'sequence */
   char      cEvent;          /* 'o'pen 'c'lose 'm'id (else/elseif/case/recover) */
} HB_ASTBLOCK, * PHB_ASTBLOCK;

/* a serialized statement/push expression tree */
typedef struct
{
   PHB_HFUNC pFunc;
   int       iLine;
   char *    szJson;          /* serialized expression tree */
   char      cKind;           /* 's' statement, 'p' pushed expression */
   HB_BOOL   fBlock;
} HB_ASTSTMT, * PHB_ASTSTMT;

typedef struct _HB_ASTDUMP
{
   HB_ASTTOKEN * pTokens;
   HB_SIZE       nTokenCount;
   HB_SIZE       nTokenAlloc;

   HB_ASTNODE *  pNodes;      /* open addressing hash, power of two */
   HB_SIZE       nNodeSize;
   HB_SIZE       nNodeCount;

   HB_ASTFUNC *  pFuncs;
   HB_SIZE       nFuncCount;
   HB_SIZE       nFuncAlloc;

   HB_ASTUSE *   pUses;
   HB_SIZE       nUseCount;
   HB_SIZE       nUseAlloc;

   HB_ASTCALL *  pCalls;
   HB_SIZE       nCallCount;
   HB_SIZE       nCallAlloc;

   HB_ASTCALL *  pSends;
   HB_SIZE       nSendCount;
   HB_SIZE       nSendAlloc;

   HB_ASTBLOCK * pBlocks;
   HB_SIZE       nBlockCount;
   HB_SIZE       nBlockAlloc;

   HB_ASTSTMT *  pStmts;
   HB_SIZE       nStmtCount;
   HB_SIZE       nStmtAlloc;

   char *        szModule;    /* source module name captured at parse time */
} HB_ASTDUMP, * PHB_ASTDUMP;

/* growing text buffer for the statement serializer */
typedef struct
{
   char *  pData;
   HB_SIZE nLen;
   HB_SIZE nAlloc;
} HB_ASTBUF, * PHB_ASTBUF;

static PHB_ASTDUMP hb_compAstDump( HB_COMP_DECL )
{
   if( ! HB_COMP_PARAM->pAst )
   {
      PHB_ASTDUMP pAst = ( PHB_ASTDUMP ) hb_xgrabz( sizeof( HB_ASTDUMP ) );

      /* capture the source module name now - during code generation
         HB_COMP_PARAM->pFileName is switched to the output file */
      if( HB_COMP_PARAM->pFileName )
      {
         HB_FNAME ModName;
         char szModName[ HB_PATH_MAX ];

         ModName.szDrive = ModName.szPath = NULL;
         ModName.szName = HB_COMP_PARAM->pFileName->szName;
         ModName.szExtension = HB_COMP_PARAM->pFileName->szExtension ?
                               HB_COMP_PARAM->pFileName->szExtension : ".prg";
         hb_fsFNameMerge( szModName, &ModName );
         pAst->szModule = hb_strdup( szModName );
      }
      HB_COMP_PARAM->pAst = pAst;
   }
   return HB_COMP_PARAM->pAst;
}

/* resolve the real owner function when compiling a codeblock body */
static PHB_HFUNC hb_compAstOwner( HB_COMP_DECL, HB_BOOL * pfBlock )
{
   PHB_HFUNC pFunc = HB_COMP_PARAM->functions.pLast;

   *pfBlock = HB_FALSE;
   while( pFunc && pFunc->pOwner )
   {
      *pfBlock = HB_TRUE;
      pFunc = pFunc->pOwner;
   }
   return pFunc;
}

/* --- token stream ------------------------------------------------------- */

void hb_compAstToken( HB_COMP_DECL, PHB_PP_TOKEN pToken )
{
   PHB_ASTDUMP pAst;
   PHB_ASTTOKEN pTok;
   int iLine = 0, iCol = -1;
   HB_BOOL fMain = HB_TRUE;
   char cProv;

   if( ! HB_COMP_PARAM->fAst || pToken == NULL )
      return;

   /* EOL tokens carry no information the line counters do not have */
   if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_EOL )
      return;

   if( hb_pp_tokenPos( HB_COMP_PARAM->pLex->pPP, pToken, &iLine, &iCol, &fMain ) )
      cProv = fMain ? ( iCol >= 0 ? 's' : 'n' ) : 'i';
   else
      cProv = 'n';
   if( ! fMain )
      iCol = -1;      /* a column in another physical file is not a column here */

   pAst = hb_compAstDump( HB_COMP_PARAM );
   if( pAst->nTokenCount == pAst->nTokenAlloc )
   {
      pAst->nTokenAlloc += HB_AST_ALLOC_BASE * 16;
      pAst->pTokens = ( HB_ASTTOKEN * ) hb_xrealloc( pAst->pTokens,
                              pAst->nTokenAlloc * sizeof( HB_ASTTOKEN ) );
   }
   pTok = &pAst->pTokens[ pAst->nTokenCount++ ];
   pTok->szText = ( char * ) memcpy( hb_xgrab( pToken->len + 1 ),
                                     pToken->value, pToken->len );
   pTok->szText[ pToken->len ] = '\0';
   pTok->nLen  = pToken->len;
   pTok->iLine = iLine;
   pTok->iCol  = iCol;
   pTok->type  = HB_PP_TOKEN_TYPE( pToken->type );
   pTok->cProv = cProv;
}

/* --- expression node births -------------------------------------------- */

static void hb_compAstNodeInsert( PHB_ASTDUMP pAst, const void * pKey,
                                  int iLine, HB_SIZE nBirthTok )
{
   HB_SIZE nAt;

   if( ! pAst->pNodes )
   {
      pAst->nNodeSize = 4096;
      pAst->pNodes = ( HB_ASTNODE * ) hb_xgrabz( pAst->nNodeSize * sizeof( HB_ASTNODE ) );
   }
   else if( ( pAst->nNodeCount << 1 ) >= pAst->nNodeSize )
   {
      HB_ASTNODE * pOld = pAst->pNodes;
      HB_SIZE nOldSize = pAst->nNodeSize, n;

      pAst->nNodeSize <<= 1;
      pAst->pNodes = ( HB_ASTNODE * ) hb_xgrabz( pAst->nNodeSize * sizeof( HB_ASTNODE ) );
      pAst->nNodeCount = 0;
      for( n = 0; n < nOldSize; ++n )
      {
         if( pOld[ n ].pKey )
         {
            nAt = ( ( HB_PTRUINT ) pOld[ n ].pKey >> 4 ) & ( pAst->nNodeSize - 1 );
            while( pAst->pNodes[ nAt ].pKey )
               nAt = ( nAt + 1 ) & ( pAst->nNodeSize - 1 );
            pAst->pNodes[ nAt ] = pOld[ n ];
            pAst->nNodeCount++;
         }
      }
      hb_xfree( pOld );
   }

   nAt = ( ( HB_PTRUINT ) pKey >> 4 ) & ( pAst->nNodeSize - 1 );
   while( pAst->pNodes[ nAt ].pKey && pAst->pNodes[ nAt ].pKey != pKey )
      nAt = ( nAt + 1 ) & ( pAst->nNodeSize - 1 );
   if( ! pAst->pNodes[ nAt ].pKey )
      pAst->nNodeCount++;
   pAst->pNodes[ nAt ].pKey = pKey;
   pAst->pNodes[ nAt ].iLine = iLine;
   pAst->pNodes[ nAt ].nBirthTok = nBirthTok;
}

static PHB_ASTNODE hb_compAstNodeFind( PHB_ASTDUMP pAst, const void * pKey )
{
   if( pAst->pNodes )
   {
      HB_SIZE nAt = ( ( HB_PTRUINT ) pKey >> 4 ) & ( pAst->nNodeSize - 1 );

      while( pAst->pNodes[ nAt ].pKey )
      {
         if( pAst->pNodes[ nAt ].pKey == pKey )
            return &pAst->pNodes[ nAt ];
         nAt = ( nAt + 1 ) & ( pAst->nNodeSize - 1 );
      }
   }
   return NULL;
}

/* every expression node passes through hb_compExprNew(): record its
   identity, current line and the token counter at birth.  A record is
   written for EVERY birth, so a recycled pointer always finds a fresh
   entry - stale entries of freed nodes are never reachable from a live
   tree */
void hb_compAstNodeBorn( HB_COMP_DECL, PHB_EXPR pExpr )
{
   PHB_ASTDUMP pAst;

   if( ! HB_COMP_PARAM->fAst || pExpr == NULL )
      return;

   pAst = hb_compAstDump( HB_COMP_PARAM );
   hb_compAstNodeInsert( pAst, pExpr, HB_COMP_PARAM->currLine,
                         pAst->nTokenCount ? pAst->nTokenCount - 1 : 0 );
}

/* --- parity records (declarations resolved at parse time) --------------- */

void hb_compAstFuncBegin( HB_COMP_DECL )
{
   PHB_ASTDUMP pAst;
   PHB_ASTFUNC pFuncInfo;

   if( ! HB_COMP_PARAM->fAst )
      return;

   pAst = hb_compAstDump( HB_COMP_PARAM );
   if( pAst->nFuncCount == pAst->nFuncAlloc )
   {
      pAst->nFuncAlloc += HB_AST_ALLOC_BASE;
      pAst->pFuncs = ( HB_ASTFUNC * ) hb_xrealloc( pAst->pFuncs,
                              pAst->nFuncAlloc * sizeof( HB_ASTFUNC ) );
   }
   pFuncInfo = &pAst->pFuncs[ pAst->nFuncCount++ ];
   pFuncInfo->pFunc = HB_COMP_PARAM->functions.pLast;
   pFuncInfo->iLine = HB_COMP_PARAM->currLine;
}

void hb_compAstUse( HB_COMP_DECL, const char * szVarName, int iScope, int iAccess )
{
   PHB_ASTDUMP pAst;
   PHB_ASTUSE pUse;
   HB_BOOL fBlock;

   if( ! HB_COMP_PARAM->fAst )
      return;

   pAst = hb_compAstDump( HB_COMP_PARAM );
   if( pAst->nUseCount == pAst->nUseAlloc )
   {
      pAst->nUseAlloc += HB_AST_ALLOC_BASE * 4;
      pAst->pUses = ( HB_ASTUSE * ) hb_xrealloc( pAst->pUses,
                              pAst->nUseAlloc * sizeof( HB_ASTUSE ) );
   }
   pUse = &pAst->pUses[ pAst->nUseCount++ ];
   pUse->szSym   = szVarName;
   pUse->pFunc   = hb_compAstOwner( HB_COMP_PARAM, &fBlock );
   pUse->iLine   = HB_COMP_PARAM->currLine;
   pUse->iScope  = iScope;
   pUse->iAccess = iAccess;
   pUse->fBlock  = fBlock;
}

/* refine the access mode of the reference just recorded by
   hb_compVariableFind() from the code generator which knows the
   reference context (read/write/by reference) */
void hb_compAstTag( HB_COMP_DECL, const char * szVarName, int iAccess )
{
   PHB_ASTDUMP pAst = HB_COMP_PARAM->pAst;

   if( pAst && pAst->nUseCount )
   {
      PHB_ASTUSE pUse = &pAst->pUses[ pAst->nUseCount - 1 ];

      if( pUse->szSym == szVarName && pUse->iLine == HB_COMP_PARAM->currLine )
         pUse->iAccess = iAccess;
   }
}

void hb_compAstCallAdd( HB_COMP_DECL, const char * szFunName )
{
   PHB_ASTDUMP pAst;
   PHB_ASTCALL pCall;
   HB_BOOL fBlock;

   if( ! HB_COMP_PARAM->fAst )
      return;

   pAst = hb_compAstDump( HB_COMP_PARAM );
   if( pAst->nCallCount == pAst->nCallAlloc )
   {
      pAst->nCallAlloc += HB_AST_ALLOC_BASE;
      pAst->pCalls = ( HB_ASTCALL * ) hb_xrealloc( pAst->pCalls,
                              pAst->nCallAlloc * sizeof( HB_ASTCALL ) );
   }
   pCall = &pAst->pCalls[ pAst->nCallCount++ ];
   pCall->szSym  = szFunName;
   pCall->pFunc  = hb_compAstOwner( HB_COMP_PARAM, &fBlock );
   pCall->iLine  = HB_COMP_PARAM->currLine;
   pCall->fBlock = fBlock;
}

void hb_compAstSendAdd( HB_COMP_DECL, const char * szMsgName )
{
   PHB_ASTDUMP pAst;
   PHB_ASTCALL pSend;
   HB_BOOL fBlock;

   if( ! HB_COMP_PARAM->fAst )
      return;

   pAst = hb_compAstDump( HB_COMP_PARAM );
   if( pAst->nSendCount == pAst->nSendAlloc )
   {
      pAst->nSendAlloc += HB_AST_ALLOC_BASE;
      pAst->pSends = ( HB_ASTCALL * ) hb_xrealloc( pAst->pSends,
                              pAst->nSendAlloc * sizeof( HB_ASTCALL ) );
   }
   pSend = &pAst->pSends[ pAst->nSendCount++ ];
   pSend->szSym  = szMsgName;
   pSend->pFunc  = hb_compAstOwner( HB_COMP_PARAM, &fBlock );
   pSend->iLine  = HB_COMP_PARAM->currLine;
   pSend->fBlock = fBlock;
}

/* --- control block events ------------------------------------------------ */

void hb_compAstBlock( HB_COMP_DECL, int iKind, int iEvent )
{
   PHB_ASTDUMP pAst;
   PHB_ASTBLOCK pBlock;
   HB_BOOL fBlock;

   if( ! HB_COMP_PARAM->fAst )
      return;

   pAst = hb_compAstDump( HB_COMP_PARAM );
   if( pAst->nBlockCount == pAst->nBlockAlloc )
   {
      pAst->nBlockAlloc += HB_AST_ALLOC_BASE;
      pAst->pBlocks = ( HB_ASTBLOCK * ) hb_xrealloc( pAst->pBlocks,
                              pAst->nBlockAlloc * sizeof( HB_ASTBLOCK ) );
   }
   pBlock = &pAst->pBlocks[ pAst->nBlockCount++ ];
   pBlock->pFunc  = hb_compAstOwner( HB_COMP_PARAM, &fBlock );
   pBlock->iLine  = HB_COMP_PARAM->currLine;
   pBlock->nTok   = pAst->nTokenCount ? pAst->nTokenCount - 1 : 0;
   pBlock->cKind  = ( char ) iKind;
   pBlock->cEvent = ( char ) iEvent;
}

/* --- statement serializer ------------------------------------------------ */

static void hb_compAstBufPut( PHB_ASTBUF pBuf, const char * szText, HB_SIZE nLen )
{
   if( pBuf->nLen + nLen >= pBuf->nAlloc )
   {
      pBuf->nAlloc = ( pBuf->nAlloc ? pBuf->nAlloc << 1 : 256 );
      while( pBuf->nLen + nLen >= pBuf->nAlloc )
         pBuf->nAlloc <<= 1;
      pBuf->pData = ( char * ) hb_xrealloc( pBuf->pData, pBuf->nAlloc );
   }
   memcpy( pBuf->pData + pBuf->nLen, szText, nLen );
   pBuf->nLen += nLen;
   pBuf->pData[ pBuf->nLen ] = '\0';
}

static void hb_compAstBufStr( PHB_ASTBUF pBuf, const char * szText )
{
   hb_compAstBufPut( pBuf, szText, strlen( szText ) );
}

static void hb_compAstBufJsonStr( PHB_ASTBUF pBuf, const char * szText, HB_SIZE nLen )
{
   HB_SIZE n;

   hb_compAstBufPut( pBuf, "\"", 1 );
   for( n = 0; n < nLen; ++n )
   {
      unsigned char uc = ( unsigned char ) szText[ n ];
      char szEsc[ 8 ];

      if( uc == '"' || uc == '\\' )
      {
         szEsc[ 0 ] = '\\';
         szEsc[ 1 ] = ( char ) uc;
         hb_compAstBufPut( pBuf, szEsc, 2 );
      }
      else if( uc < ' ' )
      {
         hb_snprintf( szEsc, sizeof( szEsc ), "\\u%04x", uc );
         hb_compAstBufStr( pBuf, szEsc );
      }
      else
         hb_compAstBufPut( pBuf, ( const char * ) &szText[ n ], 1 );
   }
   hb_compAstBufPut( pBuf, "\"", 1 );
}

static void hb_compAstBufFmt( PHB_ASTBUF pBuf, const char * szFmt, ... )
{
   char szText[ 128 ];
   va_list args;

   va_start( args, szFmt );
   hb_vsnprintf( szText, sizeof( szText ), szFmt, args );
   va_end( args );
   hb_compAstBufStr( pBuf, szText );
}

/* names indexed by the contiguous HB_EXPRTYPE enum */
static const char * const s_szExprNames[] = {
   "NONE", "NIL", "NUMERIC", "DATE", "TIMESTAMP", "STRING", "CODEBLOCK",
   "LOGICAL", "SELF", "ARRAY", "HASH", "FUNREF", "VARREF", "REFERENCE",
   "IIF", "LIST", "ARGLIST", "MACROARGLIST", "ARRAYAT", "MACRO", "FUNCALL",
   "ALIASVAR", "ALIASEXPR", "SETGET", "SEND", "FUNNAME", "ALIAS", "RTVAR",
   "VARIABLE", "POSTINC", "POSTDEC", "ASSIGN", "PLUSEQ", "MINUSEQ",
   "MULTEQ", "DIVEQ", "MODEQ", "EXPEQ", "OR", "AND", "NOT", "EQUAL", "EQ",
   "NE", "IN", "LT", "GT", "LE", "GE", "PLUS", "MINUS", "MULT", "DIV",
   "MOD", "POWER", "NEGATE", "PREINC", "PREDEC"
};

static void hb_compAstExprWrite( HB_COMP_DECL, PHB_ASTBUF pBuf, PHB_EXPR pExpr,
                                 HB_SIZE * pnTokMin, HB_SIZE * pnTokMax );

/* write a comma separated array of the pNext chained expression list */
static void hb_compAstExprList( HB_COMP_DECL, PHB_ASTBUF pBuf, PHB_EXPR pExpr,
                                HB_SIZE * pnTokMin, HB_SIZE * pnTokMax )
{
   HB_BOOL fFirst = HB_TRUE;

   hb_compAstBufStr( pBuf, "[" );
   while( pExpr )
   {
      if( ! fFirst )
         hb_compAstBufStr( pBuf, "," );
      fFirst = HB_FALSE;
      hb_compAstExprWrite( HB_COMP_PARAM, pBuf, pExpr, pnTokMin, pnTokMax );
      pExpr = pExpr->pNext;
   }
   hb_compAstBufStr( pBuf, "]" );
}

static void hb_compAstExprChild( HB_COMP_DECL, PHB_ASTBUF pBuf,
                                 const char * szName, PHB_EXPR pExpr,
                                 HB_SIZE * pnTokMin, HB_SIZE * pnTokMax )
{
   if( pExpr )
   {
      hb_compAstBufFmt( pBuf, ",\"%s\":", szName );
      hb_compAstExprWrite( HB_COMP_PARAM, pBuf, pExpr, pnTokMin, pnTokMax );
   }
}

static void hb_compAstExprWrite( HB_COMP_DECL, PHB_ASTBUF pBuf, PHB_EXPR pExpr,
                                 HB_SIZE * pnTokMin, HB_SIZE * pnTokMax )
{
   PHB_ASTDUMP pAst = HB_COMP_PARAM->pAst;
   PHB_ASTNODE pNode = hb_compAstNodeFind( pAst, pExpr );
   HB_EXPRTYPE iType = pExpr->ExprType;

   hb_compAstBufStr( pBuf, "{\"et\":\"" );
   if( iType >= 0 && iType < ( HB_EXPRTYPE ) ( sizeof( s_szExprNames ) /
                                               sizeof( s_szExprNames[ 0 ] ) ) )
      hb_compAstBufStr( pBuf, s_szExprNames[ iType ] );
   else
      hb_compAstBufFmt( pBuf, "ET%d", ( int ) iType );
   hb_compAstBufStr( pBuf, "\"" );

   if( pNode )
   {
      hb_compAstBufFmt( pBuf, ",\"line\":%d,\"tok\":%" HB_PFS "u",
                        pNode->iLine, pNode->nBirthTok );
      if( pNode->nBirthTok < *pnTokMin )
         *pnTokMin = pNode->nBirthTok;
      if( pNode->nBirthTok > *pnTokMax )
         *pnTokMax = pNode->nBirthTok;
   }

   switch( iType )
   {
      case HB_ET_NIL:
      case HB_ET_NONE:
      case HB_ET_SELF:
         break;

      case HB_ET_NUMERIC:
         if( pExpr->value.asNum.NumType == HB_ET_LONG )
            hb_compAstBufFmt( pBuf, ",\"val\":%" PFHL "d", pExpr->value.asNum.val.l );
         else
            hb_compAstBufFmt( pBuf, ",\"val\":%.17g", pExpr->value.asNum.val.d );
         break;

      case HB_ET_LOGICAL:
         hb_compAstBufFmt( pBuf, ",\"val\":%s",
                           pExpr->value.asLogical ? "true" : "false" );
         break;

      case HB_ET_DATE:
         hb_compAstBufFmt( pBuf, ",\"val\":%ld", pExpr->value.asDate.lDate );
         break;

      case HB_ET_TIMESTAMP:
         hb_compAstBufFmt( pBuf, ",\"val\":[%ld,%ld]",
                           pExpr->value.asDate.lDate, pExpr->value.asDate.lTime );
         break;

      case HB_ET_STRING:
         hb_compAstBufStr( pBuf, ",\"val\":" );
         hb_compAstBufJsonStr( pBuf, pExpr->value.asString.string, pExpr->nLength );
         break;

      case HB_ET_VARIABLE:
      case HB_ET_FUNNAME:
      case HB_ET_FUNREF:
      case HB_ET_VARREF:
      case HB_ET_ALIAS:
         if( pExpr->value.asSymbol.name )
         {
            hb_compAstBufStr( pBuf, ",\"val\":" );
            hb_compAstBufJsonStr( pBuf, pExpr->value.asSymbol.name,
                                  strlen( pExpr->value.asSymbol.name ) );
         }
         break;

      case HB_ET_RTVAR:
         if( pExpr->value.asRTVar.szName )
         {
            hb_compAstBufStr( pBuf, ",\"val\":" );
            hb_compAstBufJsonStr( pBuf, pExpr->value.asRTVar.szName,
                                  strlen( pExpr->value.asRTVar.szName ) );
         }
         else
            hb_compAstExprChild( HB_COMP_PARAM, pBuf, "macro",
                                 pExpr->value.asRTVar.pMacro, pnTokMin, pnTokMax );
         break;

      case HB_ET_MACRO:
         if( pExpr->value.asMacro.szMacro )
         {
            hb_compAstBufStr( pBuf, ",\"val\":" );
            hb_compAstBufJsonStr( pBuf, pExpr->value.asMacro.szMacro,
                                  strlen( pExpr->value.asMacro.szMacro ) );
         }
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "expr",
                              pExpr->value.asMacro.pExprList, pnTokMin, pnTokMax );
         break;

      case HB_ET_ARRAY:
      case HB_ET_HASH:
      case HB_ET_LIST:
      case HB_ET_ARGLIST:
      case HB_ET_MACROARGLIST:
      case HB_ET_IIF:
         hb_compAstBufStr( pBuf, ",\"items\":" );
         hb_compAstExprList( HB_COMP_PARAM, pBuf, pExpr->value.asList.pExprList,
                             pnTokMin, pnTokMax );
         break;

      case HB_ET_ARRAYAT:
         hb_compAstBufStr( pBuf, ",\"base\":" );
         hb_compAstExprList( HB_COMP_PARAM, pBuf, pExpr->value.asList.pExprList,
                             pnTokMin, pnTokMax );
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "index",
                              pExpr->value.asList.pIndex, pnTokMin, pnTokMax );
         break;

      case HB_ET_FUNCALL:
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "fun",
                              pExpr->value.asFunCall.pFunName, pnTokMin, pnTokMax );
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "parms",
                              pExpr->value.asFunCall.pParms, pnTokMin, pnTokMax );
         break;

      case HB_ET_SEND:
         if( pExpr->value.asMessage.szMessage )
         {
            hb_compAstBufStr( pBuf, ",\"msg\":" );
            hb_compAstBufJsonStr( pBuf, pExpr->value.asMessage.szMessage,
                                  strlen( pExpr->value.asMessage.szMessage ) );
         }
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "msgmacro",
                              pExpr->value.asMessage.pMessage, pnTokMin, pnTokMax );
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "obj",
                              pExpr->value.asMessage.pObject, pnTokMin, pnTokMax );
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "parms",
                              pExpr->value.asMessage.pParms, pnTokMin, pnTokMax );
         break;

      case HB_ET_ALIASVAR:
      case HB_ET_ALIASEXPR:
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "alias",
                              pExpr->value.asAlias.pAlias, pnTokMin, pnTokMax );
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "var",
                              pExpr->value.asAlias.pVar, pnTokMin, pnTokMax );
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "expr",
                              pExpr->value.asAlias.pExpList, pnTokMin, pnTokMax );
         break;

      case HB_ET_SETGET:
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "var",
                              pExpr->value.asSetGet.pVar, pnTokMin, pnTokMax );
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "expr",
                              pExpr->value.asSetGet.pExpr, pnTokMin, pnTokMax );
         break;

      case HB_ET_CODEBLOCK:
         hb_compAstBufFmt( pBuf, ",\"cbflags\":%d", ( int ) pExpr->value.asCodeblock.flags );
         hb_compAstBufStr( pBuf, ",\"body\":" );
         hb_compAstExprList( HB_COMP_PARAM, pBuf, pExpr->value.asCodeblock.pExprList,
                             pnTokMin, pnTokMax );
         break;

      case HB_ET_REFERENCE:
         hb_compAstExprChild( HB_COMP_PARAM, pBuf, "expr",
                              pExpr->value.asReference, pnTokMin, pnTokMax );
         break;

      default:
         /* operators: unary ones keep pRight == NULL */
         if( iType >= HB_ET_VARIABLE + 1 )
         {
            hb_compAstExprChild( HB_COMP_PARAM, pBuf, "left",
                                 pExpr->value.asOperator.pLeft, pnTokMin, pnTokMax );
            hb_compAstExprChild( HB_COMP_PARAM, pBuf, "right",
                                 pExpr->value.asOperator.pRight, pnTokMin, pnTokMax );
         }
         break;
   }

   hb_compAstBufStr( pBuf, "}" );
}

/* serialize the still-live expression tree of a statement ('s') or of a
   pushed expression such as a condition or a RETURN value ('p'), before
   HB_EA_REDUCE folds it away from what the source says */
void hb_compAstStatement( HB_COMP_DECL, PHB_EXPR pExpr, int iKind )
{
   PHB_ASTDUMP pAst;
   PHB_ASTSTMT pStmt;
   HB_ASTBUF buf;
   HB_SIZE nTokMin, nTokMax;
   HB_BOOL fBlock;

   if( ! HB_COMP_PARAM->fAst || pExpr == NULL )
      return;

   pAst = hb_compAstDump( HB_COMP_PARAM );

   buf.pData = NULL;
   buf.nLen = buf.nAlloc = 0;
   nTokMin = ( HB_SIZE ) -1;
   nTokMax = 0;
   hb_compAstExprWrite( HB_COMP_PARAM, &buf, pExpr, &nTokMin, &nTokMax );

   if( pAst->nStmtCount == pAst->nStmtAlloc )
   {
      pAst->nStmtAlloc += HB_AST_ALLOC_BASE * 4;
      pAst->pStmts = ( HB_ASTSTMT * ) hb_xrealloc( pAst->pStmts,
                              pAst->nStmtAlloc * sizeof( HB_ASTSTMT ) );
   }
   pStmt = &pAst->pStmts[ pAst->nStmtCount++ ];
   pStmt->pFunc  = hb_compAstOwner( HB_COMP_PARAM, &fBlock );
   pStmt->iLine  = HB_COMP_PARAM->currLine;
   pStmt->szJson = buf.pData;
   pStmt->cKind  = ( char ) iKind;
   pStmt->fBlock = fBlock;

   HB_SYMBOL_UNUSED( nTokMin );
   HB_SYMBOL_UNUSED( nTokMax );
}

/* --- JSON output --------------------------------------------------------- */

static const char * hb_compAstScopeName( int iScope )
{
   switch( iScope & ~HB_VS_FILEWIDE )
   {
      case HB_VS_CBLOCAL_VAR:
         return "detached";
      case HB_VS_LOCAL_VAR:
         return "local";
      case HB_VS_LOCAL_MEMVAR:
         return "memvar";
      case HB_VS_LOCAL_FIELD:
         return "field";
      case HB_VS_STATIC_VAR:
         return "static";
   }
   return "memvar_implicit";   /* HB_VS_UNDECLARED */
}

static const char * hb_compAstAccessName( int iAccess )
{
   switch( iAccess )
   {
      case 'r':
         return "read";
      case 'w':
         return "write";
      case 'x':
         return "ref";
   }
   return "use";
}

static const char * hb_compAstBlockKind( char cKind )
{
   switch( cKind )
   {
      case 'i':
         return "if";
      case 'w':
         return "while";
      case 'f':
         return "for";
      case 'c':
         return "case";
      case 's':
         return "switch";
      case 'q':
         return "sequence";
      case 'b':
         return "codeblock";
   }
   return "?";
}

static void hb_compAstWriteStr( FILE * file, const char * szText )
{
   fputc( '"', file );
   while( *szText )
   {
      unsigned char uc = ( unsigned char ) *szText++;

      if( uc == '"' || uc == '\\' )
         fprintf( file, "\\%c", uc );
      else if( uc < ' ' )
         fprintf( file, "\\u%04x", uc );
      else
         fputc( uc, file );
   }
   fputc( '"', file );
}

static HB_BOOL hb_compAstHasMacro( PHB_HFUNC pFunc )
{
   HB_SIZE nPos = 0;

   while( nPos < pFunc->nPCodePos )
   {
      HB_ISIZ nSize;

      switch( pFunc->pCode[ nPos ] )
      {
         case HB_P_MACROPOP:
         case HB_P_MACROPOPALIASED:
         case HB_P_MACROPUSH:
         case HB_P_MACROARRAYGEN:
         case HB_P_MACROPUSHLIST:
         case HB_P_MACROPUSHINDEX:
         case HB_P_MACROPUSHPARE:
         case HB_P_MACROPUSHALIASED:
         case HB_P_MACROSYMBOL:
         case HB_P_MACROTEXT:
         case HB_P_MACROFUNC:
         case HB_P_MACRODO:
         case HB_P_MACROPUSHREF:
         case HB_P_MACROSEND:
         case HB_P_MMESSAGE:
            return HB_TRUE;
      }
      nSize = hb_compPCodeSize( pFunc, nPos );
      if( nSize <= 0 )
         break;
      nPos += nSize;
   }
   return HB_FALSE;
}

static int hb_compAstFuncLine( PHB_ASTDUMP pAst, PHB_HFUNC pFunc )
{
   HB_SIZE n;

   for( n = 0; n < pAst->nFuncCount; ++n )
   {
      if( pAst->pFuncs[ n ].pFunc == pFunc )
         return pAst->pFuncs[ n ].iLine;
   }
   return 0;
}

static void hb_compAstWriteVars( FILE * file, PHB_HVAR pVar,
                                 const char * szScope, int iParamCount,
                                 HB_BOOL * pfFirst )
{
   int iVarPos = 1;

   while( pVar )
   {
      if( ! *pfFirst )
         fprintf( file, "," );
      *pfFirst = HB_FALSE;
      fprintf( file, "\n      { \"sym\": " );
      hb_compAstWriteStr( file, pVar->szName );
      fprintf( file, ", \"scope\": \"%s\", \"declLine\": %d, \"used\": %d, \"param\": %s }",
               szScope, pVar->iDeclLine, pVar->iUsed,
               iVarPos <= iParamCount ? "true" : "false" );
      ++iVarPos;
      pVar = pVar->pNext;
   }
}

HB_BOOL hb_compAstSave( HB_COMP_DECL )
{
   PHB_ASTDUMP pAst;
   HB_FNAME    FileName;
   char        szFileName[ HB_PATH_MAX ];
   char        szModuleName[ HB_PATH_MAX ];
   char *      szText;
   PHB_HFUNC   pFunc;
   FILE *      file;
   HB_BOOL     fFirstFunc;
   HB_SIZE     n;

   pAst = HB_COMP_PARAM->pAst;
   if( ! pAst || ! HB_COMP_PARAM->fAst )
      return HB_FALSE;

   FileName.szPath            =
      FileName.szName         =
         FileName.szExtension =
            FileName.szDrive  = NULL;

   if( HB_COMP_PARAM->pOutPath )
   {
      FileName.szDrive = HB_COMP_PARAM->pOutPath->szDrive;
      FileName.szPath  = HB_COMP_PARAM->pOutPath->szPath;
   }

   if( HB_COMP_PARAM->pAstFileName )
   {
      if( HB_COMP_PARAM->pAstFileName->szName )
         FileName.szName = HB_COMP_PARAM->pAstFileName->szName;

      if( HB_COMP_PARAM->pAstFileName->szExtension )
         FileName.szExtension = HB_COMP_PARAM->pAstFileName->szExtension;

      if( HB_COMP_PARAM->pAstFileName->szPath )
      {
         FileName.szDrive = HB_COMP_PARAM->pAstFileName->szDrive;
         FileName.szPath  = HB_COMP_PARAM->pAstFileName->szPath;
      }
   }

   if( ! FileName.szName )
   {
      /* pFileName is switched to the OUTPUT file during code generation;
         the source module name captured at parse time is the right one */
      if( pAst->szModule )
      {
         PHB_FNAME pMod = hb_fsFNameSplit( pAst->szModule );

         hb_strncpy( szModuleName, pMod->szName ? pMod->szName : "",
                     sizeof( szModuleName ) - 1 );
         hb_xfree( pMod );
         FileName.szName = szModuleName;
      }
      else
         FileName.szName = HB_COMP_PARAM->pFileName->szName;
   }

   if( ! FileName.szExtension )
      FileName.szExtension = ".ast.json";

   hb_fsFNameMerge( szFileName, &FileName );

   file = hb_fopen( szFileName, "w" );

   if( ! file )
   {
      hb_compGenError( HB_COMP_PARAM, hb_comp_szErrors, 'E', HB_COMP_ERR_CREATE_OUTPUT, szFileName, NULL );
      return HB_FALSE;
   }

   szText = hb_verHarbour();
   fprintf( file, "{\n  \"schema\": \"%s\",\n  \"generator\": ", HB_AST_SCHEMA );
   hb_compAstWriteStr( file, szText );
   hb_xfree( szText );

   fprintf( file, ",\n  \"module\": " );
   hb_compAstWriteStr( file, pAst->szModule ? pAst->szModule : "" );

   fprintf( file, ",\n  \"hasCDump\": %s,",
            HB_COMP_PARAM->inlines.iCount > 0 ? "true" : "false" );

   /* the consumed token stream: index in this array is the id that the
      statement nodes reference through their "tok" field */
   fprintf( file, "\n  \"tokens\": [" );
   for( n = 0; n < pAst->nTokenCount; ++n )
   {
      PHB_ASTTOKEN pTok = &pAst->pTokens[ n ];

      fprintf( file, "%s\n    { \"line\": %d, ", n ? "," : "", pTok->iLine );
      if( pTok->iCol >= 0 )
         fprintf( file, "\"col\": %d, ", pTok->iCol );
      else
         fprintf( file, "\"col\": null, " );
      fprintf( file, "\"len\": %" HB_PFS "u, \"type\": %d, \"prov\": \"%c\", \"text\": ",
               pTok->nLen, ( int ) pTok->type, pTok->cProv );
      hb_compAstWriteStr( file, pTok->szText );
      fprintf( file, " }" );
   }
   fprintf( file, "%s ],", pAst->nTokenCount ? "\n  " : "" );

   fprintf( file, "\n  \"functions\": [" );

   fFirstFunc = HB_TRUE;
   pFunc = HB_COMP_PARAM->functions.pFirst;
   while( pFunc )
   {
      HB_BOOL fFirst;

      if( ! fFirstFunc )
         fprintf( file, "," );
      fFirstFunc = HB_FALSE;

      fprintf( file, "\n  { \"name\": " );
      hb_compAstWriteStr( file, pFunc->szName ? pFunc->szName : "" );
      fprintf( file, ",\n    \"kind\": \"%s\", \"static\": %s, \"fileDecl\": %s, \"line\": %d, \"usesMacro\": %s,",
               ( pFunc->funFlags & HB_FUNF_PROCEDURE ) ? "procedure" : "function",
               ( pFunc->cScope & HB_FS_STATIC ) ? "true" : "false",
               ( pFunc->funFlags & HB_FUNF_FILE_DECL ) ? "true" : "false",
               hb_compAstFuncLine( pAst, pFunc ),
               hb_compAstHasMacro( pFunc ) ? "true" : "false" );

      fprintf( file, "\n    \"declarations\": [" );
      fFirst = HB_TRUE;
      hb_compAstWriteVars( file, pFunc->pLocals, "local", pFunc->wParamCount, &fFirst );
      hb_compAstWriteVars( file, pFunc->pStatics, "static", 0, &fFirst );
      hb_compAstWriteVars( file, pFunc->pFields, "field", 0, &fFirst );
      hb_compAstWriteVars( file, pFunc->pMemvars, "memvar", 0, &fFirst );
      hb_compAstWriteVars( file, pFunc->pPrivates, "private", 0, &fFirst );
      fprintf( file, "%s ],", fFirst ? "" : "\n   " );

      fprintf( file, "\n    \"occurrences\": [" );
      fFirst = HB_TRUE;
      for( n = 0; n < pAst->nUseCount; ++n )
      {
         PHB_ASTUSE pUse = &pAst->pUses[ n ];

         if( pUse->pFunc == pFunc )
         {
            if( ! fFirst )
               fprintf( file, "," );
            fFirst = HB_FALSE;
            fprintf( file, "\n      { \"sym\": " );
            hb_compAstWriteStr( file, pUse->szSym );
            fprintf( file, ", \"scope\": \"%s\", \"line\": %d, \"access\": \"%s\", \"block\": %s%s }",
                     hb_compAstScopeName( pUse->iScope ),
                     pUse->iLine,
                     hb_compAstAccessName( pUse->iAccess ),
                     pUse->fBlock ? "true" : "false",
                     ( pUse->iScope & HB_VS_FILEWIDE ) ? ", \"filewide\": true" : "" );
         }
      }
      fprintf( file, "%s ],", fFirst ? "" : "\n   " );

      fprintf( file, "\n    \"calls\": [" );
      fFirst = HB_TRUE;
      for( n = 0; n < pAst->nCallCount; ++n )
      {
         PHB_ASTCALL pCall = &pAst->pCalls[ n ];

         if( pCall->pFunc == pFunc )
         {
            if( ! fFirst )
               fprintf( file, "," );
            fFirst = HB_FALSE;
            fprintf( file, "\n      { \"sym\": " );
            hb_compAstWriteStr( file, pCall->szSym );
            fprintf( file, ", \"line\": %d, \"block\": %s }",
                     pCall->iLine, pCall->fBlock ? "true" : "false" );
         }
      }
      fprintf( file, "%s ],", fFirst ? "" : "\n   " );

      fprintf( file, "\n    \"sends\": [" );
      fFirst = HB_TRUE;
      for( n = 0; n < pAst->nSendCount; ++n )
      {
         PHB_ASTCALL pSend = &pAst->pSends[ n ];

         if( pSend->pFunc == pFunc )
         {
            if( ! fFirst )
               fprintf( file, "," );
            fFirst = HB_FALSE;
            fprintf( file, "\n      { \"sym\": " );
            hb_compAstWriteStr( file, pSend->szSym );
            fprintf( file, ", \"line\": %d, \"block\": %s }",
                     pSend->iLine, pSend->fBlock ? "true" : "false" );
         }
      }
      fprintf( file, "%s ],", fFirst ? "" : "\n   " );

      fprintf( file, "\n    \"blocks\": [" );
      fFirst = HB_TRUE;
      for( n = 0; n < pAst->nBlockCount; ++n )
      {
         PHB_ASTBLOCK pBlock = &pAst->pBlocks[ n ];

         if( pBlock->pFunc == pFunc )
         {
            if( ! fFirst )
               fprintf( file, "," );
            fFirst = HB_FALSE;
            fprintf( file, "\n      { \"kind\": \"%s\", \"event\": \"%s\", \"line\": %d, \"tok\": %" HB_PFS "u }",
                     hb_compAstBlockKind( pBlock->cKind ),
                     pBlock->cEvent == 'o' ? "open" :
                     pBlock->cEvent == 'c' ? "close" : "mid",
                     pBlock->iLine, pBlock->nTok );
         }
      }
      fprintf( file, "%s ],", fFirst ? "" : "\n   " );

      fprintf( file, "\n    \"statements\": [" );
      fFirst = HB_TRUE;
      for( n = 0; n < pAst->nStmtCount; ++n )
      {
         PHB_ASTSTMT pStmt = &pAst->pStmts[ n ];

         if( pStmt->pFunc == pFunc )
         {
            if( ! fFirst )
               fprintf( file, "," );
            fFirst = HB_FALSE;
            fprintf( file, "\n      { \"kind\": \"%s\", \"line\": %d, \"block\": %s, \"expr\": %s }",
                     pStmt->cKind == 's' ? "stmt" : "push",
                     pStmt->iLine, pStmt->fBlock ? "true" : "false",
                     pStmt->szJson ? pStmt->szJson : "null" );
         }
      }
      fprintf( file, "%s ] }", fFirst ? "" : "\n   " );

      pFunc = pFunc->pNext;
   }

   fprintf( file, " ]\n}\n" );
   fclose( file );

   return HB_TRUE;
}

void hb_compAstFree( HB_COMP_DECL )
{
   PHB_ASTDUMP pAst = HB_COMP_PARAM->pAst;

   if( pAst )
   {
      HB_SIZE n;

      for( n = 0; n < pAst->nTokenCount; ++n )
         hb_xfree( pAst->pTokens[ n ].szText );
      for( n = 0; n < pAst->nStmtCount; ++n )
      {
         if( pAst->pStmts[ n ].szJson )
            hb_xfree( pAst->pStmts[ n ].szJson );
      }
      if( pAst->pTokens )
         hb_xfree( pAst->pTokens );
      if( pAst->pNodes )
         hb_xfree( pAst->pNodes );
      if( pAst->pFuncs )
         hb_xfree( pAst->pFuncs );
      if( pAst->pUses )
         hb_xfree( pAst->pUses );
      if( pAst->pCalls )
         hb_xfree( pAst->pCalls );
      if( pAst->pSends )
         hb_xfree( pAst->pSends );
      if( pAst->pBlocks )
         hb_xfree( pAst->pBlocks );
      if( pAst->pStmts )
         hb_xfree( pAst->pStmts );
      if( pAst->szModule )
         hb_xfree( pAst->szModule );
      hb_xfree( pAst );
      HB_COMP_PARAM->pAst = NULL;
   }
}
