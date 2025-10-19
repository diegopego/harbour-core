#pragma -w3
#include "fixture_helpers.ch"

STATIC FUNCTION ExprSequence( nValue, cName )
   LOCAL nOffset := ( nValue + 2 ) * 5 - Max( nValue, VALUE )
   LOCAL cOut := Upper( cName ) + ":" + LTrim( Str( nOffset ) )
   LOCAL aItems := { 1, 2, 3 }
   LOCAL bEval := {|nInput| ( nInput + nOffset ) / 2 }

   aItems[ 1 ] := IIf( nOffset > 0, nOffset * nOffset, -1 )
   RETURN aItems[ 1 ] + Eval( bEval, Len( cOut ) )

FUNCTION FixtureExpressions()
   LOCAL hMap := {=>}
   LOCAL nVal := ExprSequence( VALUE / 2, "alpha" )
   LOCAL cKey := "left" + LTrim( Str( Mod( nVal, 4 ) ) )
   LOCAL lReady := .T.
   LOCAL nResult

   hMap[ cKey ] := nVal + VALUE
   hMap[ "right" ] := ( nVal / 3 ) - ( VALUE / 4 )
   hMap[ "flags" ] := { lReady, nVal > VALUE, LTrim( Str( nVal ) ) $ "alphaALPHA" }

   IF ( hMap[ cKey ] > hMap[ "right" ] ) .AND. lReady
      nResult := hMap[ cKey ] + hMap[ "right" ]
   ELSE
      nResult := VALUE - nVal
   ENDIF

   RETURN IIf( ( lReady .AND. ! Empty( hMap ) ) .OR. nResult == 0, nResult, nVal )
