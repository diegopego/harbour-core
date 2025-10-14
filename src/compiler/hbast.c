/*
 * Harbour Abstract Syntax Tree support
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

#include "hbast.h"

#include <string.h>

#define HB_AST_CHILD_HINT 4

static void hb_astPayloadClear( HB_AST_PAYLOAD * pPayload );
static void hb_astNodeEnsureCapacity( PHB_AST_NODE pNode, HB_SIZE nNeeded );

HB_AST_SPAN hb_astSpanInit( const char * pszFile, int iLine, int iColumn )
{
   HB_AST_SPAN span;

   span.pszFile    = pszFile;
   span.iLine      = iLine;
   span.iColumn    = iColumn;
   span.iEndLine   = iLine;
   span.iEndColumn = iColumn;

   return span;
}

PHB_AST hb_astNew( PHB_COMP pOwner )
{
   PHB_AST pAst = ( PHB_AST ) hb_xgrabz( sizeof( HB_AST ) );

   pAst->pOwner = pOwner;
   pAst->nextNodeId = 1;

   return pAst;
}

static void hb_astNodeRelease( PHB_AST_NODE pNode )
{
   HB_SIZE nIndex;

   if( ! pNode )
      return;

   for( nIndex = 0; nIndex < pNode->nChildren; ++nIndex )
   {
      hb_astNodeRelease( pNode->pChildren[ nIndex ] );
   }

   if( pNode->pChildren )
      hb_xfree( pNode->pChildren );

   hb_astPayloadClear( &pNode->payload );

   hb_xfree( pNode );
}

void hb_astFree( PHB_AST pAst )
{
   if( ! pAst )
      return;

   hb_astNodeRelease( pAst->pRoot );
   hb_xfree( pAst );
}

PHB_AST_NODE hb_astNodeNew( PHB_AST pAst, HB_AST_KIND kind )
{
   PHB_AST_NODE pNode = ( PHB_AST_NODE ) hb_xgrabz( sizeof( HB_AST_NODE ) );

   pNode->kind = kind;
   pNode->payload.kind = HB_AST_PAYLOAD_NONE;
   pNode->nodeId = pAst ? pAst->nextNodeId++ : 0;

   return pNode;
}

void hb_astNodeFree( PHB_AST_NODE pNode )
{
   if( ! pNode )
      return;

   hb_astNodeDetach( pNode );
   hb_astNodeRelease( pNode );
}

void hb_astNodeSetSpan( PHB_AST_NODE pNode, const HB_AST_SPAN * pSpan )
{
   if( ! pNode || ! pSpan )
      return;

   pNode->span = *pSpan;
}

static void hb_astNodeEnsureCapacity( PHB_AST_NODE pNode, HB_SIZE nNeeded )
{
   HB_SIZE nAlloc;

   if( nNeeded <= pNode->nAllocated )
      return;

   nAlloc = pNode->nAllocated ? pNode->nAllocated : HB_AST_CHILD_HINT;

   while( nAlloc < nNeeded )
   {
      HB_SIZE nNext = nAlloc << 1;

      if( nNext <= nAlloc || nNext > HB_SIZE_MAX )
      {
         nAlloc = nNeeded;
         break;
      }
      nAlloc = nNext;
   }

   if( pNode->pChildren )
      pNode->pChildren = ( PHB_AST_NODE * ) hb_xrealloc( pNode->pChildren, sizeof( PHB_AST_NODE ) * nAlloc );
   else
      pNode->pChildren = ( PHB_AST_NODE * ) hb_xgrab( sizeof( PHB_AST_NODE ) * nAlloc );

   pNode->nAllocated = nAlloc;
}

void hb_astNodeAddChild( PHB_AST_NODE pParent, PHB_AST_NODE pChild )
{
   if( ! pParent || ! pChild )
      return;

   if( pChild->pParent == pParent )
      return;

   if( pChild->pParent )
      hb_astNodeDetach( pChild );

   hb_astNodeEnsureCapacity( pParent, pParent->nChildren + 1 );

   pParent->pChildren[ pParent->nChildren++ ] = pChild;
   pChild->pParent = pParent;
}

void hb_astNodeDetach( PHB_AST_NODE pNode )
{
   PHB_AST_NODE pParent;
   HB_SIZE nIndex;

   if( ! pNode || ! pNode->pParent )
      return;

   pParent = pNode->pParent;

   for( nIndex = 0; nIndex < pParent->nChildren; ++nIndex )
   {
      if( pParent->pChildren[ nIndex ] == pNode )
      {
         if( nIndex + 1 < pParent->nChildren )
         {
            memmove( &pParent->pChildren[ nIndex ],
                     &pParent->pChildren[ nIndex + 1 ],
                     ( pParent->nChildren - nIndex - 1 ) * sizeof( PHB_AST_NODE ) );
         }
         --pParent->nChildren;
         break;
      }
   }

   pNode->pParent = NULL;
}

static void hb_astPayloadClear( HB_AST_PAYLOAD * pPayload )
{
   if( ! pPayload )
      return;

   if( pPayload->kind == HB_AST_PAYLOAD_STRING )
   {
      if( pPayload->data.string.fOwned && pPayload->data.string.pszValue )
         hb_xfree( pPayload->data.string.pszValue );
   }
   else if( pPayload->kind == HB_AST_PAYLOAD_SYMBOL )
   {
      if( pPayload->data.symbol.fOwned && pPayload->data.symbol.pszName )
         hb_xfree( ( void * ) pPayload->data.symbol.pszName );
   }

   memset( pPayload, 0, sizeof( HB_AST_PAYLOAD ) );
   pPayload->kind = HB_AST_PAYLOAD_NONE;
}

void hb_astNodeSetInteger( PHB_AST_NODE pNode, HB_MAXINT value )
{
   if( ! pNode )
      return;

   hb_astPayloadClear( &pNode->payload );

   pNode->payload.kind = HB_AST_PAYLOAD_INTEGER;
   pNode->payload.data.integer = value;
}

void hb_astNodeSetDouble( PHB_AST_NODE pNode, double value )
{
   if( ! pNode )
      return;

   hb_astPayloadClear( &pNode->payload );

   pNode->payload.kind = HB_AST_PAYLOAD_DOUBLE;
   pNode->payload.data.number = value;
}

void hb_astNodeSetLogical( PHB_AST_NODE pNode, HB_BOOL value )
{
   if( ! pNode )
      return;

   hb_astPayloadClear( &pNode->payload );

   pNode->payload.kind = HB_AST_PAYLOAD_LOGICAL;
   pNode->payload.data.logical = value;
}

void hb_astNodeSetString( PHB_AST_NODE pNode, const char * pszValue, HB_SIZE nLen, HB_BOOL fCopy )
{
   if( ! pNode )
      return;

   hb_astPayloadClear( &pNode->payload );

   pNode->payload.kind = HB_AST_PAYLOAD_STRING;

   if( ! pszValue )
   {
      pNode->payload.data.string.pszValue = NULL;
      pNode->payload.data.string.nLen = 0;
      pNode->payload.data.string.fOwned = HB_FALSE;
      return;
   }

   if( fCopy )
   {
      char * pszCopy = ( char * ) hb_xgrab( nLen + 1 );

      if( nLen )
         memcpy( pszCopy, pszValue, nLen );
      pszCopy[ nLen ] = '\0';

      pNode->payload.data.string.pszValue = pszCopy;
      pNode->payload.data.string.fOwned = HB_TRUE;
   }
   else
   {
      pNode->payload.data.string.pszValue = ( char * ) pszValue;
      pNode->payload.data.string.fOwned = HB_FALSE;
   }

   pNode->payload.data.string.nLen = nLen;
}

void hb_astNodeSetSymbol( PHB_AST_NODE pNode, const char * pszSymbol, HB_BOOL fCopy )
{
   if( ! pNode )
      return;

   hb_astPayloadClear( &pNode->payload );

   pNode->payload.kind = HB_AST_PAYLOAD_SYMBOL;

   if( pszSymbol && fCopy )
   {
      pNode->payload.data.symbol.pszName = hb_strdup( pszSymbol );
      pNode->payload.data.symbol.fOwned = HB_TRUE;
   }
   else
   {
      pNode->payload.data.symbol.pszName = pszSymbol;
      pNode->payload.data.symbol.fOwned = HB_FALSE;
   }
}

void hb_astNodeSetPointer( PHB_AST_NODE pNode, void * pPtr )
{
   if( ! pNode )
      return;

   hb_astPayloadClear( &pNode->payload );

   pNode->payload.kind = HB_AST_PAYLOAD_POINTER;
   pNode->payload.data.pointer = pPtr;
}

PHB_AST_NODE hb_astGetRoot( PHB_AST pAst )
{
   return pAst ? pAst->pRoot : NULL;
}

void hb_astSetRoot( PHB_AST pAst, PHB_AST_NODE pRoot )
{
   if( ! pAst )
      return;

   if( pAst->pRoot == pRoot )
      return;

   if( pRoot )
   {
      hb_astNodeDetach( pRoot );
      pRoot->pParent = NULL;
   }

   pAst->pRoot = pRoot;
}
