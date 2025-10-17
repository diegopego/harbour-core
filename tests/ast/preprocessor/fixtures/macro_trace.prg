/*
 * Harbour preprocessor macro trace fixture.
 *
 * Exercises nested #define expansions together with a #translate rule
 * so that instrumentation can reason about chained macro origins.
 */

#define VALUE()              3
#define DOUBLE( x )          ( ( x ) * 2 )
#define ADD_AND_DOUBLE( x, y ) DOUBLE( ( x ) + ( y ) )
#translate TRACE <expr>       => __TraceEval( <expr>, __LINE__ )

STATIC FUNCTION __TraceEval( xValue, nLine )
   /* During tests we just fold the arguments to keep compilation simple. */
   RETURN xValue + nLine

PROCEDURE Main()
   LOCAL nResult := ADD_AND_DOUBLE( 5, VALUE() )
   LOCAL nTrace := TRACE ADD_AND_DOUBLE( 1, VALUE() )

   ? nResult, nTrace
RETURN

