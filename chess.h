#define WHITE_MOVE 0
#define BLACK_MOVE 1

#define MOVE_TITLE_SIZE 10

/* Can be, like, gigantic, because I'm gonna
 * store pointers and just create a new Game for
 * each new move. */
#define MOVIE_CAPACITY 600

/* Looking it up, no real life game has ever had more than
 * 99 moves possible at once so I'd say this is a safe bet.
 * Composed games, however, can go up past 400 possible so
 * if needed expand this to like 450. */
#define MAX_MOVES 100

/* Simplifying enums */
enum piece_t {      W_K, W_Q, W_R, W_N, W_B, W_P, /* White pieces */
					B_K, B_Q, B_R, B_N, B_B, B_P, /* Black pieces */
					EMT 						  /* Empty */ };
typedef enum piece_t ChessPiece;

typedef enum { NONE, QUEENSIDE, KINGSIDE, BOTH } CastlingRights;

typedef enum { PLAYING, WHITE, BLACK, DRAW } GameCondition;

/* STRUCTS */


/* POSITION struct: holds info on... position, such as who is to
 * move and castling rights, etc. Has all info to start a game from
 * any particular position (except for 3-fold rep, for now). */
typedef struct chess_pos_t {
	/* Who is to move (whose turn it is). Value is WHITE_MOVE if white, 
	 * BLACK_MOVE if black. */
	char to_move;
	/* Castling rights for white and black */
	CastlingRights castling_rights[2];
	/* Number of moves since last capture or pawn push. For
	 * 50-move rule */
	int halfmove_clock;
	/* Number of moves that have happened so far */
	int fullmove_clock;
	/* Square where pawn was double pushed last, or -1 
	 * if none. For en passant. */
	int en_passant_target;
	/* Saving king positions for ease with checking check */
	int white_kingsrc;
	int black_kingsrc;
	/* Size 64 array of every square and what piece occupies it
	 * (and EMT if no piece does). */
	ChessPiece piece_locations[64];
} Position;




/* MOVE struct: describes a move, its source and destination on the
 * board, and promotion details. The piece that is moving is inferred
 * using the position. */
typedef struct chess_move_t {
	int src;
	int dest;

	/* Usually -1 if not promoting to any piece. */
	ChessPiece promoting_to;

	/* 1 if move is en passant, else 0 */
	int is_en_passant;

	/* Short PGN title of move */
	char title[MOVE_TITLE_SIZE];
} Move;




/* GAME struct: holds position and possible moves. */
typedef struct chess_game_t {
	Position *current_pos;
	Move current_possible_moves[MAX_MOVES];
	/* Needed since we will likely store less than
	 * MAX_MOVES in the possible moves. */
	int num_possible_moves;
} ChessGame;


/* MOVIE struct: contains a collection of ChessGames,
 * one for each half-move, in an arraylist-esque way. */
typedef struct chess_movie_t {
	ChessGame *games[MOVIE_CAPACITY];
	int move_indices[MOVIE_CAPACITY];
	int current_turn;
	int length;
} Movie;









/* Position functions */

/* Allocate new position in memory. For now: set beginning position
 * to beginning chess game position (it sounds sensible). */
Position *Position_create();

/* Frees the position from the heap. */
void Position_destroy(Position *p);

/* Returns true (1) if square at column [col], row [row] is attacked
 * by the opposite color piece, else false (0). */
int Position_is_attacked(Position *p, int col, int row);

/* Returns true (1) if the player to move is in check in this 
 * position, else false (0). */
int Position_in_check(Position *p);





/* Move functions */

/* Set the "title" field of [move] given chess position [pos]. 
 * Short title means a short PGN title like "Nc3" or something. */
void Move_set_shorttitle(Move *move, ChessGame *game);

/* Returns true (1) if move is a capture, else false (0) */
int Move_is_capture(Move move, Position *pos);

/* Returns 1 if move is kingside castle, 2 if move is queenside castle,
 * 0 if no castle. Assumes that move is with the king. */
int Move_castle_type(Move move);







/* ChessGame functions */

/* Creates a ChessGame on heap at default starting move. */
ChessGame *Game_create();
/* Frees Game memory */
void Game_destroy(ChessGame *g);

/* Modifies Position with the move at [move_index]. Assumes
 * that move picked is legal. */
void Game_alter_position(ChessGame *g, Move altering_move);

/* Function to grab piece at row and column without typing up a crazy
 * expression */
ChessPiece Game_pieceat(ChessGame *g, int row, int col);

/* Finds all the legal moves in the game at the given position, and
 * stores them in the ChessGame's current_possible_moves array. */
void Game_find_all_legal_moves(ChessGame *g);

/* Returns the index of the legal move that is equal to [m], or
 * -1 if it does not exist (i.e. illegal move) */
int Game_get_legal(ChessGame *g, Move m);

/* Copies ChessGame [src] into [target]. */
void Game_copy(ChessGame *src, ChessGame *target);

/* Advances a turn in the game with requested move. Also refills legal
 * moves and returns if the game is over or not. */
GameCondition Game_advanceturn(ChessGame *g, Move m);
GameCondition Game_advanceturn_index(ChessGame *g, int move_index);

/* Parse file with FEN data and edit game accordingly.
 * Precondition: file is in proper FEN form. If it's not then
 * there's no protection against it and the board will probs be
 * invalid or exhibit undefined behavior. */
void Game_read_FEN(ChessGame *g, char *filename);



/* Movie Functions */

Movie *Movie_create();
void Movie_destroy(Movie *m);

/* Adds a copy of [game] to the movie list.
 * Returns 1 if successful, else 0. */
int Movie_add(Movie *movie, ChessGame *game);

/* Parses the PGN file from [filename] and 
 * fills up a new movie object with its positions. 
 * Cannot include comments or before tags, for now,
 * so remove those if copy-pasting from online. Also
 * does not work (for now) with move annotations like
 * ! and ?. */
Movie *Movie_create_from_PGN(char *filename);








/* Misc stuff */

/* Converts to and from PGN-style square like "e4" */
int pgn_to_rowcol(char *pgn);
