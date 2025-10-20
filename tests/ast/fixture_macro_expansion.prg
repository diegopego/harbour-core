#pragma -w3

#define LEAF() 42
#define MIDDLE() LEAF()
#define TOP() MIDDLE()

FUNCTION FixtureMacroExpansion()
   LOCAL n := TOP()
   RETURN n

