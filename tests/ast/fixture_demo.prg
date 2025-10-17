#include "fixture_helpers.ch"
#include "fixture_extrahelpers.ch"

PROC Demo()
   LOCAL n := VALUE
   RETURN Helper() + n

PROC Outer()
   LOCAL cName := MODULE_NAME()
   InnerProc()
   RETURN Helper() + Len( cName )

STATIC PROC InnerProc()
   LOCAL nCount := INLINE_HELPER()
   RETURN nCount

FUNCTION Exported()
   RETURN INLINE_HELPER()
