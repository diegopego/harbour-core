/*
 * AST trace fixture covering declaration statements (REQUEST, ANNOUNCE, FIELD, MEMVAR, STATIC, THREAD STATIC).
 */

REQUEST HB_GT_STD
ANNOUNCE HB_GT_WIN

FIELD fieldValue
MEMVAR gShared

STATIC sCounter := 0
THREAD STATIC tsLastDriver

PROCEDURE FixtureStatements( cDriver )
   FIELD fieldValue
   MEMVAR gShared

   IF Empty( cDriver )
      cDriver := "HB_GT_STD"
   ENDIF

   sCounter++
   tsLastDriver := cDriver
   gShared := sCounter
   fieldValue := gShared

   QOut( "Driver:", cDriver )
   QOut( "Counter:", sCounter )
RETURN

FUNCTION FixtureStatementCounts()
   RETURN { sCounter, tsLastDriver }
