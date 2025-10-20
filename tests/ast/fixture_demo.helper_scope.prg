#include "fixture_helpers.ch"
#include "fixture_extrahelpers.ch"

FUNCTION Demo()
   LOCAL n := VALUE
   RETURN Helper() + n

FUNCTION Outer()
   LOCAL cNameScoped := MODULE_NAME()
   InnerProc()
   RETURN Helper() + Len( cNameScoped )

FUNCTION Another()
   LOCAL cName := "scoped literal"
   RETURN cName

STATIC FUNCTION InnerProc()
   LOCAL nCount := INLINE_HELPER()
   RETURN nCount

FUNCTION Exported()
   RETURN INLINE_HELPER()

PROCEDURE CallIncludedProc()
   IncludedProc()
   RETURN
