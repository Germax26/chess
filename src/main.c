#include <stdio.h>
#include <raylib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define CELL_WIDTH 80
#define CELL_HEIGHT 80
#define COLS 8
#define ROWS 8
#define SCREEN_WIDTH COLS*CELL_WIDTH
#define SCREEN_HEIGHT ROWS*CELL_HEIGHT
#define BOARD_LEN COLS*ROWS

#define COLOUR_BACKGROUND GetColor(0x151515FF)
#define COLOUR_BOARD_WHITE GetColor(0xF2E1C3FF)
#define COLOUR_BOARD_BLACK GetColor(0xC3A082FF)

#define SPRITE_WIDTH 480
#define SPRITE_HEIGHT 480
#define SPRITE_COLS 6
#define SPRITE_ROWS 2

Texture sprites_texture;

typedef struct {
	uint8_t x;
	uint8_t y;
} Pos;

bool pos_eq(Pos a, Pos b) {
	return a.x == b.x && a.y == b.y;
}

// Pieces

// should be in the same order as the sprite map (left to right)
typedef enum {
	TYPE_NONE,
	TYPE_KING,
	TYPE_QUEEN,
	TYPE_BISHOP,
	TYPE_KNIGHT,
	TYPE_ROOK,
	TYPE_PAWN
} Piece_Type;

// should be in the same order as the sprite map (top to bottom)
// OWNER_NONE should always be first
typedef enum {
	OWNER_NONE,
	OWNER_WHITE,
	OWNER_BLACK
} Piece_Owner;

Piece_Owner owner_next(Piece_Owner owner) {
	switch (owner) {
	case OWNER_NONE: return OWNER_NONE; // degenerate case
	case OWNER_WHITE: return OWNER_BLACK;
	case OWNER_BLACK: return OWNER_WHITE;
	}
}

int owner_direction(Piece_Owner owner) {
	switch (owner) {
	case OWNER_NONE: return 0;
	case OWNER_WHITE: return -1;
	case OWNER_BLACK: return 1;
	}
}

bool owner_is_pawn_promotable(Piece_Owner owner, Pos pos) {
	switch (owner) {
	case OWNER_NONE: return false;
	case OWNER_WHITE: return pos.y == 0;
	case OWNER_BLACK: return pos.y == ROWS-1;
	}
}

typedef struct {
	Piece_Type type;
	Piece_Owner owner;
} Piece;

// Board

// State

typedef enum {
	STATE_PREMOVE, // Selected a piece -> STATE_SELECTED
	STATE_SELECTED, // Cancel selection -> STATE_PREMOVE, Select a target -> STATE_PREMOVE, having moved piece (unless promoting)
	STATE_PROMOTION
} State_Kind;

typedef enum {
	SELECT_KIND_NONE = 0,
	SELECT_KIND_DEFAULT,
	SELECT_KIND_CASTLING,
	SELECT_KIND_PROMOTION,
	SELECT_KIND_DOUBLE_MOVE,
	SELECT_KIND_EN_PASSANT
} Selection_Kind;

typedef struct {
	uint8_t curr_col;
	int dir;
} Selection_Castling_Data;
typedef struct {
	Pos target;
	Pos pawn;
} DoubleMove_Data;

typedef struct {
	Selection_Kind kind;
	union {
		Selection_Castling_Data castling;
		DoubleMove_Data double_move;
	} data;
} Selection;

typedef struct {} Premove_Data;
typedef struct {
	Pos origin;
	Piece piece;
	Selection selections[BOARD_LEN];
} Selected_Data;
typedef struct {
	Pos pawn_pos;
} Promotion_Data;

#define SELECTION_AT(row, col) (game.state.data.selected.selections[(row)*COLS+(col)])
#define SWITCH_STATE(state, kind_) do { 			\
	(state).kind = kind_; 							\
	memset(&(state).data, 0, sizeof((state).data));	\
} while (0)

typedef struct {
	State_Kind kind;
	union {
		Premove_Data premove;
		Selected_Data selected;
		Promotion_Data promotion;
	} data;
} State;


typedef struct {
	State state;
	Piece board[BOARD_LEN];
	bool has_moved[BOARD_LEN];
	Piece_Owner turn;
	bool was_previous_move_double_move;
	DoubleMove_Data double_move;
} Game;
Game game;

#define BOARD_AT(row, col) (game.board[(row)*COLS+(col)])
#define HAS_MOVED_AT(row, col) (game.has_moved[(row)*COLS+(col)])

void set_piece(int row, int col, Piece_Type type, Piece_Owner owner) {
	BOARD_AT(row, col).type = type;
	BOARD_AT(row, col).owner = owner;
}

void reset_start_and_end_row(int row, Piece_Owner owner, uint8_t left_idx) {
	set_piece(row, left_idx+0, TYPE_ROOK, owner);
	set_piece(row, left_idx+1, TYPE_KNIGHT, owner);
	set_piece(row, left_idx+2, TYPE_BISHOP, owner);
	set_piece(row, left_idx+3, TYPE_QUEEN, owner);
	set_piece(row, left_idx+4, TYPE_KING, owner);
	set_piece(row, left_idx+5, TYPE_BISHOP, owner);
	set_piece(row, left_idx+6, TYPE_KNIGHT, owner);
	set_piece(row, left_idx+7, TYPE_ROOK, owner);
}

void reset_pawn_row(int row, Piece_Owner owner, uint8_t left_idx) {
	for (int col = 0; col < 8; ++col) set_piece(row, col+left_idx, TYPE_PAWN, owner);
}

void reset_game() {
	memset(&game, 0, sizeof(game));

	assert(COLS >= 8);
	uint8_t left_idx = (COLS-8)>>1;

	reset_start_and_end_row(0, OWNER_BLACK, left_idx);
	reset_pawn_row(1, OWNER_BLACK, left_idx);
	reset_pawn_row(ROWS-2, OWNER_WHITE, left_idx);
	reset_start_and_end_row(ROWS-1, OWNER_WHITE, left_idx);

	game.turn = OWNER_WHITE;
}

bool is_hovered(float x, float y, float w, float h) {
	Vector2 pos = GetMousePosition();
	return pos.x >= x && pos.x <= x+w && pos.y >= y && pos.y <= y+h;
}

bool piece_is_empty(Piece piece) {
	return piece.owner == OWNER_NONE || piece.type == TYPE_NONE;
}

void draw_piece(Piece piece, float x, float y) {
	if (piece.owner == OWNER_NONE || piece.type == TYPE_NONE) return;

	int sprite_col = piece.type - 1; // since TYPE_KING = 1, TYPE_QUEEN = 2, etc...
	int sprite_row = piece.owner - 1; // since OWNER_WHITE = 1, OWNER_BLACK = 2

	assert (sprite_col >= 0 && sprite_col < SPRITE_COLS);
	assert (sprite_row >= 0 && sprite_row < SPRITE_ROWS);

	/*
		Technically, the dest is offset from the square by a half of a cell in each direction,
		but the origin is set to that offset, so it cancels out. This also allows us to rotate
		the piece aroung the centre of the square, rather than the top-left corner.
	*/
	Rectangle source = {sprite_col*SPRITE_WIDTH, sprite_row*SPRITE_HEIGHT, SPRITE_WIDTH, SPRITE_HEIGHT};
	// Rectangle dest = {col*CELL_WIDTH + CELL_WIDTH/2, row*CELL_HEIGHT+CELL_HEIGHT/2, CELL_WIDTH, CELL_HEIGHT};
	Rectangle dest = {x, y, CELL_WIDTH, CELL_HEIGHT};
	DrawTexturePro(sprites_texture, source, dest, CLITERAL(Vector2){CELL_WIDTH/2, CELL_HEIGHT/2}, 0, WHITE);
}

void draw_border(Rectangle rect, Color colour) {
	DrawRectangleLinesEx(rect, 6, colour);
}

Rectangle cell_rect(int col, int row) {
	return CLITERAL(Rectangle){col*CELL_WIDTH, row*CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT};
}

Pos get_mouse_pos(Vector2 mouse_position) {
	return CLITERAL(Pos){(uint8_t)(mouse_position.x/CELL_WIDTH), (uint8_t)(mouse_position.y/CELL_HEIGHT)};
}

#define MOVEMENT(name) void movement_##name(Piece_Owner colour, uint8_t col, uint8_t row)
#define MOVEMENT_BY(name) void movement_##name(Piece_Owner colour, uint8_t col, uint8_t row, uint8_t distance)
#define MOVEMENT_WITH(name, ...) void movement_##name(Piece_Owner colour, uint8_t col, uint8_t row, uint8_t distance, __VA_ARGS__)

#define MAX(a, b) ((a) > (b)?  (a): (b))

bool is_valid_target(Piece_Owner colour, int8_t target_x, int8_t target_y) {
	if (!(0 <= target_x && target_x < COLS && 0 <= target_y && target_y < ROWS)) return false;
	return piece_is_empty(BOARD_AT(target_y, target_x)) || BOARD_AT(target_y, target_x).owner != colour;
}

MOVEMENT_WITH(line, int8_t left, int8_t up) {
	for (uint8_t i=1; i <= distance; ++i) {
		int8_t target_x = col+i*left, target_y = row+i*up;
		if (!is_valid_target(colour, target_x, target_y)) break;
		SWITCH_STATE(SELECTION_AT(target_y, target_x), SELECT_KIND_DEFAULT);
		if (!piece_is_empty(BOARD_AT(target_y, target_x))) break;
	}
}

MOVEMENT_WITH(lines, uint8_t left, uint8_t up) {
	movement_line(colour, col, row, distance, left, up);
	movement_line(colour, col, row, distance, -up, left);
	movement_line(colour, col, row, distance, -left, -up);
	movement_line(colour, col, row, distance, up, -left);
}

Selection_Kind pawn_selection(Piece_Owner colour, Pos target) {
	return owner_is_pawn_promotable(colour, target)? SELECT_KIND_PROMOTION: SELECT_KIND_DEFAULT;
}

MOVEMENT(pawn) {
	int dy = owner_direction(colour);
	for (int dx=-1; dx<=1; dx+=2) { // just does -1,1
		int8_t target_x = col+dx, target_y = row+dy;
		if (!is_valid_target(colour, target_x, target_y)) continue;
		Pos target = CLITERAL(Pos){target_x, target_y};

		if (!piece_is_empty(BOARD_AT(target_y, target_x))) {
			SWITCH_STATE(SELECTION_AT(target_y, target_x), pawn_selection(colour, target));
		} else if (game.was_previous_move_double_move && pos_eq(game.double_move.target, target)){
			SWITCH_STATE(SELECTION_AT(target_y, target_x), SELECT_KIND_EN_PASSANT);
		}
	}

	int8_t target_x = col, target_y = row+dy;
	if (!(is_valid_target(colour, target_x, target_y) && piece_is_empty(BOARD_AT(target_y, target_x)))) return;
	Pos target = CLITERAL(Pos){target_x, target_y};

	SWITCH_STATE(SELECTION_AT(target_y, target_x), pawn_selection(colour, target));

	if (HAS_MOVED_AT(row, col)) return;
	Pos single_move = target;

	target_y = row+2*dy;
	if (!(is_valid_target(colour, target_x, target_y) && piece_is_empty(BOARD_AT(target_y, target_x)))) return;
	target.y = target_y;

	SWITCH_STATE(SELECTION_AT(target_y, target_x), SELECT_KIND_DOUBLE_MOVE);
	SELECTION_AT(target_y, target_x).data.double_move = CLITERAL(DoubleMove_Data){single_move, target};
}

MOVEMENT_BY(bishop) {
	movement_lines(colour, col, row, distance, 1, 1);
}

MOVEMENT_BY(rook) {
	movement_lines(colour, col, row, distance, 1, 0);
}

MOVEMENT(knight) {
	movement_lines(colour, col, row, 1, 2, 1);
	movement_lines(colour, col, row, 1, 1, 2);
}

MOVEMENT_BY(queen) {
	movement_bishop(colour, col, row, distance);
	movement_rook(colour, col, row, distance);
}

MOVEMENT(king) {
	movement_queen(colour, col, row, 1);
	if (HAS_MOVED_AT(row, col)) return;
	for (int dir=-1; dir <= 1; dir+=2) {
		for (int i=2; i<COLS; ++i) {
			int8_t curr_col = col+dir*i;
			if (!(0 <= curr_col && curr_col < COLS)) break;
			Piece piece = BOARD_AT(row, curr_col);
			if (piece_is_empty(piece)) continue;
			if (piece.owner == colour && piece.type == TYPE_ROOK) {
				SWITCH_STATE(SELECTION_AT(row, col+2*dir), SELECT_KIND_CASTLING);
				SELECTION_AT(row, col+2*dir).data.castling = CLITERAL(Selection_Castling_Data){curr_col, dir};
			}
			break;
		}
	}
}

#define SELECTION(name) State selection_##name(uint8_t col, uint8_t row)
#define SELECTION_WITH(name, ...) State selection_##name(uint8_t col, uint8_t row, __VA_ARGS__)

SELECTION(default) {
	game.was_previous_move_double_move = false;
	memset(&game.double_move, 0, sizeof(game.double_move));
	HAS_MOVED_AT(row, col) = true;
	return CLITERAL(State){.kind = STATE_PREMOVE};
}

SELECTION(promotion) {
	(void)col;
	(void)row;
	assert(false && "Unimplemented - promotion");
}

SELECTION_WITH(double_move, DoubleMove_Data data) {
	State result = selection_default(col, row);
	game.was_previous_move_double_move = true;
	game.double_move = data;
	return result;
}

SELECTION(en_passant) {
	assert(game.was_previous_move_double_move && "Unreachable");
	Pos pawn = game.double_move.pawn;
	State result = selection_default(col, row);
	set_piece(pawn.y, pawn.x, TYPE_NONE, OWNER_NONE);
	return result;
}

SELECTION_WITH(castling, Selection_Castling_Data data) {
	selection_default(col-data.dir, row); // col is equal to col_origin+2*dir, and we want to calculate col_origin+dir
	State result = selection_default(col, row);
	BOARD_AT(row, col-data.dir) = BOARD_AT(row, data.curr_col);
	set_piece(row, data.curr_col, TYPE_NONE, OWNER_NONE);
	return result;
}

void draw_board() {
	for (int row = 0; row < ROWS; ++row) {
		for (int col = 0; col < COLS; ++col) {
			Color colour = (row + col) % 2 == 0 ? COLOUR_BOARD_WHITE : COLOUR_BOARD_BLACK;
			DrawRectangleRec(cell_rect(col, row), colour);
		}
	}

	for (int row = 0; row < ROWS; ++row) {
		for (int col = 0; col < COLS; ++col) {
			Piece piece = BOARD_AT(row, col);
			Rectangle rect = cell_rect(col, row);
			if (piece_is_empty(piece)) continue;
			draw_piece(piece, rect.x+CELL_WIDTH/2, rect.y+CELL_HEIGHT/2);
		}
	}

	if (game.state.kind == STATE_SELECTED) {
		draw_border(cell_rect(game.state.data.selected.origin.x, game.state.data.selected.origin.y), BLUE);
		for (int row = 0; row < ROWS; ++row) {
			for (int col = 0; col < COLS; ++col) {
				if (SELECTION_AT(row, col).kind != SELECT_KIND_NONE) draw_border(cell_rect(col, row), GREEN);
			}
		}
	}

	if (game.state.kind == STATE_PREMOVE || game.state.kind == STATE_SELECTED) {
		Vector2 mouse_position = GetMousePosition();
		Pos curr_pos = get_mouse_pos(mouse_position);
		draw_border(cell_rect(curr_pos.x, curr_pos.y), RED);
		if (game.state.kind == STATE_SELECTED) draw_piece(game.state.data.selected.piece, mouse_position.x, mouse_position.y);
		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			uint8_t col = curr_pos.x, row = curr_pos.y;
			if (game.state.kind == STATE_PREMOVE) {
				Piece piece = BOARD_AT(row, col);
				if (piece_is_empty(piece) || piece.owner != game.turn) return;
				set_piece(row, col, TYPE_NONE, OWNER_NONE);
				SWITCH_STATE(game.state, STATE_SELECTED);
				game.state.data.selected.origin = curr_pos;
				game.state.data.selected.piece = piece;
				// TODO: Check that these moves do not expose the king and check for any other rules that affect the legality of moves
				switch (piece.type) {
					case TYPE_NONE: assert(false && "Unreachable");
					case TYPE_KING:   { movement_king(piece.owner, col, row);                    break;}
					case TYPE_QUEEN:  { movement_queen(piece.owner, col, row, MAX(COLS, ROWS));  break;}
					case TYPE_BISHOP: { movement_bishop(piece.owner, col, row, MAX(COLS, ROWS)); break;}
					case TYPE_KNIGHT: { movement_knight(piece.owner, col, row);                  break;}
					case TYPE_ROOK:   { movement_rook(piece.owner, col, row, MAX(COLS, ROWS));   break;}
					case TYPE_PAWN:   { movement_pawn(piece.owner, col, row);                    break;}					
				}
			} else if (game.state.kind == STATE_SELECTED) {
				Selection selection = SELECTION_AT(row, col);
				if (selection.kind == SELECT_KIND_NONE) {
					if (pos_eq(game.state.data.selected.origin, curr_pos)) {
						BOARD_AT(row, col) = game.state.data.selected.piece;
						SWITCH_STATE(game.state, STATE_PREMOVE);
					}
					return;
				}
				BOARD_AT(row, col) =  game.state.data.selected.piece;
				switch (selection.kind) {
					case SELECT_KIND_NONE: assert(false && "Unreachable");
					case SELECT_KIND_DEFAULT:     { game.state = selection_default(col, row);                                 break; }
					case SELECT_KIND_DOUBLE_MOVE: { game.state = selection_double_move(col, row, selection.data.double_move); break; }
					case SELECT_KIND_EN_PASSANT:  { game.state = selection_en_passant(col, row);                              break; }
					case SELECT_KIND_PROMOTION:   { game.state = selection_promotion(col, row);                               break; }
					case SELECT_KIND_CASTLING:    { game.state = selection_castling(col, row, selection.data.castling);       break; }
				}
				game.turn = owner_next(game.turn);
			} else {
				assert(false && "Unreachable");
			}
		}
	}
}

int main() {
	SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chess");

	SetTargetFPS(60);
	SetExitKey(KEY_NULL);

	{
		// TODO: Bundle this with the executable, as currently the program won't work if you launch it in another directory that doesn't have the resources/sprites/pieces.png
		Image image = LoadImage("./resources/sprites/pieces.png");
		sprites_texture = LoadTextureFromImage(image);
		GenTextureMipmaps(&sprites_texture);
		SetTextureFilter(sprites_texture, TEXTURE_FILTER_BILINEAR);
	}

	reset_game();

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(COLOUR_BACKGROUND);
		draw_board();
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
