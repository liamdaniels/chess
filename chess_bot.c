#include "chess_bot.h"
#include <limits.h>
#include <stdlib.h>
#include <time.h>

/* For some reason, rand() does... the same thing every time? >:( 
 * So here's some strange mix with rand and the unix time */
int rando(){
	/* "seed" rand */
	srand(time(NULL));
	return rand();
}

ChessBot *ChessBot_create(ChessGame *game, BotAlgo algo, char color)
{
	ChessBot *cb_local = (ChessBot *) malloc(sizeof(ChessBot));

	cb_local->game = game;
	cb_local->algo_type = algo;
	cb_local->color = color;

	return cb_local;
}

void ChessBot_destroy(ChessBot *bot)
{
	free(bot);
}

/* TODO: put this elsewhere. But for now... */
int min_oppt_moves_eval(ChessGame *g)
{
	return -1 * g->num_possible_moves;
}

Move ChessBot_find_next_move(ChessBot *bot)
{
	/* Will be finding index, and then indexing in the legal
	 * moves. By default, will be zero. */
	int move_index = 0;
	
	switch(bot->algo_type){
		case RANDOM_MOVE:
			move_index = rando() % (bot->game->num_possible_moves);
			break;
		case MIN_OPPT_MOVES:
			move_index = ChessBot_position_eval(bot, &min_oppt_moves_eval);
			break;
	}

	return bot->game->current_possible_moves[move_index];
}

int ChessBot_position_eval(ChessBot *bot, int (*eval_game)(ChessGame *g))
{
	ChessGame *game_copy = Game_create();

	long max_score = LONG_MIN;
	int max_index = 0;
	int current_score;

	int i;
	for (i = 0; i < bot->game->num_possible_moves; i++){
		Game_copy(bot->game, game_copy);
		Game_advanceturn_index(game_copy, i);

		current_score = (*eval_game)(game_copy);

		if (current_score > max_score){
			max_score = current_score;
			max_index = i;
		}
		else if (current_score == max_score){
			/* Randomize if they are swapped or not.
			 * Spices things up. */
			const int swapped = rando() % 2;
			if (swapped == 1){
				max_score = current_score;
				max_index = i;
			}
		}
	}

	Game_destroy(game_copy);
	return max_index;
}
