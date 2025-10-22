// --------------------------------------------------------------------
// Inline instrumentation fixture
// --------------------------------------------------------------------
//
// Exercises real-world INLINE usage so compiler instrumentation emits:
//   * HB_COMP_AST_NODE_INLINE events for class methods and standalone functions
//   * HB_COMP_AST_NODE_FUNCTION_INIT / _EXIT events for supporting routines
//

#pragma -ki+

#include "hbclass.ch"

CLASS InlineSample

   DATA cName INIT ""

   METHOD New( cName ) INLINE ( ::cName := cName, Self )
   METHOD Greet() INLINE QOut( "Hello, " + ::cName )
   METHOD Add( a, b ) INLINE ( a + b )

ENDCLASS

INIT PROCEDURE InlineStartup()

   LOCAL oSample := InlineSample():New( "Trace" )

   oSample:Greet()

   RETURN

EXIT PROCEDURE InlineShutdown()

   LOCAL oSample := InlineSample():New( "Trace" )

   QOut( "Sum result: " + hb_ntos( oSample:Add( 1, 2 ) ) )

   RETURN
