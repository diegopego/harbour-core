/*
 * Harbour preprocessor command trace fixture.
 *
 * Covers #command / #xcommand rules with optional parameter lists so
 * instrumentation can validate rewrite metadata coming from command-style
 * macros.
 */

#xcommand DISPATCH <name> USING <args,...> => __Dispatch__( "<name>", { <args> }, __LINE__ )
#command  DISPATCH <name>                  => __Dispatch__( "<name>", {}, __LINE__ )

STATIC FUNCTION __Dispatch__( cName, aArgs, nLine )
   RETURN { cName, aArgs, nLine }

PROCEDURE Main()
   DISPATCH Alpha
   DISPATCH Beta USING 1, 2, 3
RETURN
