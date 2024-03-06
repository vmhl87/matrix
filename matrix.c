#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>
#include <stdio.h>
#include <time.h>

// default status bar - hours:minutes battery%
char *stat = "\\h:\\m - \\b%";
int stat_len = 0, stat_height = 1;

// catch keyboard interrupts, etc
volatile sig_atomic_t signal_status = 0;

void sighandler(int s){
	signal_status = s;
}

// cleanup (restore terminal to normal mode)
void finish(void){
	curs_set(1);
	endwin();
	exit(0);
}

// i know this shouldn't be hardcoded, but I am a lazy guy
void get_battery(int *tens, int *units){
	FILE *now, *full;
	int cur, max;

	now = fopen("/sys/class/power_supply/BAT0/charge_now", "r");
	full = fopen("/sys/class/power_supply/BAT0/charge_full", "r");

	fscanf(now, "%d", &cur);
	fscanf(full, "%d", &max);

	fclose(now); fclose(full);

	max /= 1000; cur /= max;

	if(cur > 999) cur = 999;

	*tens = cur/10; *units = cur%10;
}

// each falling string of text is represented by
// its head, its length, random seed (for pulling text
// out of a predefined trail string) and iteration mod,
// and last character stored
//
// iteration mod is used to make some strings only update
// every other, or every third, or every fourth, etc. iteration
typedef struct stream{
	int head;
	int len;
	int mod;
	int seed;
	char last;
}stream;

// try to write a character at a location; if it is within the bounds
// of the status window, do nothing
void trych(char c, int y, int x){
	if(x>3&&x<8+stat_len&&y>1&&y<4+stat_height) return;
	move(y, x);
	addch(c);
}

int main(int argc, char *argv[]){
	// trail charset
	char *trail = "";
	bool customtrail = 0;
	int trail_len;

	// if there are command line arguments, read in the
	// first one as the status bar string
	if(argc > 1 && strlen(argv[1]) > 0) stat = argv[1];
	// if there is a second one, use it as trail charset
	if(argc > 2){
		trail = argv[2];
		customtrail = 1;
		trail_len = strlen(trail);
	}

	// so that unicode box outline works
	setlocale(LC_ALL, "");

	// random seed
	srand((unsigned) time(NULL));

	// init ncurses
	initscr();
	nonl();
	cbreak();
	noecho();
	timeout(0);
	curs_set(0);

	// listen for interrupts, etc
	// this will make sure the terminal fixes
	// itself when a fault happens
	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGWINCH, sighandler);
	signal(SIGTSTP, sighandler);

	// colors
	start_color();
	use_default_colors();
	init_pair(COLOR_BLACK, -1, -1);
	init_pair(COLOR_RED, COLOR_RED, -1);
	init_pair(COLOR_CYAN, COLOR_CYAN, -1);
	init_pair(COLOR_BLUE, COLOR_BLUE, -1);
	init_pair(COLOR_GREEN, COLOR_GREEN, -1);
	init_pair(COLOR_WHITE, COLOR_WHITE, -1);
	init_pair(COLOR_GREEN, COLOR_GREEN, -1);
	init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
	init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);

	// store keypress & battery amounts
	int keypress, iter=0, bat_tens=0, bat_units=0;
	get_battery(&bat_tens, &bat_units);

	// store locations of hour, minute, battery in status
	// this means that if multiple \h \m \b are present in
	// the status, only the last ones get updated (which could
	// be useful for some things I suppose)
	int bat_loc=-1, hour_loc=-1, min_loc=-1,
		bat_height=1, hour_height=1, min_height=1;

	// initialize streams
	stream streams[COLS/2];
	for(int i=0; i<COLS/2; ++i){
		streams[i].head = -5 - (int)rand()%80;
		streams[i].len = 7 + (int)rand()%20;
		streams[i].mod = 1 + (int)rand()%3;
		streams[i].seed = (int)rand()%300;
	}

	// begin drawing status bar - hardcoded to top left corner
	move(3, 6);

	// length of current line of text (this is somewhat fragmented
	// and due to a drop-in hotfix for multiline status bar)
	int cur_len=0;
	
	// iterate over all characters of status bar
	for(int i=0; i<strlen(stat); ++i){
		// increment current length; if a character is an escape
		// char and doesn't take up space, we correct it later
		cur_len++;
		// if the current char is a backslash, process it as an
		// escape character, even if it isn't (this causes issues
		// with standalone backslashes at the end of a status)
		if(stat[i] == '\\'){
			// increment index to get to the next char
			i++;
			// account for this next char (again we account for
			// actual escape chars later)
			cur_len++;
			// if is a \h, retrieve hours and print
			if(stat[i] == 'h'){
				time_t rawtime = time(0);
				struct tm *loc = localtime(&rawtime);
				addch('0' + (loc->tm_hour)/10);
				addch('0' + (loc->tm_hour)%10);
				// set location of hour
				hour_loc = cur_len+4;
				hour_height = stat_height;
			// same with minutes
			}else if(stat[i] == 'm'){
				time_t rawtime = time(0);
				struct tm *loc = localtime(&rawtime);
				addch('0' + (loc->tm_min)/10);
				addch('0' + (loc->tm_min)%10);
				min_loc = cur_len+4;
				min_height = stat_height;
			// and same with battery
			}else if(stat[i] == 'b'){
				addch('0' + (bat_tens/10)%10);
				cur_len++;
				addch('0' + bat_tens%10);
				cur_len++;
				addch('.');
				addch('0' + bat_units);
				bat_loc = cur_len+2;
				bat_height = stat_height;
			// if it is a color escape code, set the color
			// and correct the current length, also, as a
			// result of this bad implementation, if a time or
			// battery string is colorized in the status bar it
			// doesn't stay colorized when it gets updated (though
			// I might fix this later)
			}else if(stat[i] == 'W'){
				attron(COLOR_PAIR(COLOR_WHITE));
				cur_len -= 2;
			}else if(stat[i] == 'G'){
				attron(COLOR_PAIR(COLOR_GREEN));
				cur_len -= 2;
			}else if(stat[i] == 'B'){
				attron(COLOR_PAIR(COLOR_BLUE));
				cur_len -= 2;
			}else if(stat[i] == 'R'){
				attron(COLOR_PAIR(COLOR_RED));
				cur_len -= 2;
			}else if(stat[i] == 'Y'){
				attron(COLOR_PAIR(COLOR_YELLOW));
				cur_len -= 2;
			}else if(stat[i] == 'M'){
				attron(COLOR_PAIR(COLOR_MAGENTA));
				cur_len -= 2;
			}else if(stat[i] == 'C'){
				attron(COLOR_PAIR(COLOR_CYAN));
				cur_len -= 2;
			// if the escape char is a newline, increment
			// status string height and reset position
			}else if(stat[i] == 'n'){
				cur_len -= 2;
				move(3+stat_height, 6);
				stat_height++;
				// update overall length
				if(cur_len > stat_len) stat_len = cur_len;
				cur_len = 0;
			// otherwise print the characters normally
			}else{
				addch('\\');
				addch(stat[i]);
			}
		// if normal character print normally
		}else addch(stat[i]);
	}

	// update overall length
	if(cur_len > stat_len) stat_len = cur_len;

	// print left and right bars around status
	for(int i=0; i<stat_height; ++i){
		move(3+i, 4);
		addstr("│");
		addch(' ');
		move(3+i, 6+stat_len);
		addch(' ');
		addstr("│");
	}

	// print top and bottom and corners
	move(2, 4);
	addstr("┌");
	for(int i=0; i<stat_len+2; ++i) addstr("─");
	addstr("┐");

	move(3+stat_height, 4);
	addstr("└");
	for(int i=0; i<stat_len+2; ++i) addstr("─");
	addstr("┘");

	// rendering loop
	while(++iter){
		// if we get an exception, cleanup and quit
		if(signal_status == SIGINT || signal_status == SIGQUIT) finish();
		if(signal_status == SIGTSTP) finish();

		// if q is pressed, quit
		if((keypress = wgetch(stdscr)) != ERR){
			if(keypress == 'q') finish();
		}

		// iterate over every other column (we only render half the columns
		// for cleanness)
		for(int i=0; i<COLS/2; ++i){
			// if this column should be updated this frame, do it
			if(iter%streams[i].mod == 0){
				// if the character just behind the head is visible,
				// draw it (and set it to green, because previously
				// when it was drawn as the head, it was drawn in white)
				if(streams[i].head > 0 && streams[i].head <= LINES){
					attron(COLOR_PAIR(COLOR_GREEN));
					trych(streams[i].last, streams[i].head-1, i*2);
					attroff(COLOR_PAIR(COLOR_GREEN));
				}
				// if the head can be drawn, draw it in white, and save it
				// so that it can be greenified next update
				if(streams[i].head >= 0 && streams[i].head < LINES){
					// if there is a custom trail, use the stream's random seed
					// and y position to get the correct char of the trail, otherwise
					// take a random character in the range [33, 123]
					if(customtrail) streams[i].last =
						trail[(streams[i].seed+streams[i].head)%trail_len];
					else streams[i].last = (int)rand()%90 + 33;
					trych(streams[i].last, streams[i].head, i*2);
				}
				// if the tail is in frame, draw it (by drawing a space character
				// over the previous tail
				if(streams[i].head >= streams[i].len &&
				streams[i].head < LINES+streams[i].len){
					trych(' ', streams[i].head-streams[i].len, i*2);
				}
				// if the entire trail has gone off screen, move it back up to a random
				// location and reset its length, speed, seed
				if(streams[i].head >= LINES+streams[i].len){
					streams[i].head = -5 - (int)rand()%20;
					streams[i].len = 7 + (int)rand()%20;
					streams[i].mod = 1 + (int)rand()%3;
					streams[i].seed = (int)rand()%300;
				}
				// move the head down so it falls
				streams[i].head++;
			}
		}

		// update the battery percentage every 25 seconds; if there is no
		// battery in the status string, bat_loc will be its default value
		// of -1
		if(bat_loc+1 && (iter%300)==0){
			get_battery(&bat_tens, &bat_units);
			// redraw
			move(2+bat_height, bat_loc);
			addch('0' + (bat_tens/10)%10);
			addch('0' + bat_tens%10);
			addch('.');
			addch('0' + bat_units);
		}

		// similar with clock, but these update every 5 sec
		if(hour_loc+1 && (iter%50)==0){
			move(2+hour_height, hour_loc);
			time_t rawtime = time(0);
			struct tm *loc = localtime(&rawtime);
			addch('0' + (loc->tm_hour)/11);
			addch('0' + (loc->tm_hour)%10);
		}

		if(min_loc+1 && (iter%50)==0){
			move(2+min_height, min_loc);
			time_t rawtime = time(0);
			struct tm *loc = localtime(&rawtime);
			addch('0' + (loc->tm_min)/10);
			addch('0' + (loc->tm_min)%10);
		}

		// pause for 80 ms
		napms(80);
	}
	
	// assume the loop ended properly, and cleanup
	finish();
}
