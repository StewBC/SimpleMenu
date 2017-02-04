Simple Menu

These files contain a menu "function" for/in both C and Python

The Python version uses curses and works on Windows, Linux and OS X.

The C version is more or less a port of the Python version, but it comes as
an include (.h) file and is not curses based.  It's up to the application that
includes the header to handle input and drawing.  

The demo.c and simpledemo.c programs illustrate how to use the menu system from
wcmenu.h, using curses.

For Windows, I used PDCurses34 and after unzipping, I put demo.c and 
simpledemo.cin the win32 folder.

The user can set the folling elements for the menu, all of which are optional,
except of the "items" element, in both the C and Python versions:
  y             - Row height of upper left.  If omitted, center on screen in y
  x             - Column of upper left.  If omitted, center on screen in x
  width         - Width of menu screen. If omitted, longest item length
  height        - Height of menu screen. If omitted, items+head+foot+pad
  title         - Title to be shown centered. If omitted, no header+header pad
  items         - List/array of menu items user can choose from
  callbacks     - List/array of callback functions for each item
  states        - List of enabled/disabled states for each item
  footer        - Text to scroll/wrap at the bottom of the menu
  title_height  - Padding lines between header and items. default = 2
  footer_height - Padding lines between items and footer. default = 2

For the C version, there are also these fields: 
compulsory to fill in:
  inputFunction - function pointer pointing at an input handler
  drawFunction  - function pointer pointing at a render function
  sy            - height of the screen
  sx            - width of the screen
optional
  showFunction	- function pointer pointing at a "show" function.  See below

The showFunction in the C version is there to "present" the draw calls to the 
user.  With curses, this is a good time to call refresh().  With a back-buffer
this would be a good time to flip the buffer to the front.

The C version still uses character based coordinates so using it with a GUI
is harder and will best work with a non-proportional (fixed width) font where
a simple mapping from character cells to GUI coordinates is straight-forward.

In both the C and Python version user data can also be attached and viewed/
edited in callbacks.

Through the use of callbacks, items can be altered, added, deleted, enabled and
disabled.  The system is quite powerful but aims to be simple.

The Python version is very straight-forward.  The C version is more complicated
mostly because of the added complexity of memory management, lack of 
constructors, etc.

On windows, compile with pdcurses.lib in the folder (and I used VS 2015):
cl .\demo.c -I <path to directory with curses.h> pdcurses.lib user32.lib
On Linux & OS X, using GNU c:
gcc -o demo demo.c -l curses

The original idea for this was in C in the cc65 chess I wrote for the 
Commodore 64.  I ported that version to Python, then improved it a ton and
ported the improved version back to C.

Stefan Wessels, February 2017.
swessels@email.com