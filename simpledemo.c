/*
    simpledemo.c uses wcmenu.h to implement a simple curses menu
    by Stefan Wessels, February 2017.
*/
#include "wcmenu.h"
#include <curses.h>

/* colour pairs the application (demo) uses */
#define DEMO_BLUE_CYAN          6               /* 1 - 5 are WC_CLR defines */
#define DEMO_YELLOW_BLUE        WC_CLR_DISABLED /* use menu colours directly */
#define DEMO_GREEN_BLUE         WC_CLR_TITLE    /* for drawing menu elements */
#define DEMO_WHITE_BLUE         WC_CLR_ITEMS
#define DEMO_WHITE_GREEN        WC_CLR_SELECT
#define DEMO_CYAN_BLUE          WC_CLR_FOOTER


/* map key presses from curses to wc_input defines */
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
        case 10:
        case 13:
            return WC_INPUT_KEY_ENTER;
        default:
            return 0;
    }
}

/* drawing function using curses */
void demo_draw(int y, int x, char *string, int length, int color)
{
    attron(COLOR_PAIR(color));
    mvprintw(y, x, "%-*.*s", length, length, string);
}

/* sets up the colours for curses, the background colour and clears the screen */
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

/* demo (main) program */
int main()
{
    MenuItems menuItems;
    int item, sx, sy;

    char *items[] = 
    {
        "A simple menu.",
        "Make a choice",
        "When you press ENTER",
        "That option # is returned",
        0
    };

    /* init */
    initScr();

    /* Get the size of the screen */
    getmaxyx(stdscr, sy, sx);

    /* create the MenuItems class with some tunable parameters */
    WC_menuInit(&menuItems);

    /* These must be provided */
    menuItems.inputFunction = demo_input;
    menuItems.drawFunction = demo_draw;
    menuItems.sy = sy;
    menuItems.sx = sx;
    menuItems.items = items;

    /* set a background colour and clear the screen */
    wbkgd(stdscr, COLOR_PAIR(DEMO_BLUE_CYAN));
    clear();

    /* show and run the menu */
    item = WC_menu(&menuItems);

    /* shut it all down */
    endwin() ;

    /* report which item (line) was chosen */
    printf("Item %d was selected\n", item);

    return 0;
}
