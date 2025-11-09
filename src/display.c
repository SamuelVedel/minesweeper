#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "game.h"
#include "display.h"

#define STACK_LENGTH N_COLUMNS_MAX*N_ROWS_MAX

#define SQUARE_WIDTH 3 // largeur d'une case dans le terminal
// taille maximal théorique du décompte des drapeau dans la terminal
#define MAX_FLAG_COUNT_LEN 11

#define WHITE       "\e[00m"
#define WHITE_BOLD  "\e[00;01m"
#define BOLD_GREEN  "\e[32;01m"
#define BOLD_RED    "\e[31;01m"
#define BOLD_YELLOW "\e[33;01m"

int stack[STACK_LENGTH][2] = {};
size_t stack_size = 0;

/** chaine de caractère d'explication */
char explanations[171] = {};
//int explanations_size = 71;

/** nombre de lignes dans la console */
int shell_row = 0;
/** nombre de colonnes dans la console */
int shell_col = 0;

/** largeur de la grille */
int grid_w = 0;
/** hauteur de la grille */
int grid_h = 0;
/** abscisse de début de la grille */
int grid_x = 0;
/** ordonnées de début de la grille */
int grid_y = 0;

/**
 * Retourne la longueur de la chaine de charactère
 * d'explication données en paramètres
 * sans compté ce qui set à l'affichage
 */
int explen(char *exp) {
	int len = 0;
	int counting = 1;
	for (;*exp != 0; ++exp) {
		if (counting) {
			if (*exp == '\e') {
				counting = 0;
				continue;
			}
			if ((unsigned char) *exp == 226) {
				len -= 2;
			}
			++len;
		} else {
			if (*exp == 'm') {
				counting = 1;
			}
		}
	}
	return len;
}

/** initialise la chaine de caractère d'explication */
void init_explanation() {
	sprintf(explanations, "%s%c%s: check; %s%c%s: flag; %s%c%s: wondering; move "
			"%s%c%c%c%c%s or %s↑←↓→%s; %s%c%s: exit; %s%c%s: refresh",
			WHITE_BOLD, CHECK_KEY, WHITE,
			WHITE_BOLD, FLAG_KEY, WHITE,
			WHITE_BOLD, WONDER_KEY, WHITE,
			WHITE_BOLD, UP_KEY, LEFT_KEY, DOWN_KEY, RIGHT_KEY, WHITE,
			WHITE_BOLD, WHITE,
			WHITE_BOLD, EXIT_KEY, WHITE,
			WHITE_BOLD, REFRESH_KEY, WHITE);
	//explanations_size = explen(explanations);
}

/**
 * rècupère les dimensions d'affichage
 * de la grille et les stockes dans les variables :
 *  - grid_w
 *  - grid_h
 */
void get_grid_dimensions(struct game_t *game) {
	int n_columns = game_n_columns(game);
	int n_rows = game_n_rows(game);
	
	grid_w = n_columns*SQUARE_WIDTH+2;
	grid_h = n_rows+2;
}

/**
 * calcul la position de la grille en fonction
 * des dimenssion du terminal, et stockes les
 * valeurs dans :
 *  - grid_x
 *  - grid_y
 *
 * pour que cela fonctionne correctement,
 * get_grid_dimensions et get_shell_dimensions
 * doivent avoir était appelé
 */
void get_grid_position() {
	grid_x = shell_col/2-grid_w/2;
	grid_y = shell_row/2-grid_h/2;
}

/**
 * réucpère les dimensions de la grille
 * et les stockes dans les variables :
 *  - shell_row
 *  - shell_col
 *
 * Cette fonction appel aussi la fonction
 * get_grid_position() si définit les variables :
 *  - grid_x
 *  - grid_y
 */
void get_shell_dimensions() {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	shell_row = w.ws_row;
	shell_col = w.ws_col;
	get_grid_position();
}

void exit_if(int condition, const char *prefix) {
	if (condition) {
		if ( errno != 0 ) {
			perror(prefix);
		}
		else {
			fprintf( stderr, "%s\n", prefix );
		}
		exit(1);
	}
}

void call_cmd(char *cmd, char *arg) {
	pid_t fork_pid = fork();
	exit_if(fork_pid == -1, "fork");
	
	if (fork_pid == 0) {
		char *argv[] = {
			cmd, arg, NULL
		};
		execvp(cmd, argv);
		exit_if(1, "execvp");
	}
	exit_if(wait(NULL) == 0, "wait");
}

void display_init(struct game_t *game) {
	get_grid_dimensions(game);
	get_shell_dimensions();
	init_explanation();
	
	//system("/bin/stty raw");
	call_cmd("/bin/stty", "raw");
}

void display_terminate() {
	//system("/bin/stty cooked");
	call_cmd("/bin/stty", "cooked");
	printf("\n");
}

void display_stack_add(int x, int y) {
	stack[stack_size][0] = x;
	stack[stack_size][1] = y;
	++stack_size;
}

void display_stack_pop(int *x, int *y) {
	--stack_size;
	*x = stack[stack_size][0];
	*y = stack[stack_size][1];
}

int display_stack_is_empty() {
	return stack_size == 0;
}

/**
 * Nettoie l'écran
 */
void clear_screen() {
	//printf("\e[1;1H\e[2J");
	call_cmd("clear", NULL);
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
 * Affiche la case de coordonnées
 * (x, y) sur sortie standard
 * ne bouge pas le curseur avant l'affichage
 */
void print_one_square(struct game_t *game, int x, int y) {
	char ch = get_case_char(game, x ,y);
	char color[20] = "";
	if (ch == 'F' || ch == 'x') strcpy(color, BOLD_RED);
	if (ch >= '1' && ch <= '9') strcpy(color, BOLD_GREEN);
	if (ch == '?') strcpy(color, BOLD_YELLOW);
			
	if (x == game_x(game) && y == game_y(game)) {
		printf("|%s%c%s|", color, ch, WHITE);
		//printf(" %s%c%s ", color, ch, WHITE);
	} else {
		printf(" %s%c%s ", color, ch, WHITE);
	}
}

/**
 * Affiche la case de coordonnées
 * (x, y) sur la sortie standard
 * déplace le curseur pour l'afficher
 * au bonne endroit
 */
void display_one_square(struct game_t *game, int x, int y) {
	move_cursor(grid_x+1+x*SQUARE_WIDTH, grid_y+1+y);
	print_one_square(game, x, y);
}

/**
 * Affiche les explications des touches
 * au bonne endroit
 */
void display_explanation() {
	int explanations_size = explen(explanations);
	if (explanations_size < grid_w-MAX_FLAG_COUNT_LEN) {
		move_cursor(grid_x+grid_w-explanations_size, grid_y+grid_h);
		printf("%s", explanations);
	} else {
		
	}
}

/**
 * Affiche le décompte de drapeau
 * au bonne endroit
 */
void display_flag_count(struct game_t *game) {
	move_cursor(grid_x, grid_y+grid_h);
	printf("%sF%s : %d/%d\n", BOLD_RED, WHITE, game_n_flags(game), game_n_mines(game));
}

/**
 * réaffiche toute la grille
 */
void display_grid(struct game_t *game) {
	clear_screen();
	get_shell_dimensions();
	int n_columns = game_n_columns(game);
	int n_rows = game_n_rows(game);

	// haut
	move_cursor(grid_x, grid_y);
	printf("┌");
	for (int i = 0; i < n_columns*SQUARE_WIDTH; ++i) printf("─");
	printf("┐");

	// grille
	for (int iy = 0; iy < n_rows; ++iy) {
		move_cursor(grid_x, grid_y+iy+1);
		printf("│");
		for (int ix = 0; ix < n_columns; ++ix) {
			print_one_square(game, ix, iy);
		}
		printf("│");
	}
	// bas
	move_cursor(grid_x, grid_y+n_rows+1);
	printf("└");
	for (int i = 0; i < n_columns*SQUARE_WIDTH; ++i) printf("─");
	printf("┘");
	// explication des touches
	display_explanation();
	// nombre de drapeau
	display_flag_count(game);
	move_cursor(grid_x, grid_y+grid_h+1);

}

void display_update(struct game_t *game) {
	int x, y;
	while (!display_stack_is_empty()) {
		display_stack_pop(&x, &y);
		display_one_square(game, x, y);
	}
	display_flag_count(game);
	move_cursor(grid_x, grid_y+grid_h+1);
	/*move_cursor(grid_x+game_x(game)*SQUARE_WIDTH+2,
	  grid_y+game_y(game)+1);*/
}
