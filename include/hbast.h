/*
 * Harbour Abstract Syntax Tree public structures
 *
 * Copyright 2024
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

#ifndef HB_AST_H_
#define HB_AST_H_

#include "hbapi.h"
#include "hbcompdf.h"

HB_EXTERN_BEGIN

typedef enum
{
   HB_AST_NODE_UNKNOWN = 0,
   HB_AST_NODE_PROGRAM,
   HB_AST_NODE_DECLARATION,
   HB_AST_NODE_FUNCTION,
   HB_AST_NODE_PROCEDURE,
   HB_AST_NODE_PARAMETER,
   HB_AST_NODE_STATEMENT_LIST,
   HB_AST_NODE_ASSIGNMENT,
   HB_AST_NODE_CALL,
   HB_AST_NODE_RETURN,
   HB_AST_NODE_IF,
   HB_AST_NODE_ELSEIF,
   HB_AST_NODE_ELSE,
   HB_AST_NODE_WHILE,
   HB_AST_NODE_FOR,
   HB_AST_NODE_FOREACH,
   HB_AST_NODE_DO,
   HB_AST_NODE_SWITCH,
   HB_AST_NODE_CASE,
   HB_AST_NODE_DEFAULT,
   HB_AST_NODE_TRY,
   HB_AST_NODE_CATCH,
   HB_AST_NODE_FINALLY,
   HB_AST_NODE_BREAK,
   HB_AST_NODE_CONTINUE,
   HB_AST_NODE_LITERAL,
   HB_AST_NODE_IDENTIFIER,
   HB_AST_NODE_UNARY_OP,
   HB_AST_NODE_BINARY_OP,
   HB_AST_NODE_TERNARY_OP,
   HB_AST_NODE_ARRAY,
   HB_AST_NODE_HASH,
   HB_AST_NODE_INDEX,
   HB_AST_NODE_FIELD,
   HB_AST_NODE_WITH,
   HB_AST_NODE_CLASS,
   HB_AST_NODE_METHOD,
   HB_AST_NODE_ATTRIBUTE,
   HB_AST_NODE_PRAGMA
} HB_AST_KIND;

typedef enum
{
   HB_AST_PAYLOAD_NONE = 0,
   HB_AST_PAYLOAD_INTEGER,
   HB_AST_PAYLOAD_DOUBLE,
   HB_AST_PAYLOAD_LOGICAL,
   HB_AST_PAYLOAD_STRING,
   HB_AST_PAYLOAD_SYMBOL,
   HB_AST_PAYLOAD_POINTER
} HB_AST_PAYLOAD_KIND;

typedef struct
{
   const char * pszFile;
   int          iLine;
   int          iColumn;
   int          iEndLine;
   int          iEndColumn;
} HB_AST_SPAN;

typedef struct
{
   char *   pszValue;
   HB_SIZE  nLen;
   HB_BOOL  fOwned;
} HB_AST_STRING;

typedef struct
{
   const char * pszName;
   HB_BOOL      fOwned;
} HB_AST_SYMBOL;

typedef struct
{
   HB_AST_PAYLOAD_KIND kind;
   union
   {
      HB_MAXINT    integer;
      double       number;
      HB_BOOL      logical;
      HB_AST_STRING string;
      HB_AST_SYMBOL symbol;
      void *       pointer;
   } data;
} HB_AST_PAYLOAD;

typedef struct _HB_AST_NODE
{
   HB_AST_KIND         kind;
   HB_AST_SPAN         span;
   HB_AST_PAYLOAD      payload;
   HB_SIZE             nodeId;
   struct _HB_AST_NODE * pParent;
   struct _HB_AST_NODE ** pChildren;
   HB_SIZE             nChildren;
   HB_SIZE             nAllocated;
} HB_AST_NODE;
typedef HB_AST_NODE * PHB_AST_NODE;

typedef struct _HB_AST
{
   PHB_COMP     pOwner;
   HB_AST_NODE * pRoot;
   HB_SIZE      nextNodeId;
} HB_AST;
typedef HB_AST * PHB_AST;

HB_EXPORT_INT PHB_AST       hb_astNew( PHB_COMP pOwner );
HB_EXPORT_INT void          hb_astFree( PHB_AST pAst );

HB_EXPORT_INT PHB_AST_NODE  hb_astNodeNew( PHB_AST pAst, HB_AST_KIND kind );
HB_EXPORT_INT void          hb_astNodeFree( PHB_AST_NODE pNode );

HB_EXPORT_INT void          hb_astNodeSetSpan( PHB_AST_NODE pNode, const HB_AST_SPAN * pSpan );
HB_EXPORT_INT void          hb_astNodeAddChild( PHB_AST_NODE pParent, PHB_AST_NODE pChild );
HB_EXPORT_INT void          hb_astNodeDetach( PHB_AST_NODE pNode );

HB_EXPORT_INT void          hb_astNodeSetInteger( PHB_AST_NODE pNode, HB_MAXINT value );
HB_EXPORT_INT void          hb_astNodeSetDouble( PHB_AST_NODE pNode, double value );
HB_EXPORT_INT void          hb_astNodeSetLogical( PHB_AST_NODE pNode, HB_BOOL value );
HB_EXPORT_INT void          hb_astNodeSetString( PHB_AST_NODE pNode, const char * pszValue, HB_SIZE nLen, HB_BOOL fCopy );
HB_EXPORT_INT void          hb_astNodeSetSymbol( PHB_AST_NODE pNode, const char * pszSymbol, HB_BOOL fCopy );
HB_EXPORT_INT void          hb_astNodeSetPointer( PHB_AST_NODE pNode, void * pPtr );

HB_EXPORT_INT PHB_AST_NODE  hb_astGetRoot( PHB_AST pAst );
HB_EXPORT_INT void          hb_astSetRoot( PHB_AST pAst, PHB_AST_NODE pRoot );

HB_EXPORT_INT HB_AST_SPAN   hb_astSpanInit( const char * pszFile, int iLine, int iColumn );

HB_EXTERN_END

#endif /* HB_AST_H_ */
