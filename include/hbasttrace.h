#ifndef HB_AST_TRACE_H_
#define HB_AST_TRACE_H_

#include "hbapi.h"
#include "hbcompdf.h"
#include "hbpp.h"

HB_EXTERN_BEGIN

typedef enum
{
   HB_COMP_AST_TRACE_EVENT_TOKEN = 1,
   HB_COMP_AST_TRACE_EVENT_BOUNDARY = 2
} HB_COMP_AST_TRACE_EVENT_TYPE;

typedef struct _HB_COMP_AST_TRACE_TOKEN
{
   HB_SIZE                 id;
   HB_SIZE                 sequence;
   HB_SIZE                 spaces;
   HB_SIZE                 length;
   HB_USHORT               type;
   HB_USHORT               markerIndex;
   const char *            value;
   const char *            module;
   int                     line;
   int                     column;
   int                     endColumn;
   HB_SIZE                 offset;
   HB_SIZE                 endOffset;
   const HB_PP_TRACEINFO * traceInfo;
} HB_COMP_AST_TRACE_TOKEN;

typedef struct _HB_COMP_AST_TRACE_BOUNDARY
{
   HB_SIZE  sequence;
   HB_SIZE  tokenId;
   int      code;
   int      lexState;
} HB_COMP_AST_TRACE_BOUNDARY;

typedef enum
{
   HB_COMP_AST_NODE_FUNCTION = 1
} HB_COMP_AST_NODE_KIND;

typedef enum
{
   HB_COMP_AST_NODE_EVENT_ENTER = 1,
   HB_COMP_AST_NODE_EVENT_LEAVE = 2
} HB_COMP_AST_NODE_EVENT_TYPE;

typedef struct _HB_COMP_AST_TRACE_NODE_EVENT
{
   HB_SIZE                        id;
   HB_SIZE                        sequence;
   HB_COMP_AST_NODE_KIND          kind;
   HB_COMP_AST_NODE_EVENT_TYPE    phase;
   HB_SIZE                        tokenId;
   const char *                   name;
   const void *                   handle;
} HB_COMP_AST_TRACE_NODE_EVENT;

void    hb_compAstTraceInit( PHB_COMP pComp );
void    hb_compAstTraceShutdown( PHB_COMP pComp );
void    hb_compAstTraceSetEnabled( PHB_COMP pComp, HB_BOOL fEnabled );
HB_BOOL hb_compAstTraceIsEnabled( const HB_COMP * pComp );
void    hb_compAstTraceRetainInfo( PHB_COMP pComp, PHB_PP_TRACEINFO pTraceInfo );
void    hb_compAstTraceReleaseInfo( PHB_COMP pComp, PHB_PP_TRACEINFO pTraceInfo );
HB_SIZE hb_compAstTraceOutstandingTraceinfo( const HB_COMP * pComp );
void    hb_compAstTracePublishToken( PHB_COMP pComp, const PHB_PP_TOKEN pToken );
void    hb_compAstTracePublishBoundary( PHB_COMP pComp, int code, int lexState );
HB_SIZE hb_compAstTraceTokenCount( const HB_COMP * pComp );
const HB_COMP_AST_TRACE_TOKEN * hb_compAstTraceToken( const HB_COMP * pComp, HB_SIZE index );
HB_SIZE hb_compAstTraceBoundaryCount( const HB_COMP * pComp );
const HB_COMP_AST_TRACE_BOUNDARY * hb_compAstTraceBoundary( const HB_COMP * pComp, HB_SIZE index );
void    hb_compAstTraceNodeEnter( PHB_COMP pComp, HB_COMP_AST_NODE_KIND kind, const void * handle, HB_SIZE tokenId );
void    hb_compAstTraceNodeLeave( PHB_COMP pComp, HB_COMP_AST_NODE_KIND kind, const void * handle );
HB_SIZE hb_compAstTraceNodeCount( const HB_COMP * pComp );
const HB_COMP_AST_TRACE_NODE_EVENT * hb_compAstTraceNode( const HB_COMP * pComp, HB_SIZE index );
HB_SIZE hb_compAstTraceLastTokenId( const HB_COMP * pComp );
void    hb_compAstTraceClear( PHB_COMP pComp );

HB_EXTERN_END

#endif /* HB_AST_TRACE_H_ */
