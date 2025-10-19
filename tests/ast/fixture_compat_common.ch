#ifndef __FIXTURE_COMPAT_COMMON_CH
#define __FIXTURE_COMPAT_COMMON_CH

#define APPLY_NESTED_MACROS( expr ) __FIXTURE_APPLY_NESTED_MACROS( expr )
#define __FIXTURE_APPLY_NESTED_MACROS( expr ) ( ( expr ) + __FIXTURE_APPLY_INNER( expr ) )
#define __FIXTURE_APPLY_INNER( expr ) ( ( expr ) * 2 )

STATIC FUNCTION FixtureCompatLog( aLog, cMessage )
   AAdd( aLog, "TRACE=>" + cMessage )
RETURN NIL

STATIC FUNCTION FixtureCompatCompute( xValue )
   DO CASE
   CASE HB_ISSTRING( xValue )
      RETURN APPLY_NESTED_MACROS( Len( xValue ) )
   CASE HB_ISBLOCK( xValue )
      RETURN APPLY_NESTED_MACROS( Eval( xValue ) )
   CASE HB_ISNUMERIC( xValue )
      RETURN APPLY_NESTED_MACROS( xValue * 2 )
   OTHERWISE
      RETURN APPLY_NESTED_MACROS( 1 )
   ENDCASE

RETURN APPLY_NESTED_MACROS( 0 )

#endif /* __FIXTURE_COMPAT_COMMON_CH */
