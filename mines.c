#include <curses.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define YSPEEDINITIAL 0    // initial speed of ball in y direction
#define XSPEEDINITIAL 0    // initial speed of ball in x direction
#define YSPEEDSTEP 1       // speed increment of ball y velocity
#define XSPEEDSTEP 1       // speed increment of ball x velocity
#define SLOW_DELAY 100000  // redraw delay of 100k microseconds (slow speed)
#define FAST_DELAY 30000   // redraw delay of 30k microseconds (fast speed)
enum {
	CHOICE_SLOW,           // choice code for slow speed
	CHOICE_FAST,           // choice code for fast speed
	CHOICE_CUSTOM,         // choice code for custom speed
	CHOICE_EXIT            // choice code for exiting on speed selection menu
};
#define BUF_SIZE 16        // size of character buffer into which user enters custom speed value
#define MINE_DELAY 10      // number of delay ticks until new mine (10 every 3 seconds)
#define MINE_MAX 1000      // max number of mines (space allocated in int[][] mines array)

static int select_game_speed_in_menu(WINDOW *win);
static int enter_custom_speed(WINDOW *win);
static int check_coordinate_overlap(int candidates[2], int ball[2], int token[2], int mines[MINE_MAX][2], int mine_index);

static int
select_game_speed_in_menu(WINDOW *win)
{
	/* declare choices */

	const char *choices[] = {"Slow", "Fast", "Custom"};
	const int n_items = 3;

	/* delcare items array and menu */

	ITEM **items;
	MENU *menu;

	/* allocate items array */

	items = (ITEM **) calloc(n_items + 1, sizeof(ITEM *));

	/* populate items array with new items from choices */

	int i;
	for (i = 0; i < n_items; i++) {
		items[i] = new_item(choices[i], NULL);
	}

	/* end items array with a NULL item */

	items[n_items] = (ITEM *) NULL;

	/* create menu */

	menu = new_menu(items);

	/* draw usage message and menu */

	clear();

	int max_y, max_x;
	getmaxyx(win, max_y, max_x);
	mvprintw(max_y / 2, 0, "Select game speed with j and k (q to quit)");

	post_menu(menu);

	refresh();

	/* handle user movement and selection within the menu */

	const char *choice;
	char key;
	while (1) {
		key = getch();
		if (key == 106) {
			menu_driver(menu, REQ_DOWN_ITEM);       // down (j)
		} else if (key == 107) {
			menu_driver(menu, REQ_UP_ITEM);         // up (k)
		} else if (key == 10) {
			choice = item_name(current_item(menu)); // select (Enter)
			break;
		} else if (key == 113) {
			choice = "EXIT";                        // quit (q)
			break;
		}
	}

	/* free memory for items and menu */

	unpost_menu(menu);

	for (i = 0; i < n_items; i++) {
		free_item(items[i]);
	}

	free_menu(menu);

	/* return matching choice code for selected choice  */

	for (i = 0; i < n_items; i++) {
		if (strncmp(choice, choices[i], 4 * sizeof(char)) == 0) {
			return i; // CHOICE_SLOW, CHOICE_FAST, CHOICE_CUSTOM
		}
	}

	return CHOICE_EXIT;
}

static int
enter_custom_speed(WINDOW *win)
{
	/* initialize buffer into which user will enter value */

	char buf[BUF_SIZE + 1];

	int i;
	for (i = 0; i < BUF_SIZE; i++) {
		buf[i] = ' ';
	}

	buf[BUF_SIZE] = '\0';

	int n_chars_entered = 0;
	int selected_speed;
	
	/* draw usage message */

	clear();

	/* get window bounds to center usage message */

	int max_y, max_x;
	getmaxyx(win, max_y, max_x);

	refresh();

	/* disable non-blocking getch() */

	nodelay(win, FALSE);

	/* user input loop */

	char key;
	while (1)
	{
		clear();
		mvprintw(0, 0, "%s", buf);
		mvprintw(max_y / 2, 0, "Enter speed (redraw delay in microseconds) (x to delete, q to quit)");
		mvprintw(max_y / 2 + 2, 0, "[Default slow speed is 100k, default fast speed is 30k]");
		refresh();

		key = getch();
		if (key >= 48 && key <= 57 && n_chars_entered < BUF_SIZE) {
			buf[n_chars_entered] = key;    // digit entry (0-9)
			n_chars_entered += 1;
		} else if (key == 120 && n_chars_entered > 0) {
			buf[n_chars_entered - 1] = ' '; // delete a digit (x)
			n_chars_entered -= 1;
		} else if (key == 10) {
			buf[n_chars_entered] = '\0';   // select current number (Enter)
			selected_speed = atoi(buf);
			break;
		} else if (key == 113) {
			selected_speed = -1;           // quit (q)
			break;
		}
	}

	nodelay(win, TRUE); // re-enable non-blocking getch()
	return selected_speed;

}

static int
check_coordinate_overlap(int candidates[2], int ball[2], int token[2], int mines[MINE_MAX][2], int mine_index)
{
	/* check overlap with ball */

	if (candidates[0] == ball[0] && candidates[1] == ball[1]) {
		return 1;
	}
	
	/* check overlap with token */

	if (candidates[0] == token[0] && candidates[1] == token[1]) {
		return 1;
	}

	/* check overlap with mines */

	int i;
	for (i = 0; i < mine_index; i++) {
		if (candidates[0] == mines[i][0] && candidates[1] == mines[i][1]) {
			return 1;
		}
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	/* initialize window */

	WINDOW *stdscr = initscr();
	
	if (stdscr == NULL) {
		return 1;
	}

	/* set curses options */

	noecho();              // don't echo keypresses
	curs_set(FALSE);       // don't display cursor
	cbreak();              // don't buffer user input until newline
	nodelay(stdscr, TRUE); // causes getch() to be non-blocking

	/* user selects game speed */

	int delay_choice, delay;

	delay_choice = select_game_speed_in_menu(stdscr);

	if (delay_choice == CHOICE_SLOW) {
		delay = SLOW_DELAY;
	} else if (delay_choice == CHOICE_FAST) {
		delay = FAST_DELAY;
	} else if (delay_choice == CHOICE_CUSTOM) {
		delay = enter_custom_speed(stdscr);
		if (delay == -1) {
			endwin();
			return 0;
		}
	} else if (delay_choice == CHOICE_EXIT) {
		endwin();
		return 0;
	}
	
	/* seed rng with time */

	srandom(time(NULL));

	/* ball variables */

	int ball[2] = {1, 1};       // (y,x) position of ball, starts at top left
	int yspeed = YSPEEDINITIAL; // y speed of ball
	int xspeed = XSPEEDINITIAL; // x speed of ball

	/* mine variables */

	int mines[MINE_MAX][2]; // mines array (MINE_MAX number of (y,x) coord pairs)
	int mine_index = 0;     // index to track number of mines
	int mine_timer = 0;     // counter to use as a timekeeper between new mines
	int collision = 0;      // boolean value to store whether ball has collided with a mine

	/* token variables */

	int token[2];        // (y,x) coords of current token
	int collections = 0; // number of tokens collected

	/* misc variables */

	int max_y, max_x;  // coord bounds of screen (will be set by getmaxyx())
	int key;           // user keycodes returned by getch()
	int candidates[2]; // potential (y,x) values for new spawns, to be checked against existing objects
	int i;             // general index variable for loops

	/* generate first token */

	getmaxyx(stdscr, max_y, max_x);
	token[0] = random() % max_y;
	token[1] = random() % max_x;

	/* gameloop */

	while (collision != 1)
	{
		/* clear buffer */

		clear();

		/* draw ball to buffer */

		mvprintw(ball[0], ball[1], "o");

		/* draw mines to buffer */

		for (i = 0; i < mine_index; i++) {
			mvprintw(mines[i][0], mines[i][1], "*");
		}

		/* draw token to buffer */

		mvprintw(token[0], token[1], "@");

		/* draw score message to buffer */

		mvprintw(0, max_x - 11, "Tokens: %d", collections);

		/* display buffer to screen */

		refresh();

		/* check for ball collision with mines, loop exits on collision */

		for (i = 0; i < mine_index; i++) {
			if (ball[0] == mines[i][0] && ball[1] == mines[i][1]) {
				collision = 1;
			}
		}

		/* get coordinate bounds of window (updates dynamically) */

		getmaxyx(stdscr, max_y, max_x);

		/* check for ball collection of token, generate new token coords if collected */

		if (ball[0] == token[0] && ball[1] == token[1]) {
			collections += 1;

			do {
				candidates[0] = random() % max_y;
				candidates[1] = random() % max_x;
			} while (check_coordinate_overlap(candidates, ball, token, mines, mine_index) == 1);

			token[0] = candidates[0];
			token[1] = candidates[1];
		}

		/* reverse direction if at bounds */

		if (ball[0] <= 0 || ball[0] >= max_y) {
			yspeed *= -1;
		}

		if (ball[1] <= 0 || ball[1] >= max_x) {
			xspeed *= -1;
		}

		/* user input to alter ball velocity */

		key = getch();
		if (key == 106) {
			yspeed += YSPEEDSTEP; // down (j)
		} else if (key == 107) {
			yspeed -= YSPEEDSTEP; // up (k)
		} else if (key == 108) {
			xspeed += XSPEEDSTEP; // right (l)
		} else if (key == 104) {
			xspeed -= XSPEEDSTEP; // left (h)
		} else if (key == 113) {
			break;                // quit (q)
		}

		/* update ball position from speed */

		ball[0] += yspeed;
		ball[1] += xspeed;

		/* add a mine to mines if time interval traversed and space exists */

		mine_timer += 1;
		if (mine_timer == MINE_DELAY) {
			mine_timer = 0;
			if (mine_index < MINE_MAX) {
				do {
					candidates[0] = random() % max_y;
					candidates[1] = random() % max_x;
				} while (check_coordinate_overlap(candidates, ball, token, mines, mine_index) == 1);

				mines[mine_index][0] = candidates[0];
				mines[mine_index][1] = candidates[1];

				mine_index += 1;
			}
		}

		/* pause for delay microseconds */

		usleep(delay);
	}

	/* exit */

	endwin();

	if (collections == 1) {
		printf("You collected 1 token!\n");
	} else {
		printf("You collected %d tokens!\n", collections);
	}

	return 0;
}

