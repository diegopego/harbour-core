#include "fixture_helpers.ch"
#include "fixture_extrahelpers.ch"

FUNCTION Demo()
   LOCAL n := VALUE
   RETURN Helper() + n

FUNCTION Outer()
   LOCAL cName := MODULE_NAME()
   InnerProc()
   RETURN Helper() + Len( cName )

STATIC FUNCTION InnerProc()
   LOCAL nCount := INLINE_HELPER()
   RETURN nCount

FUNCTION Exported()
   RETURN INLINE_HELPER()

PROCEDURE CallIncludedProc()
   IncludedProc()
   RETURN