#include "chess.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* For debugging :) */
void print_board(ChessGame *g)
{
	int r, c;
	ChessPiece current_p;
	for (r = 0; r < 8; r++){
		for (c = 0; c < 8; c++){
			current_p = Game_pieceat(g, r, c);
			if (current_p < 10)	printf(" ");
			printf("%d ", current_p);
		}
		printf("\n");
	}
}


int pgn_to_rowcol(char *pgn)
{
	int col = pgn[0] - 'a';
	int row = 8 - (pgn[1] - '0');
	return col + (8 * row);
}

/********************************
 *            MOVE			    *	
 * ******************************/

int Move_is_capture(Move move, Position *pos)
{
	return move.is_en_passant || pos->piece_locations[move.dest] != EMT;
}

int Move_castle_type(Move move)
{
	/* Assume mover is king! So no position info needed. */
	const int dst_col = move.dest % 8;
	const int src_col = move.src  % 8;
	if (dst_col - src_col > 1)
		return 1; /* Kingside */
	else if (dst_col - src_col < -1)
		return 2; /* Queenside */
	else
		return 0; /* Neither */
}

void Move_set_shorttitle(Move *move, ChessGame *game)
{
	Position *pos = game->current_pos;

	/* Assume valid piece */
	ChessPiece piece_orig_color = pos->piece_locations[move->src];
	ChessPiece piece = piece_orig_color % 6;
	int on_letter = 0;
	switch(piece)
	{
		case W_K:
			if (Move_castle_type(*move) == 1){
				/* Kingside castle */
				move->title[0] = 'O'; 
				move->title[1] = '-'; 
				move->title[2] = 'O'; 
				move->title[3] = '\0'; 
				return;
			}
			else if (Move_castle_type(*move) == 2){
				/* Queenside castle */
				move->title[0] = 'O'; 
				move->title[1] = '-'; 
				move->title[2] = 'O'; 
				move->title[3] = '-'; 
				move->title[4] = 'O'; 
				move->title[5] = '\0'; 
				return;
			}

			move->title[on_letter] = 'K';
			on_letter++;
			break;
		case W_Q:
			move->title[on_letter] = 'Q';
			on_letter++;
			break;
		case W_R:
			move->title[on_letter] = 'R';
			on_letter++;
			break;
		case W_N:
			move->title[on_letter] = 'N';
			on_letter++;
			break;
		case W_B:
			move->title[on_letter] = 'B';
			on_letter++;
			break;
		case W_P:
			break;
		default:
			printf("something is terribly wrong");
			break;
	}
	int src_col = move->src % 8;
	int src_row = (move->src - src_col)/8;
	int col = move->dest % 8;
	int row = (move->dest - col)/8;

	/* Add specifying file/rank, if necessary */
	if (piece != W_P){
		int ambiguous = 0;
		int same_row  = 0;
		int same_col  = 0;
		int r, c;

		for (r = 0; r < 8; r++)
			for (c = 0; c < 8; c++)
				if (Game_pieceat(game, r, c) == piece_orig_color
					&& (r != src_row || c != src_col)){
					Move temp = *move;
					temp.src = c + (8 * r);
					if (Game_get_legal(game, temp) > -1){
						ambiguous = 1;
						if (r == src_row)
							same_row = 1;
						if (c == src_col)
							same_col = 1;
					}
				}

		if (ambiguous){
			if (!same_col)
				move->title[on_letter++] = src_col + 'a';
			else if (!same_row)
				move->title[on_letter++] = src_row + '1';
			else{
				move->title[on_letter++] = src_col + 'a';
				move->title[on_letter++] = src_row + '1';
			}
		}
	}

	if (Move_is_capture(*move, pos)){
		if (piece == W_P)
			move->title[on_letter++] = src_col + 'a'; /* e.g. bxa4 */
		move->title[on_letter] = 'x';
		on_letter++;
	}

	move->title[on_letter] = col + 'a';
	move->title[on_letter + 1] = 8 + '0' - row;
	on_letter += 2;

	if (move->promoting_to != EMT){
		move->title[on_letter] = '=';
		on_letter++;
		switch(move->promoting_to){
			case W_Q:
			case B_Q:
				move->title[on_letter] = 'Q';
				break;
			case W_R:
			case B_R:
				move->title[on_letter] = 'R';
				break;
			case W_B:
			case B_B:
				move->title[on_letter] = 'B';
				break;
			case W_N:
			case B_N:
				move->title[on_letter] = 'N';
				break;
			default:
				printf("LOL you can't promote to that\n");
		}
		on_letter++;
	}

	/* Append check and checkmate notation, if necessary. */
	ChessGame *game_copy = Game_create();
	Game_copy(game, game_copy);
	Game_advanceturn(game_copy, *move);
	if (Position_in_check(game_copy->current_pos)){
		if (game_copy->num_possible_moves == 0) /* Checkmate */
			move->title[on_letter++] = '#';
		else /* Just check */
			move->title[on_letter++] = '+';
	}
	Game_destroy(game_copy);

	move->title[on_letter] = '\0';
}


/********************************
 *            POSITION			*	
 * ******************************/

Position *Position_create()
{
	Position *local_p = (Position*)malloc(sizeof(Position));
	local_p->to_move = WHITE_MOVE;
	local_p->castling_rights[0] = BOTH;
	local_p->castling_rights[1] = BOTH;
	local_p->halfmove_clock = 0;
	local_p->fullmove_clock = 1; /* Start on move 1 */
	local_p->en_passant_target = -1;

	/* Don't need to malloc array bc it's already declared that
	 * it's size 64 */

	int r,c;
	for (r = 0; r < 8; r++){
		for (c = 0; c < 8; c++){
			if (r == 0){
				ChessPiece place_me;
				switch(c){
					case 0:
					case 7:
						place_me = B_R;
						break;
					case 1:
					case 6:
						place_me = B_N;
						break;
					case 2:
					case 5:
						place_me = B_B;
						break;
					case 4:
						place_me = B_K;
						break;
					default: /* Queen */
						place_me = B_Q;
				}
				local_p->piece_locations[c + (r*8)] = place_me;
			}
			else if (r == 1)
				local_p->piece_locations[c + (r*8)] = B_P;
			else if (r == 6)
				local_p->piece_locations[c + (r*8)] = W_P;
			else if (r == 7){
				ChessPiece place_me;
				switch(c){
					case 0:
					case 7:
						place_me = W_R;
						break;
					case 1:
					case 6:
						place_me = W_N;
						break;
					case 2:
					case 5:
						place_me = W_B;
						break;
					case 4:
						place_me = W_K;
						break;
					default: /* Queen */
						place_me = W_Q;
				}
				local_p->piece_locations[c + (r*8)] = place_me;
			}
			else {
				local_p->piece_locations[c + (r*8)] = EMT;
			}
		}
	}

	local_p->white_kingsrc = 4 + (7 * 8);
	local_p->black_kingsrc = 4 + (0 * 8);

	return local_p;
}

void Position_destroy(Position *p)
{
	free(p);
}

ChessPiece Position_pieceat(Position *p, int row, int col)
{
	if (row > 7 || row < 0 || col < 0 || col > 7)
		return -1;
	return p->piece_locations[col + (8*row)];
}


/* Increments out in direction determined by [xinc] and [yinc] for
 * [piece_to_find]. Returns true (1) if piece is found, else false (0). */
int find_piece_in_direction(int xinc, int yinc, int xorigin, int yorigin, 
							 ChessPiece piece_to_find, Position *p){
	ChessPiece sq_piece;
	int curr_x = xorigin + xinc;
	int curr_y = yorigin + yinc;
	while(curr_x >= 0 && curr_x < 8 && curr_y >= 0 && curr_y < 8){
		sq_piece = Position_pieceat(p, curr_y, curr_x);
		if (sq_piece != EMT) /* A piece of some sort! Let's see if it's it */
			return (sq_piece == piece_to_find);

		curr_x += xinc;
		curr_y += yinc;
	}

	return 0;
}


int find_piece_in_diagonals(int xori, int yori, ChessPiece findme, Position *p){
	return find_piece_in_direction( 1,  1, xori, yori, findme, p)
		|| find_piece_in_direction( 1, -1, xori, yori, findme, p)
		|| find_piece_in_direction(-1,  1, xori, yori, findme, p)
		|| find_piece_in_direction(-1, -1, xori, yori, findme, p);
}

int find_piece_in_orthogonals(int xori, int yori, ChessPiece findme, Position *p){
	return find_piece_in_direction( 1,  0, xori, yori, findme, p)
		|| find_piece_in_direction( 0,  1, xori, yori, findme, p)
		|| find_piece_in_direction(-1,  0, xori, yori, findme, p)
		|| find_piece_in_direction( 0, -1, xori, yori, findme, p);
}

int Position_in_check(Position *p)
{
	int kingsrc = p->to_move == WHITE_MOVE ? p->white_kingsrc : p->black_kingsrc;
	int kingx = kingsrc % 8;
	int kingy = kingsrc / 8;

	return Position_is_attacked(p, kingx, kingy);
}

int Position_is_attacked(Position *p, int col, int row)
{
	char opposite_color = (1 + p->to_move) % 2;
	ChessPiece opp_bishop = W_B + (6 * opposite_color);
	ChessPiece opp_knight = W_N + (6 * opposite_color);
	ChessPiece opp_rook = W_R + (6 * opposite_color);
	ChessPiece opp_queen = W_Q + (6 * opposite_color);
	ChessPiece opp_king = W_K + (6 * opposite_color);
	ChessPiece opp_pawn = W_P + (6 * opposite_color);


	/* Check bishop, queen, rook */
	if (find_piece_in_diagonals(col, row, opp_queen, p))
		return 1;
	if (find_piece_in_orthogonals(col, row, opp_queen, p))
		return 1;
	if (find_piece_in_diagonals(col, row, opp_bishop, p))
		return 1;
	if (find_piece_in_orthogonals(col, row, opp_rook, p))
		return 1;

	/* Check knights */
	if (Position_pieceat(p, row + 2, col + 1) == opp_knight)
		return 1;
	if (Position_pieceat(p, row + 2, col - 1) == opp_knight)
		return 1;
	if (Position_pieceat(p, row - 2, col + 1) == opp_knight)
		return 1;
	if (Position_pieceat(p, row - 2, col - 1) == opp_knight)
		return 1;
	if (Position_pieceat(p, row + 1, col + 2) == opp_knight)
		return 1;
	if (Position_pieceat(p, row + 1, col - 2) == opp_knight)
		return 1;
	if (Position_pieceat(p, row - 1, col + 2) == opp_knight)
		return 1;
	if (Position_pieceat(p, row - 1, col - 2) == opp_knight)
		return 1;


	/* Check pawns */
	int opp_pawn_yinc = (p->to_move == WHITE_MOVE) ? -1 : 1;
	if (Position_pieceat(p, row + opp_pawn_yinc, col + 1) == opp_pawn)
		return 1;
	if (Position_pieceat(p, row + opp_pawn_yinc, col - 1) == opp_pawn)
		return 1;

	/* Check king */
	if (Position_pieceat(p, row + 1, col + 1) == opp_king)
		return 1;
	if (Position_pieceat(p, row + 1, col - 1) == opp_king)
		return 1;
	if (Position_pieceat(p, row - 1, col + 1) == opp_king)
		return 1;
	if (Position_pieceat(p, row - 1, col - 1) == opp_king)
		return 1;
	if (Position_pieceat(p, row + 1, col + 0) == opp_king)
		return 1;
	if (Position_pieceat(p, row - 1, col + 0) == opp_king)
		return 1;
	if (Position_pieceat(p, row + 0, col + 1) == opp_king)
		return 1;
	if (Position_pieceat(p, row + 0, col - 1) == opp_king)
		return 1;

	return 0;
}



/********************************
 *            CHESSGAME			*	
 * ******************************/

ChessGame *Game_create()
{
	ChessGame *game = (ChessGame *)malloc(sizeof(ChessGame));
	game->current_pos = Position_create();

	Game_find_all_legal_moves(game);
	
	return game;
}

void Game_destroy(ChessGame *g)
{
	free(g->current_pos);
	free(g);
}

void Game_alter_position(ChessGame *g, Move altering_move)
{
	int dest_sq = altering_move.dest;
	int src_sq = altering_move.src;

	/* Get info for editing non-board metadata */
	ChessPiece moving_piece = g->current_pos->piece_locations[src_sq];
	int is_capture = g->current_pos->piece_locations[dest_sq] != EMT;
	int current_mover = (int)(g->current_pos->to_move);
	CastlingRights current_cr = g->current_pos->castling_rights[current_mover];
	int dst_col = altering_move.dest % 8;
	int dst_row = (altering_move.dest - dst_col)/8;
	int src_col = altering_move.src % 8;
	int src_row = (altering_move.src - src_col)/8;
	
	/* Half move clock: set to zero if pawn push or capture, else 
	 * increase */
	if (is_capture || moving_piece % 6 == 5 /* Is pawn */ )
		g->current_pos->halfmove_clock = 0;
	else
		g->current_pos->halfmove_clock++;
	/* Full move clock: increase if it is black's turn, since that marks
	 * the end of the "full move" */
	if (current_mover == BLACK_MOVE)
		g->current_pos->fullmove_clock++;

	g->current_pos->en_passant_target = -1;
	/* Change en passant, if necessary */
	if (moving_piece % 6 == 5 /* Is pawn */ )
		/* If pawn movement of more than 1, that square is the
		 * "en passant can happen here" square. */
		if ((src_row - dst_row) > 1 || (src_row - dst_row) < -1) 
			g->current_pos->en_passant_target = altering_move.dest;
				

	/* Change castling privileges, if necessary */
	/* King moved */
	if (moving_piece % 6 == 0 /* Is king */ )
		g->current_pos->castling_rights[current_mover] = NONE;
	/* Rook moved */
	else if (moving_piece % 6 == 2 /* Is rook */ && current_cr != NONE)
		/* Check if rook is in original rank */
		if (current_mover * 7 == 7 - src_row){

			/* Check kingside */
			if (src_col == 7 && current_cr == BOTH)
				g->current_pos->castling_rights[current_mover] = QUEENSIDE;
			else if (src_col == 7 && current_cr == KINGSIDE)
				g->current_pos->castling_rights[current_mover] = NONE;
			/* Check queenside */
			else if (src_col == 0 && current_cr == BOTH)
				g->current_pos->castling_rights[current_mover] = KINGSIDE;
			else if (src_col == 0 && current_cr == QUEENSIDE)
				g->current_pos->castling_rights[current_mover] = NONE;
		}

	
	/* Actually move the piece on the board */
	g->current_pos->piece_locations[dest_sq] = moving_piece;
	g->current_pos->piece_locations[src_sq] = EMT;
	/* If King moved multiple squares, then they castled,
	 * meaning move rook too. */
	if (moving_piece % 6 == 0 /* Is king */ ){
		if (dst_col - src_col < -1 || dst_col - src_col > 1) /* big mvmt */ {
			if (dst_col == 6 /* Kingside castle */){
				/* Move rook */
				g->current_pos->piece_locations[dest_sq - 1] = 
									g->current_pos->piece_locations[dest_sq + 1];
				g->current_pos->piece_locations[dest_sq + 1] = EMT;
			}
			else { /* Queenside castle */
				/* Move rook */
				g->current_pos->piece_locations[dest_sq + 1] = 
									g->current_pos->piece_locations[dest_sq - 2];
				g->current_pos->piece_locations[dest_sq - 2] = EMT;
			}
		}

		/* Modify kingsrc */
		if (g->current_pos->to_move == WHITE_MOVE)
			g->current_pos->white_kingsrc = dest_sq;
		else
			g->current_pos->black_kingsrc = dest_sq;
	}

	/* Remove pawn that got en passanted, if necessary. */
	if (altering_move.is_en_passant == 1){
		int reverse_pawn_yinc = (current_mover == WHITE_MOVE) ? 1 : -1;
		g->current_pos->piece_locations[dest_sq + (8*reverse_pawn_yinc)] = EMT;
	}

	/* Change next to move */
	char next_mover = (current_mover == WHITE_MOVE) ? BLACK_MOVE : WHITE_MOVE;
	g->current_pos->to_move = next_mover;

	/* Promote, if necessary */
	if (altering_move.promoting_to < EMT && altering_move.promoting_to >= 0)
		g->current_pos->piece_locations[dest_sq] = altering_move.promoting_to;

}	


ChessPiece Game_pieceat(ChessGame *g, int row, int col)
{
	return g->current_pos->piece_locations[col + (8*row)];
}

/* Helper function. Returns true (1) if [piece] is the same color as
 * [color]. */
int same_color(ChessPiece piece, char color)
{	
	return ((6 * color) <= piece) && ((6 + 6 * color) > piece);
}

/* Temporarily alters position to see if the king of the currently moving
 * player would be in check after the move with the given parameters is
 * played. If so, returns 1, else 0. */
int in_check_after_move(ChessGame *g, int src, int dest, int ep)
{
	ChessPiece moving_piece = g->current_pos->piece_locations[src]; 
	ChessPiece removed_piece = g->current_pos->piece_locations[dest];
	int kingsrc = g->current_pos->to_move == WHITE_MOVE 
				   ? g->current_pos->white_kingsrc 
				   : g->current_pos->black_kingsrc;

	/* Temporatily alter board! */
	g->current_pos->piece_locations[dest] = moving_piece;
	g->current_pos->piece_locations[src] = EMT;

	/* Move kingsrc if king moves */
	if (moving_piece == W_K)
		g->current_pos->white_kingsrc = dest;
	else if (moving_piece == B_K)
		g->current_pos->black_kingsrc = dest;

	/* En passant can get you in/out of check too! Important */
	ChessPiece ep_pawn;
	if (ep){
		ep_pawn = 
			g->current_pos->piece_locations[g->current_pos->en_passant_target];	
		g->current_pos->piece_locations[g->current_pos->en_passant_target] = EMT;
	}

	int in_check = Position_in_check(g->current_pos);

	/* Swap the pieces back. */	
	g->current_pos->piece_locations[dest] = removed_piece;
	g->current_pos->piece_locations[src] = moving_piece;

	if (ep)
		g->current_pos->piece_locations[g->current_pos->en_passant_target]
			= ep_pawn;

	if (moving_piece == W_K)
		g->current_pos->white_kingsrc = kingsrc;
	else if (moving_piece == B_K)
		g->current_pos->black_kingsrc = kingsrc;

	return in_check;
}

/* Helper function that simply adds a move to [g]'s list of possible
 * moves, if they don't cause check. */
void add_move(ChessGame *g, int src, int dest, ChessPiece promo, int ep)
{
	if (!in_check_after_move(g, src, dest, ep)){
		g->current_possible_moves[g->num_possible_moves].src = src;
		g->current_possible_moves[g->num_possible_moves].dest = dest;
		g->current_possible_moves[g->num_possible_moves].promoting_to = promo;
		g->current_possible_moves[g->num_possible_moves].is_en_passant = ep;
		g->num_possible_moves++;
	}
}


/* Helper function. Repeatedly adds moves in the direction given by
 * [xinc] and [yinc] until out of bounds or hitting a piece. 
 * Will be used for bishops, queens, and rooks, so assume not 
 * pawns. */
void add_moves_in_direction(int xinc, int yinc, int xorigin, int yorigin, 
							char piece_color, ChessGame *g){
	int curr_x = xorigin + xinc;
	int curr_y = yorigin + yinc;
	while(curr_x >= 0 && curr_x < 8 && curr_y >= 0 && curr_y < 8){
		if (Game_pieceat(g, curr_y, curr_x) == EMT){
			add_move(g, xorigin + (8 * yorigin), curr_x + (8 * curr_y), EMT, 0);
		}
		else if (same_color(Game_pieceat(g, curr_y, curr_x), piece_color)){
			return; /* Blocked by same color piece! */
		}
		else{ /* Opposite color piece. */
			/* Can capture, so add, but then return. */
			add_move(g, xorigin + (8 * yorigin), curr_x + (8 * curr_y), EMT, 0);
			return;
		}

		curr_x += xinc;
		curr_y += yinc;
	}
}

/* Baby helper functions for organizing and a bit less typing. Plus
 * I could make cool Thomas crazy chess. */

void add_diagonals(int xorigin, int yorigin, char piece_color, ChessGame *g)
{
	add_moves_in_direction( 1,  1, xorigin, yorigin, piece_color, g);
	add_moves_in_direction(-1,  1, xorigin, yorigin, piece_color, g);
	add_moves_in_direction( 1, -1, xorigin, yorigin, piece_color, g);
	add_moves_in_direction(-1, -1, xorigin, yorigin, piece_color, g);
}

void add_orthogonals(int xorigin, int yorigin, char piece_color, ChessGame *g)
{
	add_moves_in_direction( 1,  0, xorigin, yorigin, piece_color, g);
	add_moves_in_direction(-1,  0, xorigin, yorigin, piece_color, g);
	add_moves_in_direction( 0,  1, xorigin, yorigin, piece_color, g);
	add_moves_in_direction( 0, -1, xorigin, yorigin, piece_color, g);
}

/* Checks bounds and same color pieces, and if valid, then adds move. */
void add_if_valid(int xsrc, int ysrc, int xinc, int yinc, ChessGame *g, 
				  char piece_color)
{
	const int ydest = ysrc + yinc;
	const int xdest = xsrc + xinc;
	if(xdest >= 0 && xdest < 8 && ydest >= 0 && ydest < 8)
		if(!same_color(Game_pieceat(g, ydest, xdest), piece_color))
			add_move(g, xsrc + (8*ysrc), xdest + (8*ydest), EMT, 0);

	
}
void add_knight_moves(int xorigin, int yorigin, char piece_color, ChessGame *g)
{
	add_if_valid(xorigin, yorigin,  2,  1, g, piece_color);	
	add_if_valid(xorigin, yorigin,  2, -1, g, piece_color);	
	add_if_valid(xorigin, yorigin, -2,  1, g, piece_color);	
	add_if_valid(xorigin, yorigin, -2, -1, g, piece_color);	
	add_if_valid(xorigin, yorigin,  1,  2, g, piece_color);	
	add_if_valid(xorigin, yorigin,  1, -2, g, piece_color);	
	add_if_valid(xorigin, yorigin, -1,  2, g, piece_color);	
	add_if_valid(xorigin, yorigin, -1, -2, g, piece_color);	
}

/* Adds pawn move as normal, but if the pawn move is to the first or last
 * rank, then adds all four promotion moves. */
void add_check_promotion(ChessGame *g, int xori, int yori, int xinc, int yinc,
						 char piece_color)
{
	if (yori + yinc == 0 || yori + yinc == 	7){
		const int src = xori + (8 * yori);
		const int dest = xori + xinc + (8 * (yori + yinc));
		add_move(g, src, dest, W_Q + (6 * piece_color), 0);
		add_move(g, src, dest, W_N + (6 * piece_color), 0);
		add_move(g, src, dest, W_B + (6 * piece_color), 0);
		add_move(g, src, dest, W_R + (6 * piece_color), 0);
	}
	else 
		add_if_valid(xori, yori, xinc, yinc, g, piece_color);
}

void add_pawn_moves(int xorigin, int yorigin, char piece_color, ChessGame *g)
{
	/* Pawn color determines direction. White moves "up" the board,
	 * -1 at a time, whereas black moves "down" which is 1. */
	int yinc = (piece_color == WHITE_MOVE) ? -1 : 1;
	
	if (Game_pieceat(g, yorigin + yinc, xorigin) == EMT/* Space in front free */){
		/* Add move one in front */
		add_check_promotion(g, xorigin, yorigin, 0, yinc, piece_color);	
		/* Add double pawn move, if possible. */
		if ((yorigin == 1 && piece_color == BLACK_MOVE)
				|| (yorigin == 6 && piece_color == WHITE_MOVE)) /* Pawn start */
			/* Check that square 2 in front is free (already checked
			 * 1 in front). */
			if (Game_pieceat(g, yorigin + (2*yinc), xorigin) == EMT)
				add_if_valid(xorigin, yorigin, 0, 2*yinc, g, piece_color);
	}

	/* Check captures */
	/* Regular captures */
	int i;
	for(i = -1; i <= 1; i += 2) {
		/* MAKE SURE IN RANGE! LOL */
		if ((yorigin + yinc) >= 0 && (yorigin + yinc) < 8 &&
			(xorigin + i) >= 0 && (xorigin + i) < 8){
			ChessPiece defended_sq = Game_pieceat(g, yorigin + yinc, xorigin + i);
			if (defended_sq != EMT && !same_color(defended_sq, piece_color))
				add_check_promotion(g, xorigin, yorigin, i, yinc, piece_color);
		}
	}

	/* En passant: this one can't promote */
	const int ep_targ = g->current_pos->en_passant_target;
	if (ep_targ != -1){
		const int ep_col = ep_targ % 8;
		const int ep_row = (ep_targ - ep_col) / 8;
		if (ep_row == yorigin){
			const int x_dif = ep_col - xorigin;
			if (x_dif == 1 || x_dif == -1)
				add_move(g, xorigin + (8 * yorigin), 
							xorigin + x_dif + (8 * (yorigin + yinc)),
							EMT, 1);
		}
	}
}


void add_king_moves(int xorigin, int yorigin, char piece_color, ChessGame *g)
{
	int color_index = (int)piece_color;

	/* Normal king moves */
	add_if_valid(xorigin, yorigin,  1,  1, g, piece_color);	
	add_if_valid(xorigin, yorigin,  1, -1, g, piece_color);	
	add_if_valid(xorigin, yorigin, -1,  1, g, piece_color);	
	add_if_valid(xorigin, yorigin, -1, -1, g, piece_color);	
	add_if_valid(xorigin, yorigin,  1,  0, g, piece_color);	
	add_if_valid(xorigin, yorigin,  0, -1, g, piece_color);	
	add_if_valid(xorigin, yorigin, -1,  0, g, piece_color);	
	add_if_valid(xorigin, yorigin,  0,  1, g, piece_color);	

	/* Castling */
	if (Game_pieceat(g, yorigin, xorigin + 1) == EMT &&
		Game_pieceat(g, yorigin, xorigin + 2) == EMT &&
		!Position_is_attacked(g->current_pos, xorigin + 1, yorigin) && 
		!Position_is_attacked(g->current_pos, xorigin + 2, yorigin) && 
		(g->current_pos->castling_rights[color_index] == BOTH ||
		 g->current_pos->castling_rights[color_index] == KINGSIDE))
		add_if_valid(xorigin, yorigin, 2, 0, g, piece_color);

	if (Game_pieceat(g, yorigin, xorigin - 1) == EMT &&
		Game_pieceat(g, yorigin, xorigin - 2) == EMT &&
		Game_pieceat(g, yorigin, xorigin - 3) == EMT &&
		!Position_is_attacked(g->current_pos, xorigin - 1, yorigin) && 
		!Position_is_attacked(g->current_pos, xorigin - 2, yorigin) && 
		(g->current_pos->castling_rights[color_index] == BOTH ||
		 g->current_pos->castling_rights[color_index] == QUEENSIDE))
		add_if_valid(xorigin, yorigin, -2, 0, g, piece_color);
}




/* Helper function. Adds all the moves for the piece at row, col to
 * the array in [g].  */
void add_legal_moves_single_piece(ChessGame *g, int row, int col){
	ChessPiece moving_piece = Game_pieceat(g, row, col);
	char moving_color = g->current_pos->to_move;

	switch( moving_piece ){
		case W_K:
		case B_K:
			add_king_moves(col, row, moving_color, g);
			break;
		case W_Q:
		case B_Q:
			add_diagonals(col, row, moving_color, g);
			add_orthogonals(col, row, moving_color, g);
			break;
		case W_R:
		case B_R:
			add_orthogonals(col, row, moving_color, g);
			break;
		case W_N:
		case B_N:
			add_knight_moves(col, row, moving_color, g);
			break;
		case W_B:
		case B_B:
			add_diagonals(col, row, moving_color, g);
			break;
		case W_P:
		case B_P:
			add_pawn_moves(col, row, moving_color, g);
			break;
		default: 
			printf("oh no. something is terribly wrong.\n");
	}
}

void Game_find_all_legal_moves(ChessGame *g)
{
	g->num_possible_moves = 0;

	/* Iterate through board */
	int r, c;
	for (r = 0; r < 8; r++)
		for (c = 0; c < 8; c++)
			if (same_color(Game_pieceat(g, r, c), g->current_pos->to_move))
				add_legal_moves_single_piece(g, r, c);
}


GameCondition Game_advanceturn(ChessGame *g, Move m)
{
	Game_alter_position(g, m);
	Game_find_all_legal_moves(g);

	/* Check if game is over */
	if (g->num_possible_moves == 0){
		if (Position_in_check(g->current_pos)){
			/* That's checkmate! See who the winner is. */
			if (g->current_pos->to_move == WHITE_MOVE)
				return BLACK;
			else
				return WHITE;
		}
		else /* Stalemate */
			return DRAW;
	}

	/* Check 50 move rule */
	if (g->current_pos->halfmove_clock >= 50)
		return DRAW;

	return PLAYING;
}


GameCondition Game_advanceturn_index(ChessGame *g, int move_index)
{
	return Game_advanceturn(g, g->current_possible_moves[move_index]);
}

Move index_move(ChessGame *g, int index)
{
	return g->current_possible_moves[index];
}

int Game_get_legal(ChessGame *g, Move m)
{
	/* TODO: PROMOTION STUFF */
	int i;
	for (i = 0; i < g->num_possible_moves; i++)
		if (m.src == index_move(g, i).src && m.dest == index_move(g, i).dest)
			return i;
	return -1;
}

void Game_copy(ChessGame *src, ChessGame *target){
	target->num_possible_moves = src->num_possible_moves;
	/* Copy moves */
	int i;
	for (i = 0; i < target->num_possible_moves; i++){
		target->current_possible_moves[i].src = 
						src->current_possible_moves[i].src;
		target->current_possible_moves[i].dest = 
						src->current_possible_moves[i].dest;
		target->current_possible_moves[i].promoting_to = 
						src->current_possible_moves[i].promoting_to;
		target->current_possible_moves[i].is_en_passant = 
						src->current_possible_moves[i].is_en_passant;
		/* No need to copy title */
	}

	/* Copy position */
	target->current_pos->to_move = src->current_pos->to_move;
	target->current_pos->castling_rights[0]= src->current_pos->castling_rights[0];
	target->current_pos->castling_rights[1]= src->current_pos->castling_rights[1];
	target->current_pos->halfmove_clock = src->current_pos->halfmove_clock;
	target->current_pos->fullmove_clock = src->current_pos->fullmove_clock;
	target->current_pos->en_passant_target = src->current_pos->en_passant_target;
	target->current_pos->black_kingsrc = src->current_pos->black_kingsrc;
	target->current_pos->white_kingsrc = src->current_pos->white_kingsrc;
	for (i = 0; i < 64; i++){
		target->current_pos->piece_locations[i] = 
						src->current_pos->piece_locations[i];
	}
}



void Game_read_FEN(ChessGame *g, char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (fp == NULL){
		printf("File doesn't exist oh no\n");
		return;
	}

	ChessPiece char_to_piece_map[128];
	char *pieces = "KQRNBPkqrnbp";
	int i;
	for (i = 0; i < 12; i++)
		char_to_piece_map[(int)pieces[i]] = i;

	/* Current letter */
	int c;

	/* Fill board */
	for (i = 0; i < 64; i++){
		/* Don't care about slash since FEN style
		 * matches board order anyway. */
		while ((c = getc(fp)) == '/');

		/* Assuming valid board, c is either a number
		 * (add that num of spaces) or a piece. */
		if (c > '0' && c < '9'){
			int j;
			for (j = i; j < i + (c - '0'); j++)
				g->current_pos->piece_locations[j] = EMT;
			i = i + (c - '0') - 1;
		}
		else {
			g->current_pos->piece_locations[i] = char_to_piece_map[(int)c];
			if (c == 'K')
				g->current_pos->white_kingsrc = i;
			else if (c == 'k')
				g->current_pos->black_kingsrc = i;
		}
	}
	
	/* Who is to move: either w or b */
	while((c = getc(fp)) == ' ');
	if (c == 'w')	g->current_pos->to_move = WHITE_MOVE;
	else			g->current_pos->to_move = BLACK_MOVE;


	/* Default settings to castling/en passant */
	g->current_pos->castling_rights[0] = NONE;
	g->current_pos->castling_rights[1] = NONE;
	g->current_pos->en_passant_target = -1;

	/* Castling Privileges */
	while((c = getc(fp)) == ' ');
	while(c != ' '){
		switch(c){
			case '-':
				break;
			case 'K':
				g->current_pos->castling_rights[0] += KINGSIDE;
				break;
			case 'Q':
				g->current_pos->castling_rights[0] += QUEENSIDE;
				break;
			case 'k':
				g->current_pos->castling_rights[1] += KINGSIDE;
				break;
			case 'q':
				g->current_pos->castling_rights[1] += QUEENSIDE;
				break;
			default:
				printf("UH OH! FEN parsin issue\n");
		}
		c = getc(fp);
	}



	/* En passant square */
	while((c = getc(fp)) == ' ');
	if (c != '-'){
		int col = c - 'a';
		c = getc(fp);
		int row = c - '1';
		g->current_pos->en_passant_target = col + (8 * row);
	}

	/* Halfmove clock */
	g->current_pos->halfmove_clock = 0;
	while((c = getc(fp)) == ' ');
	while(c != ' '){
		g->current_pos->halfmove_clock *= 10;
		g->current_pos->halfmove_clock += c - '0';
		c = getc(fp);
	}

	/* Fullmove clock */
	g->current_pos->fullmove_clock = 0;
	while((c = getc(fp)) == ' ');
	while(c != ' ' && c != '\n' && c != EOF){
		g->current_pos->fullmove_clock *= 10;
		g->current_pos->fullmove_clock += c - '0';
		c = getc(fp);
	}

	fclose(fp);

	Game_find_all_legal_moves(g);
}



/****************************
 *         MOVIE            *
 ***************************/

Movie *Movie_create()
{
	Movie *movie = (Movie*)malloc(sizeof(Movie));
	movie->current_turn = 0;
	movie->length = 0;

	return movie;
}

void Movie_destroy(Movie *m)
{
	int i;
	for (i = 0; i < m->length; i++)
		Game_destroy(m->games[i]);
	free(m);
}

int Movie_add(Movie *movie, ChessGame *game)
{
	if (movie->length < MOVIE_CAPACITY){
		movie->games[movie->length] = Game_create();
		Game_copy(game, movie->games[movie->length]);
		movie->length++;
		return 1;
	}
	printf("Movie capacity reached!\n");
	return 0;
}

/* Helper function. Checks through [game]'s moves
 * and returns the index of the move that has the
 * same name as [movename]. If none, returns -1. */
int move_index_from_string(ChessGame *game, char *movename)
{
	int i;
	for (i = 0; i < game->num_possible_moves; i++){
		Move_set_shorttitle(&game->current_possible_moves[i], game);
		if (strcmp(movename, game->current_possible_moves[i].title) == 0)
			return i;
	}
	return -1;
}


Movie *Movie_create_from_PGN(char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (fp == NULL){
		printf("File doesn't exist oh no\n");
		return NULL;
	}
	ChessGame *game = Game_create();
	Movie *movie    = Movie_create();
	int c = 0;
	char current_word[100];
	int is_move, word_index, move_index;

	Movie_add(movie, game);
		
	while (c != EOF)
	{
		is_move = 1;

		/* Scroll through whitespace, to next word. */
		while((c = getc(fp)) == ' ' || c == '\n');

		/* Grab next word */
		word_index = 0;
		while (c != ' ' && c != '\n' && c != EOF){
			/* If first letter is a number, then it's
			 * not a move word. */
			if (word_index == 0 && c >= '1' && c <= '9')
				is_move = 0;

			current_word[word_index++] = c;
			c = getc(fp);
		}
		current_word[word_index] = '\0';
		if (c == EOF)	is_move = 0;

		if (is_move)
		{
			move_index = move_index_from_string(game, &current_word[0]);
			if (move_index == -1){
				printf("move index not found! Oh no.\n");
				printf("The halfturn is: %d \n", movie->length);
				printf("The move is: %s \n", current_word);
				printf("\n\nHere is each of the available moves:\n");
				int j;
				for (j = 0; j < game->num_possible_moves; j++){
					Move_set_shorttitle(&game->current_possible_moves[j], game);
					printf("%s\n", game->current_possible_moves[j].title);
				}
				return NULL;
			}
			movie->move_indices[movie->length - 1] = move_index;
			Game_advanceturn_index(game, move_index);
			Movie_add(movie, game);
		}
	}

	fclose(fp);

	Game_destroy(game);
	return movie;
}

#if 0

/* Also for debugging, maybe... */
void make_move(ChessGame *g, char *src, char *dest)
{
	Move move;
	move.src = pgn_to_rowcol(src);
	move.dest = pgn_to_rowcol(dest);
	Game_alter_position(g, move);
}

/* FOR TESTING */
int main(int argc, char **argv)
{
	ChessGame *game = Game_create();
	GameCondition game_con;
	int moov;
	int i;
	int typed;
	Move temp_move;

	do {
		moov = 0;

		print_board(game);
		printf("Here are all possible moves:\n");
		for (i = 0; i < game->num_possible_moves; i++){
			temp_move = game->current_possible_moves[i];
			Move_set_shorttitle(&temp_move, game->current_pos);
			printf(" %d : %s \n", i, temp_move.title);
		}

		while ((typed = getchar()) != '\n' && typed != EOF){
			moov = moov * 10;
			moov += (typed - '0');
		}

			
		game_con = Game_advanceturn_index(game, moov);
	}
	while (game_con == PLAYING);

	if (game_con == DRAW)
		printf("\n\nIt's a draw!\n");
	else if (game_con == WHITE)
		printf("\n\nCheckmate! White wins. \n");
	else if (game_con == BLACK)
		printf("\n\nCheckmate! Black wins. \n");

	Game_destroy(game);
	return 0;
}
#endif
