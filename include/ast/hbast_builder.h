/*
 * Harbour AST builder (prototype skeleton)
 *
 * Produces node and symbol collections from a token stream so that different
 * front-ends (JSON, CBOR, refactoring tools) share a common construction path.
 */

#ifndef HB_AST_BUILDER_H_
#define HB_AST_BUILDER_H_

#include "ast/lexer/hbast_lexer.h"

HB_EXTERN_BEGIN

typedef struct
{
   HB_SIZE id;
   char *  pszKind;
   HB_AST_SOURCE_RANGE range;
   char *  pszStableId;
   HB_SIZE parentId;
   HB_SIZE * pChildren;
   HB_SIZE nChildCount;
   HB_SIZE * pTokens;
   HB_SIZE nTokenCount;
   HB_SIZE symbolId;
} HB_AST_NODE_INFO;

typedef struct
{
   HB_SIZE symbolId;
   char *  pszKind;
   char *  pszName;
   char *  pszQualifiedName;
   HB_SIZE * pDeclarations;
   HB_SIZE nDeclarationCount;
   HB_SIZE * pReferences;
   HB_SIZE nReferenceCount;
} HB_AST_SYMBOL_INFO;

typedef struct
{
   HB_AST_NODE_INFO * pNodes;
   HB_SIZE nNodeCount;
   HB_AST_SYMBOL_INFO * pSymbols;
   HB_SIZE nSymbolCount;
   HB_SIZE nRootId;
} HB_AST_BUILD_RESULT;

HB_BOOL hb_astBuildFromStream( const HB_AST_TOKEN_STREAM * pStream,
                               const char * pszModule,
                               HB_AST_BUILD_RESULT * pResult );
void hb_astBuildResultRelease( HB_AST_BUILD_RESULT * pResult );

HB_EXTERN_END

#endif /* HB_AST_BUILDER_H_ */
