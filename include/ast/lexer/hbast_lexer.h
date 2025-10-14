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

#ifndef HB_AST_LEXER_H_
#define HB_AST_LEXER_H_

#include "hbapi.h"

HB_EXTERN_BEGIN

typedef struct
{
   HB_SIZE nLine;
   HB_SIZE nColumn;
   HB_SIZE nOffset;
} HB_AST_SOURCE_COORD;

typedef struct
{
   HB_AST_SOURCE_COORD start;
   HB_AST_SOURCE_COORD end;
} HB_AST_SOURCE_RANGE;

typedef struct
{
   HB_U32 uHash;
   HB_SIZE nMacroDepth;
} HB_AST_TOKEN_ID;

typedef enum
{
   HB_AST_TOKEN_CHANNEL_CODE = 0,
   HB_AST_TOKEN_CHANNEL_COMMENT,
   HB_AST_TOKEN_CHANNEL_DIRECTIVE,
   HB_AST_TOKEN_CHANNEL_TRIVIA
} HB_AST_TOKEN_CHANNEL;

typedef enum
{
   HB_AST_TOKEN_KIND_UNKNOWN = 0,
   HB_AST_TOKEN_KIND_EOF
} HB_AST_TOKEN_KIND;

typedef struct
{
   HB_AST_TOKEN_ID id;
   HB_AST_SOURCE_RANGE original;
   HB_AST_SOURCE_RANGE expanded;
   HB_AST_TOKEN_KIND kind;
   HB_U16 uChannel;
   const char * pszLexeme;
   void * pMacroOrigin;
} HB_AST_TOKEN;

typedef struct _HB_AST_LEXER HB_AST_LEXER;
typedef struct _HB_AST_TOKEN_STREAM HB_AST_TOKEN_STREAM;

typedef struct
{
   const char * pszModule;
   const char * pszBuffer;
   HB_SIZE nLength;
   HB_BOOL fOwnBuffer;
} HB_AST_LEXER_SOURCE;

HB_AST_LEXER * hb_astLexerNew( const HB_AST_LEXER_SOURCE * pSource );
void hb_astLexerFree( HB_AST_LEXER * pLexer );
void hb_astLexerReset( HB_AST_LEXER * pLexer, const HB_AST_LEXER_SOURCE * pSource );
HB_BOOL hb_astLexerNextToken( HB_AST_LEXER * pLexer, HB_AST_TOKEN * pToken );

HB_BOOL hb_astLexerMacroTracePush( HB_AST_LEXER * pLexer, const char * szMacroName );
void hb_astLexerMacroTracePop( HB_AST_LEXER * pLexer );

HB_AST_TOKEN_STREAM * hb_astTokenStreamSnapshot( const HB_AST_LEXER * pLexer );
void hb_astTokenStreamRelease( HB_AST_TOKEN_STREAM * pStream );

HB_EXTERN_END

#endif /* HB_AST_LEXER_H_ */
