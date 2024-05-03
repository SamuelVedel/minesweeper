#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define N_COLUMNS_MAX 50
#define N_LINES_MAX 50

#define WHITE "\033[00m"
#define BOLD_GREEN "\033[32;01m"
#define BOLD_RED "\033[31;01m"

/**
 * énumération des différents états
 * dans les quelles peuvent être
 * les cases du jeu
 */
enum gcase {
	NOTHING_HIDE, NOTHING_FOUND, NOTHING_FLAG,
	MINE_HIDE, MINE_FOUND, MINE_FLAG,
	NOT_DEFINED
};

/** nombre de mine */
int n_mines = /*40*/160;
/** nombre de drapeau posés */
int n_flags = 0;
/** grille de jeu */
enum gcase grid[N_LINES_MAX][N_COLUMNS_MAX] = {{NOT_DEFINED}};
/** nombre de colonnes considéré */
int n_columns = /*16*/32;
/** nombre de lignes considéré */
int n_lines = /*16*/32;
/** nombre de lignes dans la console */
int shell_lines = 0;
/** nombre de colonnes dans la console */
int shell_columns = 0;
/** 0 si le jeux n'est pas terminé */
int ended = 0;

void move_cursor(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

/**
 * retourne le caractère selon lequel la case de
 * coordonnée x y doit s'afficher
 */
char get_case_char(int x, int y) {
	enum gcase cas = grid[y][x];

	if (cas == NOTHING_FOUND) {
		int num = 0;
		for (int iy = y-1; iy <= y+1; ++iy) {
			for (int ix = x-1; ix <= x+1; ++ix) {
				if (ix >= 0 && ix < n_columns && iy >=0 && iy < n_lines) {
					enum gcase gc = grid[iy][ix];
					if (gc == MINE_HIDE || gc == MINE_FOUND || gc == MINE_FLAG) ++num;
				}
			}
		}
		if (num > 0) return '0'+num;
		return ' ';
	}

	switch (cas) {
	case NOTHING_HIDE :
		return '.';
	case NOTHING_FLAG :
		return 'F';
	case MINE_HIDE :
		return '.';
	case MINE_FOUND :
		return 'x';
	case MINE_FLAG :
		return 'F';
	case NOT_DEFINED :
		return '.';
	default :
		return 'N';
	}
}

/**
 * affiche la grille
 */
void print_grid(int x, int y) {
	int grid_w = n_columns*3+2;
	int grid_h = n_lines+2;
	int start_x = shell_columns/2-grid_w/2;
	int start_y = shell_lines/2-grid_h/2;

	move_cursor(start_x, start_y);
	printf("┌");
	for (int i = 0; i < n_columns*3; ++i) printf("─");
	printf("┐");

	for (int iy = 0; iy < n_lines; ++iy) {
		move_cursor(start_x, start_y+iy+1);
		printf("│");
		for (int ix = 0; ix < n_columns; ++ix) {
			char ch = get_case_char(ix ,iy);
			char color[20] = "";
			if (ch == 'F' || ch == 'x') strcpy(color, BOLD_RED);
			if (ch >= '1' && ch <= '9') strcpy(color, BOLD_GREEN);
			
			if (ix == x && iy == y) {
				printf("|%s%c%s|", color, ch, WHITE);
			} else {
				printf(" %s%c%s ", color, ch, WHITE);
			}
		}
		printf("│");
	}
	move_cursor(start_x, start_y+n_columns+1);
	printf("└");
	for (int i = 0; i < n_columns*3; ++i) printf("─");
	printf("┘");
	move_cursor(start_x, start_y+n_columns+2);
	printf("%sF%s : %d/%d\n", BOLD_RED, WHITE, n_flags, n_mines);
	move_cursor(start_x, start_y+n_columns+3);
}

/**
 * initialise la grille selon
 * de façon à ce qu'il n'y ai
 * pas de mine en x y
 */
void init_grid(int x, int y) {
	for (int iy = 0; iy < n_lines; ++iy) {
		for (int ix = 0; ix < n_columns; ++ix) {
			grid[iy][ix] = NOTHING_HIDE;
		}
	}

	int i = n_mines;
	srand(clock());
	while (i > 0) {
		int x2 = rand()%(n_columns);
		int y2 = rand()%(n_lines);

		if ((x2 != x || y2 != y) && grid[y2][x2] == NOTHING_HIDE) {
			grid[y2][x2] = MINE_HIDE;
			--i;
		}
	}
}

/**
 * pose un drapeau si x y correspond
 * à une case cachée
 */
void put_flag(int x, int y) {
	if (grid[0][0] != NOT_DEFINED) {
		if (grid[y][x] == NOTHING_HIDE) {
			grid[y][x] = NOTHING_FLAG;
			++n_flags;
		} else if (grid[y][x] == MINE_HIDE) {
			grid[y][x] = MINE_FLAG;
			++n_flags;
		} else if (grid[y][x] == NOTHING_FLAG) {
			grid[y][x] = NOTHING_HIDE;
			--n_flags;
		} else if (grid[y][x] == MINE_FLAG) {
			grid[y][x] = MINE_HIDE;
			--n_flags;
		}
	}
}

/**
 * affiche toutes les mines
 */
void show_mine() {
	for (int iy = 0; iy < n_lines; ++iy) {
		for (int ix = 0; ix < n_columns; ++ix) {
			if (grid[iy][ix] == MINE_HIDE || grid[iy][ix] == MINE_FLAG) {
				grid[iy][ix] = MINE_FOUND;
			}
		}
	}
	print_grid(-1, -1);
}

void end() {
	ended = 1;
}

/**
 * regarde si la case x y
 * est une mine ou non
 * est agit en conséquence
 */
void check(int x, int y) {
	if (grid[0][0] == NOT_DEFINED) init_grid(x, y);
	enum gcase cas = grid[y][x];
	if (cas == MINE_HIDE) {
		show_mine();
		printf("t'est stupide");
		end();
	}
	if (cas == NOTHING_HIDE) {
		grid[y][x] = NOTHING_FOUND;

		if (get_case_char(x, y) == ' ') {
			for (int iy = y-1; iy <= y+1; ++iy) {
				for (int ix = x-1; ix <= x+1; ++ix) {
					if (ix >= 0 && ix < n_columns && iy >=0 && iy < n_lines) {
						check(ix, iy);
					}
				}
			}
		}
	}
}

/**
 * regarde si la partie est gagné,
 * si oui affiche en console un mot de vicoire
 * et quitte le jeu
 */
void is_won() {
	int won = 1;
	for (int iy = 0; iy < n_lines; ++iy) {
		for (int ix = 0; ix < n_columns; ++ix) {
			if (grid[iy][ix] == NOTHING_HIDE || grid[iy][ix] == NOTHING_FLAG) {
				won = 0;
			}
		}
	}
	if (won) {
		print_grid(-1, -1);
		printf("gg bg");
		end();
	}
}

/**
 * effectue les action en fonciotn des inputs
 */
void action(int* x, int* y, char* input) {
	int i = 0;
	while (input[i] != 0) {
		switch (input[i]) {
		case 'd' : // droite
			*x += 1;
			break;
		case 's' : // bas
			*y += 1;
			break;
		case 'q' : // gauche
			*x -= 1;
			break;
		case 'z' : // haut
			*y -= 1;
			break;
		case 'f' : // flag
			put_flag(*x, *y);
			break;
		case 'c' : // check
			check(*x, *y);
			break;
		}

		// on évite de sortir de la grille
		if (*x < 0) *x = 0;
		if (*x >= n_columns) *x = n_columns -1;
		if (*y < 0) *y = 0;
		if (*y >= n_lines) *y = n_lines-1;
		++i;
	}
	is_won();
}

int main(int argc, char* argv[]) {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, & w);
	shell_lines = w.ws_row;
	shell_columns = w.ws_col;
	
	if (argc >= 4) {
		n_columns = atoi(argv[1]);
		n_lines = atoi(argv[2]);
		n_mines = atoi(argv[3]);

		if (n_mines >= n_columns*n_lines) {
			printf("Il y à trop de mines\n");
			exit(1);
		}
		if (n_columns > N_COLUMNS_MAX || n_lines > N_LINES_MAX) {
			printf("les dimension maximal de la grille sont : %d par %d\n",
				   N_COLUMNS_MAX, N_LINES_MAX);
			exit(1);
		}
	}

	int x = (n_columns-1)/2;
	int y = (n_lines-1)/2;
	char input = 0;
	system("/bin/stty raw");
	
	while (input != '.' && !ended) {
		system("clear");
		print_grid(x, y);
		char inp[50] = {};
		input = getchar();
		inp[0] = input;
		action(&x, &y, inp);
	}
	
	system("/bin/stty cooked");
	printf("\n");
	return 0;
}
