#ifndef __GAME_H__
#define __GAME_H__

#define N_COLUMNS_MAX 50
#define N_LINES_MAX 50

#define RIGHT_KEY 'd'
#define DOWN_KEY 's'
#define LEFT_KEY 'q'
#define UP_KEY 'z'
#define CHECK_KEY 'm'
#define FLAG_KEY 'f'
#define WONDER_KEY 'w'
#define EXIT_KEY '.'

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

struct game_t;

enum gcase game_get(struct game_t *game, int ix, int iy);

int get_n_mines_around(struct game_t *game, int x, int y);

int game_n_columns(struct game_t *game);

int game_n_lines(struct game_t *game);

int game_n_flags(struct game_t *game);

int game_n_mines(struct game_t *game);

int game_x(struct game_t *game);

int game_y(struct game_t *game);

#endif
