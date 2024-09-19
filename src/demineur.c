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
#define BOLD_YELLOW "\033[33;01m"

/**
 * énumération des différents états
 * dans les quelles peuvent être
 * les cases du jeu
 */
enum gcase {
	NOTHING_HIDE, NOTHING_FOUND, NOTHING_FLAG, NOTHING_WONDERING,
	MINE_HIDE, MINE_FOUND, MINE_FLAG, MINE_WONDERING,
	NOT_DEFINED
};


char explanations[] = "\033[00;01mc\033[00m: check; \
\033[00;01mf\033[00m: flag; \033[00;01mw\033[00m: wondering; \
move \033[00;01mzqsd\033[00m or \033[00;01m↑←↓→\033[00m";
int explanations_size = 50;

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

void check(int x, int y, int first_check);

void move_cursor(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

int is_mine(enum gcase gc) {
	return gc == MINE_HIDE || gc == MINE_FOUND || gc == MINE_FLAG || gc == MINE_WONDERING;
}

int is_flag(enum gcase gc) {
	return gc == NOTHING_FLAG || gc == MINE_FLAG;
}

int get_n_mines_around(int x, int y) {
	int num = 0;
	for (int iy = y-1; iy <= y+1; ++iy) {
		for (int ix = x-1; ix <= x+1; ++ix) {
			if (ix >= 0 && ix < n_columns && iy >=0 && iy < n_lines
				&& (ix != x || iy != y)) {
				enum gcase gc = grid[iy][ix];
				if (is_mine(gc)) {
					++num;
				}
			}
		}
	}
	return num;
}

int get_n_flags_around(int x, int y) {
	int num = 0;
	for (int iy = y-1; iy <= y+1; ++iy) {
		for (int ix = x-1; ix <= x+1; ++ix) {
			if (ix >= 0 && ix < n_columns && iy >=0 && iy < n_lines
				&& (ix != x || iy != y)) {
				enum gcase gc = grid[iy][ix];
				if (is_flag(gc)) {
					++num;
				}
			}
		}
	}
	return num;
}

/*int get_n_nothing_found_around(int x, int y) {
	int num = 0;
	for (int iy = y-1; iy <= y+1; ++iy) {
		for (int ix = x-1; ix <= x+1; ++ix) {
			if (ix >= 0 && ix < n_columns && iy >=0 && iy < n_lines
				&& (ix != x || iy != y)) {
				enum gcase gc = grid[iy][ix];
				if (gc == NOTHING_FOUND) {
					++num;
				}
			}
		}
	}
	return num;
}*/

/**
 * retourne le caractère selon lequel la case de
 * coordonnée x y doit s'afficher
 */
char get_case_char(int x, int y) {
	enum gcase cas = grid[y][x];

	if (cas == NOTHING_FOUND) {
		int num = get_n_mines_around(x, y);
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
			if (ch == '?') strcpy(color, BOLD_YELLOW);
			
			if (ix == x && iy == y) {
				printf("|%s%c%s|", color, ch, WHITE);
			} else {
				printf(" %s%c%s ", color, ch, WHITE);
			}
		}
		printf("│");
	}
	move_cursor(start_x, start_y+n_lines+1);
	printf("└");
	for (int i = 0; i < n_columns*3; ++i) printf("─");
	printf("┘");
	move_cursor(start_x, start_y+grid_h);
	printf("%sF%s : %d/%d\n", BOLD_RED, WHITE, n_flags, n_mines);
	move_cursor(start_x+grid_w-explanations_size, start_y+grid_h);
	printf("%s", explanations);
	move_cursor(start_x, start_y+n_lines+3);
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

		if ((abs(x2-x) > 1 || abs(y2-y) > 1) && grid[y2][x2] == NOTHING_HIDE) {
			grid[y2][x2] = MINE_HIDE;
			--i;
		}
	}
}

/**
 * pose ou enlève un drapeau si x y correspond
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
 * pose en enlève un drapeau d'incertitude si x y correspond
 * à une case cachée
 */
void put_wondering(int x, int y) {
	if (grid[0][0] != NOT_DEFINED) {
		if (grid[y][x] == NOTHING_HIDE) {
			grid[y][x] = NOTHING_WONDERING;
		} else if (grid[y][x] == MINE_HIDE) {
			grid[y][x] = MINE_WONDERING;
		} else if (grid[y][x] == NOTHING_WONDERING) {
			grid[y][x] = NOTHING_HIDE;
		} else if (grid[y][x] == MINE_WONDERING) {
			grid[y][x] = MINE_HIDE;
		}
	}
}

/**
 * affiche toutes les mines
 */
void show_mine() {
	for (int iy = 0; iy < n_lines; ++iy) {
		for (int ix = 0; ix < n_columns; ++ix) {
			if (grid[iy][ix] == MINE_HIDE || grid[iy][ix] == MINE_FLAG
				|| grid[iy][ix] == MINE_WONDERING) {
				grid[iy][ix] = MINE_FOUND;
			}
		}
	}
	print_grid(-1, -1);
}

void end() {
	ended = 1;
}

void check_around(int x, int y) {
	for (int iy = y-1; iy <= y+1; ++iy) {
		for (int ix = x-1; ix <= x+1; ++ix) {
			if (ix >= 0 && ix < n_columns && iy >=0 && iy < n_lines
				&& (ix != x || iy != y)) {
				check(ix, iy, 0);
			}
		}
	}
}

/**
 * regarde si la case x y
 * est une mine ou non
 * est agit en conséquence
 */
void check(int x, int y, int first_check) {
	if (grid[0][0] == NOT_DEFINED) init_grid(x, y);
	enum gcase cas = grid[y][x];
	if (cas == MINE_HIDE) {
		show_mine();
		printf("t'es stupide");
		end();
	} else if (cas == NOTHING_HIDE) {
		grid[y][x] = NOTHING_FOUND;
		if (get_n_mines_around(x, y) == 0) {
			check_around(x, y);
		}
	} else if (cas == NOTHING_FOUND && first_check) {
		int n_mines = get_n_mines_around(x, y);
		int n_flags = get_n_flags_around(x, y);
		if (n_mines <= n_flags) {
			check_around(x, y);
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

char convert_arrow_char(char arrow) {
	switch (arrow) {
	case 'C':
		return 'd';
	case 'B':
		return 's';
	case 'D':
		return 'q';
	case 'A':
		return 'z';
	}
	return arrow;
}

/**
 * effectue les action en fonciotn des inputs
 */
void action(int* x, int* y, char input) {
	if (input == '\033') {
		getchar();
		input = convert_arrow_char(getchar());
	}
	switch (input) {
	case 'd': // droite
	case 'D':
		*x += 1;
		break;
	case 's': // bas
	case 'S':
		*y += 1;
		break;
	case 'q': // gauche
	case 'Q':
		*x -= 1;
		break;
	case 'z': // haut
	case 'Z':
		*y -= 1;
		break;
	case 'f': // flag
	case 'F':
		put_flag(*x, *y);
		break;
	case 'w': // wondering
	case 'W':
		put_wondering(*x, *y);
		break;
	case 'c': // check
	case 'C':
		check(*x, *y, 1);
		break;
	}

	// on évite de sortir de la grille
	if (*x < 0) *x = 0;
	if (*x >= n_columns) *x = n_columns -1;
	if (*y < 0) *y = 0;
	if (*y >= n_lines) *y = n_lines-1;
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
	//system("/bin/stty raw -echo");
	
	while (input != '.' && !ended) {
		system("clear");
		print_grid(x, y);
		input = getchar();
		action(&x, &y, input);
	}
	
	system("/bin/stty cooked");
	//system("/bin/stty cooked -echo");
	printf("\n");
	return 0;
}
