/*
	BitPaint.c
	Trivial example of a Toolbox/Carbon app
	using only API compatible with System 1.0
 
	Compiles for:
		m68k: Toolbox, System 1 - Mac OS 9.2.2
		ppc: Carbon CFM, Mac OS 8.1 - Mac OS X v10.6
		i386: Carbon Mach-O, Mac OS X v10.4 - 10.10
 
 */

#if TARGET_API_MAC_CARBON

#if __MACH__
#include <Carbon/Carbon.h>
#else
#include <Carbon.h>
#endif

#else

#include <stdio.h>
#include <Quickdraw.h>
#include <MacWindows.h>
#include <Dialogs.h>
#include <Menus.h>
#include <ToolUtils.h>
#include <Devices.h>

#endif

#include "BitPaint.h"

#define APP_NAME_STRING "\pBitPaint"
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 320
#define BITMAP_SIZE 16

WindowPtr mainWindowPtr;
Boolean quit = 0;
Rect picBounds = {0, 0, BITMAP_SIZE, BITMAP_SIZE};

char *picBitmap;

int MouseState = 0;
int HeldPixel = 0;

const short appleM = 0;
const short fileM = 1;

#define TARGET_API_MAC_TOOLBOX (!TARGET_API_MAC_CARBON)

#if TARGET_API_MAC_TOOLBOX

#define GetWindowPort(w) w
QDGlobals qd;

#endif

Rect BPGetBounds()
{
	Rect r;
	
#if TARGET_API_MAC_CARBON
	BitMap 			theScreenBits;
	GetWindowPortBounds(mainWindowPtr, &r);
#else
	r = qd.thePort->portRect;
#endif
	
	return r;
}

Rect BPGetScreenBounds()
{
	Rect r;
	BitMap theScreenBits;
	
#if TARGET_API_MAC_CARBON
	GetQDGlobalsScreenBits( &theScreenBits ); /* carbon accessor */
	r = theScreenBits.bounds;
#else
	r = qd.screenBits.bounds;
#endif
	
	return r;
}

void BPDrawWindow()
{
	Rect r = BPGetBounds();
	Rect pixelRect;
	int x = 0, y = 0;
	Pattern pattern;
	
	EraseRect(&r);
	
	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 32; x++)
		{
			SetRect(&pixelRect, x*20 - 1, y*20 - 1, (x*20)+20, (y*20)+20);
			
			if (picBitmap[y*BITMAP_SIZE+x] == 0)
			{
				
#if TARGET_API_MAC_CARBON
				FillRect(&pixelRect, GetQDGlobalsBlack(&pattern));
#else
				FillRect(&pixelRect, &qd.black);
#endif
			}
		}
	}
}

void BPTogglePixel(int x, int y)
{
	if (MouseState == 1)
	{
		HeldPixel = !picBitmap[y*BITMAP_SIZE+x];
		picBitmap[y*BITMAP_SIZE+x] = HeldPixel;
		MouseState = 2;
	}
	else
	{
		picBitmap[y*BITMAP_SIZE+x] = HeldPixel;
	}
}

void BPReset()
{
	int x = 0, y = 0;
	
	if (!picBitmap)
		picBitmap = NewPtr(BITMAP_SIZE*BITMAP_SIZE*sizeof(unsigned int));
	
	for (y = 0; y < BITMAP_SIZE; y++)
	{
		for (x = 0; x < BITMAP_SIZE; x++)
		{
			picBitmap[y*BITMAP_SIZE+x] = 1;
		}
	}
	
	BPDrawWindow();
}

void DoCommand(long mResult)
{
	short theItem;
	short theMenu;
	Str255		daName;
	short		daRefNum;
	
	theItem = LoWord(mResult);
	theMenu = HiWord(mResult);
	
	switch (theMenu)
	{
		case mApple:
		{
			if (theItem == mAppleAbout)
			{
				Alert(rUserAlert, nil);
			}
#if TARGET_API_MAC_TOOLBOX
			else
			{
				/* all non-About items in this menu are Desk Accessories */
				/* type Str255 is an array in MPW 3 */
				GetMenuItemText(GetMenuHandle(mApple), theItem, daName);
				daRefNum = OpenDeskAcc(daName);
			}
#endif
			break;
		}
		case mFile:
		{
			switch (theItem) {
				case mFileReset:
					BPReset();
					break;
					
				case mFileQuit:
					quit = 1;
					break;
					
				default:
					break;
			}
			
			break;
		}
		default:
			break;
	}
	
	HiliteMenu(0);
}

int lastTick = 0;

void RunLoop()
{
	EventRecord    theEvent;
	WindowPtr whichWindow;
	Rect windRect;
	Boolean        gotevent = 0;
	while (!quit)
	{
#if TARGET_API_MAC_TOOLBOX
		SystemTask();
#endif
		
		if (GetNextEvent(everyEvent, &theEvent))
		{
			switch (theEvent.what)
			{
				case mouseUp:
				{
					MouseState = 0;

					break;
				}
				case mouseDown:
				{
					switch (FindWindow(theEvent.where, &whichWindow))
					{
#if TARGET_API_MAC_TOOLBOX
						case inSysWindow: // Click happens in a Desk Accessory
						{
							SystemClick(&theEvent, whichWindow);
							break;
						}
#endif
						case inMenuBar:
						{
							DoCommand(MenuSelect(theEvent.where));
							break;
						}
						case inDrag:
						{
							windRect = BPGetScreenBounds();
							DragWindow(whichWindow, theEvent.where, &windRect);
							break;
						}
						case inContent:
						{
							if (whichWindow != FrontWindow())
							{
								SelectWindow(whichWindow);
							}
							else
							{
								MouseState = 1;
							}
							

							break;
						}
							
						default:
							break;
					}
					break;
				}
					
				case autoKey:
				case keyDown:
				{
					char theChar = (theEvent.message&charCodeMask);
					
					if (theEvent.modifiers&cmdKey)
					{
						DoCommand(MenuKey(theChar));
						
					}
					
					break;
				}
				case activateEvt:
				{
					if (theEvent.modifiers&activeFlag)
					{
					}
					else
					{
					}
					break;
				}
				case updateEvt:
				{
					Rect pr = BPGetBounds();
					
					BeginUpdate((WindowPtr)theEvent.message);
					EraseRect(&pr);
					BPDrawWindow();
					EndUpdate((WindowPtr)theEvent.message);
					break;
				}
				default:
					break;
			}
		}
		
		if (MouseState != 0)
		{
			GlobalToLocal(&theEvent.where);
			
			BPTogglePixel((theEvent.where.h/20), (theEvent.where.v/20));
			
			if ((TickCount()-lastTick) > 0)
			{
				BPDrawWindow();
				lastTick = TickCount();
			}
		}
		
	}
}

void SetUpMenus()
{
	short i;
	OSErr err;
	long result;
	MenuRef menu;
	
	MenuHandle myMenus[3];
	myMenus[appleM] = GetMenu(mApple);
#if TARGET_API_MAC_TOOLBOX
	AddResMenu(myMenus[appleM],'DRVR'); // System-provided Desk Accessories menu
#endif
	myMenus[appleM] = GetMenu(mApple);
	myMenus[fileM] = GetMenu(mFile);
	
	for (i = 0; i < 2; i++)
	{
		InsertMenu(myMenus[i], 0);
	}
	
#if TARGET_API_MAC_CARBON
	/* In OS X, 'Quit' moves from File to the Application Menu */
	err = Gestalt(gestaltMenuMgrAttr, &result);
	
	if (!err && (result & gestaltMenuMgrAquaLayoutMask)) {
		menu = GetMenuHandle (mFile);
		DeleteMenuItem(menu, mFileQuit);
	}
#endif
	
	DrawMenuBar();
}

#if TARGET_API_MAC_CARBON
/* 
	Quit Apple event handler is necessary for OS X
 */
static pascal OSErr QuitAppleEventHandler (const AppleEvent *appleEvt,
										   AppleEvent* reply, UInt32 refcon)
{
	quit = 1;
	return noErr;
}
#endif

void Initialize(void)
{
	OSErr err;
	
#if TARGET_API_MAC_TOOLBOX
	InitGraf((Ptr) &qd.thePort);
	InitFonts();
#endif
	
	FlushEvents(everyEvent, 0);
	
#if TARGET_API_MAC_TOOLBOX
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
#endif
	InitCursor();
	
#if TARGET_API_MAC_CARBON
	err = AEInstallEventHandler( kCoreEventClass, kAEQuitApplication,
								NewAEEventHandlerUPP(QuitAppleEventHandler), 0, false);
	if (err != noErr) ExitToShell();
#endif
	
	SetUpMenus();
}

void main()
{
	Rect windowRect;
	
	Initialize();
	
	SetRect(&windowRect, 50, 50, 50+SCREEN_WIDTH, 50+SCREEN_HEIGHT);
	
	mainWindowPtr = NewWindow(nil, &windowRect, APP_NAME_STRING, true, noGrowDocProc, (WindowPtr)-1L, true, (long)nil);
	SetPort(GetWindowPort(mainWindowPtr));
	
	BPReset();
	
	RunLoop();
}
