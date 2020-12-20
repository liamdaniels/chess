#include "chess_bot.h"
#include <stdio.h>

#define PLAYER_1_ALGO MIN_OPPT_MOVES
#define PLAYER_2_ALGO MIN_OPPT_MOVES


void bb_print_board(ChessGame *g)
{
	int r, c;
	ChessPiece current_p;
	char *pieces = "KQRNBPkqrnbp";
	for (r = 0; r < 8; r++){
		for (c = 0; c < 8; c++){
			current_p = Game_pieceat(g, r, c);
			if (current_p == EMT)
				printf("_ ");
			else
				printf("%c ", pieces[current_p]);
		}
		printf("\n");
	}
}

/* Makes two ChessBots play against each other. */
int main(int argc, char **argv)
{
	ChessGame *game = Game_create();

	ChessBot *player_1 = ChessBot_create(game, PLAYER_1_ALGO, WHITE_MOVE);
	ChessBot *player_2 = ChessBot_create(game, PLAYER_2_ALGO, BLACK_MOVE);

	Move current_move;
	GameCondition status = PLAYING;
	int turn_num = 1;
	int p1_moving = 1;
	while(status == PLAYING){
		if (p1_moving){
			printf("%d.  ", turn_num);

			current_move = ChessBot_find_next_move(player_1);
		}
		else {
			current_move = ChessBot_find_next_move(player_2);
		}

		Move_set_shorttitle(&current_move, game);
		printf("%s   ", current_move.title);

		status = Game_advanceturn(game, current_move);

		if (!p1_moving){
			turn_num++;
			printf("\n");
		}

		p1_moving = !p1_moving;
	}

	printf("\n");
	
	switch(status)
	{
		case DRAW:
			printf("DRAW\n");
			break;
		case WHITE:
			printf("WHITE WINS\n");
			break;
		case BLACK:
			printf("BLACK WINS\n");
			break;
		default:
			printf("this is impossible lmao");
	}

	bb_print_board(game);

	ChessBot_destroy(player_1);
	ChessBot_destroy(player_2);
	Game_destroy(game);
	return 0;
}
