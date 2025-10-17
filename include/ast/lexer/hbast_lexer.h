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
   HB_AST_TOKEN_KIND_EOF,
   HB_AST_TOKEN_KIND_KEYWORD,
   HB_AST_TOKEN_KIND_IDENTIFIER,
   HB_AST_TOKEN_KIND_LITERAL,
   HB_AST_TOKEN_KIND_OPERATOR,
   HB_AST_TOKEN_KIND_PUNCTUATION,
   HB_AST_TOKEN_KIND_DIRECTIVE,
   HB_AST_TOKEN_KIND_MACRO,
   HB_AST_TOKEN_KIND_NEWLINE
} HB_AST_TOKEN_KIND;

typedef struct
{
   HB_AST_TOKEN_ID id;
   HB_AST_SOURCE_RANGE original;
   HB_AST_SOURCE_RANGE expanded;
   HB_SIZE nLexemeLength;
   HB_USHORT uPPType;
   HB_AST_TOKEN_KIND kind;
   HB_U16 uChannel;
   const char * pszLexeme;
   void * pMacroOrigin;
   const char * pszModule;
} HB_AST_TOKEN;

typedef struct _HB_AST_LEXER HB_AST_LEXER;
typedef struct _HB_AST_TOKEN_STREAM HB_AST_TOKEN_STREAM;

/* Describes the input used to initialise an incremental lexer.
   Fields:
     - pszModule  : logical module name copied into every emitted token.
     - pszBuffer  : points to the source buffer, or (when fFromFile is HB_TRUE)
                    the filesystem path to open.
     - nLength    : size of the buffer in bytes; ignored for file-backed input.
     - fOwnBuffer : HB_TRUE when the lexer should free the buffer during reset.
     - fFromFile  : HB_TRUE to stream from disk, HB_FALSE for in-memory buffers. */
typedef struct
{
   const char * pszModule;
   const char * pszBuffer;
   HB_SIZE nLength;
   HB_BOOL fOwnBuffer;
   HB_BOOL fFromFile;
} HB_AST_LEXER_SOURCE;

HB_AST_LEXER * hb_astLexerNew( const HB_AST_LEXER_SOURCE * pSource );
void hb_astLexerFree( HB_AST_LEXER * pLexer );
void hb_astLexerReset( HB_AST_LEXER * pLexer, const HB_AST_LEXER_SOURCE * pSource );
HB_BOOL hb_astLexerNextToken( HB_AST_LEXER * pLexer, HB_AST_TOKEN * pToken );

HB_BOOL hb_astLexerMacroTracePush( HB_AST_LEXER * pLexer, const char * szMacroName );
void hb_astLexerMacroTracePop( HB_AST_LEXER * pLexer );

HB_AST_TOKEN_STREAM * hb_astTokenStreamSnapshot( const HB_AST_LEXER * pLexer );
void hb_astTokenStreamRelease( HB_AST_TOKEN_STREAM * pStream );
HB_SIZE hb_astTokenStreamCount( const HB_AST_TOKEN_STREAM * pStream );
const HB_AST_TOKEN * hb_astTokenStreamToken( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE nIndex );
HB_SIZE hb_astTokenStreamMacroTraceCount( const HB_AST_TOKEN_STREAM * pStream );
const void * hb_astTokenStreamMacroTrace( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE nIndex );
char * hb_astTokenStreamSerializeMacrosJson( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE * pnLength );
void hb_astTokenStreamSerializeMacrosJsonFree( char * pszJson );
HB_BOOL hb_astTokenStreamWriteMacrosJson( const HB_AST_TOKEN_STREAM * pStream, const char * pszPath );
char * hb_astTokenStreamSerializeSnapshotJson( const HB_AST_TOKEN_STREAM * pStream, const char * pszModule, HB_SIZE * pnLength );
void hb_astTokenStreamSerializeSnapshotJsonFree( char * pszJson );
HB_BOOL hb_astTokenStreamWriteSnapshotJson( const HB_AST_TOKEN_STREAM * pStream, const char * pszModule, const char * pszPath );
char * hb_astTokenStreamSerializeSymbolsJson( const HB_AST_TOKEN_STREAM * pStream, const char * pszModule, HB_SIZE * pnLength );
void hb_astTokenStreamSerializeSymbolsJsonFree( char * pszJson );
HB_BYTE * hb_astTokenStreamSerializeMacrosCbor( const HB_AST_TOKEN_STREAM * pStream, HB_SIZE * pnLength );
void hb_astTokenStreamSerializeMacrosCborFree( HB_BYTE * pBuffer );
HB_BOOL hb_astTokenStreamWriteMacrosCbor( const HB_AST_TOKEN_STREAM * pStream, const char * pszPath );
HB_BYTE * hb_astTokenStreamSerializeSnapshotCbor( const HB_AST_TOKEN_STREAM * pStream, const char * pszModule, HB_SIZE * pnLength );
void hb_astTokenStreamSerializeSnapshotCborFree( HB_BYTE * pBuffer );
HB_BOOL hb_astTokenStreamWriteSnapshotCbor( const HB_AST_TOKEN_STREAM * pStream, const char * pszModule, const char * pszPath );
HB_BYTE * hb_astTokenStreamSerializeSymbolsCbor( const HB_AST_TOKEN_STREAM * pStream, const char * pszModule, HB_SIZE * pnLength );
void hb_astTokenStreamSerializeSymbolsCborFree( HB_BYTE * pBuffer );
const char * hb_astMacroTraceName( const void * pMacroTrace );
const char * hb_astMacroTraceCallModule( const void * pMacroTrace );
HB_AST_SOURCE_RANGE hb_astMacroTraceCallRange( const void * pMacroTrace );
HB_SIZE hb_astMacroTraceDepth( const void * pMacroTrace );
const void * hb_astMacroTraceParent( const void * pMacroTrace );
HB_SIZE hb_astMacroTraceId( const void * pMacroTrace );

HB_EXTERN_END

#endif /* HB_AST_LEXER_H_ */
