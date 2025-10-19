/*
 * AST trace fixture compiled under Clipper compatibility pragmas.
 */

#pragma -w3
#pragma -kh-
#pragma -km+
#pragma -ko-

#include "error.ch"
#include "fixture_compat_common.ch"

FUNCTION FixtureCompatClipper( xValue )
   LOCAL aLog := {}
   LOCAL nTotal := APPLY_NESTED_MACROS( 2 )
   LOCAL oErr := NIL
   LOCAL oBreak := NIL
   LOCAL oSecondary := NIL

   FixtureCompatLog( aLog, "compat:CLIPPER" )

   BEGIN SEQUENCE WITH {|oErr| FixtureCompatLog( aLog, "handler:" + ;
         IIf( oErr == NIL, "none", oErr:Description ) ) }
      nTotal += APPLY_NESTED_MACROS( 3 )
      nTotal += FixtureCompatCompute( xValue )
      IF HB_ISNUMERIC( xValue )
         oBreak := ErrorNew()
         oBreak:Description := "clipper numeric blocked"
         oBreak:GenCode := 2001
         BREAK oBreak
      ENDIF
   RECOVER USING oErr
   FixtureCompatLog( aLog, "recover:" + IIf( oErr == NIL, "none", oErr:Description ) )
ENDSEQUENCE

   BEGIN SEQUENCE WITH {|oErr| FixtureCompatLog( aLog, "alt:handler:" + ;
         IIf( oErr == NIL, "none", oErr:Description ) ) }
      oSecondary := ErrorNew()
      oSecondary:Description := "clipper alt break"
      oSecondary:GenCode := 2002
      BREAK oSecondary
   RECOVER USING oErr
      FixtureCompatLog( aLog, "alt:recover:" + IIf( oErr == NIL, "none", oErr:Description ) )
   ENDSEQUENCE

   FixtureCompatLog( aLog, "compat:CLIPPER:end" )

RETURN { "CLIPPER", nTotal, aLog }
