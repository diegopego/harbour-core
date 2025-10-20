FUNCTION RefMain()
   SupportFunc()
   RETURN NIL

FUNCTION RefAnotherCaller()
   LOCAL cResult := SupportFunc()
   RETURN cResult
