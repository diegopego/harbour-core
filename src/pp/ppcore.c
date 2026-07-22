/*
 * Clipper compatible preprocessor
 *
 * Copyright 2006 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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

/* #define HB_CLP_STRICT */
/* #define HB_PP_STRICT_LINEINFO_TOKEN */

#define _HB_PP_INTERNAL

#include "hbpp.h"
#include "hbdate.h"

#define HB_PP_WARN_EXPLICIT                     1     /* C10?? */
#define HB_PP_WARN_DEFINE_REDEF                 2     /* C1005 */

#define HB_PP_ERR_ILLEGAL_CHAR                  1     /* C2004 */
#define HB_PP_ERR_STRING_TERMINATOR             2     /* C2007 */
#define HB_PP_ERR_MISSING_ENDTEXT               3     /* C2033 */
#define HB_PP_ERR_DEFINE_SYNTAX                 4     /* C2055 */
#define HB_PP_ERR_LABEL_MISSING_IN_DEFINE       5     /* C2057 */
#define HB_PP_ERR_PARE_MISSING_IN_DEFINE        6     /* C2058 */
#define HB_PP_ERR_MISSING_PATTERN_SEP           7     /* C2059 */
#define HB_PP_ERR_UNKNOWN_RESULT_MARKER         8     /* C2060 */
#define HB_PP_ERR_WRONG_LABEL                   9     /* C2061 */
#define HB_PP_ERR_BAD_MATCH_MARKER              10    /* C2062 */
#define HB_PP_ERR_EMPTY_OPTIONAL                11    /* C2065 */
#define HB_PP_ERR_UNCLOSED_OPTIONAL             12    /* C2066 */
#define HB_PP_ERR_DIRECTIVE_IFDEF               13    /* C2068 */
#define HB_PP_ERR_DIRECTIVE_ENDIF               14    /* C2069 */
#define HB_PP_ERR_DIRECTIVE_ELSE                15    /* C2070 */
#define HB_PP_ERR_DIRECTIVE_UNDEF               16    /* C2071 */
#define HB_PP_ERR_AMBIGUOUS_MATCH_PATTERN       17    /* C2072 */
#define HB_PP_ERR_NESTED_OPTIONAL               18    /* C2073 */
#define HB_PP_ERR_EXPLICIT                      19    /* C2074 */
#define HB_PP_ERR_CYCLIC_DEFINE                 20    /* C2078 */
#define HB_PP_ERR_CYCLIC_TRANSLATE              21    /* C2079 */
#define HB_PP_ERR_CYCLIC_COMMAND                22    /* C2080 */
#define HB_PP_ERR_UNTERMINATED_COMMENT          23    /* C2083 */
#define HB_PP_ERR_PRAGMA                        24    /* C20?? */
#define HB_PP_ERR_DIRECTIVE_IF                  25    /* C20?? */
#define HB_PP_ERR_CANNOT_OPEN_INPUT             26    /* C30?? */
#define HB_PP_ERR_FILE_TOO_LONG                 27    /* C30?? */
#define HB_PP_ERR_CANNOT_CREATE_FILE            28    /* C3006 */
#define HB_PP_ERR_CANNOT_OPEN_FILE              29    /* C3007 */
#define HB_PP_ERR_WRONG_FILE_NAME               30    /* C3008 */
#define HB_PP_ERR_NESTED_INCLUDES               31    /* C3009 */
#define HB_PP_ERR_INVALID_DIRECTIVE             32    /* C3010 */
#define HB_PP_ERR_CANNOT_OPEN_RULES             33    /* C3011 */
#define HB_PP_ERR_WRITE_FILE                    34    /* C3029 */


/* warning messages */
static const char * const s_pp_szWarnings[] =
{
   "1%s",                                                               /* C10?? */
   "1Redefinition or duplicate definition of #define %s"                /* C1005 */
};

/* error messages */
static const char * const s_pp_szErrors[] =
{
   "Illegal character '\\x%s'",                                         /* C2004 */
   "Unterminated string '%s'",                                          /* C2007 */
   "Missing ENDTEXT",                                                   /* C2033 */
   "Syntax error in #define",                                           /* C2055 */
   "Label missing in #define",                                          /* C2057 */
   "Comma or right parenthesis missing in #define",                     /* C2058 */
   "Missing => in #translate/#command",                                 /* C2059 */
   "Unknown result marker in #translate/#command",                      /* C2060 */
   "Label error in #translate/#command",                                /* C2061 */
   "Bad match marker in #translate/#command",                           /* C2062 */
   "Empty optional clause in #translate/#command",                      /* C2065 */
   "Unclosed optional clause in #translate/#command",                   /* C2066 */
   "Error in #ifdef",                                                   /* C2068 */
   "#endif does not match #ifdef",                                      /* C2069 */
   "#else does not match #ifdef",                                       /* C2070 */
   "Error in #undef",                                                   /* C2071 */
   "Ambiguous match pattern in #translate/#command",                    /* C2072 */
   "Result pattern contains nested clauses in #translate/#command",     /* C2073 */
   "#error '%s'",                                                       /* C2074 */
   "Circularity detected in #define '%s'",                              /* C2078 */
   "Circularity detected in #translate '%s'",                           /* C2079 */
   "Circularity detected in #command '%s'",                             /* C2080 */
   "Unterminated /* */ comment",                                        /* C2083 */

   "Error in #pragma",                                                  /* C20?? */
   "Error in #if expression",                                           /* C20?? */

   "Cannot open input file '%s'",                                       /* C30?? */

   "File %s is too long",                                               /* C30?? */

   "Can't create preprocessed output file",                             /* C3006 */
   "Can't open #include file '%s'",                                     /* C3007 */
   "Bad filename in #include",                                          /* C3008 */
   "Too many nested #includes",                                         /* C3009 */
   "Invalid name follows #",                                            /* C3010 */
   "Can't open standard rule file '%s'",                                /* C3011 */
   "Write error to intermediate file '%s'"                              /* C3029 */
};


static const HB_PP_OPERATOR s_operators[] =
{
   { ".NOT.", 5, "!"    , HB_PP_TOKEN_NOT       | HB_PP_TOKEN_STATIC },
   { ".AND.", 5, ".AND.", HB_PP_TOKEN_AND       | HB_PP_TOKEN_STATIC },
   { ".OR." , 4, ".OR." , HB_PP_TOKEN_OR        | HB_PP_TOKEN_STATIC },
#ifndef HB_CLP_STRICT
   { "..."  , 3, "..."  , HB_PP_TOKEN_EPSILON   | HB_PP_TOKEN_STATIC },
#endif
   { "**="  , 3, "^="   , HB_PP_TOKEN_EXPEQ     | HB_PP_TOKEN_STATIC },
   { "**"   , 2, "^"    , HB_PP_TOKEN_POWER     | HB_PP_TOKEN_STATIC },
   { "++"   , 2, "++"   , HB_PP_TOKEN_INC       | HB_PP_TOKEN_STATIC },
   { "--"   , 2, "--"   , HB_PP_TOKEN_DEC       | HB_PP_TOKEN_STATIC },
   { "=="   , 2, "=="   , HB_PP_TOKEN_EQUAL     | HB_PP_TOKEN_STATIC },
   { ":="   , 2, ":="   , HB_PP_TOKEN_ASSIGN    | HB_PP_TOKEN_STATIC },
   { "+="   , 2, "+="   , HB_PP_TOKEN_PLUSEQ    | HB_PP_TOKEN_STATIC },
   { "-="   , 2, "-="   , HB_PP_TOKEN_MINUSEQ   | HB_PP_TOKEN_STATIC },
   { "*="   , 2, "*="   , HB_PP_TOKEN_MULTEQ    | HB_PP_TOKEN_STATIC },
   { "/="   , 2, "/="   , HB_PP_TOKEN_DIVEQ     | HB_PP_TOKEN_STATIC },
   { "%="   , 2, "%="   , HB_PP_TOKEN_MODEQ     | HB_PP_TOKEN_STATIC },
   { "^="   , 2, "^="   , HB_PP_TOKEN_EXPEQ     | HB_PP_TOKEN_STATIC },
   { "<="   , 2, "<="   , HB_PP_TOKEN_LE        | HB_PP_TOKEN_STATIC },
   { ">="   , 2, ">="   , HB_PP_TOKEN_GE        | HB_PP_TOKEN_STATIC },
   { "!="   , 2, "<>"   , HB_PP_TOKEN_NE        | HB_PP_TOKEN_STATIC },
   { "<>"   , 2, "<>"   , HB_PP_TOKEN_NE        | HB_PP_TOKEN_STATIC },
   { "->"   , 2, "->"   , HB_PP_TOKEN_ALIAS     | HB_PP_TOKEN_STATIC },
   { "@"    , 1, "@"    , HB_PP_TOKEN_REFERENCE | HB_PP_TOKEN_STATIC },
   { "("    , 1, "("    , HB_PP_TOKEN_LEFT_PB   | HB_PP_TOKEN_STATIC },
   { ")"    , 1, ")"    , HB_PP_TOKEN_RIGHT_PB  | HB_PP_TOKEN_STATIC },
   { "["    , 1, "["    , HB_PP_TOKEN_LEFT_SB   | HB_PP_TOKEN_STATIC },
   { "]"    , 1, "]"    , HB_PP_TOKEN_RIGHT_SB  | HB_PP_TOKEN_STATIC },
   { "{"    , 1, "{"    , HB_PP_TOKEN_LEFT_CB   | HB_PP_TOKEN_STATIC },
   { "}"    , 1, "}"    , HB_PP_TOKEN_RIGHT_CB  | HB_PP_TOKEN_STATIC },
   { ","    , 1, ","    , HB_PP_TOKEN_COMMA     | HB_PP_TOKEN_STATIC },
   { "\\"   , 1, "\\"   , HB_PP_TOKEN_BACKSLASH | HB_PP_TOKEN_STATIC },
   { "|"    , 1, "|"    , HB_PP_TOKEN_PIPE      | HB_PP_TOKEN_STATIC },
   { "."    , 1, "."    , HB_PP_TOKEN_DOT       | HB_PP_TOKEN_STATIC },
   { "&"    , 1, "&"    , HB_PP_TOKEN_AMPERSAND | HB_PP_TOKEN_STATIC },
   { ":"    , 1, ":"    , HB_PP_TOKEN_SEND      | HB_PP_TOKEN_STATIC },
   { "!"    , 1, "!"    , HB_PP_TOKEN_NOT       | HB_PP_TOKEN_STATIC },
   { "="    , 1, "="    , HB_PP_TOKEN_EQ        | HB_PP_TOKEN_STATIC },
   { "<"    , 1, "<"    , HB_PP_TOKEN_LT        | HB_PP_TOKEN_STATIC },
   { ">"    , 1, ">"    , HB_PP_TOKEN_GT        | HB_PP_TOKEN_STATIC },
   { "#"    , 1, "#"    , HB_PP_TOKEN_HASH      | HB_PP_TOKEN_STATIC },
   { "$"    , 1, "$"    , HB_PP_TOKEN_IN        | HB_PP_TOKEN_STATIC },
   { "+"    , 1, "+"    , HB_PP_TOKEN_PLUS      | HB_PP_TOKEN_STATIC },
   { "-"    , 1, "-"    , HB_PP_TOKEN_MINUS     | HB_PP_TOKEN_STATIC },
   { "*"    , 1, "*"    , HB_PP_TOKEN_MULT      | HB_PP_TOKEN_STATIC },
   { "/"    , 1, "/"    , HB_PP_TOKEN_DIV       | HB_PP_TOKEN_STATIC },
   { "%"    , 1, "%"    , HB_PP_TOKEN_MOD       | HB_PP_TOKEN_STATIC },
   { "^"    , 1, "^"    , HB_PP_TOKEN_POWER     | HB_PP_TOKEN_STATIC }
/* unused: ? ~ " ' ` */
/* not accessible: " ' `  */
/* illegal in Clipper: ~ */
};

static const char s_pp_dynamicResult = 0;

static void hb_pp_disp( PHB_PP_STATE pState, const char * szMessage )
{
   if( ! pState->pDispFunc )
   {
      printf( "%s", szMessage );
      fflush( stdout );
   }
   else
      ( pState->pDispFunc )( pState->cargo, szMessage );
}

static void hb_pp_error( PHB_PP_STATE pState, char type, int iError, const char * szParam )
{
   const char * const * szMsgTable = type == 'W' ? s_pp_szWarnings : s_pp_szErrors;

   if( pState->pErrorFunc )
   {
      ( pState->pErrorFunc )( pState->cargo, szMsgTable, type, iError, szParam, NULL );
   }
   else
   {
      char line[ 16 ];
      char msg[ 200 ];
      char buffer[ 256 ];

      if( pState->pFile )
         hb_snprintf( line, sizeof( line ), "(%d) ", pState->pFile->iCurrentLine );
      else
         line[ 0 ] = '\0';
      hb_snprintf( msg, sizeof( msg ), szMsgTable[ iError - 1 ], szParam );
      hb_snprintf( buffer, sizeof( buffer ), "%s%s: %s\n", line,
                type == 'F' ? "Fatal" : type == 'W' ? "Warning" : "Error", msg );
      hb_pp_disp( pState, buffer );
   }
   if( type != 'W' )
   {
      pState->fError = HB_TRUE;
      pState->iErrors++;
   }
}

static void hb_pp_operatorsFree( PHB_PP_OPERATOR pOperators, int iOperators )
{
   PHB_PP_OPERATOR pOperator = pOperators;

   while( --iOperators >= 0 )
   {
      hb_xfree( HB_UNCONST( pOperator->name ) );
      hb_xfree( HB_UNCONST( pOperator->value ) );
      ++pOperator;
   }
   hb_xfree( pOperators );
}

static const HB_PP_OPERATOR * hb_pp_operatorFind( PHB_PP_STATE pState,
                                                  char * buffer, HB_SIZE nLen )
{
   const HB_PP_OPERATOR * pOperator = pState->pOperators;
   int i = pState->iOperators;

   while( --i >= 0 )
   {
      if( pOperator->len <= nLen && pOperator->name[ 0 ] == buffer[ 0 ] &&
          ( pOperator->len == 1 ||
            hb_strnicmp( pOperator->name + 1, buffer + 1, pOperator->len - 1 ) == 0 ) )
         return pOperator;

      ++pOperator;
   }

   pOperator = s_operators;
   i = HB_SIZEOFARRAY( s_operators );

   do
   {
      if( pOperator->len <= nLen && pOperator->name[ 0 ] == buffer[ 0 ] &&
          ( pOperator->len == 1 ||
            ( pOperator->len >= 4 ?
              hb_strnicmp( pOperator->name + 1, buffer + 1, pOperator->len - 1 ) == 0 :
              ( pOperator->name[ 1 ] == buffer[ 1 ] &&
                ( pOperator->len == 2 || pOperator->name[ 2 ] == buffer[ 2 ] ) ) ) ) )
         return pOperator;

      ++pOperator;
   }
   while( --i > 0 );

   return NULL;
}

#define HB_MEMBUF_DEFAULT_SIZE      256

static PHB_MEM_BUFFER hb_membufNew( void )
{
   PHB_MEM_BUFFER pBuffer = ( PHB_MEM_BUFFER ) hb_xgrab( sizeof( HB_MEM_BUFFER ) );

   pBuffer->nLen = 0;
   pBuffer->nAllocated = HB_MEMBUF_DEFAULT_SIZE;
   pBuffer->pBufPtr = ( char * ) hb_xgrab( pBuffer->nAllocated );

   return pBuffer;
}

static void hb_membufFree( PHB_MEM_BUFFER pBuffer )
{
   hb_xfree( pBuffer->pBufPtr );
   hb_xfree( pBuffer );
}

static void hb_membufFlush( PHB_MEM_BUFFER pBuffer )
{
   pBuffer->nLen = 0;
}

static void hb_membufRemove( PHB_MEM_BUFFER pBuffer, HB_SIZE nLeft )
{
   if( nLeft < pBuffer->nLen )
      pBuffer->nLen = nLeft;
}

static HB_SIZE hb_membufLen( const PHB_MEM_BUFFER pBuffer )
{
   return pBuffer->nLen;
}

static char * hb_membufPtr( const PHB_MEM_BUFFER pBuffer )
{
   return pBuffer->pBufPtr;
}

static void hb_membufAddCh( PHB_MEM_BUFFER pBuffer, char ch )
{
   if( pBuffer->nLen == pBuffer->nAllocated )
   {
      pBuffer->nAllocated <<= 1;
      pBuffer->pBufPtr = ( char * ) hb_xrealloc( pBuffer->pBufPtr, pBuffer->nAllocated );
   }
   pBuffer->pBufPtr[ pBuffer->nLen++ ] = ch;
}

static void hb_membufAddData( PHB_MEM_BUFFER pBuffer, const char * data, HB_SIZE nLen )
{
   if( pBuffer->nLen + nLen > pBuffer->nAllocated )
   {
      do
      {
         pBuffer->nAllocated <<= 1;
      }
      while( pBuffer->nLen + nLen > pBuffer->nAllocated );
      pBuffer->pBufPtr = ( char * ) hb_xrealloc( pBuffer->pBufPtr, pBuffer->nAllocated );
   }

   memcpy( &pBuffer->pBufPtr[ pBuffer->nLen ], data, nLen );
   pBuffer->nLen += nLen;
}

static void hb_membufAddStr( PHB_MEM_BUFFER pBuffer, const char * szText )
{
   hb_membufAddData( pBuffer, szText, strlen( szText ) );
}

static void hb_pp_tokenFree( PHB_PP_TOKEN pToken )
{
   if( HB_PP_TOKEN_ALLOC( pToken->type ) )
      hb_xfree( HB_UNCONST( pToken->value ) );
   if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_MMARKER_RESTRICT ||
       HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_MMARKER_OPTIONAL ||
       HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_RMARKER_OPTIONAL )
   {
      while( pToken->pMTokens )
      {
         PHB_PP_TOKEN pMTokens = pToken->pMTokens;
         pToken->pMTokens = pMTokens->pNext;
         hb_pp_tokenFree( pMTokens );
      }
   }
   hb_xfree( pToken );
}

static void hb_pp_tokenListFree( PHB_PP_TOKEN * pTokenPtr )
{
   if( *pTokenPtr && ! HB_PP_TOKEN_ISPREDEF( *pTokenPtr ) )
   {
      do
      {
         PHB_PP_TOKEN pToken = *pTokenPtr;
         *pTokenPtr = pToken->pNext;
         hb_pp_tokenFree( pToken );
      }
      while( *pTokenPtr );
   }
}

static int hb_pp_tokenListFreeCmd( PHB_PP_TOKEN * pTokenPtr )
{
   HB_BOOL fStop = HB_FALSE;
   int iLines = 0;

   while( *pTokenPtr && ! fStop )
   {
      PHB_PP_TOKEN pToken = *pTokenPtr;
      *pTokenPtr = pToken->pNext;
      if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_EOL )
         ++iLines;
      fStop = HB_PP_TOKEN_ISEOC( pToken );
      hb_pp_tokenFree( pToken );
   }
   return *pTokenPtr ? iLines : 0;
}

static void hb_pp_tokenMoveCommand( PHB_PP_STATE pState,
                                    PHB_PP_TOKEN * pDestPtr,
                                    PHB_PP_TOKEN * pSrcPtr )
{
   PHB_PP_TOKEN pToken;
   int iLines = 0;

   while( *pSrcPtr )
   {
      pToken = *pSrcPtr;
      *pSrcPtr = pToken->pNext;
      *pDestPtr = pToken;
      pDestPtr = &pToken->pNext;
      if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_EOL )
         ++iLines;
      if( HB_PP_TOKEN_ISEOC( pToken ) )
         break;
   }
   *pDestPtr = NULL;

   if( iLines )
   {
      pState->pFile->iLastLine = pState->pFile->iCurrentLine + iLines;
      if( *pSrcPtr )
         pState->pFile->iCurrentLine += iLines;
   }
}

static PHB_PP_TOKEN hb_pp_tokenResultEnd( PHB_PP_TOKEN * pTokenPtr, HB_BOOL fDirect )
{
   PHB_PP_TOKEN pNext = NULL;

#ifdef HB_CLP_STRICT
   HB_SYMBOL_UNUSED( fDirect );
#endif

   while( *pTokenPtr )
   {
      if( HB_PP_TOKEN_ISEOP( *pTokenPtr, fDirect ) )
      {
         pNext = *pTokenPtr;
         *pTokenPtr = NULL;
         break;
      }
      pTokenPtr = &( *pTokenPtr )->pNext;
   }

   return pNext;
}

static PHB_PP_TOKEN hb_pp_tokenNew( const char * value, HB_SIZE nLen,
                                    HB_SIZE nSpaces, HB_USHORT type )
{
   PHB_PP_TOKEN pToken = ( PHB_PP_TOKEN ) hb_xgrab( sizeof( HB_PP_TOKEN ) );

   if( HB_PP_TOKEN_ALLOC( type ) )
   {
      if( nLen <= 1 )
      {
         pToken->value = hb_szAscii[ nLen ? ( HB_UCHAR ) value[ 0 ] : 0 ];
         type |= HB_PP_TOKEN_STATIC;
      }
      else
      {
         char * val = ( char * ) memcpy( hb_xgrab( nLen + 1 ), value, nLen );
         val[ nLen ] = '\0';
         pToken->value = val;
      }
   }
   else
      pToken->value = value;

   pToken->len    = nLen;
   pToken->spaces = nSpaces;
   pToken->type   = type;
   pToken->index  = 0;
   pToken->pNext  = NULL;
   pToken->pMTokens = NULL;

   return pToken;
}

static void hb_pp_tokenSetValue( PHB_PP_TOKEN pToken,
                                 const char * value, HB_SIZE nLen )
{
   if( HB_PP_TOKEN_ALLOC( pToken->type ) )
      hb_xfree( HB_UNCONST( pToken->value ) );
   if( nLen <= 1 )
   {
      pToken->value = hb_szAscii[ nLen ? ( HB_UCHAR ) value[ 0 ] : 0 ];
      pToken->type |= HB_PP_TOKEN_STATIC;
   }
   else
   {
      char * val = ( char * ) memcpy( hb_xgrab( nLen + 1 ), value, nLen );
      val[ nLen ] = '\0';
      pToken->value = val;
      pToken->type &= ~HB_PP_TOKEN_STATIC;
   }
   pToken->len = nLen;
}

/* source position table for tokens (see hb_pp_trackPos()): open addressing
   hash keyed by token pointer; every recorded token has an entry, either
   with its physical source position or with iCol == -1 when the token was
   synthesized (rule result text, stream buffers, separators) */
typedef struct
{
   const void * pKey;
   const char * value;        /* token identity check: a recycled token */
   HB_SIZE      len;          /* pointer with other content must not match */
   int          iLine;
   int          iCol;
   HB_BOOL      fMainFile;
} HB_PP_POSITEM, * PHB_PP_POSITEM;

typedef struct
{
   PHB_PP_POSITEM pItems;
   HB_SIZE        nSize;       /* power of two */
   HB_SIZE        nCount;
} HB_PP_POSTBL, * PHB_PP_POSTBL;

static void hb_pp_posRecord( PHB_PP_STATE pState, PHB_PP_TOKEN pKey,
                             int iLine, int iCol, HB_BOOL fMainFile )
{
   PHB_PP_POSTBL pTbl = ( PHB_PP_POSTBL ) pState->pPosTbl;
   HB_SIZE nAt;

   if( ! pTbl )
   {
      pTbl = ( PHB_PP_POSTBL ) hb_xgrabz( sizeof( HB_PP_POSTBL ) );
      pTbl->nSize = 1024;
      pTbl->pItems = ( PHB_PP_POSITEM ) hb_xgrabz( pTbl->nSize * sizeof( HB_PP_POSITEM ) );
      pState->pPosTbl = pTbl;
   }
   else if( ( pTbl->nCount << 1 ) >= pTbl->nSize )
   {
      PHB_PP_POSITEM pOld = pTbl->pItems;
      HB_SIZE nOldSize = pTbl->nSize, n;

      pTbl->nSize <<= 1;
      pTbl->pItems = ( PHB_PP_POSITEM ) hb_xgrabz( pTbl->nSize * sizeof( HB_PP_POSITEM ) );
      pTbl->nCount = 0;
      for( n = 0; n < nOldSize; ++n )
      {
         if( pOld[ n ].pKey )
         {
            nAt = ( ( HB_PTRUINT ) pOld[ n ].pKey >> 4 ) & ( pTbl->nSize - 1 );
            while( pTbl->pItems[ nAt ].pKey )
               nAt = ( nAt + 1 ) & ( pTbl->nSize - 1 );
            pTbl->pItems[ nAt ] = pOld[ n ];
            pTbl->nCount++;
         }
      }
      hb_xfree( pOld );
   }

   nAt = ( ( HB_PTRUINT ) pKey >> 4 ) & ( pTbl->nSize - 1 );
   while( pTbl->pItems[ nAt ].pKey && pTbl->pItems[ nAt ].pKey != pKey )
      nAt = ( nAt + 1 ) & ( pTbl->nSize - 1 );
   if( ! pTbl->pItems[ nAt ].pKey )
      pTbl->nCount++;
   pTbl->pItems[ nAt ].pKey = pKey;
   pTbl->pItems[ nAt ].value = pKey->value;
   pTbl->pItems[ nAt ].len = pKey->len;
   pTbl->pItems[ nAt ].iLine = iLine;
   pTbl->pItems[ nAt ].iCol = iCol;
   pTbl->pItems[ nAt ].fMainFile = fMainFile;
}

static PHB_PP_POSITEM hb_pp_posFind( PHB_PP_STATE pState, PHB_PP_TOKEN pKey )
{
   PHB_PP_POSTBL pTbl = ( PHB_PP_POSTBL ) pState->pPosTbl;

   if( pTbl )
   {
      HB_SIZE nAt = ( ( HB_PTRUINT ) pKey >> 4 ) & ( pTbl->nSize - 1 );

      while( pTbl->pItems[ nAt ].pKey )
      {
         if( pTbl->pItems[ nAt ].pKey == pKey )
         {
            /* a token created by a path which does not record positions
               may reuse the memory of a freed one: the entry only counts
               when the content still matches the one recorded */
            if( pTbl->pItems[ nAt ].value == pKey->value &&
                pTbl->pItems[ nAt ].len == pKey->len )
               return &pTbl->pItems[ nAt ];
            return NULL;
         }
         nAt = ( nAt + 1 ) & ( pTbl->nSize - 1 );
      }
   }
   return NULL;
}

/* free the position table: called from hb_pp_reset() - the entries key
   token pointers of the module just compiled, and a recycled pointer
   with matching value/len in the next module would inherit a stale
   position (ghost provenance) - and from the destructor */
static void hb_pp_posTblFree( PHB_PP_STATE pState )
{
   PHB_PP_POSTBL pTbl = ( PHB_PP_POSTBL ) pState->pPosTbl;

   if( pTbl )
   {
      hb_xfree( pTbl->pItems );
      hb_xfree( pTbl );
      pState->pPosTbl = NULL;
   }
}

/* record the position of a token cut from the current physical input
   line: value still points into the line buffer at this moment */
static void hb_pp_posTrack( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                            const char * value )
{
   const char * pLine = hb_membufPtr( pState->pBuffer );
   HB_SIZE nLineLen = hb_membufLen( pState->pBuffer );
   int iCol = -1;

   if( value >= pLine && value < pLine + nLineLen )
      iCol = ( int ) ( value - pLine );

   hb_pp_posRecord( pState, pToken,
                    pState->pFile ? pState->pFile->iCurrentLine : 0, iCol,
                    pState->pFile == NULL || pState->pFile->pPrev == NULL );
}

/* token derivation table (see hb_pp_trackPos()): open addressing hash
   keyed by token pointer; records, for tokens synthesized during rule
   application, which match marker of which application each byte range
   of the token text derives from.  The synthesis operations are
   'c'lone (marker result copied into the rule result), 'p'aste (keyword
   concatenation of rule result tokens), 's'tringify (marker result
   dumped into a string), 'd'/'D' (a DYNVAL literal synthesized from the
   pp's own position state - lowercase for the LINE axis, uppercase for
   the FILE axis) and 'm' (a raw source line turned into a string by the
   strea'm' machinery; no application - iApp is -1 there) - the paste and
   stringify artifacts are the ones that otherwise lose any connection to
   the name the programmer wrote.  Identity is checked like in the
   position table: an entry only counts while the token still holds the
   recorded value/len */
typedef struct
{
   int       iApp;            /* application record index (hb_pp_trackApply()); -1 = none */
   HB_USHORT usMarker;        /* 1-based match marker number */
   char      cOp;             /* 'c', 'p', 's', 'd'/'D', 'm' - see above */
   HB_SIZE   nAt;             /* byte offset inside the token text */
   HB_SIZE   nLen;            /* byte length inside the token text */
} HB_PP_FROMITEM, * PHB_PP_FROMITEM;

typedef struct
{
   const void * pKey;
   const char * value;        /* token identity check, see hb_pp_posFind() */
   HB_SIZE      len;
   PHB_PP_FROMITEM pFrom;
   int          iFromCount;
} HB_PP_DRVITEM, * PHB_PP_DRVITEM;

typedef struct
{
   PHB_PP_DRVITEM pItems;
   HB_SIZE        nSize;       /* power of two */
   HB_SIZE        nCount;
} HB_PP_DRVTBL, * PHB_PP_DRVTBL;

/* free the derivation table: called from hb_pp_reset() - the entries
   reference per-module application indices - and from the destructor */
static void hb_pp_drvTblFree( PHB_PP_STATE pState )
{
   PHB_PP_DRVTBL pTbl = ( PHB_PP_DRVTBL ) pState->pDrvTbl;

   if( pTbl )
   {
      HB_SIZE n;

      for( n = 0; n < pTbl->nSize; ++n )
      {
         if( pTbl->pItems[ n ].pFrom )
            hb_xfree( pTbl->pItems[ n ].pFrom );
      }
      hb_xfree( pTbl->pItems );
      hb_xfree( pTbl );
      pState->pDrvTbl = NULL;
   }
}

static PHB_PP_DRVITEM hb_pp_drvFind( PHB_PP_STATE pState, PHB_PP_TOKEN pKey )
{
   PHB_PP_DRVTBL pTbl = ( PHB_PP_DRVTBL ) pState->pDrvTbl;

   if( pTbl )
   {
      HB_SIZE nAt = ( ( HB_PTRUINT ) pKey >> 4 ) & ( pTbl->nSize - 1 );

      while( pTbl->pItems[ nAt ].pKey )
      {
         if( pTbl->pItems[ nAt ].pKey == pKey )
         {
            if( pTbl->pItems[ nAt ].value == pKey->value &&
                pTbl->pItems[ nAt ].len == pKey->len )
               return &pTbl->pItems[ nAt ];
            return NULL;
         }
         nAt = ( nAt + 1 ) & ( pTbl->nSize - 1 );
      }
   }
   return NULL;
}

/* insert or replace the derivation list of a token: takes ownership of
   the pFrom array */
static void hb_pp_drvSet( PHB_PP_STATE pState, PHB_PP_TOKEN pKey,
                          PHB_PP_FROMITEM pFrom, int iFromCount )
{
   PHB_PP_DRVTBL pTbl = ( PHB_PP_DRVTBL ) pState->pDrvTbl;
   HB_SIZE nAt;

   if( ! pTbl )
   {
      pTbl = ( PHB_PP_DRVTBL ) hb_xgrabz( sizeof( HB_PP_DRVTBL ) );
      pTbl->nSize = 256;
      pTbl->pItems = ( PHB_PP_DRVITEM ) hb_xgrabz( pTbl->nSize * sizeof( HB_PP_DRVITEM ) );
      pState->pDrvTbl = pTbl;
   }
   else if( ( pTbl->nCount << 1 ) >= pTbl->nSize )
   {
      PHB_PP_DRVITEM pOld = pTbl->pItems;
      HB_SIZE nOldSize = pTbl->nSize, n;

      pTbl->nSize <<= 1;
      pTbl->pItems = ( PHB_PP_DRVITEM ) hb_xgrabz( pTbl->nSize * sizeof( HB_PP_DRVITEM ) );
      pTbl->nCount = 0;
      for( n = 0; n < nOldSize; ++n )
      {
         if( pOld[ n ].pKey )
         {
            nAt = ( ( HB_PTRUINT ) pOld[ n ].pKey >> 4 ) & ( pTbl->nSize - 1 );
            while( pTbl->pItems[ nAt ].pKey )
               nAt = ( nAt + 1 ) & ( pTbl->nSize - 1 );
            pTbl->pItems[ nAt ] = pOld[ n ];
            pTbl->nCount++;
         }
      }
      hb_xfree( pOld );
   }

   nAt = ( ( HB_PTRUINT ) pKey >> 4 ) & ( pTbl->nSize - 1 );
   while( pTbl->pItems[ nAt ].pKey && pTbl->pItems[ nAt ].pKey != pKey )
      nAt = ( nAt + 1 ) & ( pTbl->nSize - 1 );
   if( ! pTbl->pItems[ nAt ].pKey )
      pTbl->nCount++;
   else if( pTbl->pItems[ nAt ].pFrom )
      hb_xfree( pTbl->pItems[ nAt ].pFrom );
   pTbl->pItems[ nAt ].pKey = pKey;
   pTbl->pItems[ nAt ].value = pKey->value;
   pTbl->pItems[ nAt ].len = pKey->len;
   pTbl->pItems[ nAt ].pFrom = pFrom;
   pTbl->pItems[ nAt ].iFromCount = iFromCount;
}

/* record a whole-token derivation: the common case of a marker result
   clone or a stringified marker.  iDrvApp < 0 means the current
   application was not recorded (see hb_pp_trackApply()) - no fact to
   reference then */
static void hb_pp_drvAdd1( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                           HB_USHORT usMarker, char cOp )
{
   if( pState->iDrvApp >= 0 && usMarker > 0 )
   {
      PHB_PP_FROMITEM pFrom = ( PHB_PP_FROMITEM ) hb_xgrab( sizeof( HB_PP_FROMITEM ) );

      pFrom->iApp     = pState->iDrvApp;
      pFrom->usMarker = usMarker;
      pFrom->cOp      = cOp;
      pFrom->nAt      = 0;
      pFrom->nLen     = pToken->len;
      hb_pp_drvSet( pState, pToken, pFrom, 1 );
   }
}

/* a DYNVAL literal (__LINE__/__FILE__) has no source marker to derive from -
   the pp SYNTHESIZES it from its own state at the point of expansion.  Record
   a dynval from-item (marker 0) so a consumer can tell the value is
   position-sensitive - and to WHICH application it belongs - without joining
   ppApplications by line.  cOp carries the AXIS the value was read from,
   recorded at the expansion branch itself: 'd' = the current LINE, 'D' = the
   current FILE.  A consumer that shifts lines only cares about the line
   axis - exporting the axis here keeps it from re-deriving the builtin's
   meaning from its name.  Kept out of the ast-12 generating pairs by the
   same iMarker >= 1 && (p|s) filter that guards clone. */
static void hb_pp_drvAddDyn( PHB_PP_STATE pState, PHB_PP_TOKEN pToken, char cOp )
{
   if( pState->iDrvApp >= 0 )
   {
      PHB_PP_FROMITEM pFrom = ( PHB_PP_FROMITEM ) hb_xgrab( sizeof( HB_PP_FROMITEM ) );

      pFrom->iApp     = pState->iDrvApp;
      pFrom->usMarker = 0;
      pFrom->cOp      = cOp;
      pFrom->nAt      = 0;
      pFrom->nLen     = pToken->len;
      hb_pp_drvSet( pState, pToken, pFrom, 1 );
   }
}

/* the string fabricated by the STREAM machinery (TEXT/ENDTEXT, #pragma
   __text|__stream|__cstream) is the user's own source line turned into
   DATA.  Record a 'm' from-item (marker 0, iApp -1: stream mode is entered
   by a directive, not by a rule application) so a consumer can tell DATA
   from a written string literal by DECLARED fact - without inferring it
   from the position shape (a written literal starts after its delimiter,
   a raw line at column 0: true, but that is grammar knowledge, and the
   pp is the one who fabricated the token and can simply say so). */
static void hb_pp_drvAddStream( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   PHB_PP_FROMITEM pFrom = ( PHB_PP_FROMITEM ) hb_xgrab( sizeof( HB_PP_FROMITEM ) );

   pFrom->iApp     = -1;
   pFrom->usMarker = 0;
   pFrom->cOp      = 'm';
   pFrom->nAt      = 0;
   pFrom->nLen     = pToken->len;
   hb_pp_drvSet( pState, pToken, pFrom, 1 );
}

/* derivation of a keyword concatenation (see hb_pp_concatenateKeywords()):
   the merged token inherits the marker origins of both parts as 'paste'
   ranges - the second part's offsets shifted past the first - while rule
   literal parts (no derivation record) contribute nothing.  Returns the
   merged list to be attached after the merge rewrites the token value */
static PHB_PP_FROMITEM hb_pp_drvMerge( PHB_PP_STATE pState,
                                       PHB_PP_TOKEN pToken, PHB_PP_TOKEN pNext,
                                       int * piCount )
{
   PHB_PP_DRVITEM pItem1 = hb_pp_drvFind( pState, pToken );
   PHB_PP_DRVITEM pItem2 = hb_pp_drvFind( pState, pNext );
   PHB_PP_FROMITEM pFrom = NULL;
   int iCount = ( pItem1 ? pItem1->iFromCount : 0 ) +
                ( pItem2 ? pItem2->iFromCount : 0 );

   if( iCount > 0 )
   {
      int i, n = 0;

      pFrom = ( PHB_PP_FROMITEM ) hb_xgrab( iCount * sizeof( HB_PP_FROMITEM ) );
      if( pItem1 )
      {
         for( i = 0; i < pItem1->iFromCount; ++i )
         {
            pFrom[ n ] = pItem1->pFrom[ i ];
            pFrom[ n++ ].cOp = 'p';
         }
      }
      if( pItem2 )
      {
         for( i = 0; i < pItem2->iFromCount; ++i )
         {
            pFrom[ n ] = pItem2->pFrom[ i ];
            pFrom[ n ].nAt += pToken->len;
            pFrom[ n++ ].cOp = 'p';
         }
      }
   }
   *piCount = iCount;
   return pFrom;
}

static PHB_PP_TOKEN hb_pp_tokenClone( PHB_PP_STATE pState, PHB_PP_TOKEN pSource )
{
   PHB_PP_TOKEN pDest = ( PHB_PP_TOKEN ) hb_xgrab( sizeof( HB_PP_TOKEN ) );

   memcpy( pDest, pSource, sizeof( HB_PP_TOKEN ) );
   if( HB_PP_TOKEN_ALLOC( pDest->type ) )
   {
      char * val = ( char * ) memcpy( hb_xgrab( pDest->len + 1 ),
                                      pSource->value, pDest->len );
      val[ pDest->len ] = '\0';
      pDest->value = val;
   }
   pDest->pNext  = NULL;

   if( pState->fTrackPos )
   {
      /* a match marker clone keeps the position of the original source
         token even on a line rewritten by a pp rule; a clone of the
         rule's own result text has no source position */
      PHB_PP_POSITEM pItem = hb_pp_posFind( pState, pSource );
      PHB_PP_DRVITEM pDrv;

      if( pItem )
         hb_pp_posRecord( pState, pDest, pItem->iLine, pItem->iCol,
                          pItem->fMainFile );
      else
         hb_pp_posRecord( pState, pDest,
                          pState->pFile ? pState->pFile->iCurrentLine : 0,
                          -1, pState->pFile == NULL || pState->pFile->pPrev == NULL );

      /* derivation facts survive the clone: the copy carries the same
         text, so the same byte ranges derive from the same application/
         marker.  This is what keeps the origin of a GENERATED rule's
         result text alive through its applications (the stringified
         string in the expansion of a rule that another rule created -
         ast-13); a marker fill cloned right after gets its own record
         via hb_pp_drvAdd1(), which replaces this one */
      pDrv = hb_pp_drvFind( pState, pSource );
      if( pDrv && pDrv->iFromCount > 0 )
      {
         PHB_PP_FROMITEM pFrom = ( PHB_PP_FROMITEM )
                     hb_xgrab( pDrv->iFromCount * sizeof( HB_PP_FROMITEM ) );

         memcpy( pFrom, pDrv->pFrom,
                 pDrv->iFromCount * sizeof( HB_PP_FROMITEM ) );
         hb_pp_drvSet( pState, pDest, pFrom, pDrv->iFromCount );
      }
   }

   return pDest;
}

/* preprocessor rule tracking (see hb_pp_trackPos()): the registration site
   of every #define/#[x]translate/#[x]command rule and, for each rule
   application, the source positions of the tokens the rule consumed -
   those tokens (the words of a preprocessor DSL) never reach the parser,
   so this is the only record of where they live in the source.  A rule
   applied without a registration record (built-in table rules, defines
   added through the API) gets one lazily, with no file/line */
/* one token of a rule's match or result pattern, copied at registration
   time (like the application records: later token mutations cannot
   corrupt the copy).  Optional groups are flattened to open/close pseudo
   entries around their content; the literal alternatives of a restrict
   marker follow their marker entry carrying its number.  The order is
   the STORED order of the rule - the one the PP matches with - which for
   consecutive optional groups may differ from the source order (the
   registration reorders keyword-less groups); source order is
   recoverable through the token positions */
typedef struct
{
   char *    szText;          /* NULL for the opt-open/close pseudo entries */
   HB_SIZE   nLen;
   HB_USHORT type;            /* HB_PP_TOKEN_TYPE(): the marker kind for
                                 markers, the raw token type otherwise */
   HB_USHORT marker;          /* match marker number; 0 = none/unnumbered */
   char      cRole;           /* 'l'iteral, 'm'arker, 'r'estrict alternative,
                                 '[' / ']' optional group open/close */
   int       iLine;           /* 0 = no source position */
   int       iCol;            /* -1 = no source column */
   HB_BOOL   fMainFile;
   PHB_PP_FROMITEM pFrom;     /* derivation of the rule token, copied at
                                 registration (like the application tokens):
                                 non-NULL only for a rule GENERATED by
                                 another rule's expansion - its directive
                                 line was synthesized, so its tokens carry
                                 the app/marker each byte range derives
                                 from.  This is the rule GENEALOGY: it links
                                 the generated rule back to the application
                                 that created it (ast-13) */
   int       iFromCount;
   const void * pKey;         /* the pattern token this entry snapshots
                                 (ast-15): lets an application's consumed
                                 token be mapped back to the EXACT rule
                                 pattern token it matched.  Valid while the
                                 rule is registered - the pattern tokens are
                                 owned by the rule */
} HB_PP_RULETOKEN, * PHB_PP_RULETOKEN;

typedef struct
{
   const void * pRule;        /* rule identity at registration; a recycled
                                 pointer is resolved newest-first */
   char *    szFile;          /* directive file, NULL = built-in rule */
   int       iLine;           /* directive line, 0 = built-in */
   char      cType;           /* 'd'efine 't'ranslate 'c'ommand */
   HB_USHORT mode;            /* ast-16: the rule's HB_PP_CMP_* comparison mode.
                                 Was a plain fX bool (== HB_PP_CMP_STD), which
                                 could not tell CMP_CASE (the #y... family) from
                                 CMP_DBASE and reported #ycommand as "command" -
                                 i.e. as abbreviable, which it is NOT */
   char *    szHead;          /* head keyword, NULL when it is a marker */
   HB_USHORT markers;         /* number of match markers */
   /* ast-16: a rule has a LEXICAL LIFETIME - #[x|y]uncommand/#...untranslate
      remove it.  The pp knew this and dropped it on the floor: the removing
      directive left no trace at all, so a consumer could not see that a rule
      stops applying at some point of the source (nor edit that directive when
      renaming the rule it names). */
   HB_BOOL   fDel;            /* this record IS an #un... directive (it removes) */
   int       iDelOf;          /* rule record it removed; -1 = removed NOTHING
                                 (an orphan #un..., i.e. dead directive) */
   HB_BOOL   fRemoved;        /* this rule was later removed by an #un... */
   HB_PP_RULETOKEN * pMatchToks;   /* the rule seen from inside: */
   int       iMatchCount;          /* one entry per pattern token */
   HB_PP_RULETOKEN * pResultToks;
   int       iResultCount;
} HB_PP_RULEREC, * PHB_PP_RULEREC;

typedef struct
{
   char *    szText;
   HB_SIZE   nLen;
   HB_USHORT type;            /* HB_PP_TOKEN_TYPE() value */
   HB_USHORT marker;          /* 0 = rule keyword/literal, N = match marker N */
   int       iLine;
   int       iCol;            /* -1 = no source column */
   HB_BOOL   fMainFile;
   PHB_PP_FROMITEM pFrom;     /* derivation of the consumed token, copied
                                 here because the token dies with the
                                 replacement (multi-pass chains resolve
                                 through these copies) */
   int       iFromCount;
   int       iRuleTok;        /* ast-15: index into the rule's pMatchToks of
                                 the pattern token this token matched, or -1.
                                 Only meaningful for marker == 0 (a literal
                                 word of the rule): it says WHICH literal.
                                 Without it a consumer can only guess from the
                                 text - and the guess is wrong whenever a
                                 secondary keyword is a dBase abbreviation
                                 prefix of another keyword of the same rule */
} HB_PP_APPTOKEN, * PHB_PP_APPTOKEN;

typedef struct
{
   int       iRule;           /* index into the rule records */
   int       iLine;           /* input line at application */
   HB_SIZE   nTokFirst;       /* first consumed token in the token pool */
   int       iTokCount;
} HB_PP_APPREC, * PHB_PP_APPREC;

typedef struct
{
   HB_PP_RULEREC *  pRules;
   HB_SIZE          nRuleCount;
   HB_SIZE          nRuleAlloc;
   HB_PP_APPREC *   pApps;
   HB_SIZE          nAppCount;
   HB_SIZE          nAppAlloc;
   HB_PP_APPTOKEN * pToks;
   HB_SIZE          nTokCount;
   HB_SIZE          nTokAlloc;
} HB_PP_RULETBL, * PHB_PP_RULETBL;

/* ast-15: while matching, the pp pairs each source token with the pattern
   token it matched - and for a LITERAL (a pattern token with no marker index)
   it then DROPS the pairing: hb_pp_patternMatch() only records a binding when
   pMatch->index is set (that is the marker path).  So the tracking tables said
   "marker 0" for every literal without ever saying WHICH literal, and a
   consumer could only guess it from the text.  The guess breaks whenever one
   keyword of a rule is a dBase abbreviation prefix of another keyword of the
   SAME rule (`#command GRAVAR <x> GRAV <y>`: the literal `GRAV`, written in
   full, is indistinguishable from an abbreviated `GRAVAR`).  This scratch list
   keeps the pairing for the matching pass; hb_pp_trackApply() consumes it. */
typedef struct
{
   const void * pSrc;         /* the consumed source token */
   const void * pPat;         /* the rule pattern token it matched */
} HB_PP_LITITEM, * PHB_PP_LITITEM;

typedef struct
{
   HB_PP_LITITEM * pItems;
   HB_SIZE         nCount;
   HB_SIZE         nAlloc;
} HB_PP_LITTBL, * PHB_PP_LITTBL;

static void hb_pp_litClear( PHB_PP_STATE pState )
{
   if( pState->pLitTbl )
      ( ( PHB_PP_LITTBL ) pState->pLitTbl )->nCount = 0;
}

static void hb_pp_litFree( PHB_PP_STATE pState )
{
   if( pState->pLitTbl )
   {
      PHB_PP_LITTBL pTbl = ( PHB_PP_LITTBL ) pState->pLitTbl;
      if( pTbl->pItems )
         hb_xfree( pTbl->pItems );
      hb_xfree( pTbl );
      pState->pLitTbl = NULL;
   }
}

static void hb_pp_litAdd( PHB_PP_STATE pState, const void * pSrc,
                          const void * pPat )
{
   PHB_PP_LITTBL pTbl = ( PHB_PP_LITTBL ) pState->pLitTbl;

   if( ! pTbl )
   {
      pTbl = ( PHB_PP_LITTBL ) hb_xgrabz( sizeof( HB_PP_LITTBL ) );
      pState->pLitTbl = pTbl;
   }
   if( pTbl->nCount == pTbl->nAlloc )
   {
      pTbl->nAlloc += 32;
      pTbl->pItems = ( HB_PP_LITITEM * ) hb_xrealloc( pTbl->pItems,
                              pTbl->nAlloc * sizeof( HB_PP_LITITEM ) );
   }
   pTbl->pItems[ pTbl->nCount ].pSrc = pSrc;
   pTbl->pItems[ pTbl->nCount ].pPat = pPat;
   pTbl->nCount++;
}

/* the pattern token a consumed token matched, NULL when not recorded */
static const void * hb_pp_litGet( PHB_PP_STATE pState, const void * pSrc )
{
   PHB_PP_LITTBL pTbl = ( PHB_PP_LITTBL ) pState->pLitTbl;
   HB_SIZE n;

   if( pTbl )
   {
      for( n = 0; n < pTbl->nCount; ++n )
      {
         if( pTbl->pItems[ n ].pSrc == pSrc )
            return pTbl->pItems[ n ].pPat;
      }
   }

   return NULL;
}

/* free the tracking records: called from hb_pp_reset() so each compiled
   module starts a fresh table (matching the per-module AST dump) and
   from the state destructor */
static void hb_pp_ruleTblFree( PHB_PP_STATE pState )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl )
   {
      HB_SIZE n;

      for( n = 0; n < pTbl->nRuleCount; ++n )
      {
         int i;

         if( pTbl->pRules[ n ].szFile )
            hb_xfree( pTbl->pRules[ n ].szFile );
         if( pTbl->pRules[ n ].szHead )
            hb_xfree( pTbl->pRules[ n ].szHead );
         for( i = 0; i < pTbl->pRules[ n ].iMatchCount; ++i )
         {
            if( pTbl->pRules[ n ].pMatchToks[ i ].szText )
               hb_xfree( pTbl->pRules[ n ].pMatchToks[ i ].szText );
            if( pTbl->pRules[ n ].pMatchToks[ i ].pFrom )
               hb_xfree( pTbl->pRules[ n ].pMatchToks[ i ].pFrom );
         }
         if( pTbl->pRules[ n ].pMatchToks )
            hb_xfree( pTbl->pRules[ n ].pMatchToks );
         for( i = 0; i < pTbl->pRules[ n ].iResultCount; ++i )
         {
            if( pTbl->pRules[ n ].pResultToks[ i ].szText )
               hb_xfree( pTbl->pRules[ n ].pResultToks[ i ].szText );
            if( pTbl->pRules[ n ].pResultToks[ i ].pFrom )
               hb_xfree( pTbl->pRules[ n ].pResultToks[ i ].pFrom );
         }
         if( pTbl->pRules[ n ].pResultToks )
            hb_xfree( pTbl->pRules[ n ].pResultToks );
      }
      for( n = 0; n < pTbl->nTokCount; ++n )
      {
         hb_xfree( pTbl->pToks[ n ].szText );
         if( pTbl->pToks[ n ].pFrom )
            hb_xfree( pTbl->pToks[ n ].pFrom );
      }
      if( pTbl->pRules )
         hb_xfree( pTbl->pRules );
      if( pTbl->pApps )
         hb_xfree( pTbl->pApps );
      if( pTbl->pToks )
         hb_xfree( pTbl->pToks );
      hb_xfree( pTbl );
      pState->pRuleTbl = NULL;
   }
}

/* append one entry to a rule pattern snapshot (see HB_PP_RULETOKEN); the
   derivation list of a generated rule's token is deep-copied here - the
   drv table entry may be replaced once the directive tokens are freed */
static void hb_pp_ruleTokAdd( HB_PP_RULETOKEN ** pToksPtr, int * piCount,
                              int * piAlloc, const char * szText,
                              HB_SIZE nLen, HB_USHORT type, HB_USHORT marker,
                              char cRole, PHB_PP_POSITEM pPos,
                              PHB_PP_DRVITEM pDrv, const void * pKey )
{
   PHB_PP_RULETOKEN pTok;

   if( *piCount == *piAlloc )
   {
      *piAlloc += 16;
      *pToksPtr = ( HB_PP_RULETOKEN * ) hb_xrealloc( *pToksPtr,
                                 *piAlloc * sizeof( HB_PP_RULETOKEN ) );
   }
   pTok = &( *pToksPtr )[ ( *piCount )++ ];
   pTok->szText    = szText ? hb_strndup( szText, nLen ) : NULL;
   pTok->nLen      = nLen;
   pTok->type      = type;
   pTok->marker    = marker;
   pTok->cRole     = cRole;
   pTok->iLine     = pPos ? pPos->iLine : 0;
   pTok->iCol      = pPos ? pPos->iCol : -1;
   pTok->fMainFile = pPos ? pPos->fMainFile : HB_FALSE;
   pTok->pFrom     = NULL;
   pTok->iFromCount = 0;
   pTok->pKey      = pKey;      /* ast-15: identity of the pattern token */
   if( pDrv && pDrv->iFromCount > 0 )
   {
      pTok->iFromCount = pDrv->iFromCount;
      pTok->pFrom = ( PHB_PP_FROMITEM ) hb_xgrab( pDrv->iFromCount *
                                                  sizeof( HB_PP_FROMITEM ) );
      memcpy( pTok->pFrom, pDrv->pFrom,
              pDrv->iFromCount * sizeof( HB_PP_FROMITEM ) );
   }
}

/* snapshot a rule's match or result token list at registration time:
   roles come from the pattern parse the PP itself did (marker types are
   already set on the surviving name tokens, optional groups hang off
   their marker token) and positions from the position table, which at
   this moment still holds a live entry for every token cut from a
   directive line.  Optional groups recurse (the match side may nest) */
/* pStop bounds the walk (NULL = to the end of the list).  #undef needs it: its
   "pattern" is the single name token, but that token still sits in the live
   source line, so an unbounded walk would drag the rest of the line in. */
static void hb_pp_ruleTokWalk( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                               HB_BOOL fResult,
                               HB_PP_RULETOKEN ** pToksPtr,
                               int * piCount, int * piAlloc,
                               PHB_PP_TOKEN pStop )
{
   while( pToken && pToken != pStop )
   {
      HB_USHORT type = HB_PP_TOKEN_TYPE( pToken->type );
      PHB_PP_POSITEM pPos = hb_pp_posFind( pState, pToken );
      /* rule genealogy (ast-13): a rule GENERATED by another rule's
         expansion still holds live derivation entries for its directive
         tokens at this moment - copying them links the rule back to the
         application/marker that created it */
      PHB_PP_DRVITEM pDrv = hb_pp_drvFind( pState, pToken );

      if( type == HB_PP_MMARKER_OPTIONAL || type == HB_PP_RMARKER_OPTIONAL )
      {
         hb_pp_ruleTokAdd( pToksPtr, piCount, piAlloc, NULL, 0, type, 0,
                           '[', NULL, NULL, NULL );
         hb_pp_ruleTokWalk( pState, pToken->pMTokens, fResult,
                            pToksPtr, piCount, piAlloc, NULL );
         hb_pp_ruleTokAdd( pToksPtr, piCount, piAlloc, NULL, 0, type, 0,
                           ']', NULL, NULL, NULL );
      }
      else if( ! fResult && type >= HB_PP_MMARKER_REGULAR &&
               type <= HB_PP_MMARKER_NAME )
      {
         hb_pp_ruleTokAdd( pToksPtr, piCount, piAlloc, pToken->value,
                           pToken->len, type, pToken->index, 'm', pPos, pDrv,
                           pToken );
         if( type == HB_PP_MMARKER_RESTRICT )
         {
            PHB_PP_TOKEN pAlt = pToken->pMTokens;

            while( pAlt )
            {
               hb_pp_ruleTokAdd( pToksPtr, piCount, piAlloc, pAlt->value,
                                 pAlt->len, HB_PP_TOKEN_TYPE( pAlt->type ),
                                 pToken->index, 'r',
                                 hb_pp_posFind( pState, pAlt ),
                                 hb_pp_drvFind( pState, pAlt ), pAlt );
               pAlt = pAlt->pNext;
            }
         }
      }
      else if( fResult && type >= HB_PP_RMARKER_REGULAR &&
               type <= HB_PP_RMARKER_REFERENCE &&
               type != HB_PP_RMARKER_OPTIONAL )
      {
         hb_pp_ruleTokAdd( pToksPtr, piCount, piAlloc, pToken->value,
                           pToken->len, type, pToken->index, 'm', pPos, pDrv,
                           pToken );
      }
      else
      {
         hb_pp_ruleTokAdd( pToksPtr, piCount, piAlloc, pToken->value,
                           pToken->len, type, 0, 'l', pPos, pDrv, pToken );
      }
      pToken = pToken->pNext;
   }
}

/* ast-16: the recording core, driven by the PIECES of a directive (pattern,
   result, mode, markers) rather than by a registered PHB_PP_RULE - because an
   #un... directive never becomes a rule: it REMOVES one and is then thrown
   away.  pRule is the identity used to correlate applications, and is NULL for
   an #un... record (nothing ever applies it). */
static int hb_pp_trackRuleRec( PHB_PP_STATE pState, PHB_PP_RULE pRule,
                               PHB_PP_TOKEN pMatch, PHB_PP_TOKEN pResult,
                               HB_USHORT mode, HB_USHORT markers,
                               char cType, HB_BOOL fDel, int iDelOf,
                               const char * szFile, int iLine,
                               PHB_PP_TOKEN pMatchStop )
{
   PHB_PP_RULETBL pTbl;
   PHB_PP_RULEREC pRec;
   int iAlloc;

   if( ! pState->pRuleTbl )
      pState->pRuleTbl = hb_xgrabz( sizeof( HB_PP_RULETBL ) );
   pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl->nRuleCount == pTbl->nRuleAlloc )
   {
      pTbl->nRuleAlloc += 64;
      pTbl->pRules = ( HB_PP_RULEREC * ) hb_xrealloc( pTbl->pRules,
                              pTbl->nRuleAlloc * sizeof( HB_PP_RULEREC ) );
   }
   pRec = &pTbl->pRules[ pTbl->nRuleCount ];
   pRec->pRule    = pRule;
   pRec->szFile   = szFile ? hb_strdup( szFile ) : NULL;
   pRec->iLine    = iLine;
   pRec->cType    = cType;
   pRec->mode     = HB_PP_CMP_MODE( mode );
   pRec->szHead   = HB_PP_TOKEN_ISMATCH( pMatch ) ? NULL :
                    hb_strdup( pMatch->value );
   pRec->markers  = markers;
   pRec->fDel     = fDel;
   pRec->iDelOf   = iDelOf;
   pRec->fRemoved = HB_FALSE;

   pRec->pMatchToks = NULL;
   pRec->iMatchCount = iAlloc = 0;
   hb_pp_ruleTokWalk( pState, pMatch, HB_FALSE,
                      &pRec->pMatchToks, &pRec->iMatchCount, &iAlloc,
                      pMatchStop );
   pRec->pResultToks = NULL;
   pRec->iResultCount = iAlloc = 0;
   hb_pp_ruleTokWalk( pState, pResult, HB_TRUE,
                      &pRec->pResultToks, &pRec->iResultCount, &iAlloc, NULL );

   return ( int ) pTbl->nRuleCount++;
}

static int hb_pp_trackRuleAdd( PHB_PP_STATE pState, PHB_PP_RULE pRule,
                               char cType, const char * szFile, int iLine )
{
   return hb_pp_trackRuleRec( pState, pRule, pRule->pMatch, pRule->pResult,
                              pRule->mode, pRule->markers, cType,
                              HB_FALSE, -1, szFile, iLine, NULL );
}

/* ast-16: the record of the rule this #un... is about to remove, marked as
   removed so a recycled rule pointer never resolves back to it */
static int hb_pp_trackRuleRemoved( PHB_PP_STATE pState, PHB_PP_RULE pRule )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;
   HB_SIZE n;

   if( pTbl )
   {
      n = pTbl->nRuleCount;
      while( n-- )
      {
         PHB_PP_RULEREC pRec = &pTbl->pRules[ n ];

         if( pRec->pRule == ( const void * ) pRule && ! pRec->fDel &&
             ! pRec->fRemoved )
         {
            pRec->fRemoved = HB_TRUE;
            return ( int ) n;
         }
      }
   }
   return -1;
}

/* registration hook: called (gated by fTrackPos) where #define and
   #[x]translate/#[x]command rules enter the rule tables, while the
   directive's file and line are still current */
static void hb_pp_trackRule( PHB_PP_STATE pState, PHB_PP_RULE pRule, char cType )
{
   if( pRule )
      hb_pp_trackRuleAdd( pState, pRule, cType,
                          pState->pFile ? pState->pFile->szFileName : NULL,
                          pState->pFile ? pState->pFile->iCurrentLine : 0 );
}

/* application hook: called (gated by fTrackPos) at the top of
   hb_pp_patternReplace() - the funnel of every define/translate/command
   application - while the matched source tokens and the rule's marker
   results are still alive */
static void hb_pp_trackApply( PHB_PP_STATE pState, PHB_PP_RULE pRule,
                              PHB_PP_TOKEN pFirst, const char * szType )
{
   PHB_PP_RULETBL pTbl;
   PHB_PP_APPREC pApp;
   PHB_PP_TOKEN pTok, * pTokens;
   HB_USHORT * pMarkers;
   int * pLits;
   HB_SIZE nCount = 0, n;
   HB_USHORT m;
   int iRule = -1, i;

   /* until this application is recorded there is nothing the derivation
      entries of its result tokens could reference */
   pState->iDrvApp = -1;

   for( pTok = pFirst; pTok && pTok != pRule->pNextExpr; pTok = pTok->pNext )
      ++nCount;
   if( nCount == 0 )
      return;

   if( pState->pRuleTbl )
   {
      pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;
      n = pTbl->nRuleCount;
      while( n-- )
      {
         if( pTbl->pRules[ n ].pRule == ( const void * ) pRule )
         {
            iRule = ( int ) n;
            break;
         }
      }
   }
   if( iRule < 0 )
      iRule = hb_pp_trackRuleAdd( pState, pRule, szType[ 0 ], NULL, 0 );
   pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   /* which consumed tokens fill which match marker: everything not
      covered by a marker result is a literal word of the rule itself -
      and ast-15 (pLits) says WHICH literal, instead of leaving it to be
      guessed from the token text */
   pTokens = ( PHB_PP_TOKEN * ) hb_xgrab( nCount * sizeof( PHB_PP_TOKEN ) );
   pMarkers = ( HB_USHORT * ) hb_xgrabz( nCount * sizeof( HB_USHORT ) );
   pLits = ( int * ) hb_xgrab( nCount * sizeof( int ) );
   for( n = 0; n < nCount; ++n )
      pLits[ n ] = -1;
   for( pTok = pFirst, n = 0; n < nCount; pTok = pTok->pNext )
      pTokens[ n++ ] = pTok;
   for( m = 0; m < pRule->markers && pRule->pMarkers; ++m )
   {
      PHB_PP_RESULT pResult = pRule->pMarkers[ m ].pResult;

      while( pResult )
      {
         for( pTok = pResult->pFirstToken;
              pTok && pTok != pResult->pNextExpr; pTok = pTok->pNext )
         {
            for( n = 0; n < nCount; ++n )
            {
               if( pTokens[ n ] == pTok )
               {
                  pMarkers[ n ] = m + 1;
                  break;
               }
            }
         }
         pResult = pResult->pNext;
      }
   }

   if( pTbl->nAppCount == pTbl->nAppAlloc )
   {
      pTbl->nAppAlloc += 64;
      pTbl->pApps = ( HB_PP_APPREC * ) hb_xrealloc( pTbl->pApps,
                              pTbl->nAppAlloc * sizeof( HB_PP_APPREC ) );
   }
   /* ast-15: the pairing the matcher remembered (source token -> pattern token)
      becomes the INDEX of that token in the rule's match[] snapshot - the same
      index the dump exposes.  Only meaningful for a literal (marker 0). */
   if( pState->fTrackPos )
   {
      for( n = 0; n < nCount; ++n )
      {
         if( pMarkers[ n ] == 0 )
         {
            const void * pPat = hb_pp_litGet( pState, pTokens[ n ] );

            if( pPat )
            {
               for( i = 0; i < pTbl->pRules[ iRule ].iMatchCount; ++i )
               {
                  if( pTbl->pRules[ iRule ].pMatchToks[ i ].pKey == pPat )
                  {
                     pLits[ n ] = i;
                     break;
                  }
               }
            }
         }
      }
   }

   pApp = &pTbl->pApps[ pTbl->nAppCount++ ];
   pApp->iRule     = iRule;
   pApp->iLine     = pState->pFile ? pState->pFile->iCurrentLine : 0;
   pApp->nTokFirst = pTbl->nTokCount;
   pApp->iTokCount = ( int ) nCount;
   pState->iDrvApp = ( int ) pTbl->nAppCount - 1;

   for( n = 0; n < nCount; ++n )
   {
      PHB_PP_APPTOKEN pRec;
      PHB_PP_POSITEM pPos = hb_pp_posFind( pState, pTokens[ n ] );
      PHB_PP_DRVITEM pDrv = hb_pp_drvFind( pState, pTokens[ n ] );

      if( pTbl->nTokCount == pTbl->nTokAlloc )
      {
         pTbl->nTokAlloc += 256;
         pTbl->pToks = ( HB_PP_APPTOKEN * ) hb_xrealloc( pTbl->pToks,
                                 pTbl->nTokAlloc * sizeof( HB_PP_APPTOKEN ) );
      }
      pRec = &pTbl->pToks[ pTbl->nTokCount++ ];
      pRec->szText = ( char * ) memcpy( hb_xgrab( pTokens[ n ]->len + 1 ),
                                        pTokens[ n ]->value, pTokens[ n ]->len );
      pRec->szText[ pTokens[ n ]->len ] = '\0';
      pRec->nLen   = pTokens[ n ]->len;
      pRec->type   = HB_PP_TOKEN_TYPE( pTokens[ n ]->type );
      pRec->marker = pMarkers[ n ];
      pRec->iRuleTok = pLits[ n ];
      if( pPos )
      {
         pRec->iLine     = pPos->iLine;
         pRec->iCol      = pPos->iCol;
         pRec->fMainFile = pPos->fMainFile;
      }
      else
      {
         pRec->iLine     = pState->pFile ? pState->pFile->iCurrentLine : 0;
         pRec->iCol      = -1;
         pRec->fMainFile = pState->pFile == NULL || pState->pFile->pPrev == NULL;
      }
      if( pDrv && pDrv->iFromCount > 0 )
      {
         pRec->pFrom = ( PHB_PP_FROMITEM ) memcpy(
                     hb_xgrab( pDrv->iFromCount * sizeof( HB_PP_FROMITEM ) ),
                     pDrv->pFrom, pDrv->iFromCount * sizeof( HB_PP_FROMITEM ) );
         pRec->iFromCount = pDrv->iFromCount;
      }
      else
      {
         pRec->pFrom = NULL;
         pRec->iFromCount = 0;
      }
   }

   hb_xfree( pTokens );
   hb_xfree( pMarkers );
   hb_xfree( pLits );
   hb_pp_litClear( pState );   /* ast-15: the pairing dies with the application */
}

static PHB_PP_TOKEN hb_pp_tokenAdd( PHB_PP_TOKEN ** pTokenPtr,
                                    const char * value, HB_SIZE nLen,
                                    HB_SIZE nSpaces, HB_USHORT type )
{
   PHB_PP_TOKEN pToken = hb_pp_tokenNew( value, nLen, nSpaces, type );

   **pTokenPtr = pToken;
   *pTokenPtr  = &pToken->pNext;

   return pToken;
}

static void hb_pp_tokenAddCmdSep( PHB_PP_STATE pState )
{
   PHB_PP_TOKEN pToken = hb_pp_tokenAdd( &pState->pNextTokenPtr, ";", 1, pState->nSpacesNL, HB_PP_TOKEN_EOC | HB_PP_TOKEN_STATIC );

   if( pState->fTrackPos )
      hb_pp_posRecord( pState, pToken,
                       pState->pFile ? pState->pFile->iCurrentLine : 0, -1,
                       pState->pFile == NULL || pState->pFile->pPrev == NULL );
   pState->pFile->iTokens++;
   pState->fNewStatement = HB_TRUE;
   pState->fCanNextLine = HB_FALSE;
   if( pState->iBlockState )
   {
      if( pState->iBlockState == 5 )
         pState->iNestedBlock++;
      pState->iBlockState = 0;
   }
}

static void hb_pp_tokenAddNext( PHB_PP_STATE pState, const char * value, HB_SIZE nLen,
                                HB_USHORT type )
{
   const char * pValueOrig = value;   /* for position tracking: value may
                                         still point into the line buffer */
   PHB_PP_TOKEN pToken;

   if( pState->fCanNextLine )
      hb_pp_tokenAddCmdSep( pState );

   if( ! pState->fDirective )
   {
      if( pState->iNestedBlock && pState->fNewStatement &&
          HB_PP_TOKEN_TYPE( type ) == HB_PP_TOKEN_RIGHT_CB )
      {
         pState->iBlockState = 0;
         pState->iNestedBlock--;
      }
      else if( pState->usLastType == HB_PP_TOKEN_LEFT_CB &&
               HB_PP_TOKEN_TYPE( type ) == HB_PP_TOKEN_PIPE )
      {
         pState->iBlockState = 1;
      }
      else if( pState->iBlockState )
      {
         if( ( pState->iBlockState == 1 || pState->iBlockState == 2 ||
               pState->iBlockState == 4 ) &&
             HB_PP_TOKEN_TYPE( type ) == HB_PP_TOKEN_PIPE )
            pState->iBlockState = 5;
         else if( pState->iBlockState == 1 &&
                  HB_PP_TOKEN_TYPE( type ) == HB_PP_TOKEN_KEYWORD )
            pState->iBlockState = 2;
         else if( pState->iBlockState == 1 &&
                  HB_PP_TOKEN_TYPE( type ) == HB_PP_TOKEN_EPSILON )
            pState->iBlockState = 4;
         else if( pState->iBlockState == 2 &&
                  HB_PP_TOKEN_TYPE( type ) == HB_PP_TOKEN_COMMA )
            pState->iBlockState = 1;
         else
            pState->iBlockState = 0;
      }

      if( pState->fNewStatement && nLen == 1 && *value == '#' )
      {
         pState->fDirective = HB_TRUE;
         value = "#";
         type = HB_PP_TOKEN_DIRECTIVE | HB_PP_TOKEN_STATIC;
      }
   }

#ifndef HB_CLP_STRICT
   if( pState->nSpacesMin != 0 && pState->nSpaces == 0 &&
       HB_PP_TOKEN_TYPE( type ) == HB_PP_TOKEN_KEYWORD )
      pState->nSpaces = pState->nSpacesMin;
#endif
   pToken = hb_pp_tokenAdd( &pState->pNextTokenPtr, value, nLen, pState->nSpaces, type );
   if( pState->fTrackPos )
      hb_pp_posTrack( pState, pToken, pValueOrig );
   pState->pFile->iTokens++;
   pState->fNewStatement = HB_FALSE;

   pState->nSpaces = pState->nSpacesMin = 0;
   pState->usLastType = HB_PP_TOKEN_TYPE( type );

   if( pState->iInLineState != HB_PP_INLINE_OFF )
   {
      if( pState->iInLineState == HB_PP_INLINE_START &&
          pState->usLastType == HB_PP_TOKEN_LEFT_PB )
      {
         pState->iInLineState = HB_PP_INLINE_PARAM;
         pState->iInLineBraces = 1;
      }
      else if( pState->iInLineState == HB_PP_INLINE_PARAM )
      {
         if( pState->usLastType == HB_PP_TOKEN_LEFT_PB )
            pState->iInLineBraces++;
         else if( pState->usLastType == HB_PP_TOKEN_RIGHT_PB )
         {
            if( --pState->iInLineBraces == 0 )
               pState->iInLineState = HB_PP_INLINE_BODY;
         }
      }
      else
         pState->iInLineState = HB_PP_INLINE_OFF;
   }
}

static void hb_pp_tokenAddStreamFunc( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                                      const char * value, HB_SIZE nLen )
{
   while( pToken )
   {
      if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_RMARKER_STRDUMP )
      {
         if( value )
         {
            PHB_PP_TOKEN pStr = hb_pp_tokenAdd( &pState->pNextTokenPtr, value, nLen,
                                                pToken->spaces, HB_PP_TOKEN_STRING );

            /* The stream line is the user's own source text turned into DATA
               (TEXT/ENDTEXT, #pragma __text|__stream|__cstream).  Record where
               it came from: without this the string reaches the compiler with
               no position at all, so a tool cannot even REPORT that a name also
               occurs as data - it can only stay silent about it.
               In the line-by-line mode (Cl*pper TEXT/ENDTEXT) this is exactly
               the line the text sits on; in the joined modes (__stream/__cstream)
               the whole block is emitted as ONE string at the closing line, so
               the position is that of the terminator. */
            if( pState->fTrackPos )
            {
               hb_pp_posRecord( pState, pStr,
                                pState->pFile ? pState->pFile->iCurrentLine : 0, 0,
                                pState->pFile == NULL || pState->pFile->pPrev == NULL );
               /* and SAY it is data: the 'm' from-item is the declared
                  fact a consumer needs to tell this string from a written
                  literal (report it, never edit it) */
               hb_pp_drvAddStream( pState, pStr );
            }
            pState->pFile->iTokens++;
         }
      }
      else
      {
         *pState->pNextTokenPtr = hb_pp_tokenClone( pState, pToken );
         pState->pNextTokenPtr  = &( *pState->pNextTokenPtr )->pNext;
         pState->pFile->iTokens++;
      }
      pToken = pToken->pNext;
   }
   pState->fNewStatement = HB_TRUE;
}

static void hb_pp_readLine( PHB_PP_STATE pState )
{
   int ch, iLine = 0, iBOM = pState->pFile->iCurrentLine == 0 ? 1 : 0;

   for( ;; )
   {
      if( pState->pFile->pLineBuf )
      {
         if( pState->pFile->nLineBufLen )
         {
            ch = ( HB_UCHAR ) pState->pFile->pLineBuf[ 0 ];
            pState->pFile->pLineBuf++;
            pState->pFile->nLineBufLen--;
         }
         else
            break;
      }
      else
      {
         ch = fgetc( pState->pFile->file_in );
         if( ch == EOF )
         {
            pState->pFile->fEof = HB_TRUE;
            break;
         }
      }
      iLine = 1;
      /* In Clipper ^Z works like \n */
      if( ch == '\n' || ch == '\x1a' )
      {
         break;
      }
      /* Clipper strips \r characters even from quoted strings */
      else if( ch != '\r' )
      {
         hb_membufAddCh( pState->pBuffer, ( char ) ch );

         /* strip UTF-8 BOM signature */
         if( iBOM && ch == 0xBF && hb_membufLen( pState->pBuffer ) == 3 )
         {
            iBOM = 0;
            if( hb_membufPtr( pState->pBuffer )[ 0 ] == '\xEF' &&
                hb_membufPtr( pState->pBuffer )[ 1 ] == '\xBB' )
               hb_membufFlush( pState->pBuffer );
         }
      }
   }
   pState->iLineTot += iLine;
   iLine = ++pState->pFile->iCurrentLine / 100;
   if( ! pState->fQuiet && pState->fGauge &&
       iLine != pState->pFile->iLastDisp )
   {
      char szLine[ 12 ];

      pState->pFile->iLastDisp = iLine;
      hb_snprintf( szLine, sizeof( szLine ), "\r%i00\r", iLine );
      hb_pp_disp( pState, szLine );
   }
}

static HB_BOOL hb_pp_canQuote( HB_BOOL fQuote, char * pBuffer, HB_SIZE nLen,
                               HB_SIZE n, HB_SIZE * pnAt )
{
   char cQuote = 0;

   /*
    * TODO: this is Clipper compatible but it breaks valid code so we may
    *       think about changing this condition in the future.
    */
   while( n < nLen )
   {
      if( pBuffer[ n ] == ']' )
      {
         if( cQuote && ! fQuote )
         {
            HB_SIZE u = n + 1;
            cQuote = 0;
            while( u < nLen )
            {
               if( cQuote )
               {
                  if( pBuffer[ u ] == cQuote )
                     cQuote = 0;
               }
               else if( pBuffer[ u ] == '`' )
                  cQuote = '\'';
               else if( pBuffer[ u ] == '\'' || pBuffer[ u ] == '"' )
                  cQuote = pBuffer[ u ];
               else if( pBuffer[ u ] == '[' )
                  hb_pp_canQuote( HB_TRUE, pBuffer, nLen, u + 1, &u );
               ++u;
            }
            fQuote = cQuote == 0;
         }
         if( fQuote )
            *pnAt = n;
         return fQuote;
      }
      else if( ! fQuote )
      {
         if( cQuote )
         {
            if( pBuffer[ n ] == cQuote )
               cQuote = 0;
         }
         else if( pBuffer[ n ] == '`' )
            cQuote = '\'';
         else if( pBuffer[ n ] == '\'' || pBuffer[ n ] == '"' )
            cQuote = pBuffer[ n ];
         else if( HB_PP_ISILLEGAL( pBuffer[ n ] ) )
            fQuote = HB_TRUE;
      }
      ++n;
   }
   return HB_FALSE;
}

static HB_BOOL hb_pp_hasCommand( char * pBuffer, HB_SIZE nLen, HB_SIZE * pnAt, int iCmds, ... )
{
   HB_SIZE n = 0;
   va_list va;
   int i;

   va_start( va, iCmds );
   for( i = 0; i < iCmds && n < nLen; ++i )
   {
      HB_SIZE nl;
      char * cmd = va_arg( va, char * );
      nl = strlen( cmd );
      while( n < nLen && HB_PP_ISBLANK( pBuffer[ n ] ) )
         ++n;
      if( n + nl > nLen || hb_strnicmp( cmd, pBuffer + n, nl ) != 0 )
         break;
      n += nl;
      if( n < nLen && ( HB_PP_ISNEXTIDCHAR( cmd[ nl - 1 ] ) ||
                        HB_PP_ISTEXTCHAR( cmd[ nl - 1 ] ) ) &&
                      ( HB_PP_ISNEXTIDCHAR( pBuffer[ n ] ) ||
                        HB_PP_ISTEXTCHAR( pBuffer[ n ] ) ) )
         break;
   }
   va_end( va );

   if( i == iCmds )
   {
      while( n < nLen && HB_PP_ISBLANK( pBuffer[ n ] ) )
         ++n;

      if( n + 1 < nLen &&
          ( pBuffer[ n ] == '/' || pBuffer[ n ] == '&' ) &&
          pBuffer[ n ] == pBuffer[ n + 1 ] )
         /* strip the rest of line with // or && comment */
         n = nLen;

      if( n == nLen || pBuffer[ n ] == ';' ||
          ( n + 1 < nLen && pBuffer[ n ] == '/' && pBuffer[ n + 1 ] == '*' ) )
      {
         *pnAt = n;
         return HB_TRUE;
      }
   }
   return HB_FALSE;
}

static void hb_pp_dumpEnd( PHB_PP_STATE pState )
{
   pState->iStreamDump = HB_PP_STREAM_OFF;
   if( pState->iCondCompile )
   {
      hb_membufFlush( pState->pDumpBuffer );
   }
   else if( pState->pDumpFunc )
   {
      ( pState->pDumpFunc )( pState->cargo,
                             hb_membufPtr( pState->pDumpBuffer ),
                             hb_membufLen( pState->pDumpBuffer ),
                             pState->iDumpLine + 1 );

      /* I do not like it - dump data should be separated from
         preprocessed .prg code. What is inside DUMP area and
         how it will be interpreted depends on backend not on
         PP itself */
      if( pState->fWritePreprocesed )
      {
         int iLines = 0;
         char * pBuffer;
         HB_SIZE nLen;

         if( pState->pFile->fGenLineInfo )
         {
            fprintf( pState->file_out, "#line %d", pState->iDumpLine );
            if( pState->pFile->szFileName )
            {
               fprintf( pState->file_out, " \"%s\"", pState->pFile->szFileName );
            }
            fputc( '\n', pState->file_out );
            pState->pFile->fGenLineInfo = HB_FALSE;
         }
         else if( pState->pFile->iLastLine < pState->iDumpLine )
         {
            do
            {
               fputc( '\n', pState->file_out );
            }
            while( ++pState->pFile->iLastLine < pState->iDumpLine );
         }
         pBuffer = hb_membufPtr( pState->pDumpBuffer );
         nLen = hb_membufLen( pState->pDumpBuffer );
         fputs( "#pragma BEGINDUMP\n", pState->file_out );
         if( fwrite( pBuffer, sizeof( char ), nLen, pState->file_out ) != nLen )
            hb_pp_error( pState, 'F', HB_PP_ERR_WRITE_FILE, pState->szOutFileName );
         fputs( "#pragma ENDDUMP\n", pState->file_out );

         while( nLen-- )
         {
            if( *pBuffer++ == '\n' )
               ++iLines;
         }
         pState->pFile->iLastLine = pState->iDumpLine + iLines + 2;
      }
      hb_membufFlush( pState->pDumpBuffer );
   }
}

static void hb_pp_getLine( PHB_PP_STATE pState )
{
   PHB_PP_TOKEN * pInLinePtr, * pEolTokenPtr;
   char * pBuffer;
   HB_BOOL fDump = HB_FALSE;
   int iLines = 0, iStartLine;

   pInLinePtr = pEolTokenPtr = NULL;
   hb_pp_tokenListFree( &pState->pFile->pTokenList );
   pState->pNextTokenPtr = &pState->pFile->pTokenList;
   pState->pFile->iTokens = 0;
   pState->nSpaces = pState->nSpacesMin = 0;
   pState->fCanNextLine = pState->fDirective = HB_FALSE;
   pState->fNewStatement = HB_TRUE;
   pState->usLastType = HB_PP_TOKEN_NUL;
   pState->iInLineState = HB_PP_INLINE_OFF;
   pState->iInLineBraces = 0;
   pState->iBlockState = pState->iNestedBlock = 0;
   iStartLine = pState->pFile->iCurrentLine + 1;

   do
   {
      HB_SIZE nLen, n;

      hb_membufFlush( pState->pBuffer );
      hb_pp_readLine( pState );
      pBuffer = hb_membufPtr( pState->pBuffer );
      nLen = hb_membufLen( pState->pBuffer );
      if( pState->fCanNextLine )
      {
         pState->nSpaces = pState->nSpacesNL;
         /*
          * set minimum number of leading spaces to 1 to avoid problems
          * with automatic word concatenation which is not Clipper compatible
          */
         pState->nSpacesMin = 1;
         pState->fCanNextLine = HB_FALSE;
         /* Clipper left only last leading blank character from
            concatenated lines */
         if( nLen > 1 && HB_PP_ISBLANK( pBuffer[ 0 ] ) )
         {
            while( nLen > 1 && HB_PP_ISBLANK( pBuffer[ 1 ] ) )
            {
               --nLen;
               ++pBuffer;
            }
         }
      }
      else if( pState->iStreamDump && nLen == 0 )
      {
         pBuffer[ 0 ] = '\0';
         fDump = HB_TRUE;
      }
      n = 0;
      while( n < nLen || fDump )
      {
         char ch = pBuffer[ 0 ];
         if( pState->iStreamDump )
         {
            fDump = HB_FALSE;
            if( pState->iStreamDump == HB_PP_STREAM_COMMENT )
            {
               if( nLen > 0 )
               {
                  ++n;
                  if( nLen > 1 && ch == '*' && pBuffer[ 1 ] == '/' )
                  {
                     pState->iStreamDump = HB_PP_STREAM_OFF;
                     /* Clipper clear number of leading spaces when multiline
                        comment ends */
                     pState->nSpaces = 0;
                     /*
                      * but we cannot make the same because we have automatic
                      * word concatenation which is not Clipper compatible and
                      * will break code like:
                      */
#if 0
                     "//   if /**/lVar; endif" /* enclosed in double-quotes to make commit checker happy */
#endif
                     pState->nSpacesMin = 1;
                     ++n;
                  }
               }
            }
            else if( pState->iStreamDump == HB_PP_STREAM_INLINE_C )
            {
               if( nLen > 0 )
               {
                  ++n;
                  switch( pState->iInLineState )
                  {
                     case HB_PP_INLINE_QUOTE1:
                        if( ch == '\'' )
                           pState->iInLineState = HB_PP_INLINE_OFF;
                        else if( ch == '\\' && nLen > 1 )
                           ++n;
                        break;

                     case HB_PP_INLINE_QUOTE2:
                        if( ch == '"' )
                           pState->iInLineState = HB_PP_INLINE_OFF;
                        else if( ch == '\\' && nLen > 1 )
                           ++n;
                        break;

                     case HB_PP_INLINE_COMMENT:
                        if( nLen > 1 && ch == '*' && pBuffer[ 1 ] == '/' )
                        {
                           pState->iInLineState = HB_PP_INLINE_OFF;
                           ++n;
                        }
                        break;

                     default:
                        if( ch == '\'' )
                           pState->iInLineState = HB_PP_INLINE_QUOTE1;
                        else if( ch == '"' )
                           pState->iInLineState = HB_PP_INLINE_QUOTE2;
                        else if( ch == '{' )
                           ++pState->iInLineBraces;
                        else if( ch == '}' )
                        {
                           if( --pState->iInLineBraces == 0 )
                              pState->iStreamDump = HB_PP_STREAM_OFF;
                        }
                        else if( nLen > 1 )
                        {
                           if( ch == '/' && pBuffer[ 1 ] == '*' )
                           {
                              pState->iInLineState = HB_PP_INLINE_COMMENT;
                              ++n;
                           }
                           else if( ch == '/' && pBuffer[ 1 ] == '/' )
                              nLen = n = 0;
                        }
                  }
               }
               if( n )
                  hb_membufAddData( pState->pStreamBuffer, pBuffer, n );

               if( nLen == n || pState->iStreamDump == HB_PP_STREAM_OFF )
               {
                  hb_membufAddCh( pState->pStreamBuffer, '\n' );
                  if( pState->iStreamDump == HB_PP_STREAM_OFF )
                  {
                     if( pState->iCondCompile )
                     {
                        ;
                     }
                     else if( pState->pInLineFunc )
                     {
                        char szFunc[ 24 ];
                        hb_snprintf( szFunc, sizeof( szFunc ), "HB_INLINE_%03d", ++pState->iInLineCount );
                        if( pInLinePtr && *pInLinePtr )
                           hb_pp_tokenSetValue( *pInLinePtr, szFunc, strlen( szFunc ) );
                        pState->pInLineFunc( pState->cargo, szFunc,
                                    hb_membufPtr( pState->pStreamBuffer ),
                                    hb_membufLen( pState->pStreamBuffer ),
                                    pState->iDumpLine );
                     }
                     else
                     {
                        hb_pp_tokenAddNext( pState,
                                   hb_membufPtr( pState->pStreamBuffer ),
                                   hb_membufLen( pState->pStreamBuffer ),
                                   HB_PP_TOKEN_TEXT );
                     }
                     hb_membufFlush( pState->pStreamBuffer );
                  }
               }
            }
            else if( pState->iStreamDump == HB_PP_STREAM_DUMP_C )
            {
               if( hb_pp_hasCommand( pBuffer, nLen, &n, 3, "#", "pragma", "enddump" ) )
               {
                  hb_pp_dumpEnd( pState );
               }
               else
               {
                  n = nLen;
                  hb_membufAddData( pState->pDumpBuffer, pBuffer, n );
                  hb_membufAddCh( pState->pDumpBuffer, '\n' );
               }
            }
            else if( hb_pp_hasCommand( pBuffer, nLen, &n, 1, "ENDTEXT" ) ||
                     hb_pp_hasCommand( pBuffer, nLen, &n, 3, "#", "pragma", "__endtext" ) )
            {
               if( pState->iStreamDump == HB_PP_STREAM_CLIPPER )
               {
                  if( pState->pFuncEnd )
                     hb_pp_tokenAddStreamFunc( pState, pState->pFuncEnd, NULL, 0 );
               }
               else
               {
                  /* HB_PP_STREAM_PRG, HB_PP_STREAM_C */
                  hb_pp_tokenAddStreamFunc( pState, pState->pFuncOut,
                                            hb_membufPtr( pState->pStreamBuffer ),
                                            hb_membufLen( pState->pStreamBuffer ) );
                  if( pState->pFuncEnd )
                  {
                     if( pState->pFuncOut )
                        hb_pp_tokenAddCmdSep( pState );
                     hb_pp_tokenAddStreamFunc( pState, pState->pFuncEnd,
                                               hb_membufPtr( pState->pStreamBuffer ),
                                               hb_membufLen( pState->pStreamBuffer ) );
                  }
                  hb_membufFlush( pState->pStreamBuffer );
               }
               hb_pp_tokenListFree( &pState->pFuncOut );
               hb_pp_tokenListFree( &pState->pFuncEnd );
               pState->iStreamDump = HB_PP_STREAM_OFF;
            }
            else if( pState->iStreamDump == HB_PP_STREAM_CLIPPER )
            {
               n = nLen;
               hb_pp_tokenAddStreamFunc( pState, pState->pFuncOut, pBuffer, n );
            }
            else /* HB_PP_STREAM_PRG, HB_PP_STREAM_C */
            {
               n = nLen;
               if( pState->iStreamDump == HB_PP_STREAM_C )
                  hb_strRemEscSeq( pBuffer, &n );
               hb_membufAddData( pState->pStreamBuffer, pBuffer, n );
               hb_membufAddCh( pState->pStreamBuffer, '\n' );
               n = nLen; /* hb_strRemEscSeq() above could change n */
            }
         }
#ifndef HB_CLP_STRICT
         else if( ( ( ch == 'e' || ch == 'E' ) && nLen > 1 &&
                    pBuffer[ 1 ] == '"' ) || ( ch == '"' && pState->fEscStr ) )
         {
            HB_SIZE nStrip, u;

            if( ch != '"' )
               ++n;
            while( ++n < nLen && pBuffer[ n ] != '"' )
            {
               if( pBuffer[ n ] == '\\' )
               {
                  if( ++n == nLen )
                     break;
               }
            }
            if( pState->fMultiLineStr )
            {
               while( n == nLen )
               {
                  u = 1;
                  while( n > u && pBuffer[ n - u ] == ' ' )
                     ++u;
                  if( n >= u && pBuffer[ n - u ] == ';' )
                  {
                     n -= u;
                     nLen -= u;
                     u = hb_membufLen( pState->pBuffer ) - u;
                     hb_membufRemove( pState->pBuffer, u );
                     hb_pp_readLine( pState );
                     nLen += hb_membufLen( pState->pBuffer ) - u;
                     pBuffer = hb_membufPtr( pState->pBuffer ) + u - n;
                     --n;
                     while( ++n < nLen && pBuffer[ n ] != '"' )
                     {
                        if( pBuffer[ n ] == '\\' )
                        {
                           if( ++n == nLen )
                              break;
                        }
                     }
                  }
                  else
                     break;
               }
            }
            u = ch != '"' ? 2 : 1;
            nStrip = n - u;
            hb_strRemEscSeq( pBuffer + u, &nStrip );
            hb_pp_tokenAddNext( pState, pBuffer + u, nStrip,
                                HB_PP_TOKEN_STRING );
            if( n == nLen )
            {
               HB_SIZE nSkip = pBuffer - hb_membufPtr( pState->pBuffer );
               hb_membufAddCh( pState->pBuffer, '\0' );
               pBuffer = hb_membufPtr( pState->pBuffer ) + nSkip;
               hb_pp_error( pState, 'E', HB_PP_ERR_STRING_TERMINATOR, pBuffer + u - 1 );
            }
            else
               ++n;
         }
         else if( ( ch == 't' || ch == 'T' ) && nLen > 1 && pBuffer[ 1 ] == '"' )
         {
            ++n;
            while( ++n < nLen && pBuffer[ n ] != '"' )
               ;
            hb_pp_tokenAddNext( pState, pBuffer + 2, n - 2,
                                HB_PP_TOKEN_TIMESTAMP );
            if( n == nLen )
            {
               HB_SIZE nSkip = pBuffer - hb_membufPtr( pState->pBuffer ) + 1;
               hb_membufAddCh( pState->pBuffer, '\0' );
               pBuffer = hb_membufPtr( pState->pBuffer ) + nSkip;
               hb_pp_error( pState, 'E', HB_PP_ERR_STRING_TERMINATOR, pBuffer );
            }
            else
               ++n;
         }
         else if( ( ch == 'd' || ch == 'D' ) && nLen > 1 && pBuffer[ 1 ] == '"' )
         {
            ++n;
            while( ++n < nLen && pBuffer[ n ] != '"' )
               ;
            hb_pp_tokenAddNext( pState, pBuffer + 2, n - 2,
                                HB_PP_TOKEN_DATE );
            if( n == nLen )
            {
               HB_SIZE nSkip = pBuffer - hb_membufPtr( pState->pBuffer ) + 1;
               hb_membufAddCh( pState->pBuffer, '\0' );
               pBuffer = hb_membufPtr( pState->pBuffer ) + nSkip;
               hb_pp_error( pState, 'E', HB_PP_ERR_STRING_TERMINATOR, pBuffer );
            }
            else
               ++n;
         }
#endif
         else if( ch == '"' || ch == '\'' || ch == '`' )
         {
            if( ch == '`' )
               ch = '\'';
            while( ++n < nLen && pBuffer[ n ] != ch )
               ;
            if( pState->fMultiLineStr )
            {
               while( n == nLen )
               {
                  HB_SIZE u = 1;
                  while( n > u && pBuffer[ n - u ] == ' ' )
                     ++u;
                  if( n >= u && pBuffer[ n - u ] == ';' )
                  {
                     n -= u;
                     nLen -= u;
                     u = hb_membufLen( pState->pBuffer ) - u;
                     hb_membufRemove( pState->pBuffer, u );
                     hb_pp_readLine( pState );
                     nLen += hb_membufLen( pState->pBuffer ) - u;
                     pBuffer = hb_membufPtr( pState->pBuffer ) + u - n;
                     --n;
                     while( ++n < nLen && pBuffer[ n ] != ch )
                        ;
                  }
                  else
                  {
                     n = nLen;
                     break;
                  }
               }
            }
            hb_pp_tokenAddNext( pState, pBuffer + 1, n - 1,
                                HB_PP_TOKEN_STRING );
            if( n == nLen )
            {
               HB_SIZE nSkip = pBuffer - hb_membufPtr( pState->pBuffer ) + 1;
               hb_membufAddCh( pState->pBuffer, '\0' );
               pBuffer = hb_membufPtr( pState->pBuffer ) + nSkip;
               hb_pp_error( pState, 'E', HB_PP_ERR_STRING_TERMINATOR, pBuffer );
            }
            else
               ++n;
         }
         else if( ch == '[' && ! pState->fDirective &&
                  hb_pp_canQuote( pState->fCanNextLine ||
                                  HB_PP_TOKEN_CANQUOTE( pState->usLastType ),
                                  pBuffer, nLen, 1, &n ) )
         {
            hb_pp_tokenAddNext( pState, pBuffer + 1, n - 1, HB_PP_TOKEN_STRING );
            ++n;
         }
         else if( ( ch == '/' || ch == '&' ) && nLen > 1 && pBuffer[ 1 ] == ch )
         {
            /* strip the rest of line with // or && comment */
            n = nLen;
         }
         else if( ch == '*' && pState->pFile->iTokens == 0 )
         {
            /* strip the rest of line with * comment */
            n = nLen;
         }
         else if( ch == '/' && nLen > 1 && pBuffer[ 1 ] == '*' )
         {
#ifdef HB_CLP_STRICT
            /* In Clipper multiline comments used after ';' flushes
               the EOC token what causes that ';' is always command
               separator and cannot be used as line concatenator just
               before multiline comments */
            if( pState->fCanNextLine )
               hb_pp_tokenAddCmdSep( pState );
#endif
            pState->iStreamDump = HB_PP_STREAM_COMMENT;
            n += 2;
         }
         else if( ch == ' ' || ch == '\t' )
         {
            do
            {
               if( pBuffer[ n ] == ' ' )
                  pState->nSpaces++;
               else if( pBuffer[ n ] == '\t' )
                  pState->nSpaces += 4;
               else
                  break;
            }
            while( ++n < nLen );
         }
         else if( ch == ';' )
         {
            if( pState->fCanNextLine )
               hb_pp_tokenAddCmdSep( pState );
            pState->fCanNextLine = HB_TRUE;
            pState->nSpacesNL = pState->nSpaces;
            pState->nSpaces = 0;
            ++n;
         }
         else if( HB_PP_ISFIRSTIDCHAR( ch ) )
         {
            while( ++n < nLen && HB_PP_ISNEXTIDCHAR( pBuffer[ n ] ) )
               ;

            /*
             * In Clipper note can be used only as 1st token and after
             * statement separator ';' it does not work like a single line
             * comment.
             */
#ifdef HB_CLP_STRICT
            if( pState->pFile->iTokens == 0 &&
#else
            if( pState->fNewStatement &&
#endif
                n == 4 && hb_strnicmp( "NOTE", pBuffer, 4 ) == 0 )
            {
               /* strip the rest of line */
               n = nLen;
            }
            else
            {
               if( n < nLen && pBuffer[ n ] == '&' )
               {
                  /*
                   * [<keyword>][&<keyword>[.[<nextidchars>]]]+ is a single
                   * token in Clipper and this fact is important in later
                   * preprocessing so we have to replicate it
                   */
                  while( nLen - n > 1 && pBuffer[ n ] == '&' &&
                         HB_PP_ISFIRSTIDCHAR( pBuffer[ n + 1 ] ) )
                  {
                     while( ++n < nLen && HB_PP_ISNEXTIDCHAR( pBuffer[ n ] ) )
                        ;
                     if( n < nLen && pBuffer[ n ] == '.' )
                        while( ++n < nLen && HB_PP_ISNEXTIDCHAR( pBuffer[ n ] ) )
                           ;
                  }
                  if( n < nLen && pBuffer[ n ] == '&' )
                     ++n;
                  hb_pp_tokenAddNext( pState, pBuffer, n, HB_PP_TOKEN_MACROTEXT );
               }
               else if( pState->pInLineFunc &&
                        pState->iInLineState == HB_PP_INLINE_OFF &&
                        n == 9 && hb_strnicmp( "hb_inline", pBuffer, 9 ) == 0 )
               {
                  if( pState->fCanNextLine )
                     hb_pp_tokenAddCmdSep( pState );
                  pInLinePtr = pState->pNextTokenPtr;
                  hb_pp_tokenAddNext( pState, pBuffer, n, HB_PP_TOKEN_KEYWORD );
                  pState->iInLineState = HB_PP_INLINE_START;
                  pState->iInLineBraces = 0;
               }
               else
                  hb_pp_tokenAddNext( pState, pBuffer, n, HB_PP_TOKEN_KEYWORD );
            }
         }
         /* This is Clipper incompatible token - such characters are illegal
            and error message generated, to replicate this behavior is enough
            to change HB_PP_ISILLEGAL() macro */
         else if( HB_PP_ISTEXTCHAR( ch ) )
         {
            while( ++n < nLen && HB_PP_ISTEXTCHAR( pBuffer[ n ] ) )
               ;

            hb_pp_tokenAddNext( pState, pBuffer, n, HB_PP_TOKEN_TEXT );
         }
         else if( HB_PP_ISILLEGAL( ch ) )
         {
            char szCh[ 3 ];

            hb_pp_tokenAddNext( pState, pBuffer, ++n, HB_PP_TOKEN_NUL );
            hb_snprintf( szCh, sizeof( szCh ), "%02x", ch & 0xff );
            hb_pp_error( pState, 'E', HB_PP_ERR_ILLEGAL_CHAR, szCh );
         }
         else if( HB_PP_ISDIGIT( ch ) )
         {
            if( nLen >= 3 && pBuffer[ 0 ] == '0' &&
                ( pBuffer[ 1 ] == 'x' || pBuffer[ 1 ] == 'X' ) &&
                HB_PP_ISHEX( pBuffer[ 2 ] ) )
            {
               n = 2;
               while( ++n < nLen && HB_PP_ISHEX( pBuffer[ n ] ) )
                  ;

               /* (LEX: mark token as hex?) */
               hb_pp_tokenAddNext( pState, pBuffer, n, HB_PP_TOKEN_NUMBER );
            }
            else if( nLen >= 3 && pBuffer[ 0 ] == '0' &&
                     ( pBuffer[ 1 ] == 'd' || pBuffer[ 1 ] == 'D' ) &&
                     HB_PP_ISDIGIT( pBuffer[ 2 ] ) )
            {
               n = 2;
               while( ++n < nLen && HB_PP_ISDIGIT( pBuffer[ n ] ) )
                  ;

               hb_pp_tokenAddNext( pState, pBuffer, n, HB_PP_TOKEN_DATE );
            }
            else
            {
               while( ++n < nLen && HB_PP_ISDIGIT( pBuffer[ n ] ) )
                  ;
               if( nLen - n > 1 && pBuffer[ n ] == '.' &&
                                     HB_PP_ISDIGIT( pBuffer[ n + 1 ] ) )
               {
                  ++n;
                  while( ++n < nLen && HB_PP_ISDIGIT( pBuffer[ n ] ) )
                     ;
               }
               hb_pp_tokenAddNext( pState, pBuffer, n, HB_PP_TOKEN_NUMBER );
            }
         }
         else if( ch == '.' && nLen > 1 && HB_PP_ISDIGIT( pBuffer[ 1 ] ) )
         {
            while( ++n < nLen && HB_PP_ISDIGIT( pBuffer[ n ] ) )
               ;

            hb_pp_tokenAddNext( pState, pBuffer, n, HB_PP_TOKEN_NUMBER );
         }
         else if( ch == '.' && nLen >= 3 && pBuffer[ 2 ] == '.' &&
                  ( HB_PP_ISTRUE( pBuffer[ 1 ] ) || HB_PP_ISFALSE( pBuffer[ 1 ] ) ) )
         {
            const char * value = HB_PP_ISTRUE( pBuffer[ 1 ] ) ? ".T." : ".F.";

            n = 3;
            hb_pp_tokenAddNext( pState, value, n, HB_PP_TOKEN_LOGICAL | HB_PP_TOKEN_STATIC );
         }
         else if( ch == '&' && nLen > 1 && HB_PP_ISFIRSTIDCHAR( pBuffer[ 1 ] ) )
         {
            int iParts = 0;
            /*
             * [<keyword>][&<keyword>[.[<nextidchars>]]]+ is a single token in Clipper
             * and this fact is important in later preprocessing so we have
             * to replicate it
             */
            while( nLen - n > 1 && pBuffer[ n ] == '&' &&
                   HB_PP_ISFIRSTIDCHAR( pBuffer[ n + 1 ] ) )
            {
               ++iParts;
               while( ++n < nLen && HB_PP_ISNEXTIDCHAR( pBuffer[ n ] ) )
                  ;
               if( n < nLen && pBuffer[ n ] == '.' )
                  while( ++n < nLen && HB_PP_ISNEXTIDCHAR( pBuffer[ n ] ) )
                     ++iParts;
            }
            if( n < nLen && pBuffer[ n ] == '&' )
            {
               ++iParts;
               ++n;
            }
            hb_pp_tokenAddNext( pState, pBuffer, n, iParts == 1 ?
                                HB_PP_TOKEN_MACROVAR : HB_PP_TOKEN_MACROTEXT );
         }
         else if( ch == '{' && ! pState->fCanNextLine &&
                  ( pState->iInLineState == HB_PP_INLINE_BODY ||
                    pState->iInLineState == HB_PP_INLINE_START ) )
         {
            if( pState->iInLineState == HB_PP_INLINE_START )
            {
               hb_pp_tokenAddNext( pState, "(", 1, HB_PP_TOKEN_LEFT_PB | HB_PP_TOKEN_STATIC );
               hb_pp_tokenAddNext( pState, ")", 1, HB_PP_TOKEN_RIGHT_PB | HB_PP_TOKEN_STATIC );
            }
            pState->iInLineState = HB_PP_INLINE_OFF;
            pState->iStreamDump = HB_PP_STREAM_INLINE_C;
            pState->iDumpLine = pState->pFile->iCurrentLine - 1;
            if( pState->pStreamBuffer )
               hb_membufFlush( pState->pStreamBuffer );
            else
               pState->pStreamBuffer = hb_membufNew();
         }
         else
         {
            const HB_PP_OPERATOR * pOperator = hb_pp_operatorFind( pState, pBuffer, nLen );

            if( pOperator )
            {
               hb_pp_tokenAddNext( pState, pOperator->value,
                                   strlen( pOperator->value ),
                                   pOperator->type );
               n = pOperator->len;
            }
            else
            {
               hb_pp_tokenAddNext( pState, pBuffer, ++n, HB_PP_TOKEN_OTHER );
            }
         }
         pBuffer += n;
         nLen -= n;
         n = 0;
      }

      if( pEolTokenPtr &&
          ( pEolTokenPtr != pState->pNextTokenPtr ||
            ( pState->iNestedBlock && pState->pFile->iTokens &&
              ( pState->pFile->pLineBuf ? pState->pFile->nLineBufLen == 0 :
                                          pState->pFile->fEof ) ) ) )
      {
         PHB_PP_TOKEN pToken = *pEolTokenPtr;

         while( iStartLine < pState->pFile->iCurrentLine )
         {
            hb_pp_tokenAdd( &pEolTokenPtr, "\n", 1, 0, HB_PP_TOKEN_EOL | HB_PP_TOKEN_STATIC );
            pState->pFile->iTokens++;
            iStartLine++;
            iLines++;
         }
         if( pToken == NULL )
            pState->pNextTokenPtr = pEolTokenPtr;
         *pEolTokenPtr = pToken;
      }

      if( ! pState->fCanNextLine &&
          ! ( pState->iStreamDump && pState->iStreamDump != HB_PP_STREAM_CLIPPER ) &&
          ( pState->iNestedBlock || pState->iBlockState == 5 ) )
      {
         pEolTokenPtr = pState->pNextTokenPtr;
         pState->nSpaces = pState->nSpacesMin = 0;
         pState->fNewStatement = HB_TRUE;
         pState->fDirective = HB_FALSE;
         if( pState->iBlockState )
         {
            if( pState->iBlockState == 5 )
               pState->iNestedBlock++;
            pState->iBlockState = 0;
         }
      }
   }
   while( ( pState->pFile->pLineBuf ? pState->pFile->nLineBufLen != 0 :
                                      ! pState->pFile->fEof ) &&
          ( pState->fCanNextLine || pState->iNestedBlock ||
            ( pState->iStreamDump && pState->iStreamDump != HB_PP_STREAM_CLIPPER ) ) );

   if( pState->iStreamDump )
   {
      if( pState->iStreamDump == HB_PP_STREAM_COMMENT )
         hb_pp_error( pState, 'E', HB_PP_ERR_UNTERMINATED_COMMENT, NULL );
      else if( pState->iStreamDump == HB_PP_STREAM_DUMP_C )
         hb_pp_dumpEnd( pState );
      else if( pState->pFile->pLineBuf ? ! pState->pFile->nLineBufLen :
                                         pState->pFile->fEof )
         hb_pp_error( pState, 'E', HB_PP_ERR_MISSING_ENDTEXT, NULL );
   }

   if( pState->pFile->iTokens != 0 )
   {
      hb_pp_tokenAdd( &pState->pNextTokenPtr, "\n", 1, 0, HB_PP_TOKEN_EOL | HB_PP_TOKEN_STATIC );
      pState->pFile->iTokens++;
   }
   pState->pFile->iCurrentLine -= iLines;
}

static int hb_pp_tokenStr( PHB_PP_TOKEN pToken, PHB_MEM_BUFFER pBuffer,
                           HB_BOOL fSpaces, HB_BOOL fQuote, HB_USHORT ltype )
{
   int iLines = 0;
   HB_ISIZ nSpace = fSpaces ? pToken->spaces : 0;

   /* This is workaround for stringify token list and later decoding by FLEX
      which breaks Clipper compatible code */
   if( nSpace == 0 && fQuote && ltype &&
       ltype >= HB_PP_TOKEN_ASSIGN && ltype != HB_PP_TOKEN_EQ &&
       HB_PP_TOKEN_TYPE( pToken->type ) >= HB_PP_TOKEN_ASSIGN &&
       HB_PP_TOKEN_TYPE( pToken->type ) != HB_PP_TOKEN_EQ )
      nSpace = 1;

   if( nSpace > 0 )
   {
      do
      {
         hb_membufAddCh( pBuffer, ' ' );
      }
      while( --nSpace );
   }

   if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_STRING )
   {
      int iq = 7;
      HB_SIZE n;
      char ch;

      for( n = 0; iq && n < pToken->len; ++n )
      {
         switch( pToken->value[ n ] )
         {
            case '"':
               iq &= ~1;
               break;
            case '\'':
               iq &= ~2;
               break;
            case ']':
               iq &= ~4;
               break;
            case '\n':
            case '\r':
            case '\0':
               iq = 0;
               break;
         }
      }
      if( iq == 0 && fQuote )
      {
         /* generate string with 'e' prefix before opening '"' and quote
            control characters inside, f.e.:
               e"line1\nline2"
          */

         hb_membufAddCh( pBuffer, 'e' );
         hb_membufAddCh( pBuffer, '"' );
         for( n = 0; n < pToken->len; ++n )
         {
            ch = pToken->value[ n ];
            switch( ch )
            {
               case '\r':
                  iq = ch = 'r';
                  break;
               case '\n':
                  iq = ch = 'n';
                  break;
               case '\t':
                  iq = ch = 't';
                  break;
               case '\b':
                  iq = ch = 'b';
                  break;
               case '\f':
                  iq = ch = 'f';
                  break;
               case '\v':
                  iq = ch = 'v';
                  break;
               case '\a':
                  iq = ch = 'a';
                  break;
               case '\0':
                  iq = ch = '0';
                  break;
               case '"':
               case '\\':
                  iq = 1;
                  break;
               default:
                  iq = 0;
                  break;
            }
            if( iq )
               hb_membufAddCh( pBuffer, '\\' );
            hb_membufAddCh( pBuffer, ch );
         }
         hb_membufAddCh( pBuffer, '"' );
      }
      else
      {
         if( iq & 1 )
            ch = '"';
         else if( iq & 2 )
            ch = '\'';
         else
            ch = '[';

         hb_membufAddCh( pBuffer, ch );
         hb_membufAddData( pBuffer, pToken->value, pToken->len );
         hb_membufAddCh( pBuffer, ( char ) ( ch == '[' ? ']' : ch ) );
      }
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_TIMESTAMP )
   {
      if( pToken->len >= 2 && pToken->value[ 0 ] == '0' &&
          ( pToken->value[ 1 ] == 'T' || pToken->value[ 1 ] == 't' ) )
      {
         hb_membufAddData( pBuffer, pToken->value, pToken->len );
      }
      else
      {
         hb_membufAddStr( pBuffer, "t\"" );
         hb_membufAddData( pBuffer, pToken->value, pToken->len );
         hb_membufAddCh( pBuffer, '"' );
      }
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_DATE )
   {
      if( pToken->len >= 2 && pToken->value[ 0 ] == '0' &&
          ( pToken->value[ 1 ] == 'D' || pToken->value[ 1 ] == 'd' ) )
      {
         hb_membufAddData( pBuffer, pToken->value, pToken->len );
      }
      else
      {
         hb_membufAddStr( pBuffer, "d\"" );
         hb_membufAddData( pBuffer, pToken->value, pToken->len );
         hb_membufAddCh( pBuffer, '"' );
      }
   }
   else
   {
      if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_EOL )
         ++iLines;
      hb_membufAddData( pBuffer, pToken->value, pToken->len );
   }

   return iLines;
}

static HB_BOOL hb_pp_tokenValueCmp( PHB_PP_TOKEN pToken, const char * szValue, HB_USHORT mode )
{
   if( pToken->len )
   {
      if( mode == HB_PP_CMP_CASE )
         return memcmp( szValue, pToken->value, pToken->len ) == 0;
      if( mode == HB_PP_CMP_DBASE && pToken->len >= 4 &&
          ( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_KEYWORD ||
            HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_STRING ||
            HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_TEXT ) )
         return hb_strnicmp( szValue, pToken->value, pToken->len ) == 0;
      else
         return hb_stricmp( szValue, pToken->value ) == 0;
   }
   return HB_FALSE;
}

static HB_BOOL hb_pp_tokenEqual( PHB_PP_TOKEN pToken, PHB_PP_TOKEN pMatch,
                                 HB_USHORT mode )
{
   return pToken == pMatch ||
         ( mode != HB_PP_CMP_ADDR &&
           HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_TYPE( pMatch->type ) &&
           ( pToken->len == pMatch->len ||
             ( mode == HB_PP_CMP_DBASE && pMatch->len > 4 &&
               pToken->len >= 4 && pMatch->len > pToken->len ) ) &&
           hb_pp_tokenValueCmp( pToken, pMatch->value, mode ) );
}

static void hb_pp_patternClearResults( PHB_PP_RULE pRule )
{
   PHB_PP_MARKER pMarker = pRule->pMarkers;
   int i = pRule->markers;

   while( --i >= 0 )
   {
      pMarker->matches = 0;
      while( pMarker->pResult )
      {
         PHB_PP_RESULT pResult = pMarker->pResult;
         pMarker->pResult = pResult->pNext;
         hb_xfree( pResult );
      }
      ++pMarker;
   }
   pRule->pNextExpr = NULL;
}

static HB_BOOL hb_pp_patternAddResult( PHB_PP_RULE pRule, HB_USHORT marker,
                                       PHB_PP_TOKEN pFirst, PHB_PP_TOKEN pNext )
{
   PHB_PP_MARKER pMarker = &pRule->pMarkers[ marker - 1 ];

   if( pMarker->matches == 0 || pMarker->canrepeat )
   {
      PHB_PP_RESULT * pResultPtr,
               pResult = ( PHB_PP_RESULT ) hb_xgrab( sizeof( HB_PP_RESULT ) );
      pMarker->matches++;
      pResult->pFirstToken = pFirst;
      pResult->pNextExpr = pNext;
      pResult->pNext = NULL;
      pResultPtr = &pMarker->pResult;
      while( *pResultPtr )
         pResultPtr = &( *pResultPtr )->pNext;
      *pResultPtr = pResult;
      return HB_TRUE;
   }

   return HB_FALSE;
}

static PHB_PP_RULE hb_pp_ruleNew( PHB_PP_TOKEN pMatch, PHB_PP_TOKEN pResult,
                                  HB_USHORT mode, HB_USHORT markers,
                                  PHB_PP_MARKER pMarkers )
{
   PHB_PP_RULE pRule = ( PHB_PP_RULE ) hb_xgrab( sizeof( HB_PP_RULE ) );

   pRule->pPrev = NULL;
   pRule->mode = mode;
   pRule->pMatch = pMatch;
   pRule->pResult = pResult;
   pRule->markers = markers;
   pRule->pMarkers = pMarkers;
   pRule->pNextExpr = NULL;

   return pRule;
}

static void hb_pp_ruleFree( PHB_PP_RULE pRule )
{
   hb_pp_tokenListFree( &pRule->pMatch );
   hb_pp_tokenListFree( &pRule->pResult );
   hb_pp_patternClearResults( pRule );
   if( pRule->pMarkers )
      hb_xfree( pRule->pMarkers );
   hb_xfree( pRule );
}

static void hb_pp_ruleListFree( PHB_PP_RULE * pRulePtr )
{
   PHB_PP_RULE pRule;

   while( *pRulePtr )
   {
      pRule = *pRulePtr;
      *pRulePtr = pRule->pPrev;
      hb_pp_ruleFree( pRule );
   }
}

static void hb_pp_ruleListNonStdFree( PHB_PP_RULE * pRulePtr )
{
   PHB_PP_RULE pRule;

   while( *pRulePtr )
   {
      pRule = *pRulePtr;
      if( ( pRule->mode & HB_PP_STD_RULE ) != 0 )
      {
         pRulePtr = &pRule->pPrev;
      }
      else
      {
         *pRulePtr = pRule->pPrev;
         hb_pp_ruleFree( pRule );
      }
   }
}

static void hb_pp_ruleListSetStd( PHB_PP_RULE pRule )
{
   while( pRule )
   {
      pRule->mode |= HB_PP_STD_RULE;
      pRule = pRule->pPrev;
   }
}

static void hb_pp_ruleSetId( PHB_PP_STATE pState, PHB_PP_TOKEN pMatch, HB_BYTE id )
{
   if( HB_PP_TOKEN_ISMATCH( pMatch ) )
   {
      int i;
      for( i = 0; i < HB_PP_HASHID_MAX; ++i )
         pState->pMap[ i ] |= id;
   }
   else
      pState->pMap[ HB_PP_HASHID( pMatch ) ] |= id;
}

static void hb_pp_ruleListSetId( PHB_PP_STATE pState, PHB_PP_RULE pRule, HB_BYTE id )
{
   while( pRule )
   {
      hb_pp_ruleSetId( pState, pRule->pMatch, id );
      if( HB_PP_TOKEN_ISMATCH( pRule->pMatch ) )
         break;
      pRule = pRule->pPrev;
   }
}

static PHB_PP_RULE hb_pp_defineFind( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   PHB_PP_RULE pRule = pState->pDefinitions;

   /* TODO% create binary tree or hash table - the #define keyword token has
            to be unique so it's not necessary to keep the stack list,
            it will increase the speed when there is a lot of #define values */

   while( pRule && ! hb_pp_tokenEqual( pToken, pRule->pMatch, HB_PP_CMP_CASE ) )
      pRule = pRule->pPrev;

   return pRule;
}

static void hb_pp_defineAdd( PHB_PP_STATE pState, HB_USHORT mode,
                             HB_USHORT markers, PHB_PP_MARKER pMarkers,
                             PHB_PP_TOKEN pMatch, PHB_PP_TOKEN pResult )
{
   PHB_PP_RULE pRule = hb_pp_defineFind( pState, pMatch );

   if( pRule )
   {
      hb_pp_tokenListFree( &pRule->pMatch );
      hb_pp_tokenListFree( &pRule->pResult );
      hb_pp_patternClearResults( pRule );
      if( pRule->pMarkers )
         hb_xfree( pRule->pMarkers );
      pRule->pMatch = pMatch;
      pRule->pResult = pResult;
      pRule->pMarkers = pMarkers;
      pRule->markers = markers;
      pRule->mode = mode;
      hb_pp_error( pState, 'W', HB_PP_WARN_DEFINE_REDEF, pMatch->value );
   }
   else
   {
      pRule = hb_pp_ruleNew( pMatch, pResult, mode, markers, pMarkers );
      pRule->pPrev = pState->pDefinitions;
      pState->pDefinitions = pRule;
      pState->iDefinitions++;
   }
   hb_pp_ruleSetId( pState, pMatch, HB_PP_DEFINE );
}

/* ast-16: a #define has a lifetime too - #undef ends it.  This is the THIRD
   removing family (the other two, #un[x|y]command/#un...translate, go through
   hb_pp_directiveDel) and it was the one initially missed: a tool renaming the
   #define left the #undef behind, pointing at a name that no longer exists, and
   the define leaked past the point where it was switched off. */
static void hb_pp_defineDel( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   PHB_PP_RULE * pRulePtr = &pState->pDefinitions, pRule;

   while( *pRulePtr )
   {
      pRule = *pRulePtr;
      if( hb_pp_tokenEqual( pToken, pRule->pMatch, HB_PP_CMP_CASE ) )
      {
         if( pState->fTrackPos )
         {
            int iDelOf = hb_pp_trackRuleRemoved( pState, pRule );

            /* the #undef's own name token, bounded to ITSELF: it still sits in
               the live source line, and it carries the position a consumer
               needs in order to edit this directive */
            hb_pp_trackRuleRec( pState, NULL, pToken, NULL,
                                HB_PP_CMP_CASE, 0, 'd', HB_TRUE, iDelOf,
                                pState->pFile ? pState->pFile->szFileName : NULL,
                                pState->pFile ? pState->pFile->iCurrentLine : 0,
                                pToken->pNext );
         }
         *pRulePtr = pRule->pPrev;
         hb_pp_ruleFree( pRule );
         pState->iDefinitions--;
         return;
      }
      pRulePtr = &pRule->pPrev;
   }
}

static PHB_PP_FILE hb_pp_FileNew( PHB_PP_STATE pState, const char * szFileName,
                                  HB_BOOL fSysFile, HB_BOOL * pfNested,
                                  FILE * file_in, HB_BOOL fSearchPath,
                                  PHB_PP_OPEN_FUNC pOpenFunc, HB_BOOL fBinary )
{
   char szFileNameBuf[ HB_PATH_MAX ];
   const char * pLineBuf = NULL;
   HB_SIZE nLineBufLen = 0;
   HB_BOOL fFree = HB_FALSE;
   PHB_PP_FILE pFile;

   if( ! file_in )
   {
      int iAction = HB_PP_OPEN_FILE;

      if( pOpenFunc )
      {
         hb_strncpy( szFileNameBuf, szFileName, sizeof( szFileNameBuf ) - 1 );
         iAction = ( pOpenFunc )( pState->cargo, szFileNameBuf,
                                  HB_TRUE, fSysFile, fBinary,
                                  fSearchPath ? pState->pIncludePath : NULL,
                                  pfNested, &file_in,
                                  &pLineBuf, &nLineBufLen, &fFree );
         if( iAction == HB_PP_OPEN_OK )
            szFileName = szFileNameBuf;
      }

      if( iAction == HB_PP_OPEN_FILE )
      {
         PHB_FNAME pFileName = hb_fsFNameSplit( szFileName );
         HB_BOOL fNested = HB_FALSE;

         pFileName->szName = szFileName;
         pFileName->szExtension = NULL;
         if( ! fSysFile )
         {
            if( pFileName->szPath )
               file_in = hb_fopen( szFileName, fBinary ? "rb" : "r" );
            if( ! file_in && ( ! pFileName->szPath || ( ! pFileName->szDrive &&
                ! strchr( HB_OS_PATH_DELIM_CHR_LIST, ( HB_UCHAR ) pFileName->szPath[ 0 ] ) ) ) )
            {
               char * szFirstFName = NULL;
               pFile = pState->pFile;
               while( pFile )
               {
                  if( pFile->szFileName )
                     szFirstFName = pFile->szFileName;
                  pFile = pFile->pPrev;
               }
               if( szFirstFName )
               {
                  PHB_FNAME pFirstFName = hb_fsFNameSplit( szFirstFName );
                  pFileName->szPath = pFirstFName->szPath;
                  hb_fsFNameMerge( szFileNameBuf, pFileName );
                  hb_xfree( pFirstFName );
                  szFileName = szFileNameBuf;
               }
               if( ! pFileName->szPath || szFirstFName )
                  file_in = hb_fopen( szFileName, fBinary ? "rb" : "r" );
            }
            if( file_in )
               iAction = HB_PP_OPEN_OK;
            else
               fNested = hb_fsMaxFilesError();
         }

         if( iAction != HB_PP_OPEN_OK )
         {
            if( fNested )
            {
               if( pfNested )
                  *pfNested = HB_TRUE;
            }
            else if( pState->pIncludePath && fSearchPath )
            {
               HB_PATHNAMES * pPath = pState->pIncludePath;
               do
               {
                  pFileName->szPath = pPath->szPath;
                  hb_fsFNameMerge( szFileNameBuf, pFileName );
                  file_in = hb_fopen( szFileNameBuf, fBinary ? "rb" : "r" );
                  if( file_in != NULL )
                  {
                     iAction = HB_PP_OPEN_OK;
                     szFileName = szFileNameBuf;
                     break;
                  }
                  pPath = pPath->pNext;
               }
               while( pPath );
            }

            if( iAction != HB_PP_OPEN_OK && pOpenFunc && ! fNested )
            {
               hb_strncpy( szFileNameBuf, pFileName->szName, sizeof( szFileNameBuf ) - 1 );
               iAction = ( pOpenFunc )( pState->cargo, szFileNameBuf,
                                        HB_FALSE, fSysFile, fBinary,
                                        fSearchPath ? pState->pIncludePath : NULL,
                                        pfNested, &file_in,
                                        &pLineBuf, &nLineBufLen, &fFree );
               if( iAction == HB_PP_OPEN_OK )
                  szFileName = szFileNameBuf;
            }
         }
         hb_xfree( pFileName );
      }

      if( iAction != HB_PP_OPEN_OK )
         return NULL;

      if( pState->pIncFunc )
         ( pState->pIncFunc )( pState->cargo, szFileName );
   }

   pFile = ( PHB_PP_FILE ) hb_xgrabz( sizeof( HB_PP_FILE ) );

   pFile->szFileName = hb_strdup( szFileName );
   pFile->file_in = file_in;
   pFile->fFree = fFree;
   pFile->pLineBuf = pLineBuf;
   pFile->nLineBufLen = nLineBufLen;
   pFile->iLastLine = 1;

   return pFile;
}

static PHB_PP_FILE hb_pp_FileBufNew( const char * pLineBuf, HB_SIZE nLineBufLen )
{
   PHB_PP_FILE pFile = ( PHB_PP_FILE ) hb_xgrabz( sizeof( HB_PP_FILE ) );

   pFile->fFree = HB_FALSE;
   pFile->pLineBuf = pLineBuf;
   pFile->nLineBufLen = nLineBufLen;
   pFile->iLastLine = 1;

   return pFile;
}

static void hb_pp_FileFree( PHB_PP_STATE pState, PHB_PP_FILE pFile,
                            PHB_PP_CLOSE_FUNC pCloseFunc )
{
   if( pFile->file_in )
   {
      if( pCloseFunc )
         ( pCloseFunc )( pState->cargo, pFile->file_in );
      else
         fclose( pFile->file_in );
   }

   if( pFile->szFileName )
      hb_xfree( pFile->szFileName );

   if( pFile->fFree && pFile->pLineBuf )
      hb_xfree( HB_UNCONST( pFile->pLineBuf ) );

   hb_pp_tokenListFree( &pFile->pTokenList );
   hb_xfree( pFile );
}

static void hb_pp_InFileFree( PHB_PP_STATE pState )
{
   while( pState->pFile )
   {
      PHB_PP_FILE pFile = pState->pFile;
      pState->pFile = pFile->pPrev;
      hb_pp_FileFree( pState, pFile, pState->pCloseFunc );
   }
   pState->iFiles = 0;
}

static void hb_pp_OutFileFree( PHB_PP_STATE pState )
{
   if( pState->file_out )
   {
      fclose( pState->file_out );
      pState->file_out = NULL;
   }
   if( pState->szOutFileName )
   {
      hb_xfree( pState->szOutFileName );
      pState->szOutFileName = NULL;
   }
   pState->fWritePreprocesed = HB_FALSE;
}

static void hb_pp_TraceFileFree( PHB_PP_STATE pState )
{
   if( pState->file_trace )
   {
      fclose( pState->file_trace );
      pState->file_trace = NULL;
   }
   if( pState->szTraceFileName )
   {
      hb_xfree( pState->szTraceFileName );
      pState->szTraceFileName = NULL;
   }
   pState->fWriteTrace = HB_FALSE;
}

static PHB_PP_STATE hb_pp_stateNew( void )
{
   PHB_PP_STATE pState = ( PHB_PP_STATE ) hb_xgrabz( sizeof( HB_PP_STATE ) );

   /* create new line buffer */
   pState->pBuffer = hb_membufNew();

   /* set default maximum number of translations */
   pState->iMaxCycles = HB_PP_MAX_CYCLES;

   return pState;
}

static void hb_pp_stateFree( PHB_PP_STATE pState )
{
   hb_pp_InFileFree( pState );
   hb_pp_OutFileFree( pState );
   hb_pp_TraceFileFree( pState );

   if( pState->pIncludePath )
      hb_fsFreeSearchPath( pState->pIncludePath );

   if( pState->iOperators > 0 )
      hb_pp_operatorsFree( pState->pOperators, pState->iOperators );

   hb_pp_ruleListFree( &pState->pDefinitions );
   hb_pp_ruleListFree( &pState->pTranslations );
   hb_pp_ruleListFree( &pState->pCommands );

   hb_pp_tokenListFree( &pState->pTokenOut );

   hb_membufFree( pState->pBuffer );
   if( pState->pDumpBuffer )
      hb_membufFree( pState->pDumpBuffer );
   if( pState->pOutputBuffer )
      hb_membufFree( pState->pOutputBuffer );
   if( pState->pStreamBuffer )
      hb_membufFree( pState->pStreamBuffer );

   if( pState->pCondStack )
      hb_xfree( pState->pCondStack );

   hb_pp_tokenListFree( &pState->pFuncOut );
   hb_pp_tokenListFree( &pState->pFuncEnd );

   hb_pp_posTblFree( pState );
   hb_pp_ruleTblFree( pState );
   hb_pp_drvTblFree( pState );
   hb_pp_litFree( pState );

   hb_xfree( pState );
}

static PHB_PP_TOKEN hb_pp_streamFuncGet( PHB_PP_TOKEN pToken, PHB_PP_TOKEN * pFuncPtr )
{
   hb_pp_tokenListFree( pFuncPtr );

   if( pToken && HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_PIPE &&
       ! HB_PP_TOKEN_ISEOC( pToken->pNext ) )
   {
      PHB_PP_TOKEN * pStartPtr, * pEndPtr, pStart, pNext;
      pStartPtr = pEndPtr = &pToken->pNext;
      while( ! HB_PP_TOKEN_ISEOC( *pEndPtr ) &&
             HB_PP_TOKEN_TYPE( ( *pEndPtr )->type ) != HB_PP_TOKEN_PIPE )
         pEndPtr = &( *pEndPtr )->pNext;

      pToken = *pEndPtr;
      *pEndPtr = NULL;
      *pFuncPtr = pStart = *pStartPtr;
      *pStartPtr = pToken;
      /* replace %s with HB_PP_RMARKER_STRDUMP marker */
      while( pStart && pStart->pNext )
      {
         pNext = pStart->pNext;
         if( HB_PP_TOKEN_TYPE( pStart->type ) == HB_PP_TOKEN_MOD &&
             HB_PP_TOKEN_TYPE( pNext->type ) == HB_PP_TOKEN_KEYWORD &&
             pNext->len == 1 && pNext->value[ 0 ] == 's' )
         {
            HB_PP_TOKEN_SETTYPE( pStart, HB_PP_RMARKER_STRDUMP );
            pStart->pNext = pNext->pNext;
            hb_pp_tokenFree( pNext );
            pNext = pStart->pNext;
         }
         pStart = pNext;
      }
   }
   return pToken;
}

/* #pragma {__text,__stream,__cstream}|functionOut|functionEnd|functionStart */
static HB_BOOL hb_pp_pragmaStream( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   HB_BOOL fError = HB_FALSE;

   pToken = hb_pp_streamFuncGet( pToken, &pState->pFuncOut );
   pToken = hb_pp_streamFuncGet( pToken, &pState->pFuncEnd );
   if( pToken && HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_PIPE )
   {
      hb_pp_tokenSetValue( pToken, ";", 1 );
      HB_PP_TOKEN_SETTYPE( pToken, HB_PP_TOKEN_EOC );
   }

   return fError;
}

#define MAX_STREAM_SIZE       0x1000000

static void hb_pp_pragmaStreamFile( PHB_PP_STATE pState, const char * szFileName )
{
   PHB_PP_FILE pFile = hb_pp_FileNew( pState, szFileName, HB_FALSE, NULL, NULL,
                                      HB_TRUE, pState->pOpenFunc,
                                      pState->iStreamDump == HB_PP_STREAM_BINARY );

   if( pFile )
   {
      HB_SIZE nSize;

      if( pFile->file_in )
      {
         ( void ) fseek( pFile->file_in, 0L, SEEK_END );
         nSize = ftell( pFile->file_in );
         ( void ) fseek( pFile->file_in, 0L, SEEK_SET );
      }
      else
         nSize = pFile->nLineBufLen;

      if( nSize > MAX_STREAM_SIZE )
         hb_pp_error( pState, 'F', HB_PP_ERR_FILE_TOO_LONG, szFileName );
      else if( pState->pFuncOut || pState->pFuncEnd )
      {
         PHB_PP_TOKEN pToken;
         HB_BOOL fEOL = HB_FALSE;

         if( ! pState->pStreamBuffer )
            pState->pStreamBuffer = hb_membufNew();

         if( nSize )
         {
            if( pFile->file_in == NULL && pState->iStreamDump != HB_PP_STREAM_C )
               hb_membufAddData( pState->pStreamBuffer, pFile->pLineBuf, nSize );
            else
            {
               char * pBuffer = ( char * ) hb_xgrab( nSize * sizeof( char ) );

               if( pFile->file_in )
                  nSize = ( HB_SIZE ) fread( pBuffer, sizeof( char ), nSize, pFile->file_in );
               else
                  memcpy( pBuffer, pFile->pLineBuf, nSize );

               if( pState->iStreamDump == HB_PP_STREAM_C )
                  hb_strRemEscSeq( pBuffer, &nSize );

               hb_membufAddData( pState->pStreamBuffer, pBuffer, nSize );
               hb_xfree( pBuffer );
            }
         }

         /* insert new tokens into incoming buffer
          * so they can be preprocessed
          */
         pState->pNextTokenPtr = &pState->pFile->pTokenList;
         while( ! HB_PP_TOKEN_ISEOS( *pState->pNextTokenPtr ) )
            pState->pNextTokenPtr = &( *pState->pNextTokenPtr )->pNext;
         if( *pState->pNextTokenPtr == NULL )
         {
            hb_pp_tokenAdd( &pState->pNextTokenPtr, "\n", 1, 0, HB_PP_TOKEN_EOL | HB_PP_TOKEN_STATIC );
            pState->pFile->iTokens++;
         }
         else if( HB_PP_TOKEN_TYPE( ( *pState->pNextTokenPtr )->type ) == HB_PP_TOKEN_EOL )
         {
            hb_pp_tokenSetValue( *pState->pNextTokenPtr, ";", 1 );
            HB_PP_TOKEN_SETTYPE( *pState->pNextTokenPtr, HB_PP_TOKEN_EOC );
            fEOL = HB_TRUE;
         }
         pState->pNextTokenPtr = &( *pState->pNextTokenPtr )->pNext;
         pToken = *pState->pNextTokenPtr;

         if( pState->pFuncOut )
            hb_pp_tokenAddStreamFunc( pState, pState->pFuncOut,
                                      hb_membufPtr( pState->pStreamBuffer ),
                                      hb_membufLen( pState->pStreamBuffer ) );
         if( pState->pFuncEnd )
         {
            if( pState->pFuncOut )
               hb_pp_tokenAddCmdSep( pState );
            hb_pp_tokenAddStreamFunc( pState, pState->pFuncEnd,
                                      hb_membufPtr( pState->pStreamBuffer ),
                                      hb_membufLen( pState->pStreamBuffer ) );
         }
         if( fEOL )
            hb_pp_tokenAdd( &pState->pNextTokenPtr, "\n", 1, 0, HB_PP_TOKEN_EOL | HB_PP_TOKEN_STATIC );
         else
            hb_pp_tokenAdd( &pState->pNextTokenPtr, ";", 1, 0, HB_PP_TOKEN_EOC | HB_PP_TOKEN_STATIC );
         pState->pFile->iTokens++;
         pState->fNewStatement = HB_TRUE;
         *pState->pNextTokenPtr = pToken;
         hb_membufFlush( pState->pStreamBuffer );
      }
      hb_pp_FileFree( pState, pFile, pState->pCloseFunc );
   }
   else
      hb_pp_error( pState, 'F', HB_PP_ERR_CANNOT_OPEN_FILE, szFileName );

   hb_pp_tokenListFree( &pState->pFuncOut );
   hb_pp_tokenListFree( &pState->pFuncEnd );
}

static HB_BOOL hb_pp_pragmaOperatorNew( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   HB_BOOL fError = HB_TRUE;

   if( ! HB_PP_TOKEN_ISEOC( pToken ) && HB_PP_TOKEN_CANJOIN( pToken->type ) )
   {
      HB_SIZE nLen;

      hb_membufFlush( pState->pBuffer );
      do
      {
         hb_membufAddData( pState->pBuffer, pToken->value, pToken->len );
         pToken = pToken->pNext;
      }
      while( ! HB_PP_TOKEN_ISEOC( pToken ) && pToken->spaces == 0 );
      nLen = hb_membufLen( pState->pBuffer );
      if( ! HB_PP_TOKEN_ISEOC( pToken ) )
      {
         do
         {
            hb_membufAddData( pState->pBuffer, pToken->value, pToken->len );
            pToken = pToken->pNext;
         }
         while( ! HB_PP_TOKEN_ISEOC( pToken ) && pToken->spaces == 0 );
      }
      if( HB_PP_TOKEN_ISEOC( pToken ) && nLen > 0 )
      {
         PHB_PP_OPERATOR pOperator;
         char * pBuffer = hb_membufPtr( pState->pBuffer ), * pDstBuffer;
         HB_SIZE nDstLen = hb_membufLen( pState->pBuffer ) - nLen;

         if( nDstLen )
            pDstBuffer = pBuffer + nLen;
         else
         {
            pDstBuffer = pBuffer;
            nDstLen = nLen;
         }
         if( pState->iOperators )
            pState->pOperators = ( PHB_PP_OPERATOR ) hb_xrealloc(
                     pState->pOperators,
                     sizeof( HB_PP_OPERATOR ) * ( pState->iOperators + 1 ) );
         else
            pState->pOperators = ( PHB_PP_OPERATOR ) hb_xgrab(
                     sizeof( HB_PP_OPERATOR ) * ( pState->iOperators + 1 ) );
         pOperator = &pState->pOperators[ pState->iOperators++ ];
         pOperator->name  = hb_strndup( pBuffer, nLen );
         pOperator->len   = nLen;
         pOperator->value = hb_strndup( pDstBuffer, nDstLen );
         pOperator->type  = HB_PP_TOKEN_OTHER;
         fError = HB_FALSE;
      }
   }
   return fError;
}

static HB_BOOL hb_pp_setCompilerSwitch( PHB_PP_STATE pState, const char * szSwitch,
                                        int iValue )
{
   HB_BOOL fError = HB_TRUE;

   switch( szSwitch[ 0 ] )
   {
      case 'p':
      case 'P':
         if( szSwitch[ 1 ] == '\0' )
         {
            pState->fWritePreprocesed = pState->file_out != NULL && iValue != 0;
            fError = HB_FALSE;
         }
         else if( szSwitch[ 1 ] == '+' && szSwitch[ 2 ] == '\0' )
         {
            pState->fWriteTrace = pState->file_trace != NULL && iValue != 0;
            fError = HB_FALSE;
         }
         break;

      case 'q':
      case 'Q':
         if( szSwitch[ 1 ] == '\0' )
         {
            pState->fQuiet = iValue != 0;
            fError = HB_FALSE;
         }
         break;
   }

   if( pState->pSwitchFunc )
      fError = ( pState->pSwitchFunc )( pState->cargo, szSwitch, &iValue, HB_TRUE );

   return fError;
}

static HB_BOOL hb_pp_getCompilerSwitch( PHB_PP_STATE pState, const char * szSwitch,
                                        int * piValue )
{
   HB_BOOL fError = HB_TRUE;

   if( pState->pSwitchFunc )
      fError = ( pState->pSwitchFunc )( pState->cargo, szSwitch, piValue, HB_FALSE );

   if( fError )
   {
      switch( szSwitch[ 0 ] )
      {
         case 'p':
         case 'P':
            if( szSwitch[ 1 ] == '\0' )
            {
               *piValue = pState->fWritePreprocesed ? 1 : 0;
               fError = HB_FALSE;
            }
            else if( szSwitch[ 1 ] == '+' && szSwitch[ 2 ] == '\0' )
            {
               *piValue = pState->fWriteTrace ? 1 : 0;
               fError = HB_FALSE;
            }
            break;

         case 'q':
         case 'Q':
            if( szSwitch[ 1 ] == '\0' )
            {
               *piValue = pState->fQuiet ? 1 : 0;
               fError = HB_FALSE;
            }
            break;
      }
   }

   return fError;
}

static PHB_PP_TOKEN hb_pp_pragmaGetLogical( PHB_PP_TOKEN pToken, HB_BOOL * pfValue )
{
   PHB_PP_TOKEN pValue = NULL;

   if( pToken && pToken->pNext &&
       HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_KEYWORD )
   {
      if( ( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_EQ &&
            HB_PP_TOKEN_ISEOC( pToken->pNext->pNext ) ) ||
          ( pToken->pNext->pNext &&
            HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LEFT_PB &&
            HB_PP_TOKEN_TYPE( pToken->pNext->pNext->type ) == HB_PP_TOKEN_RIGHT_PB &&
            HB_PP_TOKEN_ISEOC( pToken->pNext->pNext->pNext ) ) )
      {
         pValue = pToken->pNext;
         if( hb_stricmp( pValue->value, "ON" ) == 0 )
            *pfValue = HB_TRUE;
         else if( hb_stricmp( pValue->value, "OFF" ) == 0 )
            *pfValue = HB_FALSE;
         else
            pValue = NULL;
      }
   }
   return pValue;
}

static PHB_PP_TOKEN hb_pp_pragmaGetInt( PHB_PP_TOKEN pToken, int * piValue )
{
   PHB_PP_TOKEN pValue = NULL;

   if( pToken && pToken->pNext &&
       HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_NUMBER )
   {
      if( ( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_EQ &&
            HB_PP_TOKEN_ISEOC( pToken->pNext->pNext ) ) ||
          ( pToken->pNext->pNext &&
            HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LEFT_PB &&
            HB_PP_TOKEN_TYPE( pToken->pNext->pNext->type ) == HB_PP_TOKEN_RIGHT_PB &&
            HB_PP_TOKEN_ISEOC( pToken->pNext->pNext->pNext ) ) )
      {
         pValue = pToken->pNext;
         *piValue = atoi( pValue->value );
      }
   }
   return pValue;
}

static PHB_PP_TOKEN hb_pp_pragmaGetSwitch( PHB_PP_TOKEN pToken, int * piValue )
{
   PHB_PP_TOKEN pValue = NULL;

   if( pToken && HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_KEYWORD )
   {
      HB_BOOL fNum = pToken->len > 1 && HB_PP_ISDIGIT( pToken->value[ pToken->len - 1 ] );

      if( HB_PP_TOKEN_ISEOC( pToken->pNext ) )
      {
         if( fNum )
         {
            pValue = pToken;
            *piValue = pValue->value[ pToken->len - 1 ] - '0';
         }
      }
      else if( HB_PP_TOKEN_ISEOC( pToken->pNext->pNext ) && ! fNum )
      {
         if( HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_MINUS )
         {
            pValue = pToken;
            *piValue = 0;
         }
         else if( HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_PLUS )
         {
            pValue = pToken;
            *piValue = 1;
         }
         else if( HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_NUMBER )
         {
            pValue = pToken;
            *piValue = atoi( pValue->pNext->value );
         }
      }
   }
   return pValue;
}

static void hb_pp_pragmaNew( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   PHB_PP_TOKEN pValue = NULL;
   HB_BOOL fError = HB_FALSE, fValue = HB_FALSE;
   int iValue = 0;

   if( ! pToken )
      fError = HB_TRUE;
   else if( pToken->len == 1 && HB_ISOPTSEP( pToken->value[ 0 ] ) )
   {
      if( ! pState->iCondCompile )
      {
         pToken = pToken->pNext;
         pValue = hb_pp_pragmaGetSwitch( pToken, &iValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, pValue->value, iValue );
         else
            fError = HB_TRUE;
      }
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_KEYWORD )
   {
      if( hb_pp_tokenValueCmp( pToken, "begindump", HB_PP_CMP_DBASE ) )
      {
         pState->iStreamDump = HB_PP_STREAM_DUMP_C;
         pState->iDumpLine = pState->pFile->iCurrentLine;
         if( ! pState->pDumpBuffer )
            pState->pDumpBuffer = hb_membufNew();
      }
      else if( hb_pp_tokenValueCmp( pToken, "enddump", HB_PP_CMP_DBASE ) )
      {
         pState->iStreamDump = HB_PP_STREAM_OFF;
      }
      else if( hb_pp_tokenValueCmp( pToken, "__text", HB_PP_CMP_DBASE ) )
      {
         fError = hb_pp_pragmaStream( pState, pToken->pNext );
         if( ! fError )
            pState->iStreamDump = HB_PP_STREAM_CLIPPER;
      }
      else if( hb_pp_tokenValueCmp( pToken, "__stream", HB_PP_CMP_DBASE ) )
      {
         fError = hb_pp_pragmaStream( pState, pToken->pNext );
         if( ! fError )
         {
            pState->iStreamDump = HB_PP_STREAM_PRG;
            if( ! pState->pStreamBuffer )
               pState->pStreamBuffer = hb_membufNew();
         }
      }
      else if( hb_pp_tokenValueCmp( pToken, "__cstream", HB_PP_CMP_DBASE ) )
      {
         fError = hb_pp_pragmaStream( pState, pToken->pNext );
         if( ! fError )
         {
            pState->iStreamDump = HB_PP_STREAM_C;
            if( ! pState->pStreamBuffer )
               pState->pStreamBuffer = hb_membufNew();
         }
      }
      else if( hb_pp_tokenValueCmp( pToken, "__streaminclude", HB_PP_CMP_DBASE ) )
      {
         if( pToken->pNext && HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_STRING )
         {
            fError = hb_pp_pragmaStream( pState, pToken->pNext->pNext );
            if( ! fError && ! pState->iCondCompile )
            {
               pState->iStreamDump = HB_PP_STREAM_PRG;
               hb_pp_pragmaStreamFile( pState, pToken->pNext->value );
               pState->iStreamDump = HB_PP_STREAM_OFF;
            }
         }
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "__cstreaminclude", HB_PP_CMP_DBASE ) )
      {
         if( pToken->pNext && HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_STRING )
         {
            fError = hb_pp_pragmaStream( pState, pToken->pNext->pNext );
            if( ! fError && ! pState->iCondCompile )
            {
               pState->iStreamDump = HB_PP_STREAM_C;
               hb_pp_pragmaStreamFile( pState, pToken->pNext->value );
               pState->iStreamDump = HB_PP_STREAM_OFF;
            }
         }
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "__binarystreaminclude", HB_PP_CMP_DBASE ) )
      {
         if( pToken->pNext && HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_STRING )
         {
            fError = hb_pp_pragmaStream( pState, pToken->pNext->pNext );
            if( ! fError && ! pState->iCondCompile )
            {
               pState->iStreamDump = HB_PP_STREAM_BINARY;
               hb_pp_pragmaStreamFile( pState, pToken->pNext->value );
               pState->iStreamDump = HB_PP_STREAM_OFF;
            }
         }
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "__endtext", HB_PP_CMP_DBASE ) )
      {
         pState->iStreamDump = HB_PP_STREAM_OFF;
      }
      else if( pState->iCondCompile )
      {
         /* conditional compilation - other preprocessing and output disabled */
      }
      else if( hb_pp_tokenValueCmp( pToken, "AUTOMEMVAR", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &fValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "a", ( int ) fValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "DEBUGINFO", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &fValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "b", ( int ) fValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "DYNAMICMEMVAR", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &fValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "v", ( int ) fValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "ENABLEWARNINGS", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &fValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "w", fValue ? 1 : 0 );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "ESCAPEDSTRINGS", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &pState->fEscStr );
         fError = pValue == NULL;
      }
      else if( hb_pp_tokenValueCmp( pToken, "MULTILINESTRINGS", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &pState->fMultiLineStr );
         fError = pValue == NULL;
      }
      else if( hb_pp_tokenValueCmp( pToken, "EXITSEVERITY", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetInt( pToken->pNext, &iValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "es", iValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "LINENUMBER", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &fValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "l", fValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "NOSTARTPROC", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetInt( pToken->pNext, &iValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "n", iValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "OPERATOR", HB_PP_CMP_DBASE ) )
      {
         fError = hb_pp_pragmaOperatorNew( pState, pToken->pNext );
      }
      else if( hb_pp_tokenValueCmp( pToken, "PREPROCESSING", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &fValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "p", fValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "SHORTCUT", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &fValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "z", fValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "RECURSELEVEL", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetInt( pToken->pNext, &pState->iMaxCycles );
         fError = pValue == NULL;
      }
      /* xHarbour extension */
      else if( hb_pp_tokenValueCmp( pToken, "TEXTHIDDEN", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetInt( pToken->pNext, &iValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, pToken->value, iValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "TRACE", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &fValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "p+", fValue );
         else
            fError = HB_TRUE;
      }
      else if( hb_pp_tokenValueCmp( pToken, "TRACEPRAGMAS", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetLogical( pToken->pNext, &pState->fTracePragmas );
         fError = pValue == NULL;
      }
      else if( hb_pp_tokenValueCmp( pToken, "WARNINGLEVEL", HB_PP_CMP_DBASE ) )
      {
         pValue = hb_pp_pragmaGetInt( pToken->pNext, &iValue );
         if( pValue )
            fError = hb_pp_setCompilerSwitch( pState, "w", iValue );
         else
            fError = HB_TRUE;
      }
      else
         fError = HB_TRUE;
   }
   else
      fError = HB_TRUE;

   if( pState->iCondCompile )
   {
      ;
   }
   else if( fError )
   {
      hb_pp_error( pState, 'E', HB_PP_ERR_PRAGMA, NULL );
   }
   else if( pState->fTracePragmas || pState->fWriteTrace )
   {
      char szLine[ 12 ];

      hb_snprintf( szLine, sizeof( szLine ), "%d", pState->pFile->iCurrentLine );
      hb_membufFlush( pState->pBuffer );
      hb_membufAddCh( pState->pBuffer, '(' );
      hb_membufAddStr( pState->pBuffer, szLine );
      hb_membufAddStr( pState->pBuffer, ") #pragma " );
      hb_membufAddStr( pState->pBuffer, pToken->value );
      if( pValue && pValue != pToken )
      {
         hb_membufAddStr( pState->pBuffer, " set to '" );
         hb_membufAddStr( pState->pBuffer, pValue->value );
         hb_membufAddCh( pState->pBuffer, '\'' );
      }
      hb_membufAddCh( pState->pBuffer, '\n' );
      if( pState->fWriteTrace )
      {
         if( fwrite( hb_membufPtr( pState->pBuffer ), sizeof( char ),
                     hb_membufLen( pState->pBuffer ), pState->file_trace ) !=
             hb_membufLen( pState->pBuffer ) )
         {
            hb_pp_error( pState, 'F', HB_PP_ERR_WRITE_FILE, pState->szTraceFileName );
         }
      }
      if( pState->fTracePragmas )
      {
         hb_membufAddCh( pState->pBuffer, '\0' );
         hb_pp_disp( pState, hb_membufPtr( pState->pBuffer ) );
      }
   }
}

static void hb_pp_defineNew( PHB_PP_STATE pState, PHB_PP_TOKEN pToken, HB_BOOL fDirect )
{
   PHB_PP_TOKEN pMatch = pToken ? pToken->pNext : NULL;

   if( ! pMatch || HB_PP_TOKEN_TYPE( pMatch->type ) != HB_PP_TOKEN_KEYWORD )
   {
      hb_pp_error( pState, 'E', HB_PP_ERR_DEFINE_SYNTAX, NULL );
   }
   else
   {
      PHB_PP_TOKEN pResult, pLast = pMatch->pNext, pParam;
      PHB_PP_MARKER pMarkers = NULL;
      HB_USHORT usPCount = 0, usParam;

      /* pseudo function? */
      if( pLast && HB_PP_TOKEN_TYPE( pLast->type ) == HB_PP_TOKEN_LEFT_PB &&
          pLast->spaces == 0 )
      {
         HB_USHORT type = HB_PP_TOKEN_KEYWORD;
         for( ;; )
         {
            pLast = pLast->pNext;
            if( pLast && ( usPCount == 0 || type == HB_PP_TOKEN_COMMA ) &&
                HB_PP_TOKEN_TYPE( pLast->type ) == HB_PP_TOKEN_RIGHT_PB )
               break;
            if( ! pLast || type != HB_PP_TOKEN_TYPE( pLast->type ) )
            {
               if( type == HB_PP_TOKEN_KEYWORD )
                  hb_pp_error( pState, 'E', HB_PP_ERR_LABEL_MISSING_IN_DEFINE, NULL );
               else
                  hb_pp_error( pState, 'E', HB_PP_ERR_PARE_MISSING_IN_DEFINE, NULL );
               return;
            }
            else if( type == HB_PP_TOKEN_KEYWORD )
            {
               ++usPCount;
               type = HB_PP_TOKEN_COMMA;
            }
            else
               type = HB_PP_TOKEN_KEYWORD;
         }
      }
      else  /* simple keyword define */
         pLast = pMatch;
      pResult = pLast->pNext;
      pLast->pNext = NULL;
      pToken->pNext = hb_pp_tokenResultEnd( &pResult, fDirect );
      if( usPCount )
      {
         usPCount = 0;
         pParam = pMatch->pNext->pNext;
         while( HB_PP_TOKEN_TYPE( pParam->type ) == HB_PP_TOKEN_KEYWORD )
         {
            usParam = 0;
            /* Check if it's not repeated ID */
            pLast = pMatch->pNext->pNext;
            while( pLast != pParam && ! hb_pp_tokenEqual( pParam, pLast, HB_PP_CMP_CASE ) )
            {
               pLast = pLast->pNext;
            }
            if( pLast == pParam )
            {
               pLast = pResult;
               /* replace parameter tokens in result pattern with regular
                  result markers */
               while( pLast )
               {
                  if( hb_pp_tokenEqual( pParam, pLast, HB_PP_CMP_CASE ) )
                  {
                     HB_PP_TOKEN_SETTYPE( pLast, HB_PP_RMARKER_REGULAR );
                     if( usParam == 0 )
                        usParam = ++usPCount;
                     pLast->index = usParam;
                  }
                  pLast = pLast->pNext;
               }
            }
            HB_PP_TOKEN_SETTYPE( pParam, HB_PP_MMARKER_REGULAR );
            pParam->index = usParam;
            pParam = pParam->pNext;
            if( HB_PP_TOKEN_TYPE( pParam->type ) == HB_PP_TOKEN_COMMA )
               pParam = pParam->pNext;
         }
         if( usPCount )
         {
            /* create regular match and result markers from parameters */
            pMarkers = ( PHB_PP_MARKER ) hb_xgrabz( usPCount * sizeof( HB_PP_MARKER ) );
         }
      }
      hb_pp_defineAdd( pState, HB_PP_CMP_CASE, usPCount, pMarkers, pMatch, pResult );
      if( pState->fTrackPos )
         hb_pp_trackRule( pState, hb_pp_defineFind( pState, pMatch ), 'd' );
   }
}

static HB_BOOL hb_pp_tokenUnQuotedGet( PHB_PP_TOKEN ** pTokenPtr, HB_BOOL * pfQuoted,
                                       HB_BOOL fFree )
{
   PHB_PP_TOKEN pToken = **pTokenPtr;

   *pfQuoted = HB_FALSE;
   if( pToken )
   {
      if( fFree )
      {
         **pTokenPtr = pToken->pNext;
         hb_pp_tokenFree( pToken );
      }
      else
      {
         *pTokenPtr = &pToken->pNext;
      }
      pToken = **pTokenPtr;
      if( pToken )
      {
         if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_BACKSLASH )
         {
            *pfQuoted = HB_TRUE;
            if( pToken->pNext )
               pToken->pNext->spaces = pToken->spaces;
            **pTokenPtr = pToken->pNext;
            hb_pp_tokenFree( pToken );
            pToken = **pTokenPtr;
         }
      }
   }

   return pToken != NULL;
}

static HB_BOOL hb_pp_matchMarkerNew( PHB_PP_TOKEN * pTokenPtr,
                                     PHB_PP_MARKERLST * pMarkerListPtr )
{
   HB_USHORT type = HB_PP_TOKEN_NUL;
   PHB_PP_TOKEN pMarkerId = NULL, pMTokens = NULL;
   HB_BOOL fQuoted;

   /* At start pTokenPtr points to '<' token */

   if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted )
   {
      if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_KEYWORD )
      {
         pMarkerId = *pTokenPtr;
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) && ! fQuoted )
         {
            if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
               type = HB_PP_MMARKER_REGULAR;
            else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_COMMA )
            {
               int i = 3;
               do
               {
                  if( ! hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) || fQuoted )
                     break;
                  if( i == 3 && HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_EPSILON )
                  {
                     i = 0;
                     break;
                  }
                  if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) != HB_PP_TOKEN_DOT )
                     break;
               }
               while( --i > 0 );
               if( i == 0 && hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) &&
                   ! fQuoted && HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
                  type = HB_PP_MMARKER_LIST;
            }
            else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_SEND )
            {
               if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) )
               {
                  PHB_PP_TOKEN pLast = NULL;
                  do
                  {
                     if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT && ! fQuoted )
                     {
                        if( pLast )
                        {
                           pMTokens = pMarkerId->pNext;
                           pMarkerId->pNext = *pTokenPtr;
                           pTokenPtr = &pMarkerId->pNext;
                           pLast->pNext = NULL;
                        }
                        type = HB_PP_MMARKER_RESTRICT;
                        break;
                     }
                     pLast = *pTokenPtr;
                  }
                  while( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) );
               }
            }
         }
      }
      else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_MULT )
      {
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
             HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_KEYWORD )
         {
            pMarkerId = *pTokenPtr;
            if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_MULT &&
                hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
               type = HB_PP_MMARKER_WILD;
         }
      }
      else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_LEFT_PB )
      {
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
             HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_KEYWORD )
         {
            pMarkerId = *pTokenPtr;
            if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_RIGHT_PB &&
                hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
               type = HB_PP_MMARKER_EXTEXP;
         }
      }
      else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_NOT )
      {
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
             HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_KEYWORD )
         {
            pMarkerId = *pTokenPtr;
            if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_NOT &&
                hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
               type = HB_PP_MMARKER_NAME;
         }
      }
   }

   if( type != HB_PP_TOKEN_NUL )
   {
      PHB_PP_MARKERLST pMrkLst = *pMarkerListPtr, pMrkPrev = NULL;
      PHB_PP_MARKERPTR pMrkPtr;

      while( pMrkLst && ! hb_pp_tokenEqual( pMrkLst->pMatchMarkers->pToken,
                                            pMarkerId, HB_PP_CMP_CASE ) )
      {
         pMrkPrev = pMrkLst;
         pMrkLst = pMrkLst->pNext;
      }
      if( ! pMrkLst )
      {
         pMrkLst = ( PHB_PP_MARKERLST ) hb_xgrab( sizeof( HB_PP_MARKERLST ) );
         if( pMrkPrev )
            pMrkPrev->pNext = pMrkLst;
         else
            *pMarkerListPtr = pMrkLst;
         pMrkLst->pNext = NULL;
         pMrkLst->pMatchMarkers = NULL;
         pMrkLst->canrepeat = HB_TRUE;
         pMrkLst->index = 0;
      }
      pMrkPtr = ( PHB_PP_MARKERPTR ) hb_xgrab( sizeof( HB_PP_MARKERPTR ) );
      pMrkPtr->pNext = pMrkLst->pMatchMarkers;
      pMrkLst->pMatchMarkers = pMrkPtr;
      pMrkPtr->pToken = pMarkerId;
      pMrkPtr->pMTokens = pMTokens;
      pMrkPtr->type = type;
      /* mark non restricted markers for later detection two consecutive
         optional match markers */
      if( type != HB_PP_MMARKER_RESTRICT )
         pMarkerId->type |= HB_PP_TOKEN_MATCHMARKER;
      /* free the trailing '>' marker token */
      pMTokens = *pTokenPtr;
      *pTokenPtr = pMTokens->pNext;
      hb_pp_tokenFree( pMTokens );
      return HB_TRUE;
   }
   return HB_FALSE;
}

static HB_BOOL hb_pp_matchHasKeywords( PHB_PP_TOKEN pToken )
{
   /* Now we are strictly Clipper compatible here though the nested
      optional markers which have keywords on deeper levels are not
      recognized. Exactly the same makes Clipper PP */
   while( HB_PP_TOKEN_ISMATCH( pToken ) )
      pToken = pToken->pNext;
   return pToken != NULL;
}

static HB_BOOL hb_pp_matchPatternNew( PHB_PP_STATE pState, PHB_PP_TOKEN * pTokenPtr,
                                      PHB_PP_MARKERLST * pMarkerListPtr,
                                      PHB_PP_TOKEN ** pOptional )
{
   PHB_PP_TOKEN * pLastPtr = NULL;
   HB_BOOL fQuoted = HB_FALSE;

   if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_BACKSLASH )
   {
      PHB_PP_TOKEN pToken = *pTokenPtr;
      *pTokenPtr = pToken->pNext;
      hb_pp_tokenFree( pToken );
      fQuoted = HB_TRUE;
   }

   do
   {
      if( ! fQuoted )
      {
         if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_LT )
         {
            if( ! hb_pp_matchMarkerNew( pTokenPtr, pMarkerListPtr ) )
            {
               hb_pp_error( pState, 'E', HB_PP_ERR_BAD_MATCH_MARKER, NULL );
               return HB_FALSE;
            }
            /* now pTokenPtr points to marker keyword, all other tokens
               have been stripped */
         }
         else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_RIGHT_SB )
         {
            if( pOptional )
            {
               *pOptional = pTokenPtr;
               return HB_TRUE;
            }
         }
         else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_LEFT_SB )
         {
            PHB_PP_TOKEN * pStopOptPtr = NULL;
            if( ! ( *pTokenPtr )->pNext )
            {
               /* assign pOptional only to force error below */
               pOptional = &pTokenPtr;
               break;
            }
            else if( ! hb_pp_matchPatternNew( pState, &( *pTokenPtr )->pNext,
                                              pMarkerListPtr, &pStopOptPtr ) )
               return HB_FALSE;
            else if( *pStopOptPtr == ( *pTokenPtr )->pNext )
            {
               hb_pp_error( pState, 'E', HB_PP_ERR_EMPTY_OPTIONAL, NULL );
               return HB_FALSE;
            }
            else
            {
               PHB_PP_TOKEN pToken, pOptTok = ( *pTokenPtr )->pNext;
               pToken = *pStopOptPtr;
               *pStopOptPtr = NULL;
               ( *pTokenPtr )->pNext = pToken->pNext;
               hb_pp_tokenFree( pToken );
               /* create new optional match marker */
               HB_PP_TOKEN_SETTYPE( *pTokenPtr, HB_PP_MMARKER_OPTIONAL );
               if( ( *pTokenPtr )->spaces > 1 )
                  ( *pTokenPtr )->spaces = 1;
               ( *pTokenPtr )->type |= HB_PP_TOKEN_MATCHMARKER;
               ( *pTokenPtr )->pMTokens = pOptTok;
               if( pLastPtr && ! hb_pp_matchHasKeywords( *pLastPtr ) )
               {
                  if( ! hb_pp_matchHasKeywords( pOptTok ) )
                  {
                     hb_pp_error( pState, 'E', HB_PP_ERR_AMBIGUOUS_MATCH_PATTERN, NULL );
                     return HB_FALSE;
                  }
                  /* replace the order for these optional tokens to keep
                     the ones with keywords 1st */
                  ( *pTokenPtr )->pMTokens = *pLastPtr;
                  *pLastPtr = pOptTok;
               }
               pLastPtr = &( *pTokenPtr )->pMTokens;
               /* to skip resetting pLastPtr below */
               continue;
            }
         }
      }
      pLastPtr = NULL;
   }
   while( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) );

   if( pOptional )
   {
      hb_pp_error( pState, 'E', HB_PP_ERR_UNCLOSED_OPTIONAL, NULL );
      return HB_FALSE;
   }

   return HB_TRUE;
}

static HB_BOOL hb_pp_resultMarkerNew( PHB_PP_STATE pState,
                                      PHB_PP_TOKEN * pTokenPtr,
                                      PHB_PP_MARKERLST * pMarkerListPtr,
                                      HB_BOOL fDump, HB_BOOL fOptional,
                                      HB_USHORT * pusPCount, HB_SIZE spaces )
{
   HB_USHORT type = HB_PP_TOKEN_NUL, rtype;
   PHB_PP_TOKEN pMarkerId = NULL, pToken;
   HB_BOOL fQuoted;

   /* At start pTokenPtr points to '<' token */
   if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted )
   {
      rtype = HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type );
      if( rtype == HB_PP_TOKEN_KEYWORD || rtype == HB_PP_TOKEN_STRING )
      {
         pMarkerId = *pTokenPtr;
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) && ! fQuoted &&
             HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
         {
            if( rtype == HB_PP_TOKEN_STRING )
            {
               type = HB_PP_RMARKER_STRSTD;
               HB_PP_TOKEN_SETTYPE( pMarkerId, HB_PP_TOKEN_KEYWORD );
            }
            else
               type = fDump ? HB_PP_RMARKER_STRDUMP : HB_PP_RMARKER_REGULAR;
         }
      }
      else if( rtype == HB_PP_TOKEN_LEFT_PB )
      {
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
             HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_KEYWORD )
         {
            pMarkerId = *pTokenPtr;
            if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_RIGHT_PB &&
                hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
               type = HB_PP_RMARKER_STRSMART;
         }
      }
      else if( rtype == HB_PP_TOKEN_LEFT_CB )
      {
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
             HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_KEYWORD )
         {
            pMarkerId = *pTokenPtr;
            if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_RIGHT_CB &&
                hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
               type = HB_PP_RMARKER_BLOCK;
         }
      }
      else if( rtype == HB_PP_TOKEN_DOT )
      {
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
             HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_KEYWORD )
         {
            pMarkerId = *pTokenPtr;
            if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_DOT &&
                hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
                HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
               type = HB_PP_RMARKER_LOGICAL;
         }
      }
      else if( rtype == HB_PP_TOKEN_MINUS )
      {
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
             HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_KEYWORD )
         {
            pMarkerId = *pTokenPtr;
            if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) && ! fQuoted )
            {
               /* <-id-> was bad choice for marker type because -> is single
                  ALIAS token so we have to add workaround for it now */
               if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_ALIAS ||
                   ( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_MINUS &&
                     hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
                     HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT ) )
                  type = HB_PP_RMARKER_NUL;
            }
         }
      }
      else if( rtype == HB_PP_TOKEN_REFERENCE )
      {
         /* <@> */
         if( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_TRUE ) && ! fQuoted &&
             HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_GT )
            type = HB_PP_RMARKER_REFERENCE;
      }
   }

   if( type == HB_PP_TOKEN_NUL )
   {
      hb_pp_error( pState, 'E', HB_PP_ERR_WRONG_LABEL, NULL );
   }
   else if( type == HB_PP_RMARKER_REFERENCE )
   {
      hb_pp_tokenSetValue( *pTokenPtr, "~", 1 );
      HB_PP_TOKEN_SETTYPE( *pTokenPtr, type );
      return HB_TRUE;
   }
   else
   {
      PHB_PP_MARKERLST pMrkLst = *pMarkerListPtr;

      while( pMrkLst && ! hb_pp_tokenEqual( pMrkLst->pMatchMarkers->pToken,
                                            pMarkerId, HB_PP_CMP_CASE ) )
      {
         pMrkLst = pMrkLst->pNext;
      }

      if( ! pMrkLst )
      {
         hb_pp_error( pState, 'E', HB_PP_ERR_UNKNOWN_RESULT_MARKER, NULL );
      }
      else
      {
         if( ! pMrkLst->index )
            pMrkLst->index = ++( *pusPCount );
         if( ! fOptional )
            pMrkLst->canrepeat = HB_FALSE;
         HB_PP_TOKEN_SETTYPE( pMarkerId, type );
         pMarkerId->index = pMrkLst->index;
         pMarkerId->spaces = spaces;
         /* free the trailing '>' marker token */
         pToken = *pTokenPtr;
         *pTokenPtr = pToken->pNext;
         hb_pp_tokenFree( pToken );
         return HB_TRUE;
      }
   }
   return HB_FALSE;
}

static HB_BOOL hb_pp_patternCompare( PHB_PP_TOKEN pToken1, PHB_PP_TOKEN pToken2 )
{
   while( pToken1 && pToken2 )
   {
      if( ! hb_pp_tokenEqual( pToken1, pToken2, HB_PP_CMP_STD ) )
         break;
      if( HB_PP_TOKEN_TYPE( pToken1->type ) == HB_PP_MMARKER_RESTRICT ||
          HB_PP_TOKEN_TYPE( pToken1->type ) == HB_PP_MMARKER_OPTIONAL ||
          HB_PP_TOKEN_TYPE( pToken1->type ) == HB_PP_RMARKER_OPTIONAL )
      {
         if( ! hb_pp_patternCompare( pToken1->pMTokens, pToken2->pMTokens ) )
            break;
      }
      pToken1 = pToken1->pNext;
      pToken2 = pToken2->pNext;
   }
   return ! pToken1 && ! pToken2;
}

/* ast-16: piDelOf (when tracking) receives the record index of the rule that
   was removed, or stays -1 when this #un... matched no rule at all - an ORPHAN
   removing directive, which is silent dead code today */
static void hb_pp_directiveDel( PHB_PP_STATE pState, PHB_PP_TOKEN pMatch,
                                HB_USHORT markers, PHB_PP_MARKER pMarkers,
                                HB_USHORT mode, HB_BOOL fCommand,
                                int * piDelOf )
{
   PHB_PP_RULE pRule, * pRulePtr = fCommand ? &pState->pCommands :
                                              &pState->pTranslations;

   while( *pRulePtr )
   {
      pRule = *pRulePtr;
      if( HB_PP_CMP_MODE( pRule->mode ) == mode && pRule->markers == markers )
      {
         HB_USHORT u;
         for( u = 0; u < markers; ++u )
         {
            if( pRule->pMarkers[ u ].canrepeat != pMarkers[ u ].canrepeat )
               break;
         }
         if( u == markers && hb_pp_patternCompare( pRule->pMatch, pMatch ) )
         {
            /* while the rule is still alive: say WHICH one is going away */
            if( piDelOf )
               *piDelOf = hb_pp_trackRuleRemoved( pState, pRule );
            *pRulePtr = pRule->pPrev;
            hb_pp_ruleFree( pRule );
            if( fCommand )
               pState->iCommands--;
            else
               pState->iTranslations--;
            return;
         }
      }
      pRulePtr = &pRule->pPrev;
   }
}

static void hb_pp_directiveNew( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                                HB_USHORT mode, HB_BOOL fCommand, HB_BOOL fDirect,
                                HB_BOOL fDelete )
{
   PHB_PP_TOKEN pResult, pMatch, pStart, pLast;
   HB_BOOL fValid = HB_FALSE;

#ifdef HB_CLP_STRICT
   HB_SYMBOL_UNUSED( fDirect );
#endif

   pMatch = pResult = pLast = NULL;
   if( pToken->pNext )
   {
      pStart = pToken->pNext;
      while( ! HB_PP_TOKEN_ISEOP( pStart, fDirect ) )
      {
         if( pMatch )
         {
            /* Clipper PP makes something like that for result pattern of
             #[x]translate and #[x]command */
            if( pStart->spaces > 1 )
               pStart->spaces = 1;
         }
         else if( pStart->pNext &&
                  HB_PP_TOKEN_TYPE( pStart->type ) == HB_PP_TOKEN_EQ &&
                  HB_PP_TOKEN_TYPE( pStart->pNext->type ) == HB_PP_TOKEN_GT )
         {
            fValid = HB_TRUE;
            if( ! pLast )
               break;

            pLast->pNext = NULL;
            pMatch = pToken->pNext;
            pToken->pNext = pStart;
            pToken = pStart = pStart->pNext;
         }
         pLast = pStart;
         pStart = pStart->pNext;
      }
      if( pMatch && pLast != pToken )
      {
         pLast->pNext = NULL;
         pResult = pToken->pNext;
         pToken->pNext = pStart;
      }
   }

   if( ! fValid )
   {
      hb_pp_error( pState, 'E', HB_PP_ERR_MISSING_PATTERN_SEP, NULL );
   }
   else if( pMatch ) /* isn't dummy directive? */
   {
      PHB_PP_MARKERLST pMarkerList = NULL, pMrkLst;
      PHB_PP_MARKERPTR pMrkPtr;
      PHB_PP_MARKER pMarkers = NULL;
      HB_USHORT usPCount = 0;

      fValid = hb_pp_matchPatternNew( pState, &pMatch, &pMarkerList, NULL );
      if( fValid )
      {
         if( pResult )
         {
            PHB_PP_TOKEN * pTokenPtr, * pDumpPtr = NULL, * pOptStart = NULL;
            HB_BOOL fQuoted = HB_FALSE;

            if( HB_PP_TOKEN_TYPE( pResult->type ) == HB_PP_TOKEN_BACKSLASH )
            {
               fQuoted = HB_TRUE;
               pLast = pResult;
               pResult = pResult->pNext;
               hb_pp_tokenFree( pLast );
            }
            pTokenPtr = &pResult;
            do
            {
               if( ! fQuoted )
               {
                  if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_HASH )
                  {
                     pDumpPtr = pTokenPtr;
                     /* to skip pDumpPtr reseting below */
                     continue;
                  }
                  else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_LT )
                  {
                     HB_SIZE spaces = ( *pTokenPtr )->spaces;
                     /* Free the string dump token: '#'. Clipper PP always
                        does it without checking type of next marker */
                     if( pDumpPtr )
                     {
                        pLast = *pDumpPtr;
                        spaces = pLast->spaces;
                        *pDumpPtr = pLast->pNext;
                        hb_pp_tokenFree( pLast );
                        pTokenPtr = pDumpPtr;
                     }

                     if( ! hb_pp_resultMarkerNew( pState, pTokenPtr, &pMarkerList,
                                                  pDumpPtr != NULL, pOptStart != NULL,
                                                  &usPCount, spaces ) )
                     {
                        fValid = HB_FALSE;
                        break;
                     }
                     /* now pTokenPtr points to marker keyword, all other tokens
                        have been stripped */
                  }
                  else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_LEFT_SB )
                  {
                     if( pOptStart )
                     {
                        fValid = HB_FALSE;
                        hb_pp_error( pState, 'E', HB_PP_ERR_NESTED_OPTIONAL, NULL );
                        break;
                     }
                     pOptStart = pTokenPtr;
                  }
                  else if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_RIGHT_SB && pOptStart )
                  {
                     pLast      = *pTokenPtr;
                     *pTokenPtr = NULL;
                     ( *pOptStart )->pMTokens = ( *pOptStart )->pNext;
                     ( *pOptStart )->pNext    = pLast->pNext;
                     HB_PP_TOKEN_SETTYPE( *pOptStart, HB_PP_RMARKER_OPTIONAL );
#ifndef HB_CLP_STRICT
                     /* This is not Clipper compatible but we have word
                        concatenation and without this modification we
                        will introduce very serious bug */
                     if( ( *pOptStart )->pMTokens &&
                         ( *pOptStart )->pMTokens->spaces == 0 &&
                         ( *pOptStart )->spaces > 0 &&
                         HB_PP_TOKEN_TYPE( ( *pOptStart )->pMTokens->type ) !=
                                                            HB_PP_TOKEN_COMMA )
                        ( *pOptStart )->pMTokens->spaces = 1;
#endif
                     pTokenPtr = pOptStart;
                     pOptStart = NULL;
                     hb_pp_tokenFree( pLast );
                  }
               }
               /* reset pDumpPtr */
               pDumpPtr = NULL;
            }
            while( hb_pp_tokenUnQuotedGet( &pTokenPtr, &fQuoted, HB_FALSE ) );

            if( fValid && pOptStart )
            {
               fValid = HB_FALSE;
               hb_pp_error( pState, 'E', HB_PP_ERR_UNKNOWN_RESULT_MARKER, NULL );
            }
         }
      }

      /* AST dump (gated by fTrackPos): give an index to EVERY match marker,
         including the ones the result never references. Without an index the
         pp matches such a marker and then DISCARDS the binding - see
         hb_pp_patternMatch(), which only calls hb_pp_patternAddResult() when
         pMatch->index is set. The consequence for a consumer of the tracking
         tables: the fill of an unused marker reaches hb_pp_trackApply() with
         marker 0, i.e. indistinguishable from a literal word of the rule
         itself (the comment there assumed exactly that, and it is false).
         Indexing them makes the binding a FACT instead of something a consumer
         has to guess from the token text. The expansion is unaffected: the
         result never references these markers, so nothing new is stuffed - the
         only cost is one marker slot and the matched span being remembered.
         Gated, so a default build of Harbour is byte-for-byte untouched. */
      if( fValid && pState->fTrackPos )
      {
         pMrkLst = pMarkerList;
         while( pMrkLst )
         {
            if( pMrkLst->index == 0 && pMrkLst->pMatchMarkers )
               pMrkLst->index = ++usPCount;
            pMrkLst = pMrkLst->pNext;
         }
      }

      if( fValid && usPCount )
      {
         /* create regular match and result markers from parameters */
         pMarkers = ( PHB_PP_MARKER ) hb_xgrabz( usPCount * sizeof( HB_PP_MARKER ) );
      }

      /* free marker index list */
      while( pMarkerList )
      {
         pMrkLst = pMarkerList;
         while( pMrkLst->pMatchMarkers )
         {
            pMrkPtr = pMrkLst->pMatchMarkers;
            pMrkLst->pMatchMarkers = pMrkPtr->pNext;
            /* set match token type and parameters */
            if( pMarkers && pMrkLst->index )
            {
               pMarkers[ pMrkLst->index - 1 ].canrepeat = pMrkLst->canrepeat;
               pMrkPtr->pToken->index = pMrkLst->index;
            }
            pMrkPtr->pToken->pMTokens = pMrkPtr->pMTokens;
            HB_PP_TOKEN_SETTYPE( pMrkPtr->pToken, pMrkPtr->type );
            hb_xfree( pMrkPtr );
         }
         pMarkerList = pMarkerList->pNext;
         hb_xfree( pMrkLst );
      }

      if( fValid )
      {
         if( fDelete )
         {
            int iDelOf = -1;

            hb_pp_directiveDel( pState, pMatch, usPCount, pMarkers, mode,
                                fCommand, pState->fTrackPos ? &iDelOf : NULL );
            /* ast-16: the removing directive itself is a FACT - record it while
               its pattern tokens (and their source positions) are still alive;
               they are freed a few lines below.  Without this the pp removes a
               rule and leaves no trace: a consumer cannot see that the rule's
               lifetime ends here, nor edit this directive when renaming the
               rule it names. */
            if( pState->fTrackPos )
               hb_pp_trackRuleRec( pState, NULL, pMatch, pResult, mode,
                                   usPCount, fCommand ? 'c' : 't',
                                   HB_TRUE, iDelOf,
                                   pState->pFile ? pState->pFile->szFileName : NULL,
                                   pState->pFile ? pState->pFile->iCurrentLine : 0,
                                   NULL );
            if( pMarkers )
               hb_xfree( pMarkers );
         }
         else
         {
            PHB_PP_RULE pRule;
            pRule = hb_pp_ruleNew( pMatch, pResult, mode, usPCount, pMarkers );
            if( fCommand )
            {
               pRule->pPrev = pState->pCommands;
               pState->pCommands = pRule;
               pState->iCommands++;
               hb_pp_ruleSetId( pState, pMatch, HB_PP_COMMAND );
            }
            else
            {
               pRule->pPrev = pState->pTranslations;
               pState->pTranslations = pRule;
               pState->iTranslations++;
               hb_pp_ruleSetId( pState, pMatch, HB_PP_TRANSLATE );
            }
            if( pState->fTrackPos )
               hb_pp_trackRule( pState, pRule, fCommand ? 'c' : 't' );
            pMatch = pResult = NULL;
         }
      }
   }
   hb_pp_tokenListFree( &pMatch );
   hb_pp_tokenListFree( &pResult );
}

static HB_BOOL hb_pp_tokenStartExtBlock( PHB_PP_TOKEN * pTokenPtr )
{
   PHB_PP_TOKEN pToken = *pTokenPtr;

   if( pToken && HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LEFT_CB &&
       pToken->pNext && HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_PIPE )
   {
      HB_USHORT prevtype = HB_PP_TOKEN_COMMA;
      pToken = pToken->pNext->pNext;
      while( pToken )
      {
         HB_USHORT type = HB_PP_TOKEN_TYPE( pToken->type );
         if( ( ( type == HB_PP_TOKEN_KEYWORD || type == HB_PP_TOKEN_EPSILON ) &&
               prevtype == HB_PP_TOKEN_COMMA ) ||
             ( type == HB_PP_TOKEN_COMMA && prevtype == HB_PP_TOKEN_KEYWORD ) )
         {
            prevtype = type;
            pToken = pToken->pNext;
         }
         else
            break;
      }
      if( pToken && pToken->pNext && HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_PIPE &&
          HB_PP_TOKEN_ISEOC( pToken->pNext ) )
      {
         *pTokenPtr = pToken->pNext;
         return HB_TRUE;
      }
   }
   return HB_FALSE;
}

static HB_BOOL hb_pp_tokenStopExtBlock( PHB_PP_TOKEN * pTokenPtr )
{
   PHB_PP_TOKEN pToken = *pTokenPtr;

   if( HB_PP_TOKEN_ISEOC( pToken ) && pToken->pNext )
   {
      pToken = pToken->pNext;
      if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_RIGHT_CB )
      {
         *pTokenPtr = pToken->pNext;
         return HB_TRUE;
      }
      if( pToken->pNext &&
          HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_KEYWORD &&
          HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_KEYWORD )
      {
         PHB_PP_TOKEN pFirst = pToken;

         if( hb_pp_tokenValueCmp( pToken, "INIT", HB_PP_CMP_DBASE ) ||
             hb_pp_tokenValueCmp( pToken, "EXIT", HB_PP_CMP_DBASE ) ||
             hb_pp_tokenValueCmp( pToken, "STATIC", HB_PP_CMP_DBASE ) )
            pToken = pToken->pNext;

         if( hb_pp_tokenValueCmp( pToken, "FUNCTION", HB_PP_CMP_DBASE ) ||
             hb_pp_tokenValueCmp( pToken, "PROCEDURE", HB_PP_CMP_DBASE ) )
         {
            if( pToken != pFirst ||
                HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_KEYWORD )

            *pTokenPtr = pFirst;
            return HB_TRUE;
         }
      }
   }
   return HB_FALSE;
}

static HB_BOOL hb_pp_tokenSkipExp( PHB_PP_TOKEN * pTokenPtr, PHB_PP_TOKEN pStop,
                                   HB_USHORT mode, HB_BOOL * pfStop )
{
   HB_USHORT curtype, prevtype = 0, lbrtype = 0, rbrtype = 0;
   PHB_PP_TOKEN pToken = *pTokenPtr, pPrev;
   int iBraces = 0;
   HB_BOOL fMatch;

   if( pfStop )
      *pfStop = HB_FALSE;

   for( ;; )
   {
      pPrev = pToken;
      if( hb_pp_tokenStartExtBlock( &pToken ) )
      {
         int iExtBlock = 1;
         while( pToken )
         {
            if( hb_pp_tokenStartExtBlock( &pToken ) )
               iExtBlock++;
            else if( hb_pp_tokenStopExtBlock( &pToken ) )
            {
               if( --iExtBlock == 0 )
                  break;
            }
            else
               pToken = pToken->pNext;
         }
         if( iExtBlock )
            pToken = pPrev;
      }

      if( mode == HB_PP_CMP_ADDR ? pToken == pStop :
                                   HB_PP_TOKEN_ISEOC( pToken ) )
      {
         if( pfStop )
            *pfStop = HB_TRUE;
         break;
      }
      curtype = HB_PP_TOKEN_TYPE( pToken->type );
      if( iBraces )
      {
         if( curtype == lbrtype )
            ++iBraces;
         else if( curtype == rbrtype )
            --iBraces;
      }
      else if( curtype == HB_PP_TOKEN_COMMA )
      {
         if( pfStop )
         {
            if( mode != HB_PP_CMP_ADDR && HB_PP_TOKEN_NEEDRIGHT( prevtype ) )
               *pfStop = HB_TRUE;
            else
               pToken = pToken->pNext;
         }
         break;
      }
      else if( mode != HB_PP_CMP_ADDR &&
               ( HB_PP_TOKEN_CLOSE_BR( curtype ) ||
                 ( ! HB_PP_TOKEN_CANJOIN( curtype ) &&
                   ! HB_PP_TOKEN_CANJOIN( prevtype ) ) ||
                 ( HB_PP_TOKEN_NEEDRIGHT( prevtype ) &&
                   ! HB_PP_TOKEN_ISEXPTOKEN( pToken ) ) ||
                 ( pStop && hb_pp_tokenEqual( pToken, pStop, mode ) ) ) )
      {
         if( pfStop )
            *pfStop = HB_TRUE;
         break;
      }
      else if( HB_PP_TOKEN_OPEN_BR( curtype ) )
      {
         lbrtype = curtype;
         rbrtype = ( curtype == HB_PP_TOKEN_LEFT_PB ? HB_PP_TOKEN_RIGHT_PB :
                   ( curtype == HB_PP_TOKEN_LEFT_SB ? HB_PP_TOKEN_RIGHT_SB :
                                                      HB_PP_TOKEN_RIGHT_CB ) );
         ++iBraces;
      }
      if( ! HB_PP_TOKEN_ISNEUTRAL( curtype ) )
         prevtype = curtype;
      pToken = pToken->pNext;
   }

   fMatch = pToken != *pTokenPtr;
   *pTokenPtr = pToken;

   return fMatch;
}

static HB_BOOL hb_pp_tokenCanStartExp( PHB_PP_TOKEN pToken )
{
   if( ! HB_PP_TOKEN_NEEDLEFT( pToken ) && ! HB_PP_TOKEN_ISEOC( pToken ) )
   {
      if( HB_PP_TOKEN_TYPE( pToken->type ) != HB_PP_TOKEN_LEFT_SB )
         return HB_TRUE;
      else
      {
         PHB_PP_TOKEN pEoc = NULL;

         pToken = pToken->pNext;
         while( ! HB_PP_TOKEN_ISEOL( pToken ) )
         {
            if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_RIGHT_SB )
            {
               if( pEoc )
               {
                  do
                  {
                     if( HB_PP_TOKEN_TYPE( pEoc->type ) == HB_PP_TOKEN_EOC )
                        HB_PP_TOKEN_SETTYPE( pEoc, HB_PP_TOKEN_TEXT );
                     pEoc = pEoc->pNext;
                  }
                  while( pEoc != pToken );
               }
               return HB_TRUE;
            }
            if( ! pEoc && HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_EOC )
               pEoc = pToken;
            pToken = pToken->pNext;
         }
      }
   }
   return HB_FALSE;
}

static HB_BOOL hb_pp_tokenMatch( PHB_PP_TOKEN pMatch, PHB_PP_TOKEN * pTokenPtr,
                                 PHB_PP_TOKEN pStop, HB_USHORT mode )
{
   HB_BOOL fMatch = HB_FALSE;
   HB_USHORT type;

   type = HB_PP_TOKEN_TYPE( pMatch->type );
   if( type == HB_PP_MMARKER_REGULAR )
   {
      if( hb_pp_tokenCanStartExp( *pTokenPtr ) )
      {
         if( ! pStop )
            pStop = pMatch->pNext;
         fMatch = hb_pp_tokenSkipExp( pTokenPtr, pStop, mode, NULL );
      }
   }
   else if( type == HB_PP_MMARKER_LIST )
   {
      if( hb_pp_tokenCanStartExp( *pTokenPtr ) )
      {
         HB_BOOL fStop = HB_FALSE;
         if( ! pStop )
            pStop = pMatch->pNext;
         do
         {
            if( ! hb_pp_tokenSkipExp( pTokenPtr, pStop, mode, &fStop ) )
               break;
            fMatch = HB_TRUE;
         }
         while( ! fStop );
      }
   }
   else if( type == HB_PP_MMARKER_RESTRICT )
   {
      PHB_PP_TOKEN pRestrict = pMatch->pMTokens, pToken = *pTokenPtr;

      /*
       * Here we are strictly Clipper compatible. Clipper accepts dummy
       * restrict marker which starts from comma, <id: ,[ something,...]>
       * which always match empty expression. The same effect can be
       * reached by giving ,, in the world list on other positions.
       */
      while( pRestrict )
      {
         if( HB_PP_TOKEN_TYPE( pRestrict->type ) == HB_PP_TOKEN_COMMA )
         {
            *pTokenPtr = pToken;
            fMatch = HB_TRUE;
            break;
         }
         else if( HB_PP_TOKEN_TYPE( pRestrict->type ) == HB_PP_TOKEN_AMPERSAND &&
                  ( ! pRestrict->pNext ||
                    HB_PP_TOKEN_TYPE( pRestrict->pNext->type ) == HB_PP_TOKEN_COMMA ) &&
                  ( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_MACROVAR ||
                    HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_MACROTEXT ||
                    ( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_AMPERSAND &&
                      pToken->pNext &&
                      HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_LEFT_PB ) ) )
         {
            if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_MACROVAR ||
                HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_MACROTEXT )
            {
               *pTokenPtr = pToken->pNext;
            }
            else
            {
               int iBraces = 1;
               pToken = pToken->pNext->pNext;
               while( iBraces > 0 && ! HB_PP_TOKEN_ISEOC( pToken ) )
               {
                  if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LEFT_PB )
                     ++iBraces;
                  else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_RIGHT_PB )
                     --iBraces;
                  pToken = pToken->pNext;
               }
               *pTokenPtr = pToken;
            }
            fMatch = HB_TRUE;
            break;
         }
         else if( ! HB_PP_TOKEN_ISEOC( pToken ) &&
                  hb_pp_tokenEqual( pToken, pRestrict, mode ) )
         {
            pToken = pToken->pNext;
            pRestrict = pRestrict->pNext;
            if( ! pRestrict )
            {
               *pTokenPtr = pToken;
               fMatch = HB_TRUE;
               break;
            }
         }
         else
         {
            pToken = *pTokenPtr;
            do
            {
               type = HB_PP_TOKEN_TYPE( pRestrict->type );
               pRestrict = pRestrict->pNext;
            }
            while( pRestrict && type != HB_PP_TOKEN_COMMA );
         }
      }
   }
   else if( type == HB_PP_MMARKER_WILD )
   {
      /* TODO? now we are strictly Clipper compatible, but we may
         want to add some additional stop markers in the future here
         to support wild match markers also as not the last expression */
      if( ! HB_PP_TOKEN_ISEOS( *pTokenPtr ) )
      {
         fMatch = HB_TRUE;
         do
         {
            *pTokenPtr = ( *pTokenPtr )->pNext;
         }
         while( ! HB_PP_TOKEN_ISEOS( *pTokenPtr ) );
      }
   }
   else if( type == HB_PP_MMARKER_EXTEXP )
   {
      if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) != HB_PP_TOKEN_RIGHT_PB &&
          HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) != HB_PP_TOKEN_RIGHT_SB &&
          HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) != HB_PP_TOKEN_COMMA &&
          hb_pp_tokenCanStartExp( *pTokenPtr ) )
      {
         if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_LEFT_PB )
         {
            if( ! pStop )
               pStop = pMatch->pNext;
            fMatch = hb_pp_tokenSkipExp( pTokenPtr, pStop, mode, NULL );
         }
         else
         {
            do
            {
               *pTokenPtr = ( *pTokenPtr )->pNext;
            }
            while( ! HB_PP_TOKEN_ISEOC( *pTokenPtr ) &&
                   ( *pTokenPtr )->spaces == 0 &&
                   HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) != HB_PP_TOKEN_COMMA );

            fMatch = HB_TRUE;
         }
      }
   }
   else if( type == HB_PP_MMARKER_NAME )
   {
      if( HB_PP_TOKEN_TYPE( ( *pTokenPtr )->type ) == HB_PP_TOKEN_KEYWORD )
      {
         *pTokenPtr = ( *pTokenPtr )->pNext;
         fMatch = HB_TRUE;
      }
   }
   else if( hb_pp_tokenEqual( *pTokenPtr, pMatch, mode ) )
   {
      *pTokenPtr = ( *pTokenPtr )->pNext;
      fMatch = HB_TRUE;
   }

   return fMatch;
}

static HB_BOOL hb_pp_patternMatch( PHB_PP_STATE pState,
                                   PHB_PP_TOKEN pMatch, PHB_PP_TOKEN * pTokenPtr,
                                   PHB_PP_TOKEN pStop,
                                   HB_USHORT mode, PHB_PP_RULE pRule )
{
   PHB_PP_TOKEN pToken = *pTokenPtr;
   PHB_PP_TOKEN pFirst;
   HB_BOOL fOverflow = HB_FALSE;

   while( pMatch && ! HB_PP_TOKEN_ISEOS( pToken ) )
   {
      if( HB_PP_TOKEN_TYPE( pMatch->type ) == HB_PP_MMARKER_OPTIONAL )
      {
         PHB_PP_TOKEN pOptional = pMatch, pLast, pNewStop = pMatch->pNext;

         while( pNewStop && HB_PP_TOKEN_TYPE( pNewStop->type ) == HB_PP_MMARKER_OPTIONAL )
            pNewStop = pNewStop->pNext;

         do
         {
            pLast = pOptional;
            pFirst = pToken;
            if( hb_pp_patternMatch( pState, pOptional->pMTokens, &pToken, pNewStop, mode, NULL ) &&
                pFirst != pToken )
            {
               if( pRule && ! hb_pp_patternMatch( pState, pOptional->pMTokens, &pFirst, pNewStop, mode, pRule ) )
               {
                  fOverflow = HB_TRUE;
                  break;
               }
               pOptional = pMatch;
            }
            else
               pOptional = pOptional->pNext;
         }
         while( pOptional && HB_PP_TOKEN_TYPE( pOptional->type ) == HB_PP_MMARKER_OPTIONAL &&
                ! HB_PP_TOKEN_ISEOS( pToken ) );
         pMatch = pLast;
      }
      else
      {
         pFirst = pToken;
         if( hb_pp_tokenMatch( pMatch, &pToken, pStop, mode ) )
         {
            if( pRule && pMatch->index && pFirst != pToken )
            {
               if( ! hb_pp_patternAddResult( pRule, pMatch->index, pFirst, pToken ) )
               {
                  fOverflow = HB_TRUE;
                  break;
               }
            }
            /* ast-15 (gated by fTrackPos): the pattern token has no marker
               index - it is a LITERAL word of the rule, and this is the only
               moment the pp knows WHICH literal the source token matched.
               Until now the pairing was simply dropped and the tracking tables
               reported a bare "marker 0" for every literal, leaving a consumer
               to guess the literal from the text.  Remember it instead; a
               default build (fTrackPos off) is untouched. */
            else if( pState->fTrackPos && pRule && ! pMatch->index &&
                     pFirst != pToken )
               hb_pp_litAdd( pState, pFirst, pMatch );
         }
         else
            break;
      }

      pMatch = pMatch->pNext;
   }

   if( ! fOverflow )
   {
      while( pMatch && HB_PP_TOKEN_TYPE( pMatch->type ) == HB_PP_MMARKER_OPTIONAL )
         pMatch = pMatch->pNext;
      if( pMatch == NULL )
      {
         *pTokenPtr = pToken;
         if( pRule )
            pRule->pNextExpr = pToken;
         return HB_TRUE;
      }
   }
   return HB_FALSE;
}

static HB_BOOL hb_pp_patternCmp( PHB_PP_STATE pState, PHB_PP_RULE pRule,
                                 PHB_PP_TOKEN pToken, HB_BOOL fCommand )
{
   PHB_PP_TOKEN pFirst = pToken;

   if( hb_pp_patternMatch( pState, pRule->pMatch, &pToken, NULL,
                           HB_PP_CMP_MODE( pRule->mode ), NULL ) )
   {
      if( ! fCommand || HB_PP_TOKEN_ISEOC( pToken ) )
      {
         /* the recording pass starts here (ast-15: and only here are the
            literal pairings of THIS match worth keeping) */
         hb_pp_litClear( pState );
         if( hb_pp_patternMatch( pState, pRule->pMatch, &pFirst, NULL,
                                 HB_PP_CMP_MODE( pRule->mode ), pRule ) )
            return HB_TRUE;
         else
         {
            hb_pp_patternClearResults( pRule );
            hb_pp_litClear( pState );
         }
      }
   }
   return HB_FALSE;
}

static PHB_PP_RESULT hb_pp_matchResultGet( PHB_PP_RULE pRule, HB_USHORT usMatch,
                                           HB_USHORT usIndex )
{
   PHB_PP_MARKER pMarker = &pRule->pMarkers[ usIndex - 1];
   PHB_PP_RESULT pMarkerResult;

   /* Clipper PP does not check status of match marker but only how many
      different values were assigned to match pattern */
   if( pMarker->matches == 1 )
      pMarkerResult = pMarker->pResult;
   else if( usMatch < pMarker->matches )
   {
      pMarkerResult = pMarker->pResult;
      while( usMatch-- )
         pMarkerResult = pMarkerResult->pNext;
   }
   else
      pMarkerResult = NULL;

   return pMarkerResult;
}

static PHB_PP_TOKEN * hb_pp_matchResultLstAdd( PHB_PP_STATE pState,
                                               HB_SIZE spaces, HB_USHORT type,
                                               PHB_PP_TOKEN * pResultPtr,
                                               PHB_PP_TOKEN pToken,
                                               PHB_PP_TOKEN pStop,
                                               HB_USHORT usMarker )
{
   PHB_PP_TOKEN pNext;
   HB_BOOL fFirst = HB_TRUE, fStop = HB_FALSE;

   for( ;; )
   {
      pNext = pToken;
      if( hb_pp_tokenSkipExp( &pNext, pStop, HB_PP_CMP_ADDR, &fStop ) &&
          ( fStop ? pToken : pToken->pNext ) != pNext )
      {
         /* Check for '&' token followed by single keyword or '('
            token and do not stringify such expressions but
            clone them */
         if( type == HB_PP_RMARKER_BLOCK )
         {
            HB_BOOL fBlock = HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LEFT_CB &&
                             pToken->pNext &&
                             ( fStop ? pToken->pNext : pToken->pNext->pNext ) != pNext &&
                             HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_PIPE;

            if( ! fBlock )
            {
               hb_pp_tokenAdd( &pResultPtr, "{", 1, fFirst ? spaces : 1,
                               HB_PP_TOKEN_LEFT_CB | HB_PP_TOKEN_STATIC );
               hb_pp_tokenAdd( &pResultPtr, "|", 1, 0, HB_PP_TOKEN_PIPE | HB_PP_TOKEN_STATIC );
               hb_pp_tokenAdd( &pResultPtr, "|", 1, 0, HB_PP_TOKEN_PIPE | HB_PP_TOKEN_STATIC );
               fFirst = HB_FALSE;
            }
            do
            {
               *pResultPtr = hb_pp_tokenClone( pState, pToken );
               if( pState->fTrackPos )
                  hb_pp_drvAdd1( pState, *pResultPtr, usMarker, 'c' );
               if( fFirst )
               {
                  ( *pResultPtr )->spaces = spaces;
                  fFirst = HB_FALSE;
               }
               pResultPtr = &( *pResultPtr )->pNext;
               pToken = pToken->pNext;
            }
            while( ( fStop ? pToken : pToken->pNext ) != pNext );
            if( ! fBlock )
               hb_pp_tokenAdd( &pResultPtr, "}", 1, 0, HB_PP_TOKEN_RIGHT_CB | HB_PP_TOKEN_STATIC );
         }
         else if( ( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_MACROVAR ||
                    HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_MACROTEXT ) &&
                  ( fStop ? pToken->pNext : pToken->pNext->pNext ) == pNext )
         {
            if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_MACROVAR )
            {
               PHB_PP_TOKEN pNew = hb_pp_tokenAdd( &pResultPtr, pToken->value + 1, pToken->len -
                               ( pToken->value[ pToken->len - 1 ] == '.' ? 2 : 1 ),
                               fFirst ? spaces : pToken->spaces,
                               HB_PP_TOKEN_KEYWORD );

               if( pState->fTrackPos )
                  hb_pp_drvAdd1( pState, pNew, usMarker, 'c' );
            }
            else
            {
               PHB_PP_TOKEN pNew;

               hb_membufFlush( pState->pBuffer );
               hb_pp_tokenStr( pToken, pState->pBuffer, HB_FALSE, HB_FALSE, 0 );
               pNew = hb_pp_tokenAdd( &pResultPtr,
                               hb_membufPtr( pState->pBuffer ),
                               hb_membufLen( pState->pBuffer ),
                               fFirst ? spaces : pToken->spaces,
                               HB_PP_TOKEN_STRING );
               if( pState->fTrackPos )
                  hb_pp_drvAdd1( pState, pNew, usMarker, 's' );
            }
            pToken = pToken->pNext;
            fFirst = HB_FALSE;
         }
         else if( ( type == HB_PP_RMARKER_STRSMART &&
                    ( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_STRING ||
                      HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LEFT_PB ) ) ||
                  ( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_AMPERSAND &&
                    pToken->pNext &&
                    ( fStop ? pToken->pNext : pToken->pNext->pNext ) != pNext &&
                    HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_LEFT_PB ) )
         {
            if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_AMPERSAND )
               pToken = pToken->pNext;
            do
            {
               *pResultPtr = hb_pp_tokenClone( pState, pToken );
               if( pState->fTrackPos )
                  hb_pp_drvAdd1( pState, *pResultPtr, usMarker, 'c' );
               if( fFirst )
               {
                  ( *pResultPtr )->spaces = spaces;
                  fFirst = HB_FALSE;
               }
               pResultPtr = &( *pResultPtr )->pNext;
               pToken = pToken->pNext;
            }
            while( ( fStop ? pToken : pToken->pNext ) != pNext );
         }
         else
         {
            /* leading spaces calculation in Clipper is broken when
               separate tokens are stringified, it can be quite
               easy checked that it will interact with translation
               done just before - spaces are partially inherited.
               It means that Clipper PP does not clear some static
               buffers where holds this information.
               I decided to keep original internal spacing except the
               first token */
            HB_BOOL fSpaces = HB_FALSE;
            PHB_PP_TOKEN pNew;
            if( ! fFirst )
               spaces = pToken->spaces;
            hb_membufFlush( pState->pBuffer );
            do
            {
               hb_pp_tokenStr( pToken, pState->pBuffer, fSpaces, HB_FALSE, 0 );
               fSpaces = HB_TRUE;
               pToken = pToken->pNext;
            }
            while( ( fStop ? pToken : pToken->pNext ) != pNext );
            pNew = hb_pp_tokenAdd( &pResultPtr,
                            hb_membufPtr( pState->pBuffer ),
                            hb_membufLen( pState->pBuffer ),
                            spaces, HB_PP_TOKEN_STRING );
            if( pState->fTrackPos )
               hb_pp_drvAdd1( pState, pNew, usMarker, 's' );
            fFirst = HB_FALSE;
         }
      }
      if( fStop )
         break;
      /* clone comma token */
      *pResultPtr = hb_pp_tokenClone( pState, pToken );
      if( pState->fTrackPos )
         hb_pp_drvAdd1( pState, *pResultPtr, usMarker, 'c' );
      if( fFirst )
      {
         ( *pResultPtr )->spaces = spaces;
         fFirst = HB_FALSE;
      }
      pResultPtr = &( *pResultPtr )->pNext;
      pToken = pNext;
   }

   return pResultPtr;
}

static PHB_PP_TOKEN * hb_pp_matchResultAdd( PHB_PP_STATE pState,
                                            PHB_PP_RULE pRule, PHB_PP_TOKEN * pResultPtr,
                                            PHB_PP_TOKEN pMatch, HB_USHORT usMatch )
{
   PHB_PP_RESULT pMarkerResult = hb_pp_matchResultGet( pRule, usMatch, pMatch->index );
   PHB_PP_TOKEN pToken, pStop;

   if( HB_PP_TOKEN_TYPE( pMatch->type ) == HB_PP_RMARKER_REGULAR )
   {
      if( pMarkerResult )
      {
         HB_BOOL fFirst = HB_TRUE;
         pToken = pMarkerResult->pFirstToken;
         pStop = pMarkerResult->pNextExpr;
         if( pToken != pStop )
         {
            do
            {
               *pResultPtr = hb_pp_tokenClone( pState, pToken );
               if( pState->fTrackPos )
                  hb_pp_drvAdd1( pState, *pResultPtr, pMatch->index, 'c' );
               if( fFirst )
               {
                  ( *pResultPtr )->spaces = pMatch->spaces;
                  fFirst = HB_FALSE;
               }
               pResultPtr = &( *pResultPtr )->pNext;
               pToken = pToken->pNext;
            }
            while( pToken != pStop );
         }
      }
   }
   else if( HB_PP_TOKEN_TYPE( pMatch->type ) == HB_PP_RMARKER_STRDUMP )
   {
      hb_membufFlush( pState->pBuffer );
      if( pMarkerResult )
      {
         pToken = pMarkerResult->pFirstToken;
         pStop = pMarkerResult->pNextExpr;
         if( pToken != pStop )
         {
            HB_BOOL fSpaces = HB_FALSE;
            do
            {
               hb_pp_tokenStr( pToken, pState->pBuffer, fSpaces, HB_FALSE, 0 );
               fSpaces = HB_TRUE;
               pToken = pToken->pNext;
            }
            while( pToken != pStop );
         }
      }
      {
         PHB_PP_TOKEN pStr = hb_pp_tokenAdd( &pResultPtr,
                      hb_membufPtr( pState->pBuffer ),
                      hb_membufLen( pState->pBuffer ),
                      pMatch->spaces, HB_PP_TOKEN_STRING );

         if( pState->fTrackPos )
         {
            /* the stringified text comes from the matched marker tokens:
               inherit the position of the first of them (overwriting any
               stale entry a recycled pointer might otherwise hit) */
            PHB_PP_POSITEM pPos = pMarkerResult ?
                  hb_pp_posFind( pState, pMarkerResult->pFirstToken ) : NULL;

            if( pPos )
               hb_pp_posRecord( pState, pStr, pPos->iLine, pPos->iCol,
                                pPos->fMainFile );
            else
               hb_pp_posRecord( pState, pStr,
                                pState->pFile ? pState->pFile->iCurrentLine : 0,
                                -1, pState->pFile == NULL ||
                                    pState->pFile->pPrev == NULL );
            hb_pp_drvAdd1( pState, pStr, pMatch->index, 's' );
         }
      }
   }
   else if( HB_PP_TOKEN_TYPE( pMatch->type ) == HB_PP_RMARKER_STRSTD ||
            HB_PP_TOKEN_TYPE( pMatch->type ) == HB_PP_RMARKER_STRSMART ||
            HB_PP_TOKEN_TYPE( pMatch->type ) == HB_PP_RMARKER_BLOCK )
   {
      if( pMarkerResult )
      {
         pToken = pMarkerResult->pFirstToken;
         pStop = pMarkerResult->pNextExpr;
         /* We have to divide the expression to comma separated ones */
         if( pToken != pStop )
         {
            pResultPtr = hb_pp_matchResultLstAdd( pState, pMatch->spaces,
                  HB_PP_TOKEN_TYPE( pMatch->type ), pResultPtr, pToken, pStop,
                  pMatch->index );
         }
      }
   }
   else if( HB_PP_TOKEN_TYPE( pMatch->type ) == HB_PP_RMARKER_LOGICAL )
   {
      /* Clipper documentation is wrong and Clipper PP only checks
         if such pattern was assigned not is non empty */
      hb_pp_tokenAdd( &pResultPtr, pMarkerResult ? ".T." : ".F.", 3,
               pMatch->spaces, HB_PP_TOKEN_LOGICAL | HB_PP_TOKEN_STATIC );
   }
   else if( HB_PP_TOKEN_TYPE( pMatch->type ) == HB_PP_RMARKER_NUL )
   {
      /* nothing to stuff */
   }
   else
   {
      /* TODO? internal error? */
   }

   return pResultPtr;
}

static PHB_PP_TOKEN *  hb_pp_patternStuff( PHB_PP_STATE pState,
                                           PHB_PP_RULE pRule, HB_USHORT usMatch,
                                           PHB_PP_TOKEN pResultPattern,
                                           PHB_PP_TOKEN * pResultPtr )
{
   while( pResultPattern )
   {
      if( pResultPattern->index )
      {
         pResultPtr = hb_pp_matchResultAdd( pState, pRule, pResultPtr, pResultPattern, usMatch );
      }
      else if( HB_PP_TOKEN_TYPE( pResultPattern->type ) == HB_PP_RMARKER_OPTIONAL )
      {
         HB_USHORT usMaxMatch = 0, matches;
         PHB_PP_TOKEN pToken = pResultPattern->pMTokens;
         while( pToken )
         {
            if( pToken->index )
            {
               matches = pRule->pMarkers[ pToken->index - 1 ].matches;
               if( matches > usMaxMatch )
                  usMaxMatch = matches;
            }
            pToken = pToken->pNext;
         }
         for( matches = 0; matches < usMaxMatch; ++matches )
         {
            pResultPtr = hb_pp_patternStuff( pState, pRule, matches,
                                             pResultPattern->pMTokens,
                                             pResultPtr );
         }
      }
      else if( HB_PP_TOKEN_TYPE( pResultPattern->type ) == HB_PP_RMARKER_DYNVAL )
      {
         if( hb_pp_tokenValueCmp( pResultPattern, "__FILE__", HB_PP_CMP_CASE ) )
         {
            const char * szFileName = pState->pFile ?
                                      pState->pFile->szFileName : NULL;
            if( ! szFileName )
               szFileName = "";
            *pResultPtr = hb_pp_tokenNew( szFileName, strlen( szFileName ), 0,
                                          HB_PP_TOKEN_STRING );
            if( pState->fTrackPos )
               hb_pp_drvAddDyn( pState, *pResultPtr, 'D' );   /* FILE axis */
            pResultPtr = &( *pResultPtr )->pNext;
         }
         else if( hb_pp_tokenValueCmp( pResultPattern, "__LINE__", HB_PP_CMP_CASE ) )
         {
            char line[ 16 ];
            hb_snprintf( line, sizeof( line ), "%d",
                         pState->pFile ? pState->pFile->iCurrentLine : 0 );
            *pResultPtr = hb_pp_tokenNew( line, strlen( line ), 0,
                                          HB_PP_TOKEN_NUMBER );
            if( pState->fTrackPos )
               hb_pp_drvAddDyn( pState, *pResultPtr, 'd' );   /* LINE axis */
            pResultPtr = &( *pResultPtr )->pNext;
         }
      }
      else if( HB_PP_TOKEN_TYPE( pResultPattern->type ) == HB_PP_RMARKER_REFERENCE )
      {
         PHB_PP_TOKEN * pTokenPtr = pResultPtr;
         hb_pp_tokenAdd( &pResultPtr, "<@>", 3, pResultPattern->spaces,
                         HB_PP_RMARKER_REFERENCE | HB_PP_TOKEN_STATIC );
         ( *pTokenPtr )->pMTokens = pRule->pMatch;
      }
      else
      {
         *pResultPtr = hb_pp_tokenClone( pState, pResultPattern );
         pResultPtr = &( *pResultPtr )->pNext;
      }
      pResultPattern = pResultPattern->pNext;
   }

   return pResultPtr;
}

static char * hb_pp_tokenListStr( PHB_PP_TOKEN pToken, PHB_PP_TOKEN pStop,
                                  HB_BOOL fStop, PHB_MEM_BUFFER pBuffer,
                                  HB_BOOL fQuote, HB_BOOL fEol )
{
   HB_USHORT ltype = HB_PP_TOKEN_NUL;
   HB_BOOL fSpaces = HB_FALSE;

   hb_membufFlush( pBuffer );
   while( pToken && ( fStop ? pToken != pStop : ! HB_PP_TOKEN_ISEOC( pToken ) ) )
   {
      hb_pp_tokenStr( pToken, pBuffer, fSpaces, fQuote, ltype );
      ltype = HB_PP_TOKEN_TYPE( pToken->type );
      fSpaces = HB_TRUE;
      pToken = pToken->pNext;
   }
   if( fEol )
      hb_membufAddCh( pBuffer, '\n' );
   hb_membufAddCh( pBuffer, '\0' );

   return hb_membufPtr( pBuffer );
}

static void hb_pp_patternReplace( PHB_PP_STATE pState, PHB_PP_RULE pRule,
                                  PHB_PP_TOKEN * pTokenPtr, const char * szType )
{
   PHB_PP_TOKEN pFinalResult = NULL, * pResultPtr, pSource;

   if( pState->fTrackPos )
      hb_pp_trackApply( pState, pRule, *pTokenPtr, szType );

   pResultPtr = hb_pp_patternStuff( pState, pRule, 0, pRule->pResult, &pFinalResult );

   /* store original matched token pointer */
   pSource = *pTokenPtr;

   /* Copy number of leading spaces from the first matched token
      to the first result token */
   if( pFinalResult && pSource )
      pFinalResult->spaces = pSource->spaces;

   /* Write trace information */
   if( pState->fWriteTrace )
   {
      fprintf( pState->file_trace, "%s(%d) >%s<\n",
               pState->pFile && pState->pFile->szFileName ? pState->pFile->szFileName : "",
               pState->pFile ? pState->pFile->iCurrentLine : 0,
               /* the source string */
               hb_pp_tokenListStr( pSource, pRule->pNextExpr, HB_TRUE,
                                   pState->pBuffer, HB_TRUE, HB_FALSE ) );
      fprintf( pState->file_trace, "#%s%s >%s<\n",
               pRule->mode == HB_PP_CMP_STD ? "x" : "", szType,
               /* the result string */
               hb_pp_tokenListStr( pFinalResult, *pResultPtr, HB_TRUE,
                                   pState->pBuffer, HB_TRUE, HB_FALSE ) );
   }

   /* Replace matched tokens with result pattern */
   *pResultPtr = pRule->pNextExpr;
   *pTokenPtr = pFinalResult;

   /* Free the matched tokens */
   while( pSource != pRule->pNextExpr )
   {
      PHB_PP_TOKEN pToken = pSource;
      pSource = pSource->pNext;
      hb_pp_tokenFree( pToken );
   }

   hb_pp_patternClearResults( pRule );
}

static void hb_pp_processCondDefined( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   PHB_PP_TOKEN pNext;

   while( ! HB_PP_TOKEN_ISEOS( pToken ) )
   {
      pNext = pToken->pNext;
      if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_KEYWORD &&
          ( hb_pp_tokenValueCmp( pToken, "defined", HB_PP_CMP_CASE ) ||
            hb_pp_tokenValueCmp( pToken, "__pragma", HB_PP_CMP_CASE ) ) &&
          pNext && HB_PP_TOKEN_TYPE( pNext->type ) == HB_PP_TOKEN_LEFT_PB &&
          pNext->pNext && HB_PP_TOKEN_TYPE( pNext->pNext->type ) == HB_PP_TOKEN_KEYWORD &&
          pNext->pNext->pNext && HB_PP_TOKEN_TYPE( pNext->pNext->pNext->type ) == HB_PP_TOKEN_RIGHT_PB )
      {
         const char * szValue = NULL;
         char buffer[ 32 ];

         if( pToken->value[ 0 ] == '_' )
         {
            const char * szSwitch;

            if( hb_pp_tokenValueCmp( pNext->pNext, "AUTOMEMVAR", HB_PP_CMP_DBASE ) )
               szSwitch = "a";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "DEBUGINFO", HB_PP_CMP_DBASE ) )
               szSwitch = "b";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "DYNAMICMEMVAR", HB_PP_CMP_DBASE ) )
               szSwitch = "v";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "EXITSEVERITY", HB_PP_CMP_DBASE ) )
               szSwitch = "es";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "LINENUMBER", HB_PP_CMP_DBASE ) )
               szSwitch = "l";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "NOSTARTPROC", HB_PP_CMP_DBASE ) )
               szSwitch = "n";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "PREPROCESSING", HB_PP_CMP_DBASE ) )
               szSwitch = "p";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "SHORTCUT", HB_PP_CMP_DBASE ) )
               szSwitch = "z";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "TEXTHIDDEN", HB_PP_CMP_DBASE ) )
               szSwitch = "TEXTHIDDEN";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "TRACE", HB_PP_CMP_DBASE ) )
               szSwitch = "p+";
            else if( hb_pp_tokenValueCmp( pNext->pNext, "WARNINGLEVEL", HB_PP_CMP_DBASE ) )
               szSwitch = "w";
            else
               szSwitch = pNext->pNext->value;

            if( szSwitch )
            {
               int iValue = 0;
               if( ! hb_pp_getCompilerSwitch( pState, szSwitch, &iValue ) )
                  szValue = hb_numToStr( buffer, sizeof( buffer ), iValue );
            }
         }
         else
            szValue = hb_pp_defineFind( pState, pNext->pNext ) != NULL ?
                      "1" : "0";

         if( szValue )
         {
            hb_pp_tokenSetValue( pToken, szValue, strlen( szValue ) );
            HB_PP_TOKEN_SETTYPE( pToken, HB_PP_TOKEN_NUMBER );
            pToken->pNext = pNext->pNext->pNext->pNext;
            pNext->pNext->pNext->pNext = NULL;
            hb_pp_tokenListFree( &pNext );
         }
      }
      pToken = pToken->pNext;
   }
}

static HB_BOOL hb_pp_processDefine( PHB_PP_STATE pState, PHB_PP_TOKEN * pFirstPtr )
{
   PHB_PP_TOKEN * pPrevPtr;
   HB_BOOL fSubst = HB_FALSE, fRepeat;
   int iCycle = 0;

   do
   {
      pPrevPtr = NULL;
      fRepeat = HB_FALSE;
      while( ! HB_PP_TOKEN_ISEOS( *pFirstPtr ) )
      {
         if( HB_PP_TOKEN_TYPE( ( *pFirstPtr )->type ) == HB_PP_TOKEN_KEYWORD &&
             ( pState->pMap[ HB_PP_HASHID( *pFirstPtr ) ] & HB_PP_DEFINE ) )
         {
            PHB_PP_RULE pRule = hb_pp_defineFind( pState, *pFirstPtr );
            if( pRule )
            {
               if( hb_pp_patternCmp( pState, pRule, *pFirstPtr, HB_FALSE ) )
               {
                  hb_pp_patternReplace( pState, pRule, pFirstPtr, "define" );
                  fSubst = fRepeat = HB_TRUE;
                  if( ++pState->iCycle > pState->iMaxCycles ||
                      ++iCycle > HB_PP_MAX_REPEATS + pState->iDefinitions )
                  {
                     pState->iCycle = pState->iMaxCycles + 1;
                     hb_pp_error( pState, 'E', HB_PP_ERR_CYCLIC_DEFINE, pRule->pMatch->value );
                     return HB_TRUE;
                  }
                  continue;
               }
               if( ! pPrevPtr )
                  pPrevPtr = pFirstPtr;
            }
         }
         iCycle = 0;
         pFirstPtr = &( *pFirstPtr )->pNext;
      }
      pFirstPtr = pPrevPtr;
   }
   while( pFirstPtr && fRepeat );

   return fSubst;
}

static HB_BOOL hb_pp_processTranslate( PHB_PP_STATE pState, PHB_PP_TOKEN * pFirstPtr )
{
   HB_BOOL fSubst = HB_FALSE, fRepeat;
   int iCycle = 0;

   do
   {
      PHB_PP_TOKEN * pTokenPtr = pFirstPtr;
      fRepeat = HB_FALSE;
      while( ! HB_PP_TOKEN_ISEOS( *pTokenPtr ) )
      {
         if( pState->pMap[ HB_PP_HASHID( *pTokenPtr ) ] & HB_PP_TRANSLATE )
         {
            PHB_PP_RULE pRule = pState->pTranslations;
            while( pRule )
            {
               if( hb_pp_patternCmp( pState, pRule, *pTokenPtr, HB_FALSE ) )
               {
                  hb_pp_patternReplace( pState, pRule, pTokenPtr, "translate" );
                  fSubst = fRepeat = HB_TRUE;
                  if( ++pState->iCycle > pState->iMaxCycles ||
                      ++iCycle > HB_PP_MAX_REPEATS + pState->iTranslations )
                  {
                     pState->iCycle = pState->iMaxCycles + 1;
                     hb_pp_error( pState, 'E', HB_PP_ERR_CYCLIC_TRANSLATE, pRule->pMatch->value );
                     return HB_TRUE;
                  }
                  pRule = pState->pTranslations;
                  continue;
               }
               pRule = pRule->pPrev;
            }
         }
         iCycle = 0;
         pTokenPtr = &( *pTokenPtr )->pNext;
      }
   }
   while( fRepeat );

   return fSubst;
}

static HB_BOOL hb_pp_processCommand( PHB_PP_STATE pState, PHB_PP_TOKEN * pFirstPtr )
{
   PHB_PP_RULE pRule;
   HB_BOOL fSubst = HB_FALSE, fRepeat = HB_TRUE;
   int iCycle = 0;

   while( fRepeat && ! HB_PP_TOKEN_ISEOC( *pFirstPtr ) &&
          ( pState->pMap[ HB_PP_HASHID( *pFirstPtr ) ] & HB_PP_COMMAND ) )
   {
      fRepeat = HB_FALSE;
      pRule = pState->pCommands;
      while( pRule )
      {
         if( hb_pp_patternCmp( pState, pRule, *pFirstPtr, HB_TRUE ) )
         {
            hb_pp_patternReplace( pState, pRule, pFirstPtr, "command" );
            fSubst = fRepeat = HB_TRUE;
            if( ++pState->iCycle > pState->iMaxCycles ||
                ++iCycle > HB_PP_MAX_REPEATS + pState->iCommands )
            {
               pState->iCycle = pState->iMaxCycles + 1;
               hb_pp_error( pState, 'E', HB_PP_ERR_CYCLIC_COMMAND, pRule->pMatch->value );
               return HB_TRUE;
            }
            break;
         }
         pRule = pRule->pPrev;
      }
   }

   /* This is strictly compatible with Clipper PP which internally supports
         text <!linefunc!>,<!endfunc!>
      as stream begin directive */
   if( ! HB_PP_TOKEN_ISEOC( *pFirstPtr ) &&
       hb_pp_tokenValueCmp( *pFirstPtr, "TEXT", HB_PP_CMP_DBASE ) )
   {
      PHB_PP_TOKEN pToken = ( *pFirstPtr )->pNext, * pFuncPtr;

      if( pToken &&
          HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_KEYWORD &&
          pToken->pNext &&
          HB_PP_TOKEN_TYPE( pToken->pNext->type ) == HB_PP_TOKEN_COMMA &&
          pToken->pNext->pNext &&
          HB_PP_TOKEN_TYPE( pToken->pNext->pNext->type ) == HB_PP_TOKEN_KEYWORD &&
          HB_PP_TOKEN_ISEOC( pToken->pNext->pNext->pNext ) )
      {
         hb_pp_tokenListFree( &pState->pFuncOut );
         hb_pp_tokenListFree( &pState->pFuncEnd );

         pFuncPtr = &pState->pFuncOut;
         hb_pp_tokenAdd( &pFuncPtr, pToken->value, pToken->len, 0, HB_PP_TOKEN_KEYWORD );
         hb_pp_tokenAdd( &pFuncPtr, "(", 1, 0, HB_PP_TOKEN_LEFT_PB | HB_PP_TOKEN_STATIC );
         hb_pp_tokenAdd( &pFuncPtr, "%", 1, 1, HB_PP_RMARKER_STRDUMP | HB_PP_TOKEN_STATIC );
         hb_pp_tokenAdd( &pFuncPtr, ")", 1, 1, HB_PP_TOKEN_RIGHT_PB | HB_PP_TOKEN_STATIC );

         pToken = pToken->pNext->pNext;
         pFuncPtr = &pState->pFuncEnd;
         hb_pp_tokenAdd( &pFuncPtr, pToken->value, pToken->len, 0, HB_PP_TOKEN_KEYWORD );
         hb_pp_tokenAdd( &pFuncPtr, "(", 1, 0, HB_PP_TOKEN_LEFT_PB | HB_PP_TOKEN_STATIC );
         hb_pp_tokenAdd( &pFuncPtr, ")", 1, 1, HB_PP_TOKEN_RIGHT_PB | HB_PP_TOKEN_STATIC );
         pState->iStreamDump = HB_PP_STREAM_CLIPPER;
         hb_pp_tokenListFreeCmd( pFirstPtr );
         fSubst = HB_TRUE;
      }
   }

   return fSubst;
}

static HB_BOOL hb_pp_concatenateKeywords( PHB_PP_STATE pState, PHB_PP_TOKEN * pFirstPtr )
{
   PHB_PP_TOKEN pToken = *pFirstPtr, pNext;
   HB_BOOL fChanged = HB_FALSE;

   while( pToken && pToken->pNext )
   {
      pNext = pToken->pNext;
      if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_KEYWORD &&
          pNext->spaces == 0 &&
          HB_PP_TOKEN_TYPE( pNext->type ) == HB_PP_TOKEN_KEYWORD )
      {
         PHB_PP_FROMITEM pFrom = NULL;
         int iFromCount = 0;

         /* this is the paste: the merged token replaces the value of the
            first part (making its position entry stale by design - the
            composite has no single source position), so the marker
            origins of both parts must be captured before the rewrite */
         if( pState->fTrackPos )
            pFrom = hb_pp_drvMerge( pState, pToken, pNext, &iFromCount );

         hb_membufFlush( pState->pBuffer );
         hb_membufAddData( pState->pBuffer, pToken->value, pToken->len );
         hb_membufAddData( pState->pBuffer, pNext->value, pNext->len );

         /* Write trace information */
         if( pState->fWriteTrace )
         {
            fprintf( pState->file_trace, "%s(%d) >%s %s<\n(concatenate) >%s%s<\n",
                     pState->pFile && pState->pFile->szFileName ? pState->pFile->szFileName : "",
                     pState->pFile ? pState->pFile->iCurrentLine : 0,
                     pToken->value, pNext->value,
                     pToken->value, pNext->value );
         }

         hb_pp_tokenSetValue( pToken, hb_membufPtr( pState->pBuffer ),
                                      hb_membufLen( pState->pBuffer ) );
         if( pFrom )
            hb_pp_drvSet( pState, pToken, pFrom, iFromCount );
         pToken->pNext = pNext->pNext;
         hb_pp_tokenFree( pNext );
         fChanged = HB_TRUE;
      }
      else
         pToken = pNext;
   }

   return fChanged;
}

static PHB_PP_TOKEN hb_pp_calcPrecedence( PHB_PP_TOKEN pToken,
                                          int * piNextOper, int * piNextPrec )
{
   PHB_PP_TOKEN pNext = pToken->pNext;

   *piNextOper = HB_PP_TOKEN_TYPE( pToken->type );
   switch( *piNextOper )
   {
      /* not */
      case HB_PP_TOKEN_NOT:
         *piNextPrec = HB_PP_PREC_NOT;
         break;

      case HB_PP_TOKEN_LT:
      case HB_PP_TOKEN_GT:
         if( pNext && HB_PP_TOKEN_TYPE( pNext->type ) == *piNextOper &&
             pNext->spaces == 0 )
         {
            *piNextPrec = HB_PP_PREC_BIT;
            *piNextOper = *piNextOper == HB_PP_TOKEN_LT ? HB_PP_TOKEN_SHIFTL :
                                                          HB_PP_TOKEN_SHIFTR;
            pNext = pNext->pNext;
            break;
         }
         /* fallthrough */
      /* relational */
      case HB_PP_TOKEN_EQUAL:
      case HB_PP_TOKEN_HASH:
      case HB_PP_TOKEN_NE:
      case HB_PP_TOKEN_LE:
      case HB_PP_TOKEN_GE:
         *piNextPrec = HB_PP_PREC_REL;
         break;

      /* logical */
      case HB_PP_TOKEN_AND:
      case HB_PP_TOKEN_OR:
         *piNextPrec = HB_PP_PREC_LOG;
         break;

      /* bit */
      case HB_PP_TOKEN_PIPE:
         if( pNext && HB_PP_TOKEN_TYPE( pNext->type ) == HB_PP_TOKEN_PIPE &&
             pNext->spaces == 0 )
         {
            *piNextPrec = HB_PP_PREC_LOG;
            *piNextOper = HB_PP_TOKEN_OR;
            pNext = pNext->pNext;
         }
         else
            *piNextPrec = HB_PP_PREC_BIT;
         break;
      case HB_PP_TOKEN_AMPERSAND:
         /* It will not work because && will be stripped as comment */
         if( pNext && HB_PP_TOKEN_TYPE( pNext->type ) == HB_PP_TOKEN_AMPERSAND &&
             pNext->spaces == 0 )
         {
            *piNextPrec = HB_PP_PREC_LOG;
            *piNextOper = HB_PP_TOKEN_AND;
            pNext = pNext->pNext;
         }
         else
            *piNextPrec = HB_PP_PREC_BIT;
         break;
      case HB_PP_TOKEN_POWER:
         *piNextPrec = HB_PP_PREC_BIT;
         break;

      case HB_PP_TOKEN_BITXOR:
      case HB_PP_TOKEN_SHIFTL:
      case HB_PP_TOKEN_SHIFTR:
         *piNextPrec = HB_PP_PREC_BIT;
         break;

      /* math plus/minus */
      case HB_PP_TOKEN_PLUS:
      case HB_PP_TOKEN_MINUS:
         *piNextPrec = HB_PP_PREC_PLUS;
         break;

      /* math mult/div/mode */
      case HB_PP_TOKEN_MULT:
      case HB_PP_TOKEN_DIV:
      case HB_PP_TOKEN_MOD:
         *piNextPrec = HB_PP_PREC_MULT;
         break;

      default:
         *piNextPrec = HB_PP_PREC_NUL;
         break;
   }

   return pNext;
}

static HB_BOOL hb_pp_calcReduce( HB_MAXINT * plValue, int iOperation )
{
   switch( iOperation )
   {
      case HB_PP_TOKEN_AND:
         if( *plValue == 0 )
            return HB_TRUE;
         break;
      case HB_PP_TOKEN_OR:
         if( *plValue )
         {
            *plValue = 1;
            return HB_TRUE;
         }
         break;
   }

   return HB_FALSE;
}

static HB_MAXINT hb_pp_calcOperation( HB_MAXINT lValueLeft, HB_MAXINT lValueRight,
                                      int iOperation, HB_BOOL * pfError )
{
   switch( iOperation )
   {
      case HB_PP_TOKEN_EQUAL:
         lValueLeft = ( lValueLeft == lValueRight ) ? 1 : 0;
         break;
      case HB_PP_TOKEN_HASH:
      case HB_PP_TOKEN_NE:
         lValueLeft = ( lValueLeft != lValueRight ) ? 1 : 0;
         break;
      case HB_PP_TOKEN_LE:
         lValueLeft = ( lValueLeft <= lValueRight ) ? 1 : 0;
         break;
      case HB_PP_TOKEN_GE:
         lValueLeft = ( lValueLeft >= lValueRight ) ? 1 : 0;
         break;
      case HB_PP_TOKEN_LT:
         lValueLeft = ( lValueLeft < lValueRight ) ? 1 : 0;
         break;
      case HB_PP_TOKEN_GT:
         lValueLeft = ( lValueLeft > lValueRight ) ? 1 : 0;
         break;

      case HB_PP_TOKEN_AND:
         lValueLeft = ( lValueLeft && lValueRight ) ? 1 : 0;
         break;
      case HB_PP_TOKEN_OR:
         lValueLeft = ( lValueLeft || lValueRight ) ? 1 : 0;
         break;

      case HB_PP_TOKEN_PIPE:
         lValueLeft |= lValueRight;
         break;
      case HB_PP_TOKEN_AMPERSAND:
         lValueLeft &= lValueRight;
         break;
      case HB_PP_TOKEN_POWER:
      case HB_PP_TOKEN_BITXOR:
         lValueLeft ^= lValueRight;
         break;
      case HB_PP_TOKEN_SHIFTL:
         lValueLeft <<= lValueRight;
         break;
      case HB_PP_TOKEN_SHIFTR:
         lValueLeft >>= lValueRight;
         break;

      case HB_PP_TOKEN_PLUS:
         lValueLeft += lValueRight;
         break;
      case HB_PP_TOKEN_MINUS:
         lValueLeft -= lValueRight;
         break;
      case HB_PP_TOKEN_MULT:
         lValueLeft *= lValueRight;
         break;
      case HB_PP_TOKEN_DIV:
         if( lValueRight == 0 )
            *pfError = HB_TRUE;
         else
            lValueLeft /= lValueRight;
         break;
      case HB_PP_TOKEN_MOD:
         if( lValueRight == 0 )
            *pfError = HB_TRUE;
         else
            lValueLeft %= lValueRight;
         break;
   }

   return lValueLeft;
}

static PHB_PP_TOKEN hb_pp_calcValue( PHB_PP_TOKEN pToken, int iPrecedense,
                                     HB_MAXINT * plValue, HB_BOOL * pfError,
                                     HB_BOOL * pfUndef )
{
   if( HB_PP_TOKEN_ISEOC( pToken ) )
      *pfError = HB_TRUE;
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_MINUS )
   {
      pToken = hb_pp_calcValue( pToken->pNext, HB_PP_PREC_NEG, plValue, pfError, pfUndef );
      *plValue = - *plValue;
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_PLUS )
   {
      pToken = hb_pp_calcValue( pToken->pNext, iPrecedense, plValue, pfError, pfUndef );
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_NOT )
   {
      pToken = hb_pp_calcValue( pToken->pNext, HB_PP_PREC_NOT, plValue, pfError, pfUndef );
      *plValue = *plValue ? 0 : 1;
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LEFT_PB )
   {
      *pfError = HB_TRUE;
      pToken = hb_pp_calcValue( pToken->pNext, HB_PP_PREC_NUL, plValue, pfError, pfUndef );
      if( ! *pfError && ! HB_PP_TOKEN_ISEOC( pToken ) &&
          HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_RIGHT_PB )
         pToken = pToken->pNext;
      else
         *pfError = HB_TRUE;
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_RIGHT_PB )
   {
      return pToken;
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_NUMBER )
   {
      int iOverflow;
      *plValue = hb_strValInt( pToken->value, &iOverflow );
      if( iOverflow )
         *pfError = HB_TRUE;
      else
      {
         *pfError = HB_FALSE;
         pToken = pToken->pNext;
      }
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LOGICAL )
   {
      *plValue = HB_PP_ISTRUE( pToken->value[ 1 ] ) ? 1 : 0;
      *pfError = HB_FALSE;
      pToken = pToken->pNext;
   }
   else if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_KEYWORD )
   {
      *plValue = 0;
      pToken = pToken->pNext;
      *pfUndef = HB_TRUE;
      *pfError = HB_FALSE;
   }
   else
      *pfError = HB_TRUE;

   while( ! ( *pfError || HB_PP_TOKEN_ISEOC( pToken ) ||
              HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_RIGHT_PB ) )
   {
      int iNextOper, iNextPrec;
      PHB_PP_TOKEN pNext;
      pNext = hb_pp_calcPrecedence( pToken, &iNextOper, &iNextPrec );
      if( iNextPrec < HB_PP_PREC_LOG )
         *pfError = HB_TRUE;
      else if( iNextPrec > iPrecedense )
      {
         HB_BOOL fDefined = ( ! *pfUndef ) && hb_pp_calcReduce( plValue, iNextOper );
         HB_MAXINT lValue = 0;
         *pfError = HB_TRUE;
         pToken = hb_pp_calcValue( pNext, iNextPrec, &lValue, pfError, pfUndef );
         if( ! *pfError )
            *plValue = hb_pp_calcOperation( *plValue, lValue, iNextOper, pfError );
         if( fDefined )
            *pfUndef = HB_FALSE;
      }
      else
         break;
   }

   return pToken;
}

static HB_MAXINT hb_pp_calculateValue( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                                       HB_BOOL fNoError )
{
   HB_BOOL fError = HB_TRUE, fUndef = HB_FALSE;
   HB_MAXINT lValue = 0;

   pToken = hb_pp_calcValue( pToken, HB_PP_PREC_NUL, &lValue, &fError, &fUndef );
   if( ! HB_PP_TOKEN_ISEOC( pToken ) || fUndef )
      fError = HB_TRUE;

   if( fError )
   {
      if( ! fNoError )
         hb_pp_error( pState, 'E', HB_PP_ERR_DIRECTIVE_IF, NULL );
      lValue = 0;
   }

   return lValue;
}

static void hb_pp_conditionPush( PHB_PP_STATE pState, HB_BOOL fCond )
{
   if( pState->iCondCount == pState->iCondStackSize )
   {
      pState->iCondStackSize += 5;
      if( pState->pCondStack )
         pState->pCondStack = ( int * ) hb_xrealloc( pState->pCondStack,
                                 pState->iCondStackSize * sizeof( HB_BOOL ) );
      else
         pState->pCondStack = ( int * ) hb_xgrab( pState->iCondStackSize *
                                                          sizeof( HB_BOOL ) );
   }
   pState->pCondStack[ pState->iCondCount++ ] = pState->iCondCompile;
   pState->iCondCompile = pState->iCondCompile ? HB_PP_COND_DISABLE :
                          ( fCond ? 0 : HB_PP_COND_ELSE );
}

static void hb_pp_condCompile( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                               HB_BOOL fNot )
{
   if( ! pToken || HB_PP_TOKEN_TYPE( pToken->type ) != HB_PP_TOKEN_KEYWORD ||
       ! HB_PP_TOKEN_ISEOC( pToken->pNext ) )
   {
      hb_pp_error( pState, 'E', HB_PP_ERR_DIRECTIVE_IFDEF, NULL );
   }
   else
   {
      HB_BOOL fCond = HB_FALSE;

      if( pState->iCondCompile == 0 )
      {
         fCond = hb_pp_defineFind( pState, pToken ) != NULL;
         if( ! fNot )
            fCond = ! fCond;
      }
      hb_pp_conditionPush( pState, fCond );
   }
}

static void hb_pp_condCompileIf( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   /* preprocess all define(s) */
   hb_pp_processCondDefined( pState, pToken->pNext );
   hb_pp_processDefine( pState, &pToken->pNext );
   hb_pp_conditionPush( pState, hb_pp_calculateValue( pState, pToken->pNext,
                                             pState->iCondCompile != 0 ) != 0 );
}

static void hb_pp_condCompileElif( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   if( ( pState->iCondCompile & HB_PP_COND_DISABLE ) == 0 )
   {
      if( pState->iCondCompile )
      {
         /* preprocess all define(s) */
         hb_pp_processCondDefined( pState, pToken->pNext );
         hb_pp_processDefine( pState, &pToken->pNext );
         if( hb_pp_calculateValue( pState, pToken->pNext, HB_FALSE ) != 0 )
            pState->iCondCompile ^= HB_PP_COND_ELSE;
      }
      else
         pState->iCondCompile = HB_PP_COND_DISABLE;
   }
}

static void hb_pp_lineTokens( PHB_PP_TOKEN ** pTokenPtr, const char * szFileName, int iLine )
{
   char szLine[ 12 ];

   hb_snprintf( szLine, sizeof( szLine ), "%d", iLine );
   hb_pp_tokenAdd( pTokenPtr, "#", 1, 0, HB_PP_TOKEN_DIRECTIVE | HB_PP_TOKEN_STATIC );
   hb_pp_tokenAdd( pTokenPtr, "line", 4, 0, HB_PP_TOKEN_KEYWORD | HB_PP_TOKEN_STATIC );
   hb_pp_tokenAdd( pTokenPtr, szLine, strlen( szLine ), 1, HB_PP_TOKEN_NUMBER );
   if( szFileName )
      hb_pp_tokenAdd( pTokenPtr, szFileName, strlen( szFileName ), 1, HB_PP_TOKEN_STRING );
   hb_pp_tokenAdd( pTokenPtr, "\n", 1, 0, HB_PP_TOKEN_EOL | HB_PP_TOKEN_STATIC );
}

static void hb_pp_genLineTokens( PHB_PP_STATE pState )
{
   pState->pNextTokenPtr = &pState->pTokenOut;

   if( pState->pFile->fGenLineInfo )
   {
      hb_pp_lineTokens( &pState->pNextTokenPtr, pState->pFile->szFileName,
                                                pState->pFile->iCurrentLine );
      pState->pFile->iLastLine = pState->pFile->iCurrentLine;
      pState->pFile->fGenLineInfo = HB_FALSE;
   }
   else if( pState->pFile->iLastLine < pState->pFile->iCurrentLine )
   {
      do
      {
         hb_pp_tokenAdd( &pState->pNextTokenPtr, "\n", 1, 0, HB_PP_TOKEN_EOL | HB_PP_TOKEN_STATIC );
      }
      while( ++pState->pFile->iLastLine < pState->pFile->iCurrentLine );
   }
   hb_pp_tokenMoveCommand( pState, pState->pNextTokenPtr,
                           &pState->pFile->pTokenList );
}

static void hb_pp_includeFile( PHB_PP_STATE pState, const char * szFileName, HB_BOOL fSysFile )
{
   if( pState->iFiles >= HB_PP_MAX_INCLUDED_FILES )
   {
      hb_pp_error( pState, 'F', HB_PP_ERR_NESTED_INCLUDES, NULL );
   }
   else
   {
      HB_BOOL fNested = HB_FALSE;
      PHB_PP_FILE pFile = hb_pp_FileNew( pState, szFileName, fSysFile, &fNested,
                                         NULL, HB_TRUE, pState->pOpenFunc, HB_FALSE );
      if( pFile )
      {
#if defined( HB_PP_STRICT_LINEINFO_TOKEN )
         pState->pNextTokenPtr = &pState->pTokenOut;
         if( pState->pFile->fGenLineInfo )
         {
            hb_pp_lineTokens( &pState->pNextTokenPtr, pState->pFile->szFileName,
                                                      pState->pFile->iCurrentLine );
            pState->pFile->iLastLine = pState->pFile->iCurrentLine;
            pState->pFile->fGenLineInfo = HB_FALSE;
         }
         hb_pp_lineTokens( &pState->pNextTokenPtr, szFileName, 1 );
#else
         pFile->fGenLineInfo = HB_TRUE;
#endif
         pFile->pPrev = pState->pFile;
         pState->pFile = pFile;
         pState->iFiles++;
      }
      else if( fNested )
         hb_pp_error( pState, 'F', HB_PP_ERR_NESTED_INCLUDES, NULL );
      else
         hb_pp_error( pState, 'F', HB_PP_ERR_CANNOT_OPEN_FILE, szFileName );
   }
}

static void hb_pp_includeClose( PHB_PP_STATE pState )
{
   PHB_PP_FILE pFile = pState->pFile;

   pState->pFile = pFile->pPrev;
   pState->iFiles--;

#if defined( HB_PP_STRICT_LINEINFO_TOKEN )
   if( pFile->fGenLineInfo )
   {
      pState->pNextTokenPtr = &pState->pTokenOut;
      hb_pp_lineTokens( &pState->pNextTokenPtr, pFile->szFileName, pFile->iCurrentLine + 1 );
   }
#endif
   if( pState->pFile )
      pState->pFile->fGenLineInfo = HB_TRUE;

   hb_pp_FileFree( pState, pFile, pState->pCloseFunc );
}

static void hb_pp_preprocessToken( PHB_PP_STATE pState )
{
   while( ! pState->pTokenOut && pState->pFile )
   {
      if( ! pState->pFile->pTokenList )
      {
         while( pState->pFile->pLineBuf ? pState->pFile->nLineBufLen != 0 :
                                          ! pState->pFile->fEof )
         {
            hb_pp_getLine( pState );
            if( pState->pFile->pTokenList /* || pState->fError */ )
               break;
         }

         if( ! pState->pFile->pTokenList )
         {
#if 0       /* disabled for files included from buffer */
            if( pState->pFile->pLineBuf )
               break;
#endif
            /* this condition is only for compiler core code compatibility */
            if( ! pState->pFile->pPrev )
               break;
            hb_pp_includeClose( pState );
            continue;
         }
      }

      if( HB_PP_TOKEN_ISDIRECTIVE( pState->pFile->pTokenList ) )
      {
         HB_BOOL fError = HB_FALSE, fDirect;
         /* Store it here to avoid possible problems after #INCLUDE */
         PHB_PP_TOKEN * pFreePtr = &pState->pFile->pTokenList;
         PHB_PP_TOKEN pToken = *pFreePtr;

         fDirect = HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_DIRECTIVE;

         pToken = pToken->pNext;
         if( ! pToken )
         {
            fError = HB_TRUE;
         }
#ifndef HB_CLP_STRICT
         /* Harbour PP extension */
         else if( fDirect && pState->pFile->iCurrentLine == 1 &&
                  HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_NOT &&
                  pToken->spaces == 0 && pState->pFile->pTokenList->spaces == 0 )
         {
            /* ignore first line if it begins with "#!"
               minor extension which allow to use the same source code
               as scripts in *nix system and compile it, this feature
               will be necessary also when we integrate compiler with HVM and
               add support for direct execution compiled .prg files */
         }
#endif
         else if( HB_PP_TOKEN_TYPE( pToken->type ) != HB_PP_TOKEN_KEYWORD )
         {
            fError = HB_TRUE;
         }
         else if( hb_pp_tokenValueCmp( pToken, "IFDEF", HB_PP_CMP_DBASE ) )
         {
            hb_pp_condCompile( pState, pToken->pNext, HB_TRUE );
         }
         else if( hb_pp_tokenValueCmp( pToken, "IFNDEF", HB_PP_CMP_DBASE ) )
         {
            hb_pp_condCompile( pState, pToken->pNext, HB_FALSE );
         }
#ifndef HB_CLP_STRICT
         /* Harbour PP extension */
         else if( hb_pp_tokenValueCmp( pToken, "IF", HB_PP_CMP_DBASE ) )
         {
            hb_pp_condCompileIf( pState, pToken );
         }
         else if( hb_pp_tokenValueCmp( pToken, "ELIF", HB_PP_CMP_DBASE ) )
         {
            if( pState->iCondCount )
               hb_pp_condCompileElif( pState, pToken );
            else
               hb_pp_error( pState, 'E', HB_PP_ERR_DIRECTIVE_ELSE, NULL );
         }
#endif
         else if( hb_pp_tokenValueCmp( pToken, "ENDIF", HB_PP_CMP_DBASE ) )
         {
            if( pState->iCondCount )
               pState->iCondCompile = pState->pCondStack[ --pState->iCondCount ];
            else
               hb_pp_error( pState, 'E', HB_PP_ERR_DIRECTIVE_ENDIF, NULL );
         }
         else if( hb_pp_tokenValueCmp( pToken, "ELSE", HB_PP_CMP_DBASE ) )
         {
            if( pState->iCondCount )
               pState->iCondCompile ^= HB_PP_COND_ELSE;
            else
               hb_pp_error( pState, 'E', HB_PP_ERR_DIRECTIVE_ELSE, NULL );
         }
         /* #pragma support is always enabled even in strict compatibility
            mode to allow control by programmer some PP issues */
         else if( hb_pp_tokenValueCmp( pToken, "PRAGMA", HB_PP_CMP_DBASE ) )
         {
            hb_pp_pragmaNew( pState, pToken->pNext );
         }
         else if( pState->iCondCompile )
         {
            /* conditional compilation - other preprocessing and output disabled */
         }
         else if( hb_pp_tokenValueCmp( pToken, "INCLUDE", HB_PP_CMP_DBASE ) )
         {
            pToken = pToken->pNext;
            if( pToken && HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_STRING )
               hb_pp_includeFile( pState, pToken->value, HB_FALSE );
            else if( pToken && HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LT )
            {
               pToken = pToken->pNext;
               hb_membufFlush( pState->pBuffer );
               while( ! HB_PP_TOKEN_ISEOC( pToken ) &&
                      HB_PP_TOKEN_TYPE( pToken->type ) != HB_PP_TOKEN_GT )
               {
                  hb_membufAddData( pState->pBuffer, pToken->value, pToken->len );
                  pToken = pToken->pNext;
               }
               if( hb_membufLen( pState->pBuffer ) > 0 &&
                   ! HB_PP_TOKEN_ISEOC( pToken ) &&
                   HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_GT )
               {
                  hb_membufAddCh( pState->pBuffer, '\0' );
                  hb_pp_includeFile( pState, hb_membufPtr( pState->pBuffer ), HB_TRUE );
               }
               else
                  hb_pp_error( pState, 'F', HB_PP_ERR_WRONG_FILE_NAME, NULL );
            }
            else
               hb_pp_error( pState, 'F', HB_PP_ERR_WRONG_FILE_NAME, NULL );
         }
         else if( hb_pp_tokenValueCmp( pToken, "REQUIRE", HB_PP_CMP_STD ) )
         {
            /* do nothing. this directive is processed by hbmk2 to
               pull in external modules. */
         }
         else if( hb_pp_tokenValueCmp( pToken, "STDOUT", HB_PP_CMP_DBASE ) )
         {
            hb_pp_disp( pState, hb_pp_tokenListStr( pToken->pNext, NULL, HB_FALSE,
                                                    pState->pBuffer, HB_FALSE, HB_TRUE ) );
         }
         else if( hb_pp_tokenValueCmp( pToken, "ERROR", HB_PP_CMP_DBASE ) )
         {
            hb_pp_error( pState, 'E', HB_PP_ERR_EXPLICIT,
                         hb_pp_tokenListStr( pToken->pNext, NULL, HB_FALSE,
                                             pState->pBuffer, HB_FALSE, HB_FALSE ) );
         }
         else if( hb_pp_tokenValueCmp( pToken, "WARNING", HB_PP_CMP_DBASE ) )
         {
            hb_pp_error( pState, 'W', HB_PP_WARN_EXPLICIT,
                         hb_pp_tokenListStr( pToken->pNext, NULL, HB_FALSE,
                                             pState->pBuffer, HB_FALSE, HB_FALSE ) );
         }
         else if( hb_pp_tokenValueCmp( pToken, "DEFINE", HB_PP_CMP_DBASE ) )
         {
            hb_pp_defineNew( pState, pToken, fDirect );
         }
         else if( hb_pp_tokenValueCmp( pToken, "UNDEF", HB_PP_CMP_DBASE ) )
         {
            pToken = pToken->pNext;
            if( ! pToken || HB_PP_TOKEN_TYPE( pToken->type ) != HB_PP_TOKEN_KEYWORD ||
                ! HB_PP_TOKEN_ISEOC( pToken->pNext ) )
               hb_pp_error( pState, 'E', HB_PP_ERR_DIRECTIVE_UNDEF, NULL );
            else
               hb_pp_defineDel( pState, pToken );
         }
         else if( hb_pp_tokenValueCmp( pToken, "TRANSLATE", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_DBASE, HB_FALSE, fDirect, HB_FALSE );
         }
         else if( hb_pp_tokenValueCmp( pToken, "XTRANSLATE", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_STD, HB_FALSE, fDirect, HB_FALSE );
         }
#ifndef HB_CLP_STRICT
         else if( hb_pp_tokenValueCmp( pToken, "YTRANSLATE", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_CASE, HB_FALSE, fDirect, HB_FALSE );
         }
#endif
         else if( hb_pp_tokenValueCmp( pToken, "COMMAND", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_DBASE, HB_TRUE, fDirect, HB_FALSE );
         }
         else if( hb_pp_tokenValueCmp( pToken, "XCOMMAND", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_STD, HB_TRUE, fDirect, HB_FALSE );
         }
#ifndef HB_CLP_STRICT
         else if( hb_pp_tokenValueCmp( pToken, "YCOMMAND", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_CASE, HB_TRUE, fDirect, HB_FALSE );
         }
         /* Harbour PP extensions */
         else if( hb_pp_tokenValueCmp( pToken, "UNTRANSLATE", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_DBASE, HB_FALSE, fDirect, HB_TRUE );
         }
         else if( hb_pp_tokenValueCmp( pToken, "XUNTRANSLATE", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_STD, HB_FALSE, fDirect, HB_TRUE );
         }
         else if( hb_pp_tokenValueCmp( pToken, "YUNTRANSLATE", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_CASE, HB_FALSE, fDirect, HB_TRUE );
         }
         else if( hb_pp_tokenValueCmp( pToken, "UNCOMMAND", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_DBASE, HB_TRUE, fDirect, HB_TRUE );
         }
         else if( hb_pp_tokenValueCmp( pToken, "XUNCOMMAND", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_STD, HB_TRUE, fDirect, HB_TRUE );
         }
         else if( hb_pp_tokenValueCmp( pToken, "YUNCOMMAND", HB_PP_CMP_DBASE ) )
         {
            hb_pp_directiveNew( pState, pToken, HB_PP_CMP_CASE, HB_TRUE, fDirect, HB_TRUE );
         }
         /* Clipper PP does not accept #line and generates error */
         else if( hb_pp_tokenValueCmp( pToken, "LINE", HB_PP_CMP_DBASE ) )
         {
            /* ignore #line directives */
         }
#endif
         else
            fError = HB_TRUE;

         if( fError )
            hb_pp_error( pState, 'F', HB_PP_ERR_INVALID_DIRECTIVE, NULL );
         pState->pFile->iCurrentLine += hb_pp_tokenListFreeCmd( pFreePtr );
         continue;
      }
      else if( pState->iCondCompile )
      {
         pState->pFile->iCurrentLine += hb_pp_tokenListFreeCmd( &pState->pFile->pTokenList );
      }
      else
      {
         HB_BOOL fDirective = HB_FALSE;

         pState->iCycle = 0;
         while( ! HB_PP_TOKEN_ISEOC( pState->pFile->pTokenList ) &&
                pState->iCycle <= pState->iMaxCycles )
         {
            if( HB_PP_TOKEN_ISDIRECTIVE( pState->pFile->pTokenList ) )
            {
               fDirective = HB_TRUE;
               break;
            }
#ifndef HB_CLP_STRICT
            /* Harbour extension: concatenate keywords without spaces between
               them */
            hb_pp_concatenateKeywords( pState, &pState->pFile->pTokenList );
#endif
            if( hb_pp_processDefine( pState, &pState->pFile->pTokenList ) )
               continue;
            if( hb_pp_processTranslate( pState, &pState->pFile->pTokenList ) )
               continue;
            if( hb_pp_processCommand( pState, &pState->pFile->pTokenList ) )
               continue;
            break;
         }
         if( ! fDirective && pState->pFile->pTokenList )
            hb_pp_genLineTokens( pState );
      }
   }
}

/*
 * exported functions
 */

/*
 * internal function to initialize predefined PP rules
 */
void hb_pp_initRules( PHB_PP_RULE * pRulesPtr, int * piRules,
                      const HB_PP_DEFRULE pDefRules[], int iDefRules )
{
   PHB_PP_MARKER pMarkers;
   PHB_PP_RULE pRule;

   hb_pp_ruleListFree( pRulesPtr );
   *piRules = iDefRules;

   while( --iDefRules >= 0 )
   {
      const HB_PP_DEFRULE * pDefRule = pDefRules + iDefRules;
      if( pDefRule->markers > 0 )
      {
         HB_USHORT marker;
         HB_ULONG ulBit;

         pMarkers = ( PHB_PP_MARKER ) hb_xgrabz( pDefRule->markers * sizeof( HB_PP_MARKER ) );
         for( marker = 0, ulBit = 1; marker < pDefRule->markers; ++marker, ulBit <<= 1 )
         {
            if( pDefRule->repeatbits & ulBit )
               pMarkers[ marker ].canrepeat = HB_TRUE;
         }
      }
      else
         pMarkers = NULL;
      pRule = hb_pp_ruleNew( pDefRule->pMatch, pDefRule->pResult,
                             pDefRule->mode, pDefRule->markers, pMarkers );
      pRule->pPrev = *pRulesPtr;
      *pRulesPtr = pRule;
   }
}


/*
 * get preprocessed token
 */
/* enable/disable source position tracking of tokens: with it on, every
   token cut from an input file (and every clone made of it during rule
   application) gets an entry queryable through hb_pp_tokenPos() */
void hb_pp_trackPos( PHB_PP_STATE pState, HB_BOOL fEnable )
{
   pState->fTrackPos = fEnable;
}

/* source position of a token: returns HB_FALSE when the token has no
   entry (tracking off or token created outside the tracked paths);
   *piCol == -1 means "no column": token synthesized by a pp rule or
   separator - the line number is still meaningful */
HB_BOOL hb_pp_tokenPos( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                        int * piLine, int * piCol, HB_BOOL * pfMainFile )
{
   PHB_PP_POSITEM pItem = hb_pp_posFind( pState, pToken );

   if( pItem )
   {
      if( piLine )
         *piLine = pItem->iLine;
      if( piCol )
         *piCol = pItem->iCol;
      if( pfMainFile )
         *pfMainFile = pItem->fMainFile;
      return HB_TRUE;
   }
   return HB_FALSE;
}

/* tracked preprocessor rules (see hb_pp_trackApply()): registration facts
   of each rule seen by the current module - *pszFile == NULL means a
   built-in or API-created rule with no source directive */
int hb_pp_trackRuleCount( PHB_PP_STATE pState )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   return pTbl ? ( int ) pTbl->nRuleCount : 0;
}

/* ast-16: piMode replaces the old fX bool (which collapsed the #y... family
   onto the abbreviable #command); pfDel/piDelOf/pfRemoved expose the rule's
   LIFETIME - see HB_PP_RULEREC */
HB_BOOL hb_pp_trackRuleGet( PHB_PP_STATE pState, int iRule,
                            int * piType, int * piMode,
                            const char ** pszFile, int * piLine,
                            const char ** pszHead, int * piMarkers,
                            HB_BOOL * pfDel, int * piDelOf,
                            HB_BOOL * pfRemoved )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl && iRule >= 0 && ( HB_SIZE ) iRule < pTbl->nRuleCount )
   {
      PHB_PP_RULEREC pRec = &pTbl->pRules[ iRule ];

      if( piType )
         *piType = pRec->cType;
      if( piMode )
         *piMode = pRec->mode;
      if( pszFile )
         *pszFile = pRec->szFile;
      if( piLine )
         *piLine = pRec->iLine;
      if( pszHead )
         *pszHead = pRec->szHead;
      if( piMarkers )
         *piMarkers = pRec->markers;
      if( pfDel )
         *pfDel = pRec->fDel;
      if( piDelOf )
         *piDelOf = pRec->iDelOf;
      if( pfRemoved )
         *pfRemoved = pRec->fRemoved;
      return HB_TRUE;
   }
   return HB_FALSE;
}

/* the tracked rule seen from inside (see HB_PP_RULETOKEN): one entry per
   match/result pattern token, in the rule's STORED order, with the role
   the PP assigned when it parsed the directive and the source position
   of the token in the directive's file */
int hb_pp_trackRuleTokenCount( PHB_PP_STATE pState, int iRule,
                               HB_BOOL fResult )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl && iRule >= 0 && ( HB_SIZE ) iRule < pTbl->nRuleCount )
      return fResult ? pTbl->pRules[ iRule ].iResultCount :
                       pTbl->pRules[ iRule ].iMatchCount;
   return 0;
}

HB_BOOL hb_pp_trackRuleToken( PHB_PP_STATE pState, int iRule,
                              HB_BOOL fResult, int iToken,
                              const char ** pszText, HB_SIZE * pnLen,
                              int * piType, int * piMarker, char * pcRole,
                              int * piLine, int * piCol,
                              HB_BOOL * pfMainFile )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl && iRule >= 0 && ( HB_SIZE ) iRule < pTbl->nRuleCount )
   {
      PHB_PP_RULEREC pRec = &pTbl->pRules[ iRule ];
      PHB_PP_RULETOKEN pTok;

      if( iToken < 0 ||
          iToken >= ( fResult ? pRec->iResultCount : pRec->iMatchCount ) )
         return HB_FALSE;
      pTok = fResult ? &pRec->pResultToks[ iToken ] :
                       &pRec->pMatchToks[ iToken ];
      if( pszText )
         *pszText = pTok->szText;
      if( pnLen )
         *pnLen = pTok->nLen;
      if( piType )
         *piType = pTok->type;
      if( piMarker )
         *piMarker = pTok->marker;
      if( pcRole )
         *pcRole = pTok->cRole;
      if( piLine )
         *piLine = pTok->iLine;
      if( piCol )
         *piCol = pTok->iCol;
      if( pfMainFile )
         *pfMainFile = pTok->fMainFile;
      return HB_TRUE;
   }
   return HB_FALSE;
}

/* rule genealogy (ast-13): derivation facts of a rule's own pattern
   token, copied at registration time.  Non-empty only for a rule
   GENERATED by another rule's expansion - the from items name the
   application/marker each byte range of the token derives from, i.e.
   which application CREATED this rule */
int hb_pp_trackRuleTokenFromCount( PHB_PP_STATE pState, int iRule,
                                   HB_BOOL fResult, int iToken )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl && iRule >= 0 && ( HB_SIZE ) iRule < pTbl->nRuleCount )
   {
      PHB_PP_RULEREC pRec = &pTbl->pRules[ iRule ];

      if( iToken >= 0 &&
          iToken < ( fResult ? pRec->iResultCount : pRec->iMatchCount ) )
         return ( fResult ? pRec->pResultToks[ iToken ] :
                            pRec->pMatchToks[ iToken ] ).iFromCount;
   }
   return 0;
}

HB_BOOL hb_pp_trackRuleTokenFromGet( PHB_PP_STATE pState, int iRule,
                                     HB_BOOL fResult, int iToken, int iFrom,
                                     int * piApp, int * piMarker, char * pcOp,
                                     HB_SIZE * pnAt, HB_SIZE * pnLen )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl && iRule >= 0 && ( HB_SIZE ) iRule < pTbl->nRuleCount )
   {
      PHB_PP_RULEREC pRec = &pTbl->pRules[ iRule ];
      PHB_PP_RULETOKEN pTok;

      if( iToken < 0 ||
          iToken >= ( fResult ? pRec->iResultCount : pRec->iMatchCount ) )
         return HB_FALSE;
      pTok = fResult ? &pRec->pResultToks[ iToken ] :
                       &pRec->pMatchToks[ iToken ];
      if( iFrom >= 0 && iFrom < pTok->iFromCount )
      {
         PHB_PP_FROMITEM pFrom = &pTok->pFrom[ iFrom ];

         if( piApp )
            *piApp = pFrom->iApp;
         if( piMarker )
            *piMarker = pFrom->usMarker;
         if( pcOp )
            *pcOp = pFrom->cOp;
         if( pnAt )
            *pnAt = pFrom->nAt;
         if( pnLen )
            *pnLen = pFrom->nLen;
         return HB_TRUE;
      }
   }
   return HB_FALSE;
}

/* tracked rule applications: one entry per hb_pp_patternReplace() call
   with the source span of the consumed tokens */
int hb_pp_trackApplyCount( PHB_PP_STATE pState )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   return pTbl ? ( int ) pTbl->nAppCount : 0;
}

HB_BOOL hb_pp_trackApplyGet( PHB_PP_STATE pState, int iApply,
                             int * piRule, int * piLine, int * piTokens )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl && iApply >= 0 && ( HB_SIZE ) iApply < pTbl->nAppCount )
   {
      PHB_PP_APPREC pApp = &pTbl->pApps[ iApply ];

      if( piRule )
         *piRule = pApp->iRule;
      if( piLine )
         *piLine = pApp->iLine;
      if( piTokens )
         *piTokens = pApp->iTokCount;
      return HB_TRUE;
   }
   return HB_FALSE;
}

HB_BOOL hb_pp_trackApplyToken( PHB_PP_STATE pState, int iApply, int iToken,
                               const char ** pszText, HB_SIZE * pnLen,
                               int * piType, int * piMarker, int * piLine,
                               int * piCol, HB_BOOL * pfMainFile,
                               int * piRuleTok )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl && iApply >= 0 && ( HB_SIZE ) iApply < pTbl->nAppCount &&
       iToken >= 0 && iToken < pTbl->pApps[ iApply ].iTokCount )
   {
      PHB_PP_APPTOKEN pRec = &pTbl->pToks[ pTbl->pApps[ iApply ].nTokFirst +
                                           iToken ];

      if( pszText )
         *pszText = pRec->szText;
      if( pnLen )
         *pnLen = pRec->nLen;
      if( piType )
         *piType = pRec->type;
      if( piMarker )
         *piMarker = pRec->marker;
      if( piLine )
         *piLine = pRec->iLine;
      if( piCol )
         *piCol = pRec->iCol;
      if( pfMainFile )
         *pfMainFile = pRec->fMainFile;
      if( piRuleTok )        /* ast-15: WHICH literal of the rule, -1 = none */
         *piRuleTok = pRec->iRuleTok;
      return HB_TRUE;
   }
   return HB_FALSE;
}

/* derivation facts of a token (see hb_pp_trackPos()): from which match
   marker of which tracked application each byte range of a synthesized
   token derives - 'c'lone, 'p'aste or 's'tringify.  Zero entries means
   the token was not synthesized from a match marker */
int hb_pp_tokenFromCount( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   PHB_PP_DRVITEM pItem = hb_pp_drvFind( pState, pToken );

   return pItem ? pItem->iFromCount : 0;
}

HB_BOOL hb_pp_tokenFromGet( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                            int iFrom, int * piApp, int * piMarker,
                            char * pcOp, HB_SIZE * pnAt, HB_SIZE * pnLen )
{
   PHB_PP_DRVITEM pItem = hb_pp_drvFind( pState, pToken );

   if( pItem && iFrom >= 0 && iFrom < pItem->iFromCount )
   {
      PHB_PP_FROMITEM pRec = &pItem->pFrom[ iFrom ];

      if( piApp )
         *piApp = pRec->iApp;
      if( piMarker )
         *piMarker = pRec->usMarker;
      if( pcOp )
         *pcOp = pRec->cOp;
      if( pnAt )
         *pnAt = pRec->nAt;
      if( pnLen )
         *pnLen = pRec->nLen;
      return HB_TRUE;
   }
   return HB_FALSE;
}

/* derivation facts of a consumed application token, copied at
   application time (the consumed token dies with the replacement):
   how a multi-pass chain resolves one hop further back */
int hb_pp_trackApplyTokenFromCount( PHB_PP_STATE pState, int iApply, int iToken )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl && iApply >= 0 && ( HB_SIZE ) iApply < pTbl->nAppCount &&
       iToken >= 0 && iToken < pTbl->pApps[ iApply ].iTokCount )
      return pTbl->pToks[ pTbl->pApps[ iApply ].nTokFirst + iToken ].iFromCount;
   return 0;
}

HB_BOOL hb_pp_trackApplyTokenFromGet( PHB_PP_STATE pState, int iApply,
                                      int iToken, int iFrom, int * piApp,
                                      int * piMarker, char * pcOp,
                                      HB_SIZE * pnAt, HB_SIZE * pnLen )
{
   PHB_PP_RULETBL pTbl = ( PHB_PP_RULETBL ) pState->pRuleTbl;

   if( pTbl && iApply >= 0 && ( HB_SIZE ) iApply < pTbl->nAppCount &&
       iToken >= 0 && iToken < pTbl->pApps[ iApply ].iTokCount )
   {
      PHB_PP_APPTOKEN pTok = &pTbl->pToks[ pTbl->pApps[ iApply ].nTokFirst +
                                           iToken ];

      if( iFrom >= 0 && iFrom < pTok->iFromCount )
      {
         PHB_PP_FROMITEM pRec = &pTok->pFrom[ iFrom ];

         if( piApp )
            *piApp = pRec->iApp;
         if( piMarker )
            *piMarker = pRec->usMarker;
         if( pcOp )
            *pcOp = pRec->cOp;
         if( pnAt )
            *pnAt = pRec->nAt;
         if( pnLen )
            *pnLen = pRec->nLen;
         return HB_TRUE;
      }
   }
   return HB_FALSE;
}

PHB_PP_TOKEN hb_pp_tokenGet( PHB_PP_STATE pState )
{
   pState->fError = HB_FALSE;

   if( pState->pTokenOut )
   {
      PHB_PP_TOKEN pToken = pState->pTokenOut;
      pState->pTokenOut = pToken->pNext;
      hb_pp_tokenFree( pToken );
   }

   for( ;; )
   {
      if( ! pState->pTokenOut )
      {
         hb_pp_preprocessToken( pState );
         if( ! pState->pTokenOut )
            break;
      }
      while( pState->pTokenOut &&
             HB_PP_TOKEN_TYPE( pState->pTokenOut->type ) ==
                                                   HB_PP_RMARKER_REFERENCE )
      {
         PHB_PP_TOKEN pToken = pState->pTokenOut;
         pState->pTokenOut = pToken->pNext;
         hb_pp_tokenFree( pToken );
      }
      if( pState->pTokenOut )
         break;
   }

   if( pState->fWritePreprocesed && pState->pTokenOut )
   {
      hb_membufFlush( pState->pBuffer );
      hb_pp_tokenStr( pState->pTokenOut, pState->pBuffer, HB_TRUE, HB_TRUE,
                      pState->usLastType );
      pState->usLastType = HB_PP_TOKEN_TYPE( pState->pTokenOut->type );
      if( fwrite( hb_membufPtr( pState->pBuffer ), sizeof( char ),
                  hb_membufLen( pState->pBuffer ), pState->file_out ) !=
          hb_membufLen( pState->pBuffer ) )
      {
         hb_pp_error( pState, 'F', HB_PP_ERR_WRITE_FILE, pState->szOutFileName );
      }
   }

   return pState->pTokenOut;
}


/*
 * create new PP context
 */
PHB_PP_STATE hb_pp_new( void )
{
   return hb_pp_stateNew();
}

/*
 * free PP context
 */
void hb_pp_free( PHB_PP_STATE pState )
{
   hb_pp_stateFree( pState );
}

/*
 * initialize PP context
 */
void hb_pp_init( PHB_PP_STATE pState,
                 HB_BOOL fQuiet, HB_BOOL fGauge, int iCycles, void * cargo,
                 PHB_PP_OPEN_FUNC  pOpenFunc, PHB_PP_CLOSE_FUNC pCloseFunc,
                 PHB_PP_ERROR_FUNC pErrorFunc, PHB_PP_DISP_FUNC pDispFunc,
                 PHB_PP_DUMP_FUNC pDumpFunc, PHB_PP_INLINE_FUNC pInLineFunc,
                 PHB_PP_SWITCH_FUNC pSwitchFunc )
{
   pState->fQuiet      = pState->fQuietSet = fQuiet;
   pState->fGauge      = fGauge;
   pState->iMaxCycles  = pState->iMaxCyclesSet = ( iCycles > 0 ) ? iCycles : HB_PP_MAX_CYCLES;
   pState->cargo       = cargo;
   pState->pOpenFunc   = pOpenFunc;
   pState->pCloseFunc  = pCloseFunc;
   pState->pErrorFunc  = pErrorFunc;
   pState->pDispFunc   = pDispFunc;
   pState->pDumpFunc   = pDumpFunc;
   pState->pInLineFunc = pInLineFunc;
   pState->pSwitchFunc = pSwitchFunc;
}

void hb_pp_setIncFunc( PHB_PP_STATE pState, PHB_PP_INC_FUNC pIncFunc )
{
   pState->pIncFunc = pIncFunc;
}

/*
 * reset PP context, used for multiple .prg file compilation
 * with DO ... or *.clp files
 */
void hb_pp_reset( PHB_PP_STATE pState )
{
   pState->fError        = HB_FALSE;
   pState->iErrors       = 0;
   pState->iLineTot      = 0;
   pState->fEscStr       = HB_FALSE;
   pState->fMultiLineStr = HB_FALSE;
   pState->fTracePragmas = HB_FALSE;
   pState->fQuiet        = pState->fQuietSet;
   pState->iMaxCycles    = pState->iMaxCyclesSet;
   pState->iCondCompile  = 0;
   pState->iCondCount    = 0;
   pState->iStreamDump   = HB_PP_STREAM_OFF;

   hb_pp_tokenListFree( &pState->pFuncOut );
   hb_pp_tokenListFree( &pState->pFuncEnd );

   hb_pp_InFileFree( pState );
   hb_pp_OutFileFree( pState );
   hb_pp_TraceFileFree( pState );

   if( pState->iOperators > 0 )
   {
      hb_pp_operatorsFree( pState->pOperators, pState->iOperators );
      pState->pOperators = NULL;
      pState->iOperators = 0;
   }

   hb_pp_ruleListNonStdFree( &pState->pDefinitions );
   hb_pp_ruleListNonStdFree( &pState->pTranslations );
   hb_pp_ruleListNonStdFree( &pState->pCommands );

   /* rule tracking records are per compiled module, like the AST dump
      which consumes them; the derivation table references application
      indices of those records, so it resets together; the position
      table keys token pointers of the module just compiled - stale
      entries could ghost-match recycled pointers in the next module */
   hb_pp_ruleTblFree( pState );
   hb_pp_drvTblFree( pState );
   hb_pp_litFree( pState );
   hb_pp_posTblFree( pState );
   pState->iDrvApp = -1;
}

/*
 * add search path for included files
 */
void hb_pp_addSearchPath( PHB_PP_STATE pState, const char * szPath, HB_BOOL fReplace )
{
   if( fReplace && pState->pIncludePath )
   {
      hb_fsFreeSearchPath( pState->pIncludePath );
      pState->pIncludePath = NULL;
   }

   if( szPath && *szPath )
   {
      hb_fsAddSearchPath( szPath, &pState->pIncludePath );
   }
}

/*
 * mark current rules as standard ones
 */
void hb_pp_setStdBase( PHB_PP_STATE pState )
{
   pState->fError = HB_FALSE;
   hb_pp_ruleListSetStd( pState->pDefinitions );
   hb_pp_ruleListSetStd( pState->pTranslations );
   hb_pp_ruleListSetStd( pState->pCommands );
   memset( pState->pMap, 0, sizeof( pState->pMap ) );
   hb_pp_ruleListSetId( pState, pState->pDefinitions, HB_PP_DEFINE );
   hb_pp_ruleListSetId( pState, pState->pTranslations, HB_PP_TRANSLATE );
   hb_pp_ruleListSetId( pState, pState->pCommands, HB_PP_COMMAND );

   /* clear total number of preprocessed lines so we will report only
    * lines in compiled .prg files
    */
   pState->iLineTot = 0;
}

/*
 * initialize dynamic definitions
 */
void hb_pp_initDynDefines( PHB_PP_STATE pState, HB_BOOL fArchDefs )
{
   char szResult[ 65 ];
   int iYear, iMonth, iDay, i;
   long lDate, lTime;

   if( fArchDefs )
   {
      static const char * s_szPlatform = "__PLATFORM__%s";

      char szDefine[ 65 ];

      if( hb_verPlatformMacro() )
      {
         hb_snprintf( szDefine, sizeof( szDefine ), s_szPlatform, hb_verPlatformMacro() );
         hb_pp_addDefine( pState, szDefine, NULL );
      }
#if defined( HB_OS_UNIX )
      hb_snprintf( szDefine, sizeof( szDefine ), s_szPlatform, "UNIX" );
      hb_pp_addDefine( pState, szDefine, NULL );
#endif
#if defined( HB_OS_WIN_CE )
      hb_snprintf( szDefine, sizeof( szDefine ), s_szPlatform, "WINDOWS" );
      hb_pp_addDefine( pState, szDefine, NULL );
#endif

      hb_snprintf( szResult, sizeof( szResult ), "%d", ( int ) sizeof( void * ) );
#if defined( HB_ARCH_16BIT )
      hb_pp_addDefine( pState, "__ARCH16BIT__", szResult );
#elif defined( HB_ARCH_32BIT )
      hb_pp_addDefine( pState, "__ARCH32BIT__", szResult );
#elif defined( HB_ARCH_64BIT )
      hb_pp_addDefine( pState, "__ARCH64BIT__", szResult );
#endif

#if defined( HB_LITTLE_ENDIAN )
      hb_pp_addDefine( pState, "__LITTLE_ENDIAN__", szResult );
#elif defined( HB_BIG_ENDIAN )
      hb_pp_addDefine( pState, "__BIG_ENDIAN__", szResult );
#elif defined( HB_PDP_ENDIAN )
      hb_pp_addDefine( pState, "__PDP_ENDIAN__", szResult );
#endif
   }

#if defined( __HARBOUR__ )
   hb_snprintf( szResult, sizeof( szResult ), "0x%02X%02X%02X", HB_VER_MAJOR & 0xFF, HB_VER_MINOR & 0xFF, HB_VER_RELEASE & 0xFF );
   hb_pp_addDefine( pState, "__HARBOUR__", szResult );
#endif

   /* __DATE__ */
   hb_dateToday( &iYear, &iMonth, &iDay );
   hb_dateStrPut( szResult + 1, iYear, iMonth, iDay );
   szResult[ 0 ] = '"';
   szResult[ 9 ] = '"';
   szResult[ 10 ] = '\0';
   hb_pp_addDefine( pState, "__DATE__", szResult );

   /* __TIME__ */
   hb_dateTimeStr( szResult + 1 );
   szResult[ 0 ] = '"';
   szResult[ 9 ] = '"';
   szResult[ 10 ] = '\0';
   hb_pp_addDefine( pState, "__TIME__", szResult );

   /* __TIMESTAMP__ */
   szResult[ 0 ] = 't';
   szResult[ 1 ] = '"';
   hb_timeStampGet( &lDate, &lTime );
   hb_timeStampStr( szResult + 2, lDate, lTime );
   i = ( int ) strlen( szResult );
   szResult[ i++ ] = '"';
   szResult[ i ] = '\0';
   hb_pp_addDefine( pState, "__TIMESTAMP__", szResult );

   hb_pp_addDefine( pState, "__FILE__", &s_pp_dynamicResult );
   hb_pp_addDefine( pState, "__LINE__", &s_pp_dynamicResult );

#ifdef HB_START_PROCEDURE
   hb_pp_addDefine( pState, "__HB_MAIN__", HB_START_PROCEDURE );
#endif
}

/*
 * read preprocess rules from file
 */
void hb_pp_readRules( PHB_PP_STATE pState, const char * szRulesFile )
{
   char szFileName[ HB_PATH_MAX ];
   PHB_PP_FILE pFile = pState->pFile;
   PHB_FNAME pFileName;

   pFileName = hb_fsFNameSplit( szRulesFile );
   if( ! pFileName->szExtension )
      pFileName->szExtension = ".ch";
   hb_fsFNameMerge( szFileName, pFileName );
   hb_xfree( pFileName );

   pState->pFile = hb_pp_FileNew( pState, szFileName, HB_FALSE, NULL, NULL,
                                  HB_TRUE, pState->pOpenFunc, HB_FALSE );
   if( ! pState->pFile )
   {
      pState->pFile = pFile;
      hb_pp_error( pState, 'F', HB_PP_ERR_CANNOT_OPEN_RULES, szFileName );
   }
   else
   {
      HB_BOOL fError = HB_FALSE;

      pState->iFiles++;
      pState->usLastType = HB_PP_TOKEN_NUL;
      while( hb_pp_tokenGet( pState ) )
      {
         if( pState->fError )
            fError = HB_TRUE;
      }
      if( pState->pFile )
      {
         hb_pp_FileFree( pState, pState->pFile, pState->pCloseFunc );
         pState->iFiles--;
      }
      pState->pFile = pFile;
      if( fError )
         pState->fError = HB_TRUE;
   }
}

/*
 * close all open input files and set the given buffer as input stream
 */
HB_BOOL hb_pp_inBuffer( PHB_PP_STATE pState, const char * szFileName,
                        const char * pBuffer, HB_SIZE nLen, int iStartLine )
{
   hb_pp_InFileFree( pState );

   pState->fError = HB_FALSE;

   pState->pFile = hb_pp_FileBufNew( pBuffer, nLen );
   if( szFileName )
      pState->pFile->szFileName = hb_strdup( szFileName );
   pState->pFile->iCurrentLine = iStartLine;
   pState->pFile->iLastLine = iStartLine + 1;
   pState->iFiles++;
   return HB_TRUE;
}

/*
 * close all open input files and set the given one as new
 */
HB_BOOL hb_pp_inFile( PHB_PP_STATE pState, const char * szFileName,
                      HB_BOOL fSearchPath, FILE * file_in, HB_BOOL fError )
{
   hb_pp_InFileFree( pState );

   pState->fError = HB_FALSE;

   pState->pFile = hb_pp_FileNew( pState, szFileName, HB_FALSE, NULL,
                                  file_in, fSearchPath, NULL, HB_FALSE );
   if( pState->pFile )
   {
      pState->iFiles++;
      return HB_TRUE;
   }
   if( fError )
      hb_pp_error( pState, 'F', HB_PP_ERR_CANNOT_OPEN_INPUT, szFileName );
   return HB_FALSE;
}

/*
 * set output (.ppo) file
 */
HB_BOOL hb_pp_outFile( PHB_PP_STATE pState, const char * szOutFileName,
                       FILE * file_out )
{
   pState->fError = HB_FALSE;
   hb_pp_OutFileFree( pState );

   if( szOutFileName )
   {

      if( file_out )
         pState->file_out = file_out;
      else
         pState->file_out = hb_fopen( szOutFileName, "w" );

      if( pState->file_out )
      {
         pState->szOutFileName = hb_strdup( szOutFileName );
         pState->fWritePreprocesed = HB_TRUE;
      }
      else
      {
         hb_pp_error( pState, 'F', HB_PP_ERR_CANNOT_CREATE_FILE, szOutFileName );
      }
   }
   return ! pState->fError;
}

/*
 * set trace (.ppt) file
 */
HB_BOOL hb_pp_traceFile( PHB_PP_STATE pState, const char * szTraceFileName, FILE * file_trace )
{
   pState->fError = HB_FALSE;
   hb_pp_TraceFileFree( pState );

   if( szTraceFileName )
   {

      if( file_trace )
         pState->file_trace = file_trace;
      else
         pState->file_trace = hb_fopen( szTraceFileName, "w" );

      if( pState->file_trace )
      {
         pState->szTraceFileName = hb_strdup( szTraceFileName );
         pState->fWriteTrace = HB_TRUE;
      }
      else
      {
         hb_pp_error( pState, 'F', HB_PP_ERR_CANNOT_CREATE_FILE, szTraceFileName );
      }
   }
   return ! pState->fError;
}

/*
 * check error status of last PP operation
 */
HB_BOOL hb_pp_lasterror( PHB_PP_STATE pState )
{
   return pState->fError;
}

/*
 * retrieve number of errors which appeared during preprocessing
 */
int hb_pp_errorCount( PHB_PP_STATE pState )
{
   return pState->iErrors;
}

/*
 * return currently preprocessed file name
 */
char * hb_pp_fileName( PHB_PP_STATE pState )
{
   if( pState->pFile )
      return pState->pFile->szFileName;
   else
      return NULL;
}

/*
 * return currently preprocessed line number
 */
int hb_pp_line( PHB_PP_STATE pState )
{
   if( pState->pFile )
      return pState->pFile->iCurrentLine;
   else
      return 0;
}

int hb_pp_lineTot( PHB_PP_STATE pState )
{
   return pState->iLineTot;
}

/*
 * return output file name (.ppo)
 */
char * hb_pp_outFileName( PHB_PP_STATE pState )
{
   return pState->szOutFileName;
}

/*
 * return trace output file name (.ppt)
 */
char * hb_pp_traceFileName( PHB_PP_STATE pState )
{
   return pState->szTraceFileName;
}

/*
 * return if EOF was reached
 */
HB_BOOL hb_pp_eof( PHB_PP_STATE pState )
{
   return pState->pFile->fEof;
}

/*
 * add new define value
 */
void hb_pp_addDefine( PHB_PP_STATE pState, const char * szDefName,
                      const char * szDefValue )
{
   PHB_PP_TOKEN pMatch, pResult, pToken;
   PHB_PP_FILE pFile;

   pState->fError = HB_FALSE;

   pFile = hb_pp_FileBufNew( szDefName, strlen( szDefName ) );
   pFile->pPrev = pState->pFile;
   pState->pFile = pFile;
   pState->iFiles++;
   hb_pp_getLine( pState );
   pMatch = pState->pFile->pTokenList;
   pState->pFile->pTokenList = NULL;
   pToken = hb_pp_tokenResultEnd( &pMatch, HB_TRUE );
   hb_pp_tokenListFree( &pToken );

   if( szDefValue && ! pState->fError )
   {
      if( szDefValue == &s_pp_dynamicResult )
      {
         pResult = hb_pp_tokenNew( szDefName, strlen( szDefName ), 0,
                                   HB_PP_RMARKER_DYNVAL | HB_PP_TOKEN_STATIC );
      }
      else
      {
         pFile->pLineBuf = szDefValue;
         pFile->nLineBufLen = strlen( szDefValue );
         hb_pp_getLine( pState );
         pResult = pState->pFile->pTokenList;
         pState->pFile->pTokenList = NULL;
         pToken = hb_pp_tokenResultEnd( &pResult, HB_TRUE );
         hb_pp_tokenListFree( &pToken );
      }
   }
   else
      pResult = NULL;

   if( pState->fError || ! pMatch )
   {
      hb_pp_tokenListFree( &pMatch );
      hb_pp_tokenListFree( &pResult );
   }
   else
   {
      hb_pp_defineAdd( pState, HB_PP_CMP_CASE, 0, NULL, pMatch, pResult );
   }
   pState->pFile = pFile->pPrev;
   hb_pp_FileFree( pState, pFile, NULL );
   pState->iFiles--;
}

/*
 * delete define value
 */
void hb_pp_delDefine( PHB_PP_STATE pState, const char * szDefName )
{
   PHB_PP_TOKEN pToken;

   pToken = hb_pp_tokenNew( szDefName, strlen( szDefName ),
                            0, HB_PP_TOKEN_KEYWORD );
   hb_pp_defineDel( pState, pToken );
   hb_pp_tokenFree( pToken );
}

/*
 * set stream mode
 */
void hb_pp_setStream( PHB_PP_STATE pState, int iMode )
{
   pState->fError = HB_FALSE;
   switch( iMode )
   {
      case HB_PP_STREAM_DUMP_C:
         pState->iDumpLine = pState->pFile ? pState->pFile->iCurrentLine : 0;
         if( ! pState->pDumpBuffer )
            pState->pDumpBuffer = hb_membufNew();
         pState->iStreamDump = iMode;
         break;

      case HB_PP_STREAM_INLINE_C:
         pState->iDumpLine = pState->pFile ? pState->pFile->iCurrentLine : 0;
         /* fallthrough */
      case HB_PP_STREAM_CLIPPER:
      case HB_PP_STREAM_PRG:
      case HB_PP_STREAM_C:
         if( ! pState->pStreamBuffer )
            pState->pStreamBuffer = hb_membufNew();
         /* fallthrough */
      case HB_PP_STREAM_OFF:
      case HB_PP_STREAM_COMMENT:
         pState->iStreamDump = iMode;
         break;

      default:
         pState->fError = HB_TRUE;
   }
}

/*
 * return next preprocessed line
 */
char * hb_pp_nextLine( PHB_PP_STATE pState, HB_SIZE * pnLen )
{
   if( pState->pFile )
   {
      PHB_PP_TOKEN pToken;
      HB_BOOL fError = HB_FALSE;
      HB_USHORT ltype;

      if( ! pState->pOutputBuffer )
         pState->pOutputBuffer = hb_membufNew();
      else
         hb_membufFlush( pState->pOutputBuffer );

      pState->usLastType = ltype = HB_PP_TOKEN_NUL;
      while( ( pToken = hb_pp_tokenGet( pState ) ) != NULL )
      {
         if( pState->fError )
            fError = HB_TRUE;
         if( hb_pp_tokenStr( pToken, pState->pOutputBuffer, HB_TRUE, HB_TRUE, ltype ) )
            break;
         /* only single command in one call */
         if( ! pState->pTokenOut->pNext )
            break;
         ltype = HB_PP_TOKEN_TYPE( pToken->type );
      }
      if( fError )
         pState->fError = HB_TRUE;

      if( pnLen )
         *pnLen = hb_membufLen( pState->pOutputBuffer );
      hb_membufAddCh( pState->pOutputBuffer, '\0' );

      return hb_membufPtr( pState->pOutputBuffer );
   }

   if( pnLen )
      *pnLen = 0;
   return NULL;
}

/*
 * preprocess given buffer
 */
char * hb_pp_parseLine( PHB_PP_STATE pState, const char * pLine, HB_SIZE * pnLen )
{
   PHB_PP_TOKEN pToken;
   PHB_PP_FILE pFile;
   HB_BOOL fError = HB_FALSE;
   HB_USHORT ltype;
   HB_SIZE nLen;

   if( ! pState->pOutputBuffer )
      pState->pOutputBuffer = hb_membufNew();
   else
      hb_membufFlush( pState->pOutputBuffer );

   nLen = pnLen ? *pnLen : strlen( pLine );

   pFile = hb_pp_FileBufNew( pLine, nLen );
   pFile->pPrev = pState->pFile;
   pState->pFile = pFile;
   pState->iFiles++;

   pState->usLastType = ltype = HB_PP_TOKEN_NUL;
   while( ( pToken = hb_pp_tokenGet( pState ) ) != NULL )
   {
      if( pState->fError )
         fError = HB_TRUE;
      hb_pp_tokenStr( pToken, pState->pOutputBuffer, HB_TRUE, HB_TRUE, ltype );
      ltype = HB_PP_TOKEN_TYPE( pToken->type );
   }
   if( fError )
      pState->fError = HB_TRUE;

   if( ( nLen && pLine[ nLen - 1 ] == '\n' ) ||
       hb_membufLen( pState->pOutputBuffer ) == 0 ||
       hb_membufPtr( pState->pOutputBuffer )
                        [ hb_membufLen( pState->pOutputBuffer ) - 1 ] != '\n' )
      hb_membufAddCh( pState->pOutputBuffer, '\0' );
   else
      hb_membufPtr( pState->pOutputBuffer )
                        [ hb_membufLen( pState->pOutputBuffer ) - 1 ] = '\0';

   if( pnLen )
      *pnLen = hb_membufLen( pState->pOutputBuffer ) - 1;

   if( pState->pFile == pFile )
   {
      pState->pFile = pFile->pPrev;
      hb_pp_FileFree( pState, pFile, NULL );
      pState->iFiles--;
   }

   return hb_membufPtr( pState->pOutputBuffer );
}

/*
 * create new PP context for macro compiler
 */
PHB_PP_STATE hb_pp_lexNew( const char * pMacroString, HB_SIZE nLen )
{
   PHB_PP_STATE pState = hb_pp_new();

   pState->fQuiet = HB_TRUE;
   pState->fGauge = HB_FALSE;
   pState->pFile = hb_pp_FileBufNew( pMacroString, nLen );
   hb_pp_getLine( pState );
   pState->pTokenOut = pState->pFile->pTokenList;
   pState->pFile->pTokenList = NULL;
   hb_pp_FileFree( pState, pState->pFile, NULL );
   pState->pFile = NULL;
   if( pState->fError )
   {
      hb_pp_free( pState );
      pState = NULL;
   }
   else
      pState->pNextTokenPtr = &pState->pTokenOut;

   return pState;
}

PHB_PP_TOKEN hb_pp_lexGet( PHB_PP_STATE pState )
{
   PHB_PP_TOKEN pToken = *pState->pNextTokenPtr;

   if( pToken )
      pState->pNextTokenPtr = &pToken->pNext;

   return pToken;
}

HB_BOOL hb_pp_tokenNextExp( PHB_PP_TOKEN * pTokenPtr )
{
   if( hb_pp_tokenCanStartExp( *pTokenPtr ) )
   {
      HB_BOOL fStop = HB_FALSE;
      if( hb_pp_tokenSkipExp( pTokenPtr, NULL, HB_PP_CMP_STD, &fStop ) && ! fStop )
         return HB_TRUE;
   }

   return HB_FALSE;
}

/*
 * convert token letters to upper cases
 * strip leading '&' and trailing '.' (if any) from macrovar token
 */
void hb_pp_tokenUpper( PHB_PP_TOKEN pToken )
{
   if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_MACROVAR )
   {
      if( pToken->len > HB_SYMBOL_NAME_LEN + 1 )
         pToken->len = HB_SYMBOL_NAME_LEN + 1;
      if( pToken->value[ pToken->len - 1 ] == '.' )
         pToken->len -= 2;
      else
         pToken->len--;

      if( pToken->len <= 1 )
      {
         HB_UCHAR ucVal = pToken->len ? ( HB_UCHAR ) pToken->value[ 1 ] : 0;
         if( HB_PP_TOKEN_ALLOC( pToken->type ) )
         {
            hb_xfree( HB_UNCONST( pToken->value ) );
            pToken->type |= HB_PP_TOKEN_STATIC;
         }
         pToken->value = hb_szAscii[ ucVal ];
      }
      else
      {
         if( ! HB_PP_TOKEN_ALLOC( pToken->type ) )
         {
            pToken->value = ( char * ) memcpy( hb_xgrab( pToken->len + 1 ),
                                               pToken->value + 1, pToken->len );
            pToken->type &= ~HB_PP_TOKEN_STATIC;
         }
         else
            memmove( HB_UNCONST( pToken->value ), pToken->value + 1, pToken->len );
         ( ( char * ) HB_UNCONST( pToken->value ) )[ pToken->len ] = '\0';
      }
   }
   else if( pToken->len > 1 )
   {
      if( ! HB_PP_TOKEN_ALLOC( pToken->type ) )
      {
         char * value = ( char * ) hb_xgrab( pToken->len + 1 );
         memcpy( value, pToken->value, pToken->len + 1 );
         pToken->value = value;
         pToken->type &= ~HB_PP_TOKEN_STATIC;
      }
      if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_KEYWORD &&
          pToken->len > HB_SYMBOL_NAME_LEN )
      {
         pToken->len = HB_SYMBOL_NAME_LEN;
         ( ( char * ) HB_UNCONST( pToken->value ) )[ HB_SYMBOL_NAME_LEN ] = '\0';
      }
   }

   if( pToken->len <= 1 )
   {
      HB_UCHAR ucVal = ( HB_UCHAR ) HB_PP_UPPER( pToken->value[ 0 ] );
      if( HB_PP_TOKEN_ALLOC( pToken->type ) )
      {
         hb_xfree( HB_UNCONST( pToken->value ) );
         pToken->type |= HB_PP_TOKEN_STATIC;
      }
      pToken->value = hb_szAscii[ ucVal ];
   }
   else
      hb_strupr( ( char * ) HB_UNCONST( pToken->value ) );
}

/*
 * convert tokens between '[' and ']' tokens into single string token
 * and replace the converted tokens with the new string
 */
void hb_pp_tokenToString( PHB_PP_STATE pState, PHB_PP_TOKEN pToken )
{
   HB_BOOL fError = HB_TRUE;

   pState->fError = HB_FALSE;
   hb_membufFlush( pState->pBuffer );
   if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LEFT_SB )
   {
      PHB_PP_TOKEN pTok, pFirst, pLast = NULL;
      pFirst = pTok = pToken->pNext;
      while( ! HB_PP_TOKEN_ISEOL( pTok ) )
      {
         pLast = pTok;
         if( HB_PP_TOKEN_TYPE( pTok->type ) == HB_PP_TOKEN_RIGHT_SB )
         {
            while( pTok->spaces > 0 )
            {
               hb_membufAddCh( pState->pBuffer, ' ' );
               pTok->spaces--;
            }
            fError = HB_FALSE;
            pTok = pTok->pNext;
            break;
         }
         else if( HB_PP_TOKEN_TYPE( pTok->type ) == HB_PP_TOKEN_EOC &&
                  ! pTok->pNext && pState->pFile->pTokenList )
         {
            hb_pp_tokenMoveCommand( pState, &pTok->pNext,
                                    &pState->pFile->pTokenList );
         }
         hb_pp_tokenStr( pTok, pState->pBuffer, HB_TRUE, HB_FALSE, 0 );
         pTok = pTok->pNext;
      }
      if( pLast )
      {
         pLast->pNext = NULL;
         pToken->pNext = pTok;
         hb_pp_tokenListFree( &pFirst );
      }
      hb_pp_tokenSetValue( pToken, hb_membufPtr( pState->pBuffer ),
                                   hb_membufLen( pState->pBuffer ) );
      HB_PP_TOKEN_SETTYPE( pToken, HB_PP_TOKEN_STRING );
      if( pState->fWritePreprocesed )
      {
         if( ! fError )
            hb_membufAddCh( pState->pBuffer, ']' );
         if( fwrite( hb_membufPtr( pState->pBuffer ), sizeof( char ),
                     hb_membufLen( pState->pBuffer ), pState->file_out ) !=
             hb_membufLen( pState->pBuffer ) )
         {
            hb_pp_error( pState, 'F', HB_PP_ERR_WRITE_FILE, pState->szOutFileName );
         }
      }
   }

   if( fError )
   {
      hb_membufAddCh( pState->pBuffer, '\0' );
      hb_pp_error( pState, 'E', HB_PP_ERR_STRING_TERMINATOR,
                   hb_membufPtr( pState->pBuffer ) );
   }
}

char * hb_pp_tokenBlockString( PHB_PP_STATE pState, PHB_PP_TOKEN pToken,
                               int * piType, HB_SIZE * pnLen )
{
   *piType = 0;
   hb_membufFlush( pState->pBuffer );
   if( HB_PP_TOKEN_TYPE( pToken->type ) == HB_PP_TOKEN_LEFT_CB )
   {
      HB_USHORT ltype = HB_PP_TOKEN_NUL;
      int iBraces = 0;
      do
      {
         hb_pp_tokenStr( pToken, pState->pBuffer, ltype != HB_PP_TOKEN_NUL,
                         HB_TRUE, ltype );
         ltype = HB_PP_TOKEN_TYPE( pToken->type );
         switch( ltype )
         {
            case HB_PP_TOKEN_MACROVAR:
            case HB_PP_TOKEN_MACROTEXT:
               *piType |= HB_BLOCK_MACROVAR;
               break;
            case HB_PP_TOKEN_RIGHT_CB:
               --iBraces;
               break;
            case HB_PP_TOKEN_LEFT_CB:
               ++iBraces;
               break;
         }
         pToken = pToken->pNext;
      }
      while( iBraces && ! HB_PP_TOKEN_ISEOC( pToken ) );
   }
   *pnLen = hb_membufLen( pState->pBuffer );
   hb_membufAddCh( pState->pBuffer, '\0' );
   return hb_membufPtr( pState->pBuffer );
}
