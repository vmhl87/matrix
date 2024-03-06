# This project was never meant for release

I created this to use as a screensaver for my chromebook.
I liked the aesthetic of cmatrix, but wanted to customize it more,
specifically to add a status bar with battery percentage and time.
I created this with one purpose only, and it's hardcoded for
my chromebook's filesystem structure. Use at your own risk.
It's also set in the Makefile to include ncurses headers from a
non-standard location, which is a result of my odd setup.

This program can be run through command-line, with up to 2 arguments -
status string and trail characterset. My script does limited parsing
of the status string, for example, hours and minutes are represented
by the escape characters `\h` and `\m`, and battery percentage is
similarly represented by `\b`. Colors can be set with `\R`, `\G`, `\B`,
`\Y`, `\C`, `\W`, etc, and newlines are `\n`.

The parser isn't smart enough to check if the status string goes off the screen,
so be careful of that.

Enjoy!