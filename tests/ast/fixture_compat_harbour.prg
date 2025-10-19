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
   LOCAL oInner := NIL

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
   FixtureCompatLog( aLog, "after:outer" )

   BEGIN SEQUENCE WITH {|oErr| FixtureCompatLog( aLog, "nested:handler:" + ;
         IIf( oErr == NIL, "none", oErr:Description ) ) }
      FixtureCompatLog( aLog, "nested:enter" )
      BEGIN SEQUENCE WITH {|oInner| FixtureCompatLog( aLog, "nested:innerhandler:" + ;
            IIf( oInner == NIL, "none", oInner:Description ) ) }
         FixtureCompatLog( aLog, "nested:work" )
         oInner := ErrorNew()
         oInner:Description := "harbour nested break"
         oInner:GenCode := 3001
         BREAK oInner
      RECOVER USING oInner
         FixtureCompatLog( aLog, "nested:innerrecover:" + IIf( oInner == NIL, "none", oInner:Description ) )
         oInner:Description := oInner:Description + ":reraised"
         BREAK oInner
      ENDSEQUENCE
      FixtureCompatLog( aLog, "nested:innerexit" )
   RECOVER USING oErr
      FixtureCompatLog( aLog, "nested:recover:" + IIf( oErr == NIL, "none", oErr:Description ) )
   ENDSEQUENCE
   FixtureCompatLog( aLog, "nested:outerexit" )

   FixtureCompatLog( aLog, "compat:HARBOUR:end" )

RETURN { "HARBOUR", nTotal, aLog }
