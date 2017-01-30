/*
	menu.c is a Windows/Linux/OS X simple curses menu
	system by Stefan Wessels, January 2017.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

// menu return values
#define MENU_ERROR_TOO_SMALL    -5  //the visible area of the menu is too small for even 1 menu-item
#define MENU_ERROR_NONE_ENABLED -4  //menu contains no "enabled" menu items
#define MENU_ERROR_NOT_ONSCREEN -3  //top left corner of menu isn't on-screen enough to show a menu
#define MENU_ERROR_WINDOW_SMALL -2  //the window is too small to show a menu
#define MENU_ERROR_CANCEL       -1  //ESC key pressed to leave menu

// array sentinels/placeholders
#define _MENU_ENABLED			(1)
#define _MENU_DISABLED			(-1)
#define _MENU_NONE				(-1)
#define _MENU_ENDSTATE			(-1)
#define _MENU_NO_CALLBACK		((cbf_ptr)-1)
#define _MENU_TERMINAL			(0)

// how fast the footer and too long menu items scroll
#define _MENU_SCROLL_SPEED    	(_MENU_BILLION/8)

//menu key handling
#define _MENU_INPUT_KEY_UP		1
#define _MENU_INPUT_KEY_DOWN	2
#define _MENU_INPUT_KEY_ENTER	4
#define _MENU_INPUT_KEY_ESCAPE	8

#define _MENU_INPUT_MOTION    	(_MENU_INPUT_KEY_UP | _MENU_INPUT_KEY_DOWN)
#define _MENU_INPUT_SELECT    	_MENU_INPUT_KEY_ENTER
#define _MENU_INPUT_BACKUP    	_MENU_INPUT_KEY_ESCAPE

/* 
	thes MENU_ defines are mandatory but the 6, 7, 10... are for demo purposes
	and will need to be set to whatever the application defines for color_pairs.
	see defenition of init_pair later.  Pick COLOR_PAIRs here for menu items
*/
#define MENU_CLR_TITLE      6
#define MENU_CLR_ITEMS      7
#define MENU_CLR_FOOTER     10
#define MENU_CLR_SELECT     8
#define MENU_CLR_DISABLED   5

// number of nanoseconds in a second
#define _MENU_BILLION             (1E9)

// define clock_gettime for windows and a platform specific time diff function
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

#include <windows.h>

// this does not exist in Windows
struct timespec { LARGE_INTEGER count; long tv_nsec; };
#define CLOCK_MONOTONIC		0

// for initialization
static BOOL gMenu_time_init = 1;
static LARGE_INTEGER gCountsPerSec;

// get a nanosecond time (ignore seconds but easy to add if needed)
int clock_gettime(int dummy, struct timespec *ct)
{
    if(gMenu_time_init)
    {
        gMenu_time_init = 0;

        if(0 == QueryPerformanceFrequency(&gCountsPerSec))
            gCountsPerSec.QuadPart = 0;
    }

    if((NULL == ct) || (gCountsPerSec.QuadPart <= 0) || (0 == QueryPerformanceCounter(&(ct->count))))
        return -1;

    ct->tv_nsec = ((ct->count.QuadPart % gCountsPerSec.QuadPart) * _MENU_BILLION) / gCountsPerSec.QuadPart;

    return 0;
}

// get the delta between two sub-second timers
long _menu_elapsedTime(struct timespec start, struct timespec end)
{
	return (((end.count.QuadPart - start.count.QuadPart) * _MENU_BILLION) / gCountsPerSec.QuadPart);
}

// Linus / OS X
#else

#include <time.h>

// get the delta between the nanosecond timers (ignoring the seconds component)
long _menu_elapsedTime(struct timespec start, struct timespec end)
{
	long temp;

	if ((end.tv_nsec-start.tv_nsec)<0) 
		temp = _MENU_BILLION+end.tv_nsec-start.tv_nsec;
	else
		temp = end.tv_nsec-start.tv_nsec;

	return temp;
}

#endif

// prototype for callback function
struct tagMenuItems;
typedef int (*cbf_ptr)(struct tagMenuItems *, int);

// contains all elements to make/draw a menu
typedef struct tagMenuItems
{
        int y;
        int x;
        int width;
        int height;
        int header_height;
        int footer_height;
        char *title;
        char *footer;
        char **items;
        int *states;
        cbf_ptr *callbacks;
        int  selfOwnsMemory;
        void *userData_ptr;
} MenuItems;

// not the right way to do min/max but works for the menu
#define _menu_max(a,b) (a) > (b) ? (a) : (b)
#define _menu_min(a,b) (a) < (b) ? (a) : (b)
    
// _menu_* functions are helpers

// gets lenth of longest item in array "items"
int _menu_maxItemLength(char **items)
{
    int maxItemLen = 0;
    char **item = items;

    while(*item)
    {
        maxItemLen = _menu_max(maxItemLen, strlen(*item));
        item++;
    }
    return maxItemLen;
}

// counts the number of pointers in a 0 terminated array (of pointer sized elements)
int _menu_len(void *array)
{
	char **entry = (char **)array;
    while(*entry)
        entry++;

    return entry-(char **)array;
}

// counts the number of int's in a null-terminated array (of int-sized elements)
int _menu_count(void *array)
{
	int *entry = (int *)array;
    while(*entry)
        entry++;

    return entry-(int *)array;
}

// clone all of the elements in a menuItems, in place, so the
// memory belongs to this menuItems.  Sets selfOwnsMemory to 1
void _menu_take_ownership(MenuItems *menuItems)
{
	int i, length;
	void *temp;

	menuItems->title = strdup(menuItems->title);
	menuItems->footer = strdup(menuItems->footer);

	length = _menu_len(menuItems->items);
	temp = malloc((length+1)*sizeof(char*));
	for(i=0;i<length;i++)
	{
		((char**)temp)[i] = strdup(menuItems->items[i]);
	}
	((char**)temp)[length] = 0;
	menuItems->items = (char**)temp;

	length = _menu_count(menuItems->states)+1;
	temp = malloc(length*sizeof(int));
	for(i=0;i<length;i++)
		((int*)temp)[i] = menuItems->states[i];
	menuItems->states = (int*)temp;

	length = _menu_len(menuItems->callbacks)+1;
	temp = malloc(length*sizeof(cbf_ptr));
	for(i=0;i<length;i++)
		((cbf_ptr*)temp)[i] = menuItems->callbacks[i];
	menuItems->callbacks = (cbf_ptr*)temp;

	menuItems->selfOwnsMemory = 1;
}

// finds the next item in "status" that has a 1 from selectedItem in direction (1 or -1)
// returns -1 or len(menuItems->states) if it runs off the end of the list
int _menu_next_item(MenuItems *menuItems, int selectedItem, int direction)
{
	int numMenuStates;

    selectedItem += direction;

    // always just next if there are no states
    if(!menuItems->states)
        return selectedItem;

    numMenuStates = _menu_count(menuItems->states);

    while(1)
    {
        if(selectedItem >= numMenuStates || selectedItem < 0)
            return selectedItem;
        if(menuItems->states[selectedItem] == _MENU_ENABLED)
            return selectedItem;
        selectedItem += direction;
    }
}

// map key presses to key defines
int _menu_input(int key)
{
	switch(key)
	{
		case 27:
			return _MENU_INPUT_KEY_ESCAPE;
		case KEY_UP:
			return _MENU_INPUT_KEY_UP;
		case KEY_DOWN:
			return _MENU_INPUT_KEY_DOWN;
		case KEY_ENTER:
		case 13:
		case 10:
		case KEY_BACKSPACE:
			return _MENU_INPUT_KEY_ENTER;
		default:
			return 0;
	}
}

// inits a MenuItems struct to sane values
void menuInit(MenuItems *menuItems)
{
	menuItems->x = menuItems->y = menuItems->width = menuItems->height = _MENU_NONE;
	menuItems->header_height = menuItems->footer_height = 2;
	menuItems->title = menuItems->footer = 0;
	menuItems->items = 0;
	menuItems->states = 0;
	menuItems->callbacks = 0;
	menuItems->userData_ptr = 0;
	menuItems->selfOwnsMemory = 0;
}

// if selfOwnsMemory is 1, calls free on alloc'd memory
void menu_cleanup(MenuItems *menuItems)
{
	if(menuItems->selfOwnsMemory)
	{
		int i, length;
		if(menuItems->title)
		{
			free(menuItems->title);
			menuItems->title = 0;
		}
		if(menuItems->footer)
		{
			free(menuItems->footer);
			menuItems->footer = 0;
		}

		length = _menu_len(menuItems->items);
		for(i=0;i<length;i++)
		{
			free(menuItems->items[i]);
		}
		if(menuItems->items)
		{
			free(menuItems->items);
			menuItems->items = 0;
		}

		if(menuItems->states)
		{
			free(menuItems->states);
			menuItems->states = 0;
		}

		if(menuItems->callbacks)
		{
			free(menuItems->callbacks);
			menuItems->callbacks = 0;
		}
		menuItems->selfOwnsMemory = 0;
	}
}

/* 
shows a menu and returns user choice or error
    return values are:
	-5  the visible area of the menu is too small for even 1 menu-item
	-4  menu contains no "enabled" menu items
	-3  top left corner of menu isn't on-screen enough to show a menu
	-2  the window is too small to show a menu
	-1  ESC key pressed to leave menu
	0   1 st menu item selected
	1   2 nd menu item selected
	... etc.
*/
int menu(MenuItems *menuItems)
{
	int i,
		_x,
		_y,
		sx,
		sy,
		numMenuItems,
		numMenuHeaders,
		numMenuFooters,
		numMenuStates,
		numVisibleItems,
		titleLength,
		footerLength,
		selectedItem,
		topItem,
		itemOffset,
		itemDirection,
		footerOffset,
		line,
		key;
    struct timespec startTime, thisTime;

    // make sure there are items provided
    if(!menuItems->items || !*menuItems->items)
    	return MENU_ERROR_NONE_ENABLED;

    // make sure there's enough screen to display at least line with the selectors (><) and 1 char
    getmaxyx(stdscr, sy, sx);
    if(sy < 1 || sx < 3)
        return MENU_ERROR_WINDOW_SMALL;

    // create placeholder y/x locations
   	_y = _MENU_NONE == menuItems->y ? 0 : menuItems->y;
   	_x = _MENU_NONE == menuItems->x ? 0 : menuItems->x;

    // make sure the top left edge of the menu is on-screen
    if(_y < 0 || _y >= sy || _x < 0 || _x > sx-3)
        return MENU_ERROR_NOT_ONSCREEN;

    // get sizes of menu elements
    numMenuItems = _menu_len(menuItems->items);
    numMenuHeaders = menuItems->title ? menuItems->header_height : 0;
    numMenuFooters = menuItems->footer ? menuItems->footer_height : 0;
    numMenuStates = menuItems->states ? _menu_count(menuItems->states) : 0;

    // get length of the header & footer
	titleLength = menuItems->title ? strlen(menuItems->title) : 0;
    footerLength = menuItems->footer ? strlen(menuItems->footer) : 0;

    // now calc height if not provided
    if(_MENU_NONE == menuItems->height)
        menuItems->height = numMenuItems + numMenuHeaders + numMenuFooters;
    // make sure height fits on screen
    if(_y + menuItems->height > sy - 1)
        menuItems->height = sy - _y - 1;

    // calc width if not provided
    if(_MENU_NONE == menuItems->width)
        menuItems->width = _menu_max(_menu_maxItemLength(menuItems->items), titleLength);
    // make sure it fits on the screen
    if(_x + menuItems->width > sx - 2)
        menuItems->width = sx - _x - 2;

    // centre the menu if y or x was not provided
    if(_MENU_NONE == menuItems->y)
        menuItems->y = _menu_max(0,(int)((sy-menuItems->height)/2));
    if(_MENU_NONE == menuItems->x)
        menuItems->x = _menu_max(0,(int)((sx-(menuItems->width+2))/2));

    // calculate how many items can be shown
    numVisibleItems = menuItems->height - (numMenuHeaders + numMenuFooters);

    // show 1st enabled item as selected
    selectedItem = _menu_next_item(menuItems, -1, 1);
    if(selectedItem > numMenuItems)
        return MENU_ERROR_NONE_ENABLED;
    // handle 1st selectable item not being on-screen
    topItem = 0;
    if(selectedItem - topItem >= numVisibleItems)
        topItem = selectedItem - numVisibleItems + 1;
    // start selected item from 1st character
    itemOffset = 0;
    // if selected item has to scroll, scroll it to the left
    itemDirection = 1;
    // start footer from 1st character
    footerOffset = 0;

    // if no items can be shown then throw exception
    if(numVisibleItems < 1)
        return MENU_ERROR_TOO_SMALL;

    // init the line (y) variable
    line = menuItems->y;

    // show the title if there is one to be shown
    if(menuItems->title)
    {
    	attron(COLOR_PAIR(MENU_CLR_TITLE));
        mvprintw(line, menuItems->x, " %*.s%.*s%*.s ", _menu_max(0,((menuItems->width+(titleLength % 2 ? 0 : 1))/2)-(titleLength/2)), "", menuItems->width, menuItems->title, _menu_max(0,(menuItems->width/2)-(titleLength/2)), "");
        line += 1;
        // pad out the header area
        while(line - menuItems->y < numMenuHeaders)
        {
            mvprintw(line, menuItems->x, " %*.s ", menuItems->width, "");
            line += 1;
        }

        // now the menu starts below the title
        menuItems->y = line;
    }

    // get time for scrolling purposes
    clock_gettime(CLOCK_MONOTONIC,&startTime);
    // go into the main loop
    while(1)
    {
    	// how many items to draw
    	int numItemsToDraw = _menu_min(numMenuItems,topItem+numVisibleItems);
        // get time now to calculate elapsed time
        clock_gettime(CLOCK_MONOTONIC,&thisTime);
        // start at the top to draw
        line = menuItems->y;

        // show the visible menu items, highlighting the selected item
        for(i=topItem; i<numItemsToDraw; i++)
        {
            char displayLength, displayOpen = ' ';

            // pick the enabled/disabled colour
            if(!menuItems->states || i >= numMenuStates || menuItems->states[i] == _MENU_ENABLED)
                attron(COLOR_PAIR(MENU_CLR_ITEMS));
            else
                attron(COLOR_PAIR(MENU_CLR_DISABLED));
            // handle the item that's selected
            if(i == selectedItem)
            {
                displayOpen = '>';
                attron(COLOR_PAIR(MENU_CLR_SELECT));
                // if selected item should scroll based on time
        		if(_menu_elapsedTime(startTime, thisTime) > _MENU_SCROLL_SPEED)
                {
	                // if the item is longer than the menu width, bounce the item back and forth in the menu display
                    displayLength = strlen(menuItems->items[i]);
                    if(displayLength > menuItems->width)
                    {
                        itemOffset += itemDirection;
                        // swap scroll itemDirection but hold for one frame at either end
                        if(itemOffset == 0 || itemOffset > displayLength - menuItems->width)
                        {
                            if(itemDirection)
                                itemDirection = 0;
                            else if(itemOffset == 0)
                                itemDirection = 1;
                            else
                                itemDirection = -1;
                        }
                    }
                }
            }

            // show 1st character of string
			mvaddch(line, menuItems->x, displayOpen);

            // display the item
            if(i == selectedItem)
            	printw("%-*.*s", menuItems->width, menuItems->width, &menuItems->items[i][itemOffset]);
            else
              	printw("%-*.*s", menuItems->width, menuItems->width, menuItems->items[i]);

            // put < on selected except the top/bottom when there are more options off-screen which then get ^ or V
            if(i == topItem && topItem != 0)
                addch('^');
            else if(i == topItem+numVisibleItems-1 && i != numMenuItems-1)
                addch('v');
            else if(i == selectedItem)
            	addch('<');
            else
            	addch(' ');
            
            line += 1;
        }

        // set colour for footer area
       	attron(COLOR_PAIR((MENU_CLR_FOOTER)));
        // pad out the footer area, if there is one
        while(line < menuItems->y + numVisibleItems + numMenuFooters)
        {
            mvprintw(line, menuItems->x, " %*.s ", menuItems->width, "");
            line += 1;
        }

        // display the footer if there is one
        if(menuItems->footer)
        {
            int remain = footerLength - footerOffset;
            mvprintw(line, menuItems->x, " %.*s", menuItems->width, &menuItems->footer[footerOffset]);
            // fill the footer ine with the footer text by wrapping
            while(remain < menuItems->width)
            {
                mvprintw(line, menuItems->x+remain, " %.*s", menuItems->width-remain, menuItems->footer);
                remain += footerLength;
            }
            printw(" ");
        }

        // make it all visible
       refresh();

        // calculate a new scroll position for the footer
        if(_menu_elapsedTime(startTime, thisTime) > _MENU_SCROLL_SPEED)
        {
            footerOffset += 1;
            clock_gettime(CLOCK_MONOTONIC,&startTime);
            if(footerOffset == footerLength)
                footerOffset = 0;
        }

        // handle keyboard
        key = _menu_input(getch());
        // this allows callbaks to "press keys"
        while(key)
        {
            // cursor key up/down
            if(key & _MENU_INPUT_MOTION)
            {
                itemOffset = 0;
                itemDirection = 1;
                // cursor down
                if(key & _MENU_INPUT_KEY_DOWN)
                {
                    int i = _menu_next_item(menuItems, selectedItem, 1);
                    if(i >= numMenuItems)
                    {
                        i = _menu_next_item(menuItems, -1, 1);
                        if(i >= numMenuItems)
                            return MENU_ERROR_NONE_ENABLED;
                        topItem = 0;
                    }
                    // make sure newly selected item is visible
                    if(i - topItem >= numVisibleItems)
                        topItem = i - numVisibleItems + 1;
                    selectedItem = i;
                }
                // cursor up
                if(key & _MENU_INPUT_KEY_UP)
                {
                    i = _menu_next_item(menuItems, selectedItem, -1);
                    if(i < 0)
                    {
                        i = _menu_next_item(menuItems, numMenuItems, -1);
                        if(i < 0)
                            return MENU_ERROR_NONE_ENABLED;
                        topItem = _menu_max(0,numMenuItems - numVisibleItems);
                    }
                    if(topItem > i)
                        topItem = i;
                    selectedItem = i;
                }
                key = 0;
            }
            // ENTER key
            else if(key & _MENU_INPUT_SELECT)
            {
                if(menuItems->callbacks)
                {
                    // see if there's a callback and that it's a function
                    if(selectedItem < _menu_len(menuItems->callbacks) && _MENU_NO_CALLBACK != menuItems->callbacks[selectedItem])
                    {
                    	// the callbak return value should be 0 or a key-define
                        key = menuItems->callbacks[selectedItem](menuItems, selectedItem);
                        // re-check how many items in the menu as a callback can add/delete items
                        numMenuItems = _menu_len(menuItems->items);
                        if(!numMenuItems)
                        	return MENU_ERROR_NONE_ENABLED;
                        numMenuStates = menuItems->states ? _menu_count(menuItems->states) : 0;
                    }
                }
                // test again - The callback may have altered the key, but if not then done
                if(key & _MENU_INPUT_SELECT)
                    return selectedItem;
            }
            // ESC key pressed exits with -1
            else if(key & _MENU_INPUT_BACKUP)
                return MENU_ERROR_CANCEL;
            else
                break; // ignore other key-values
        }
    }
}

/*
The code below is for demo purposes.  

The callbacks are installed on the menu items to change the behavior or look of
the menu, or to affect a variable that needs "tuning" through the menu
*/

/*
these are custom to the application, not for the menu, even though the color indicies above
do refer to these COLOR_PAIRs.
*/
#define CR_BLUE_CYAN        1
#define CR_BLACK_CYAN       2
#define CR_WHITE_CYAN       3
#define CR_RED_CYAN         4
#define CR_YELLOW_BLUE      5
#define CR_GREEN_BLUE       6
#define CR_WHITE_BLUE       7
#define CR_WHITE_GREEN      8
#define CR_BLACK_WHITE      9
#define CR_CYAN_BLUE        10
#define CR_BLUE_YELLOW      11
#define CR_RED_BLUE         12

// app specific user structure pointed to by userData_ptr
typedef struct tagUserData
{
	int value;
	int length;
} UserData;

// make a buffer and use printf style formatting to put a string in the buffer
char *makeString(char *format, ... )
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

// toggle a menu item between 1 (on) and 0 (off) and turn on/off other menu option based on the toggle
int change(MenuItems *menuItems, int selectedItem)
{
	int i, value = 1 - atoi(menuItems->items[selectedItem]);

	// if you know the menu will be changed, it's easier to "clone" the memory
	// before calling menu, otherwise this needs to be in every callback
	if(!menuItems->selfOwnsMemory)
		_menu_take_ownership(menuItems);

	free(menuItems->items[selectedItem]);
    menuItems->items[selectedItem] = makeString("%ld", value);
    for(i=selectedItem+1; i<selectedItem+3; i++)
        menuItems->states[i] = value ? _MENU_ENABLED : _MENU_DISABLED;
    return _MENU_INPUT_KEY_DOWN;
}

// update a variable stored in the menuItens class based on selecting the option
int increment(MenuItems *menuItems, int selectedItem)
{
	if(!menuItems->selfOwnsMemory)
		_menu_take_ownership(menuItems);

    free(menuItems->items[selectedItem]);
    menuItems->items[selectedItem] = makeString("Value: %d", ++((UserData*)menuItems->userData_ptr)->value);
    return 0;
}

// add more options to the menu
int append(MenuItems *menuItems, int selectedItem)
{
	int length;

	if(!menuItems->selfOwnsMemory)
		_menu_take_ownership(menuItems);

	length = _menu_len(menuItems->items);
	menuItems->items = realloc(menuItems->items,(length+2)*sizeof(char*));
	menuItems->items[length] = makeString("New Item %d",length);
	menuItems->items[length+1] = 0;

    return 0;
}

// remove extra options from the menu
int delete(MenuItems *menuItems, int selectedItem)
{
	int length = _menu_len(menuItems->items);

	if(!menuItems->selfOwnsMemory)
		_menu_take_ownership(menuItems);

    if(length > ((UserData*)menuItems->userData_ptr)->length)
    {
    	free(menuItems->items[length-1]);
        menuItems->items = realloc(menuItems->items,length*sizeof(char*));
        menuItems->items[length-1] = 0;
    }
    return 0;
}

// sets up the colours for curses, the background colour and clears the screen
void initScr(void)
{
	initscr();
    keypad(stdscr, TRUE);
	nonl();
	cbreak();
	noecho();
	timeout(0);

    if(has_colors())
    {
        start_color();
        init_pair(CR_BLUE_CYAN, COLOR_BLUE, COLOR_CYAN);
        init_pair(CR_BLACK_CYAN, COLOR_BLACK, COLOR_CYAN);
        init_pair(CR_WHITE_CYAN, COLOR_WHITE, COLOR_CYAN);
        init_pair(CR_RED_CYAN, COLOR_RED, COLOR_CYAN);
        init_pair(CR_YELLOW_BLUE, COLOR_YELLOW, COLOR_BLUE);
        init_pair(CR_GREEN_BLUE, COLOR_GREEN, COLOR_BLUE);
        init_pair(CR_WHITE_BLUE, COLOR_WHITE, COLOR_BLUE);
        init_pair(CR_WHITE_GREEN, COLOR_WHITE, COLOR_GREEN);
        init_pair(CR_BLACK_WHITE, COLOR_BLACK, COLOR_WHITE);
        init_pair(CR_CYAN_BLUE, COLOR_CYAN, COLOR_BLUE);
        init_pair(CR_BLUE_YELLOW, COLOR_BLUE, COLOR_YELLOW);
        init_pair(CR_RED_BLUE, COLOR_RED, COLOR_BLUE);
    }
}

// called from the curses.wrapper - main program
int main()
{
    MenuItems menuItems;
    UserData userData;
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
    	_MENU_ENABLED,
    	_MENU_DISABLED,
    	_MENU_ENABLED,
    	_MENU_ENABLED,
    	_MENU_ENABLED,
    	_MENU_ENABLED,
    	0
    };
    cbf_ptr callbacks[] = {_MENU_NO_CALLBACK, _MENU_NO_CALLBACK, increment, change, append, delete, 0};

    // init
    initScr();

    // create the MenuItems class with some tunable parameters
    // comment out anything here to see how it affects the menu
    menuInit(&menuItems);
    menuItems.footer_height=0;
    menuItems.width=33;
    menuItems.height=12;
    menuItems.title="Hello, World!";
    menuItems.x=2;
    menuItems.items = items;
    menuItems.states = states;
    menuItems.footer="*** Bye, World! It's been nice knowing you, but now it's time for me to go. ";
    menuItems.callbacks = callbacks;
    menuItems.userData_ptr = (void*)&userData;

    // add the tunable variable to the class
    userData.value = 10;

    // add how many items there are to bein with, to the class
    userData.length = _menu_len(menuItems.items);

    // set a background colour and clear the screen
    wbkgd(stdscr, COLOR_PAIR(1));
    erase();

    // hide the cursor
    curs_set(0);
    // show and run the menu
    item = menu(&menuItems);
    // clean up the self-owned memory if needed
    menu_cleanup(&menuItems);
    // show the cursor
    curs_set(1);

    //enable a cursor and show how the menu was terminated
    mvprintw(0,0,"Item: %d was selected to exit the menu.", item);
    refresh();
    // blocking input on
    timeout(-1) ;
    getch();

    // shut it all down
	endwin() ;

    return 0;
}
