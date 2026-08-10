/*
 * Compile time RTL argument checking
 *
 * Copyright 1999 Jose Lalin <dezac@corevia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * (or visit their website at https://www.gnu.org/licenses/).
 *
 */

#include "hbcomp.h"

/* NOTE: iMinParam = -1, means no lower limit
 *       iMaxParam = -1, means no upper limit
 */

typedef struct
{
   const char * cFuncName;                /* function name                   */
   int          iMinParam;                /* min number of parameters needed */
   int          iMaxParam;                /* max number of parameters needed */
} HB_FUNCINFO, * PHB_FUNCINFO;


/* NOTE: THIS TABLE MUST BE SORTED ALPHABETICALLY
 */
static const HB_FUNCINFO s_stdFunc[] =
{
   { "AADD"      , 2,  2 },
   { "ABS"       , 1,  1 },
   { "ASC"       , 1,  1 },
   { "AT"        , 2,  2 },
   { "BOF"       , 0,  0 },
   { "BREAK"     , 0,  1 },
   { "CDOW"      , 1,  1 },
   { "CHR"       , 1,  1 },
   { "CMONTH"    , 1,  1 },
   { "COL"       , 0,  0 },
   { "CTOD"      , 1,  1 },
   { "DATE"      , 0,  0 },
   { "DAY"       , 1,  1 },
   { "DELETED"   , 0,  0 },
   { "DEVPOS"    , 2,  2 },
   { "DOW"       , 1,  1 },
   { "DTOC"      , 1,  1 },
   { "DTOS"      , 1,  1 },
   { "EMPTY"     , 1,  1 },
   { "EOF"       , 0,  0 },
   { "EVAL"      , 1, -1 },
   { "EXP"       , 1,  1 },
   { "FCOUNT"    , 0,  0 },
   { "FIELDNAME" , 1,  1 },
   { "FILE"      , 1,  1 },
   { "FLOCK"     , 0,  0 },
   { "FOUND"     , 0,  0 },
   { "INKEY"     , 0,  2 },
   { "INT"       , 1,  1 },
   { "LASTREC"   , 0,  0 },
   { "LEFT"      , 2,  2 },
   { "LEN"       , 1,  1 },
   { "LOCK"      , 0,  0 },
   { "LOG"       , 1,  1 },
   { "LOWER"     , 1,  1 },
   { "LTRIM"     , 1,  1 },
   { "MAX"       , 2,  2 },
   { "MIN"       , 2,  2 },
   { "MONTH"     , 1,  1 },
   { "PCOL"      , 0,  0 },
   { "PCOUNT"    , 0,  0 },
   { "PROW"      , 0,  0 },
   { "QSELF"     , 0,  0 },
   { "RECCOUNT"  , 0,  0 },
   { "RECNO"     , 0,  0 },
   { "REPLICATE" , 2,  2 },
   { "RLOCK"     , 0,  0 },
   { "ROUND"     , 2,  2 },
   { "ROW"       , 0,  0 },
   { "RTRIM"     , 1,  1 },
   { "SECONDS"   , 0,  0 },
   { "SELECT"    , 0,  1 },
   { "SETPOS"    , 2,  2 },
   { "SETPOSBS"  , 0,  0 },
   { "SPACE"     , 1,  1 },
   { "SQRT"      , 1,  1 },
   { "STR"       , 1,  3 },
   { "SUBSTR"    , 2,  3 },
   { "TIME"      , 0,  0 },
   { "TRANSFORM" , 2,  2 },
   { "TRIM"      , 1,  1 },
   { "TYPE"      , 1,  1 },
   { "UPPER"     , 1,  1 },
   { "VAL"       , 1,  1 },
   { "VALTYPE"   , 1,  1 },
   { "WORD"      , 1,  1 },
   { "YEAR"      , 1,  1 }
};

HB_BOOL hb_compFunCallCheck( HB_COMP_DECL, const char * szFuncCall, int iArgs )
{
   unsigned int uiFirst = 0, uiLast = HB_SIZEOFARRAY( s_stdFunc ) - 1, uiMiddle;
   int iLen = ( int ) strlen( szFuncCall ), iCmp;

   /* Respect 4 or more letters shortcuts
    * SECO() is not allowed because of Clipper function Seconds()
    * however SECO32() is a valid name.
    */
   if( iLen < 4 )
      iLen = 4;
   do
   {
      uiMiddle = ( uiFirst + uiLast ) >> 1;
      iCmp = strncmp( szFuncCall, s_stdFunc[ uiMiddle ].cFuncName, iLen );
      if( iCmp <= 0 )
         uiLast = uiMiddle;
      else
         uiFirst = uiMiddle + 1;
   }
   while( uiFirst < uiLast );

   if( uiFirst != uiMiddle )
      iCmp = strncmp( szFuncCall, s_stdFunc[ uiFirst ].cFuncName, iLen );

   if( iCmp == 0 )
   {
      const HB_FUNCINFO * pFunc = &s_stdFunc[ uiFirst ];

      if( ( pFunc->iMinParam != -1 && iArgs < pFunc->iMinParam ) ||
          ( pFunc->iMaxParam != -1 && iArgs > pFunc->iMaxParam ) )
      {
         char szMsg[ 64 ];

         if( HB_COMP_ISSUPPORTED( HB_COMPFLAG_HARBOUR ) )
         {
            if( pFunc->iMinParam == pFunc->iMaxParam )
               hb_snprintf( szMsg, sizeof( szMsg ), "\nPassed: %i, expected: %i", iArgs, pFunc->iMinParam );
            else if( pFunc->iMaxParam == -1 )
               hb_snprintf( szMsg, sizeof( szMsg ), "\nPassed: %i, expected at least: %i", iArgs, pFunc->iMinParam );
            else if( pFunc->iMinParam == -1 )
               hb_snprintf( szMsg, sizeof( szMsg ), "\nPassed: %i, expected less than: %i", iArgs, pFunc->iMaxParam );
            else
               hb_snprintf( szMsg, sizeof( szMsg ), "\nPassed: %i, expected from: %i to: %i", iArgs, pFunc->iMinParam, pFunc->iMaxParam );
         }
         else
            szMsg[ 0 ] = '\0';

         hb_compGenError( HB_COMP_PARAM, hb_comp_szErrors, 'E', HB_COMP_ERR_CHECKING_ARGS, szFuncCall, szMsg );

         return HB_FALSE;
      }
   }

   return HB_TRUE;
}

/* --- run-time name resolution (ast-25; message class ast-26) -----------------------------------
 *
 * Which core functions reach a SYMBOL through a value the program computes
 * while it runs.  `__mvGet( cName )` reads a memvar nobody can name at
 * compile time; `hb_macroBlock( cExpr )` compiles whatever the string holds.
 * The compiler already knows the call is there - what it never said is that
 * this particular callee is a door out of the static graph.
 *
 * Whoever reads a dump is then able to answer a question that used to have no
 * answer: does anything here reach a name I am about to change?  The tool that
 * asks it today is a refactoring one, and before this table its only way of
 * guessing was to compare the text of string literals against the symbol -
 * which one concatenation defeats (`__mvGet( "xC" + "fg" )`).  The knowledge
 * belongs here because it is knowledge about the RTL, and the RTL is ours.
 * The same table lets the compiler warn about dynamic access under dead code
 * elimination, where it is a real trap.
 *
 * HOW THE LIST WAS DERIVED, so it can be re-derived: every HB_FUNC in
 * src/rtl and src/vm exported by include/harbour.hbx whose body turns a
 * character value taken from a PARAMETER into a dynamic symbol
 * (hb_dynsym*), a memvar (hb_memvar*), a message (hb_objGetMsgSym) or
 * compiled code (hb_macro*).  Hits whose string is a constant inside the C
 * (HBHash, HBPointer and __XHelp look up "NEW"/"HELP") are NOT resolving a
 * name for the program, and stay out.
 *
 * The workarea field family (FieldBlock, FieldWBlock) was measured with the
 * rest and is deliberately NOT here yet: nothing consumes it, and a fact with
 * no consumer is a fact nobody checks.
 *
 * Two class functions stay out for a stronger reason than that, and it is
 * worth writing down because their names would otherwise look like an
 * oversight.  __clsAddMsg, __clsModMsg and __clsDelMsg DO resolve a message
 * from a string - but hbclass.ch emits them for every member of every class,
 * with the name it stringified from the source itself.  Marking them would
 * put a run-time door in every program that declares a class, all of them
 * false.  __clsInstSuper is the same story with a compile-time symbol
 * (`@<ClassFuncName>()`) as the argument: a door that is closed.
 *
 * __objHasMsg and __objHasMsgAssigned were in this table for one measurement
 * and came straight back out, which is why they are named here: hbclass.ch
 * emits `__objHasMsg( oInstance, "InitClass" )` in the class-creation
 * sequence, so EVERY `ENDCLASS` in every program reported a run-time door at
 * the line of its own declaration.  They also only ASK whether a message
 * exists - they do not send it - so nothing is reached through them anyway.
 *
 * NOTE: THIS TABLE MUST BE SORTED ALPHABETICALLY (ASCII, so '_' comes last)
 */
typedef struct
{
   const char * cFuncName;
   const char * cKind;      /* what class of symbol the name reaches */
} HB_DYNNAMEINFO;

static const HB_DYNNAMEINFO s_dynName[] =
{
   { "DO"               , "function" },
   { "HB_EXECFROMARRAY" , "function" },
   { "HB_MACROBLOCK"    , "code"     },
   { "HB_THREADSTART"   , "function" },
   { "MEMVARBLOCK"      , "memvar"   },
   { "PROCFILE"         , "function" },
   { "TYPE"             , "code"     },
   { "__CLSMSGTYPE"     , "message"  },
   { "__DYNSN2PTR"      , "function" },
   { "__DYNSN2SYM"      , "function" },
   { "__MVEXIST"        , "memvar"   },
   { "__MVGET"          , "memvar"   },
   { "__MVGETDEF"       , "memvar"   },
   { "__MVPRIVATE"      , "memvar"   },
   { "__MVPUBLIC"       , "memvar"   },
   { "__MVPUT"          , "memvar"   },
   { "__MVRELEASE"      , "memvar"   },
   { "__MVRESTORE"      , "memvar"   },
   { "__MVSAVE"         , "memvar"   },
   { "__MVSCOPE"        , "memvar"   },
   { "__MVXRELEASE"     , "memvar"   },
   { "__OBJSENDMSG"     , "message"  }
};

/* The name class this call resolves at run time, or NULL for the calls that
 * resolve nothing - which is nearly all of them.
 *
 * The match is EXACT, unlike hb_compFunCallCheck() above: the 4-letter
 * shortcut there is Cl*pper compatibility for the argument count check, and
 * an abbreviated call does not link to the abbreviated symbol anyway.  A
 * statement about what a call does must hold for the call that is really
 * being made.
 */
const char * hb_compFunDynName( const char * szFuncCall )
{
   unsigned int uiFirst = 0, uiLast = HB_SIZEOFARRAY( s_dynName );

   while( uiFirst < uiLast )
   {
      unsigned int uiMiddle = ( uiFirst + uiLast ) >> 1;
      int iCmp = strcmp( szFuncCall, s_dynName[ uiMiddle ].cFuncName );

      if( iCmp == 0 )
         return s_dynName[ uiMiddle ].cKind;
      else if( iCmp < 0 )
         uiLast = uiMiddle;
      else
         uiFirst = uiMiddle + 1;
   }

   return NULL;
}
