/*
    windemo.c uses wcmenu.h to implement a curses menu
    by Stefan Wessels, February 2017.
*/
#include "wcmenu.h"
#include <stdio.h>

static const char *cszWindowClass = "MenuDemoClass";
static const char *cszWindowName = "Windows Demo";

/*
 * class that holds all elements a Windows app needs
 * to work, and also has a few elements the menu
 * system needs
 */
class WinApp
{
public:
	HWND		hWnd;
	HINSTANCE	hInstance;
	HINSTANCE	hPrevInstance;
	LPSTR		lpCmdLine;
	int			nShowCmd;
	char*		szWindowClass;
	char*		szMainMenu;
	char*		szWindowName;
	WNDPROC		lpfnWndProc;
	int			nExitCode;

	/* this is for the menu demo */
	unsigned int nRawKeyState;
	HFONT		hFont;
	int 		fontWidth;

private:

public:
	WinApp() :
		hWnd(NULL),
		hInstance(NULL),
		hPrevInstance(NULL),
		lpCmdLine(0),
		nShowCmd(0),
		szWindowClass(NULL),
		szMainMenu(NULL),
		szWindowName(NULL),
		lpfnWndProc(NULL),
		nExitCode(0)
		{;}

	~WinApp() {;}

	void	Setup(WNDPROC wndProc, const char *classname = cszWindowClass, const char *windowname = cszWindowName, const char *menuname = NULL);
	bool	Activate(HINSTANCE hinstace, HINSTANCE  hprevinstance, LPSTR lpcmdline, int nshowcmd);
	void	Run(void);
	int		GetExitCode(void) { return nExitCode; }
};

/*
 * the global application object
 */
WinApp	theApp;

/*
 * message loop for this application
 * demo specific - records some key input from windows
 */
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_KEYDOWN:
			switch(wParam)
			{
				case VK_RETURN:
					theApp.nRawKeyState |= WC_INPUT_KEY_ENTER;
				break;

				case VK_ESCAPE:
					theApp.nRawKeyState |= WC_INPUT_KEY_ESCAPE;
				break;

				case VK_UP:
					theApp.nRawKeyState |= WC_INPUT_KEY_UP;
				break;

				case VK_DOWN:
					theApp.nRawKeyState |= WC_INPUT_KEY_DOWN;
				break;
			}
			break;

		case WM_KEYUP:
			switch(wParam)
			{
				case VK_RETURN:
					theApp.nRawKeyState &= ~WC_INPUT_KEY_ENTER;
				break;

				case VK_ESCAPE:
					theApp.nRawKeyState &= ~WC_INPUT_KEY_ESCAPE;
				break;

				case VK_UP:
					theApp.nRawKeyState &= ~WC_INPUT_KEY_UP;
				break;

				case VK_DOWN:
					theApp.nRawKeyState &= ~WC_INPUT_KEY_DOWN;
				break;
			}
			break;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

/*
 * the main application
 */
int WINAPI WinMain(HINSTANCE hInstace, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	theApp.Setup(MainWndProc);
	if(theApp.Activate(hInstace, hPrevInstance, lpCmdLine, nShowCmd))
		theApp.Run();

	return theApp.GetExitCode();
};

/*
 * show a dialog of system error messages
 */
void SystemErrorMessageBox(void)
{
	LPVOID	lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER  | FORMAT_MESSAGE_IGNORE_INSERTS, 
		0, 
		GetLastError(), 
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		(LPTSTR)&lpMsgBuf, 
		0, 
		0);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION);
	LocalFree(lpMsgBuf);
}

/*
 * fill in some application needed variables
 */
void WinApp::Setup(WNDPROC wndProc, const char *classname, const char *windowname, const char *menuname)
{
	lpfnWndProc		= wndProc;
	szWindowName	= (char*)windowname;
	szMainMenu		= (char*)menuname;
	szWindowClass	= (char*)classname;
}

/*
 * do some neccesary windows registration and such
 */
bool WinApp::Activate(HINSTANCE hinstace, HINSTANCE hprevinstance, LPSTR lpcmdline, int nshowcmd)
{
	WNDCLASSEX wcx; 

	if(!lpfnWndProc)
		return false;

	hInstance			= hinstace;
	hPrevInstance		= hprevinstance;
	lpCmdLine			= lpcmdline;
	nShowCmd			= nshowcmd;

	wcx.cbSize			= sizeof(WNDCLASSEX);
	wcx.style			= CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc		= lpfnWndProc;
	wcx.cbClsExtra		= 0;
	wcx.cbWndExtra		= 0;
	wcx.hInstance		= hInstance;
	wcx.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	wcx.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcx.lpszMenuName	= szMainMenu;
	wcx.lpszClassName	= szWindowClass;
	wcx.hIconSm			= (HICON)LoadImage(hInstance, MAKEINTRESOURCE(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR); 

	if(!RegisterClassEx(&wcx))
	{
		SystemErrorMessageBox();
		return false;
	}

	hWnd = CreateWindow(
		szWindowClass,
		szWindowName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(HWND) NULL,
		(HMENU) NULL,
		hInstance,
		(LPVOID) NULL);

	if(!hWnd)
	{
		SystemErrorMessageBox();
		return false;
	}

	ShowWindow(hWnd, nShowCmd);
	UpdateWindow(hWnd);

	return true;
 	
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*\
              D E M O    C O D E    S T A R T S    H E R E
\*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

COLORREF COLOR_GREEN	= RGB(0x00, 0xff, 0x00);
COLORREF COLOR_BLUE		= RGB(0x00, 0x00, 0xff);
COLORREF COLOR_YELLOW	= RGB(0xff, 0xff, 0x00);
COLORREF COLOR_CYAN		= RGB(0x00, 0xff, 0xff);
COLORREF COLOR_WHITE	= RGB(0xff, 0xff, 0xff);

struct
{
	COLORREF 	fore;
	COLORREF 	back;
} colours[] = {

        { COLOR_BLUE, 	COLOR_CYAN },
        { COLOR_YELLOW, COLOR_BLUE },
        { COLOR_GREEN, 	COLOR_BLUE },
        { COLOR_WHITE, 	COLOR_BLUE },
        { COLOR_WHITE, 	COLOR_GREEN },
        { COLOR_CYAN, 	COLOR_BLUE },
};

/* app specific user structure pointed to by userData_ptr */
typedef struct tagUserData
{
    int value;
    int length;
} UserData;

/* make a buffer and use printf style formatting to put a string in the buffer */
WC_GLOBAL char *makeString(char *format, ... )
{
    int size;
    char *buffer = 0;
    va_list args;

    va_start(args, format);
    size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if(size)
    {
        buffer = (char*)malloc(size+1);
        va_start(args, format);
        vsnprintf(buffer, size+1, format, args);
        va_end(args);
    }
    return buffer;
}

/* toggle a menu item between 1 (on) and 0 (off) and turn on/off other menu option based on the toggle */
WC_GLOBAL int change(MenuItems *menuItems, int selectedItem)
{
    int i, value = 1 - atoi(menuItems->items[selectedItem]);

    /* if you know the menu will be changed, it's easier to "clone" the memory */
    /* before calling menu, otherwise this needs to be in every callback */
    if(!menuItems->selfOwnsMemory)
        WC_menu_take_ownership(menuItems);

    free(menuItems->items[selectedItem]);
    menuItems->items[selectedItem] = makeString("%ld", value);
    for(i=selectedItem+1; i<selectedItem+3; i++)
        menuItems->states[i] = value ? WC_ENABLED : WC_DISABLED;
    return WC_INPUT_KEY_DOWN;
}

/* update a variable stored in the menuItens class based on selecting the option */
WC_GLOBAL int increment(MenuItems *menuItems, int selectedItem)
{
    if(!menuItems->selfOwnsMemory)
        WC_menu_take_ownership(menuItems);

    free(menuItems->items[selectedItem]);
    menuItems->items[selectedItem] = makeString("Value: %d", ++((UserData*)menuItems->userData_ptr)->value);
    return 0;
}

/* add more options to the menu */
WC_GLOBAL int append(MenuItems *menuItems, int selectedItem)
{
    int length;

    if(!menuItems->selfOwnsMemory)
        WC_menu_take_ownership(menuItems);

    length = WC_menu_len(menuItems->items);
    menuItems->items = (char**)realloc(menuItems->items,(length+2)*sizeof(char*));
    menuItems->items[length] = makeString("New Item %d",length);
    menuItems->items[length+1] = 0;

    return 0;
}

/* remove extra options from the menu */
WC_GLOBAL int remove(MenuItems *menuItems, int selectedItem)
{
    int length = WC_menu_len(menuItems->items);

    if(!menuItems->selfOwnsMemory)
        WC_menu_take_ownership(menuItems);

    if(length > ((UserData*)menuItems->userData_ptr)->length)
    {
        free(menuItems->items[length-1]);
        menuItems->items = (char**)realloc(menuItems->items,length*sizeof(char*));
        menuItems->items[length-1] = 0;
    }
    return 0;
}

/* debounce windows keys */
WC_GLOBAL int demo_input(void)
{
	static long nInitialKeyDelay = 400000000;
	static long nRepeatKeyDelay  = 120000000;
	static unsigned int nOldRawKeyState = 0;
	static long nDelayTime = nInitialKeyDelay;
	static struct timespec oldTime = {0,0};
	struct timespec nowTime;

	clock_gettime(CLOCK_MONOTONIC, &nowTime);
	long elapsed = WC_menu_elapsedTime(oldTime, nowTime);
	oldTime = nowTime;
	if(nOldRawKeyState == theApp.nRawKeyState)
	{
		if(elapsed > nDelayTime)
			nDelayTime = nRepeatKeyDelay;
		else
		{
			nDelayTime -= elapsed;
			return 0;
		}
	}
	else
	{
		nDelayTime = nInitialKeyDelay;
		nOldRawKeyState = theApp.nRawKeyState;
	}
	return theApp.nRawKeyState;
}

/* windows drawing function to draw menu */
WC_GLOBAL void demo_draw(int y, int x, char *string, int length, int color)
{
	char *str = makeString("%-*.*s", length, length, string);
	HDC hDC = GetDC(theApp.hWnd);
	COLORREF  oldFColor = SetTextColor(hDC, colours[color].fore);
	COLORREF  oldBColor = SetBkColor(hDC, colours[color].back);
	HFONT oldFont = (HFONT)SelectObject(hDC, (HGDIOBJ)theApp.hFont);
	TextOut(hDC, x * theApp.fontWidth, y * theApp.fontWidth, str, strlen(str));
	SelectObject(hDC, oldFont);
	SetBkColor(hDC, oldBColor);
	SetTextColor(hDC, oldFColor);
	SelectObject(hDC, (HGDIOBJ)oldFont);
	ReleaseDC(theApp.hWnd, hDC);
	free(str);
}

/* 
 * force a windows redraw/update.  this is needed to keep the message
 * pump working, otherwise the whole menu system stalls out.  I could
 * fix this with a peekmessage, which I should do in future.
 */
WC_GLOBAL void demo_show(void)
{
	InvalidateRect(theApp.hWnd, 0, false);
}

/* demo (main) program */
void WinApp::Run(void)
{
    MenuItems menuItems;
    UserData userData;
    RECT rect;
    const int colsPerScreen = 80;
    int item;

    char *items[] = 
    {
        "This is a long title - longer than the menu is wide.  Selecting it ends the demo.",
        "This disabled",
        "Value: 10",
        "1",
        "Append Item",
        "Delete Item",
        0
    };
    int states[] = 
    {
        WC_ENABLED,
        WC_DISABLED,
        WC_ENABLED,
        WC_ENABLED,
        WC_ENABLED,
        WC_ENABLED,
        0
    };
    cbf_ptr callbacks[] = {WC_NO_CALLBACK, WC_NO_CALLBACK, increment, change, append, remove, 0};

    /* get the size of the screen */
    GetWindowRect(theApp.hWnd, &rect);

    theApp.fontWidth = (rect.right - rect.left) / colsPerScreen;
	theApp.hFont = CreateFont(theApp.fontWidth, theApp.fontWidth, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Consolas");

    /* create the MenuItems class with some tunable parameters */
    WC_menuInit(&menuItems);

    /* these must be provided */
    menuItems.inputFunction = demo_input;
    menuItems.drawFunction = demo_draw;
    menuItems.sy = (rect.bottom - rect.top) / theApp.fontWidth;
    menuItems.sx = (rect.right - rect.left) / theApp.fontWidth;
    menuItems.items = items;

    /* these are all optional */
    /* comment out anything here to see how it affects the menu */
    menuItems.showFunction = demo_show;

    /*menuItems.y=2; */
    menuItems.x=2;
    menuItems.width=33;
    menuItems.height=12;
    menuItems.title="Hello, World!";
    /* menuItems.title_height = 3; */
    menuItems.states = states;
    menuItems.footer="*** Bye, World! It's been nice knowing you, but now it's time for me to go. ";
    menuItems.footer_height=0;
    menuItems.callbacks = callbacks;
    menuItems.userData_ptr = (void*)&userData;

    /* add the tunable variable to the class */
    userData.value = 10;

    /* add how many items there are to begin with, to the class */
    userData.length = WC_menu_len(menuItems.items);

    /* set a background colour and clear the screen */
   SetClassLongPtr(theApp.hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(0, 0xff, 0xff)));
   InvalidateRect(theApp.hWnd, NULL, true);

    /* show and run the menu */
    item = WC_menu(&menuItems);

    /* clean up the self-owned memory if needed */
    WC_menu_cleanup(&menuItems);

    /* clean up the font */
	DeleteObject(theApp.hFont);
	theApp.hFont = NULL;

	/* the windows app will quit now */
    nExitCode = item;
}
