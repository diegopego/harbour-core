//================
#define _FOLDER_CH
#define _ODBC_CH
#define _TREE_CH
//----------------------------------------------------------------------------//

#include "FiveWin.ch"
#include "Constant.ch"
#include "InKey.ch"
#include "FwError.ch"

#define SW_NORMAL              1
#define SW_MAXIMIZE            3
#define SW_MINIMIZE            6
#define SW_RESTORE             9

#define SC_KEYMENU      0xF100

#define WM_SYSCOMMAND        274    // 0x112
#define WM_INITMENUPOPUP     279    // 0x117
#define WM_LBUTTONDBLCLK     515    // 0x203
#define WM_DRAWITEM           43    // 0x2B
#define WM_MEASUREITEM        44    // 0x2C
#define WM_SETFONT            48    // 0x30
#define WM_QUERYDRAGICON      55    // 0x37
#define WM_ERASEBKGND         20    // 0x14
#define WM_CHILDACTIVATE      34    // 0x22
#define WM_ICONERASEBKGND     39    // 0x27
#define WM_GETFONT            49    // 0x0031
#define WM_NCPAINT           133    // 0x085
#define WM_NCMOUSEMOVE       160    // 0xA0
#define WM_MBUTTONDOWN       519
#define WM_MBUTTONUP         520
#define WM_CTLCOLOR           25    // 0x19
#define WM_CTLCOLOREDIT      307    // 0x133
#define WM_CTLCOLORLISTBOX   308    // 0x134
#define WM_CTLCOLORSTATIC    312    // 0x138
#define WM_GETMINMAXINFO      36    // 0x024
#define WM_NOTIFY             78    // 0x4E
#define WM_HELP               83    // 0x0053

#define WM_DISPLAYCHANGE    0x007E
#define WM_SETTINGCHANGE    WM_WININICHANGE

#define WS_EX_TOOLWINDOW     128
#define WS_EX_CLIENTEDGE     512
#define WS_EX_CONTEXTHELP   1024

#define CBN_SELCHANGE          1
#define CBN_DROPDOWN           7
#define CBN_CLOSEUP            8

#define IDC_ARROW          32512

#define SIZE_MINIMIZED         1

#define SC_RESTORE         61728
#define SC_CLOSE           61536   // 0xF060
#define SC_CLOSE_OPEN      61539   // 0xF063  Close and Popup already opened
#define SC_MINIMIZE        61472
#define SC_NEXT            61504
#define SC_MAXIMIZE        61488   // 0xF030

#define SW_HIDE                0
#define SW_SHOWNA              8

#define DLGC_BUTTON         8192   // 0x2000

#define CS_DBLCLKS             8
#define CW_USEDEFAULT      32768

#define HORZRES             8
#define VERTRES            10
#define VERTSIZE            6

#define GWL_STYLE         -16
#define GW_CHILD            5
#define GWL_EXSTYLE       -20

#define SM_CXSCREEN         0

#define COLOR_WINDOW        5
#define COLOR_WINDOWTEXT    8
#define WM_SETICON        128  // 0x80
#define WM_MENUCHAR       288  // 0x0120
#define WM_MENUSELECT     287  // 0x11F

#define FN_ZIP          15001

#define OBJ_PEN             1
#define OBJ_EXTPEN          11
#define OBJ_BRUSH           2
#define NULL_BRUSH          5

#define TVN_SELCHANGEDA  -402  // CommCtrl TreeView notification
#define WS_EX_LAYERED  524288

#define GWL_ID   (-12)
#define NO_ID  65535

// Tool Tip Messages

#define TTM_SETTITLEA           (WM_USER + 32)  // wParam = TTI_*, lParam = char* szTitle
#define TTM_SETTITLEW           (WM_USER + 33)  // wParam = TTI_*, lParam = wchar* szTitle
#define TTM_SETTIPBKCOLOR       (WM_USER + 19)
#define TTM_SETTIPTEXTCOLOR     (WM_USER + 20)
#define TTM_SETMAXTIPWIDTH      (WM_USER + 24)

#ifdef UNICODE
#define TTM_SETTITLE            TTM_SETTITLEW
#else
#define TTM_SETTITLE            TTM_SETTITLEA
#endif

#define VK_LMENU          0xA4

#define CS_DROPSHADOW  0x00020000
#define GCL_STYLE         (-26)

#define LWA_COLORKEY       1
#define LWA_ALPHA          2

#define TME_LEAVE          2

#define DEFAULT_GUI_FONT  17

extern set

#ifndef __XHARBOUR__
   extern notify
#endif

static aWindows    := {} // Keep this array here as first static !!!
static oWndDefault

static oWinDlgLog
static lDownLog
static lLineLog
static lDlgCouple
static lFwSysLog   := .F.
static aRectWinLog  //:= { 0, 0, 700, 400 }

// ToolTips statics support
static oToolTip, oTmr, hPrvWnd, lToolTip := .f., hWndParent := 0,;
       hToolTip := 0

static uDropInfo, oWndDragBegin

static lRButtonMenu  := .F.

static lTTBalloon := .F. // Global tooltips shape: defaults to standard
static lSkins := .F. // Global Skins Flag
static cBrwClassList := "|TXBROWSE|TWBROWSE|"         // added FWH 13.03
static hSysMenuFont
static tmp
static aInitInfo

//----------------------------------------------------------------------------//

function nWindows()  ; return Len( aWindows )
function GetAllWin( nItem ) ; return If( IsNum( nItem ), aWindows[ nItem ], aWindows )

//----------------------------------------------------------------------------//

function SetBalloon( lOnOff )

   local lPrevious := lTTBalloon

   if ValType( lOnOff ) == "L"
      lTTBalloon = lOnOff
   endif

return lPrevious

//----------------------------------------------------------------------------//

function SetSkins( lOnOff )

   local lPrevious := lSkins

   if ValType( lOnOff ) == "L"
      lSkins = lOnOff
   endif

return lPrevious

//----------------------------------------------------------------------------//

CLASS TWindow

   DATA   hWnd, nOldProc, nRefID

   DATA   bInit, bMoved, bLClicked, bLButtonUp, bKeyDown, bPainted
   DATA   bMButtonDown, bMButtonUp, bRClicked, bRButtonUp, bMouseWheel
   DATA   bResized, bLDblClick, bWhen, bValid, bKeyChar, bMLeave, bMMoved
   DATA   bGotFocus, bLostFocus, bDropFiles, bDdeInit, bDdeExecute
   DATA   bCommNotify, bMenuSelect, bZip, bUnZip, bDropOver
   DATA   bOnDisplayChange, bOnSettingChange, bHandleGesture, bOnDeviceChange

   // default editing capabilities
   DATA   bCopy, bCut, bFind, bPaste, bPrint
   DATA   bUnDo, bReDo, bDelete, bSelectAll, bFindNext, bReplace, bProperties

   DATA   cCaption, cPS, cVarName, nPaintCount, cMsg, cToolTip
   DATA   Cargo                // Ok here you have it <g>
   DATA   hDC, nId
   DATA   lActive AS LOGICAL INIT .t.
   DATA   lCancel, lFocused, lVisible, lDesign, lVbx, lValidating AS LOGICAL
   DATA   lBalloon
   DATA   nTop, nLeft, nBottom, nRight INIT 0          // AS NUMERIC
   DATA   nStyle, nExStyle, nChrHeight, nChrWidth, nLastKey
   DATA   nClrPane, nClrText, bClrGrad
   DATA   nResult, nHelpId, hCtlFocus
   DATA   aControls INIT {}
   DATA   aMinMaxInfo
   DATA   oBar, oBrush, oCursor, oFont, oIcon, oMenu
   DATA   oSysMenu, oPopup, oMsgBar, oWnd, oVScroll, oHScroll
   DATA   oDragCursor // Cursor object to use for a Drag&Drop process
   DATA   oCtlFocus   // Current control object focused
   DATA   oTop, oLeft, oBottom, oRight, oClient
   DATA   lAutoSizeCtrls AS LOGICAL INIT .F.

   DATA   bSocket, bTaskBar, bEraseBkGnd

   DATA   OnClick, OnMouseMove, OnKeyDown, OnMove, OnPaint, OnResize
   DATA   nPosition
   DATA   lFastEdit AS LOGICAL INIT .F.
   DATA   aGradColors

   DATA   oIconSm
   DATA   l2007    INIT .F.
   DATA   l2010    INIT .F.
   DATA   l2013    INIT .F.
   DATA   l2015    INIT .F.

   DATA   bRButtonMenuUp
   DATA   bNcPaint

//   DATA   lUnicode INIT .F.

   //
   DATA   lxUnicode   INIT .F. PROTECTED  // What the programmer wanted
   ASSIGN lUnicode( l ) INLINE If( Empty( ::hWnd ), ::lxUnicode := l, )
   ACCESS lUnicode INLINE If( Empty( ::hWnd ), ::lxUnicode, IsWindowUnicode( ::hWnd ) )
   ACCESS HasFocus   INLINE ( ::hWnd == GetFocus() )
   //

   CLASSDATA lRegistered AS LOGICAL

   CLASSDATA oMItemSelect            // current menu item selected
   CLASSDATA nToolTip AS NUMERIC INIT 900

   CLASSDATA aProperties INIT { "cTitle", "cVarName", "nClrText",;
                                "nClrPane", "nTop", "nLeft", "nWidth", "nHeight",;
                                "nRight", "nBottom", ;  //"nStyle", ;
                                "Cargo", "oMenu", "oFont" }

   CLASSDATA aEvents INIT { { "OnClick", "nRow", "nCol", "nKeyFlags" },;
                            { "OnMouseMove", "nRow", "nCol", "nKeyFlags" },;
                            { "OnKeyDown", "nKey", "nFlags" },;
                            { "OnMove" },;
                            { "OnPaint" }, { "OnResize" } }


   CLASSDATA bAppFocus


   DATA lWebCode         INIT .F.
   CLASSDATA cLastBtt    INIT ""
   CLASSDATA cLastItem   INIT ""

   CLASSDATA lHtml       INIT .T.
   CLASSDATA cHtml       INIT ""
   CLASSDATA cStyle      INIT ""
   CLASSDATA cJScript    INIT ""
   CLASSDATA cPathImgs   INIT "fwimages"
   CLASSDATA cUrlInit    INIT "'http://forums.fivetechsupport.com/index.php'"

   //CLASSDATA nInitID INIT 100

   DATA hAlphaColor, hAlphaLevel
   DATA oToolTip

   METHOD New( nTop, nLeft, nBottom, nRight, cTitle, nStyle, oMenu,;
               oBrush, oIcon, oWnd, lVScroll, lHScroll, nClrFore, nClrBack,;
               oCursor, cBorder, lSysMenu, lCaption, lMin, lMax, lPixel, nExStyle,;
               cVarName, nHelpId, lUnicode, nWidth, nHeight ) CONSTRUCTOR

   METHOD Activate( cShow, bLClicked, bRClicked, bMoved, bResized, bPainted,;
                    bKeyDown, bInit, bUp, bDown, bPgUp, bPgDown,;
                    bLeft, bRight, bPgLeft, bPgRight, bValid, bDropFiles,;
                    bLButtonUp, lCentered, oMonitor )

   METHOD AddControl( oControl ) INLINE ;
                        If( ::aControls == nil, ::aControls := {},),;
                        AAdd( ::aControls, oControl ), ::lValidating := .f.

   METHOD AsyncSelect( nSocket, nLParam )

   MESSAGE BeginPaint METHOD _BeginPaint()

   METHOD AngleArc( x, y, radius, angle, sweep, uPen, uBrush ) INLINE ;
         FW_AngleArc( Self, x, y, radius, angle, sweep, uPen, uBrush )

   METHOD Box( nTop, nLeft, nBottom, nRight, uBorder, uBrush, aText ) INLINE ( ;  // nClrOrPen: nil or color or hPen or oPen
      FW_Box( Self, { nTop, nLeft, nBottom, nRight }, uBorder, uBrush, aText, 0 ) )

   METHOD Capture() INLINE  SetCapture( ::hWnd )

   METHOD Center( oWnd ) INLINE ;
                 If( isZoomed( ::hWnd ), ::Restore(), ), ;
                 If( ! isIconic( ::hWnd ), ;
                     ( WndCenterEx( ::hWnd, If( oWnd != nil, oWnd:hWnd, 0 ) ), ;
                       ::CoorsUpdate() ), )

   METHOD CheckToolTip( nRow, nCol )

   METHOD ChildLevel( oClass ) INLINE ChildLevel( Self, oClass )
         //defined at db10.prg and at harbour.prg

   METHOD Close() VIRTUAL

   METHOD Command( nWParam, nLParam )

   METHOD CommNotify( nDevice, nStatus )

   METHOD Circle( nRow, nCol, nWidth )

   METHOD CoorsUpdate()

   // METHOD Copy( lAll ) INLINE  WndCopy( ::hWnd, lAll )
   METHOD Copy() INLINE If( ::bCopy != nil, Eval( ::bCopy, Self ),)

   METHOD Create( cClsName )

   METHOD CtlColor( hWndChild, hDCChild ) INLINE ;
          If( GetWindowLong( hWndChild, GWL_ID ) == NO_ID .or. ;
              GetParent( hWndChild ) != ::hWnd,, ;
              SendMessage( hWndChild, FM_COLOR, hDCChild ) )

   METHOD cTitle( cNewTitle ) SETGET

   METHOD Cut() INLINE If( ::bCut != nil, Eval( ::bCut, Self ),)

   METHOD DdeInitiate( hWndClient, nAppName, nTopicName )

   METHOD DdeAck( hWndSender, nLParam ) INLINE  DdeAck( hWndSender, nLParam )

   METHOD DdeExecute( hWndSender, nCommand ) INLINE ;
          If( ::bDdeExecute != nil,;
              Eval( ::bDdeExecute, hWndSender, DdeGetCommand( nCommand ) ),),;
        PostMessage( hWndSender, WM_DDE_ACK, ::hWnd, nMakeLong( 1, nCommand ) )

   METHOD DdeTerminate( hWndSender ) INLINE DdeTerminate( hWndSender )

   METHOD Delete() INLINE If( ::bDelete != nil, Eval( ::bDelete, Self ),)

   METHOD DestroyToolTip()

   METHOD Disable()  INLINE ::lActive := .f.,;
                            If( ::hWnd != 0, EnableWindow( ::hWnd, .f. ),)

   METHOD DispBegin()             // double buffer painting
   METHOD DispEnd( aRestore )     // double buffer painting

   METHOD DrawItem( nIdCtl, pItemStruct )

   METHOD DropFiles( hDrop )

   METHOD DropOver( nRow, nCol, nKeyFlags )

   METHOD EditTitle( cTitle )

   METHOD Ellipse( nRow, nCol, nBottom, nRight, nClrBorder, nClrFill, aText ) INLINE ( ;
         FW_Box( Self, { nRow, nCol, nBottom, nRight }, nClrBorder, nClrFill, aText, 2 ) )

   METHOD Enable()   INLINE ::lActive := .t.,;
                            If( ::hWnd != 0, EnableWindow( ::hWnd, .t. ),)

   METHOD End() BLOCK ;   // It has to be Block
          { | Self, lEnd | If( IsLogic( lEnd := ::lValid() ), ;
          If( lEnd, ::PostMsg( WM_CLOSE ),), ), lEnd }

   METHOD EndPaint() INLINE ::nPaintCount--,;
                     EndPaint( ::hWnd, ::cPS ), ::cPS := nil, ::hDC := nil, 0 // keep this zero here!

   METHOD EraseBkGnd() INLINE 1

   METHOD Event( nEvent ) INLINE ::aEvents[ nEvent ]

   METHOD EveCount() INLINE Len( ::aEvents )

   METHOD FastEdit( nRow, nCol )

   METHOD FastEditItems()

   METHOD Find() INLINE If( ::bFind != nil, Eval( ::bFind, Self ),)

   METHOD FindNext() INLINE If( ::bFindNext != nil, Eval( ::bFindNext, Self ),)

   METHOD FirstActiveCtrl()

   METHOD FloodFill( nRow, nCol, nRGBColor ) INLINE ;
          FloodFill( ::hDC, nRow, nCol, nRGBColor )

   METHOD GenDbf( cDbfName )

   METHOD GenLocals()

   METHOD cGenPrg( cFileName, lDlgUnits )

   METHOD nGetChrHeight() INLINE ;
                          ::nChrHeight := nWndChrHeight( ::hWnd,;
                          If( ::oFont != nil, ::oFont:hFont,) )

   METHOD GetCliRect()

   METHOD GetCliAreaRect()

   METHOD GetFont()

   METHOD GetRect()

   METHOD GetDC() INLINE ;
          If( ::hDC == nil, ::hDC := GetDC( ::hWnd ),),;
          If( ::nPaintCount == nil, ::nPaintCount := 1, ::nPaintCount++ ), ::hDC

   METHOD GetDlgCode( nLastKey ) VIRTUAL

   METHOD GetMinMaxInfo( pMinMaxInfo ) INLINE ;
                If( ::aMinMaxInfo != nil,;
                    SetMinMax( pMinMaxInfo, ::aMinMaxInfo ),)

   METHOD GetWidth( cText, oFont ) BLOCK { | Self, cText, oFont, nSize | ;
                    oFont := If( oFont == nil, ::oFont, oFont ),;
                    nSize := GetTextWidth( ::GetDC(), cText,;
                                               If( oFont != nil, oFont:hFont,) ),;
                    ::ReleaseDC(), nSize }

   METHOD GetText() INLINE GetWindowText( ::hWnd )

   METHOD GoNextCtrl( hCtrl )
   METHOD GoPrevCtrl( hCtrl )

   METHOD GotFocus( hFromWnd )

   METHOD GoTop() INLINE BringWindowToTop( ::hWnd )

   METHOD Gradient( aGradColors )

   METHOD HandleEvent( nMsg, nWParam, nLParam ) EXTERN ;
                       WndHandleEvent( Self, nMsg, nWParam, nLParam )

   METHOD HardCopy( nScale, lUser )

   METHOD nHeight( nNewHeight ) SETGET

   // Generated by pressing F1 on an open Menu

   METHOD Help() INLINE ::HelpTopic()

   METHOD HelpF1() INLINE If( ::oMItemSelect != nil,;
                              ::oMItemSelect:HelpTopic(), ::HelpTopic() )

   MESSAGE HelpTopic METHOD __HelpTopic()

   METHOD Hide() INLINE ( ::lVisible := .F., ShowWindow( ::hWnd, SW_HIDE ) )

   METHOD HScroll( nWParam, nLParam )

   METHOD Html()

   METHOD Iconize() INLINE CloseWindow( ::hWnd )

   METHOD IsEnabled() INLINE IsWindowEnabled( ::hWnd )

   METHOD IsIconic() INLINE IsIconic( ::hWnd )

   METHOD IsVisible() INLINE IsWindowVisible( ::hWnd )

   METHOD InitMenuPopup( hPopup, nIndex, lSystem )

   METHOD Inspect( cData ) VIRTUAL

   METHOD KeyDown( nKey, nFlags )
   METHOD KeyChar( nKey, nFlags )

   METHOD KillFocus( hWndFocus ) INLINE ::LostFocus( hWndFocus )

   METHOD LastActiveCtrl()

   METHOD LButtonDown( nRow, nCol, nKeyFlags, lTouch )
   METHOD LButtonUp( nRow, nCol, nKeyFlags )
   METHOD LDblClick( nRow, nCol, nKeyFlags )

   METHOD Line( nTop, nLeft, nBottom, nRight, nColor, nSize )
   METHOD Lines( aRowCol, anoPen )

   METHOD LockUpdate() INLINE LockWindowUpdate( ::hWnd )

   METHOD Arrow( nRow1, nCol1, nRow2, nCol2, nColor, nPenSize ) INLINE ;
          ( GDIP_DrawArrow( ::GetDC(), nRow1, nCol1, nRow2, nCol2, nColor, nPenSize ), ;
            ::ReleaseDC() )

   METHOD Link( lSubClass )

   METHOD Load( cInfo )
   METHOD LoadFile( cFileName )

   METHOD LostFocus( hWndGetFocus )

   METHOD MenuChar( nAscii, nType, nHMenu )

   METHOD MenuSelect( nIdItem, nFlags, nHMenu )

   METHOD Moved( nRow, nCol ) INLINE ;
            If( ::bMoved != nil, Eval( ::bMoved, nRow, nCol, Self ),), ;
            ::CoorsUpdate()

   METHOD NcActivate( lOnOff ) INLINE If( ! lOnOff .and. ;
                                          ::bLostFocus != nil .and. ;
                                          GetFocus() != ::hWnd,;
                                      Eval( ::bLostFocus, Self, GetFocus() ),), nil

   METHOD NcMouseMove( nHitTestCode, nRow, nCol )

   METHOD lWhen() INLINE  If( ::bWhen != nil, Eval( ::bWhen ), .t. )

   METHOD Maximize() INLINE ShowWindow( ::hWnd, SW_MAXIMIZE )

   METHOD MButtonDown( nRow, nCol, nKeyFlags )
   METHOD MButtonUp( nRow, nCol, nKeyFlags )

   METHOD MeasureItem( nIdCtl, pMitStruct )

   METHOD Minimize() INLINE  ShowWindow( ::hWnd, SW_MINIMIZE )

   METHOD MouseLeave( nRow, nCol, nKeyFlags )

   METHOD MouseMove( nRow, nCol, nKeyFlags )

   METHOD MouseWheel( nKey, nDelta, nXPos, nYPos ) INLINE ;
          If( ::bMouseWheel != nil, Eval( ::bMouseWheel, nKey, nDelta, nXPos, nYPos ), nil )

   METHOD Move( nTop, nLeft, nWidth, nHeight, lRepaint )

   METHOD Normal() INLINE ShowWindow( ::hWnd, SW_NORMAL )

   METHOD Notify( nIdCtrl, nPtrNMHDR ) // Common controls notifications

   METHOD NcPaint() VIRTUAL //

   // SETGET separately
   METHOD nWidth( nNewWidth ) SETGET

   METHOD Paint()

   METHOD PaintBack( hDC )

   METHOD PaletteChanged( hWndPalChg ) INLINE PalChgEvent( hWndPalChg )

   METHOD Paste() INLINE If( ::bPaste != nil, Eval( ::bPaste, Self ),)

   METHOD PostMsg( nMsg, nWParam, nLParam ) INLINE PostMessage( ::hWnd, nMsg, nWParam, nLParam )

   METHOD Print( oTarget, nRow, nCol, nScale )

   METHOD Property( n ) INLINE ::aProperties[ n ]

   METHOD Properties() INLINE If( ::bProperties != nil,;
                       Eval( ::bProperties, Self ),)

   METHOD PropCount() INLINE Len( ::aProperties )

   METHOD QueryDragIcon() INLINE ;
          If( ::oIcon != nil, ::oIcon:hIcon, ExtractIcon( "user.exe" ) )

   METHOD QueryEndSession() INLINE If( ::End(), 1, 0 )

   METHOD QueryNewPalette() INLINE If( IsIconic( ::hWnd ), 0, QryNewPalEvent() )

   METHOD RButtonDown( nRow, nCol, nKeyFlags )
   METHOD RButtonUp( nRow, nCol, nKeyFlags )
   METHOD RButtonMenuUp( nOrdIt, hMnu, nR, nC )

   METHOD Restore() INLINE ShowWindow( ::hWnd, SW_RESTORE )

   METHOD Destroy()     // previously called Release()

   METHOD ReDo() INLINE If( ::bReDo != nil, Eval( ::bReDo, Self ),)

   METHOD RegisterDragDrop() INLINE RegisterDragDrop( ::hWnd )

   METHOD ReleaseDC() INLINE  If( --::nPaintCount == 0,;
                              If( ReleaseDC( ::hWnd, ::hDC ), ::hDC := nil,),)


   METHOD Refresh( lErase ) INLINE InvalidateRect( ::hWnd,;
                                   If( lErase != nil, lErase, .t. ) )

   METHOD Register( nClsStyle )

   METHOD ReSize( nSizeType, nWidth, nHeight )

   METHOD RestoreState( cState )

   METHOD ReadFile( cFileName ) INLINE uLoadObject( cFileName )

   METHOD ReadImage( uSource, aSize, lGdipImage ) INLINE FW_ReadImage( Self, uSource, aSize, lGDIPImage )

   METHOD Replace() INLINE If( ::bReplace != nil, Eval( ::bReplace, Self ),)

   METHOD RevokeDragDrop() INLINE RevokeDragDrop( ::hWnd )

   METHOD RoundBox( nTop, nLeft, nBottom, nRight, nX, nY, uBorder, uFill, aText ) INLINE ( ;
         FW_Box( Self, { nTop, nLeft, nBottom, nRight }, uBorder, uFill, aText, { nX, nY } ) )

   METHOD Save()

   METHOD SaveFile( cFileName )

   METHOD SaveState() // --> cState (to save and restore later)

   METHOD SaveToBmp( cBmpFile )

   METHOD Image( nType, aCrop )  // All params Optional

   METHOD SaveAsImage( cImageFile, aCrop, nRectType, lShow )  // All params Optional

   METHOD SaveToRC()

   METHOD SaveToText( nIndent )

   METHOD Say( nRow, nCol, cText, nClrFore, nClrBack, oFont, lPixel,;
               lTransparent, nAlign )

   METHOD SayBitmap( nRow, nCol, coBitmap, nWidth, nHeight )

   METHOD SayRect( nRow, nCol, cText, nClrFore, nClrBack, nWidth )

   METHOD SayText( cText, aRect, cAlign, oFont, nClrText, nClrBack, lBorder, nAddlStyle ) ;
         INLINE FW_SayText( Self, cText, aRect, cAlign, oFont, nClrText, nClrBack, lBorder, nAddlStyle )

   METHOD SayHollow( cText, aRect, cAlign, oFont, nClrText, nClrBack, nClrPen, nPenSize ) INLINE ;
       FW_SayHollow( Self, cText, aRect, cAlign, oFont, nClrText, nClrBack, nClrPen, nPenSize )

   METHOD DrawImage( uImage, aRect, lTransp, nResizeMode, nAlphaLevel, lGray, cAlign, aColorMap ) INLINE ;
          FW_DrawImage( Self, uImage, aRect, lTransp, nResizeMode, nAlphaLevel, lGray, cAlign, aColorMap )

   METHOD SayBarCode( cText, aRect, cType, nClrText, nClrBack, lVertical, ;
                      lTransparent, nPinSize, nFlags, lPrinter ) INLINE ;
          FW_SayBarCode( Self, cText, aRect, cType, nClrText, nClrBack, lVertical, ;
                        lTransparent, nPinSize, nFlags, lPrinter )
   //
   METHOD DrawShapes( aShapes, aRect ) INLINE FW_DrawShapes( Self, aShapes, aRect )

   METHOD PieChart( aRect, aValues, aColors, aPen, nStartAngle, nInnerDia, aLabels, oFont ) INLINE ;
          FW_PieChart( Self, aRect, aValues, aColors, aPen, nStartAngle, nInnerDia, aLabels, oFont )

   METHOD SelColor( lFore )  INLINE ;
          lFore := If( lFore == nil, lFore := .f., lFore ),;
          ::SetColor( If( lFore, ChooseColor( ::nClrText ), ::nClrText ),;
                      If( lFore, ::nClrPane, ChooseColor( ::nClrPane ) ) ),;
          ::Refresh()

   METHOD SelectAll() INLINE If( ::bSelectAll != nil, Eval( ::bSelectAll, Self ),)

   METHOD SendMsg( nMsg, nWParam, nLParam ) INLINE SendMessage( ::hWnd, nMsg, nWParam, nLParam )


   METHOD SetBounds( nLeft, nTop, nRight, nBottom )

   METHOD SetBrush( oBrush ) INLINE If( ::oBrush != nil, ::oBrush:End(),),;
                                    ::oBrush := oBrush,;
                                    If( oBrush:nCount == nil, oBrush:nCount := 1, oBrush:nCount++),;
                                    ::Refresh()

   METHOD SetColor( nClrFore, nClrBack, oBrush )

   METHOD SetCoors( oRect )

   MESSAGE SetFocus METHOD __SetFocus()


   METHOD SelFont() BLOCK { | Self, nClr | nClr := ::nClrText,;
                            ::GetFont(), ::oFont := ::oFont:Choose( @nClr ), ;
                            ::SetFont( ::oFont ), ;
                            ::nClrText := nClr, ::Refresh() }


   METHOD SetFont( oFont )

   METHOD SetIcon( oIcon ) INLINE If( ::oIcon != nil, ::oIcon:End(), nil ),;
                           ::oIcon := oIcon,;
                           If( ::oIcon != nil,;
                           ::SendMsg( WM_SETICON, 0, oIcon:hIcon ), nil )

   METHOD SetPos( nRow, nCol ) INLINE ::Move( nRow, nCol )

   METHOD SetMenu( oMenu ) INLINE  ::oMenu := oMenu, oMenu:oWnd := Self,;
                                   SetMenu( ::hWnd, oMenu:hMenu ),;
                                   If( oMenu:oAccTable != nil, oMenu:oAccTable:Activate(), nil )

   METHOD SetMsg( cText, lDefault )

   MESSAGE SetWindowTheme METHOD __SetWindowTheme
   METHOD __SetWindowTheme( cSub, cIds )

/*
   METHOD SetWindowTheme( cSub, cIds ) INLINE _SetWindowTheme( ::hWnd, cSub, cIds )
*/

   METHOD GetPixel( nX, nY ) INLINE ( ;
                             tmp := GetPixel( ::GetDC(), nX, nY ),;
                             ::ReleaseDC(), tmp )

   METHOD SetPixel( nX, nY, nColor ) INLINE ;
                               SetPixel( ::GetDC(), nX, nY, nColor ),;
                               ::ReleaseDC()

   METHOD SetSize( nWidth, nHeight, lRepaint ) INLINE ;
                   WndSetSize( ::hWnd, nWidth, nHeight, lRepaint ),;
                   ::CoorsUpdate(),;
                   If( ::aGradColors != nil, ::Gradient(),)

   METHOD SetText( cText ) INLINE ::cCaption := cText, SetWindowText( ::hWnd, cText )

   METHOD Shadow()

   METHOD Show() INLINE  ( ::lVisible := .T., ShowWindow( ::hWnd, SW_SHOWNA ) )

   METHOD ShowToolTip( nRow, nCol, cToolTip )

   METHOD SysCommand( nWParam, nLParam )

   METHOD TaskBar( nWParam, nLParam ) INLINE If( ::bTaskBar != nil,;
                          Eval( ::bTaskBar, nWParam, nLParam ),)

   METHOD Timer( nTimerId ) INLINE  TimerEvent( nTimerId )

   METHOD ToolWindow()

   METHOD UnDo() INLINE If( ::bUnDo != nil, Eval( ::bUnDo, Self ),)

   METHOD UnLink()

   METHOD UnZip( nPercent ) INLINE ;
                            If( ::bUnZip != nil, Eval( ::bUnZip, nPercent ),)

   METHOD ClientEdge() INLINE SetWindowLong( ::hWnd, GWL_EXSTYLE,;
             nOr( GetWindowLong( ::hWnd, GWL_EXSTYLE ), WS_EX_CLIENTEDGE ) ),;
             ::Refresh()

   METHOD Zip( nZipInfo ) INLINE ;
                      If( ::bZip != nil, Eval( ::bZip, nZipInfo ),)

   METHOD Update() INLINE AEval( ::aControls,;
          { | o | If( ( Upper( o:ClassName() ) $ "TFOLDER;TFOLDEREX" ), o:Update(),;
          If( o:lUpdate, o:Refresh(),)) } )

   METHOD lValid() INLINE If( ::bValid != nil, Eval( ::bValid, Self ), .t. )

   METHOD VScroll( nWParam, nLParam )

   METHOD nVertRes() BLOCK ;
          { | Self, nRes | nRes := GetDeviceCaps( ::GetDC(), VERTRES  ),;
                           ::ReleaseDC(), nRes }


   METHOD nHorzRes() BLOCK ;
          { | Self, nRes | nRes := GetDeviceCaps( ::GetDC(), HORZRES  ),;
                           ::ReleaseDC(), nRes }

   METHOD AEvalWhen()

   METHOD VbxFireEvent( pEventInfo ) INLINE VBXEvent( pEventInfo )

   METHOD GetSetGWLStyle( lExtended, nStyle, lOnOff ) PROTECTED
   METHOD WinStyle(   nStyle, lOnOff ) INLINE ::GetSetGWLStyle( .f., nStyle, lOnOff )
   METHOD WinExStyle( nStyle, lOnOff ) INLINE ::GetSetGWLStyle( .t., nStyle, lOnOff )

   METHOD SetAlphaLevel() PROTECTED

   ACCESS nSeeThroClr         INLINE ::hAlphaColor
   ACCESS nOpacity            INLINE ::hAlphaLevel
   ASSIGN nSeeThroClr( nNew ) INLINE ( ::hAlphaColor := nNew, ::SetAlphaLevel(), ::hAlphaColor )
   ASSIGN nOpacity( nNew )    INLINE ( ::hAlphaLevel := nNew, ::SetAlphaLevel(), ::hAlphaLevel )

   METHOD OnDisplayChange( nBitsPerPixel, nWidth, nHeight )
   METHOD OnSettingChange( nWParam, nLParam )
   METHOD OnDeviceChange( nWParam, nLParam )
   METHOD HandleGesture(   nGesture, nLParam )

   METHOD AppFocus( lGotFocus )

ENDCLASS

//----------------------------------------------------------------------------//

METHOD Register( nClsStyle )  CLASS TWindow

   local hUser

   DEFAULT ::lRegistered := .f. // XBPP workaround

   if ::lRegistered
      return nil
   endif

   hUser = GetInstance()

   DEFAULT nClsStyle  := nOR( CS_VREDRAW, CS_HREDRAW ),;
           ::nClrPane := GetSysColor( COLOR_WINDOW ),;
           ::oBrush   := TBrush():New( ,::nClrPane )

   nClsStyle = nOr( nClsStyle, CS_GLOBALCLASS, CS_DBLCLKS )

   if ::lUnicode
      if GetClassInfoW( hUser, ::ClassName() ) == nil
        ::lRegistered = RegisterClassW( ::ClassName(), nClsStyle,,, hUser, 0,;
                                            ::oBrush:hBrush,,,;
                                            If( ::oIcon != nil, ::oIcon:hIcon, 0 ) )
      else
         ::lRegistered = .t.
      endif
   else
      if GetClassInfo( hUser, ::ClassName() ) == nil
         ::lRegistered = RegisterClass( ::ClassName(), nClsStyle,,, hUser, 0,;
                                           ::oBrush:hBrush,,,;
                                           If( ::oIcon != nil, ::oIcon:hIcon, 0 ) )
      else
         ::lRegistered = .t.
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD Create( cClsName )  CLASS TWindow

   DEFAULT cClsName := ::ClassName(), ::cCaption := "",;
           ::nStyle := WS_OVERLAPPEDWINDOW,;
           ::nTop   := 0, ::nLeft := 0, ::nBottom := 10, ::nRight := 10,;
           ::nId    := 0

   if ::oWnd != nil
      ::nStyle = nOR( ::nStyle, WS_CHILD )
   endif
/*
   // ::lUnicode or not is decided while defining window

   if FW_SetUnicode()
      ::lUnicode = FW_SetUnicode()
   endif
*/

   if ::lUnicode
      if ::nBottom != CW_USEDEFAULT
         ::hWnd = CreateWindowExW( cClsName, ::cCaption, ::nStyle, ;
                                ::nLeft, ::nTop, ::nRight - ::nLeft + 1, ;
                                ::nBottom - ::nTop + 1, ;
                                If( ::oWnd != nil, ::oWnd:hWnd, 0 ), ;
                                ::nId,, ::nExStyle )
      else
         ::hWnd = CreateWindowExW( cClsName, ::cCaption, ::nStyle, ;
                                ::nLeft, ::nTop, ::nRight, ::nBottom, ;
                                If( ::oWnd != nil, ::oWnd:hWnd, 0 ), ;
                                ::nId,, ::nExStyle )
      endif
   else
      if ::nBottom != CW_USEDEFAULT
         ::hWnd = CreateWindow( cClsName, ::cCaption, ::nStyle, ;
                                ::nLeft, ::nTop, ::nRight - ::nLeft + 1, ;
                                ::nBottom - ::nTop + 1, ;
                                If( ::oWnd != nil, ::oWnd:hWnd, 0 ), ;
                                ::nId,, ::nExStyle )
      else
         ::hWnd = CreateWindow( cClsName, ::cCaption, ::nStyle, ;
                                ::nLeft, ::nTop, ::nRight, ::nBottom, ;
                                If( ::oWnd != nil, ::oWnd:hWnd, 0 ), ;
                                ::nId,, ::nExStyle )
      endif
   endif

   if ::hWnd == 0
      WndCreateError( Self )
   else
      ::nRefID := ::Link()
   endif

return nil

//----------------------------------------------------------------------------//

function WndCreateError( Self )

   local cInfo := Chr( 13 ) + Chr( 10 ) + ;
                  "Class: " + ::ClassName() + CRLF + ;
                  "Caption: " + cValToChar( ::cCaption )

   cInfo += CRLF + "System Error: " + GetErrMsg() // Win32 System Error

   Eval( ErrorBlock(), _FwGenError( CANNOTCREATE_WINDOW_OR_CONTROL, cInfo ) )

return nil

//----------------------------------------------------------------------------//

METHOD New( nTop, nLeft, nBottom, nRight, cTitle, nStyle, oMenu,;
            oBrush, oIcon, oWnd, lVScroll, lHScroll, nClrFore, nClrBack,;
            oCursor, cBorder, lSysMenu, lCaption, lMin, lMax, lPixel, nExStyle,;
            cVarName, nHelpId, lUnicode, nWidth, nHeight ) CLASS TWindow

   DEFAULT nTop     := 2, nLeft := 2, nBottom := 20, nRight := 70,;
           lVScroll := .f., lHScroll := .f.,;
           nClrFore := GetSysColor( COLOR_WINDOWTEXT ),;
           nClrBack := GetSysColor( COLOR_WINDOW ),;
           nStyle   := 0,;
           cBorder  := "SINGLE", lSysMenu := .t., lCaption := .t.,;
           lMin     := .t., lMax := .t., lPixel := .f., nExStyle := 0,;
           lUnicode := FW_SetUnicode()

   if nStyle == 0
      nStyle = nOr( WS_CLIPCHILDREN,;
                    If( cBorder == "NONE",   WS_POPUP, 0 ),;
                    If( cBorder == "SINGLE", WS_THICKFRAME, 0 ),;
                    If( lCaption, WS_CAPTION, 0 ),;
                    If( lSysMenu .and. lCaption, WS_SYSMENU, 0 ),;
                    If( lMin .and. lCaption, WS_MINIMIZEBOX, 0 ),;
                    If( lMax .and. lCaption, WS_MAXIMIZEBOX, 0 ),;
                    If( lVScroll, WS_VSCROLL, 0 ),;
                    If( lHScroll, WS_HSCROLL, 0 ) )
   endif

   ::nTop      = nTop * If( lPixel, 1, WIN_CHARPIX_H )
   ::nLeft     = nLeft * If( lPixel, 1, WIN_CHARPIX_W )
   ::nBottom   = nBottom * If( lPixel, 1, WIN_CHARPIX_H )
   ::nRight    = nRight * If( lPixel, 1, WIN_CHARPIX_W )
   ::nStyle    = nStyle
   ::cCaption  = cTitle
   ::oCursor   = oCursor
   ::oMenu     = oMenu
   ::oWnd      = oWnd
   ::oIcon     = oIcon
   ::lVisible  = .t.               // As soon as we create it, it exists!
   ::aControls = {}
   ::nLastKey  = 0
   ::lValidating = .f.
   ::nExStyle  = nExStyle
   ::nHelpId   = nHelpId
   ::lUnicode  = lUnicode

   if ValType( oIcon ) == "C"
      if File( oIcon )
         DEFINE ICON oIcon FILENAME oIcon
      else
         DEFINE ICON oIcon RESOURCE oIcon
      endif
      ::oIcon = oIcon
   endif

   ::Register()
   ::Create()

   if ::cCaption != nil          // auto hide window. Headers of oBrow:oCols is use twindow on mouse
      ::Hide()                   // move event. but not title
   endif

   DEFAULT cVarName := "oWnd" + AllTrim( Str( Len( aWindows ) ) )

   ::cVarName = cVarName

   ::SetColor( nClrFore, nClrBack, oBrush )

   if oMenu != nil
      ::SetMenu( oMenu )
/*
      SetMenu( ::hWnd, oMenu:hMenu )
      oMenu:oWnd = Self
      if oMenu:oAccTable != nil
         oMenu:oAccTable:Activate()
      endif
*/
   endif

   if lVScroll
      DEFINE SCROLLBAR ::oVScroll VERTICAL OF Self
   endif
   if lHScroll
      DEFINE SCROLLBAR ::oHScroll HORIZONTAL OF Self
   endif

   ::GetFont()

   if ! Empty( nWidth )
      ::SetSize( nWidth, nHeight )
   endif

   SetWndDefault( Self )                   // Set Default DEFINEd Window

return Self

//----------------------------------------------------------------------------//

METHOD Activate( cShow, bLClicked, bRClicked, bMoved, bResized, bPainted,;
                 bKeyDown, bInit, bUp, bDown, bPgUp, bPgDown,;
                 bLeft, bRight, bPgLeft, bPgRight, bValid, bDropFiles,;
                 bLButtonUp, lCentered, oMonitor ) CLASS TWindow

   local oVScroll, oHScroll

   DEFAULT cShow := "NORMAL", lCentered := .F.

   ( bMoved, bResized, bKeyDown, bInit, bLButtonUp )

   ::nResult     = nil
   ::lValidating = .f.

   DEFAULT oMonitor := FW_SetMonitor()
   if oMonitor != nil
      if HB_ISNUMERIC( oMonitor )
         oMonitor := FW_GetMonitor( oMonitor )
      elseif !( HB_ISOBJECT( oMonitor ) .and. ( oMonitor:IsKindOf( "TMONITOR" ) .or. oMonitor:IsKindOf( "TWINDOW" ) ) )
         oMonitor := nil
      endif
   endif

   if bValid != nil
      ::bValid = bValid
   endif

   if bDropFiles != nil
      ::bDropFiles = bDropFiles
      DragAcceptFiles( ::hWnd, .t. )
   endif

   if bPainted != nil
      if ::IsKindof( "TMDIFRAME" )            // for "CLASS MyApp FROM TMdiFrame"
         ::oWndClient:bPainted = bPainted
      else
         ::bPainted  = bPainted
      endif
   endif

   if bLClicked != nil
      if ::IsKindof( "TMDIFRAME" )
         ::oWndClient:bLClicked = bLClicked
      else
         ::bLClicked = bLClicked
      endif
   endif

   if bRClicked != nil
      if ::IsKindof( "TMDIFRAME" )
         ::oWndClient:bRClicked = bRClicked
      else
         ::bRClicked = bRClicked
      endif
   endif

   // For WS_VSCROLL and WS_HSCROLL styles

   if ::IsKindof( "TMDIFRAME" )
      oVScroll = ::oWndClient:oVScroll
      oHScroll = ::oWndClient:oHScroll
   else
      oVScroll = ::oVScroll
      oHScroll = ::oHScroll
   endif

   if oVScroll != nil       // When using VSCROLL clause
      if bUp != nil
         oVScroll:bGoUp = bUp
      else
         if ::IsKindof( "TMDIFRAME" )
            oVScroll:bGoUp = { || ScrollWindow( ::oWndClient:hWnd, 0, 20, 0, GetClientRect( ::oWndClient:hWnd ) ) }
         endif
      endif
      if bDown != nil
         oVScroll:bGoDown = bDown
      else
         if ::IsKindof( "TMDIFRAME" )
            oVScroll:bGoDown = { || ScrollWindow( ::oWndClient:hWnd, 0, -20, 0, GetClientRect( ::oWndClient:hWnd ) ) }
         endif
      endif
      if bPgUp != nil
         oVScroll:bPageUp = bPgUp
      endif
      if bPgDown != nil
         oVScroll:bPageDown = bPgDown
      endif
   endif

   if oHScroll != nil       // When using HSCROLL clause
      if bLeft != nil
         oHScroll:bGoUp = bLeft
      else
         if ::IsKindof( "TMDIFRAME" )
            oHScroll:bGoUp = { || ScrollWindow( ::oWndClient:hWnd, 20, 0, 0, GetClientRect( ::oWndClient:hWnd ) ), ReleaseCapture() }
         endif
      endif
      if bRight != nil
         oHScroll:bGoDown = bRight
      else
         if ::IsKindof( "TMDIFRAME" )
            oHScroll:bGoDown = { || ScrollWindow( ::oWndClient:hWnd, -20, 0, 0, GetClientRect( ::oWndClient:hWnd ) ), ReleaseCapture() }
         endif
      endif
      if bPgLeft != nil
         oHScroll:bPageUp = bPgLeft
      endif
      if bPgRight != nil
         oHScroll:bPageDown = bPgRight
      endif
   endif

   ::AEvalWhen()

   if ! lWRunning()
      SetWndApp( ::hWnd )
   endif

   if ::bInit != nil
      Eval( ::bInit, Self )
   endif

   ::Resize( 0, ::nWidth, ::nHeight )

   if lCentered
      if oMonitor == nil
         ::Center()
      else
         WndCenterEx( ::hWnd, oMonitor )
      endif
   elseif oMonitor != nil .and. oMonitor:IsKindOf( "TMONITOR" )
      oMonitor:Move( Self )
   endif

   ShowWindow( ::hWnd, AScan( { "HIDDEN", "NORMAL", "ICONIZED", "MAXIMIZED" }, cShow ) - 1 )
   UpdateWindow( ::hWnd )

   ::lVisible = .t.

   if ::oWnd == nil
      if ! lWRunning()
         WinRun( Self:hWnd )
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD AsyncSelect( nSocket, nLParam ) CLASS TWindow

   if ::bSocket != nil
      Eval( ::bSocket, nSocket, nLParam )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD Circle( nRow, nCol, nWidth ) CLASS TWindow

   ::GetDC()
   Ellipse( ::hDC, nCol, nRow, nCol + nWidth - 1, nRow + nWidth - 1 )
   ::ReleaseDC()

return nil

//----------------------------------------------------------------------------//

METHOD Command( nWParam, nLParam ) CLASS TWindow

   local nNotifyCode, nID, hWndCtl, oCtrl

   nNotifyCode = nHiWord( nWParam )
   nID         = nLoWord( nWParam )
   hWndCtl     = nLParam

   do case
      case ::oPopup != nil
           ::oPopup:Command( nID )

      case hWndCtl == 0 .and. ::oMenu != nil
           ::oMenu:Command( nID )

      case GetClassName( hWndCtl ) == "ToolbarWindow32"
           oWndFromHwnd( hWndCtl ):Command( nWParam, nLParam )

      case hWndCtl != 0
           do case
              case nNotifyCode == BN_CLICKED
                   SendMessage( hWndCtl, FM_CLICK, 0, 0 )

              case nNotifyCode == CBN_CLOSEUP
                   SendMessage( hWndCtl, FM_CLOSEUP, 0, 0 )

              case nNotifyCode == CBN_SELCHANGE
                   SendMessage( hWndCtl, FM_CHANGE, 0, 0 )

              case nNotifyCode == CBN_DROPDOWN
                   SendMessage( hWndCtl, FM_DROPDOWN, 0, 0 )

           endcase

      case GetClassName( hWndCtl ) == "Edit"
           oCtrl := oWndFromHwnd( hWndCtl )
           if oCtrl != nil .and. oCtrl:ClassName() == "TEDIT"
              oCtrl:Command( nWParam, nLParam )
              return nil
           endif
   endcase

return nil

//----------------------------------------------------------------------------//

METHOD CoorsUpdate() CLASS TWindow

   local aRect := GetCoors( ::hWnd )

   ::nTop    = aRect[ 1 ]
   ::nLeft   = aRect[ 2 ]
   ::nBottom = aRect[ 3 ]
   ::nRight  = aRect[ 4 ]

return nil

//----------------------------------------------------------------------------//

METHOD cTitle( cNewTitle ) CLASS TWindow

   if cNewTitle != nil
      ::SetText( cNewTitle )
      ::cCaption = cNewTitle
   endif

return ::GetText()   //GetWindowText( ::hWnd )

//----------------------------------------------------------------------------//

METHOD DdeInitiate( hWndClient, nAppName, nTopicName )  CLASS TWindow

   local xRet

   If ::bDdeInit == nil
      return nil
   endif

   xRet := Eval( ::bDdeInit, hWndClient,;
                 GlobalGetAtomName( nAppName ),;
                 GlobalGetAtomName( nTopicName ) )

   if xRet == Nil .or. Valtype(xRet) != "L" .or. xRet
      SendMessage( hWndClient, WM_DDE_ACK, ::hWnd,;
                   nMakeLong( nAppName, nTopicName ) )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD DispBegin() CLASS TWindow

   local aInfo

   aInfo = FWDispBegin( ::hWnd, ::hDC )
   ::hDC = aInfo[ 3 ]  // Memory hDC

return aInfo

//----------------------------------------------------------------------------//

METHOD DispEnd( aRestore ) CLASS TWindow

   FWDispEnd( aRestore )
   ::hDC = aRestore[ 2 ]

return nil

//----------------------------------------------------------------------------//

METHOD DrawItem( nIdCtl, pItemStruct ) CLASS TWindow

   local oPopup  := If( ::oPopup != nil, ::oPopup, ;
                       If( ::oSysMenu != nil, ::oSysMenu, nil ) )
   if nIdCtl == 0
      Fw_MenuDraw( pItemStruct, oPopup, hSysMenuFont, Self )
   else
      return SendMessage( GetDlgItem( ::hWnd, nIdCtl ), FM_DRAW, nIdCtl, pItemStruct )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD DropFiles( hDrop ) CLASS TWindow

   local aCoors := { 0, 0 }

   if ! Empty( ::bDropFiles )
      DragQueryPoint( hDrop, aCoors )
      Eval( ::bDropFiles, aCoors[ 2 ], aCoors[ 1 ], DragQueryFiles( hDrop ) )
   endif

   DragFinish( hDrop )

return nil

//----------------------------------------------------------------------------//

METHOD GenDbf( cDbfName ) CLASS TWindow

   DEFAULT cDbfName := RTrim( SubStr( ::GetText(), 1, 8 ) ) + ".dbf"

   ::CoorsUpdate()

   if File( cDbfName )
      if ! MsgYesNo( cDbfName + CRLF + ;
                     "This file already exists" + CRLF + ;
                     "Do you want to overwrite it ?" )
         return nil
      endif
   else
      DbCreate( cDbfName,;
                { { "CONTROL", "C", 15, 0},;
                  { "CAPTION", "C", 40, 0},;
                  { "ID", "N", 4, 0},;
                  { "TOP", "N", 4, 0},;
                  { "LEFT", "N", 4, 0},;
                  { "WIDTH", "N", 4, 0},;
                  { "HEIGHT", "N", 4, 0},;
                  { "STYLE", "N", 15, 0},;
                  { "CLRFORE", "N", 8, 0},;
                  { "CLRBACK", "N", 8, 0},;
                  { "FNTNAME", "C", 15, 0},;
                  { "FNTWIDTH", "N", 4, 0},;
                  { "FNTHEIGHT", "N", 4, 0},;
                  { "FNTBOLD", "L", 1, 0},;
                  { "FNTITALIC", "L", 1, 0} } )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD GenLocals() CLASS TWindow

   local cLocals := "   local " + ::cVarName, n

   for n = 1 to Len( ::aControls )
      cLocals += ::aControls[ n ]:GenLocals()
   next

   if ::oMsgBar != nil
      cLocals += ::oMsgBar:GenLocals()
   endif

return cLocals

//----------------------------------------------------------------------------//

METHOD cGenPRG( cFileName, lDlgUnits ) CLASS TWindow

   local cPrg := ""

   DEFAULT lDlgUnits := .F.

   ::CoorsUpdate()

   cPrg += '#include "FiveWin.ch"' + CRLF + CRLF
   cPrg += "//" + Replicate( "-", 76 ) + "//" + CRLF + CRLF
   cPrg += "function BuildWindow()" + CRLF + CRLF

   cPrg += ::GenLocals() + CRLF

   if ::oBar != nil
      cPrg += "   local oBar" + CRLF
   endif

   if ::oBrush != nil
      cPrg += "   local oBrush" + CRLF
   endif

   if ::oBrush != nil
      cPrg += CRLF + ::oBrush:cGenPRG() + CRLF
   endif

   cPrg += CRLF + ;
           '   DEFINE WINDOW ' + ::cVarName + ' TITLE "' + ::cTitle + '" ;' + CRLF + ;
           "      FROM " + ;
           If( IsZoomed( ::hWnd ), "0", Str( ::nTop / WIN_CHARPIX_H, 3 ) ) + ", " + ;
           If( IsZoomed( ::hWnd ), "0", Str( ::nLeft / WIN_CHARPIX_W, 3 ) ) + " TO " + ;
           Str( ::nBottom / WIN_CHARPIX_H, 3 ) + ", " + Str( ::nRight / WIN_CHARPIX_W, 3 )                       // 16,8

   if ::oMenu != nil
      cPrg += " ;" + CRLF + "      MENU BuildMenu()"
   endif

   if ::oBrush != nil
      cPrg += " ;" + CRLF + "      BRUSH oBrush"
   endif

   if ! Empty( ::aControls )
      cPrg += CRLF
      AEval( ::aControls, { | oCtrl | cPrg += oCtrl:cGenPRG( lDlgUnits ) } )
   endif

   if ::oMsgBar != nil
      cPrg += ::oMsgBar:cGenPrg() + CRLF
   endif

   cPrg += "   ACTIVATE WINDOW " + ::cVarName + If( IsZoomed( ::hWnd ), " MAXIMIZED", "" ) + CRLF + CRLF
   cPrg += "return " + ::cVarName + CRLF + CRLF
   cPrg += "//" + Replicate( "-", 76 ) + "//" + CRLF

   if ::oMenu != nil
      cPrg += CRLF + "static function BuildMenu()" + CRLF + CRLF
      cPrg += "   local oMenu" + CRLF + CRLF
      cPrg += ::oMenu:cGenPrg() + CRLF
      cPrg += "return oMenu" + CRLF + CRLF
      cPrg += "//" + Replicate( "-", 76 ) + "//" + CRLF
   endif

   if ! Empty( cFileName )
      MemoWrit( cFileName, cPrg )
   endif

return cPrg

//----------------------------------------------------------------------------//

/*
METHOD EraseBkGnd( hDC ) CLASS TWindow

   if ! Empty( ::bEraseBkGnd )
      return Eval( ::bEraseBkGnd, hDC )
   endif

   if IsIconic( ::hWnd )
      if ::oWnd != nil
         FillRect( hDC, GetClientRect( ::hWnd ), ::oWnd:oBrush:hBrush )
         return 1
      endif
      return 0
   endif

   if ::oBrush != nil .and. ! Empty( ::oBrush:hBrush )
      FillRect( hDC, GetClientRect( ::hWnd ), ::oBrush:hBrush )
      return 1
   endif

return nil
*/

//----------------------------------------------------------------------------//

METHOD GetCliRect() CLASS TWindow

   local aRect := GetClientRect( ::hWnd )

return TRect():New( aRect[ 1 ], aRect[ 2 ], aRect[ 3 ], aRect[ 4 ] )

//----------------------------------------------------------------------------//

METHOD GetCliAreaRect() CLASS TWindow

   local oRect := ::GetCliRect()

   if ::oTop != nil
      oRect:nTop     := ::oTop:nTop + ::oTop:nHeight
   endif
   if ::oLeft != nil
      oRect:nLeft    := ::oLeft:nLeft + ::oLeft:nWidth
   endif
   if ::oRight != nil
      oRect:nRight   := ::oRight:nLeft
   endif
   if ::oBottom != nil
      oRect:nBottom  := ::oBottom:nTop
   elseif ::oMsgBar != nil
      oRect:nBottom  := ::oMsgBar:nTop
   endif

return oRect

//----------------------------------------------------------------------------//

METHOD GetRect() CLASS TWindow

   local aRect := GetWndRect( ::hWnd )

return TRect():New( aRect[ 1 ], aRect[ 2 ], aRect[ 3 ], aRect[ 4 ] )

//----------------------------------------------------------------------------//

METHOD MeasureItem( nIdCtl, pMitStruct ) CLASS TWindow

   local nAt
   local hMFont     := Nil

   // Warning: On this message the Controls still are not initialized
   // because WM_MEASUREITEM is sent before of WM_INITDIALOG
   if nIdCtl == 0 // Menu
      Fw_MeasureItem( pMitStruct, hSysMenuFont, Self )
      return .F.  // default behavior

   endif
   if ( nAt := AScan( ::aControls, { | oCtrl | oCtrl:nId == nIdCtl } ) ) != 0
      return ::aControls[ nAt ]:FillMeasure( pMitStruct )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD MenuChar( nAscii, nType, nHMenu ) CLASS TWindow

   local oMenu, n, nAt
   local nPos

   ( nType )

   if ::oPopup != nil
      oMenu = ::oPopup
   else
      if ::oMenu != nil
         oMenu = ::oMenu
      endif
   endif
   if oMenu != nil
      if oMenu:hMenu == nHMenu .or. ( oMenu := oMenu:GetSubMenu( nHMenu ) ) != nil
         for n = 1 to Len( oMenu:aItems )
            if ! Empty( oMenu:aItems[ n ]:cPrompt )
               if ( nAt := At( "&", oMenu:aItems[ n ]:cPrompt ) ) != 0 .and. ;
                  Upper( SubStr( oMenu:aItems[ n ]:cPrompt, nAt + 1, 1 ) ) == ;
                  Upper( Chr( nAscii ) )
                  if oMenu:oWnd != nil .and. oMenu:oWnd:IsKindof( "TMDIFRAME" )
                     if Upper( oMenu:oWnd:oWndActive:ClassName() ) == "TMDICHILD" .and. ;
                        lAnd( oMenu:oWnd:oWndActive:nStyle, WS_SYSMENU )
                        if IsZoomed( oMenu:oWnd:oWndActive:hWnd )
                           nPos := n
                        else
                           nPos := n - 1
                        endif
                     else
                        nPos := n - 1
                     endif
                  else
                     nPos := n - 1
                  endif
                  return nMakeLong( nPos, 2 )
               endif
            endif
         next
         return 0
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD MenuSelect( nIdItem, nFlags, nHMenu ) CLASS TWindow

   if ::oToolTip != nil
      ::oToolTip:Hide()
      SysRefresh()
   endif

   if oToolTip != nil
      oToolTip:End()
      SysRefresh()
      oToolTip = nil
   endif

   if nFlags == -1 .and. nHMenu == 0   // The pulldown is beeing closed !
      ::oMItemSelect = nil
   else
      if ::oMenu != nil .or. ::oPopup != nil
         if ::oPopup != nil
            ::oMItemSelect = ::oPopup:GetMenuItem( nIdItem )
         else
            ::oMItemSelect = ::oMenu:GetMenuItem( nIdItem )
         endif
         ::SetMsg( If( ::oMItemSelect != nil, ::oMItemSelect:cMsg,) )
      endif
   endif

   if ::bMenuSelect != nil
      Eval( ::bMenuSelect, ::oMItemSelect, nFlags, nHMenu )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD nHeight( nNewHeight ) CLASS TWindow

   if PCount() > 0
      return WndHeight( ::hWnd, XEVal( nNewHeight, Self ) )
   else
      if ! Empty( ::hWnd )
         return WndHeight( ::hWnd )
      else
         return XEVal( ::nBottom, Self ) - XEVal( ::nTop, Self ) + 1
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD Notify( nIdCtrl, nPtrNMHDR ) CLASS TWindow

   local nCode     := GetNMHDRCode( nPtrNMHDR )
   local nHWndFrom := GetNMHDRHWndFrom( nPtrNMHDR )
   local oCtrl     := oWndFromHwnd( nHWndFrom )

   if oCtrl != nil
      do case
         case nCode == 103 .or. nCode == TVN_SELCHANGEDA  // TreeView item selected
              oCtrl:SelChanged()

         otherwise
              if oCtrl:hWnd != ::hWnd
                 return oCtrl:Notify( nIdCtrl, nPtrNMHDR )
              endif
      endcase
   endif

return nil

//----------------------------------------------------------------------------//

METHOD nWidth( nNewWidth ) CLASS TWindow

   if PCount() > 0
      return WndWidth( ::hWnd, XEVal( nNewWidth, Self ) )
   else
      if ! Empty( ::hWnd )
         return WndWidth( ::hWnd )
      else
         return XEVal( ::nRight, Self ) - XEVal( ::nLeft, Self ) + 1
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD Print( oTarget, nRow, nCol, nScale ) CLASS TWindow

   local lNew := .f.

   DEFAULT nRow := 0, nCol := 0, nScale := 4

   if ::bPrint != nil
      Eval( ::bPrint, Self )
      return nil
   endif

   if oTarget == nil
      lNew = .t.
      PRINTER oTarget NAME ::GetText()
         PAGE
         SysRefresh()
   endif

   WndPrint( ::hWnd, oTarget:hDC, nRow, nCol, nScale )

   if lNew
         ENDPAGE
      ENDPRINT
   endif

return nil

//----------------------------------------------------------------------------//

METHOD Save() CLASS TWindow

   local n
   local cType, cInfo := "", cMethod
   local oWnd  := &( ::ClassName() + "()" )
   local uData, nProps := 0

   oWnd = oWnd:New()

   for n = 1 to Len( ::aProperties )
       if ! ( uData := OSend( Self, ::aProperties[ n ] ) ) == ;
          OSend( oWnd, ::aProperties[ n ] )
          cInfo += ( I2Bin( Len( ::aProperties[ n ] ) ) + ;
                     ::aProperties[ n ] )
          nProps++
          cType = ValType( uData )
          do case
             case cType == "A"
                  cInfo += ASave( uData )

             case cType == "O"
                  cInfo += uData:Save()

             otherwise
                  cInfo += ( cType + I2Bin( Len( uData := cValToChar( uData ) ) ) + ;
                             uData )
          endcase
       endif
   next

   if ::aEvents != nil
      for n = 1 to Len( ::aEvents )
         if ( cMethod := OSend( Self, ::aEvents[ n ][ 1 ] ) ) != nil
            cInfo += ( I2Bin( Len( ::aEvents[ n ][ 1 ] ) ) + ;
                       ::aEvents[ n ][ 1 ] )
            nProps++
            cInfo += ( "C" + I2Bin( Len( cMethod ) ) + cMethod )
         endif
      next
   endif

   oWnd:End()

return "O" + I2Bin( 2 + Len( ::ClassName() ) + 2 + Len( cInfo ) ) + ;
       I2Bin( Len( ::ClassName() ) ) + ;
       ::ClassName() + I2Bin( nProps ) + cInfo

//----------------------------------------------------------------------------//

function AToText( aArray, cArrayName, nIndent )

   local cText := "", cType
   local n

   for n = 1 to Len( aArray )
      cType = ValType( aArray[ n ] )
      do case
         case cType == "C"
              cText += Space( nIndent ) + "::" + cArrayName + "[ " + LTrim( Str( n ) ) + " ] = "
              cText += '"' + aArray[ n ] + '"' + CRLF

         case cType == "L"
              cText += Space( nIndent ) + "::" + cArrayName + "[ " + LTrim( Str( n ) ) + " ] = "
              cText += If( aArray[ n ], ".T.", ".F." ) + CRLF

         case cType == "N"
              cText += Space( nIndent ) + "::" + cArrayName + "[ " + LTrim( Str( n ) ) + " ] = "
              cText += LTrim( Str( aArray[ n ] ) ) + CRLF

         case cType == "A"
              cText += AToText( aArray[ n ], "::" + cArrayName + "[ " + LTrim( Str( n ) ) + " ]", nIndent + 3 ) + CRLF

         case cType == "O"
              cText += aArray[ n ]:SaveToText( nIndent, cArrayName + "[ " + Alltrim( Str( n ) ) + " ]" ) + CRLF + CRLF

         case cType == "U"
              cText += Space( nIndent ) + "::" + cArrayName + "[ " + LTrim( Str( n ) ) + " ] = "
              cText += "nil" + CRLF
      endcase
   next

return cText

//----------------------------------------------------------------------------//

METHOD SaveState() CLASS TWindow

   if !Empty( ::hWnd ) .and. !::IsKindOf( "TCONTROL" ) //.and. !::WinStyle( WS_CHILD )
      return STRTOHEX( GetWindowPlacement( ::hWnd ) )
   endif

return ""

//----------------------------------------------------------------------------//

METHOD SaveToBmp( cBmpFile ) CLASS TWindow

   local hBmp := WndBitmap( ::hWnd )
   local hDib := DibFromBitmap( hBmp )

   DibWrite( cBmpFile, hDib )
   GloBalFree( hDib )
   DeleteObject( hBmp )

return ( File( cBmpFile ) )

//----------------------------------------------------------------------------//

METHOD Image( nType, aCrop ) CLASS TWindow // all params optional

   local pImage, oRect

   DEFAULT nType  := 1

   if nType == 0
      pImage   := WndBmp( Self, 0, .t. )
   else

      oRect    := If( nType == 1, ::GetCliRect(), ::GetCliAreaRect() )

      if ValType( aCrop ) == "A"
         oRect := oRect:Modify( aCrop, .t. )
      endif

      pImage   := GDIPLUSCaptureRectWnd( ::hWnd, oRect:nLeft, oRect:nTop, oRect:nWidth, oRect:nHeight )

   endif

return pImage

//----------------------------------------------------------------------------//

METHOD SaveAsImage( cImageFile, aCrop, nRectType, lShow ) CLASS TWindow // @cImageFile. All Params optional

   local pImage, lRet

   if ValType( nRectType ) == "L"; lShow := nRectType; nRectType := nil; endif
   DEFAULT nRectType := 1

   pImage   := ::Image( nRectType, aCrop )
   lRet     := FW_SaveImage( pImage, @cImageFile, 100, nil, .f. ) // treat as non-alpha bmp
   GDIP_DeleteImage( pImage )

   if lRet .and. lShow == .t.
      ximage( cImageFile )
   endif

return lRet

//----------------------------------------------------------------------------//
/*
METHOD SaveAsImage( cImageFile, aCrop, nRectType, lShow ) CLASS TWindow // @cImageFile. All Params optional

   local hWnd, hBmp, oRect
   local lRet

   if ValType( nRectType ) == "L"; lShow := nRectType; nRectType := nil; endif
   DEFAULT nRectType := 1

   oRect    := If( nRectType == 0, ::GetRect(), If( nRectType == 1, ::GetCliRect(), ::GetCliAreaRect() ) )
   hWnd     := If( nRectType == 0, GetDesktopWindow(), ::hWnd )

   if ValType( aCrop ) == "A"
      oRect := oRect:Modify( aCrop )
   endif

   hBmp     := MakeBkBmpEx( hWnd, oRect:nTop, oRect:nLeft, oRect:nWidth, oRect:nHeight )
   lRet     := FW_SaveImage( hBmp, @cImageFile, 100, nil, .f. ) // treat as non-alpha bmp
   DeleteObject( hBmp )

   if lRet .and. lShow == .t.
      ximage( cImageFile )
   endif

return lRet

//----------------------------------------------------------------------------//
*/

/*
METHOD SaveToPng( cBmpFile ) CLASS TWindow

   local hBmp := WndBitmap( ::hWnd )

   Save2PngFile( hBmp, cBmpFile )

return ( File( cBmpFile ) )
*/
//----------------------------------------------------------------------------//

METHOD SaveToRC() CLASS TWindow

   local cRC, n

   DEFAULT ::cCaption := ::GetText()

   cRC = Upper( StrToken( ::cCaption, 1 ) ) + " DIALOG "

   cRC += AllTrim( Str( ::nTop ) ) + ", "
   cRC += AllTrim( Str( ::nLeft ) ) + ", "
   cRC += AllTrim( Str( ::nWidth ) ) + ", "
   cRC += AllTrim( Str( ::nHeight ) ) + CRLF

   DEFAULT ::oFont := TFont():New()

   cRC += "STYLE DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU" + CRLF
   cRC += 'CAPTION "' + ::cCaption + '"' + CRLF
   cRC += 'FONT 8, "' + ::oFont:cFaceName + '"' + CRLF
   cRC += "{" + CRLF

   if ::aControls != nil
      for n = 1 to Len( ::aControls )
         cRC += ::aControls[ n ]:SaveToRC( 3 ) + CRLF
      next
   endif

   cRC += "}" + CRLF

return cRC

//----------------------------------------------------------------------------//

METHOD SaveToText( nIndent ) CLASS TWindow

   local n, cType, cInfo
   local cMethod, uData
   local oWnd  := &( ::ClassName() + "()" )

   DEFAULT nIndent := 0

   DEFAULT ::cVarName := "noname"

   cInfo := Space( nIndent ) + "OBJECT " + If( nIndent > 0, "::", "" ) + ;
            ::cVarName + " AS " + ;
            If( nIndent > 0, Upper( Left( ::ClassName(), 2 ) ) + ;
            Lower( SubStr( ::ClassName(), 3 ) ), If( ::IsDerivedFrom( "TFORM" ), ::cClassName, ::ClassName() ) ) + ;
            CRLF + CRLF

   oWnd = oWnd:New()

   for n = 1 to Len( ::aProperties )
       // if ::aProperties[ n ] == "cVarName"

       // else
          if ! ( uData := OSend( Self, ::aProperties[ n ] ) ) == ;
             OSend( oWnd, ::aProperties[ n ] )
             cType = ValType( uData )
             do case
                case cType == "C"
                     cInfo += Space( nIndent ) + "   ::" + ::aProperties[ n ] + " = "
                     cInfo += '"' + uData + '"' + CRLF

                case cType == "A"
                     if ::aProperties[ n ] != "aControls"
                        cInfo += Space( nIndent + 3 ) + "::" + ::aProperties[ n ] + ;
                                 " = Array( " + AllTrim( Str( Len( uData ) ) ) + " )" + CRLF + CRLF
                     endif
                     cInfo += AToText( uData, ::aProperties[ n ], nIndent + 3 )
                     if Right( cInfo, 13 ) != "ENDOBJECT" + CRLF + CRLF
                        cInfo += CRLF
                     endif

                case cType == "O"
                     cInfo += CRLF + uData:SaveToText( nIndent + 3 )

                otherwise
                     cInfo += Space( nIndent ) + "   ::" + ::aProperties[ n ] + " = "
                     cInfo += cValToChar( uData ) + CRLF
             endcase
          endif
       // endif
   next

   if ::aEvents != nil
      for n = 1 to Len( ::aEvents )
         if ( cMethod := OSend( Self, ::aEvents[ n ][ 1 ] ) ) != nil
            cInfo += Space( nIndent ) + "   ::" + ::aEvents[ n ][ 1 ] + " = '"
            cInfo += cMethod + "'" + CRLF
            /*
            cParams1 = "{ |"
            cParams2 = "("
            for m = 2 to Len( ::aEvents[ n ] )
               cParams1 += If( m > 2, ", ", " " ) + ::aEvents[ n ][ m ]
               cParams2 += If( m > 2, ", ", " " ) + ::aEvents[ n ][ m ]
            next
            cParams1 += " | "
            cParams2 += " )"
            if ::oWnd != nil
               cInfo += cParams1 + "::oWnd:" + cMethod + cParams2 + " }" + CRLF
            else
               cInfo += cParams1 + " ::" + cMethod + cParams2 + " }" + CRLF
            endif
            */
         endif
      next
   endif

   cInfo += CRLF + Space( nIndent ) + "ENDOBJECT"

   if oWnd:IsKindOf( "TDIALOG" )
      DEFAULT oWnd:lModal := .F.
   endif

   oWnd:End()

return cInfo

//----------------------------------------------------------------------------//

function ASave( aArray )

   local n, cType, uData
   local cInfo := ""

   for n = 1 to Len( aArray )
      cType = ValType( aArray[ n ] )
      do case
         case cType == "A"
              cInfo += ASave( aArray[ n ] )

         case cType == "O"
              cInfo += aArray[ n ]:Save()

         otherwise
              cInfo += ( cType + I2Bin( Len( uData := cValToChar( aArray[ n ] ) ) ) + ;
                         uData )
      endcase
   next

return "A" + I2Bin( 2 + Len( cInfo ) ) + I2Bin( Len( aArray ) ) + cInfo

//----------------------------------------------------------------------------//

function ARead( cInfo )

   local nPos := 4, nLen, n
   local aArray, cType, cBuffer

   nLen   = Bin2I( SubStr( cInfo, nPos, 2 ) )
   nPos  += 2
   aArray = Array( nLen )

   // LogFile( "c:\aread.txt", { "ARead", nLen } )

   for n = 1 to Len( aArray )
      cType = SubStr( cInfo, nPos++, 1 )
      nLen  = Bin2I( SubStr( cInfo, nPos, 2 ) )
      nPos += 2
      cBuffer = SubStr( cInfo, nPos, nLen )
      nPos += nLen

      // LogFile( "c:\aread.txt", { cType, nLen, cBuffer } )

      do case
         case cType == "A"
              aArray[ n ] = ARead( "A" + I2Bin( nLen ) + cBuffer )

         case cType == "O"
              aArray[ n ] = ORead( cBuffer )

         case cType == "C"
              aArray[ n ] = cBuffer

         case cType == "D"
              aArray[ n ] = CToD( cBuffer )

         case cType == "L"
              aArray[ n ] = ( cBuffer == ".T." )

         case cType == "N"
              aArray[ n ] = Val( cBuffer )
      endcase
   next

return aArray

//----------------------------------------------------------------------------//

function ORead( cInfo )

   local nLen, cClassName, oObj
   local nPos := 1

   nLen       = Bin2I( SubStr( cInfo, nPos, 2 ) )
   nPos      += 2
   cClassName = SubStr( cInfo, nPos, nLen )
   nPos      += nLen
   oObj       = &( cClassName + "()" )

   oObj:New()
   oObj:Load( SubStr( cInfo, nPos ) )

return oObj

//----------------------------------------------------------------------------//

METHOD SaveFile( cFileName ) CLASS TWindow

   DEFAULT cFileName := SubStr( ::cVarName, 2 ) + ".ffm"

return MemoWrit( cFileName, ::Save() )

//----------------------------------------------------------------------------//

METHOD Say( nRow, nCol, cText, nClrFore, nClrBack, oFont, lPixel,;
            lTransparent, nAlign ) CLASS TWindow

   DEFAULT nClrFore := ::nClrText,;
           nClrBack := ::nClrPane,;
           oFont    := ::oFont,;
           lPixel   := .f.,;
           lTransparent := .f.

   if ValType( nClrFore ) == "C"      //  xBase Color string
      nClrBack = nClrFore
      nClrFore = nGetForeRGB( nClrFore )
      nClrBack = nGetBackRGB( nClrBack )
   endif

   ::GetDC()

   DEFAULT nAlign := GetTextAlign( ::hDC )

   WSay( ::hWnd, ::hDC, nRow, nCol, cValToChar( cText ), nClrFore, nClrBack,;
            If( oFont != nil, oFont:hFont, 0 ), lPixel, lTransparent, nAlign )

   ::ReleaseDC()

return nil

//----------------------------------------------------------------------------//

METHOD SayBitmap( nRow, nCol, coBitmap, nWidth, nHeight ) CLASS TWindow

   local cType := ValType( coBitmap )

   if cType == "C"
      DEFINE BITMAP coBitmap FILENAME coBitmap
   endif

   PalBmpDraw( ::GetDC(), nRow, nCol, coBitmap:hBitmap, coBitmap:hPalette,;
               nWidth, nHeight,,, ::nClrPane )
   ::ReleaseDC()

   if cType == "C"
      coBitmap:End()
   endif

return nil

//----------------------------------------------------------------------------//

METHOD SayRect( nRow, nCol, cText, nClrFore, nClrBack, nWidth ) CLASS TWindow

   DEFAULT nClrFore := CLR_BLACK, nClrBack := CLR_WHITE

   ::GetDC()
   WSayRect( ::hWnd, ::hDC, nRow, nCol, cText, nClrFore, nClrBack, nWidth )
   ::ReleaseDC()

return nil

//----------------------------------------------------------------------------//

METHOD LButtonDown( nRow, nCol, nKeyFlags, lTouch ) CLASS TWindow

   if hb_IsBlock( ::bLClicked )
      return Eval( ::bLClicked, nRow, nCol, nKeyFlags, Self, lTouch )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD LButtonUp( nRow, nCol, nKeyFlags ) CLASS TWindow

   if ::lHtml
   if "BUTTON" $ ::ClassName() .or. "BTN" $ ::ClassName()
      ::cLastBtt  := CharRem( " .,;&%$#!ï¿½?=)([]{}:", ::cCaption )
         FWLOG ::cLastBtt
      endif
   endif

   // Visual FiveWin
   if ::OnClick != nil
      return Eval( ::OnClick, nRow, nCol, nKeyFlags )
   endif

   if hb_IsBlock( ::bLButtonUp )
      return Eval( ::bLButtonUp, nRow, nCol, nKeyFlags, Self )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD LDblClick( nRow, nCol, nKeyFlags ) CLASS TWindow

   if ::bLDblClick != nil
      return Eval( ::bLDblClick, nRow, nCol, nKeyFlags, Self )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD Line( nTop, nLeft, nBottom, nRight, nColor, nSize ) CLASS TWindow
   // nColor can be oPen/hPen also. Then nSize should be nil

   local hPen, lNewPen, hOldPen

   ::GetDC()
   hPen     := hValToPen( If( nSize == nil, nColor, { nColor, nSize } ), @lNewPen )
   if hPen != nil
      hOldPen  := SelectObject( ::hDC, hPen )
   endif
   MoveTo( ::hDC, nLeft, nTop )
   LineTo( ::hDC, nRight, nBottom )
   if hOldPen != nil
      SelectObject( ::hDC, hOldPen )
      if lNewPen
         DeleteObject( hPen )
      endif
   endif
   ::ReleaseDC()

return nil

//----------------------------------------------------------------------------//

METHOD Lines( aRowCol, anoPen ) CLASS TWindow

   local hPen, lNewPen, hOldPen

   ::GetDC()
   hPen     := hValToPen( anoPen, @lNewPen )

   if hPen != nil
      hOldPen  := SelectObject( ::hDC, hPen )
   endif
   MoveTo( ::hDC, aRowCol[ 1, 2 ], aRowCol[ 1, 1 ] )
   AEval( aRowCol, { |a| LineTo( ::hDC, a[ 2 ], a[ 1 ] ) }, 2 )
   if hOldPen != nil
      SelectObject( ::hDC, hOldPen )
      if lNewPen
         DeleteObject( hPen )
      endif
   endif

   ::ReleaseDC()

return nil

//----------------------------------------------------------------------------//

METHOD RButtonDown( nRow, nCol, nKeyFlags ) CLASS TWindow

   if ::bRClicked != nil
      Eval( ::bRClicked, nRow, nCol, nKeyFlags, Self )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD RButtonUp( nRow, nCol, nKeyFlags ) CLASS TWindow

   if ::bRButtonUp != nil
      Eval( ::bRButtonUp, nRow, nCol, nKeyFlags, Self )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD MButtonDown( nRow, nCol, nKeyFlags ) CLASS TWindow

   if ::bMButtonDown != nil
      Eval( ::bMButtonDown, nRow, nCol, nKeyFlags, Self )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD MButtonUp( nRow, nCol, nKeyFlags ) CLASS TWindow

   if ::bMButtonUp != nil
      Eval( ::bMButtonUp, nRow, nCol, nKeyFlags, Self )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD RButtonMenuUp( nOrdIt, hMnu, nR, nC ) CLASS TWindow

   local aPoint
   DEFAULT nR    := 40
   DEFAULT nC    := 100
   aPoint        := { nR, nC }
   aPoint = ScreenToClient( ::hWnd, aPoint )
   if ::bRButtonMenuUp != nil .and. !lRButtonMenu
      lRButtonMenu  := .T.    // not allowed recursive calls
      Eval( ::bRButtonMenuUp, nOrdIt, hMnu, aPoint[ 1 ], aPoint[ 2 ], Self )
      lRButtonMenu  := .F.
   endif

return nil

//----------------------------------------------------------------------------//

METHOD SetMsg( cText, lDefault ) CLASS TWindow

   local oMsgBar := ::oMsgBar

   DEFAULT lDefault := .f.

   if ::oBottom != nil .and. Upper( ::oBottom:ClassName() ) $ "TMSGBAR,TSTATUSBAR"
      oMsgBar = ::oBottom
   endif

   if oMsgBar != nil
      if lDefault
         oMsgBar:cMsgDef := cText
      else
         oMsgBar:SetMsg( cText )
      endif
   else
      if ::oWnd != nil
         ::oWnd:SetMsg( cText )
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD Link( lSubClass ) CLASS TWindow

   local nAt := AScan( aWindows, 0 )

   DEFAULT lSubClass := .T.

   if ::hWnd != 0
      if nAt != 0
         aWindows[ nAt ] = Self
      else
         AAdd( aWindows, Self )
         nAt = Len( aWindows )
      endif

      if lSubClass
         ::nOldProc = XXChangeProc( ::hWnd, nAt, ::lxUnicode )
      endif
   endif

return nAt

//----------------------------------------------------------------------------//

METHOD Destroy() CLASS TWindow


   if ( ::hWnd == nil .or. ::hWnd == 0 ) .and. ;
      ( ::nResult == nil .or. ( ValType( ::nResult ) == "N" .and. ::nResult == 0 ) )

      if ::oBrush != nil  // GET created from a combobox/dbcombo has no ::hWnd  -Jun 2014
         ::oBrush:End()
      endif

      if ::oFont != nil
         ::oFont:End()
         ::oFont = nil
      endif

      return nil
   endif

   if ::oBrush != nil
      ::oBrush:End()
   endif
   if ::oCursor != nil
      ::oCursor:End()
   endif
   if ::oDragCursor != nil
      ::oDragCursor:End()
   endif
   if ::oIcon != nil
      ::oIcon:End()
   endif

   if ::oFont != nil
      ::oFont:End()
   endif

   if ::oMenu != nil
      ::oMenu:Destroy()  // Don't use End() here because it sends a Msg !!!
   endif
   if ::oSysMenu != nil
      ::oSysMenu:End()
   endif
   if ::oVScroll != nil
      ::oVScroll:Destroy()
   endif
   if ::oHScroll != nil
      ::oHScroll:Destroy()
   endif

   if ! Empty( ::hWnd )
      ::UnLink()
   endif

   if ::hWnd != 0 .and. GetWndApp() == ::hWnd
      PostQuitMessage( 0 )
   endif

   if ::IsKindOf( "TDIALOG" ) .and. ! ::lModal // TDialog Class or inherited Class
      DeRegDialog( ::hWnd )
   endif

   if oWndDefault == Self
      oWndDefault = nil
   endif

   ::hWnd = 0

return nil

//----------------------------------------------------------------------------//

METHOD ReSize( nSizeType, nWidth, nHeight ) CLASS TWindow

   ::CoorsUpdate()

   // New Alignment techniques
   if ::oTop != nil
      ::oTop:AdjTop()
   endif
   if ::oBottom != nil
      ::oBottom:AdjBottom()
   endif

   if ::oMsgBar != nil .and. ! ::oMsgBar == ::oBottom
      ::oMsgBar:Adjust()
   endif
   if ::oBar != nil .and. ! ::oBar == ::oTop
      ::oBar:Adjust()
   endif

   if ::oLeft != nil
      ::oLeft:AdjLeft()
   endif
   if ::oRight != nil
      ::oRight:AdjRight()
   endif

   if ::oClient != nil
      ::oClient:AdjClient()
   endif

   if ::OnResize != nil
      Eval( ::OnResize, Self )
   endif

   if ::bResized != nil
      Eval( ::bResized, nSizeType, nWidth, nHeight )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD RestoreState( cState ) CLASS TWindow

   if !Empty( ::hWnd ) .and. !Empty( cState ) .and. !::IsKindOf( "TCONTROL" ) //.and. !::WinStyle( WS_CHILD )
      SetWindowPlacement( ::hWnd, HEXTOSTR( cState ) )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD SetBounds( nLeft, nTop, nRight, nBottom ) CLASS TWindow

   SetWindowPos( ::hWnd, 0, nTop, nLeft, nRight - nLeft + 1,;
                 nBottom - nTop + 1, 4 )

return nil

//----------------------------------------------------------------------------//

METHOD SetCoors( oRect ) CLASS TWindow

   SetWindowPos( ::hWnd, 0, oRect:nTop, oRect:nLeft,;
                 oRect:nRight - oRect:nLeft + 1,;
                 oRect:nBottom - oRect:nTop + 1, 4 )    // Important:
                                                        // Use 4 for
   ::nTop    = oRect:nTop                               // for keeping
   ::nLeft   = oRect:nLeft                              // ZOrder !!!!
   ::nBottom = oRect:nBottom
   ::nRight  = oRect:nRight

return nil

//----------------------------------------------------------------------------//

METHOD UnLink() CLASS TWindow

   local nAt := AScan( aWindows,;
                       { | o | ValType( o ) == "O" .and. o:hWnd == ::hWnd } )

   if ::nOldProc != nil .and. ::hWnd != 0
      RestProc( ::hWnd, ::nOldProc )
      ::nOldProc = nil
   endif

   if nAt > 0 .and. nAt <= Len( aWindows )
      aWindows[ nAt ] = 0
   endif

return nil

//----------------------------------------------------------------------------//

METHOD VScroll( nWParam, nLParam ) CLASS TWindow

   local nScrHandle  := nLParam
   local nScrollCode := nLoWord( nWParam )
   local nPos        := nHiWord( nWParam )

   if nScrHandle == 0                   // Window ScrollBar
      if ::oVScroll != nil
         do case
            case nScrollCode == SB_LINEUP
                 ::oVScroll:GoUp()

            case nScrollCode == SB_LINEDOWN
                 ::oVScroll:GoDown()

            case nScrollCode == SB_PAGEUP
                 ::oVScroll:PageUp()

            case nScrollCode == SB_PAGEDOWN
                 ::oVScroll:PageDown()

            case nScrollCode == SB_THUMBPOSITION
                 ::oVScroll:ThumbPos( nPos )

            case nScrollCode == SB_THUMBTRACK
                 ::oVScroll:ThumbTrack( nPos )

            case nScrollCode == SB_ENDSCROLL
                 return 0
         endcase
      endif
   else                                 // Control ScrollBar
      do case
         case nScrollCode == SB_LINEUP
              SendMessage( nScrHandle, FM_SCROLLUP )

         case nScrollCode == SB_LINEDOWN
              SendMessage( nScrHandle, FM_SCROLLDOWN )

         case nScrollCode == SB_PAGEUP
              SendMessage( nScrHandle, FM_SCROLLPGUP )

         case nScrollCode == SB_PAGEDOWN
              SendMessage( nScrHandle, FM_SCROLLPGDN )

         case nScrollCode == SB_THUMBPOSITION
              SendMessage( nScrHandle, FM_THUMBPOS, nPos )

         case nScrollCode == SB_THUMBTRACK
              SendMessage( nScrHandle, FM_THUMBTRACK, nPos )
      endcase
   endif

return 0

//----------------------------------------------------------------------------//

METHOD ToolWindow() CLASS TWindow

   SetWindowLong( ::hWnd, GWL_EXSTYLE,;
                  nOr( GetWindowLong( ::hWnd, GWL_EXSTYLE ), WS_EX_TOOLWINDOW ) )
   SetWindowPos( ::hWnd, 0, ::nTop, ::nLeft, ::nRight - ::nLeft + 1,;
                 ::nBottom - ::nTop + 1, 55 )
   // 55 is nOr( SWP_NOSIZE, SWP_NOMOVE, SWP_NOZORDER,;
   // SWP_NOACTIVATE, SWP_FRAMECHANGED )

return nil

//----------------------------------------------------------------------------//

METHOD HScroll( nWParam, nLParam ) CLASS TWindow

   local nScrHandle  := nLParam
   local nScrollCode := nLoWord( nWParam )
   local nPos        := nHiWord( nWParam )

   if nScrHandle == 0 .and. ::oHScroll != nil    // Window ScrollBar
      do case
         case nScrollCode == SB_LINEUP
              ::oHScroll:GoUp()

         case nScrollCode == SB_LINEDOWN
              ::oHScroll:GoDown()

         case nScrollCode == SB_PAGEUP
              ::oHScroll:PageUp()

         case nScrollCode == SB_PAGEDOWN
              ::oHScroll:PageDown()

         case nScrollCode == SB_THUMBPOSITION
              ::oHScroll:ThumbPos( nPos )

         case nScrollCode == SB_THUMBTRACK
              ::oHScroll:ThumbTrack( nPos )

         case nScrollCode == SB_ENDSCROLL
              return 0
      endcase
   else                                 // Control ScrollBar
      do case
         case nScrollCode == SB_LINEUP
              SendMessage( nScrHandle, FM_SCROLLUP )

         case nScrollCode == SB_LINEDOWN
              SendMessage( nScrHandle, FM_SCROLLDOWN )

         case nScrollCode == SB_PAGEUP
              SendMessage( nScrHandle, FM_SCROLLPGUP )

         case nScrollCode == SB_PAGEDOWN
              SendMessage( nScrHandle, FM_SCROLLPGDN )

         case nScrollCode == SB_THUMBPOSITION
              SendMessage( nScrHandle, FM_THUMBPOS, nPos )

         case nScrollCode == SB_THUMBTRACK
              SendMessage( nScrHandle, FM_THUMBTRACK, nPos )
      endcase
   endif

return 0

//----------------------------------------------------------------------------//

METHOD Html() CLASS TWindow

   local cHtml
   local oCtrl
   local nW
   local nH

   nH := ( ( ::nHeight - 16 ) / GetSysMetrics( 1 ) ) * 100
   nW := ( ( ::nWidth - 16 ) / GetSysMetrics( 0 ) )  * 100

//   <meta http-equiv="Access-Control-Allow-Origin" content="*">

//   <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.0/css/all.css'>
//   <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>
//   <link href="https://maxcdn.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css" rel="stylesheet" id="bootstrap-css">
//   <script src="https://maxcdn.bootstrapcdn.com/bootstrap/4.3.1/js/bootstrap.min.js"></script>
/*
   <script src="https://code.jquery.com/jquery-3.6.0.min.js" integrity="sha256-/xUj+3OJU5yExlq6GSYGSHk7tPXikynS7ogEvDej/m4=" crossorigin="anonymous"></script>
   <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3" crossorigin="anonymous">
   <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p" crossorigin="anonymous"></script>

<script src="https://cdn.jsdelivr.net/npm/@popperjs/core@2.11.5/dist/umd/popper.min.js" ></script>
https://unpkg.com/@popperjs/core@2

   <link rel="stylesheet" href="https://ajax.googleapis.com/ajax/libs/jqueryui/1.13.1/themes/smoothness/jquery-ui.css">
   <script src="https://ajax.googleapis.com/ajax/libs/jqueryui/1.13.1/jquery-ui.min.js"></script>
   <link rel="stylesheet" type="text/css" href="https://ajax.googleapis.com/ajax/libs/jqueryui/1.8.1/themes/base/jquery-ui.css"/>
*/
/*
   max-width: 500px;
   margin: auto;
*/
/*
    display: flex;
    justify-content: center;
    align-items: center;
*/
// <meta charset="utf-8">
   TEXT INTO cHtml
<!DOCTYPE html>
<html lang="en">

<head>
   <meta http-equiv="content-type" content="text/html; utf-8">
   <meta http-equiv="X-UA-Compatible" content="IE=edge">
   <meta name="viewport" content="width=device-width, initial-scale=1">
   <title>FWH WebApp</title>
   <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.0/css/all.css'>
   <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3" crossorigin="anonymous">
   <script src="https://code.jquery.com/jquery-3.6.0.min.js" integrity="sha256-/xUj+3OJU5yExlq6GSYGSHk7tPXikynS7ogEvDej/m4=" crossorigin="anonymous"></script>
   <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p" crossorigin="anonymous"></script>
   <style>
   html, body {
       margin: 0;
       padding: 0;
       height: #nH#%;
       width: #nW#%;
   }
   #cStyle#

   </style>

</head>

<body>
   <div id="main" class="container-fluid" style="height: 100%;padding:0px;overflow:hidden">
   ENDTEXT
   cHtml := StrTran( cHtml, "FWH WebApp", if( !Empty( WndMain() ), WndMain():cTitle, ::cCaption ) )
   cHtml := StrTran( cHtml, "#nW#", AllTrim( Str( nW ) ) )
   cHtml := StrTran( cHtml, "#nH#", AllTrim( Str( nH ) ) )

   if ! Empty( ::oMenu )
      cHtml += ::oMenu:Html()
   endif

   for each oCtrl in ::aControls
      cHtml += oCtrl:Html()
   next

   cHtml  += '   <div id="client_main" class="container-fluid" style="padding:0px;overflow:hidden;"></div>' + CRLF

   if ! Empty( ::oMsgBar )
      cHtml += CRLF + ::oMsgBar:Html()
   endif

   cHtml += CRLF + "   </div>"
   cHtml    += CRLF + "   </div>"
   cHtml += CRLF + "</body>" + CRLF
   cHtml += "</html>"
   cHtml := StrTran( cHtml, "#cStyle#", ::cStyle )
   ::cHtml  := cHtml

return cHtml

//----------------------------------------------------------------------------//

METHOD SysCommand( nWParam, nLParam ) CLASS TWindow

   local bKeyAction, i
   local lRibbon := .F.
   local o, oRibbon

   ( nLParam )

   if ::oBar != nil
      AEval( ::oBar:aControls,;
             { | oCtl | If( oCtl:IsKindOf( "TBTNBMP" ),;
                            oCtl:LostFocus(), ) } )
   endif

   if ::IsKindOf( "TMDIFRAME" )
      for i = 1 to Len( ::oWndClient:aWnd )
         if ::oWndClient:aWnd[ i ]:oBar != nil
            AEval( ::oWndClient:aWnd[ i ]:oBar:aControls,;
                   { | oCtl | If( oCtl:IsKindOf( "TBTNBMP" ),;
                                  oCtl:LostFocus(), ) } )
         endif
      next
   endif

   if ::IsKindOf( "TDIALOG" )
      AEval( ::aControls,;
             { | oCtl | If( oCtl:IsKindOf( "TBTNBMP" ),;
                            oCtl:LostFocus(), ) } )
   endif

   if nWParam == SC_KEYMENU // VK_F10
      if ::aControls != nil
         for each o in ::aControls
            if o:IsKindOf( "TRIBBONBAR" )
               if o:oPanelAcc == NIL
                  if o:lUseAcc
                     o:KeybMode()
                     lRibbon := .T.
                  endif
               else
                  o:RunAction( 0 )
               endif
               oRibbon := o
               exit
            endif
         next
         if oRibbon != nil .and. oRibbon:lUseAcc
            return 0
         endif
      endif

      if ( bKeyAction := SetKey( VK_F10 ) ) != nil .and. ! lRibbon
         Eval( bKeyAction, ProcName( 4 ), ProcLine( 4 ), Self )
         return 0
      endif
   endif

   if ::oSysMenu != nil .and. nWParam < 61440           // 0xF000
      ::oSysMenu:Command( nWParam )
   else
      do case
         case nWParam == SC_CLOSE .or. nWParam == SC_CLOSE_OPEN
              return ::End()
      endcase
   endif

return nil

//----------------------------------------------------------------------------//

METHOD Paint() CLASS TWindow

   local aInfo, uVal := 0

   if IsIconic( ::hWnd )
      if ::IsKindOf( "TMDICHILD" )
         ::SendMsg( WM_ERASEBKGND, ::hDC, 0 )
      else
         ::SendMsg( WM_ICONERASEBKGND, ::hDC, 0 )
      endif
      DrawIcon( ::hDC, 0, 0,;
         If( ::oIcon != nil, ::oIcon:hIcon, ExtractIcon( "user.exe" ) ) )
   else
      aInfo = ::DispBegin()
      if ValType( ::bEraseBkGnd ) == "B"
         Eval( ::bEraseBkGnd, ::hDC )
      else
         ::PaintBack( ::hDC )
      endif
      if ValType( ::bPainted ) == "B"
         uVal = Eval( ::bPainted, ::hDC, ::cPS, Self )
      endif
      ::DispEnd( aInfo )
  endif

return uVal

//----------------------------------------------------------------------------//

METHOD PaintBack( hDC ) CLASS TWindow

   local uVal := 0
   local nOrgX := 0, nOrgY := 0

   ::oBrush:Resize( Self, @nOrgX, @nOrgY )
   SetBrushOrgEx( hDC, nOrgX, nOrgY )

   FillRect( hDC, GetClientRect( ::hWnd ), ::oBrush:hBrush )

return uVal

//----------------------------------------------------------------------------//

METHOD Gradient( aGradColors ) CLASS TWindow

   local hDC, hBmp, hBmpOld
   local oRect    := ::GetCliRect()

   DEFAULT aGradColors := ::aGradColors

   if aGradColors == nil
      return nil
   endif

   hDC = CreateCompatibleDC( ::GetDC() )
   hBmp = CreateCompatibleBitMap( ::hDC, ::nWidth, ::nHeight )
   hBmpOld = SelectObject( hDC, hBmp )

//   GradientFill( hDC, 0, 0, ::nHeight, ::nWidth, aGradColors )  // FWH 17.07
// Gradient size should match ClientRect
   GradientFill( hDC, oRect:nTop, oRect:nLeft, oRect:nHeight, oRect:nWidth, aGradColors ) // FWH 17.07

   if ::oBrush != nil
      ::oBrush:End()
   endif

   ::oBrush = TBrush():New()
   WITH OBJECT ::oBrush
      :hBitmap       = hBmp
      :hBrush        = CreatePatternBrush( hBmp )
      // Next 2 lines help resizing of brush
      :aGrad         = aGradColors  // FWH17.07
      :nResizeMode   = 1            // FWH17.07
   END

   SelectObject( hDC, hBmpOld )

   ::ReleaseDC()

return nil

//----------------------------------------------------------------------------//

METHOD HardCopy( nScale, lUser ) CLASS TWindow

   local oPrn

   DEFAULT lUser := .t.

   if lUser
      PRINT oPrn NAME ::cTitle FROM USER
   else
      PRINT oPrn NAME ::cTitle
   endif

      PAGE
         ::Refresh()
         SysRefresh()     // Let Windows process
         ::Print( oPrn, 0, 0, nScale )
      ENDPAGE
   ENDPRINT

return nil

//----------------------------------------------------------------------------//

METHOD Move( nTop, nLeft, nWidth, nHeight, lRepaint ) CLASS TWindow

   DEFAULT nTop     := ::nTop,;
           nLeft    := ::nLeft,;
           nWidth   := ::nWidth,;
           nHeight  := ::nHeight,;
           lRepaint := .T.

   MoveWindow( ::hWnd, nTop, nLeft, nWidth, nHeight, lRepaint )

   ::CoorsUpdate()

return nil

//----------------------------------------------------------------------------//

METHOD SetColor( nClrFore, nClrBack, oBrush ) CLASS TWindow

   // DEFAULT colors get assigned at :Colors() method
   // because there we _do_ have a hDC already created

   if ValType( nClrFore ) == "C" .and. Left( nClrFore, 1 ) == "#" .and. Len( nClrFore ) == 7
      nClrFore = nRGB( nClrFore )
   endif

   if ValType( nClrBack ) == "C" .and. Left( nClrBack, 1 ) == "#" .and. Len( nClrBack ) == 7
      nClrBack = nRGB( nClrBack )
   endif

   if ValType( nClrFore ) == "C"
      nClrBack = nClrFore                   // It is a dBase Color string
      nClrFore = nGetForeRGB( nClrFore )
      nClrBack = nGetBackRGB( nClrBack )
   endif

   ::nClrText = nClrFore
   ::nClrPane = nClrBack

   if ::oBrush != nil
      ::oBrush:End()
      ::oBrush = NIL
   endif
   if oBrush != nil
      ::oBrush = oBrush
      oBrush:nCount++
   else
      ::oBrush = TBrush():New( , nClrBack )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD KeyChar( nKey, nFlags ) CLASS TWindow

   local bKeyAction := SetKey( nKey )

   if bKeyAction != nil .and. lAnd( nFlags, 16777216 ) // function Key
      return Eval( bKeyAction, ProcName( 4 ), ProcLine( 4 ), Self )
   endif

   if ::bKeyChar != nil
      return Eval( ::bKeyChar, nKey, nFlags )
   endif

   if nKey == VK_ESCAPE .and. ::oWnd != nil
      ::oWnd:KeyChar( nKey, nFlags )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD InitMenuPopup( hPopup, nIndex, lSystem ) CLASS TWindow

   local oPopup

   ( nIndex )

   if ! lSystem
      if ! Empty( ::oPopup ) // .and. ::oPopup:hMenu == hPopup A.L. 20/01/06
         ::oPopup:Initiate()
         if ( oPopup := ::oPopup:GetPopup( hPopup ) ) != nil
            oPopup:Initiate()
         endif
         return nil
      endif
      if ! Empty( ::oMenu )
         ::oMenu:Initiate()
         if ( oPopup := ::oMenu:GetPopup( hPopup ) ) != nil
            oPopup:Initiate()
         endif
      endif
   else
      if ! Empty( ::oSysMenu )
         ::oSysMenu:Initiate()
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD KeyDown( nKey, nFlags ) CLASS TWindow

   local bKeyAction := SetKey( nKey )


   if nKey == VK_TAB .and. ::oWnd != nil .and. GetKeyState( VK_SHIFT )
      ::oWnd:GoPrevCtrl( ::hWnd )
      return 0
   endif

   if nKey == VK_TAB .and. ::oWnd != nil
      ::oWnd:GoNextCtrl( ::hWnd )
      return 0
   endif

   if nKey == VK_F4 .and. GetKeyState( VK_CONTROL ) .and. ;
      ::oWnd != nil .and. ::oWnd:IsKindOf( "TMDICHILD" )
      ::oWnd:KeyDown( nKey, nFlags )
      return 0
   endif

   if nKey == VK_F6 .and. GetKeyState( VK_CONTROL ) .and. ;
      ::oWnd != nil .and. ::oWnd:IsKindOf( "TMDICHILD" )
      ::oWnd:KeyDown( nKey, nFlags )
      return 0
   endif

  if bKeyAction != nil     // Clipper SET KEYs !!!
     Eval( bKeyAction, ProcName( 4 ), ProcLine( 4 ), Self )
     return 0
   endif

   if nKey == VK_F1
      // ::HelpTopic()  // as WM_HELP is now supported by controls
      return 0
   endif

   if ::OnKeyDown != nil
      Eval( ::OnKeyDown, Self, nKey, nFlags )
   endif

   if ! ::IsKindOf( "TGET" ) .and. ::bKeyDown != nil
      return Eval( ::bKeyDown, nKey, nFlags, Self )
   endif

   if ::oWnd != nil .and. IsChild( ::oWnd:hWnd, ::hWnd )
      ::oWnd:KeyDown( nKey, nFlags )
   endif

return nil

//----------------------------------------------------------------------------//
// Some friends functions

function SetWndDefault( oWnd ) ; oWndDefault := oWnd ; return nil

function GetWndDefault() ; return oWndDefault

//----------------------------------------------------------------------------//

METHOD GotFocus( hFromWnd ) CLASS TWindow

   ::lFocused = .t.

   if ::bGotFocus != nil
      Eval( ::bGotFocus, Self, hFromWnd )
   endif

   if ! Empty( ::hCtlFocus )
      if Upper( GetClassName( ::hCtlFocus ) ) $ "SYSTABCONTROL32,TPAGES,TFOLDEREX"
         AEval( ::aControls, {| Ctrl | If( Ctrl:hWnd != ::hCtlFocus, , ;
                If( ! Empty( Ctrl:aDialogs ), SetFocus( Ctrl:aDialogs[ Ctrl:nOption ]:hCtlFocus ), ) ) } )
      else
         SetFocus( ::hCtlFocus )
      endif
   else
      if ::aControls != nil .and. Len( ::aControls ) > 0
         if ::aControls[ 1 ] != nil
            ::hCtlFocus = NextDlgTab( ::hWnd ) // ,::aControls[ 1 ]:hWnd
            SetFocus( ::hCtlFocus )
         endif
      endif
   endif


return 0   // no standard behavior

//----------------------------------------------------------------------------//

METHOD GoNextCtrl( hCtrl ) CLASS TWindow

   local hCtlNext := NextDlgTab( ::hWnd, hCtrl )

   if ::oWnd:ClassName() $ "TFOLDER,TFOLDEREX,TPAGES"
      if hCtrl == NextDlgTab( ::hWnd, GetWindow( ::hWnd, GW_CHILD ), .T. ) // last ctrl ?
         hCtlNext = NextDlgTab( ::oWnd:oWnd:hWnd, ::oWnd:hWnd )
      endif
   endif

   ::hCtlFocus = hCtrl
   SetFocus( hCtlNext )

return hCtlNext

//----------------------------------------------------------------------------//

METHOD GoPrevCtrl( hCtrl ) CLASS TWindow

   local hCtlPrev := NextDlgTab( ::hWnd, hCtrl, .T. )
   local oCtl, oDlg

   if ::oWnd:ClassName() $ "TFOLDER,TFOLDEREX,TPAGES"
      if hCtrl == NextDlgTab( ::hWnd ) // first ctrl ?
         hCtlPrev = NextDlgTab( ::oWnd:oWnd:hWnd, ::oWnd:hWnd, .T. )
      endif
   endif

   ::hCtlFocus = hCtrl

   if GetClassName( hCtlPrev ) $ "SysTabControl32,TFOLDEREX,TPAGES"
      oCtl = oWndFromHwnd( hCtlPrev )
      oDlg = oCtl:aDialogs[ oCtl:nOption ]
      hCtlPrev = NextDlgTab( oDlg:hWnd, GetWindow( oDlg:hWnd, GW_CHILD ), .T. )
   endif

   SetFocus( hCtlPrev )

return nil

//----------------------------------------------------------------------------//

METHOD GetFont() CLASS TWindow

   local hFont, aInfo, oFont

/*
   if ::oFont == nil .and. ::oWnd != nil .and. ::oWnd:oFont != nil
      ::oFont = ::oWnd:oFont
      ::SendMsg( WM_SETFONT, ::oFont:hFont )
      DEFAULT ::oFont:nCount := 0
      ::oFont:nCount++
      return ::oFont
   endif
*/

   if ::oFont == nil
      if ::oWnd != nil
         ::oFont  = ::oWnd:GetFont()
         if ::oFont != nil
            ::oFont:nCount++
            ::SendMsg( WM_SETFONT, ::oFont:hFont )
         endif
      endif
   endif

   if ::oFont == nil
      if ( hFont := ::SendMsg( WM_GETFONT ) ) != 0
         aInfo = GetFontInfo( hFont )
         oFont = TFont()
         oFont:hFont     = hFont
         oFont:nCount    = 1
         oFont:nHeight   = aInfo[ 1 ]
         oFont:nWeight   = aInfo[ 2 ]
         oFont:lBold     = aInfo[ 3 ]
         oFont:cFaceName = aInfo[ 4 ]
         oFont:nAscent   = aInfo[ 5 ]
         oFont:nDescent  = aInfo[ 6 ]
         oFont:nInternalLeading = aInfo[ 7 ]
         oFont:nExternalLeading = aInfo[ 8 ]
         oFont:nCharSet  = aInfo[ 9 ]
         oFont:lDestroy  = .f.
         ::oFont = oFont
         ::nChrHeight    = aInfo[ 1 ]
         ::nChrWidth     = GetTextWidth( , "B" )
      else
         DEFINE FONT ::oFont NAME GetSysFont() SIZE 0, -12 // BOLD
/*
         if ::oFont:lNew
            ::oFont:nCount--
         endif
*/
         // Decrementing the font count is not necessary after fixing the bug in SetFont method
         // Jun 2014
      endif
   endif

return ::oFont

//----------------------------------------------------------------------------//

METHOD AEvalWhen() CLASS TWindow

   local oControl, aControls := ::aControls

   if aControls != nil .and. ! Empty( aControls )
      for each oControl in aControls
         if oControl != nil .and. oControl:bWhen != nil
            if Eval( oControl:bWhen )
               oControl:Enable()
            else
               oControl:Disable()
            endif
        endif
        if oControl != nil
           if oControl:ClassName $ "TFOLDER,TFOLDEREX,TPAGES"
              oControl:aDialogs[ oControl:nOption ]:AEvalWhen()
           elseif !Empty( oControl:aControls )
              oControl:AEvalWhen()
           endif
        endif
      next oControl
   endif

return nil

//----------------------------------------------------------------------------//

METHOD SetFont( oFont ) CLASS TWindow

/*
   // Code prior to 31 May 2014: Bug: Increments ::oFount:nCount even when ::oFont is same as oFont

   if ::oFont != nil .and. oFont != nil
      if ::oFont:hFont != oFont:hFont
         ::oFont:End()
      endif
   endif

   if oFont != nil
      ::oFont = oFont
      DEFAULT oFont:nCount := 0  // Xbase++ requirement
      oFont:nCount++
      ::SendMsg( WM_SETFONT, oFont:hFont )
   endif
*/
   // Code adopted on 04 Jun 2014

   if oFont != nil .and. ! Empty( oFont:hFont )
       oFont:nCount++
       if ::oFont != nil
          ::oFont:End()
       endif
      ::oFont     = oFont
      ::SendMsg( WM_SETFONT, ::oFont:hFont )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD __SetFocus() CLASS TWindow

   if ::lWhen()
      SetFocus( ::hWnd )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD Load( cInfo ) CLASS TWindow

   local nPos := 1, nProps, n, nLen
   local cData, cType, cBuffer

   nProps = Bin2I( SubStr( cInfo, nPos, 2 ) )
   nPos += 2

   for n = 1 to nProps
      nLen  = Bin2I( SubStr( cInfo, nPos, 2 ) )
      nPos += 2
      cData = SubStr( cInfo, nPos, nLen )
      nPos += nLen
      cType = SubStr( cInfo, nPos++, 1 )
      nLen  = Bin2I( SubStr( cInfo, nPos, 2 ) )
      nPos += 2
      cBuffer = SubStr( cInfo, nPos, nLen )
      nPos += nLen

      do case
         case cType == "A"
              OSend( Self, "_" + cData, ARead( cBuffer ) )

         case cType == "O"
              if cData == "oMenu"
                 ::SetMenu( ORead( cBuffer ) )
              else
                 OSend( Self, "_" + cData, ORead( cBuffer ) )
              endif

         case cType == "C"
              if SubStr( cData, 1, 2 ) == "On"
                 if ::oWnd == nil
                    OSend( Self, "_" + cData, { | u1, u2, u3, u4 | OSend( Self, cBuffer, u1, u2, u3, u4 ) } )
                 else
                    OSend( Self, "_" + cData, { | u1, u2, u3, u4 | OSend( Self:oWnd, cBuffer, u1, u2, u3, u4 ) } )
                 endif
              else
                 OSend( Self, "_" + cData, cBuffer )
              endif
              if cData == "cVarName" .and. ! GetWndDefault() == Self
                 OSend( GetWndDefault(), "_" + ::cVarName, Self )
              endif

         case cType == "L"
              OSend( Self, "_" + cData, cBuffer == ".T." )

         case cType == "N"
              OSend( Self, "_" + cData, Val( cBuffer ) )
      endcase
   next

   if Upper( ::ClassName() ) $ "TLISTBOX;TCOMBOBOX;TTREEVIEW"
      ::SetItems( ::aItems )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD LoadFile( cFileName ) CLASS TWindow

   local cInfo, nPos := 4

   DEFAULT cFileName := ""

   if ! File( cFileName )
      MsgStop( "File not found: " + cFileName )
      return nil
   endif

   cInfo  = MemoRead( cFileName )
   nPos  += ( 2 + Bin2I( SubStr( cInfo, nPos, 2 ) ) )

   ::Load( SubStr( cInfo, nPos ) )

return nil

//----------------------------------------------------------------------------//

METHOD LostFocus( hWndGetFocus ) CLASS TWindow

   ::lFocused = .f.

   if ::oToolTip != nil
      ::oToolTip:Hide()
   endif

   if oToolTip != nil
      oToolTip:End()
      oToolTip = nil
   endif

   if ! Empty( ::bLostFocus )
      return Eval( ::bLostFocus, Self, hWndGetFocus )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD MouseLeave( nRow, nCol, nKeyFlags ) CLASS TWindow

   if ::oToolTip != nil
      ::oToolTip:Hide()
      ::oToolTip:lShowAgain = .T.
   endif

   if ! Empty( ::bMLeave )
      Eval( ::bMLeave, nRow, nCol, nKeyFlags, Self )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD MouseMove( nRow, nCol, nKeyFlags ) CLASS TWindow

   if ::oCursor != nil
      SetCursor( ::oCursor:hCursor )
   else
      CursorArrow()
   endif

   ::SetMsg( ::cMsg )

   ::CheckToolTip( nRow, nCol )

   if ::OnMouseMove != nil
      if ValType( ::OnMouseMove ) == "B"
         Eval( ::OnMouseMove, Self, nRow, nCol, nKeyFlags )
      endif
      if ValType( ::OnMouseMove ) == "C"
         OSend( Self, ::OnMouseMove, Self, nRow, nCol, nKeyFlags )
      endif
   endif

   if ::bMMoved != nil
      return Eval( ::bMMoved, nRow, nCol, nKeyFlags )
   endif

   TrackMouseEvent( ::hWnd, TME_LEAVE )

return 0

//----------------------------------------------------------------------------//

METHOD CheckToolTip( nRow, nCol ) CLASS TWindow

   local aPoint

   if ::oToolTip != nil
      if ::oToolTip:nTop != ::nTop + nRow + 20 .and. ;
         ::oToolTip:nLeft != ::nLeft + nCol + 20
         aPoint = { nRow + 20, nCol + 20 }
         ClientToScreen( ::hWnd, aPoint )
         ::oToolTip:SetPos( aPoint[ 1 ], aPoint[ 2 ] )
      endif
      if ( ! ::oToolTip:IsVisible() ) .and. ::oToolTip:lShowAgain
         ::oToolTip:Show()
         ::oToolTip:lShowAgain = .F.
      endif
      return nil
   endif

   if ::cToolTip == nil .and. ::hWnd != hWndParent
      if ::hWnd != hToolTip
         lToolTip = .f.
      endif
   endif

   if ::cToolTip == nil
      if hPrvWnd != ::hWnd
         hPrvWnd  = ::hWnd
      endif
      if oToolTip != nil
         oToolTip:End()
         oToolTip = NIL
      endif
      if oTmr != NIL
         oTmr:End()
         oTmr = NIL
      endif
   else
      if hPrvWnd != ::hWnd
         hWndParent = GetParent( ::hWnd )
         hPrvWnd    = ::hWnd
         if oToolTip != nil
            oToolTip:End()
            oToolTip = NIL
         endif
         if oTmr != NIL
            oTmr:End()
            oTmr = NIL
         endif

         ::ShowToolTip()

      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD ShowToolTip( nRow, nCol, cToolTip ) CLASS TWindow

   local oTemp, hWnd
   local cIcon, hIcon, cText, cTitle, nClrFore, nClrBack, nWidth
   local nDelayTime, nDelayType

   DEFAULT nCol := 7, nRow := ::nHeight() + 7, ;
           cToolTip := ::cToolTip

   if oToolTip == nil

      if ValType( cToolTip ) == "B"
         cToolTip = Eval( cToolTip, Self, nRow, nCol )
      endif

      DEFINE WINDOW oToolTip FROM 0, 0 TO 1, 5 ;
         STYLE nOr( WS_POPUP, WS_BORDER ) ;
         COLOR 0, RGB( 255, 255, 225 ) OF Self

      oTemp = oToolTip

      DestroyWindow( oToolTip:hWnd )

      if ValType( cToolTip ) == 'A'
         ASize( cToolTip, 8 ) //5 )
         cText    = cValToChar( cToolTip[ 1 ] )
         cTitle   = cToolTip[ 2 ]
         hIcon    = nil
         if ! Empty( cTitle )
            cIcon = cToolTip[ 3 ]
            if ValType( cIcon ) == 'C'
               if Empty( cFileExt( cIcon ) )
                  hIcon    := LoadIcon( GetResources(), cIcon )
               elseif Upper( cFileExt( cIcon ) ) == "ICO" .and. File( cIcon )
                  hIcon    := ExtractIcon( cIcon )
               endif
            elseif ValType( cIcon ) == 'N'
               hIcon    := cIcon
               cIcon    := nil
            else
               cIcon    := nil
            endif
            if Empty( hIcon )
               hIcon    := TTI_INFO
               cIcon    := nil
            endif
         endif

         nClrFore = cToolTip[ 4 ]
         nClrBack = cToolTip[ 5 ]
         nWidth   = cToolTip[ 6 ]
         nDelayTime = cToolTip [ 7 ]
         nDelayType = cToolTip [ 8 ]
      else
         cText    = cValToChar( cToolTip )
      endif

/*
      if IsWindowUnicode( ::hWnd )
//         cText = HB_UTF8LEFT( cText, 98 )
         hWnd = CreateToolTipWNew( ::hWnd, cText, IfNil( ::lBalloon, lTTBalloon ), cTitle, hIcon, nClrFore, nClrBack, nWidth, nDelayTime, nDelayType )
      else
//         cText = Left( cText, 99 )
         hWnd = CreateToolTipANew( ::hWnd, cText, IfNil( ::lBalloon, lTTBalloon ), cTitle, hIcon, nClrFore, nClrBack, nWidth, nDelayTime, nDelayType )
      endif
*/
      hWnd = CreateToolTipWNew( ::hWnd, cText, IfNil( ::lBalloon, lTTBalloon ), cTitle, hIcon, nClrFore, nClrBack, nWidth, nDelayTime, nDelayType )

      if !Empty( cIcon ) .and. ! Empty( hIcon )
         DestroyIcon( hIcon )
      endif

      oToolTip = oTemp
      oToolTip:hWnd = hWnd

      if oToolTip != nil
         hToolTip = oToolTip:hWnd
      endif
   endif

   lToolTip = .T.

return nil

//----------------------------------------------------------------------------//

METHOD Shadow() CLASS TWindow

   SetClassLong( ::hWnd, GCL_STYLE, nOr( GetClassLong( ::hWnd, GCL_STYLE ), CS_DROPSHADOW ) )

return nil

//----------------------------------------------------------------------------//

METHOD DestroyToolTip() CLASS TWindow

  if oToolTip != nil
     oToolTip:End()
     oToolTip = nil
  endif

  hPrvWnd = 0
  hWndParent = 0

return nil

//----------------------------------------------------------------------------//

METHOD NcMouseMove( nHitTestCode, nRow, nCol ) CLASS TWindow

   ( nHitTestCode, nRow, nCol )

   hWndParent = 0
   hPrvWnd  = 0
   lToolTip = .f.
   if ::oToolTip != nil
      ::oToolTip:Hide()
      ::oToolTip:lShowAgain = .T.
   endif
   if oToolTip != NIL
      oToolTip:End()
      oToolTip = NIL
   endif
   if oTmr != NIL
      oTmr:End()
      oTmr = NIL
   endif

return nil

//----------------------------------------------------------------------------//

METHOD CommNotify( nDevice, nStatus ) CLASS TWindow

   if ::bCommNotify != nil
      return Eval( ::bCommNotify, nDevice, nStatus )
   endif

return nil

//----------------------------------------------------------------------------//

METHOD __HelpTopic() CLASS TWindow

   static lShow := .F.

   if ! lShow
      lShow = .T.
      if Empty( ::nHelpId )
         if ::oWnd != nil .and. ;
            ! Upper( ::oWnd:ClassName() ) $ "TFOLDER,TPAGES,TDIALOG,TWINDOW,TMDIFRAME,TMDICHILD,TFOLDEREX"
            ::oWnd:HelpTopic()
         else
            if Empty( GetHelpTopic() )
                HelpIndex()
            else
                HelpTopic()
            endif
         endif
      else
         HelpTopic( ::nHelpId )
      endif
      lShow = .F.
   endif

return nil

//----------------------------------------------------------------------------//

METHOD DropOver( nRow, nCol, nKeyFlags ) CLASS TWindow

   local aPoint   := { nRow, nCol }

   if ::bDropOver != nil
      if oWndDragBegin != nil
         aPoint = ClientToScreen( oWndDragBegin:hWnd, aPoint )
         if aPoint[ 1 ] > 32768
            aPoint[ 1 ] -= 65535
         endif
         if aPoint[ 2 ] > 32768
            aPoint[ 2 ] -= 65535
         endif
         aPoint = ScreenToClient( ::hWnd, aPoint )
      endif
      Eval( ::bDropOver, GetDropInfo(), nRow, nCol, nKeyFlags, aPoint, oWndDragBegin )
      oWndDragBegin  = nil
   endif

return nil

//----------------------------------------------------------------------------//

METHOD _BeginPaint() CLASS TWindow

   local cPS

   if ::nPaintCount == nil
      ::nPaintCount = 1
   else
      ::nPaintCount++
   endif

   ::hDC = BeginPaint( ::hWnd, @cPS )
   ::cPS = cPS

return nil

//----------------------------------------------------------------------------//
// FiveWin own Drag & Drop support functions

function SetDragBeginWnd( oWnd )

   oWndDragBegin = oWnd

return nil

//----------------------------------------------------------------------------//

function SetDropInfo( uInfo )

   uDropInfo = uInfo

return nil

//----------------------------------------------------------------------------//

function GetDropInfo()

   local uInfo := uDropInfo

   uDropInfo = nil

return uInfo

//----------------------------------------------------------------------------//

function WndMain()

   local nAt := AScan( aWindows, { | o | ! Empty( o ) .and. o:hWnd == GetWndApp() } )

return If( nAt != 0, aWindows[ nAt ], nil )

//----------------------------------------------------------------------------//

function _FWH( hWnd, nMsg, nWParam, nLParam, nAt )

   local oWnd

   static aRet := { 0, 0 }

   ( hWnd )

   if nAt != 0
      oWnd = aWindows[ nAt ]
      if ValType( oWnd ) == "O"
         aRet[ 1 ] = oWnd:HandleEvent( nMsg, nWParam, nLParam )
         aRet[ 2 ] = oWnd:nOldProc
      endif
      return aRet
   endif

return nil

//----------------------------------------------------------------------------//

METHOD LastActiveCtrl() CLASS TWindow

   local n := Len( ::aControls )
   local oControl, oRadMenu

   if n == 1
      return ::aControls[ 1 ]
   endif

   while n >= 1
      If Upper( ::aControls[ n ]:ClassName() ) == 'TRADIO'
         oRadMenu = ::aControls[ n ]:oRadMenu
         while n >= 1 .and. Upper( ::aControls[ n ]:ClassName() ) == 'TRADIO' .and. ;
               ::aControls[ n ]:oRadMenu == oRadMenu .and. ( !::aControls[ n ]:lChecked .or. ;
               !::aControls[ n ]:lActive)
            n--
         end
         oControl := ::aControls[ n ]
         exit
      endif
      if ::aControls[ n ]:lActive .and. ;
         lAnd( GetWindowLong( ::aControls[ n ]:hWnd, GWL_STYLE ), WS_TABSTOP )
         oControl := ::aControls[ n ]
         exit
      endif
      n--
   end

   if oControl == nil
      oControl := ::aControls[ 1 ]
   endif

return oControl

//------------------------------------------------------------------------//

METHOD EditTitle( cTitle ) CLASS TWindow

   local oDlg, cOldTitle := PadR( ::cCaption, 100 )

   if Empty( cTitle )
      cTitle = PadR( ::cCaption, 100 )
   else
      cTitle = PadR( cTitle, 100 )
   endif

   DEFINE DIALOG oDlg TITLE "window Title" SIZE 600, 110
   oDlg:lTruePixel := .f.

   @ 1, 1 GET cTitle OF oDlg SIZE 285, 12
      // ON CHANGE ::SetText( oGet:GetText() )

   @ 2, 18 BUTTON "&Ok" OF oDlg ACTION ( ::SetText( AllTrim( cTitle ) ), oDlg:End() )

   @ 2, 25 BUTTON "&Cancel" OF oDlg ACTION ( ::SetText( cOldTitle ), oDlg:End() )

   ACTIVATE DIALOG oDlg CENTERED

return RTrim( cTitle )

//------------------------------------------------------------------------//

METHOD FastEdit( nRow, nCol ) CLASS TWindow

   local oPopup

   MENU oPopup POPUP
      ::FastEditItems()
   ENDMENU

   ACTIVATE POPUP oPopup WINDOW Self AT nRow, nCol

return nil

//------------------------------------------------------------------------//

METHOD FastEditItems() CLASS TWindow

   local oThis := Self

   MENUITEM "Source code..." ACTION ( oMenuItem ), SourceEdit( oThis:cGenPrg(), "Source code" )

return nil

//------------------------------------------------------------------------//

METHOD FirstActiveCtrl() CLASS TWindow

   local n := 1
   local oControl, oRadMenu

   if Len( ::aControls ) == 1
      return ::aControls[ 1 ]
   endif

   while n <= Len( ::aControls )
      If Upper( ::aControls[ n ]:ClassName() ) == 'TRADIO'
         oRadMenu = ::aControls[ n ]:oRadMenu
         while n <= Len( ::aControls ) .and. Upper( ::aControls[ n ]:ClassName() ) == 'TRADIO' .and. ;
               ::aControls[ n ]:oRadMenu == oRadMenu .and. ( !::aControls[ n ]:lChecked .or. ;
               !::aControls[ n ]:lActive )
            n++
         end
         oControl := ::aControls[ n ]
         exit
      endif
      If ::aControls[ n ]:lActive .and. ;
         lAnd( GetWindowLong( ::aControls[ n ]:hWnd, GWL_STYLE ), WS_TABSTOP )
         oControl := ::aControls[ n ]
         exit
      endif
      n++
   end

   if oControl == nil
      oControl = ::aControls[ 1 ]
   endif

return oControl

//------------------------------------------------------------------------//

METHOD GetSetGWLStyle( lExtended, nStyle, lOnOff ) CLASS TWindow

   local nGwl     := If( lExtended, GWL_EXSTYLE, GWL_STYLE )
   local nSet, uRet

   nSet     := If( Empty( ::hWnd ), If( lExtended, ::nExStyle, ::nStyle ), GetWindowLong( ::hWnd, nGwl ) )
   uRet     := nSet

   if ValType( nStyle ) == 'N'
      uRet     := lAnd( nSet, nStyle )
      if ValType( lOnOff ) == 'L'
         if Empty( ::hWnd )
            nSet  := If( lOnOff, nOr( nSet, nStyle ), nAnd( nSet, nNot( nStyle ) ) )
            if lExtended
               ::nExStyle  := nSet
            else
               ::nStyle    := nSet
            endif
         else
            if uRet .and. !lOnOff
               SetWindowLong( ::hWnd, nGwl, nAnd( nSet, nNot( nStyle ) ) )
            elseif ! uRet .and. lOnOff
               SetWindowLong( ::hWnd, nGwl, nOr( nSet, nStyle ) )
            endif
         endif
      endif
   endif

return uRet

//----------------------------------------------------------------------------//

METHOD SetAlphaLevel() CLASS TWindow

   local nExStyle    := GetWindowLong( ::hWnd, GWL_EXSTYLE )

   if !lAnd( ::nStyle, WS_CHILD )
      if ::hAlphaColor == nil .and. ::hAlphaLevel == nil
         // cancel layered style
         if lAnd( nExStyle, WS_EX_LAYERED )
            SetWindowLong( ::hWnd, nAnd( nExStyle, nNot( WS_EX_LAYERED ) ) )
         endif
      else
         if ! lAnd( nExStyle, WS_EX_LAYERED )
            SetWindowLong( ::hWnd, GWL_EXSTYLE, nOr( nExStyle, WS_EX_LAYERED ) )
         endif
         // Check range
         if ::hAlphaColor != nil
            ::hAlphaColor  := nAnd( ::hAlphaColor, 0xFFFFFF )
         endif
         if ::hAlphaLevel != nil
            ::hAlphaLevel  := nAnd( ::hAlphaLevel, 255 )
         endif
         SetLayeredWindowAttributes( ::hwnd,   ;
                  IfNil( ::hAlphaColor, 0 ),   ;
                  IfNil( ::hAlphaLevel, 255 ), ; // ignored for LWA_COLORKEY
                  If( ::hAlphaColor == nil, LWA_ALPHA, LWA_COLORKEY ) )
      endif
   endif

return Self

//----------------------------------------------------------------------------//

METHOD OnDisplayChange( nBitsPerPixel, nWidth, nHeight ) CLASS TWindow

   local uRet

   if ValType( ::bOnDisplayChange ) == 'B'
      uRet :=  Eval( ::bOnDisplayChange, Self, nBitsPerPixel, nWidth, nHeight )
      if ValType( uRet ) == 'N' .and. uRet == 0
         return 0
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD OnSettingChange( nWParam, nLParam ) CLASS TWindow

   local uRet

   DeviceTouchSpace( .t. )

   if ValType( ::bOnSettingChange ) == 'B'
      uRet  := Eval( ::bOnSettingChange, Self, nWParam, nLParam )
      if ValType( uRet ) == 'N' .and. uRet == 0
         return 0
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD OnDeviceChange( nWParam, nLParam ) CLASS TWindow

   local uRet

   DeviceTouchSpace( .t. )

   if ValType( ::bOnDeviceChange ) == 'B'
      uRet  := Eval( ::bOnDeviceChange, Self, nWParam, nLParam )
      if ValType( uRet ) == 'N' .and. uRet == 0
         return 0
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD HandleGesture( nGesture, nLParam ) CLASS TWindow

   local uRet

   if ValType( ::bHandleGesture ) == 'B'
      uRet := Eval( ::bHandleGesture, Self, nGesture, nLParam )
      if ValType( uRet ) == 'N' .and. uRet == 0
         return 0
      endif
   endif

return nil

//----------------------------------------------------------------------------//

METHOD AppFocus( lGotFocus ) CLASS TWindow

   if ::bAppFocus != nil
      Eval( ::bAppFocus, lGotFocus, Self )
      return 0
   endif

return nil

METHOD __SetWindowTheme( cSub, cIds )
return SetWindowTheme( cSub, cIds )

//----------------------------------------------------------------------------//
// SUPPORT FUNCTIONS
//----------------------------------------------------------------------------//

function WndParents( xWnd1, xWnd2 )

   Local aParent1, aParent2
   Local nFor, nLen

   aParent1 := ScanParents( xWnd1 )
   aParent2 := ScanParents( xWnd2 )

   nLen := Len( aParent1 )

   for nFor := 1 to nLen
      if Ascan( aParent2, aParent1[ nFor ] ) > 0
         return .T.
      end
   next

return .F.

//----------------------------------------------------------------------------//

static function ScanParents( xWnd )

   local aWnd := {}
   local cClassName
   local hWnd

   if ValType( xWnd ) == "O"
      xWnd = xWnd:hWnd
   end

   while ( hWnd := GetParent( xWnd ) ) != 0
      cClassName = Upper( GetClassName( hWnd ) )
/*
      // commented out in FWH 13.02
      if "BROWSE" $ cClassName // Both TWBrowse and TCBrowse
         exit
      endif
*/

/*
      // commented out in FWH 13.03
      oWnd  = oWndFromhWnd( hWnd )
      if oWnd != nil
         if oWnd:IsKindOf( "TXBROWSE" ) .or. oWnd:IsKindOf( "TWBROWSE" )
            exit
         endif
      endif
*/

      // BEGIN INSERT IN FWH 13.03
      if '|' + cClassName + '|' $ cBrwClassList
         exit
      endif
      // END INSERT FWH 13.03


      AAdd( aWnd, hWnd )
      if cClassName == "#32770" .and. ;
         ! lAnd( GetWindowLong( hWnd, GWL_STYLE ), WS_CHILD )
         exit
      endif
      if cClassName $ "TMDICHILD;TWINDOW"
         exit
      end
      xWnd := hWnd
   end

return aWnd

//----------------------------------------------------------------------------//

function oWndFromhWnd( hWnd )

   local nAt := AScan( aWindows,;
                       { | o | ValType( o ) == "O" .and. o:hWnd == hWnd } )

return If( nAt > 0, aWindows[ nAt ] , nil )

//----------------------------------------------------------------------------//

function SetAlpha( lOnOff )

  local lOldStatus

  static lStatus := .T.

  lOldStatus = lStatus

  if PCount() == 1 .and. ValType( lOnOff ) == "L"
     lStatus = lOnOff
  endif

return lOldStatus

//----------------------------------------------------------------------------//

function DBufferEnd( hDC, aRestore )

   aRestore[ 3 ] = hDC
   FWDispEnd( aRestore )

return nil

//----------------------------------------------------------------------------//

function BrwClasses( cAddClass )  // added FWH 13.03

   if ! Empty( cAddClass ) .and. ! ( '|' + cAddClass + '|' $ cBrwClassList )
      cBrwClassList     += cAddClass + '|'
   endif

return cBrwClassList

//----------------------------------------------------------------------------//

function FW_TouchFriendly( lNew )

   static lStatus := .f.

   local lRet     := lStatus

   if HB_ISLOGICAL( lNew )
      lStatus  := lNew
   endif

return lRet

//----------------------------------------------------------------------------//

function DeviceTouchSpace( lRecalc )

   static nPix

   local hDC

   if nPix == nil .or. lRecalc == .t.
      hDC   := GetDC( 0 )

      // Calculate for 6 milli metres

      nPix  := Round( 6.0 * GetDeviceCaps( hDC, VERTRES ) / GetDeviceCaps( hDC, VERTSIZE ), 0 )
      ReleaseDC( 0, hDC )
   endif

return nPix

//----------------------------------------------------------------------------//

EXIT PROCEDURE exit_twindow

   if hSysMenuFont != nil
      DeleteObject( hSysMenuFont )
   endif

return

//----------------------------------------------------------------------------//

Function StartFWLog( nTop, nLeft, nHeight, nWidth, lDown, lLines, lCouple, cTitle )

   DEFAULT nWidth    := Int( GetSysMetrics( 0 ) / 3 )
   DEFAULT nHeight   := Int( GetSysMetrics( 1 ) * 0.8 )
   DEFAULT nTop      := 32
   DEFAULT nLeft     := GetSysMetrics( 0 ) - nWidth
   DEFAULT lDown     := .F.
   DEFAULT lLines    := .T.
   DEFAULT lCouple   := .T.
   DEFAULT cTitle    := ""
   lDownLog          := lDown
   lLineLog          := lLines
   lDlgCouple        := lCouple
   nLeft             -= 4
   aRectWinLog       := { nTop, nLeft, nHeight, nWidth }
   if Len( hb_aParams() ) > 0
      MsgLog( "Start", nTop, nLeft, nHeight, nWidth, cTitle )
   endif

Return lCouple

//----------------------------------------------------------------------------//

Function GetMsgLog()
Return oWinDlgLog

//----------------------------------------------------------------------------//

#define MF_BYPOSITION  1024  // 0x0400
#define MF_DISABLED       2

Function MsgLog( xValor, nR, nC, nH, nW, cTitle )

   local oSayLog
   local cVarT      := ""
   local cLine
   local nLinesLog
   local oBarLog
   local oBtn
   local cFileSave  := ""
   if Empty( aRectWinLog )
      StartFWLog()
   endif
   DEFAULT nH       := aRectWinLog[ 3 ]
   DEFAULT nW       := aRectWinLog[ 4 ]
   DEFAULT nR       := aRectWinLog[ 1 ]
   DEFAULT nC       := aRectWinLog[ 2 ]
   DEFAULT cTitle   := "MsgLog"

   if Empty( AScan( GetAllWin(), { | o | o:ClassName() == "TDIALOG" .and. o:Cargo = "MsgLog" } ) )
      cFileSave    := Lower( cFileSetExt( cFilePath( SetLogFile() ) + "FW_" + cFileNoPath( ExeName() ), "log" ) )
      DEFINE DIALOG oWinDlgLog FROM nR, nC TO nR + nH, nC + nW TITLE cTitle PIXEL
      oWinDlgLog:Cargo     := "MsgLog"
      oWinDlgLog:lHelpIcon := .F.

      DEFINE BUTTONBAR oBarLog OF oWinDlgLog SIZE 32, 32 2015 NOBORDER // HEIGHT 56
      DEFINE BUTTON oBtn OF oBarLog GROUP ;
         ACTION  ( This ), oWinDlgLog:End() ;
         TOOLTIP FWString( "Exit" )
         oBtn:hBitMap1 := FWBitmap( "Exit2" )

      DEFINE BUTTON oBtn OF oBarLog GROUP ;
         ACTION ( ( This ), oWinDlgLog:aControls[ 2 ]:GoTop(), oWinDlgLog:aControls[ 2 ]:Refresh() ) ;
         TOOLTIP FWString( "Top" )
         oBtn:hBitMap1 := FWBitmap( "top2" )

      DEFINE BUTTON oBtn OF oBarLog ;
         ACTION  ( This ), oWinDlgLog:aControls[ 2 ]:GoBottom() ;
         TOOLTIP FWString( "Bottom" )
         oBtn:hBitMap1 := FWBitmap( "Bottom2" )

      DEFINE BUTTON oBtn OF oBarLog GROUP ;
         ACTION ( ( This ), lLineLog  := !lLineLog, oSayLog:Refresh() ) ;
         TOOLTIP "Show / Hide Lines number"

      oBtn:hBitMap1 := FWBitmap( "Bottom2" )
      oBtn:hBitmap2 := FWBitmap( "previous2"  )
      oBtn:bBmpNo   := { || If( lLineLog, 2, 1 ) }

      DEFINE BUTTON oBtn OF oBarLog GROUP ;
         TOOLTIP FWString( "Save" ) ;
         ACTION ( This ), LogFile( cFileSave, oWinDlgLog:aControls[ 2 ]:VarGet(), .F. )
         oBtn:hBitMap1 := FWBitmap( "Save" )

      DEFINE BUTTON oBtn OF oBarLog GROUP ;
         TOOLTIP FWString( "Print" ) WHEN .F.
         oBtn:hBitMap1  := FWBitmap( "Printer" )

      @ Int( oBarLog:nBottom / 2 ) + 11, 00 GET cVarT MEMO ;
         SIZE Int( ( nW ) / 2 ) - 4, ;
              Int( ( nH ) / 2 ) - ( Int( oBarLog:nBottom / 2 ) + 11 ) ;
         PIXEL OF oWinDlgLog NOBORDER COLOR CLR_BLUE, Rgb( 245, 245, 245 )

      @ Int( oBarLog:nBottom / 2 ) + 1, 00 SAY oSayLog PROMPT if( lLineLog, "  Count ", "  Count " ) + ;
         "      Value" ;
         PIXEL OF oWinDlgLog ;
         SIZE Int( ( nW ) / 2 ), 10 COLOR CLR_WHITE, CLR_GRAY VCENTER

      ACTIVATE DIALOG oWinDlgLog NOWAIT ;
         ON INIT ( ( Self ), if( lDlgCouple, SetWindowLong( oWinDlgLog:hWnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW ), ), ;
                   RemoveMenu( GetSystemMenu( oWinDlgLog:hWnd, .F.), 1, nOR( MF_BYPOSITION, MF_DISABLED) ) ) ;
         VALID ( ( Self ), aRectWinLog := { oWinDlgLog:nTop, oWinDlgLog:nLeft, oWinDlgLog:nBottom, oWinDlgLog:nRight }, .T. )

   endif
   if !Empty( oWinDlgLog )
      // Seguramente podamos prescindir del contador nLinesLog
      nLinesLog   := MLCount( AllTrim( oWinDlgLog:aControls[ 2 ]:VarGet() ) ) + 1
      SetForeGroundWindow( oWinDlgLog:hWnd )
      if xValor = Nil                  //Nul
         xValor := "NULL"
      elseif ValType( xValor ) = "N"   //Numero
         xValor := LTrim( Str( xValor ) ) + CRLF + Chr( 9 )
      elseif ValType( xValor ) = "D"   //Data
         xValor := DToC( xValor ) + CRLF + Chr( 9 )
      elseif ValType( xValor ) = "L"   //Logic
         xValor := if( xValor, "TRUE", "FALSE" ) + CRLF + Chr( 9 )
      elseif ValType( xValor ) = "A"   //Array
         xValor := FW_ArrayAsList( xValor, CRLF + Chr( 9 ) )
      elseif ValType( xValor ) = "O"   //Object
         xValor := xValor:ClassName() + CRLF + ;
                   FW_ArrayAsList( ClearDatasList( xValor ), CRLF + Chr( 9 ) ) //CRLF )
      endif
      WITH OBJECT oWinDlgLog:aControls[ 2 ]
         cLine     := DToC( Date() ) + " " + Time() + " - "
         if !lDownLog
            :Gotop()
            :VarPut( StrZero( nLinesLog, 6 ) + " - " + cLine + xValor + CRLF + :VarGet() )
         else
            :GoBottom()
            :VarPut( if( !Empty( :VarGet() ), :VarGet() + CRLF, "" ) + ;
                  StrZero( nLinesLog, 6 ) + " - " + cLine + xValor )
         endif
         :Refresh()
      END
   endif

Return Nil

//----------------------------------------------------------------------------//

Function ClearDatasList( oValor, aVals )

   local x
   local aList := ASort( __objGetValueList( oValor, { "LXUNICODE" } ) )
   DEFAULT aVals := {}
   For x = 1 to Len( aList )
      if !hb_IsNil( aList[ x ][ 2 ] )
         AAdd( aVals, { aList[ x ][ 2 ], Chr( 9 ) + aList[ x ][ 1 ] } )
      endif
   Next x

Return aVals

//----------------------------------------------------------------------------//

Function GetSetWinSysLog( lValue )

   if !HB_IsNIL( lValue ) .and. ValType( lValue ) = "L"
      lFwSysLog   := lValue
   endif

Return lFwSysLog

//------------------------------------------------------------------------//

init procedure InitTime()

   aInitInfo := { Date(), Seconds() }

return

//------------------------------------------------------------------------//

function TimeFromStart()

   local nDay := Date() - aInitInfo[ 1 ], ;
         nSecond := Seconds(), cRet := "", ;
         nYear, nMounth, nHour, nMin, nSec

   if nDay < 0 .or. ( nDay == 0 .and. nSecond < aInitInfo[ 2 ] )
      return "System Date/Time changed"
   endif

   if nDay == 0
      nSecond -= aInitInfo[ 2 ]
   else
      nYear   := Int( nDay / 365 )
      nMounth := Int( ( nDay - ( nYear * 365 ) ) / 30 )
      nDay    := Int( nDay - ( ( nYear * 365 ) + ( nMounth * 30 ) ) )

      if nYear > 0
         cRet += AllTrim( Str( nYear ) ) + " year "
      endif

      if nMounth > 0
         cRet += AllTrim( Str( nMounth ) ) + " mounth "
      endif

      if nDay > 0 .and. nSecond < aInitInfo[ 2 ] .and. ( ( 86400 - aInitInfo[ 2 ] ) + nSecond ) < 86400
         nDay--
         nSecond += ( 86400 - aInitInfo[ 2 ] )
      endif

      if nDay > 0
         cRet += AllTrim( Str( nDay ) ) + " day "
      endif
   endif

   nHour := Int( nSecond / 3600 )
   nMin  := Int( ( nSecond - ( nHour * 3600 ) ) / 60 )
   nSec  := Int( nSecond - ( nHour * 3600 ) - ( nMin * 60 ) )

   if nHour > 0
      cRet += AllTrim( Str( nHour ) ) + " hour "
   endif

   if nMin > 0
      cRet += AllTrim( Str( nMin ) ) + " min "
   endif

return cRet += AllTrim( Str( nSec ) ) + " sec"

//----------------------------------------------------------------------------//

function GetWndTitleHeight( hWnd )

   local oWnd, aRect, aPoint
   
   if IsObj( hWnd )
      oWnd := hWnd
      hWnd := oWnd:hWnd
   endif
   
   if !IsWindow( hWnd )
      return 0
   endif
   
   aRect  := GetWndRect( hWnd )
   aPoint := ClienttoScreen( hWnd, { 0, 0 } )
   
   return ( aPoint[ 1 ] - aRect[ 1 ] ) + ( aPoint[ 2 ] - aRect[ 2 ] )

//----------------------------------------------------------------------------//

/*

#pragma BEGINDUMP

#include <windows.h>
#include <uxtheme.h>
#include <hbapi.h>
#include <fwh.h>

HB_FUNC_STATIC( _SETWINDOWTHEME )
{
   HRESULT h = SetWindowTheme( ( HWND ) fw_parH( 1 ), ( LPCWSTR ) fw_parWide( 2 ), ( LPCWSTR ) fw_parWide( 3 ) );
   hb_retl( h == S_OK );
}

#pragma ENDDUMP

//----------------------------------------------------------------------------//

*/

