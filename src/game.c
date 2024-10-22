#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "game.h"
#include "display.h"

struct game_t {
	/** nombre de mine */
	int n_mines;
	/** nombre de drapeau posés */
	int n_flags;
	/** grille de jeu */
	enum gcase grid[N_LINES_MAX][N_COLUMNS_MAX];
	/** nombre de colonnes considéré */
	int n_columns;
	/** nombre de lignes considéré */
	int n_lines;
	/** 0 si le jeux n'est pas terminé */
	int ended;
	/** abscisses de l'emplacement du curseur */
	int x;
	/** ordonnées de l'emplacement du curseur */
	int y;
};

void game_init(struct game_t *g) {
	g->n_mines = /*40*/160;
	g->n_flags = 0;
	for (int iy = 0; iy < N_LINES_MAX; ++iy) {
		for (int ix = 0; ix < N_COLUMNS_MAX; ++ix) {
			g->grid[iy][ix] = NOT_DEFINED;
		}
	}
	g->n_columns = /*16*/32;
	g->n_lines = /*16*/32;
	g->ended = 0;
	g->x = (g->n_columns-1)/2;
	g->y = (g->n_lines-1)/2;
}

enum gcase game_get(struct game_t *game, int ix, int iy) {
	return game->grid[iy][ix];
}

int game_n_columns(struct game_t *game) {
	return game->n_columns;
}

int game_n_lines(struct game_t *game) {
	return game->n_lines;
}

int game_n_flags(struct game_t *game) {
	return game->n_flags;
}

int game_n_mines(struct game_t *game) {
	return game->n_mines;
}

int game_x(struct game_t *game) {
	return game->x;
}

int game_y(struct game_t *game) {
	return game->y;
}

void check(struct game_t *game, int x, int y, int first_check);

int is_mine(enum gcase gc) {
	return gc == MINE_HIDE || gc == MINE_FOUND || gc == MINE_FLAG || gc == MINE_WONDERING;
}

int is_flag(enum gcase gc) {
	return gc == NOTHING_FLAG || gc == MINE_FLAG;
}

int get_n_mines_around(struct game_t *game, int x, int y) {
	int num = 0;
	for (int iy = y-1; iy <= y+1; ++iy) {
		for (int ix = x-1; ix <= x+1; ++ix) {
			if (ix >= 0 && ix < game->n_columns && iy >=0 && iy < game->n_lines
				&& (ix != x || iy != y)) {
				//enum gcase gc = game.grid[iy][ix];
				enum gcase gc = game_get(game, ix, iy);
				if (is_mine(gc)) {
					++num;
				}
			}
		}
	}
	return num;
}

int get_n_flags_around(struct game_t *game, int x, int y) {
	int num = 0;
	for (int iy = y-1; iy <= y+1; ++iy) {
		for (int ix = x-1; ix <= x+1; ++ix) {
			if (ix >= 0 && ix < game->n_columns && iy >=0 && iy < game->n_lines
				&& (ix != x || iy != y)) {
				enum gcase gc = game->grid[iy][ix];
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
 * initialise la grille selon
 * de façon à ce qu'il n'y ai
 * pas de mine en x y
 */
void init_grid(struct game_t *game, int x, int y) {
	for (int iy = 0; iy < game->n_lines; ++iy) {
		for (int ix = 0; ix < game->n_columns; ++ix) {
			game->grid[iy][ix] = NOTHING_HIDE;
		}
	}

	int i = game->n_mines;
	srand(clock());
	while (i > 0) {
		int x2 = rand()%(game->n_columns);
		int y2 = rand()%(game->n_lines);

		if ((abs(x2-x) > 1 || abs(y2-y) > 1) && game->grid[y2][x2] == NOTHING_HIDE) {
			game->grid[y2][x2] = MINE_HIDE;
			--i;
		}
	}
}

/**
 * pose ou enlève un drapeau si x y correspond
 * à une case cachée
 */
void put_flag(struct game_t *game, int x, int y) {
	if (game->grid[0][0] != NOT_DEFINED) {
		if (game->grid[y][x] == NOTHING_HIDE) {
			game->grid[y][x] = NOTHING_FLAG;
			++game->n_flags;
		} else if (game->grid[y][x] == MINE_HIDE) {
			game->grid[y][x] = MINE_FLAG;
			++game->n_flags;
		} else if (game->grid[y][x] == NOTHING_FLAG) {
			game->grid[y][x] = NOTHING_HIDE;
			--game->n_flags;
		} else if (game->grid[y][x] == MINE_FLAG) {
			game->grid[y][x] = MINE_HIDE;
			--game->n_flags;
		}
	}
}

/**
 * pose en enlève un drapeau d'incertitude si x y correspond
 * à une case cachée
 */
void put_wondering(struct game_t *game, int x, int y) {
	if (game->grid[0][0] != NOT_DEFINED) {
		if (game->grid[y][x] == NOTHING_HIDE) {
			game->grid[y][x] = NOTHING_WONDERING;
		} else if (game->grid[y][x] == MINE_HIDE) {
			game->grid[y][x] = MINE_WONDERING;
		} else if (game->grid[y][x] == NOTHING_WONDERING) {
			game->grid[y][x] = NOTHING_HIDE;
		} else if (game->grid[y][x] == MINE_WONDERING) {
			game->grid[y][x] = MINE_HIDE;
		}
	}
}

/**
 * affiche toutes les mines
 */
void show_mines(struct game_t *game) {
	for (int iy = 0; iy < game->n_lines; ++iy) {
		for (int ix = 0; ix < game->n_columns; ++ix) {
			if (game->grid[iy][ix] == MINE_HIDE || game->grid[iy][ix] == MINE_FLAG
				|| game->grid[iy][ix] == MINE_WONDERING) {
				game->grid[iy][ix] = MINE_FOUND;
			}
		}
	}
	display_grid(game);
}

/**
 * met des drapeaux sur toutes les mines
 */
void cover_mines(struct game_t *game) {
	for (int iy = 0; iy < game->n_lines; ++iy) {
		for (int ix = 0; ix < game->n_columns; ++ix) {
			if (game->grid[iy][ix] == MINE_HIDE
				|| game->grid[iy][ix] == MINE_WONDERING) {
				game->grid[iy][ix] = MINE_FLAG;
			}
		}
	}
	display_grid(game);
}

void end(struct game_t *game) {
	game->ended = 1;
}

void check_around(struct game_t *game, int x, int y) {
	for (int iy = y-1; iy <= y+1; ++iy) {
		for (int ix = x-1; ix <= x+1; ++ix) {
			if (ix >= 0 && ix < game->n_columns && iy >=0 && iy < game->n_lines
				&& (ix != x || iy != y)) {
				check(game, ix, iy, 0);
			}
		}
	}
}

/**
 * regarde si la case x y
 * est une mine ou non
 * est agit en conséquence
 * first_check : première vérification ?
 */
void check(struct game_t *game, int x, int y, int first_check) {
	if (game->grid[0][0] == NOT_DEFINED) init_grid(game, x, y);
	enum gcase cas = game->grid[y][x];
	if (cas == MINE_HIDE) {
		show_mines(game);
		printf("t'es stupide");
		end(game);
	} else if (cas == NOTHING_HIDE) {
		display_stack_add(x, y);
		game->grid[y][x] = NOTHING_FOUND;
		if (get_n_mines_around(game, x, y) == 0) {
			check_around(game, x, y);
		}
	} else if (cas == NOTHING_FOUND && first_check) {
		int n_mines = get_n_mines_around(game, x, y);
		int n_flags = get_n_flags_around(game, x, y);
		if (n_mines <= n_flags) {
			check_around(game, x, y);
		}
	}
}

/**
 * regarde si la partie est gagné,
 * si oui affiche en console un mot de vicoire
 * et quitte le jeu
 */
void is_won(struct game_t *game) {
	int won = 1;
	for (int iy = 0; iy < game->n_lines; ++iy) {
		for (int ix = 0; ix < game->n_columns; ++ix) {
			if (game->grid[iy][ix] == NOTHING_HIDE || game->grid[iy][ix] == NOTHING_FLAG
				|| game->grid[iy][ix] == NOT_DEFINED) {
				won = 0;
			}
		}
	}
	if (won) {
		cover_mines(game);
		display_grid(game);
		printf("gg bg");
		end(game);
	}
}

char convert_arrow_char(char arrow) {
	switch (arrow) {
	case 'C':
		return RIGHT_KEY;
	case 'B':
		return DOWN_KEY;
	case 'D':
		return LEFT_KEY;
	case 'A':
		return UP_KEY;
	}
	return arrow;
}

/**
 * effectue les action en fonciotn des inputs
 */
void action(struct game_t *game, char input) {
	display_stack_add(game->x, game->y);
	
	if (input == '\033') {
		getchar();
		input = convert_arrow_char(getchar());
	}
	switch (input) {
	case RIGHT_KEY: // droite
		game->x += 1;
		break;
	case DOWN_KEY: // bas
		game->y += 1;
		break;
	case LEFT_KEY: // gauche
		game->x -= 1;
		break;
	case UP_KEY: // haut
		game->y -= 1;
		break;
	case FLAG_KEY: // flag
		put_flag(game, game->x, game->y);
		break;
	case WONDER_KEY: // wondering
		put_wondering(game, game->x, game->y);
		break;
	case CHECK_KEY: // check
		check(game, game->x, game->y, 1);
		break;
	case REFRESH_KEY: // refresh
		display_grid(game);
		break;
	}

	// on évite de sortir de la grille
	if (game->x < 0) game->x = 0;
	if (game->x >= game->n_columns) game->x = game->n_columns -1;
	if (game->y < 0) game->y = 0;
	if (game->y >= game->n_lines) game->y = game->n_lines-1;
	is_won(game);
	
	display_stack_add(game->x, game->y);
}

int main(int argc, char* argv[]) {
	struct game_t game = {};
	game_init(&game);
	
	if (argc >= 4) {
		game.n_columns = atoi(argv[1]);
		game.n_lines = atoi(argv[2]);
		game.n_mines = atoi(argv[3]);
	
		if (game.n_mines >= game.n_columns*game.n_lines) {
			printf("Il y à trop de mines\n");
			exit(1);
		}
		if (game.n_columns > N_COLUMNS_MAX || game.n_lines > N_LINES_MAX) {
			printf("les dimension maximal de la grille sont : %d par %d\n",
				   N_COLUMNS_MAX, N_LINES_MAX);
			exit(1);
		}
	}
	
	display_init(&game);
	display_grid(&game);
	
	char input = 0;
	
	while (input != '.' && !game.ended) {
		display_update(&game);
		input = getchar();
		action(&game, input);
	}
	
	display_terminate();
	
	return 0;
}
