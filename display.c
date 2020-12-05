#include <SDL2/SDL.h>
#include "../../my_API/sdl/sdl_util.h"
#include "chess_bot.h"
#include <string.h>

#define WIN_W 1408
#define WIN_H 960

#define BOARD_X 50
#define BOARD_Y 50
#define SQUARE_SIDELEN 80

#define DARK_SQ_COLOR 0x592A0AFF
#define LIGHT_SQ_COLOR 0xFFC6A1FF
#define SELECT_SQ_COLOR 0x4A9CB0FF

#define PIECES_PIC "pics/pieces.png"
#define PIECES_COLS 6
#define PIECES_FRAME_SIDELEN 128

#define FONT_PIC "pics/tom_vii_font.png"
#define FONT_HEIGHT 17
#define FONT_WIDTH 11
#define FONTCHARS " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789`-=[]\\;',./~!@#$%^&*()_+{}|:\"<>?"
/* Width, height of each letter in pixels */
#define FONTSIZE_H 32
#define FONTSIZE_W 22
#define FSPACE 18

#define MAX_MOVES_PER_LINE 3

#define MOVES_Y BOARD_Y - 30
#define MOVES_X BOARD_X + (8 * SQUARE_SIDELEN) + 10

#define MOVES_STRLEN 2048

#define FRAME_DELAY 16
#define MAX_ANIMATION_FRAMES 20

/*  Info for moving pieces around the board,
 *  i.e. "animating" them. */
typedef struct {
	int active;
	int src;
	int dest;
	int frame;
	int drawx;
	int drawy;
} PieceAnimator;

/* All necessary variables for UI go here */
typedef struct {
	WinRend winrend;	

	int draw_board;
	int draw_moves;

	Sprite *pieces_spr;
	Font *font;

	ChessGame *game;

	/* Should both be -1 if nothing on board
	 * is selected */
	int row_selected;
	int col_selected;

	/* Should have -1 as attributes while move is
	 * not being selected. Once both src and dest are full, 
	 * either the move is valid and it goes through or its 
	 * attributes reset if it's not valid. */
	Move move;

	PieceAnimator anim;

	/* For writing the moves played on the screen */
	char moves_str[MOVES_STRLEN];
	int str_position;

	GameCondition game_status;

	/* Plays against human player, if applicable. */
	ChessBot *bot;
	/* True if bot is playing, false if 2 humans are playing or
	 * if we are watching a movie. */
	int bot_playing;

} UI_container;


void anim_update(UI_container *ui)
{
	const int src_x  = ((ui->anim.src % 8 ) * SQUARE_SIDELEN) + BOARD_X;
	const int src_y  = ((ui->anim.src / 8 ) * SQUARE_SIDELEN) + BOARD_Y;
	const int dest_x = ((ui->anim.dest % 8) * SQUARE_SIDELEN) + BOARD_X;
	const int dest_y = ((ui->anim.dest / 8) * SQUARE_SIDELEN) + BOARD_Y;

	const int x_inc = (dest_x - src_x) / MAX_ANIMATION_FRAMES;
	const int y_inc = (dest_y - src_y) / MAX_ANIMATION_FRAMES;
	
	ui->anim.drawx = src_x + (x_inc * ui->anim.frame);
	ui->anim.drawy = src_y + (y_inc * ui->anim.frame);

	if (ui->anim.frame >= MAX_ANIMATION_FRAMES)
		ui->anim.active = 0;
	else
		ui->anim.frame++;
}

void anim_begin(UI_container *ui, int src, int dest)
{
	ui->anim.active = 1;
	ui->anim.src = src;
	ui->anim.dest = dest;
	ui->anim.frame = 0;
	anim_update(ui);
}

int is_anim(UI_container *ui, int row, int col)
{
	const int dest_row = ui->anim.dest / 8;
	const int dest_col = ui->anim.dest % 8;
	return row == dest_row && col == dest_col;
}

void draw(UI_container *ui)
{
	SDL_Delay(FRAME_DELAY);

	if (ui->draw_board)
	{
		unsigned long color;
		int is_light;
		int r, c;
		for (c = 0; c < 8; c++){
			for (r = 0; r < 8; r++){
				const int square_x = c * SQUARE_SIDELEN + BOARD_X;
				const int square_y = r * SQUARE_SIDELEN + BOARD_Y;
				/* Draw board square */
				is_light = (c % 2) == (r % 2);
				color = is_light ? LIGHT_SQ_COLOR : DARK_SQ_COLOR;

				if (ui->row_selected == r && ui->col_selected == c)	
					color = SELECT_SQ_COLOR;

				SDLUTIL_fillrect(&(ui->winrend), 
								 square_x, square_y,
								 SQUARE_SIDELEN, SQUARE_SIDELEN, color);
				/* Draw piece, as long as it's not being animated */ 
				const ChessPiece piece = Game_pieceat(ui->game, r, c);
				if (piece != EMT && !(ui->anim.active && is_anim(ui, r, c)))
						Sprite_drawat(ui->pieces_spr, square_x, square_y, piece);
			}
		}
		/* Draw animated piece */
		if (ui->anim.active){
			const ChessPiece piece = Game_pieceat(ui->game,
												  ui->anim.dest / 8,
												  ui->anim.dest % 8);
			Sprite_drawat(ui->pieces_spr, ui->anim.drawx, ui->anim.drawy, piece);
		}
	}
	
	if (ui->draw_moves)
	{
		/* DRAW MOVES */
		Font_draw_string(ui->font, ui->moves_str, MOVES_X, MOVES_Y, FSPACE);
	}
}




/* Write move string to UI's move-containing big string */
void UI_write_move(UI_container *ui, Move move)
{
	/* Set move title */
	Move_set_shorttitle(&move, ui->game->current_pos);
	
	int pos = ui->str_position;

	/* Add move num if necessary */
	if (ui->game->current_pos->to_move == WHITE_MOVE){
		int move_num = ui->game->current_pos->fullmove_clock;

		if ((move_num % MAX_MOVES_PER_LINE) == 1)
			ui->moves_str[pos++] = '\n';

		/* Assume game has no more than 1000 moves */
		if (move_num >= 100){
			ui->moves_str[pos++] = (move_num / 100) + '0';
			move_num = move_num % 100;
		}
		if (move_num >= 10){
			ui->moves_str[pos++] = (move_num / 10) + '0';
			move_num = move_num % 10;
		}
		ui->moves_str[pos++] = move_num + '0';
		ui->moves_str[pos++] = '.';
	}

	/* Copy m's title into string. String should always
	 * end with a null terminator per Move_set_shorttitle. */
	int i;
	for (i = 0; move.title[i] != '\0'; i++)
		ui->moves_str[pos++] = move.title[i];

	ui->moves_str[pos++] = ' ';
	ui->moves_str[pos] = '\0';
	ui->str_position = pos;
}

/* Append score of game to the end */
void UI_write_endscore(UI_container *ui)
{
	const int MAX_SCORE_LEN = 8;
	char score[MAX_SCORE_LEN];

	if (ui->game_status == WHITE)
		strcpy(score, "1-0");
	else if (ui->game_status == BLACK)
		strcpy(score, "0-1");
	else
		strcpy(score, "1/2-1/2");

	int pos = ui->str_position;
	int i;
	for (i = 0; score[i] != '\0'; i++)
		ui->moves_str[pos++] = score[i];
	ui->moves_str[pos] = '\0';
}


void loop(UI_container *ui)
{
	int ui_dirty = 1;
	int move_selection = 0;
	SDL_MouseButtonEvent *mouse_ev;
	
	for (;;){
		if (!ui->anim.active){
			SDL_Event event;
			if (SDL_WaitEvent(&event)){
				switch(event.type){
					case SDL_QUIT:
						return;
					case SDL_MOUSEBUTTONDOWN:
						mouse_ev = (SDL_MouseButtonEvent*)&event;
						/* Check that click is in board bounds */
						if (mouse_ev->x >= BOARD_X 
						 && mouse_ev->x < BOARD_X + SQUARE_SIDELEN * 8
						 && mouse_ev->y >= BOARD_Y 
						 && mouse_ev->y < BOARD_Y + SQUARE_SIDELEN * 8){
							ui->row_selected = (mouse_ev->y - BOARD_Y) 
															/ SQUARE_SIDELEN;
							ui->col_selected = (mouse_ev->x - BOARD_X) 
															/ SQUARE_SIDELEN;
							move_selection = 1;
						}
						else{
							ui->row_selected = -1;
							ui->col_selected = -1;
						}
						ui_dirty = 1;
						break;
				}
			}

			/* Manage move */
			if (move_selection && 
				(!ui->bot_playing ||
				 ui->game->current_pos->to_move != ui->bot->color)){

				if (ui->move.src == -1)
					ui->move.src = ui->col_selected + (8 * ui->row_selected);
				else { /* Editing destination! Attempt move. */
					ui->move.dest = ui->col_selected + (8 * ui->row_selected);
					
					/* TODO: make promotions to non-queen pieces available */
					
					int legal_ind = Game_get_legal(ui->game, ui->move);
					if (legal_ind != -1){
						UI_write_move(
								ui,ui->game->current_possible_moves[legal_ind]);

						ui->game_status = 
							Game_advanceturn_index(ui->game, legal_ind);

						/* Animation */
						anim_begin(ui, ui->move.src, ui->move.dest);

						ui->move.src     = -1;
						ui->move.dest    = -1;
						ui->row_selected = -1;
						ui->col_selected = -1;
					}
					else { /* Assume changing source */
						ui->move.src = ui->move.dest;
						ui->move.dest = -1;
					}
				}
			}

			if (ui->game_status != PLAYING)
				UI_write_endscore(ui);			
			else if (ui->bot_playing && 
					 !ui->anim.active &&
					 ui->game->current_pos->to_move == ui->bot->color){
				/* Bot move! */
				Move bot_move = ChessBot_find_next_move(ui->bot);
				UI_write_move(ui, bot_move);
				ui->game_status = Game_advanceturn(ui->game, bot_move);

				/* Animation */
				anim_begin(ui, bot_move.src, bot_move.dest);

				if (ui->game_status != PLAYING)
					UI_write_endscore(ui);			
			}
		}
		else{
			anim_update(ui);
			ui_dirty = 1;
		}

	
		if (ui_dirty){
			SDLUTIL_clearscreen(&(ui->winrend), 0xFFFFFFFF);
			draw(ui);
			SDL_RenderPresent(ui->winrend.rend);
			ui_dirty = 0;
		}
	}
}

/* Assign default "constructor" values to UI_container. 
 * Make sure this is after SDLUTIL_begin is called. */
void UI_assign_defaults(UI_container *ui)
{
	ui->draw_board = 1;
	ui->draw_moves = 1;

	ui->pieces_spr = Sprite_create_from_picture(
						PIECES_PIC, 0, 0, PIECES_COLS, 
						PIECES_FRAME_SIDELEN, 
						PIECES_FRAME_SIDELEN, ui->winrend.rend);
	Sprite_resize(ui->pieces_spr, SQUARE_SIDELEN, SQUARE_SIDELEN);

	ui->font = Font_create(
						FONT_PIC, strlen(FONTCHARS), FONT_HEIGHT, FONT_WIDTH,
						ui->winrend.rend, FONTCHARS);
	Font_resize(ui->font, FONTSIZE_W, FONTSIZE_H);
	
	ui->row_selected = -1;
	ui->col_selected = -1;

	ui->move.src  = -1;
	ui->move.dest = -1;
	ui->move.promoting_to = EMT;

	ui->str_position= 0;
	ui->moves_str[0] = '\0';

	ui->game_status = PLAYING;

	ui->anim.active = 0;
	ui->anim.frame = 0;
	ui->anim.src = ui->anim.dest = 0;
	ui->anim.drawx = ui->anim.drawy = 0;
}

#define BOT_ALGO MIN_OPPT_MOVES
#define STARTING_FROM_FEN 0
#define FENFILE "games/FEN/ep_checkmate_test"

int main(int argc, char **argv)
{
	UI_container ui;

	/* TODO: let user set things with argv? */

	ui.game = Game_create();
	
	/* BOT */
	ui.bot = ChessBot_create(ui.game, BOT_ALGO, BLACK_MOVE);
	ui.bot_playing = 1;

	/* FEN, if wanted */
	if (STARTING_FROM_FEN)
		Game_read_FEN(ui.game, FENFILE);


	SDLUTIL_begin(SDL_INIT_VIDEO, &(ui.winrend), WIN_W, WIN_H, "chess");
	UI_assign_defaults(&ui);

	loop(&ui);

	/* Destroy */
	ChessBot_destroy(ui.bot);
	Game_destroy(ui.game);
	Sprite_destroy(ui.pieces_spr);
	Font_destroy(ui.font);
	SDLUTIL_end(&(ui.winrend));
	return 0;
}
