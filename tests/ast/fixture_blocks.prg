FUNCTION FixtureBlocks( nInitial )
   LOCAL n := IIF( nInitial == NIL, 0, nInitial )
   LOCAL aValues := { 1, 2, 3 }
   LOCAL xItem

   IF n > 3
      n -= 2
   ELSEIF n == 0
      n++
   ELSE
      n += Len( aValues )
   ENDIF

   FOR EACH xItem IN aValues
      IF ValType( xItem ) == "N"
         n += xItem
      ENDIF
   NEXT

   DO WHILE n > 0
      n--
      EXIT
   ENDDO

   RETURN n
