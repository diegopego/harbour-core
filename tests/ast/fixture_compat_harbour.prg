/*
 * AST trace fixture compiled under Harbour compatibility pragmas.
 */

#pragma -w3
#pragma -kh+
#pragma -km-
#pragma -ko+

#include "error.ch"
#include "fixture_compat_common.ch"

FUNCTION FixtureCompatHarbour( xValue )
   LOCAL aLog := {}
   LOCAL nTotal := APPLY_NESTED_MACROS( 5 )
   LOCAL oErr := NIL

   FixtureCompatLog( aLog, "compat:HARBOUR" )

   BEGIN SEQUENCE WITH {|oErr| FixtureCompatLog( aLog, "handler:" + ;
         IIf( oErr == NIL, "none", oErr:Description ) ) }
      IF HB_ISSTRING( xValue )
         nTotal += Len( Upper( xValue ) )
      ELSE
         nTotal += FixtureCompatCompute( xValue )
      ENDIF
   RECOVER USING oErr
      FixtureCompatLog( aLog, "recover:" + IIf( oErr == NIL, "none", oErr:Description ) )
   ENDSEQUENCE

   FixtureCompatLog( aLog, "compat:HARBOUR:end" )

RETURN { "HARBOUR", nTotal, aLog }
