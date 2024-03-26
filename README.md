# cmatrix, but not

I liked the aesthetic of cmatrix, but wanted to customize it more,
specifically to add a status bar with battery percentage and time.
I created this specifically for my chromebook, and it has a couple paths
hardcoded, such as the location of the battery interface. Not the best
practice, but I am lazy.

This program is run through command-line, with up to 2 arguments -
status string and trail characterset. My script does limited parsing
of the status string, for example, hours and minutes are represented
by the escape characters `\h` and `\m`, and battery percentage is
similarly represented by `\b`. Colors can be set with `\R`, `\G`, `\B`,
`\Y`, `\C`, `\W`, etc, and newlines are `\n`. The charset does not accept
any escape character options.

For example, matrix can be run with `matrix "hello world" "01"` to display the
status `hello world` in the top left, with 0s and 1s falling down the screen.

The parser isn't smart enough to check if the status string goes off the screen,
so be careful of that. It'll likely cause graphical issues.

Quit the program with 'q' or Ctrl-c. You can also pause/resume the display with
the 'l' key.

Enjoy!


Note: You'll need ncursesw headers to compile. On Debian-based distros this
is usually the package `libncursesw5-dev`.
