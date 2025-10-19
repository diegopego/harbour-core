#ifndef HB_AST_TRACE_H_
#define HB_AST_TRACE_H_

#include "hbapi.h"
#include "hbcompdf.h"
#include "hbpp.h"

HB_EXTERN_BEGIN

typedef enum
{
   HB_COMP_AST_TRACE_EVENT_TOKEN = 1,
   HB_COMP_AST_TRACE_EVENT_BOUNDARY = 2,
   HB_COMP_AST_TRACE_EVENT_PREPROCESSOR = 3
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

typedef struct _HB_COMP_AST_TRACE_PP_EVENT
{
   HB_SIZE                 sequence;
   const char *            ruleKind;
   const char *            macroName;
   const char *            callModule;
   int                     callLine;
   int                     callColumn;
   int                     callEndLine;
   int                     callEndColumn;
   HB_SIZE                 callOffset;
   HB_SIZE                 callEndOffset;
   HB_SIZE                 expansionId;
   const char *            source;
   const char *            result;
   const HB_PP_TRACEINFO * traceInfo;
} HB_COMP_AST_TRACE_PP_EVENT;

typedef enum
{
   HB_COMP_AST_NODE_FUNCTION = 1,
   HB_COMP_AST_NODE_CLASS = 2,
   HB_COMP_AST_NODE_CLASS_METHOD = 3,
   HB_COMP_AST_NODE_CLASS_DATA = 4,
   HB_COMP_AST_NODE_STATEMENT_IF = 10,
   HB_COMP_AST_NODE_STATEMENT_CASE = 11,
   HB_COMP_AST_NODE_STATEMENT_WHILE = 12,
   HB_COMP_AST_NODE_STATEMENT_FOR = 13,
   HB_COMP_AST_NODE_STATEMENT_FOREACH = 14,
   HB_COMP_AST_NODE_STATEMENT_SWITCH = 15,
   HB_COMP_AST_NODE_STATEMENT_WITH = 16,
   HB_COMP_AST_NODE_STATEMENT_SEQUENCE = 17,
   HB_COMP_AST_NODE_CODEBLOCK = 18
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
void    hb_compAstTracePublishPreprocessorEvent( PHB_COMP pComp, const HB_PP_TRACE_EVENT * pEvent );
HB_SIZE hb_compAstTraceTokenCount( const HB_COMP * pComp );
const HB_COMP_AST_TRACE_TOKEN * hb_compAstTraceToken( const HB_COMP * pComp, HB_SIZE index );
HB_SIZE hb_compAstTraceBoundaryCount( const HB_COMP * pComp );
const HB_COMP_AST_TRACE_BOUNDARY * hb_compAstTraceBoundary( const HB_COMP * pComp, HB_SIZE index );
HB_SIZE hb_compAstTracePpEventCount( const HB_COMP * pComp );
const HB_COMP_AST_TRACE_PP_EVENT * hb_compAstTracePpEvent( const HB_COMP * pComp, HB_SIZE index );
HB_SIZE hb_compAstTraceNodeEnter( PHB_COMP pComp, HB_COMP_AST_NODE_KIND kind, const void * handle, HB_SIZE tokenId );
void    hb_compAstTraceNodeLeave( PHB_COMP pComp, HB_COMP_AST_NODE_KIND kind, const void * handle );
void    hb_compAstTraceNodeLeaveById( PHB_COMP pComp, HB_COMP_AST_NODE_KIND kind, HB_SIZE nodeId );
void    hb_compAstTraceNodeEnterStack( PHB_COMP pComp, HB_COMP_AST_NODE_KIND kind, HB_SIZE tokenId );
void    hb_compAstTraceNodeLeaveStack( PHB_COMP pComp, HB_COMP_AST_NODE_KIND kind );
HB_SIZE hb_compAstTraceNodeCount( const HB_COMP * pComp );
const HB_COMP_AST_TRACE_NODE_EVENT * hb_compAstTraceNode( const HB_COMP * pComp, HB_SIZE index );
HB_SIZE hb_compAstTraceLastTokenId( const HB_COMP * pComp );
void    hb_compAstTraceClear( PHB_COMP pComp );

HB_EXTERN_END

#endif /* HB_AST_TRACE_H_ */
