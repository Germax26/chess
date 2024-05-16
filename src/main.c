#include <stdio.h>
#include <raylib.h>
#include <string.h>
#include <assert.h>

#define CELL_WIDTH 80
#define CELL_HEIGHT 80
#define COLS 8
#define ROWS 8
#define SCREEN_WIDTH COLS*CELL_WIDTH
#define SCREEN_HEIGHT ROWS*CELL_HEIGHT

#define COLOUR_BACKGROUND GetColor(0x151515FF)
#define COLOUR_BOARD_WHITE GetColor(0xF2E1C3FF)
#define COLOUR_BOARD_BLACK GetColor(0xC3A082FF)

#define SPRITE_WIDTH 480
#define SPRITE_HEIGHT 480
#define SPRITE_COLS 6
#define SPRITE_ROWS 2

// Assets
Texture sprites_texture;

// Board

// should be in the same order as the sprite map (top to bottom)
// OWNER_NONE should always be last
typedef enum {
	OWNER_NONE,
	OWNER_WHITE,
	OWNER_BLACK
} Piece_Owner;

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

typedef struct {
	Piece_Type type;
	Piece_Owner owner;
} Piece;

#define BOARD_LEN COLS*ROWS
Piece board[BOARD_LEN];

#define BOARD_AT(row, col) (board[row*ROWS+col])

// State

typedef enum {
	STATE_PREMOVE, // Selected a piece -> STATE_SELECTED
	STATE_SELECTED, // Cancel selection -> STATE_REMOVE, Select a target -> STATE_MOVING (Most of the time)
	STATE_MOVING
} State_Kind;

typedef struct {} Premove_Data;
typedef struct {
	size_t selected;
} Selected_Data;
typedef struct {
	size_t origin;
	size_t target;
	float t;
} Moving_Data;

typedef struct {
	State_Kind state_kind;
	union {
		Premove_Data premove;
		Selected_Data selected;
		Moving_Data moving;
	} data;
} State;

State state;

void set_piece(int row, int col, Piece_Type type, Piece_Owner owner) {
	BOARD_AT(row, col).type = type;
	BOARD_AT(row, col).owner = owner;
}

void reset_start_and_end_row(int row, Piece_Owner owner) {
	set_piece(row, 0, TYPE_ROOK, owner);
	set_piece(row, 1, TYPE_KNIGHT, owner);
	set_piece(row, 2, TYPE_BISHOP, owner);
	set_piece(row, 3, TYPE_QUEEN, owner);
	set_piece(row, 4, TYPE_KING, owner);
	set_piece(row, 5, TYPE_BISHOP, owner);
	set_piece(row, 6, TYPE_KNIGHT, owner);
	set_piece(row, 7, TYPE_ROOK, owner);
}

void reset_pawn_row(int row, Piece_Owner owner) {
	for (int col = 0; col < COLS; ++col) set_piece(row, col, TYPE_PAWN, owner);
}

void reset_empty_row(int row) {
	for (int col = 0; col < COLS; ++col) set_piece(row, col, TYPE_NONE, OWNER_NONE);
}

void reset_board() {
	reset_start_and_end_row(0, OWNER_BLACK);
	reset_pawn_row(1, OWNER_BLACK);
	for (int row = 2; row < 6; ++row) reset_empty_row(row);
	reset_pawn_row(6, OWNER_WHITE);
	reset_start_and_end_row(7, OWNER_WHITE);
}

void transition_to_premove() {
	state.state_kind = STATE_PREMOVE;
	memset(&state.data, 0, sizeof(state.data));
}

void transition_to_selected(size_t index) {
	state.state_kind = STATE_SELECTED;
	state.data.selected = CLITERAL(Selected_Data){index};
}

bool is_hovered(float x, float y, float w, float h) {
	Vector2 pos = GetMousePosition();
	return pos.x >= x && pos.x <= x+w && pos.y >= y && pos.y <= y+h;
}

void draw_board() {
	for (int row = 0; row < 9; ++row) {
		for (int col = 0; col < 16; ++col) {
			Color colour = (row + col) % 2 == 0 ? COLOUR_BOARD_WHITE : COLOUR_BOARD_BLACK;
			Rectangle rect = {col*CELL_WIDTH, row*CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT};
			DrawRectangle(rect.x, rect.y, CELL_WIDTH, CELL_HEIGHT, colour);
			if(is_hovered(rect.x, rect.y, CELL_WIDTH, CELL_HEIGHT)) DrawRectangleLinesEx(rect, 5, RED);
		}
	}

	for (int row = 0; row < ROWS; ++row) {
		for (int col = 0; col < COLS; ++col) {
			Piece piece = BOARD_AT(row, col);
			if (piece.owner == OWNER_NONE || piece.type == TYPE_NONE) continue;

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
			Rectangle dest = {col*CELL_WIDTH + CELL_WIDTH/2, row*CELL_HEIGHT+CELL_HEIGHT/2, CELL_WIDTH, CELL_HEIGHT};
			DrawTexturePro(sprites_texture, source, dest, CLITERAL(Vector2){CELL_WIDTH/2, CELL_HEIGHT/2}, 0, WHITE);
		}
	}
}

int main() {
	SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chess");

	SetTargetFPS(60);
	SetExitKey(KEY_NULL);

	{
		Image image = LoadImage("./resources/sprites/pieces.png");
		sprites_texture = LoadTextureFromImage(image);
		GenTextureMipmaps(&sprites_texture);
		SetTextureFilter(sprites_texture, TEXTURE_FILTER_BILINEAR);
	}

	reset_board();
	transition_to_premove();
	
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(COLOUR_BACKGROUND);
		draw_board();
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
