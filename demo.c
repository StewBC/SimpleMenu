/*
    demo.c uses wcmenu.h to implement a simple curses menu
    system by Stefan Wessels, February 2017.
*/
#include "wcmenu.h"
#include <curses.h>

// colour pairs the application (demo) uses
#define DEMO_BLUE_CYAN          6
#define DEMO_YELLOW_BLUE        WC_CLR_DISABLED
#define DEMO_GREEN_BLUE         WC_CLR_TITLE
#define DEMO_WHITE_BLUE         WC_CLR_ITEMS
#define DEMO_WHITE_GREEN        WC_CLR_SELECT
#define DEMO_CYAN_BLUE          WC_CLR_FOOTER

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
        WC_menu_take_ownership(menuItems);

    free(menuItems->items[selectedItem]);
    menuItems->items[selectedItem] = makeString("%ld", value);
    for(i=selectedItem+1; i<selectedItem+3; i++)
        menuItems->states[i] = value ? WC_ENABLED : WC_DISABLED;
    return WC_INPUT_KEY_DOWN;
}

// update a variable stored in the menuItens class based on selecting the option
int increment(MenuItems *menuItems, int selectedItem)
{
    if(!menuItems->selfOwnsMemory)
        WC_menu_take_ownership(menuItems);

    free(menuItems->items[selectedItem]);
    menuItems->items[selectedItem] = makeString("Value: %d", ++((UserData*)menuItems->userData_ptr)->value);
    return 0;
}

// add more options to the menu
int append(MenuItems *menuItems, int selectedItem)
{
    int length;

    if(!menuItems->selfOwnsMemory)
        WC_menu_take_ownership(menuItems);

    length = WC_menu_len(menuItems->items);
    menuItems->items = realloc(menuItems->items,(length+2)*sizeof(char*));
    menuItems->items[length] = makeString("New Item %d",length);
    menuItems->items[length+1] = 0;

    return 0;
}

// remove extra options from the menu
int delete(MenuItems *menuItems, int selectedItem)
{
    int length = WC_menu_len(menuItems->items);

    if(!menuItems->selfOwnsMemory)
        WC_menu_take_ownership(menuItems);

    if(length > ((UserData*)menuItems->userData_ptr)->length)
    {
        free(menuItems->items[length-1]);
        menuItems->items = realloc(menuItems->items,length*sizeof(char*));
        menuItems->items[length-1] = 0;
    }
    return 0;
}

// map key presses to key defines
int demo_input(void)
{
    switch(getch())
    {
        case 27:
            return WC_INPUT_KEY_ESCAPE;
        case KEY_UP:
            return WC_INPUT_KEY_UP;
        case KEY_DOWN:
            return WC_INPUT_KEY_DOWN;
        case KEY_ENTER:
        case 13:
        case 10:
            return WC_INPUT_KEY_ENTER;
        default:
            return 0;
    }
}

void demo_draw(int y, int x, char *string, int length, int color)
{
    attron(COLOR_PAIR(color));
    mvprintw(y, x, "%-*.*s", length, length, string);
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
        init_pair(DEMO_BLUE_CYAN, COLOR_BLUE, COLOR_CYAN);
        init_pair(DEMO_YELLOW_BLUE, COLOR_YELLOW, COLOR_BLUE);
        init_pair(DEMO_GREEN_BLUE, COLOR_GREEN, COLOR_BLUE);
        init_pair(DEMO_WHITE_BLUE, COLOR_WHITE, COLOR_BLUE);
        init_pair(DEMO_WHITE_GREEN, COLOR_WHITE, COLOR_GREEN);
        init_pair(DEMO_CYAN_BLUE, COLOR_CYAN, COLOR_BLUE);
    }
}

// demo (main) program
int main()
{
    MenuItems menuItems;
    UserData userData;
    int item, sx, sy;

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
    cbf_ptr callbacks[] = {WC_NO_CALLBACK, WC_NO_CALLBACK, increment, change, append, delete, 0};

    // init
    initScr();

    // Get the size of the screen
    getmaxyx(stdscr, sy, sx);

    // create the MenuItems class with some tunable parameters
    WC_menuInit(&menuItems);

    // These must be provided
    menuItems.inputFunction = demo_input;
    menuItems.drawFunction = demo_draw;
    menuItems.sy = sy;
    menuItems.sx = sx;
    menuItems.items = items;

    // These are all optional
    // comment out anything here to see how it affects the menu
    //menuItems.y=2;
    menuItems.x=2;
    menuItems.width=33;
    menuItems.height=12;
    menuItems.title="Hello, World!";
    // menuItems.title_height = 3;
    menuItems.states = states;
    menuItems.footer="*** Bye, World! It's been nice knowing you, but now it's time for me to go. ";
    menuItems.footer_height=0;
    menuItems.callbacks = callbacks;
    menuItems.userData_ptr = (void*)&userData;

    // add the tunable variable to the class
    userData.value = 10;

    // add how many items there are to begin with, to the class
    userData.length = WC_menu_len(menuItems.items);

    // set a background colour and clear the screen
    wbkgd(stdscr, COLOR_PAIR(DEMO_BLUE_CYAN));
    clear();

    // hide the cursor
    curs_set(0);
    // show and run the menu
    item = WC_menu(&menuItems);
    // clean up the self-owned memory if needed
    WC_menu_cleanup(&menuItems);
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
