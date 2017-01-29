Simple Menu

These files contain a menu "function" for/in both C and Python, using curses
on Windows, Linux and OS X.  The C version is more or less a port of the
Python version.  

The user can set the folling elements for the menu, all of which are optional,
except of the "items" element:
  y             - Row height of upper left.  If omitted, center on screen in y
  x             - Column of upper left.  If omitted, center on screen in x
  width         - Width of menu screen. If omitted, longest item length
  height        - Height of menu screen. If omitted, items+head+foot+pad
  title         - Title to be shown centered. If omitted, no header+header pad
  items         - List/array of menu items user can choose from
  callbacks     - List/array of callback functions for each item
  states        - List of enabled/disabled states for each item
  footer        - Text to scroll/wrap at the bottom of the menu
  header_height - Padding lines between header and items. default = 2
  footer_height - Padding lines between items and footer. default = 2

In both the C and Python version user data can also be attached and viewed/
edited in callbacks.

Through the use of callbacks, items can be alterd, added, deleted, enabled and
disabled.  The system is quite powerful but aims to be simple.

The Python version is very straight-forward.  The C version is more complicated
mostly because of the added complexity of memory management, lack of 
constructors, etc.

The two menu.* files do compile and run as-is so they are self documented
and illustrate the functionality.

The original idea for this was in C in the cc65 chess I wrote for the 
Commodore 64.  I ported that version to Python, then improved it a ton and
ported the improved version back to C.

Stefan Wessels, January 2017.
swessels@email.com