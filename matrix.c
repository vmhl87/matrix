#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>
#include <stdio.h>
#include <time.h>

char *stat = "\\h:\\m - \\b%";
int stat_len = 0, stat_height = 1;

volatile sig_atomic_t signal_status = 0;

void sighandler(int s){
	signal_status = s;
}

void finish(void){
	curs_set(1);
	endwin();
	exit(0);
}

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

typedef struct stream{
	int head;
	int len;
	int mod;
	char last;
}stream;

void trych(char c, int y, int x){
	if(x>3&&x<8+stat_len&&y>1&&y<4+stat_height) return;
	move(y, x);
	addch(c);
}

int main(int argc, char *argv[]){
	if(argc > 1) stat = argv[1];

	setlocale(LC_ALL, "");

	srand((unsigned) time(NULL));

	initscr();
	nonl();
	cbreak();
	noecho();
	timeout(0);
	curs_set(0);

	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGWINCH, sighandler);
	signal(SIGTSTP, sighandler);

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

	int keypress, iter=0, bat_tens=0, bat_units=0;
	get_battery(&bat_tens, &bat_units);

	int bat_loc=-1, hour_loc=-1, min_loc=-1,
		bat_height=1, hour_height=1, min_height=1;

	stream streams[COLS/2];
	for(int i=0; i<COLS/2; ++i){
		streams[i].head = -5 - (int)rand()%80;
		streams[i].len = 7 + (int)rand()%20;
		streams[i].mod = 1 + (int)rand()%3;
	}

	move(3, 6);

	int cur_len=0;
	
	for(int i=0; i<strlen(stat); ++i){
		cur_len++;
		if(stat[i] == '\\'){
			i++;
			cur_len++;
			if(stat[i] == 'h'){
				time_t rawtime = time(0);
				struct tm *loc = localtime(&rawtime);
				addch('0' + (loc->tm_hour)/10);
				addch('0' + (loc->tm_hour)%10);
				hour_loc = cur_len+4;
				hour_height = stat_height;
			}else if(stat[i] == 'm'){
				time_t rawtime = time(0);
				struct tm *loc = localtime(&rawtime);
				addch('0' + (loc->tm_min)/10);
				addch('0' + (loc->tm_min)%10);
				min_loc = cur_len+4;
				min_height = stat_height;
			}else if(stat[i] == 'b'){
				addch('0' + (bat_tens/10)%10);
				cur_len++;
				addch('0' + bat_tens%10);
				cur_len++;
				addch('.');
				addch('0' + bat_units);
				bat_loc = cur_len+2;
				bat_height = stat_height;
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
			}else if(stat[i] == 'n'){
				cur_len -= 2;
				move(3+stat_height, 6);
				stat_height++;
				if(cur_len > stat_len) stat_len = cur_len;
				cur_len = 0;
			}else{
				addch('\\');
				addch(stat[i]);
			}
		}else addch(stat[i]);
	}

	if(cur_len > stat_len) stat_len = cur_len;

	for(int i=0; i<stat_height; ++i){
		move(3+i, 4);
		addstr("│");
		addch(' ');
		move(3+i, 6+stat_len);
		addch(' ');
		addstr("│");
	}

	move(2, 4);
	addstr("┌");
	for(int i=0; i<stat_len+2; ++i) addstr("─");
	addstr("┐");

	move(3+stat_height, 4);
	addstr("└");
	for(int i=0; i<stat_len+2; ++i) addstr("─");
	addstr("┘");

	while(++iter){
		if(signal_status == SIGINT || signal_status == SIGQUIT) finish();
		if(signal_status == SIGTSTP) finish();

		if((keypress = wgetch(stdscr)) != ERR){
			if(keypress == 'q') finish();
		}

		for(int i=0; i<COLS/2; ++i){
			if(iter%streams[i].mod == 0){
				if(streams[i].head > 0 && streams[i].head <= LINES){
					attron(COLOR_PAIR(COLOR_GREEN));
					trych(streams[i].last, streams[i].head-1, i*2);
					attroff(COLOR_PAIR(COLOR_GREEN));
				}
				if(streams[i].head >= 0 && streams[i].head < LINES){
					streams[i].last = (int)rand()%90 + 33;
					trych(streams[i].last, streams[i].head, i*2);
				}
				if(streams[i].head >= streams[i].len &&
				streams[i].head < LINES+streams[i].len){
					trych(' ', streams[i].head-streams[i].len, i*2);
				}
				if(streams[i].head >= LINES+streams[i].len){
					streams[i].head = -5 - (int)rand()%20;
					streams[i].len = 7 + (int)rand()%20;
					streams[i].mod = 1 + (int)rand()%3;
				}
				streams[i].head++;
			}
		}

		if(bat_loc+1 && (iter%300)==0){
			get_battery(&bat_tens, &bat_units);
			move(2+bat_height, bat_loc);
			addch('0' + (bat_tens/10)%10);
			addch('0' + bat_tens%10);
			addch('.');
			addch('0' + bat_units);
		}

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

		napms(80);
	}
	
	finish();
}
