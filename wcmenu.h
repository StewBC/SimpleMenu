/*
    wcmenu.h is a simple menu system by Stefan Wessels, February 2017.
    It has been tested on Windows, Linux and OS X
*/

#ifndef WCMENU_H_
#define WCMENU_H_

#if defined(__cplusplus) || defined(__cplusplus__) || defined(__CPLUSPLUS)
extern "C" {
#endif

/* menu return values */
#define WC_ERROR_TOO_SMALL    -5  /*the visible area of the menu is too small for even 1 menu-item */
#define WC_ERROR_NONE_ENABLED -4  /*menu contains no "enabled" menu items */
#define WC_ERROR_NOT_ONSCREEN -3  /*top left corner of menu isn't on-screen enough to show a menu */
#define WC_ERROR_WINDOW_SMALL -2  /*the window is too small to show a menu */
#define WC_ERROR_CANCEL       -1  /*ESC key pressed to leave menu */

/* colours to use when draing the menu */
#define WC_CLR_TITLE          1
#define WC_CLR_ITEMS          2
#define WC_CLR_FOOTER         3
#define WC_CLR_SELECT         4
#define WC_CLR_DISABLED       5

/* array sentinels/placeholders */
#define WC_ENABLED           (1)
#define WC_DISABLED          (-1)
#define WC_NONE              (-1)
#define WC_ENDSTATE          (-1)
#define WC_NO_CALLBACK       ((cbf_ptr)-1)
#define WC_TERMINAL          (0)

/* classification */
#define WC_INTERNAL         static
#define WC_CONSTANT         static
#define WC_GLOBAL           static

/* number of nanoseconds in a second */
#define WC_BILLION           (1E9)

/* how fast the footer and too long menu items scroll */
#define WC_SCROLL_SPEED      (WC_BILLION/8)

/*menu key handling */
#define WC_INPUT_KEY_UP      1
#define WC_INPUT_KEY_DOWN    2
#define WC_INPUT_KEY_ENTER   4
#define WC_INPUT_KEY_ESCAPE  8

#define WC_INPUT_MOTION      (WC_INPUT_KEY_UP | WC_INPUT_KEY_DOWN)
#define WC_INPUT_SELECT      WC_INPUT_KEY_ENTER
#define WC_INPUT_BACKUP      WC_INPUT_KEY_ESCAPE

/* define/include timespec struct */
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

#include <windows.h>
/* this does not exist in Windows */
struct timespec { LARGE_INTEGER count; long tv_nsec; };
#define CLOCK_MONOTONIC      0
WC_INTERNAL int clock_gettime(int dummy, struct timespec *ct);

/* for initialization */
WC_GLOBAL BOOL WC_gMenuTimeInit = 1;
WC_GLOBAL LARGE_INTEGER WC_gCountsPerSec;

#else /* !Windows */

#include <time.h>

#endif /* !Windows */

/* other needed includes */
#include <stdlib.h>
#include <string.h>

WC_INTERNAL long WC_menu_elapsedTime(struct timespec start, struct timespec end);

/* prototype for callback function */
struct tagMenuItems;
typedef int (*cbf_ptr)(struct tagMenuItems *, int);

/* prototype for draw function */
typedef void (*WC_menu_draw)(int y, int x, char *string, int length, int color);

/* contains all elements to make/draw a menu */
typedef struct tagMenuItems
{
        /* mandatory */
        int sy;                 /* screen size Y */
        int sx;                 /* screen size X */
        char **items;           /* array of char* (text for) items in menu */
        int (*inputFunction)(void); /* menu calls this to read keyboard */
        WC_menu_draw drawFunction; /* menu calls this for program to render */

        /* optional */
        int y;                  /* where on-screen in Y */
        int x;                  /* where on-screen in X */
        int height;             /* how high */
        int width;              /* how wide */
        char *title;            /* the title text */
        int title_height;       /* pad out the title to # rows */
        char *footer;           /* a scrolling/wrapping footer  */
        int footer_height;      /* pad out the footer to # rows */
        int *states;            /* enabled/disabled items in menu */
        cbf_ptr *callbacks;     /* callbakcs for selecting items */
        void *userData_ptr;     /* pointer to any user defined data */
        void (*showFunction)(void); /* called at the end of each frame */
        
        /* internal */
        int  selfOwnsMemory;    /* 1 = call free on elements; 0 = don't */
} MenuItems;

/* not the right way to do min/max but works for the menu */
#define WC_menu_max(a,b) ((a) > (b) ? (a) : (b))
#define WC_menu_min(a,b) ((a) < (b) ? (a) : (b))
    
/*--------------------------------------------------------------------------*\
  internal functions used by the menu system
\*--------------------------------------------------------------------------*/
/* gets lenth of longest item in array "items" */
WC_INTERNAL int WC_menu_maxItemLength(char **items);
/* counts the number of pointers in a 0 terminated array (of pointer sized elements) */
WC_INTERNAL int WC_menu_len(void *array);
/* counts the number of int's in a null-terminated array (of int-sized elements) */
WC_INTERNAL int WC_menu_count(void *array);
/* finds the next item in "status" that has a 1 from selectedItem in direction (1 or -1) */
/* returns -1 or len(menuItems->states) if it runs off the end of the list */
WC_INTERNAL int WC_menu_next_item(MenuItems *menuItems, int selectedItem, int direction);

/*--------------------------------------------------------------------------*\
  user callable functions
\*--------------------------------------------------------------------------*/

/* 
   inits a MenuItems struct to sane values.  Always call to set up a menu at
   least once.
*/
WC_GLOBAL void WC_menuInit(MenuItems *menuItems);
/* 
   clone all of the elements in a menuItems, in place, so the 
   memory belongs to this menuItems.  Sets selfOwnsMemory to 1.
   only call if a callback will make changes to the menuItems
*/
WC_GLOBAL void WC_menu_take_ownership(MenuItems *menuItems);
/* 
   if selfOwnsMemory is 1, calls free on alloc'd memory, in other
   words, this only needs to be called if WC_menu_take_ownership
   was called, meaning a callback made changes to menuItems 
*/
WC_GLOBAL void WC_menu_cleanup(MenuItems *menuItems);
/* 
   shows a menu and returns user choice or error.  the menu
   item chose, if done, is 0 based from the first option. 
   -1 is cancel and anything else negative is an error as 
   defined/explained by the WC_ERROR defiens
*/
WC_GLOBAL int WC_menu(MenuItems *menuItems);

#if defined(__cplusplus) || defined(__cplusplus__) || defined(__CPLUSPLUS)
}
#endif /*__cplusplus */
#endif /* WCMENU_H_ */

/*--------------------------------------------------------------------------*\
  implementation
\*--------------------------------------------------------------------------*/

#ifndef WC_MENU_IMPLEMENTATION
#define WC_MENU_IMPLEMENTATION

/* define clock_gettime for windows and a platform specific time diff function */
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

/* get a nanosecond time (ignore seconds but eamenuItems->sy to add if needed) */
WC_INTERNAL int clock_gettime(int dummy, struct timespec *ct)
{
    if(WC_gMenuTimeInit)
    {
        WC_gMenuTimeInit = 0;

        if(0 == QueryPerformanceFrequency(&WC_gCountsPerSec))
            WC_gCountsPerSec.QuadPart = 0;
    }

    if((NULL == ct) || (WC_gCountsPerSec.QuadPart <= 0) || (0 == QueryPerformanceCounter(&(ct->count))))
        return -1;

    ct->tv_nsec = ((ct->count.QuadPart % WC_gCountsPerSec.QuadPart) * WC_BILLION) / WC_gCountsPerSec.QuadPart;

    return 0;
}

/* get the delta between two sub-second timers */
WC_INTERNAL long WC_menu_elapsedTime(struct timespec start, struct timespec end)
{
    return (((end.count.QuadPart - start.count.QuadPart) * WC_BILLION) / WC_gCountsPerSec.QuadPart);
}

#else /* !Windows */

/* get the delta between the nanosecond timers (ignoring the seconds component) */
WC_INTERNAL long WC_menu_elapsedTime(struct timespec start, struct timespec end)
{
    long temp;

    if ((end.tv_nsec-start.tv_nsec)<0) 
        temp = WC_BILLION+end.tv_nsec-start.tv_nsec;
    else
        temp = end.tv_nsec-start.tv_nsec;

    return temp;
}

#endif /* !Windows */

/* gets lenth of longest item in array "items" */
WC_INTERNAL int WC_menu_maxItemLength(char **items)
{
    int maxItemLen = 0;
    char **item = items;

    while(*item)
    {
        maxItemLen = WC_menu_max(maxItemLen, strlen(*item));
        item++;
    }
    return maxItemLen;
}

/* counts the number of pointers in a 0 terminated array (of pointer sized elements) */
WC_INTERNAL int WC_menu_len(void *array)
{
    char **entry = (char **)array;
    while(*entry)
        entry++;

    return entry-(char **)array;
}

/* counts the number of int's in a null-terminated array (of int-sized elements) */
WC_INTERNAL int WC_menu_count(void *array)
{
    int *entry = (int *)array;
    while(*entry)
        entry++;

    return entry-(int *)array;
}

/* finds the next item in "status" that has a 1 from selectedItem in direction (1 or -1) */
/* returns -1 or len(menuItems->states) if it runs off the end of the list */
WC_INTERNAL int WC_menu_next_item(MenuItems *menuItems, int selectedItem, int direction)
{
    int numMenuStates;

    selectedItem += direction;

    /* always just next if there are no states */
    if(!menuItems->states)
        return selectedItem;

    numMenuStates = WC_menu_count(menuItems->states);

    while(1)
    {
        if(selectedItem >= numMenuStates || selectedItem < 0)
            return selectedItem;
        if(menuItems->states[selectedItem] == WC_ENABLED)
            return selectedItem;
        selectedItem += direction;
    }
}

/* inits a MenuItems struct to sane values */
WC_GLOBAL void WC_menuInit(MenuItems *menuItems)
{
    menuItems->sy = menuItems->sx = WC_NONE;
    menuItems->inputFunction = 0;
    menuItems->drawFunction = 0;
    menuItems->showFunction = 0;
    menuItems->y = menuItems->x = menuItems->height = menuItems->width = WC_NONE;
    menuItems->title_height = menuItems->footer_height = 2;
    menuItems->title = menuItems->footer = 0;
    menuItems->items = 0;
    menuItems->states = 0;
    menuItems->callbacks = 0;
    menuItems->userData_ptr = 0;
    menuItems->selfOwnsMemory = 0;
}

/* clone all of the elements in a menuItems, in place, so the */
/* memory belongs to this menuItems.  Sets selfOwnsMemory to 1 */
WC_GLOBAL void WC_menu_take_ownership(MenuItems *menuItems)
{
    int i, length;
    void *temp;

    menuItems->title = strdup(menuItems->title);
    menuItems->footer = strdup(menuItems->footer);

    length = WC_menu_len(menuItems->items);
    temp = malloc((length+1)*sizeof(char*));
    for(i=0;i<length;i++)
    {
        ((char**)temp)[i] = strdup(menuItems->items[i]);
    }
    ((char**)temp)[length] = 0;
    menuItems->items = (char**)temp;

    length = WC_menu_count(menuItems->states)+1;
    temp = malloc(length*sizeof(int));
    for(i=0;i<length;i++)
        ((int*)temp)[i] = menuItems->states[i];
    menuItems->states = (int*)temp;

    length = WC_menu_len(menuItems->callbacks)+1;
    temp = malloc(length*sizeof(cbf_ptr));
    for(i=0;i<length;i++)
        ((cbf_ptr*)temp)[i] = menuItems->callbacks[i];
    menuItems->callbacks = (cbf_ptr*)temp;

    menuItems->selfOwnsMemory = 1;
}

/* if selfOwnsMemory is 1, calls free on alloc'd memory */
WC_GLOBAL void WC_menu_cleanup(MenuItems *menuItems)
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

        length = WC_menu_len(menuItems->items);
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

/* shows a menu and returns user choice or error */
WC_GLOBAL int WC_menu(MenuItems *menuItems)
{
    int i,
        _x,
        _y,
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

    /* make sure there are items provided */
    if(!menuItems->items || !*menuItems->items)
        return WC_ERROR_NONE_ENABLED;

    /* make sure there's enough screen to display at least line with the selectors (><) and 1 char */
    if(menuItems->sy < 1 || menuItems->sx < 3)
        return WC_ERROR_WINDOW_SMALL;

    /* create placeholder y/x locations */
    _y = WC_NONE == menuItems->y ? 0 : menuItems->y;
    _x = WC_NONE == menuItems->x ? 0 : menuItems->x;

    /* make sure the top left edge of the menu is on-screen */
    if(_y < 0 || _y >= menuItems->sy || _x < 0 || _x > menuItems->sx-3)
        return WC_ERROR_NOT_ONSCREEN;

    /* get sizes of menu elements */
    numMenuItems = WC_menu_len(menuItems->items);
    numMenuHeaders = menuItems->title ? menuItems->title_height : 0;
    numMenuFooters = menuItems->footer ? menuItems->footer_height : 0;
    numMenuStates = menuItems->states ? WC_menu_count(menuItems->states) : 0;

    /* get length of the title & footer */
    titleLength = menuItems->title ? strlen(menuItems->title) : 0;
    footerLength = menuItems->footer ? strlen(menuItems->footer) : 0;

    /* now calc height if not provided */
    if(WC_NONE == menuItems->height)
        menuItems->height = numMenuItems + numMenuHeaders + numMenuFooters;
    /* make sure height fits on screen */
    if(_y + menuItems->height > menuItems->sy - 1)
        menuItems->height = menuItems->sy - _y - 1;

    /* calc width if not provided */
    if(WC_NONE == menuItems->width)
        menuItems->width = WC_menu_max(WC_menu_maxItemLength(menuItems->items), titleLength);
    /* make sure it fits on the screen */
    if(_x + menuItems->width > menuItems->sx - 2)
        menuItems->width = menuItems->sx - _x - 2;

    /* centre the menu if y or x was not provided */
    if(WC_NONE == menuItems->y)
        menuItems->y = WC_menu_max(0,(int)((menuItems->sy-menuItems->height)/2));
    if(WC_NONE == menuItems->x)
        menuItems->x = WC_menu_max(0,(int)((menuItems->sx-(menuItems->width+2))/2));

    /* calculate how many items can be shown */
    numVisibleItems = menuItems->height - (numMenuHeaders + numMenuFooters);

    /* show 1st enabled item as selected */
    selectedItem = WC_menu_next_item(menuItems, -1, 1);
    if(selectedItem > numMenuItems)
        return WC_ERROR_NONE_ENABLED;
    /* handle 1st selectable item not being on-screen */
    topItem = 0;
    if(selectedItem - topItem >= numVisibleItems)
        topItem = selectedItem - numVisibleItems + 1;
    /* start selected item from 1st character */
    itemOffset = 0;
    /* if selected item has to scroll, scroll it to the left */
    itemDirection = 1;
    /* start footer from 1st character */
    footerOffset = 0;

    /* if no items can be shown then throw exception */
    if(numVisibleItems < 1)
        return WC_ERROR_TOO_SMALL;

    /* init the line (y) variable */
    line = menuItems->y;

    /* get time for scrolling purposes */
    clock_gettime(CLOCK_MONOTONIC,&startTime);
    /* go into the main loop */
    while(1)
    {
#ifdef _WINDOWS
		MSG	msg;
		GetMessage(&msg, (HWND)NULL, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
        if(msg.message == WM_QUIT)
            return -1;
#endif //_WINDOWS
        /* how many items to draw */
        int numItemsToDraw = WC_menu_min(numMenuItems,topItem+numVisibleItems);
        char displayOpen[2] = " ";
        int color;

        /* get time now to calculate elapsed time */
        clock_gettime(CLOCK_MONOTONIC,&thisTime);
        /* start at the top to draw */
        line = menuItems->y;

		/* show the title if there is one to be shown */
		if(menuItems->title)
		{
			int titleLengthClamped = WC_menu_min(titleLength, menuItems->width);
			int length = ((menuItems->width + (titleLengthClamped % 2 ? 0 : 1)) / 2) - (titleLengthClamped / 2) + 1;
			menuItems->drawFunction(line, menuItems->x, " ", length, WC_CLR_TITLE);
			menuItems->drawFunction(line, menuItems->x + length, menuItems->title, titleLengthClamped, WC_CLR_TITLE);
			menuItems->drawFunction(line, menuItems->x + length + titleLengthClamped, " ", 1 + WC_menu_max(0, (menuItems->width / 2) - (titleLengthClamped / 2)), WC_CLR_TITLE);
			line += 1;
			/* pad out the title area */
			while(line - menuItems->y < numMenuHeaders)
			{
				menuItems->drawFunction(line, menuItems->x, " ", menuItems->width + 2, WC_CLR_TITLE);
				line += 1;
			}
		}
		
		/* show the visible menu items, highlighting the selected item */
        for(i=topItem; i<numItemsToDraw; i++)
        {
            char displayLength;

            /* pick the enabled/disabled colour */
            if(menuItems->states && i < numMenuStates && menuItems->states[i] != WC_ENABLED)
                color = WC_CLR_DISABLED;
            else
                color = WC_CLR_ITEMS;

            /* handle the item that's selected */
            if(i == selectedItem)
            {
                *displayOpen = '>';
                color = WC_CLR_SELECT;
                /* if selected item should scroll based on time */
                if(WC_menu_elapsedTime(startTime, thisTime) > WC_SCROLL_SPEED)
                {
                    /* if the item is longer than the menu width, bounce the item back and forth in the menu display */
                    displayLength = strlen(menuItems->items[i]);
                    if(displayLength > menuItems->width)
                    {
                        itemOffset += itemDirection;
                        /* swap scroll itemDirection but hold for one frame at either end */
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
            else
            {
                *displayOpen = ' ';
            }

            /* show 1st character of string */
            menuItems->drawFunction(line, menuItems->x, displayOpen, 1, color);

            if(i == selectedItem)
                menuItems->drawFunction(line, menuItems->x+1, &menuItems->items[i][itemOffset], menuItems->width, color);
            else
                menuItems->drawFunction(line, menuItems->x+1, menuItems->items[i], menuItems->width, color);

            /* put < on selected except the top/bottom when there are more options off-screen which then get ^ or V */
            if(i == topItem && topItem != 0)
                *displayOpen = '^';
            else if(i == topItem+numVisibleItems-1 && i != numMenuItems-1)
                *displayOpen = 'v';
            else if(i == selectedItem)
                *displayOpen = '<';
            else
                *displayOpen = ' ';
            menuItems->drawFunction(line, menuItems->x+1+menuItems->width, displayOpen, 1, color);
            
            line += 1;
        }

        /* pad out the footer area, if there is one */
        while(line < menuItems->y + numVisibleItems + numMenuFooters + numMenuHeaders)
        {
            menuItems->drawFunction(line, menuItems->x, " ", menuItems->width+2, WC_CLR_FOOTER);
            line += 1;
        }

        /* display the footer if there is one */
        if(menuItems->footer)
        {
            int remain = footerLength - footerOffset;
            int column = menuItems->x;
            int length = WC_menu_min(remain, menuItems->width);

            /* open the line and print as much of the footer as is visible */
            menuItems->drawFunction(line, column++, " ", 1, WC_CLR_FOOTER);
            menuItems->drawFunction(line, column, &menuItems->footer[footerOffset], length, WC_CLR_FOOTER);

            /* fill the footer ine with the footer text by wrapping */
            while(remain < menuItems->width)
            {
                column += length;
                length = WC_menu_min(footerLength, menuItems->width-remain);
                menuItems->drawFunction(line, column, menuItems->footer, length, WC_CLR_FOOTER);
                remain += footerLength;
            }
            column += length;
            menuItems->drawFunction(line, column, " ", 1, WC_CLR_FOOTER);
        }

        if(menuItems->showFunction)
            menuItems->showFunction();

        /* calculate a new scroll position for the footer */
        if(WC_menu_elapsedTime(startTime, thisTime) > WC_SCROLL_SPEED)
        {
            footerOffset += 1;
            clock_gettime(CLOCK_MONOTONIC,&startTime);
            if(footerOffset == footerLength)
                footerOffset = 0;
        }

        /* handle keyboard */
        key = menuItems->inputFunction();
        /* this allows callbaks to "press keys" */
        while(key)
        {
            /* cursor key up/down */
            if(key & WC_INPUT_MOTION)
            {
                itemOffset = 0;
                itemDirection = 1;
                /* cursor down */
                if(key & WC_INPUT_KEY_DOWN)
                {
                    int i = WC_menu_next_item(menuItems, selectedItem, 1);
                    if(i >= numMenuItems)
                    {
                        i = WC_menu_next_item(menuItems, -1, 1);
                        if(i >= numMenuItems)
                            return WC_ERROR_NONE_ENABLED;
                        topItem = 0;
                    }
                    /* make sure newly selected item is visible */
                    if(i - topItem >= numVisibleItems)
                        topItem = i - numVisibleItems + 1;
                    selectedItem = i;
                }
                /* cursor up */
                if(key & WC_INPUT_KEY_UP)
                {
                    i = WC_menu_next_item(menuItems, selectedItem, -1);
                    if(i < 0)
                    {
                        i = WC_menu_next_item(menuItems, numMenuItems, -1);
                        if(i < 0)
                            return WC_ERROR_NONE_ENABLED;
                        topItem = WC_menu_max(0,numMenuItems - numVisibleItems);
                    }
                    if(topItem > i)
                        topItem = i;
                    selectedItem = i;
                }
                key = 0;
            }
            /* ENTER key */
            else if(key & WC_INPUT_SELECT)
            {
                if(menuItems->callbacks)
                {
                    /* see if there's a callback and that it's a function */
                    if(selectedItem < WC_menu_len(menuItems->callbacks) && WC_NO_CALLBACK != menuItems->callbacks[selectedItem])
                    {
                        /* the callbak return value should be 0 or a key-define */
                        key = menuItems->callbacks[selectedItem](menuItems, selectedItem);
                        /* re-check how many items in the menu as a callback can add/delete items */
                        numMenuItems = WC_menu_len(menuItems->items);
                        if(!numMenuItems)
                            return WC_ERROR_NONE_ENABLED;
                        numMenuStates = menuItems->states ? WC_menu_count(menuItems->states) : 0;
                    }
                }
                /* test again - The callback may have altered the key, but if not then done */
                if(key & WC_INPUT_SELECT)
                    return selectedItem;
            }
            /* ESC key pressed exits with -1 */
            else if(key & WC_INPUT_BACKUP)
                return WC_ERROR_CANCEL;
            else
                break; /* ignore other key-values */
        }
    }
}

#endif /* WC_MENU_IMPLEMENTATION */
