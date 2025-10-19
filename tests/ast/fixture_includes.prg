#pragma -w3
#include "error.ch"
#include "fixture_include_chain.ch"

#define FEATURE_FLAG .T.
#define FEATURE_NAME "include:primary"

STATIC PROCEDURE AppendTag( aLog, cLabel )
   ChainTouch( aLog, INCLUDE_CHAIN_LABEL( cLabel ) )
   RETURN

FUNCTION FixtureIncludes()
   LOCAL aLog := {}
   LOCAL bAppend := {|cLabel| AppendTag( aLog, cLabel ) }
   LOCAL cHeader := IIf( FEATURE_FLAG, FEATURE_NAME, "include:disabled" )

   AppendTag( aLog, cHeader )
   Eval( bAppend, "callback" )
   IncludedProc()

   RETURN aLog
