/*
 * AST trace fixture exercising preprocessor directives/commands (#define/#xcommand),
 * conditional compilation, and inline preprocessing expressions.
 */

#define DENSITY( weight, volume ) IIF( ( volume ) == 0, 0, ( weight ) / ( volume ) )
#define DIALECT_LABEL   "HARBOUR"

FUNCTION FixturePreprocDirectives( nWeight, nVolume )
   LOCAL lClipper
   LOCAL nDensity

   #ifdef __CLIPPER__
      lClipper := .T.
   #else
      lClipper := .F.
   #endif

   IF nWeight == NIL
      nWeight := 10
   ENDIF

   IF nVolume == NIL
      nVolume := 2
   ENDIF

   nDensity := DENSITY( nWeight, nVolume )
   IF lClipper
      nDensity := FixturePreprocDirectivesScale( nDensity + Len( "__CLIPPER__" ) )
      QOut( "__CLIPPER__", nDensity )
      RETURN { "__CLIPPER__", nDensity }
   ENDIF

   nDensity := FixturePreprocDirectivesScale( nDensity + Len( DIALECT_LABEL ) )
   QOut( DIALECT_LABEL, nDensity )
RETURN { DIALECT_LABEL, nDensity }

STATIC FUNCTION FixturePreprocDirectivesScale( nValue )
   #define SCALE_FACTOR( x ) ( ( x ) * 2 )

   RETURN SCALE_FACTOR( nValue )
