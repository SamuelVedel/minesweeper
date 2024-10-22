#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "game.h"

/**
 * Initialise l'affichage
 */
void display_init(struct game_t *game);

/**
 * A appelé à la fin du programme quand
 * on ne veut plus afficher
 */
void display_terminate();

/**
 * réaffiche la grille entière
 */
void display_grid(struct game_t *game);

/**
 * ajoute la case d'incides (x, y) dans la pile
 * d'attente des cases à réafficher
 */
void display_stack_add(int x, int y);

/**
 * Met à jour l'affichage en fonction des cases
 * à réaffichier
 */
void display_update(struct game_t *game);

#endif
