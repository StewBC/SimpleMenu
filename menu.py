"""
    menu.py is a Windows/Linux/OS X simple curses menu
    system by Stefan Wessels, January 2017.
"""
import curses
import time
import copy

windows = False
try:
    import msvcrt
    windows = True
except ImportError as e:
    import sys
    import select


#menu key handling
INPUT_MOTION    = [curses.KEY_UP, curses.KEY_DOWN]
INPUT_SELECT    = [curses.KEY_ENTER, 10, 13]
INPUT_BACKUP    = 27 # ESC key

# how fast the footer and too long menu items scroll
SCROLL_SPEED    = 0.15

# see defenition of color_pairs later.  Pick color_pairs here for menu items
MENU_CLR_TITLE      = 6
MENU_CLR_ITEMS      = 7
MENU_CLR_FOOTER     = 10
MENU_CLR_SELECT     = 8
MENU_CLR_DISABLED   = 5

"""
these are custom to the application, not for the menu, even though the color indicies above
do refer to these color_pairs.
"""
CR_BLUE_CYAN        = 1
CR_BLACK_CYAN       = 2
CR_WHITE_CYAN       = 3
CR_RED_CYAN         = 4
CR_YELLOW_BLUE      = 5
CR_GREEN_BLUE       = 6
CR_WHITE_BLUE       = 7
CR_WHITE_GREEN      = 8
CR_BLACK_WHITE      = 9
CR_CYAN_BLUE        = 10
CR_BLUE_YELLOW      = 11
CR_RED_BLUE         = 12


# contains all elements to make/draw a menu
class MenuItems:
    def __init__(self, *args, **kwargs):
        self.y = kwargs.get('y', None)
        self.x = kwargs.get('x',None)
        self.width = kwargs.get('width',None) 
        self.height = kwargs.get('height',None) 
        self.title = kwargs.get('title', None)
        self.items = kwargs.get('items', None)
        self.callbacks = kwargs.get('callbacks', None)
        self.states = kwargs.get('states', None)
        self.footer = kwargs.get('footer', None)
        self.header_height = kwargs.get('header_height', 2)
        self.footer_height = kwargs.get('footer_height', 2)
    def __repr__(self):
        return "[y:{} x:{} w:{} h:{}]".format(self.y, self.x, self.width, self.height)

# shows a menu and returns user choice
def menu(menuItems):
    # gets lenth of longest item in array "items"
    def _maxItemLength(items):
        maxItemLen = 0
        for item in items:
            maxItemLen = max(maxItemLen, len(item))
        return maxItemLen

    # finds the next item in "status" that has a 1 from selectedItem in direction (1 or -1)
    # returns -1 or len(menuItems.states) if it runs off the end of the list
    def _next_item(menuItems, selectedItem, direction):
        selectedItem += direction
        # always just next if there are no states
        if not menuItems.states:
            return selectedItem
        numMenuStates = len(menuItems.states)
        while True:
            if selectedItem >= numMenuStates or selectedItem < 0:
                return selectedItem
            if menuItems.states[selectedItem]:
                return selectedItem
            selectedItem += direction

    # you can call menu with an array only
    if not isinstance(menuItems, MenuItems):
        if isinstance(menuItems, list):
            menuItems = MenuItems(items=menuItems)
        else:
            raise Exception("menuItems must be of class MenuItem or a list not {}".format(type(menuItems)))

    # make sure there's enough screen to display at leaset line with the selectors (><) and 1 char
    sy, sx = stdscr.getmaxyx()
    if sy < 1 or sx < 3:
        raise Exception("screen too small")

    # create placeholder y/x locations
    _y = 0 if menuItems.y is None else menuItems.y
    _x = 0 if menuItems.x is None else menuItems.x

    # make sure the top left edge of the menu is on-screen
    if _y < 0 or _y >= sy or _x < 0 or _x > sx-3:
        raise Exception("menu top/left too off-screen")

    # get height of menu
    numMenuItems = len(menuItems.items)
    numMenuHeaders = menuItems.header_height if menuItems.title is not None else 0
    numMenuFooters = menuItems.footer_height if menuItems.footer is not None else 0

    # get length of footer
    footerLength = 0 if not menuItems.footer else len(menuItems.footer)

    # now calc height if not provided
    if menuItems.height is None:
        menuItems.height = numMenuItems + numMenuHeaders + numMenuFooters;
    # make sure height fits on screen
    if _y + menuItems.height > sy - 1:
        menuItems.height = sy - _y - 1

    # calc width if not provided
    if menuItems.width is None:
        titleLength = 0 if menuItems.title is None else len(menuItems.title)
        menuItems.width = max(_maxItemLength(menuItems.items), titleLength)
    # make sure it fits on the screen
    if _x + menuItems.width > sx - 2:
        menuItems.width = sx - _x - 2

    # centre the menu if y or x was not provided
    if menuItems.y is None:
        menuItems.y = max(0,int((sy-menuItems.height)/2))
    if menuItems.x is None:
        menuItems.x = max(0,int((sx-(menuItems.width+2))/2))

    # calculate how many items can be shown
    numVisibleItems = menuItems.height - (numMenuHeaders + numMenuFooters)

    # show 1st item as selected
    selectedItem = _next_item(menuItems, -1, 1)
    if selectedItem > numMenuItems:
        raise Exception("No enabled menu items")
    topItem = 0
    if selectedItem - topItem >= numVisibleItems:
        topItem = selectedItem - numVisibleItems + 1
    # start selected item from 1st character
    itemOffset = 0
    # if selected item has to scroll, scroll it left
    itemDirection = 1
    # start footer from 1st character
    footerOffset = 0

    # if none, then throw exception
    if numVisibleItems < 1:
        raise Exception("menu too short to show any items")

    # init the line (y) variable
    line = menuItems.y

    # show the title if there is one to be shown
    if menuItems.title is not None:
        stdscr.addstr(line, menuItems.x, " {:^{width}} ".format(menuItems.title[:menuItems.width], width=menuItems.width), curses.color_pair(MENU_CLR_TITLE))
        line += 1
        while line - menuItems.y < numMenuHeaders:
            stdscr.addstr(line, menuItems.x, " " * (2 + menuItems.width), curses.color_pair(MENU_CLR_FOOTER))
            line += 1

        # now the menu starts below the title
        menuItems.y = line

    # get time for scrolling purposes
    startTime = time.time()
    # go into the main loop
    while True:
        # get time now to calculate elapsed time
        thisTime = time.time()
        # start at the top to draw
        line = menuItems.y

        # show the visible menu items, highlighting the selected item
        for i in range(topItem, min(numMenuItems,topItem+numVisibleItems)):
            display = " {:{width}} "
            if menuItems.states is None or i >= len(menuItems.states) or menuItems.states[i]:
                color = curses.color_pair(MENU_CLR_ITEMS)
            else:
                color = curses.color_pair(MENU_CLR_DISABLED)
            if i == selectedItem:
                display = ">{:{width}}<"
                color = curses.color_pair(MENU_CLR_SELECT)
                # if the item is longer than the menu width, bounce the item back and forth in the menu display
                if thisTime-startTime > SCROLL_SPEED:
                    displayLength = len(menuItems.items[i])
                    if displayLength > menuItems.width:
                        itemOffset += itemDirection
                        # swap scroll itemDirection but hold for one frame at either end
                        if itemOffset == 0 or itemOffset > displayLength - menuItems.width:
                            if itemDirection:
                                itemDirection = 0
                            elif itemOffset == 0:
                                itemDirection = 1
                            else:
                                itemDirection = -1
            # put ^ or V on the top/bottom lines if there are more options but keep > on selected
            if i == topItem and topItem != 0:
                display = display[:len(display)-1]+"^"
            elif i == topItem+numVisibleItems-1 and i != numMenuItems-1:
                display = display[:len(display)-1]+"v"
            
            # format the line for display
            if i == selectedItem:
                display = display.format(menuItems.items[i][itemOffset:itemOffset+menuItems.width], width=menuItems.width)
            else:
                display = display.format(menuItems.items[i][:menuItems.width], width=menuItems.width)

            # show the item
            stdscr.addstr(line, menuItems.x, display, color)
            line += 1

        # pad out the footer area, if there is one
        while line < menuItems.y + numVisibleItems + numMenuFooters:
            stdscr.addstr(line, menuItems.x, " " * (2 + menuItems.width), curses.color_pair(MENU_CLR_FOOTER))
            line += 1

        # display the footer if there is one
        if menuItems.footer is not None:
            stdscr.addstr(line, menuItems.x, " " + menuItems.footer[footerOffset:footerOffset+menuItems.width], curses.color_pair(MENU_CLR_FOOTER))
            remain = footerLength-footerOffset
            while remain < menuItems.width:
                string = menuItems.footer[:menuItems.width-remain]
                stdscr.addstr(line, menuItems.x+1+remain, string, curses.color_pair(MENU_CLR_FOOTER))
                remain += len(string)
            stdscr.addstr(" ", curses.color_pair(MENU_CLR_FOOTER))

        # make it all visible
        stdscr.refresh()

        # calculate a new scroll position for the footer
        if thisTime-startTime > SCROLL_SPEED:
            footerOffset += 1
            startTime = time.time()
            if footerOffset == footerLength:
                footerOffset = 0

        # test keys and deal with key presses
        keyPressed = 0
        if windows:
            keyPressed = msvcrt.kbhit()
        else:
            dr, dw, de = select.select([sys.stdin], [], [], 0)
            if not dr == []:
                keyPressed = 1

        if keyPressed:
            key = stdscr.getch()
            # this allows callbaks to "press keys"
            while key:
                # cursor key up/down
                if key in INPUT_MOTION:
                    itemOffset = 0
                    itemDirection = 1
                    # cursor up
                    if key == curses.KEY_DOWN:
                        i = _next_item(menuItems, selectedItem, 1)
                        if i >= numMenuItems:
                            i = _next_item(menuItems, -1, 1)
                            if i >= numMenuItems:
                                raise Exception("No enabled menu items")
                            topItem = 0
                        if i - topItem >= numVisibleItems:
                            topItem = i - numVisibleItems + 1
                        selectedItem = i
                    # cursor down
                    if key == curses.KEY_UP:
                        i = _next_item(menuItems, selectedItem, -1)
                        if i < 0:
                            i = _next_item(menuItems, numMenuItems, -1)
                            if i < 0:
                                raise Exception("No enabled menu items")
                            topItem = max(0,numMenuItems - numVisibleItems)
                        if topItem > i:
                            topItem = i
                        selectedItem = i
                    key = None

                # ENTER key
                elif key in INPUT_SELECT:
                    if menuItems.callbacks is not None:
                        # make sure there's a callback and that it's a function
                        if selectedItem < len(menuItems.callbacks) and callable(menuItems.callbacks[selectedItem]):
                            key = menuItems.callbacks[selectedItem](menuItems, selectedItem)
                            numMenuItems = len(menuItems.items)
                    # test again - The callback may have altered the key
                    if key in INPUT_SELECT:
                        return selectedItem

                # BACKUP (normally ESC) breaks out of the menu
                elif key == INPUT_BACKUP:
                    return -1
                # ignore all other keys
                else:
                    break

"""
This code is for demo purposes.  

The callbacks are installed on the menu items to change the behavior or look of
the menu, or to affect a variable that needs "tuning" through the menu
"""

# toggle a menu item between 1 (on) and 0 (off) and turn on/off other menu option based on the toggle
def change(menuItems, selectedItem):
    value = 1-int(menuItems.items[selectedItem])
    menuItems.items[selectedItem] = "{}".format(value)
    for i in range(selectedItem+1, selectedItem+3):
        menuItems.states[i] = value
    return curses.KEY_DOWN

# update a variable stored in the menuItens class based on selecting the option
def increment(menuItems, selectedItem):
    menuItems.value += 1
    menuItems.items[selectedItem] = "Value: {}".format(menuItems.value)
    return None

# add more options to the menu
def append(menuItems, selectedItem):
    menuItems.items.append("New Item {}".format(len(menuItems.items)))
    return None

# remove extra options from the menu
def delete(menuItems, selectedItem):
    if len(menuItems.items) > menuItems.length:
        menuItems.items.pop()
    return None

# sets up the colours for curses, the background colour and clears the screen
def initScr(win):
    global stdscr
    stdscr = win
    if curses.has_colors():
        curses.init_pair(CR_BLUE_CYAN, curses.COLOR_BLUE, curses.COLOR_CYAN)
        curses.init_pair(CR_BLACK_CYAN, curses.COLOR_BLACK, curses.COLOR_CYAN)
        curses.init_pair(CR_WHITE_CYAN, curses.COLOR_WHITE, curses.COLOR_CYAN)
        curses.init_pair(CR_RED_CYAN, curses.COLOR_RED, curses.COLOR_CYAN);
        curses.init_pair(CR_YELLOW_BLUE, curses.COLOR_YELLOW, curses.COLOR_BLUE);
        curses.init_pair(CR_GREEN_BLUE, curses.COLOR_GREEN, curses.COLOR_BLUE);
        curses.init_pair(CR_WHITE_BLUE, curses.COLOR_WHITE, curses.COLOR_BLUE);
        curses.init_pair(CR_WHITE_GREEN, curses.COLOR_WHITE, curses.COLOR_GREEN);
        curses.init_pair(CR_BLACK_WHITE, curses.COLOR_BLACK, curses.COLOR_WHITE);
        curses.init_pair(CR_CYAN_BLUE, curses.COLOR_CYAN, curses.COLOR_BLUE);
        curses.init_pair(CR_BLUE_YELLOW, curses.COLOR_BLUE, curses.COLOR_YELLOW);
        curses.init_pair(CR_RED_BLUE, curses.COLOR_RED, curses.COLOR_BLUE);

# called from the curses.wrapper - main program
def main(win):

    # init
    initScr(win)

    # make a list of callbacks that correspond to the list of menu items
    callbacks = [None, None, increment, change, append, delete]

    # set an initial value that can be tweake in a menu
    value = 10

    # create the MenuItems class with some tunable parameters
    menuItems = MenuItems(
        callbacks=callbacks,
        footer_height=0,
        width=33,
        height=12,
        title="Hello, World",
        x=2,
        items=[
            "This is a long title - longer than the menu is wide.  Selecting it ends the demo", 
            "This disabled",
            "Value: {}".format(value), 
            "1", 
            "Append Item", 
            "Delete Item",
            ],
        states = [1,0,1,1,1,1,1,1],
        footer="*** Bye, World! ")

    # add the tunable variable to the class
    menuItems.value = value
    # add how many items there are to bein with, to the class
    menuItems.length = len(menuItems.items)

    # hide the cursor
    curses.curs_set(0)

    # Show and run the menu
    item = menu(menuItems)

    # enable a cursor and show how the menu was terminated
    curses.curs_set(1)
    stdscr.addstr(0,0,"Item: {} was selected to exit the menu.".format(item))
    stdscr.refresh()
    stdscr.getch()

curses.wrapper(main)
