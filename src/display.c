#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "game.h"
#include "display.h"

#define WHITE "\033[00m"
#define WHITE_BOLD "\033[00;01m"
#define BOLD_GREEN "\033[32;01m"
#define BOLD_RED "\033[31;01m"
#define BOLD_YELLOW "\033[33;01m"

/** chaine de caractère d'explication */
char explanations[146] = {};
int explanations_size = 59;

/** nombre de lignes dans la console */
int shell_lines = 0;
/** nombre de colonnes dans la console */
int shell_columns = 0;

/** initialise la chaine de caractère d'explication */
void init_explanation() {
	sprintf(explanations, "%s%c%s: check; %s%c%s: flag; %s%c%s: wondering; move \
%s%c%c%c%c%s or %s↑←↓→%s; %s%c%s: exit",
			WHITE_BOLD, CHECK_KEY, WHITE,
			WHITE_BOLD, FLAG_KEY, WHITE,
			WHITE_BOLD, WONDER_KEY, WHITE,
			WHITE_BOLD, UP_KEY, LEFT_KEY, DOWN_KEY, RIGHT_KEY, WHITE,
			WHITE_BOLD, WHITE,
			WHITE_BOLD, EXIT_KEY, WHITE);
}

void display_init() {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, & w);
	shell_lines = w.ws_row;
	shell_columns = w.ws_col;
	
	init_explanation();
	
	system("/bin/stty raw");
}

void display_terminate() {
	system("/bin/stty cooked");
	printf("\n");
}

/** bouge le curseur dans la console */
void move_cursor(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

/**
 * retourne le caractère selon lequel la case de
 * coordonnée x y doit s'afficher
 */
char get_case_char(struct game_t *game, int x, int y) {
	enum gcase cas = game_get(game, x, y);

	if (cas == NOTHING_FOUND) {
		int num = get_n_mines_around(game, x, y);
		if (num > 0) return '0'+num;
		return ' ';
	}

	switch (cas) {
	case NOTHING_HIDE :
		return '.';
	case NOTHING_FLAG :
		return 'F';
	case NOTHING_WONDERING:
		return '?';
	case MINE_HIDE :
		return '.';
	case MINE_FOUND :
		return 'x';
	case MINE_FLAG :
		return 'F';
	case MINE_WONDERING:
		return '?';
	case NOT_DEFINED :
		return '.';
	default :
		return 'N';
	}
}

/**
 * réaffiche toute la grille
 */
void display_grid(struct game_t *game, int x, int y) {
	int n_columns = game_n_columns(game);
	int n_lines = game_n_lines(game);
	
	int grid_w = n_columns*3+2;
	int grid_h = n_lines+2;
	int start_x = shell_columns/2-grid_w/2;
	int start_y = shell_lines/2-grid_h/2;

	// haut
	move_cursor(start_x, start_y);
	printf("┌");
	for (int i = 0; i < n_columns*3; ++i) printf("─");
	printf("┐");

	// grille
	for (int iy = 0; iy < n_lines; ++iy) {
		move_cursor(start_x, start_y+iy+1);
		printf("│");
		for (int ix = 0; ix < n_columns; ++ix) {
			char ch = get_case_char(game, ix ,iy);
			char color[20] = "";
			if (ch == 'F' || ch == 'x') strcpy(color, BOLD_RED);
			if (ch >= '1' && ch <= '9') strcpy(color, BOLD_GREEN);
			if (ch == '?') strcpy(color, BOLD_YELLOW);
			
			if (ix == x && iy == y) {
				printf("|%s%c%s|", color, ch, WHITE);
			} else {
				printf(" %s%c%s ", color, ch, WHITE);
			}
		}
		printf("│");
	}
	// bas
	move_cursor(start_x, start_y+n_lines+1);
	printf("└");
	for (int i = 0; i < n_columns*3; ++i) printf("─");
	printf("┘");
	// explication des touches
	move_cursor(start_x+grid_w-explanations_size, start_y+grid_h);
	printf("%s", explanations);
	// nombre de drapeau
	move_cursor(start_x, start_y+grid_h);
	printf("%sF%s : %d/%d\n", BOLD_RED, WHITE, game_n_flags(game), game_n_mines(game));
	move_cursor(start_x, start_y+n_lines+3);

}
