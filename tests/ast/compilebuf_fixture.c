/*
 * Harbour 3.2.0dev (r2510040809)
 * GNU C 11.4 (64-bit)
 * Generated C source from "compilebuf_fixture.prg"
 */

#include "hbvmpub.h"
#include "hbinit.h"


HB_FUNC( DEMO );


HB_INIT_SYMBOLS_BEGIN( hb_vm_SymbolInit_COMPILEBUF_FIXTURE )
{ "DEMO", {HB_FS_PUBLIC | HB_FS_FIRST | HB_FS_LOCAL}, {HB_FUNCNAME( DEMO )}, NULL }
HB_INIT_SYMBOLS_EX_END( hb_vm_SymbolInit_COMPILEBUF_FIXTURE, "compilebuf_fixture.prg", 0x0, 0x0003 )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup hb_vm_SymbolInit_COMPILEBUF_FIXTURE
#elif defined( HB_DATASEG_STARTUP )
   #define HB_DATASEG_BODY    HB_DATASEG_FUNC( hb_vm_SymbolInit_COMPILEBUF_FIXTURE )
   #include "hbiniseg.h"
#endif

HB_FUNC( DEMO )
{
	static const HB_BYTE pcode[] =
	{
		13,1,0,36,2,0,122,80,1,36,3,0,95,1,
		110,7
	};

	hb_vmExecute( pcode, symbols );
}

