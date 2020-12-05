#include "chess.h"

typedef enum { RANDOM_MOVE, MIN_OPPT_MOVES } BotAlgo;

/* General structure for all simple/greedy chess algo bots. */
typedef struct chessbot_t
{
	/* Used as a pointer, not an internal game. Is not destroyed
	 * when ChessBot is. */
	ChessGame *game;

	BotAlgo algo_type;

	/* If it's playing as black or white */
	char color;

} ChessBot;

	
ChessBot *ChessBot_create(ChessGame *game, BotAlgo algo, char color);
void ChessBot_destroy(ChessBot *bot);


/* Calculates next move based on whatever algorithm the bot has and
 * whatever its inner position is. 
 * Should not be used unless the next move is theirs. */
Move ChessBot_find_next_move(ChessBot *bot);

/* Applies the position evaluation function [eval_game] to the current position
 * in the ChessBot by making copies for each possible move and seeing which
 * evals the highest. Returns the index of the highest-evalling move. */
int ChessBot_position_eval(ChessBot *bot, int (*eval_game)(ChessGame *g));
