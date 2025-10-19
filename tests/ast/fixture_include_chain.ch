#ifndef FIXTURE_INCLUDE_CHAIN_CH
#define FIXTURE_INCLUDE_CHAIN_CH

#include "fixture_helpers.ch"
#include "fixture_extrahelpers.ch"

#undef MODULE_NAME
#define MODULE_NAME() "tests/ast/fixture_includes.prg"

#define INCLUDE_CHAIN_LABEL( suffix ) MODULE_NAME() + ":" + suffix

STATIC PROCEDURE ChainTouch( aLog, cLabel )
   AAdd( aLog, { cLabel, VALUE, INLINE_HELPER() } )
   RETURN

#endif /* FIXTURE_INCLUDE_CHAIN_CH */
