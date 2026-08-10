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

#define HB_AST_SCHEMA         "ast-26"

/* how much of a dump is read back to find the provenance block: it sits in
   the first lines, before the token stream, so a fixed head is enough */
#define HB_AST_PROV_HEAD      ( 256 * 1024 )
#define HB_AST_ALLOC_BASE     64

/* one derivation fact of a synthesized token (see hb_pp_tokenFromGet()):
   the byte range [nAt, nAt+nLen) of the token text derives from match
   marker iMarker of tracked application iApp */
typedef struct
{
   int     iApp;
   int     iMarker;
   char    cOp;               /* 'c'lone, 'p'aste, 's'tringify */
   HB_SIZE nAt;
   HB_SIZE nLen;
} HB_ASTFROM, * PHB_ASTFROM;

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
   PHB_ASTFROM pFrom;         /* derivation facts, NULL = none */
   int       iFromCount;
   /* ast-23: the rule application that PRODUCED this token, -1 = none.
      Answers "which directive wrote this name?" for a token whose only
      written position is in another file - the 40% of sites that a .ch
      contributes in real Harbour code. */
   int       iApp;
} HB_ASTTOKEN, * PHB_ASTTOKEN;

/* "this node has no token of its own" - and it stays that way rather than
   borrowing one from a neighbour (ast-21) */
#define HB_AST_TOK_NONE       ( ( HB_SIZE ) -1 )

/* birth record of an expression node: the pointer is only ever looked up
   while the node is alive (during hb_compAstStatement() serialization of
   the still-live tree), so entries of freed nodes are never reached */
typedef struct
{
   const void * pKey;         /* PHB_EXPR at birth */
   int          iLine;
   HB_SIZE      nBirthTok;    /* token counter value at birth */
   /* ast-21: the token that spells this node's NAME, handed over by the
      parser (@N) - unlike nBirthTok, which is only where the token counter
      happened to stand when the node was allocated, and is off by whatever
      lookahead the parser needed */
   HB_SIZE      nNameTok;
} HB_ASTNODE, * PHB_ASTNODE;

typedef struct
{
   PHB_HFUNC pFunc;
   int       iLine;
} HB_ASTFUNC, * PHB_ASTFUNC;

/* a variable declaration captured at parse time - BEFORE the pcode
   optimizer prunes pLocals (it deletes the hbclass Self and unused
   locals at function end) and independent of the -w3 gate of the
   declaration subsystem: the written type/class NAMES are the fact */
typedef struct
{
   const char * szSym;        /* interned identifier */
   PHB_HFUNC    pFunc;        /* resolved owner function */
   int          iLine;
   int          iScope;       /* HB_VSCOMP_* value at declaration */
   HB_BYTE      cType;        /* declared type char, ' ' = none */
   const char * szClass;      /* AS CLASS name as written, NULL = none */
   HB_BOOL      fDim;         /* dimensioned form (LOCAL a[ n ]): the 'A'
                                 is the array form's internal mark, not a
                                 written AS annotation (ast-7) */
   HB_BOOL      fChk;         /* -kt PROLOGUE check emitted for this
                                 parameter declaration (ast-8) */
   int          iNameLine;    /* position of the WRITTEN name token
                                 (ast-9 anchor fact); iNameCol -1 = the
                                 name has no plain-source token to
                                 anchor to (directive result, include,
                                 synthesized) - the fact stays absent */
   int          iNameCol;
} HB_ASTDECL, * PHB_ASTDECL;

typedef struct
{
   const char * szSym;        /* interned identifier */
   PHB_HFUNC    pFunc;        /* resolved owner function */
   int          iLine;
   int          iScope;       /* HB_VS_* value */
   int          iAccess;      /* 'r' read, 'w' write, 'x' by ref, 'u' use */
   HB_BOOL      fBlock;       /* reference made inside a codeblock body */
   HB_BOOL      fChk;         /* -kt post-store check emitted right after
                                 this write (ast-8) */
   /* ast-21: the token that spells this site's name, as the parser handed it
      over.  HB_AST_TOK_NONE when the chain does not reach - a name the
      compiler made up, or one that arrived through a macro */
   HB_SIZE      nTok;
} HB_ASTUSE, * PHB_ASTUSE;

typedef struct
{
   const char * szSym;
   PHB_HFUNC    pFunc;
   int          iLine;
   HB_BOOL      fBlock;
   HB_SIZE      nTok;         /* ast-21, same as HB_ASTUSE */
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
   HB_BOOL   fRet;            /* pushed expression is a RETURN value */
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

   HB_ASTDECL *  pDecls;
   HB_SIZE       nDeclCount;
   HB_SIZE       nDeclAlloc;

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

   HB_BOOL       fRetPending; /* next pushed expression is a RETURN value */

   /* ast-21: the node whose pcode is being generated, marked by HB_EXPR_USE()
      so the site recorders can reach the token the parser gave it */
   PHB_EXPR      pCurNode;

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
   if( ! hb_pp_tokenAppGet( HB_COMP_PARAM->pLex->pPP, pToken, &pTok->iApp ) )
      pTok->iApp = -1;

   /* derivation facts must be copied now: the pp entry dies with the
      token, the dump is written much later */
   pTok->pFrom = NULL;
   pTok->iFromCount = hb_pp_tokenFromCount( HB_COMP_PARAM->pLex->pPP, pToken );
   if( pTok->iFromCount > 0 )
   {
      int i;

      pTok->pFrom = ( PHB_ASTFROM ) hb_xgrab( pTok->iFromCount *
                                              sizeof( HB_ASTFROM ) );
      for( i = 0; i < pTok->iFromCount; ++i )
      {
         PHB_ASTFROM pFrom = &pTok->pFrom[ i ];

         hb_pp_tokenFromGet( HB_COMP_PARAM->pLex->pPP, pToken, i,
                             &pFrom->iApp, &pFrom->iMarker, &pFrom->cOp,
                             &pFrom->nAt, &pFrom->nLen );
      }
   }
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
   /* a fresh birth on a recycled pointer must not inherit the name token of
      the dead node that used to live there */
   pAst->pNodes[ nAt ].nNameTok = HB_AST_TOK_NONE;
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

/* ast-21: the lexer stamping the symbol it is about to hand to the parser.
   Called for EVERY symbol, including when -x is off - so it does the least
   work possible and never allocates: the dump may not even exist yet. */
void hb_compAstTokMark( HB_COMP_DECL, HB_COMP_YYLTYPE * pLoc )
{
   PHB_ASTDUMP pAst = ( PHB_ASTDUMP ) HB_COMP_PARAM->pAst;

   pLoc->nTok = ( HB_COMP_PARAM->fAst && pAst && pAst->nTokenCount )
                ? pAst->nTokenCount - 1 : HB_AST_TOK_NONE;
}

/* ast-21: a rule action handing over the token of the name it just read.
   Returns the node so it can wrap the constructor in place:

      $$ = hb_compAstNodeAt( HB_COMP_PARAM, hb_compExprNewVar( $1, ... ), @1 );

   This is the ONLY way a node acquires a name token.  A node built somewhere
   the parser cannot reach - a variable the compiler synthesizes, a name that
   arrived through a macro - keeps HB_AST_TOK_NONE and is reported without a
   column, which is the truth about it. */
PHB_EXPR hb_compAstNodeAt( HB_COMP_DECL, PHB_EXPR pExpr, HB_COMP_YYLTYPE loc )
{
   if( HB_COMP_PARAM->fAst && pExpr )
   {
      PHB_ASTNODE pNode = hb_compAstNodeFind( hb_compAstDump( HB_COMP_PARAM ), pExpr );

      if( pNode )
         pNode->nNameTok = loc.nTok;
   }
   return pExpr;
}

/* ast-21: the expression walk, saying which node it is on.

   The three site recorders below run deep inside code generation, where the
   only thing in hand is a name - by then the tree walk is several calls away.
   Every walk step goes through here (HB_EXPR_USE), so this is where a recorder
   can ask "which node am I generating for?" and read the token the parser gave
   that node.

   It saves and restores rather than just assigning because expressions nest:
   generating a send evaluates the object expression first, and that inner walk
   moves the mark to nodes of its own before the send records its site. */
PHB_EXPR hb_compAstExprUse( PHB_EXPR pSelf, HB_EXPR_MESSAGE iMessage, HB_COMP_DECL )
{
   PHB_ASTDUMP pAst = hb_compAstDump( HB_COMP_PARAM );
   PHB_EXPR pSave = pAst->pCurNode, pRet;

   pAst->pCurNode = pSelf;
   pRet = hb_comp_ExprTable[ pSelf->ExprType ]( pSelf, iMessage, HB_COMP_PARAM );
   pAst->pCurNode = pSave;

   return pRet;
}

/* ast-21: mark/unmark the node a piece of generated pcode belongs to.  See
   HB_AST_SITE_BEGIN in hbexprop.h for why the walk alone is not enough */
PHB_EXPR hb_compAstNodeOn( HB_COMP_DECL, PHB_EXPR pNode )
{
   PHB_ASTDUMP pAst = hb_compAstDump( HB_COMP_PARAM );
   PHB_EXPR pSave = pAst->pCurNode;

   pAst->pCurNode = pNode;

   return pSave;
}

void hb_compAstNodeOff( HB_COMP_DECL, PHB_EXPR pSave )
{
   ( ( PHB_ASTDUMP ) HB_COMP_PARAM->pAst )->pCurNode = pSave;
}

/* ast-21: the read the assignment optimizer is about to throw away.

   `var := var <op> exp` is rewritten to `var <op>= exp` and the node of the
   middle operand - the second `var` AS THE PROGRAMMER WROTE IT - is freed at
   reduce time, so code generation never walks it and no site is ever recorded
   for it.  The pcode is right; the record of the source is not, and a tool
   answering "where is this name used" would leave out an occurrence that is
   plainly there in the file.

   The scope comes from a scope-only query (piPos NULL), which by contract of
   hb_compVariableFind() records nothing - the site written here is the only
   one this adds. */
void hb_compAstFoldedRead( HB_COMP_DECL, PHB_EXPR pNode )
{
   int iScope;
   PHB_EXPR pSave;

   if( ! HB_COMP_PARAM->fAst || ! pNode || pNode->ExprType != HB_ET_VARIABLE )
      return;

   hb_compVariableFind( HB_COMP_PARAM, pNode->value.asSymbol.name, NULL, &iScope );

   pSave = hb_compAstNodeOn( HB_COMP_PARAM, pNode );
   hb_compAstUse( HB_COMP_PARAM, pNode->value.asSymbol.name, iScope, 'r' );
   hb_compAstNodeOff( HB_COMP_PARAM, pSave );
}

/* ast-21: resolve the variable a NODE names, recording the site against THAT
   node instead of against whatever the walk is standing on.

   The operator optimizations in hbexprb.c (n += 1, n++, n[ i ] := v ...) read
   the name straight out of their left operand while the walk is marked on the
   operator itself.  The operand is the node the parser gave a token to, so
   without saying which node the name came from the site would come out with
   no position at all - the capture of `nTotal` by a codeblock in
   `nTotal += x` was exactly that. */
PHB_HVAR hb_compAstVarFind( HB_COMP_DECL, PHB_EXPR pNode, int * piVar, int * piScope )
{
   PHB_ASTDUMP pAst;
   PHB_EXPR pSave;
   PHB_HVAR pVar;

   if( ! HB_COMP_PARAM->fAst )
      return hb_compVariableFind( HB_COMP_PARAM, pNode->value.asSymbol.name,
                                  piVar, piScope );

   pAst = hb_compAstDump( HB_COMP_PARAM );
   pSave = pAst->pCurNode;
   pAst->pCurNode = pNode;
   pVar = hb_compVariableFind( HB_COMP_PARAM, pNode->value.asSymbol.name,
                               piVar, piScope );
   pAst->pCurNode = pSave;

   return pVar;
}

/* ast-21: the source token of the site being recorded, or HB_AST_TOK_NONE.

   The name is checked against the marked node because code generation does
   not always flow through the node that owns the name: the compiler pushes
   variables of its own making (a FOR enumerator, an implicit Self), and those
   sites must come out WITHOUT a position rather than with the position of
   whatever node happens to be marked.  Identifiers are interned, so this is a
   pointer comparison, not a text one. */
/* the interned name a node spells, for the nodes that spell one */
static const char * hb_compAstNodeName( PHB_EXPR pExpr )
{
   switch( pExpr->ExprType )
   {
      case HB_ET_VARIABLE:
      case HB_ET_VARREF:
      case HB_ET_FUNNAME:
      case HB_ET_ALIAS:
         return pExpr->value.asSymbol.name;
      case HB_ET_SEND:
         return pExpr->value.asMessage.szMessage;
      default:
         return NULL;
   }
}

static HB_SIZE hb_compAstSiteTok( PHB_ASTDUMP pAst, const char * szName )
{
   PHB_ASTNODE pNode;
   const char * szNode;

   if( ! pAst->pCurNode || ! szName )
      return HB_AST_TOK_NONE;

   pNode = hb_compAstNodeFind( pAst, pAst->pCurNode );
   if( ! pNode || pNode->nNameTok == HB_AST_TOK_NONE )
      return HB_AST_TOK_NONE;

   szNode = hb_compAstNodeName( pAst->pCurNode );
   if( ! szNode )
      return HB_AST_TOK_NONE;

   /* an assignment to a member is generated under the underscore name this
      compiler makes up for it (o:X := v -> _X) while the node - and the
      source - spell X.  Undoing our own mangling is reading our own record */
   if( szNode != szName &&
       ! ( szName[ 0 ] == '_' && strcmp( szName + 1, szNode ) == 0 ) )
      return HB_AST_TOK_NONE;

   return pNode->nNameTok;
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

/* anchor fact (ast-9): position of the WRITTEN name token of a
   declaration captured right after the parser consumed it - the
   nearest preceding token whose text is the name. The scan stops at
   the FIRST text match: when that token is not plain source (an
   include, a synthesized directive result) there is no writable
   position and the fact stays absent - never a guess. The window
   covers the declaration tail the parser may have consumed after the
   name (AS CLASS <name>, ',', ':=' lookahead). When the variable is
   named after its own AS CLASS class the nearest match is the class
   name token - szSkipClass skips exactly that one */
#define HB_AST_NAMEPOS_WINDOW  16

/* ast-21: write the position of a site from the token the parser handed
   over, and say WHERE it is when that is not the line the record carries.

   `line` is the line the compiler was ON when the site was recorded.  For a
   statement continued with ';' that is the LAST physical line of it, so a
   site written earlier in the statement - `OutStd( "x" + ;` / `cMsg + ;` /
   `"y" )` - carries a line its token is not on.  `tokLine` is emitted only
   when the two differ, so its mere presence tells a consumer "this site is
   not where `line` says", and its absence keeps the common record as short
   as it always was.  `line` NEVER changes meaning - consumers correlating
   sites with other channels by it keep working.

   A site whose token is not plain source of this module gets NO position.
   That is the honest answer and not a shortcut: the name of such a site is
   written in a header or produced by a directive, so there is no place in
   this file to point at - and the `from` and `ppApplications` channels are
   what describe where it really comes from. */
static void hb_compAstWriteSitePos( FILE * file, PHB_ASTDUMP pAst,
                                    HB_SIZE nTok, int iLine )
{
   PHB_ASTTOKEN pTok;

   if( nTok == HB_AST_TOK_NONE || nTok >= pAst->nTokenCount )
      return;

   pTok = &pAst->pTokens[ nTok ];

   if( pTok->cProv != 's' || pTok->iCol < 0 || pTok->iLine <= 0 )
   {
      /* ast-23: sem posicao NESTE arquivo - mas se o nome foi escrito por uma
         diretiva, o lugar que o programador de fato editaria e' a APLICACAO
         dela, que ppApplications[] ja' publica com linha, coluna e tamanho.
         E' o unico fato que existe sobre este sitio, e sem ele o consumidor
         recebia so' a linha do statement (40,3% dos sitios de codigo real).
         O indice, nunca "a aplicacao que esta' na mesma linha": duas diretivas
         numa linha tornariam isso adivinhacao. */
      if( pTok->iApp >= 0 )
         fprintf( file, ", \"app\": %d", pTok->iApp );
      return;
   }

   if( pTok->iLine != iLine )
      fprintf( file, ", \"tokLine\": %d", pTok->iLine );
   fprintf( file, ", \"col\": %d", pTok->iCol );
}

static void hb_compAstNamePos( PHB_ASTDUMP pAst, const char * szName,
                               const char * szSkipClass,
                               int * piLine, int * piCol )
{
   HB_SIZE n = pAst->nTokenCount;
   HB_SIZE nStop = n > HB_AST_NAMEPOS_WINDOW ? n - HB_AST_NAMEPOS_WINDOW : 0;
   HB_BOOL fSkip = szSkipClass && hb_stricmp( szName, szSkipClass ) == 0;

   *piLine = 0;
   *piCol  = -1;
   while( n-- > nStop )
   {
      PHB_ASTTOKEN pTok = &pAst->pTokens[ n ];

      if( pTok->type == HB_PP_TOKEN_KEYWORD &&
          hb_stricmp( pTok->szText, szName ) == 0 )
      {
         if( fSkip )
         {
            fSkip = HB_FALSE;
            continue;
         }
         if( pTok->cProv == 's' && pTok->iCol >= 0 )
         {
            *piLine = pTok->iLine;
            *piCol  = pTok->iCol;
         }
         return;
      }
   }
}

/* a variable declaration accepted by hb_compVariableAdd(); the declared
   type/class come from the HB_VARTYPE node as WRITTEN (independent of
   the -w3 class resolution and of the pcode optimizer) */
void hb_compAstDecl( HB_COMP_DECL, const char * szVarName, PHB_VARTYPE pVarType )
{
   PHB_ASTDUMP pAst;
   PHB_ASTDECL pDecl;
   HB_BOOL fBlock;

   if( ! HB_COMP_PARAM->fAst )
      return;

   pAst = hb_compAstDump( HB_COMP_PARAM );
   if( pAst->nDeclCount == pAst->nDeclAlloc )
   {
      pAst->nDeclAlloc += HB_AST_ALLOC_BASE;
      pAst->pDecls = ( HB_ASTDECL * ) hb_xrealloc( pAst->pDecls,
                              pAst->nDeclAlloc * sizeof( HB_ASTDECL ) );
   }
   pDecl = &pAst->pDecls[ pAst->nDeclCount++ ];
   pDecl->szSym   = szVarName;
   pDecl->pFunc   = hb_compAstOwner( HB_COMP_PARAM, &fBlock );
   pDecl->iLine   = HB_COMP_PARAM->currLine;
   pDecl->iScope  = HB_COMP_PARAM->iVarScope;
   pDecl->cType   = pVarType ? pVarType->cVarType : ' ';
   pDecl->szClass = pVarType ? pVarType->szFromClass : NULL;
   pDecl->fDim    = HB_FALSE;
   pDecl->fChk    = HB_FALSE;
   /* block-scope declarations materialize at the END of the block, when
      the token stream is already past the body - their position was
      captured at parse and arrives by the hb_compAstDeclPos() retro-tag
      (inline and extended materialization loops) */
   if( fBlock )
   {
      pDecl->iNameLine = 0;
      pDecl->iNameCol  = -1;
   }
   else
      hb_compAstNamePos( pAst, szVarName, pDecl->szClass,
                         &pDecl->iNameLine, &pDecl->iNameCol );
}

/* retro-tags the declaration just recorded with the written name token
   position carried from parse time (block parameters: HB_CBVAR fields,
   same pattern as hb_compAstDeclDim) */
void hb_compAstDeclPos( HB_COMP_DECL, int iNameLine, int iNameCol )
{
   PHB_ASTDUMP pAst = HB_COMP_PARAM->pAst;

   if( pAst && pAst->nDeclCount && iNameCol >= 0 )
   {
      pAst->pDecls[ pAst->nDeclCount - 1 ].iNameLine = iNameLine;
      pAst->pDecls[ pAst->nDeclCount - 1 ].iNameCol  = iNameCol;
   }
}

/* stamps the LAST codeblock parameter of pCB with the position of its
   written name token - called from the BlockVarList grammar action,
   where the body is not parsed yet so the nearest match IS the written
   parameter (the position then travels inside HB_CBVAR until the block
   materializes its locals) */
void hb_compAstCBVarPos( HB_COMP_DECL, PHB_EXPR pCB )
{
   PHB_CBVAR pVar;

   if( ! HB_COMP_PARAM->fAst || pCB == NULL )
      return;

   pVar = pCB->value.asCodeblock.pLocals;
   if( pVar )
   {
      while( pVar->pNext )
         pVar = pVar->pNext;
      hb_compAstNamePos( hb_compAstDump( HB_COMP_PARAM ), pVar->szName,
                         pVar->szFromClass, &pVar->iPosLine, &pVar->iPosCol );
   }
}

/* retro-tags the declaration just recorded as the DIMENSIONED form
   (same pattern as hb_compAstTag): hb_compVariableDim() only knows it
   right after hb_compVariableAdd() already fired the capture */
void hb_compAstDeclDim( HB_COMP_DECL )
{
   PHB_ASTDUMP pAst = HB_COMP_PARAM->pAst;

   if( pAst && pAst->nDeclCount )
      pAst->pDecls[ pAst->nDeclCount - 1 ].fDim = HB_TRUE;
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
   pUse->fChk    = HB_FALSE;
   pUse->nTok = hb_compAstSiteTok( pAst, szVarName );
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

/* retro-tags the write just recorded as IMPOSED: the -kt post-store
   check was emitted right after it (ast-8; only the emitter knows -
   same pattern as hb_compAstTag) */
void hb_compAstUseChk( HB_COMP_DECL, const char * szVarName )
{
   PHB_ASTDUMP pAst = HB_COMP_PARAM->pAst;

   if( pAst && pAst->nUseCount )
   {
      PHB_ASTUSE pUse = &pAst->pUses[ pAst->nUseCount - 1 ];

      if( pUse->szSym == szVarName && pUse->iLine == HB_COMP_PARAM->currLine )
         pUse->fChk = HB_TRUE;
   }
}

/* marks the LATEST declaration of <szVarName> in the current owner as
   covered by an emitted -kt PROLOGUE check (function and codeblock
   parameters; ast-8). Latest = the parameter list just materialized */
void hb_compAstDeclChk( HB_COMP_DECL, const char * szVarName )
{
   PHB_ASTDUMP pAst = HB_COMP_PARAM->pAst;

   if( pAst )
   {
      HB_BOOL fBlock;
      PHB_HFUNC pOwner = hb_compAstOwner( HB_COMP_PARAM, &fBlock );
      HB_SIZE n = pAst->nDeclCount;

      while( n-- )
      {
         PHB_ASTDECL pDecl = &pAst->pDecls[ n ];

         if( pDecl->pFunc == pOwner && pDecl->szSym == szVarName )
         {
            pDecl->fChk = HB_TRUE;
            break;
         }
      }
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
   pCall->nTok = hb_compAstSiteTok( pAst, szFunName );
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
   pSend->nTok = hb_compAstSiteTok( pAst, szMsgName );
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
         /* ast-11 (completude M-B): the block's OWN parameters as declared,
            attached to the block NODE. A Self-rooted send inside a block is
            typed by the SPECIFIC block it sits in, so two blocks sharing one
            source line (a VAR ... IS getter and its "_" setter) stop being
            ambiguous - the line-count guard the consumer used for block
            params is no longer the only anchor. Dump-only: read from the
            already-alive asCodeblock.pLocals, no pcode effect. The fact-only
            inline-Self sentinel maps to 'S' just like every other type. */
         {
            PHB_CBVAR pVar = pExpr->value.asCodeblock.pLocals;
            hb_compAstBufStr( pBuf, ",\"params\":[" );
            while( pVar )
            {
               HB_BYTE cType = pVar->bType;
               if( cType == HB_VARTYPE_INLINE_SELF )
                  cType = 'S';
               hb_compAstBufStr( pBuf,
                  pVar == pExpr->value.asCodeblock.pLocals ? "{\"sym\":" : ",{\"sym\":" );
               hb_compAstBufJsonStr( pBuf, pVar->szName, strlen( pVar->szName ) );
               if( cType != ' ' && cType != '\0' )
                  hb_compAstBufFmt( pBuf, ",\"type\":\"%c\"", cType );
               if( pVar->szFromClass )
               {
                  hb_compAstBufStr( pBuf, ",\"class\":" );
                  hb_compAstBufJsonStr( pBuf, pVar->szFromClass, strlen( pVar->szFromClass ) );
               }
               hb_compAstBufStr( pBuf, "}" );
               pVar = pVar->pNext;
            }
            hb_compAstBufStr( pBuf, "]" );
         }
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
   pStmt->fRet   = pAst->fRetPending && iKind == 'p';
   pAst->fRetPending = HB_FALSE;

   HB_SYMBOL_UNUSED( nTokMin );
   HB_SYMBOL_UNUSED( nTokMax );
}

/* arm the RETURN flag for the push that immediately follows: called from
   the RETURN Expression grammar action right before hb_compExprGenPush(),
   whose hb_compAstStatement() record consumes the flag - this is the only
   spot where the compiler still knows the push carries the RETURN value */
void hb_compAstReturn( HB_COMP_DECL )
{
   if( HB_COMP_PARAM->fAst )
      hb_compAstDump( HB_COMP_PARAM )->fRetPending = HB_TRUE;
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

/* declaration scope (HB_VSCOMP_* at declaration time) -> schema string;
   the strings match the ast-3 vocabulary, "public" is new in ast-4 */
static const char * hb_compAstDeclScope( int iScope )
{
   if( iScope & HB_VSCOMP_FIELD )
      return "field";
   if( iScope & HB_VSCOMP_STATIC )
      return "static";
   if( ( iScope & HB_VSCOMP_MEMVAR ) == HB_VSCOMP_MEMVAR )
      return "memvar";
   if( iScope & HB_VSCOMP_PRIVATE )
      return "private";
   if( iScope & HB_VSCOMP_PUBLIC )
      return "public";
   return "local";
}

/* declared type char + AS CLASS name; nothing emitted for untyped */
static void hb_compAstWriteType( FILE * file, HB_BYTE cType, const char * szClass )
{
   /* RD: the fact-only inline-Self class (HB_VARTYPE_INLINE_SELF) is a
      normal declared class to every consumer of the dump */
   if( cType == HB_VARTYPE_INLINE_SELF )
      cType = 'S';
   if( cType != ' ' && cType != '\0' )
      fprintf( file, ", \"type\": \"%c\"", cType );
   if( szClass )
   {
      fprintf( file, ", \"class\": " );
      hb_compAstWriteStr( file, szClass );
   }
}

/* one declared function/method signature (HB_HDECLARED). Parameter type
   chars may carry the BYREF (+60) / OPTIONAL (+90) offsets of the
   declaration subsystem - decoded best effort, the ranges overlap */
static void hb_compAstWriteDeclared( FILE * file, PHB_HDECLARED pDeclared,
                                     HB_BOOL * pfFirst )
{
   HB_USHORT i;

   if( ! *pfFirst )
      fprintf( file, "," );
   *pfFirst = HB_FALSE;
   fprintf( file, "\n      { \"name\": " );
   hb_compAstWriteStr( file, pDeclared->szName );
   /* pClass (and pParamClasses[ i ] below) are only WRITTEN by the
      subsystem when the type char is 'S'/'s' - garbage otherwise */
   hb_compAstWriteType( file, pDeclared->cType,
                        ( HB_TOUPPER( pDeclared->cType ) == 'S' &&
                          pDeclared->pClass ) ?
                        pDeclared->pClass->szName : NULL );
   fprintf( file, ", \"params\": [" );
   for( i = 0; i < pDeclared->iParamCount; ++i )
   {
      int iType = pDeclared->cParamTypes ? pDeclared->cParamTypes[ i ] : ' ';
      HB_BOOL fByRef = HB_FALSE, fOptional = HB_FALSE;

      if( iType > 122 + 60 )
      {
         fOptional = HB_TRUE;
         iType -= 90;
      }
      else if( iType > 122 )
      {
         fByRef = HB_TRUE;
         iType -= 60;
      }
      fprintf( file, "%s{", i ? ", " : " " );
      if( iType != ' ' && iType != '\0' )
         fprintf( file, " \"type\": \"%c\",", iType );
      if( ( iType == 'S' || iType == 's' ) &&
          pDeclared->pParamClasses && pDeclared->pParamClasses[ i ] )
      {
         fprintf( file, " \"class\": " );
         hb_compAstWriteStr( file, pDeclared->pParamClasses[ i ]->szName );
         fprintf( file, "," );
      }
      fprintf( file, " \"byref\": %s, \"optional\": %s }",
               fByRef ? "true" : "false", fOptional ? "true" : "false" );
   }
   fprintf( file, "%s] }", pDeclared->iParamCount ? " " : "" );
}

/* ast-18: the memvar names a STRING literal macro-references (macrotext,
   "&<name>").  When macrotext substitution is enabled (the default; -kM
   turns it off) a string literal that contains "&<name>" is re-expanded at
   RUN TIME to the value of that memvar - the string is DATA whose behaviour
   depends on the memvar.  A consumer renaming a memvar must know which
   strings follow it; without this the fact is buried in the HB_P_MACROTEXT
   pcode and the consumer would have to re-scan the string text itself.
   The "&" NAME EXTRACTION matches hb_compPushMacroText() byte for byte: "&"
   followed by [_A-Za-z] starts a name, [_A-Za-z0-9] continues it; "&(" and a
   trailing "&" are ignored.  Names are upcased (memvars are case-insensitive).
   Emits nothing when macrotext is off or no name is found.

   What this does NOT replicate: hb_compPushMacroText() then tests the name's
   scope (hb_compVariableScope) and only a MEMVAR/undeclared name becomes a
   runtime HB_P_MACROTEXT - a declared LOCAL under -kd is decomposed at compile
   time ("prefix" + var) instead.  This list is the LEXICAL &names only, so
   under -kd it can over-list a declared local.  In the default mode there is
   no divergence (&<local> is the compile error E0042).  The consumer treats
   the list as a REPORT candidate and never edits the string, so the cost of
   the over-list is one extra warning, never a wrong edit.  If a consumer ever
   needs the exact runtime-macrotext set, the fact must be recorded at the
   scope decision in hb_compPushMacroText, not re-derived here. */
static void hb_compAstWriteMacroVars( FILE * file, const char * szText,
                                      HB_SIZE nLen )
{
   HB_SIZE n = 0;
   HB_BOOL fFirst = HB_TRUE;

   while( n < nLen )
   {
      if( szText[ n++ ] == '&' )
      {
         char szName[ HB_SYMBOL_NAME_LEN + 1 ];
         int iSize = 0;

         while( n < nLen && iSize < HB_SYMBOL_NAME_LEN )
         {
            char ch = szText[ n ];
            if( ch >= 'a' && ch <= 'z' )
               szName[ iSize++ ] = ch - ( 'a' - 'A' );
            else if( ch == '_' || ( ch >= 'A' && ch <= 'Z' ) ||
                     ( iSize > 0 && ch >= '0' && ch <= '9' ) )
               szName[ iSize++ ] = ch;
            else
               break;
            ++n;
         }
         if( iSize )
         {
            szName[ iSize ] = '\0';
            fprintf( file, "%s", fFirst ? ", \"macrovars\": [ " : ", " );
            hb_compAstWriteStr( file, szName );
            fFirst = HB_FALSE;
         }
      }
   }
   if( ! fFirst )
      fprintf( file, " ]" );
}

/* one derivation fact inside a "from" list (ast-3): which match marker
   of which application the byte range [at, at+len) of the token text
   derives from.  ast-18: "app" is null when there is no application to
   point at (op "stream": the stream machinery, entered by a directive,
   fabricated the token from a raw source line); op "dynval" carries
   "axis" ("line"/"file") - the position axis the pp read the value from,
   recorded at the expansion branch itself */
static void hb_compAstWriteFromItem( FILE * file, HB_BOOL fFirst, int iApp,
                                     int iMarker, char cOp, HB_SIZE nAt,
                                     HB_SIZE nLen )
{
   fprintf( file, "%s{ ", fFirst ? "" : ", " );
   if( iApp >= 0 )
      fprintf( file, "\"app\": %d, ", iApp );
   else
      fprintf( file, "\"app\": null, " );
   fprintf( file, "\"marker\": %d, \"op\": \"%s\", ", iMarker,
            cOp == 'c' ? "clone" : cOp == 'p' ? "paste" :
            cOp == 'm' ? "stream" :
            ( cOp == 'd' || cOp == 'D' ) ? "dynval" : "stringify" );
   if( cOp == 'd' || cOp == 'D' )
      fprintf( file, "\"axis\": \"%s\", ", cOp == 'd' ? "line" : "file" );
   fprintf( file, "\"at\": %" HB_PFS "u, \"len\": %" HB_PFS "u }", nAt, nLen );
}

/* remember that the value written at marker <iMarker> of application <iApp>
   feeds a paste/stringify.  Pairs are collected first and indexed after,
   because the widest marker number is only known once the scan is over */
static void hb_compAstGenPairAdd( int ** ppPairs, HB_SIZE * pnPairs,
                                  HB_SIZE * pnAlloc, int iApp, int iMarker )
{
   if( *pnPairs == *pnAlloc )
   {
      *pnAlloc = *pnAlloc ? *pnAlloc << 1 : 64;
      *ppPairs = ( int * ) hb_xrealloc( *ppPairs, *pnAlloc * 2 * sizeof( int ) );
   }
   ( *ppPairs )[ *pnPairs * 2 ]     = iApp;
   ( *ppPairs )[ *pnPairs * 2 + 1 ] = iMarker;
   ++( *pnPairs );
}

/* ast-12: does the value the programmer wrote at a match marker of an
   application feed a PASTE or STRINGIFY derivation - i.e. does it GENERATE an
   artifact (a keyword concatenated from it, or a string dumped from it) that
   otherwise loses any connection to that name?  'c'lone (the value copied
   through as-is, e.g. an argument passed to the expanded call) is NOT
   generation.  The whole derivation graph already lives on the AST tokens
   (pFrom, ast-3), so this needs no pp API - it is a scan of what is already
   recorded.  A consumer that must rename/track the site uses this to tell
   "the name that GENERATES code" (rename it and its artifacts) from "a bound
   symbol that merely flows into a command" (rename it as the local/param it
   is).

   The answer is a property of the (application, marker) PAIR, so the whole
   set of generating pairs is built ONCE per module, here, and every token
   then answers by lookup.  It used to be a reverse scan run per consumed
   marker token, each run walking the entire token stream AND every
   application: O(markers x module), quadratic in module size.  Measured on
   the stock branch, a module of 16k expanded command lines needed over a
   minute to dump what takes a fraction of a second to compile; the same
   module now dumps in about a second.  Both sources below are the sources
   the old scan walked, so the answer is unchanged - only the number of times
   it is computed is. */
static HB_BYTE * hb_compAstGenSet( PHB_ASTDUMP pAst, PHB_PP_STATE pPP,
                                   int iAppCount, int * piWidth )
{
   int * piPairs = NULL;
   HB_BYTE * pSet = NULL;
   HB_SIZE nPairs = 0, nAlloc = 0, n;
   int iWidth = 1, iA, i;

   /* (1) the surviving token stream: a paste/stringify artifact that reached
      the compiler as-is (e.g. a generated FUNCTION name that is not consumed
      by an outer expansion) */
   for( n = 0; n < pAst->nTokenCount; ++n )
   {
      PHB_ASTTOKEN pTok = &pAst->pTokens[ n ];

      for( i = 0; i < pTok->iFromCount; ++i )
      {
         if( pTok->pFrom[ i ].iMarker >= 1 &&
             ( pTok->pFrom[ i ].cOp == 'p' || pTok->pFrom[ i ].cOp == 's' ) )
            hb_compAstGenPairAdd( &piPairs, &nPairs, &nAlloc,
                                  pTok->pFrom[ i ].iApp,
                                  pTok->pFrom[ i ].iMarker );
      }
   }

   /* (2) the CONSUMED tokens of pp applications keep the ORIGINAL synthesis
      op even when the token that survived into tokens[] was later re-cloned
      by an OUTER expansion.  `? EVENTO x` (EVENTO stringifies x, then the `?`
      command clones EVENTO's result): tokens[] shows the outer 'clone', but
      the `?` application's consumed token still records the inner 'stringify'
      that ties x to the generated string.  Without this source a marker whose
      artifact is re-consumed reads as non-generating (bug found on a pure
      #xtranslate stringify inside a command). */
   for( iA = 0; iA < iAppCount; ++iA )
   {
      int iRule, iLine, iTokens, iTok;

      hb_pp_trackApplyGet( pPP, iA, &iRule, &iLine, &iTokens );
      for( iTok = 0; iTok < iTokens; ++iTok )
      {
         int iFromCount = hb_pp_trackApplyTokenFromCount( pPP, iA, iTok );
         int iFrom;

         for( iFrom = 0; iFrom < iFromCount; ++iFrom )
         {
            int iFApp, iFMk;
            char cOp;
            HB_SIZE nAt, nFLen;

            hb_pp_trackApplyTokenFromGet( pPP, iA, iTok, iFrom, &iFApp, &iFMk,
                                          &cOp, &nAt, &nFLen );
            if( iFMk >= 1 && ( cOp == 'p' || cOp == 's' ) )
               hb_compAstGenPairAdd( &piPairs, &nPairs, &nAlloc, iFApp, iFMk );
         }
      }
   }

   for( n = 0; n < nPairs; ++n )
   {
      if( piPairs[ n * 2 + 1 ] >= iWidth )
         iWidth = piPairs[ n * 2 + 1 ] + 1;
   }

   if( nPairs > 0 && iAppCount > 0 )
   {
      pSet = ( HB_BYTE * ) hb_xgrabz( ( HB_SIZE ) iAppCount * iWidth );
      for( n = 0; n < nPairs; ++n )
      {
         int iApp = piPairs[ n * 2 ], iMk = piPairs[ n * 2 + 1 ];

         if( iApp >= 0 && iApp < iAppCount && iMk >= 1 && iMk < iWidth )
            pSet[ ( HB_SIZE ) iApp * iWidth + iMk ] = 1;
      }
   }

   if( piPairs )
      hb_xfree( piPairs );

   *piWidth = iWidth;
   return pSet;
}

/* marker kind vocabulary of the PP pattern parse (hbpp.h): the match
   side and the result side use disjoint constant ranges */
static const char * hb_compAstRuleMkind( int iType )
{
   switch( iType )
   {
      case HB_PP_MMARKER_REGULAR:   return "regular";
      case HB_PP_MMARKER_LIST:      return "list";
      case HB_PP_MMARKER_RESTRICT:  return "restrict";
      case HB_PP_MMARKER_WILD:      return "wild";
      case HB_PP_MMARKER_EXTEXP:    return "extexp";
      case HB_PP_MMARKER_NAME:      return "name";
      case HB_PP_RMARKER_REGULAR:   return "regular";
      case HB_PP_RMARKER_STRDUMP:   return "strdump";
      case HB_PP_RMARKER_STRSTD:    return "strstd";
      case HB_PP_RMARKER_STRSMART:  return "strsmart";
      case HB_PP_RMARKER_BLOCK:     return "block";
      case HB_PP_RMARKER_LOGICAL:   return "logical";
      case HB_PP_RMARKER_NUL:       return "nul";
      case HB_PP_RMARKER_DYNVAL:    return "dynval";
      case HB_PP_RMARKER_REFERENCE: return "reference";
   }
   return "unknown";
}

/* one side (match or result) of a tracked rule, seen from inside: the
   token roles the PP assigned parsing the directive, in the rule's
   STORED order (consecutive keyword-less optional groups are reordered
   by the PP at registration - source order is recoverable through the
   positions).  Unlike tokens[], the column IS emitted for include-file
   tokens: the rule record names the directive's file, and rules live in
   .ch files - a position outside that file (a rule defined inside
   another rule's expansion) simply fails the consumer's byte-exact check
   against it.  A rule GENERATED by another rule's expansion additionally
   carries "from" on its pattern tokens (ast-13, rule genealogy): the
   application/marker that created it, positively identifiable instead
   of only failing the position check */
static void hb_compAstWriteRuleToks( FILE * file, PHB_PP_STATE pPP,
                                     int iRule, HB_BOOL fResult )
{
   int iCount = hb_pp_trackRuleTokenCount( pPP, iRule, fResult );
   int i;

   fprintf( file, "[" );
   for( i = 0; i < iCount; ++i )
   {
      const char * szText;
      HB_SIZE nLen;
      int iType, iMarker, iLine, iCol;
      char cRole;
      HB_BOOL fMain;

      hb_pp_trackRuleToken( pPP, iRule, fResult, i, &szText, &nLen, &iType,
                            &iMarker, &cRole, &iLine, &iCol, &fMain );
      fprintf( file, "%s\n        ", i ? "," : "" );
      if( cRole == '[' || cRole == ']' )
      {
         fprintf( file, "{ \"role\": \"opt-%s\" }",
                  cRole == '[' ? "open" : "close" );
         continue;
      }
      fprintf( file, "{ \"role\": \"%s\", ",
               cRole == 'm' ? "marker" :
               cRole == 'r' ? "restrict" : "literal" );
      if( cRole == 'm' )
         fprintf( file, "\"marker\": %d, \"mkind\": \"%s\", ", iMarker,
                  hb_compAstRuleMkind( iType ) );
      else
      {
         if( cRole == 'r' )
            fprintf( file, "\"marker\": %d, ", iMarker );
         fprintf( file, "\"type\": %d, ", iType );
      }
      if( iLine > 0 )
         fprintf( file, "\"line\": %d, ", iLine );
      else
         fprintf( file, "\"line\": null, " );
      if( iCol >= 0 )
         fprintf( file, "\"col\": %d, ", iCol );
      else
         fprintf( file, "\"col\": null, " );
      fprintf( file, "\"len\": %" HB_PFS "u, \"prov\": \"%c\", \"text\": ",
               nLen, iLine > 0 ? ( fMain ? 's' : 'i' ) : 'n' );
      hb_compAstWriteStr( file, szText );
      /* ast-13 (rule genealogy): a rule GENERATED by another rule's
         expansion carries "from" on its pattern tokens - which
         application/marker each byte range derives from, i.e. which
         application CREATED the rule.  Same item format as ast-3;
         absent on rules written directly in source/include files */
      {
         int iFromCount = hb_pp_trackRuleTokenFromCount( pPP, iRule,
                                                         fResult, i );
         if( iFromCount > 0 )
         {
            int iFrom;

            fprintf( file, ", \"from\": [ " );
            for( iFrom = 0; iFrom < iFromCount; ++iFrom )
            {
               int iApp, iMarker;
               char cOp;
               HB_SIZE nAt, nFromLen;

               hb_pp_trackRuleTokenFromGet( pPP, iRule, fResult, i, iFrom,
                                            &iApp, &iMarker, &cOp,
                                            &nAt, &nFromLen );
               hb_compAstWriteFromItem( file, iFrom == 0, iApp, iMarker,
                                        cOp, nAt, nFromLen );
            }
            fprintf( file, " ]" );
         }
      }
      fprintf( file, " }" );
   }
   fprintf( file, "%s]", iCount ? "\n      " : "" );
}

/* Content checksum of one source file, FNV-1a 64-bit, as lowercase hex.
   Returns HB_FALSE when the file cannot be read (the caller then omits the
   file's checksum rather than writing a wrong one).

   Why a local checksum instead of hb_md5file(): md5 lives in libhbrtl, and
   linking it here would drag the whole runtime (hb_fileExtOpen, hb_fileRead,
   hb_parc, hb_retclen, ...) into the compiler, which is deliberately lean.
   This is not cryptography - the adversary is "the file changed", not a
   forger - so a fast non-cryptographic content hash is the right tool. */
/* FNV-1a 64: the digest of this dump - the provenance file sums and the
   per-function pcode hashes below all use it, so a consumer carries one
   comparison rule.  Every hash is paired with the exact byte count of the
   hashed stream: a collision would additionally need equal length */
#define HB_AST_FNV_INIT       HB_ULL( 14695981039346656037 )

static HB_U64 hb_compAstFnv( HB_U64 nHash, const void * pData, HB_SIZE nLen )
{
   const unsigned char * pBytes = ( const unsigned char * ) pData;
   HB_SIZE n;

   for( n = 0; n < nLen; ++n )
   {
      nHash ^= ( HB_U64 ) pBytes[ n ];
      nHash *= HB_ULL( 1099511628211 );
   }
   return nHash;
}

static HB_BOOL hb_compAstFileSum( const char * pszFileName, char * szOut,
                                  HB_FOFFSET * pnSize )
{
   FILE *        file;
   HB_U64        nHash = HB_AST_FNV_INIT;
   unsigned char buffer[ 4096 ];
   size_t        nRead;
   HB_FOFFSET    nSize = 0;

   file = hb_fopen( pszFileName, "rb" );
   if( ! file )
      return HB_FALSE;

   while( ( nRead = fread( buffer, 1, sizeof( buffer ), file ) ) > 0 )
   {
      nHash = hb_compAstFnv( nHash, buffer, ( HB_SIZE ) nRead );
      nSize += ( HB_FOFFSET ) nRead;
   }
   fclose( file );

   hb_snprintf( szOut, 17, "%016" PFHL "x", nHash );
   *pnSize = nSize;

   return HB_TRUE;
}

/* ast-24: per-function pcode identity, so a consumer can prove "this
   refactoring left every function's code alone" from the dump instead of
   parsing the .hrb container - a private reader of a foreign binary format
   drifts, the dump's schema is exact and refuses loudly when it moves.

   Two hashes per function, both over pFunc->pCode (the same bytes the
   .hrb/.c generators serialize, still alive here because hb_compAstSave()
   runs after hb_compGenOutput() and before hb_compCompileEnd()):

   - "pcodeHash": the raw bytes.  Equality means byte-identical code.
   - "pcodeNormHash": the bytes with every symbol-table INDEX operand
     replaced by the symbol's NAME (and the near/wide opcode pairs folded
     into the wide one, since the index width is an encoding accident).
     pcode addresses memvars, fields, messages and function symbols by
     position in the module's symbol table, so ADDING one symbol renumbers
     the table and changes the raw pcode of every function that references
     one behind it - without changing what any of them does.  This hash is
     the fact that survives that renumbering.

   The opcode list mirrors genc.c, the in-tree authority on which operands
   are symbol-table indexes (each hb_compSymbolName() site in its verbose
   comments); the operand is always the first one.  HB_P_WITHOBJECTMESSAGE
   uses 0xFFFF as "no symbol - already pushed via HB_P_MACROSYMBOL", kept
   as raw bytes below, exactly as the VM special-cases it. */
static int hb_compAstSymOperand( HB_BYTE opcode, HB_BYTE * pCanon )
{
   *pCanon = opcode;
   switch( opcode )
   {
      case HB_P_PUSHSYMNEAR:
         *pCanon = HB_P_PUSHSYM;
         return 1;
      case HB_P_PUSHALIASEDFIELDNEAR:
         *pCanon = HB_P_PUSHALIASEDFIELD;
         return 1;
      case HB_P_POPALIASEDFIELDNEAR:
         *pCanon = HB_P_POPALIASEDFIELD;
         return 1;
      case HB_P_MESSAGE:
      case HB_P_PARAMETER:
      case HB_P_POPALIASEDFIELD:
      case HB_P_POPALIASEDVAR:
      case HB_P_POPFIELD:
      case HB_P_POPMEMVAR:
      case HB_P_POPVARIABLE:
      case HB_P_PUSHALIASEDFIELD:
      case HB_P_PUSHALIASEDVAR:
      case HB_P_PUSHFIELD:
      case HB_P_PUSHFUNCSYM:
      case HB_P_PUSHMEMVAR:
      case HB_P_PUSHMEMVARREF:
      case HB_P_PUSHSYM:
      case HB_P_PUSHVARIABLE:
      case HB_P_WITHOBJECTMESSAGE:
         return 2;
   }
   return 0;
}

static void hb_compAstPcodeHashes( HB_COMP_DECL, PHB_HFUNC pFunc,
                                   char * szRaw, char * szNorm )
{
   HB_U64  nRaw  = hb_compAstFnv( HB_AST_FNV_INIT, pFunc->pCode,
                                  pFunc->nPCodePos );
   HB_U64  nNorm = HB_AST_FNV_INIT;
   HB_SIZE nPos  = 0;

   while( nPos < pFunc->nPCodePos )
   {
      HB_BYTE      opcode = pFunc->pCode[ nPos ];
      HB_ISIZ      nSize  = hb_compPCodeSize( pFunc, nPos );
      HB_BYTE      cCanon;
      int          iWidth = hb_compAstSymOperand( opcode, &cCanon );
      const char * szName = NULL;

      if( nSize <= 0 || nPos + ( HB_SIZE ) nSize > pFunc->nPCodePos )
      {
         /* unknown or truncated opcode: no way to keep walking - hash the
            rest raw so the value still commits to every byte */
         nNorm = hb_compAstFnv( nNorm, pFunc->pCode + nPos,
                                pFunc->nPCodePos - nPos );
         break;
      }
      if( iWidth > 0 )
      {
         HB_USHORT uiSym = iWidth == 1 ? pFunc->pCode[ nPos + 1 ] :
                           HB_PCODE_MKUSHORT( &pFunc->pCode[ nPos + 1 ] );

         if( ! ( opcode == HB_P_WITHOBJECTMESSAGE && uiSym == 0xFFFF ) )
            szName = hb_compSymbolName( HB_COMP_PARAM, uiSym );
      }
      if( szName )
      {
         nNorm = hb_compAstFnv( nNorm, &cCanon, 1 );
         nNorm = hb_compAstFnv( nNorm, szName, strlen( szName ) + 1 );
         nNorm = hb_compAstFnv( nNorm, pFunc->pCode + nPos + 1 + iWidth,
                                ( HB_SIZE ) nSize - 1 - iWidth );
      }
      else
         nNorm = hb_compAstFnv( nNorm, pFunc->pCode + nPos,
                                ( HB_SIZE ) nSize );
      nPos += ( HB_SIZE ) nSize;
   }

   hb_snprintf( szRaw, 17, "%016" PFHL "x", nRaw );
   hb_snprintf( szNorm, 17, "%016" PFHL "x", nNorm );
}

/* ast-22: PROVENANCE - what this dump was made FROM.
   The consumer's question is "does this dump still correspond to the sources?",
   and until now the only available evidence was the file timestamp. That
   evidence lies in two measured ways: an edit within the same second as the
   compile is invisible (the incremental build compares with ~1s resolution),
   and a deleted dump leaves no trace at all. Both are decided here instead:
   the artifact carries the identity of every file it was built from, so the
   answer is a comparison of facts, never an inference from the clock.

   The file list is the compiler's own (HB_COMP_PARAM->incfiles): it already
   resolves transitive includes and honours conditional compilation, which is
   exactly what "-gd" reports. The defines are listed because the same bytes
   compiled with a different -D legitimately produce a different dump. */
static void hb_compAstWriteProvenance( HB_COMP_DECL, FILE * file )
{
   PHB_INCLST   pIncFile = HB_COMP_PARAM->incfiles;
   PHB_PPDEFINE pDefine  = HB_COMP_PARAM->ppdefines;
   HB_BOOL      fFirst   = HB_TRUE;

   fprintf( file, ",\n  \"provenance\": { \"sum\": \"fnv1a64\", \"files\": [" );

   while( pIncFile )
   {
      char       szSum[ 17 ];
      HB_FOFFSET nSize = 0;

      fprintf( file, "%s\n    { \"path\": ", fFirst ? "" : "," );
      hb_compAstWriteStr( file, pIncFile->szFileName );
      if( hb_compAstFileSum( pIncFile->szFileName, szSum, &nSize ) )
         fprintf( file, ", \"size\": %" PFHL "d, \"sum\": \"%s\" }", nSize, szSum );
      else
         fprintf( file, ", \"unreadable\": true }" );

      fFirst   = HB_FALSE;
      pIncFile = pIncFile->pNext;
   }

   fprintf( file, "%s ]", fFirst ? "" : "\n   " );

   fprintf( file, ", \"defines\": [" );
   fFirst = HB_TRUE;
   while( pDefine )
   {
      fprintf( file, "%s\n    { \"name\": ", fFirst ? "" : "," );
      hb_compAstWriteStr( file, pDefine->szName );
      if( pDefine->szValue )
      {
         fprintf( file, ", \"value\": " );
         hb_compAstWriteStr( file, pDefine->szValue );
      }
      fprintf( file, " }" );
      fFirst  = HB_FALSE;
      pDefine = pDefine->pNext;
   }
   fprintf( file, "%s ] }", fFirst ? "" : "\n   " );
}

/* --filesum: the content hash of each listed file, one per line, and nothing
   else. Format is `<sum> <size> <path>`, in the order given - the same shape
   `md5sum` and friends use, so it reads the same way by eye and by script.
   A file that cannot be read prints `- - <path>`: the caller must be able to
   tell "changed" from "could not look", and a missing line would blur them.

   This exists so that whoever holds a dump can ask whether it still matches
   the sources WITHOUT compiling: the expensive thing is the compile, and
   compiling to find out whether we needed to compile saves nothing. The hash
   is computed by the same code that wrote the provenance into the dump, which
   is the whole point - a second implementation elsewhere would drift, and the
   day it drifted every cached dump would silently look stale (or, worse,
   fresh). */
/* --- reading back the provenance -----------------------------------------
   The compiler reads a block it WROTE itself, in a shape it controls, sitting
   in the first bytes of the file (before "hasCDump"). That is why this is a
   linear scan and not a JSON parser: a general parser would be a large thing
   to carry for one known block, and this one lives next to the writer above,
   so the two move together.
   ------------------------------------------------------------------------- */

/* copies the JSON string starting at *pSrc (which points at the opening quote)
   into szOut, undoing the three escapes the writer emits. Returns the position
   after the closing quote, or NULL when the string is not well formed. */
static const char * hb_compAstReadStr( const char * pSrc, char * szOut, int iMax )
{
   int i = 0;

   if( *pSrc != '"' )
      return NULL;
   ++pSrc;

   while( *pSrc && *pSrc != '"' )
   {
      char c = *pSrc++;

      if( c == '\\' )
      {
         if( *pSrc == 'u' )
         {
            int  j;
            long lVal = 0;

            ++pSrc;
            for( j = 0; j < 4 && HB_ISXDIGIT( ( HB_UCHAR ) *pSrc ); ++j )
            {
               char h = *pSrc++;
               lVal = lVal * 16 + ( HB_ISDIGIT( ( HB_UCHAR ) h ) ? h - '0' :
                                    ( HB_TOUPPER( h ) - 'A' + 10 ) );
            }
            c = ( char ) lVal;
         }
         else
            c = *pSrc++;
      }
      if( i < iMax - 1 )
         szOut[ i++ ] = c;
   }
   if( *pSrc != '"' )
      return NULL;

   szOut[ i ] = '\0';

   return pSrc + 1;
}

/* Does the dump still match the sources it was made from?
   szWhy receives the FIRST divergence found, in the caller's words - "which
   file and what about it", never a bare false: a consumer that is told only
   "stale" has to guess whether to recompile one module or distrust the lot. */
static HB_BOOL hb_compAstProvFresh( const char * pszDump, char * szWhy, int iWhyLen )
{
   FILE *       file;
   char *       pBuf;
   const char * p;
   HB_SIZE      nRead;
   HB_BOOL      fFresh = HB_TRUE;

   file = hb_fopen( pszDump, "rb" );
   if( ! file )
   {
      hb_snprintf( szWhy, iWhyLen, "dump not found" );
      return HB_FALSE;
   }

   /* the block is at the top of the file, before the token stream; reading a
      fixed head is enough and keeps a multi-megabyte dump from being paged in
      to answer a question about its first lines */
   pBuf = ( char * ) hb_xgrab( HB_AST_PROV_HEAD + 1 );
   nRead = ( HB_SIZE ) fread( pBuf, 1, HB_AST_PROV_HEAD, file );
   pBuf[ nRead ] = '\0';
   fclose( file );

   p = strstr( pBuf, "\"provenance\"" );
   if( ! p || ! ( p = strstr( p, "\"files\"" ) ) )
   {
      hb_snprintf( szWhy, iWhyLen, "no provenance in the dump (older schema?)" );
      hb_xfree( pBuf );
      return HB_FALSE;
   }

   while( fFresh && ( p = strstr( p, "{ \"path\": " ) ) != NULL )
   {
      char       szPath[ HB_PATH_MAX ];
      char       szSum[ 32 ];
      char       szNow[ 17 ];
      HB_FOFFSET nSize = 0, nNow = 0;

      p += 10;
      p = hb_compAstReadStr( p, szPath, sizeof( szPath ) );
      if( ! p )
         break;

      /* a file the compiler could not read when it wrote the dump cannot be
         confirmed now either - saying "fresh" here would be inventing a fact */
      if( strstr( p, "\"unreadable\"" ) != NULL &&
          strstr( p, "\"unreadable\"" ) < strstr( p, "}" ) )
      {
         hb_snprintf( szWhy, iWhyLen, "%s: was unreadable when the dump was written", szPath );
         fFresh = HB_FALSE;
         break;
      }

      p = strstr( p, "\"size\": " );
      if( ! p )
         break;
      nSize = ( HB_FOFFSET ) strtol( p + 8, NULL, 10 );

      p = strstr( p, "\"sum\": " );
      if( ! p )
         break;
      p = hb_compAstReadStr( p + 7, szSum, sizeof( szSum ) );
      if( ! p )
         break;

      if( ! hb_compAstFileSum( szPath, szNow, &nNow ) )
      {
         hb_snprintf( szWhy, iWhyLen, "%s: gone", szPath );
         fFresh = HB_FALSE;
      }
      else if( nNow != nSize || strcmp( szNow, szSum ) != 0 )
      {
         hb_snprintf( szWhy, iWhyLen, "%s: changed", szPath );
         fFresh = HB_FALSE;
      }
   }

   hb_xfree( pBuf );

   return fFresh;
}

/* --ast-fresh: prints ONLY the dumps that no longer match, one per line, as
   `<dump><TAB><why>`. Silence means every dump given still corresponds to its
   sources, and the exit is non-zero when any line was printed.

   Printing only the stale ones is not brevity: a caller that has to tell them
   apart by a `fresh`/`stale` prefix is deciding a ROLE by comparing text, and
   text is the one thing this project refuses to decide by. Here the presence
   of the line IS the fact, and its first field is the dump - nothing to
   classify. */
void hb_compAstFreshPrint( HB_COMP_DECL, int argc, const char * const argv[] )
{
   int i;

   for( i = 1; i < argc; ++i )
   {
      char szWhy[ 256 ];
      char szLine[ HB_PATH_MAX + 320 ];

      if( HB_ISOPTSEP( argv[ i ][ 0 ] ) )
         continue;

      szWhy[ 0 ] = '\0';
      if( ! hb_compAstProvFresh( argv[ i ], szWhy, sizeof( szWhy ) ) )
      {
         hb_snprintf( szLine, sizeof( szLine ), "%s\t%s\n", argv[ i ], szWhy );
         HB_COMP_PARAM->fAstStale = HB_TRUE;
         hb_compOutStd( HB_COMP_PARAM, szLine );
      }
   }
}

void hb_compAstSumsPrint( HB_COMP_DECL, int argc, const char * const argv[] )
{
   int i;

   for( i = 1; i < argc; ++i )
   {
      char       szSum[ 17 ];
      char       szLine[ HB_PATH_MAX + 64 ];
      HB_FOFFSET nSize = 0;

      if( HB_ISOPTSEP( argv[ i ][ 0 ] ) )
         continue;

      if( hb_compAstFileSum( argv[ i ], szSum, &nSize ) )
         hb_snprintf( szLine, sizeof( szLine ), "%s %" PFHL "d %s\n",
                      szSum, nSize, argv[ i ] );
      else
         hb_snprintf( szLine, sizeof( szLine ), "- - %s\n", argv[ i ] );

      hb_compOutStd( HB_COMP_PARAM, szLine );
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

   hb_compAstWriteProvenance( HB_COMP_PARAM, file );

   fprintf( file, ",\n  \"hasCDump\": %s,",
            HB_COMP_PARAM->inlines.iCount > 0 ? "true" : "false" );

   /* ast-7: was the module compiled with -kt (runtime checks for the
      declared type annotations)? an annotated symbol in a -kt module is
      an imposed invariant - the consumer's "guaranteed" layer keys on it */
   fprintf( file, "\n  \"kt\": %s,",
            HB_SUPPORT_CHKTYPE ? "true" : "false" );

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
      if( pTok->iApp >= 0 )
         fprintf( file, ", \"app\": %d", pTok->iApp );
      if( pTok->iFromCount > 0 )
      {
         int i;

         fprintf( file, ", \"from\": [ " );
         for( i = 0; i < pTok->iFromCount; ++i )
            hb_compAstWriteFromItem( file, i == 0, pTok->pFrom[ i ].iApp,
                                     pTok->pFrom[ i ].iMarker,
                                     pTok->pFrom[ i ].cOp,
                                     pTok->pFrom[ i ].nAt,
                                     pTok->pFrom[ i ].nLen );
         fprintf( file, " ]" );
      }
      /* ast-18: a string literal that macro-references a memvar ("&<name>")
         re-expands at RUN TIME - list those names so a consumer renaming a
         memvar knows which data strings follow it.  Only when macrotext is
         enabled (-kM off), matching the compiler's own pcode decision */
      if( HB_PP_TOKEN_TYPE( pTok->type ) == HB_PP_TOKEN_STRING &&
          HB_SUPPORT_MACROTEXT )
         hb_compAstWriteMacroVars( file, pTok->szText, pTok->nLen );
      fprintf( file, " }" );
   }
   fprintf( file, "%s ],", pAst->nTokenCount ? "\n  " : "" );

   /* preprocessor rules and their applications (tracked by the pp, see
      hb_pp_trackPos()): the consumed tokens of an application are the
      words of the source line a rule rewrote - the DSL words among them
      never reach the parser, so they exist only here */
   {
      PHB_PP_STATE pPP = HB_COMP_PARAM->pLex ? HB_COMP_PARAM->pLex->pPP : NULL;
      HB_BYTE * pGenSet;
      int iCount, i, iGenWidth = 1;

      fprintf( file, "\n  \"ppRules\": [" );
      iCount = pPP ? hb_pp_trackRuleCount( pPP ) : 0;
      for( i = 0; i < iCount; ++i )
      {
         const char * szFile, * szHead;
         int iType, iLine, iMarkers, iMode, iDelOf;
         HB_BOOL fDel, fRemoved;

         hb_pp_trackRuleGet( pPP, i, &iType, &iMode, &szFile, &iLine,
                             &szHead, &iMarkers, &fDel, &iDelOf, &fRemoved );
         /* ast-16: the kind now carries the FAMILY as the pp really sees it -
            the comparison mode ("" = dBase/abbreviable, "x" = exact, "y" =
            exact and case sensitive) - and whether the directive REMOVES a
            rule instead of creating one ("un"). */
         /* #define -> "define" / "undef" (the directive really is spelled
            #undef); the rule families carry the comparison mode as a prefix */
         if( iType == 'd' )
            fprintf( file, "%s\n    { \"id\": %d, \"kind\": \"%s\", \"file\": ",
                     i ? "," : "", i, fDel ? "undef" : "define" );
         else
            fprintf( file, "%s\n    { \"id\": %d, \"kind\": \"%s%s%s\", \"file\": ",
                     i ? "," : "", i,
                     iMode == HB_PP_CMP_STD ? "x" :
                     iMode == HB_PP_CMP_CASE ? "y" : "",
                     fDel ? "un" : "",
                     iType == 't' ? "translate" : "command" );
         if( szFile )
            hb_compAstWriteStr( file, szFile );
         else
            fprintf( file, "null" );
         fprintf( file, ", \"line\": %d, \"head\": ", iLine );
         if( szHead )
            hb_compAstWriteStr( file, szHead );
         else
            fprintf( file, "null" );
         /* ast-16: the rule's LIFETIME. On an #un... record, "undoes" is the id
            of the rule it removed, or null when it removed NOTHING (an orphan
            removing directive - silent dead code). On a created rule, "removed"
            says an #un... later took it out of the rule table. */
         if( fDel )
         {
            fprintf( file, ", \"undoes\": " );
            if( iDelOf >= 0 )
               fprintf( file, "%d", iDelOf );
            else
               fprintf( file, "null" );
         }
         else if( fRemoved )
            fprintf( file, ", \"removed\": true" );
         fprintf( file, ", \"markers\": %d,\n      \"match\": ", iMarkers );
         hb_compAstWriteRuleToks( file, pPP, i, HB_FALSE );
         fprintf( file, ",\n      \"result\": " );
         hb_compAstWriteRuleToks( file, pPP, i, HB_TRUE );
         fprintf( file, " }" );
      }
      fprintf( file, "%s ],", iCount ? "\n  " : "" );

      /* ast-19: what conditional compilation SKIPPED.  Every other channel of
         this dump describes the program that WAS built; this one describes the
         part of the file that was NOT, and it exists so a consumer can tell how
         far its own verification reached.  A rename that edits the branch that
         compiled, verifies that branch and reports success is telling the truth
         about a scope it never states - while the other configuration is left
         calling a name that no longer exists.
         REPORT ONLY - nothing here may be edited: this text never became a
         symbol, so a word in it that spells like one is not known to BE one.
         Proving that would mean compiling the other branch, i.e. another
         program. */
      fprintf( file, "\n  \"ppSkipped\": [" );
      iCount = pPP ? hb_pp_trackSkipCount( pPP ) : 0;
      for( i = 0; i < iCount; ++i )
      {
         const char * szFile, * szCond;
         int iFrom, iTo, iToks, j;

         hb_pp_trackSkipGet( pPP, i, &szFile, &szCond, &iFrom, &iTo, &iToks );
         fprintf( file, "%s\n    { \"file\": ", i ? "," : "" );
         if( szFile )
            hb_compAstWriteStr( file, szFile );
         else
            fprintf( file, "null" );
         fprintf( file, ", \"from\": %d, \"to\": %d, \"cond\": ", iFrom, iTo );
         if( szCond )
            hb_compAstWriteStr( file, szCond );
         else
            fprintf( file, "null" );
         fprintf( file, ",\n      \"tokens\": [" );
         for( j = 0; j < iToks; ++j )
         {
            const char * szText = NULL;
            int iLine = 0, iCol = -1;

            hb_pp_trackSkipToken( pPP, i, j, &szText, &iLine, &iCol );
            fprintf( file, "%s { \"text\": ", j ? "," : "" );
            hb_compAstWriteStr( file, szText ? szText : "" );
            fprintf( file, ", \"line\": %d, \"col\": ", iLine );
            if( iCol >= 0 )
               fprintf( file, "%d", iCol );
            else
               fprintf( file, "null" );
            fprintf( file, " }" );
         }
         fprintf( file, "%s] }", iToks ? " " : "" );
      }
      fprintf( file, "%s ],", iCount ? "\n  " : "" );

      fprintf( file, "\n  \"ppApplications\": [" );
      iCount = pPP ? hb_pp_trackApplyCount( pPP ) : 0;
      /* ast-12: the generating (application, marker) pairs, computed once */
      pGenSet = pPP ? hb_compAstGenSet( pAst, pPP, iCount, &iGenWidth ) : NULL;
      for( i = 0; i < iCount; ++i )
      {
         int iRule, iLine, iTokens, iTok;

         hb_pp_trackApplyGet( pPP, i, &iRule, &iLine, &iTokens );
         fprintf( file, "%s\n    { \"rule\": %d, \"line\": %d, \"tokens\": [",
                  i ? "," : "", iRule, iLine );
         for( iTok = 0; iTok < iTokens; ++iTok )
         {
            const char * szText;
            HB_SIZE nLen;
            int iType, iMarker, iTokLine, iCol, iRuleTok;
            HB_BOOL fMain;
            char cProv;

            hb_pp_trackApplyToken( pPP, i, iTok, &szText, &nLen, &iType,
                                   &iMarker, &iTokLine, &iCol, &fMain,
                                   &iRuleTok );
            /* same provenance/column rules as tokens[]: a column in
               another physical file is not a column here */
            cProv = fMain ? ( iCol >= 0 ? 's' : 'n' ) : 'i';
            if( ! fMain )
               iCol = -1;
            fprintf( file, "%s\n      { \"line\": %d, ",
                     iTok ? "," : "", iTokLine );
            if( iCol >= 0 )
               fprintf( file, "\"col\": %d, ", iCol );
            else
               fprintf( file, "\"col\": null, " );
            fprintf( file, "\"len\": %" HB_PFS "u, \"type\": %d, \"prov\": \"%c\", \"marker\": %d, ",
                     nLen, iType, cProv, iMarker );
            /* ast-15: WHICH literal of the rule this token matched (index into
               the rule's match[]).  Only for marker == 0; the pp knows it while
               matching and used to drop it, leaving a consumer to guess the
               literal from the text - a guess that breaks when one keyword of a
               rule is a dBase abbreviation prefix of another keyword of it. */
            if( iRuleTok >= 0 )
               fprintf( file, "\"ruletok\": %d, ", iRuleTok );
            fprintf( file, "\"text\": " );
            hb_compAstWriteStr( file, szText );
            {
               int iFromCount = hb_pp_trackApplyTokenFromCount( pPP, i, iTok );

               if( iFromCount > 0 )
               {
                  int iFrom, iApp, iMk;
                  char cOp;
                  HB_SIZE nAt, nFromLen;

                  fprintf( file, ", \"from\": [ " );
                  for( iFrom = 0; iFrom < iFromCount; ++iFrom )
                  {
                     hb_pp_trackApplyTokenFromGet( pPP, i, iTok, iFrom,
                                                   &iApp, &iMk, &cOp,
                                                   &nAt, &nFromLen );
                     hb_compAstWriteFromItem( file, iFrom == 0, iApp, iMk,
                                              cOp, nAt, nFromLen );
                  }
                  fprintf( file, " ]" );
               }
            }
            /* ast-12: mark the source-marker fill whose written name FEEDS a
               paste/stringify (it GENERATES an artifact - the rename target
               is the marker and its derivatives, not a homonym local the
               expansion may also fabricate). Absent = does not generate. */
            if( iMarker >= 1 && pGenSet && iMarker < iGenWidth &&
                pGenSet[ ( HB_SIZE ) i * iGenWidth + iMarker ] )
               fprintf( file, ", \"generates\": true" );
            fprintf( file, " }" );
         }
         fprintf( file, "%s ] }", iTokens ? "\n    " : "" );
      }
      fprintf( file, "%s ],", iCount ? "\n  " : "" );

      if( pGenSet )
         hb_xfree( pGenSet );
   }

   /* the DECLARE subsystem tables (language-level type declarations:
      DECLARE CLASS/_HB_CLASS, DECLARE_MEMBER/_HB_MEMBER, DECLARE fun()),
      still alive here - hb_compDeclaredReset() only runs when the NEXT
      module starts; kept populated at any -w level by the fAst gates */
   fprintf( file, "\n  \"declared\": { \"classes\": [" );
   {
      PHB_HCLASS pClass = HB_COMP_PARAM->pFirstClass;
      HB_BOOL fFirstDecl = HB_TRUE;

      while( pClass )
      {
         PHB_HDECLARED pMethod = pClass->pMethod;
         HB_BOOL fFirstMth = HB_TRUE;

         if( ! fFirstDecl )
            fprintf( file, "," );
         fFirstDecl = HB_FALSE;
         fprintf( file, "\n    { \"name\": " );
         hb_compAstWriteStr( file, pClass->szName );
         fprintf( file, ", \"methods\": [" );
         while( pMethod )
         {
            hb_compAstWriteDeclared( file, pMethod, &fFirstMth );
            pMethod = pMethod->pNext;
         }
         fprintf( file, "%s ] }", fFirstMth ? "" : "\n    " );
         pClass = pClass->pNext;
      }
      fprintf( file, "%s ],\n    \"functions\": [", fFirstDecl ? "" : "\n  " );

      fFirstDecl = HB_TRUE;
      {
         PHB_HDECLARED pDeclared = HB_COMP_PARAM->pFirstDeclared;

         while( pDeclared )
         {
            hb_compAstWriteDeclared( file, pDeclared, &fFirstDecl );
            pDeclared = pDeclared->pNext;
         }
      }
      fprintf( file, "%s ] },", fFirstDecl ? "" : "\n    " );
   }

   /* ast-24: the module's symbol table, in table order - the same list the
      .hrb generator serializes (genhrb.c), so index N here IS the index the
      pcode operands reference.  "scope" is the full 16-bit compiler scope
      (the .hrb byte format strips the upper byte - a known FIXME there);
      "link" is the linker classification the .hrb symbol type byte carries:
      "func" defined here, "extern" defined elsewhere, "deferred" late-bound,
      "none" not a function */
   fprintf( file, "\n  \"symbols\": [" );
   {
      PHB_HSYMBOL pSym = HB_COMP_PARAM->symbols.pFirst;
      HB_BOOL fFirstSym = HB_TRUE;

      while( pSym )
      {
         fprintf( file, "%s\n    { \"name\": ", fFirstSym ? "" : "," );
         fFirstSym = HB_FALSE;
         hb_compAstWriteStr( file, pSym->szName );
         fprintf( file, ", \"scope\": %d, \"link\": \"%s\" }",
                  ( int ) pSym->cScope,
                  ( pSym->cScope & HB_FS_LOCAL ) ? "func" :
                  ( pSym->cScope & HB_FS_DEFERRED ) ? "deferred" :
                  pSym->iFunc ? "extern" : "none" );
         pSym = pSym->pNext;
      }
      fprintf( file, "%s ],", fFirstSym ? "" : "\n  " );
   }

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

      /* ast-24: only for the functions the .hrb serializes - a FILE_DECL
         pseudo function never becomes code */
      if( ( pFunc->funFlags & HB_FUNF_FILE_DECL ) == 0 && pFunc->pCode )
      {
         char szRaw[ 17 ], szNorm[ 17 ];

         hb_compAstPcodeHashes( HB_COMP_PARAM, pFunc, szRaw, szNorm );
         fprintf( file, "\n    \"pcodeSize\": %" HB_PFS "u, \"pcodeHash\": \"%s\", \"pcodeNormHash\": \"%s\",",
                  pFunc->nPCodePos, szRaw, szNorm );
      }

      fprintf( file, "\n    \"declarations\": [" );
      fFirst = HB_TRUE;
      for( n = 0; n < pAst->nDeclCount; ++n )
      {
         PHB_ASTDECL pDecl = &pAst->pDecls[ n ];

         if( pDecl->pFunc == pFunc )
         {
            if( ! fFirst )
               fprintf( file, "," );
            fFirst = HB_FALSE;
            fprintf( file, "\n      { \"sym\": " );
            hb_compAstWriteStr( file, pDecl->szSym );
            fprintf( file, ", \"scope\": \"%s\", \"declLine\": %d, \"param\": %s",
                     hb_compAstDeclScope( pDecl->iScope ), pDecl->iLine,
                     ( pDecl->iScope & HB_VSCOMP_PARAMETER ) ? "true" : "false" );
            if( pDecl->fDim )
               fprintf( file, ", \"dim\": true" );
            if( pDecl->fChk )
               fprintf( file, ", \"chk\": true" );
            if( pDecl->iNameCol >= 0 )
               fprintf( file, ", \"nameLine\": %d, \"nameCol\": %d",
                        pDecl->iNameLine, pDecl->iNameCol );
            hb_compAstWriteType( file, pDecl->cType, pDecl->szClass );
            fprintf( file, " }" );
         }
      }
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
            fprintf( file, ", \"scope\": \"%s\", \"line\": %d",
                     hb_compAstScopeName( pUse->iScope ), pUse->iLine );
            hb_compAstWriteSitePos( file, pAst, pUse->nTok, pUse->iLine );
            fprintf( file, ", \"access\": \"%s\", \"block\": %s%s%s }",
                     hb_compAstAccessName( pUse->iAccess ),
                     pUse->fBlock ? "true" : "false",
                     ( pUse->iScope & HB_VS_FILEWIDE ) ? ", \"filewide\": true" : "",
                     pUse->fChk ? ", \"chk\": true" : "" );
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
            /* ast-25: this callee reaches a symbol through a name the program
               computes while it runs (hb_compFunDynName()).  Only for a call
               the SOURCE makes: `PRIVATE x` compiles into a __mvPrivate() of
               its own, and the name there is a compile-time symbol that
               declarations[] already carries - there is no written call to
               point a reader at, and reporting one would invent a door. */
            const char * szDyn = pCall->nTok != HB_AST_TOK_NONE ?
                                 hb_compFunDynName( pCall->szSym ) : NULL;

            if( ! fFirst )
               fprintf( file, "," );
            fFirst = HB_FALSE;
            fprintf( file, "\n      { \"sym\": " );
            hb_compAstWriteStr( file, pCall->szSym );
            fprintf( file, ", \"line\": %d", pCall->iLine );
            hb_compAstWriteSitePos( file, pAst, pCall->nTok, pCall->iLine );
            if( szDyn )
               fprintf( file, ", \"dyn\": \"%s\"", szDyn );
            fprintf( file, ", \"block\": %s }", pCall->fBlock ? "true" : "false" );
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
            fprintf( file, ", \"line\": %d", pSend->iLine );
            hb_compAstWriteSitePos( file, pAst, pSend->nTok, pSend->iLine );
            fprintf( file, ", \"block\": %s }", pSend->fBlock ? "true" : "false" );
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
            fprintf( file, "\n      { \"kind\": \"%s\", \"line\": %d, \"block\": %s,%s \"expr\": %s }",
                     pStmt->cKind == 's' ? "stmt" : "push",
                     pStmt->iLine, pStmt->fBlock ? "true" : "false",
                     pStmt->fRet ? " \"ret\": true," : "",
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
      {
         hb_xfree( pAst->pTokens[ n ].szText );
         if( pAst->pTokens[ n ].pFrom )
            hb_xfree( pAst->pTokens[ n ].pFrom );
      }
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
      if( pAst->pDecls )
         hb_xfree( pAst->pDecls );
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
