// E == EMPTY, X = OFF BOARD, - == CANNOT HAPPEN
// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
// E  W  W  W  W  W  W  X  -  B  B  B  B  B  B  -
// E  P  N  B  R  Q  K  X  -  P  N  B  R  Q  K  -

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>  

#define U8 unsigned __int8
#define S16 signed __int16
#define U16 unsigned __int16
#define S32 signed __int32
#define U32 unsigned __int32
#define S64 signed __int64
#define U64 unsigned __int64
#define move_t unsigned __int32
#define FALSE 0
#define TRUE 1
#define MAX(a, bBoard) ((a) > (bBoard) ? (a) : (bBoard))
#define MIN(a, bBoard) ((a) < (bBoard) ? (a) : (bBoard))
#define MATE 32000
#define INF 32001
#define MAX_PLY 64
#define NAME "Toucan"
#define VERSION "2026-04-04"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define WHITE 0x0
#define BLACK 0x8

#define PIECE_MASK  0x7
#define COLOUR_MASK 0x8

#define PAWN   1
#define KNIGHT 2
#define BISHOP 3
#define ROOK   4
#define QUEEN  5
#define KING   6
#define EDGE   7

#define W_PAWN   PAWN
#define W_KNIGHT KNIGHT
#define W_BISHOP BISHOP
#define W_ROOK   ROOK
#define W_QUEEN  QUEEN
#define W_KING   KING

#define B_PAWN   (PAWN   | BLACK)
#define B_KNIGHT (KNIGHT | BLACK)
#define B_BISHOP (BISHOP | BLACK)
#define B_ROOK   (ROOK   | BLACK)
#define B_QUEEN  (QUEEN  | BLACK)
#define B_KING   (KING   | BLACK)

#define WHITE_RIGHTS_KING  0x1
#define WHITE_RIGHTS_QUEEN 0x2
#define BLACK_RIGHTS_KING  0x4
#define BLACK_RIGHTS_QUEEN 0x8

#define WHITE_RIGHTS  (WHITE_RIGHTS_QUEEN | WHITE_RIGHTS_KING)
#define BLACK_RIGHTS  (BLACK_RIGHTS_QUEEN | BLACK_RIGHTS_KING)

#define W_OFFSET_ORTH  -12
#define W_OFFSET_DIAG1 -13
#define W_OFFSET_DIAG2 -11

#define B_OFFSET_ORTH  12
#define B_OFFSET_DIAG1 13
#define B_OFFSET_DIAG2 11

#define TT_SIZE  (1 << 20)
#define TT_MASK  ((TT_SIZE) - 1)

#define MAX_MOVES 256

#define ALL_MOVES   0
#define NOISY_MOVES 1

#define MOVE_TO_BITS      0
#define MOVE_FR_BITS      8
#define MOVE_TOOBJ_BITS   16
#define MOVE_FROBJ_BITS   20
#define MOVE_PROMAS_BITS  29

#define MOVE_TO_MASK       0x000000FF
#define MOVE_FR_MASK       0x0000FF00
#define MOVE_TOOBJ_MASK    0x000F0000
#define MOVE_FROBJ_MASK    0x00F00000
#define MOVE_KINGMOVE_MASK 0x01000000
#define MOVE_EPTAKE_MASK   0x02000000
#define MOVE_EPMAKE_MASK   0x04000000
#define MOVE_CASTLE_MASK   0x08000000
#define MOVE_PROMOTE_MASK  0x10000000
#define MOVE_PROMAS_MASK   0x60000000
#define MOVE_SPARE_MASK    0x80000000

#define MOVE_CAPTURE_MASK (MOVE_TOOBJ_MASK | MOVE_EPTAKE_MASK)
#define MOVE_IKKY_MASK    (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | MOVE_PROMOTE_MASK | MOVE_EPTAKE_MASK | MOVE_EPMAKE_MASK)
#define MOVE_DRAW_MASK    (MOVE_TOOBJ_MASK | MOVE_PROMOTE_MASK | MOVE_EPTAKE_MASK)

#define MOVE_E1G1 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (W_KING << MOVE_FROBJ_BITS) | (SQ_E1 << MOVE_FR_BITS) | SQ_G1)
#define MOVE_E1C1 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (W_KING << MOVE_FROBJ_BITS) | (SQ_E1 << MOVE_FR_BITS) | SQ_C1)
#define MOVE_E8G8 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (B_KING << MOVE_FROBJ_BITS) | (SQ_E8 << MOVE_FR_BITS) | SQ_G8)
#define MOVE_E8C8 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (B_KING << MOVE_FROBJ_BITS) | (SQ_E8 << MOVE_FR_BITS) | SQ_C8)

#define QPRO (((QUEEN-KNIGHT)  << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)
#define RPRO (((ROOK-KNIGHT)   << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)
#define BPRO (((BISHOP-KNIGHT) << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)
#define NPRO (((KNIGHT-KNIGHT) << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)

#define FLIP64(sq) ((sq)^56)

enum Bound { UPPER, LOWER, EXACT };

typedef struct {
	move_t moves[MAX_MOVES];
	int ranks[MAX_MOVES];
	int movesNum;
	int moveNext;
	int noisy;
}MoveList;

typedef struct {
	U64 hash;
	int move;
	int score;
	int depth;
	U8 flag;
}TTEntry;

typedef struct {
	U8 post;
	U8 stop;
	int depthLimit;
	U64 timeStart;
	U64 timeLimit;
	U64 nodes;
	U64 nodesLimit;
}SearchInfo;

typedef struct {
	int bRights;
	int bEP;
	int move50;
}SCache;

move_t tBestMove = 0;

const int KNIGHT_OFFSETS[] = { 25,-25,23,-23,14,-14,10,-10 };
const int BISHOP_OFFSETS[] = { 11,-11,13,-13 };
const int ROOK_OFFSETS[] = { 1,-1,12,-12 };
const int QUEEN_OFFSETS[] = { 11,-11,13,-13,1,-1,12,-12 };
const int KING_OFFSETS[] = { 11,-11,13,-13,1,-1,12,-12 };

const int IS_O[] = { 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0 };
const int IS_E[] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_OE[] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0 };

const int IS_P[] = { 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 };
const int IS_N[] = { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 };
const int IS_NBRQ[] = { 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 };
const int IS_NBRQKE[] = { 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0 };
const int IS_RQKE[] = { 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0 };
const int IS_Q[] = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 };
const int IS_QKE[] = { 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0 };
const int IS_K[] = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
const int IS_KN[] = { 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0 };
const int IS_SLIDER[] = { 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0 };

const int IS_W[] = { 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WE[] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WP[] = { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WN[] = { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WNBRQ[] = { 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WPNBRQ[] = { 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WPNBRQE[] = { 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WB[] = { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WR[] = { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WBQ[] = { 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WRQ[] = { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WQ[] = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const int IS_WK[] = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const int IS_B[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0 };
const int IS_BE[] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0 };
const int IS_BP[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 };
const int IS_BN[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 };
const int IS_BNBRQ[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 };
const int IS_BPNBRQ[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0 };
const int IS_BPNBRQE[] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0 };
const int IS_BB[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 };
const int IS_BR[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };
const int IS_BBQ[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 };
const int IS_BRQ[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 };
const int IS_BQ[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 };
const int IS_BK[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 };

const int* WB_CAN_CAPTURE[] = { IS_BPNBRQ, IS_WPNBRQ };
const int* WB_OUR_PIECE[] = { IS_W,      IS_B };
const int* WB_RQ[] = { IS_WRQ,    IS_BRQ };
const int* WB_BQ[] = { IS_WBQ,    IS_BBQ };

const int WB_PAWN[] = { W_PAWN, B_PAWN };

const int WB_OFFSET_ORTH[] = { W_OFFSET_ORTH,  B_OFFSET_ORTH };
const int WB_OFFSET_DIAG1[] = { W_OFFSET_DIAG1, B_OFFSET_DIAG1 };
const int WB_OFFSET_DIAG2[] = { W_OFFSET_DIAG2, B_OFFSET_DIAG2 };

const int WB_HOME_RANK[] = { 2, 7 };
const int WB_PROMOTE_RANK[] = { 7, 2 };
const int WB_EP_RANK[] = { 5, 4 };

const char OBJ_CHAR[] = { ' ','A','N','B','R','Q','K',' ',' ','a','n','b','r','q','k',' ' };

const int MASK_RIGHTS[144] = { 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
							  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
							  15, 15, ~8, 15, 15, 15, ~12,15, 15, ~4, 15, 15,
							  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
							  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
							  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
							  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
							  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
							  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
							  15, 15, ~2, 15, 15, 15, ~3, 15, 15, ~1, 15, 15,
							  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
							  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 };


const int B88[] = { 26, 27, 28, 29, 30, 31, 32, 33,
					38, 39, 40, 41, 42, 43, 44, 45,
					50, 51, 52, 53, 54, 55, 56, 57,
					62, 63, 64, 65, 66, 67, 68, 69,
					74, 75, 76, 77, 78, 79, 80, 81,
					86, 87, 88, 89, 90, 91, 92, 93,
					98, 99,100,101,102,103,104,105,
				   110,111,112,113,114,115,116,117 };

enum Square {
	SQ_A8 = 26, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
	SQ_A7 = 38, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
	SQ_A6 = 50, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
	SQ_A5 = 62, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
	SQ_A4 = 74, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
	SQ_A3 = 86, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
	SQ_A2 = 98, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
	SQ_A1 = 110, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
	SQ_NB
};

const char* const COORDS[] = { "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??",
							   "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??",
							   "??", "??", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "??", "??",
							   "??", "??", "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "??", "??",
							   "??", "??", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "??", "??",
							   "??", "??", "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "??", "??",
							   "??", "??", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "??", "??",
							   "??", "??", "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "??", "??",
							   "??", "??", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "??", "??",
							   "??", "??", "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "??", "??",
							   "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??",
							   "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??" };

const int bRank[] = {
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0,
					0, 0, 7, 7, 7, 7, 7, 7, 7, 7, 0, 0,
					0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0,
					0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0,
					0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0,
					0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
					0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0,
					0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const int bFile[] = {
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
					0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
					0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
					0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
					0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
					0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
					0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
					0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const int bCentre[] = {
					  0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0,
					  0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0,
					  0, 0, 1, 2,  3,  4,  4,  3,  2,  1, 0, 0,
					  0, 0, 2, 6,  8,  10, 10, 8,  6,  2, 0, 0,
					  0, 0, 3, 8,  15, 18, 18, 15, 8,  3, 0, 0,
					  0, 0, 4, 10, 18, 28, 28, 18, 10, 4, 0, 0,
					  0, 0, 4, 10, 18, 28, 28, 19, 10, 4, 0, 0,
					  0, 0, 3, 8,  15, 18, 18, 15, 8,  3, 0, 0,
					  0, 0, 2, 6,  8,  10, 10, 8,  6,  2, 0, 0,
					  0, 0, 1, 2,  3,  4,  4,  3,  2,  1, 0, 0,
					  0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0,
					  0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0 };


int mg_value[8] = { 0,82, 337, 365, 477, 1025,  0,0 };
int eg_value[8] = { 0,94, 281, 297, 512,  936,  0,0 };

int mg_pawn_table[64] = {
	  0,   0,   0,   0,   0,   0,  0,   0,
	 98, 134,  61,  95,  68, 126, 34, -11,
	 -6,   7,  26,  31,  65,  56, 25, -20,
	-14,  13,   6,  21,  23,  12, 17, -23,
	-27,  -2,  -5,  12,  17,   6, 10, -25,
	-26,  -4,  -4, -10,   3,   3, 33, -12,
	-35,  -1, -20, -23, -15,  24, 38, -22,
	  0,   0,   0,   0,   0,   0,  0,   0,
};

int eg_pawn_table[64] = {
	  0,   0,   0,   0,   0,   0,   0,   0,
	178, 173, 158, 134, 147, 132, 165, 187,
	 94, 100,  85,  67,  56,  53,  82,  84,
	 32,  24,  13,   5,  -2,   4,  17,  17,
	 13,   9,  -3,  -7,  -7,  -8,   3,  -1,
	  4,   7,  -6,   1,   0,  -5,  -1,  -8,
	 13,   8,   8,  10,  13,   0,   2,  -7,
	  0,   0,   0,   0,   0,   0,   0,   0,
};

int mg_knight_table[64] = {
	-167, -89, -34, -49,  61, -97, -15, -107,
	 -73, -41,  72,  36,  23,  62,   7,  -17,
	 -47,  60,  37,  65,  84, 129,  73,   44,
	  -9,  17,  19,  53,  37,  69,  18,   22,
	 -13,   4,  16,  13,  28,  19,  21,   -8,
	 -23,  -9,  12,  10,  19,  17,  25,  -16,
	 -29, -53, -12,  -3,  -1,  18, -14,  -19,
	-105, -21, -58, -33, -17, -28, -19,  -23,
};

int eg_knight_table[64] = {
	-58, -38, -13, -28, -31, -27, -63, -99,
	-25,  -8, -25,  -2,  -9, -25, -24, -52,
	-24, -20,  10,   9,  -1,  -9, -19, -41,
	-17,   3,  22,  22,  22,  11,   8, -18,
	-18,  -6,  16,  25,  16,  17,   4, -18,
	-23,  -3,  -1,  15,  10,  -3, -20, -22,
	-42, -20, -10,  -5,  -2, -20, -23, -44,
	-29, -51, -23, -15, -22, -18, -50, -64,
};

int mg_bishop_table[64] = {
	-29,   4, -82, -37, -25, -42,   7,  -8,
	-26,  16, -18, -13,  30,  59,  18, -47,
	-16,  37,  43,  40,  35,  50,  37,  -2,
	 -4,   5,  19,  50,  37,  37,   7,  -2,
	 -6,  13,  13,  26,  34,  12,  10,   4,
	  0,  15,  15,  15,  14,  27,  18,  10,
	  4,  15,  16,   0,   7,  21,  33,   1,
	-33,  -3, -14, -21, -13, -12, -39, -21,
};

int eg_bishop_table[64] = {
	-14, -21, -11,  -8, -7,  -9, -17, -24,
	 -8,  -4,   7, -12, -3, -13,  -4, -14,
	  2,  -8,   0,  -1, -2,   6,   0,   4,
	 -3,   9,  12,   9, 14,  10,   3,   2,
	 -6,   3,  13,  19,  7,  10,  -3,  -9,
	-12,  -3,   8,  10, 13,   3,  -7, -15,
	-14, -18,  -7,  -1,  4,  -9, -15, -27,
	-23,  -9, -23,  -5, -9, -16,  -5, -17,
};

int mg_rook_table[64] = {
	 32,  42,  32,  51, 63,  9,  31,  43,
	 27,  32,  58,  62, 80, 67,  26,  44,
	 -5,  19,  26,  36, 17, 45,  61,  16,
	-24, -11,   7,  26, 24, 35,  -8, -20,
	-36, -26, -12,  -1,  9, -7,   6, -23,
	-45, -25, -16, -17,  3,  0,  -5, -33,
	-44, -16, -20,  -9, -1, 11,  -6, -71,
	-19, -13,   1,  17, 16,  7, -37, -26,
};

int eg_rook_table[64] = {
	13, 10, 18, 15, 12,  12,   8,   5,
	11, 13, 13, 11, -3,   3,   8,   3,
	 7,  7,  7,  5,  4,  -3,  -5,  -3,
	 4,  3, 13,  1,  2,   1,  -1,   2,
	 3,  5,  8,  4, -5,  -6,  -8, -11,
	-4,  0, -5, -1, -7, -12,  -8, -16,
	-6, -6,  0,  2, -9,  -9, -11,  -3,
	-9,  2,  3, -1, -5, -13,   4, -20,
};

int mg_queen_table[64] = {
	-28,   0,  29,  12,  59,  44,  43,  45,
	-24, -39,  -5,   1, -16,  57,  28,  54,
	-13, -17,   7,   8,  29,  56,  47,  57,
	-27, -27, -16, -16,  -1,  17,  -2,   1,
	 -9, -26,  -9, -10,  -2,  -4,   3,  -3,
	-14,   2, -11,  -2,  -5,   2,  14,   5,
	-35,  -8,  11,   2,   8,  15,  -3,   1,
	 -1, -18,  -9,  10, -15, -25, -31, -50,
};

int eg_queen_table[64] = {
	 -9,  22,  22,  27,  27,  19,  10,  20,
	-17,  20,  32,  41,  58,  25,  30,   0,
	-20,   6,   9,  49,  47,  35,  19,   9,
	  3,  22,  24,  45,  57,  40,  57,  36,
	-18,  28,  19,  47,  31,  34,  39,  23,
	-16, -27,  15,   6,   9,  17,  10,   5,
	-22, -23, -30, -16, -16, -23, -36, -32,
	-33, -28, -22, -43,  -5, -32, -20, -41,
};

int mg_king_table[64] = {
	-65,  23,  16, -15, -56, -34,   2,  13,
	 29,  -1, -20,  -7,  -8,  -4, -38, -29,
	 -9,  24,   2, -16, -20,   6,  22, -22,
	-17, -20, -12, -27, -30, -25, -14, -36,
	-49,  -1, -27, -39, -46, -44, -33, -51,
	-14, -14, -22, -46, -44, -30, -15, -27,
	  1,   7,  -8, -64, -43, -16,   9,   8,
	-15,  36,  12, -54,   8, -28,  24,  14,
};

int eg_king_table[64] = {
	-74, -35, -18, -18, -11,  15,   4, -17,
	-12,  17,  14,  17,  17,  38,  23,  11,
	 10,  17,  23,  15,  20,  45,  44,  13,
	 -8,  22,  24,  27,  26,  33,  26,   3,
	-18,  -4,  21,  24,  27,  23,   9, -11,
	-19,  -3,  11,  21,  23,  16,   7,  -9,
	-27, -11,   4,  13,  14,   4,  -5, -17,
	-53, -34, -21, -11, -28, -14, -24, -43
};

int* mg_pesto_table[8] =
{
	mg_pawn_table,
	mg_pawn_table,
	mg_knight_table,
	mg_bishop_table,
	mg_rook_table,
	mg_queen_table,
	mg_king_table,
	mg_pawn_table
};

int* eg_pesto_table[8] =
{
	eg_pawn_table,
	eg_pawn_table,
	eg_knight_table,
	eg_bishop_table,
	eg_rook_table,
	eg_queen_table,
	eg_king_table,
	eg_pawn_table
};

int pstMg[0xf][64];
int pstEg[0xf][64];

int BPAWN_PST[144];
int BKNIGHT_PST[144];
int BBISHOP_PST[144];
int BROOK_PST[144];
int BQUEEN_PST[144];
int BKING_MID_PST[144];
int BKING_END_PST[144];


int bBoard[144];
int bTurn = 0;
int bRights = 0;
int bEP = 0;
int bMove50 = 0;
int bKings[2] = { 0,0 };

U64 keys[16 * 144];
int historyCount = 0;
U64 historyHash[1024];
TTEntry  tt[TT_SIZE];
SearchInfo info;

static U64 Rand64(void) {
	static U64 s = 1070372631ULL;
	s ^= s >> 12;
	s ^= s << 25;
	s ^= s >> 27;
	return s * 0x2545F4914F6CDD1DULL;
}

static inline U64 GetTimeMs() {
	return (U64)GetTickCount64();
}

static int CheckUp() {
	if ((++info.nodes & 0xffff) == 0) {
		if (info.timeLimit && GetTimeMs() - info.timeStart > info.timeLimit)
			info.stop = 1;
		if (info.nodesLimit && info.nodes > info.nodesLimit)
			info.stop = 1;
	}
	return info.stop;
}

static char* ParseToken(char* string, char* token) {
	while (*string == ' ')
		string++;
	while (*string != ' ' && *string != '\0' && *string != '\n')
		*token++ = *string++;
	*token = '\0';
	return string;
}

static void InitHash() {
	for (int i = 0; i < 16 * 144; i++)
		keys[i] = Rand64();
}

static U64 GetHash() {
	U64 hash = bTurn;
	for (int i = 0; i < 64; i++) {
		const int sq = B88[i];
		const int piece = bBoard[sq];
		if (piece)
			hash ^= keys[piece * 144 + sq];
	}
	hash ^= keys[bRights];
	hash ^= keys[7 * 144 + bEP];
	return hash;
}

static int Permill() {
	int pm = 0;
	for (int n = 0; n < 1000; n++)
		if (tt[n].hash)
			pm++;
	return pm;
}

static int IsRepetition(U64 hash) {
	int limit = max(0, historyCount - bMove50);
	for (int n = historyCount - 4; n >= limit; n -= 2) {
		if (historyHash[n] == hash)
			return TRUE;
	}
	return FALSE;
}

static void TTClear() {
	memset(tt, 0, sizeof(tt));
}

int objColour(int obj) {
	return obj & COLOUR_MASK;
}

int objPiece(int obj) {
	return obj & PIECE_MASK;
}

static int ColourIndex(int c) {
	return c >> 3;
}


static int ColourToggle(int c) {
	return c ^ COLOUR_MASK;
}

static void PrintBoard() {
	const char* s = "   +---+---+---+---+---+---+---+---+\n";
	const char* t = "     A   B   C   D   E   F   G   H\n";
	printf(t);
	for (int rank = 7; rank >= 0; rank--) {
		printf(s);
		printf(" %d |", rank + 1);
		for (int file = 0; file <= 7; file++) {
			printf(" %c |", OBJ_CHAR[bBoard[B88[(7 - rank) * 8 + file]]]);
		}
		printf(" %d \n", rank + 1);
	}
	printf(s);
	printf(t);

	if (bTurn == WHITE)
		printf("w");
	else
		printf("b");
	printf(" ");

	if (bRights) {
		if (bRights & WHITE_RIGHTS_KING)
			printf("K");
		if (bRights & WHITE_RIGHTS_QUEEN)
			printf("Q");
		if (bRights & BLACK_RIGHTS_KING)
			printf("k");
		if (bRights & BLACK_RIGHTS_QUEEN)
			printf("q");
		printf(" ");
	}
	else
		printf("- ");

	if (bEP)
		printf("%s", COORDS[bEP]);
	else
		printf("-");

	printf("\n");

	printf("hash : %16llx\n", GetHash());
}

static int Distance(int sq1, int sq2) {
	const int dx = abs(bFile[sq1] - bFile[sq2]);
	const int dy = abs(bRank[sq1] - bRank[sq2]);
	return dx > dy ? dx : dy;
}

static int IsSquareAttacked(int sq, int byCol) {
	const int cx = ColourIndex(byCol);
	const int OFFSET_DIAG1 = -WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = -WB_OFFSET_DIAG2[cx];
	const int BY_PAWN = WB_PAWN[cx];
	const int* RQ = WB_RQ[cx];
	const int* BQ = WB_BQ[cx];
	const int N = KNIGHT | byCol;
	int fr = 0;
	if (bBoard[sq + OFFSET_DIAG1] == BY_PAWN || bBoard[sq + OFFSET_DIAG2] == BY_PAWN)
		return 1;
	if ((bBoard[sq + -10] == N) ||
		(bBoard[sq + -23] == N) ||
		(bBoard[sq + -14] == N) ||
		(bBoard[sq + -25] == N) ||
		(bBoard[sq + 10] == N) ||
		(bBoard[sq + 23] == N) ||
		(bBoard[sq + 14] == N) ||
		(bBoard[sq + 25] == N)) return 1;
	fr = sq + 1;  while (!bBoard[fr]) fr += 1;  if (RQ[bBoard[fr]]) return 1;
	fr = sq - 1;  while (!bBoard[fr]) fr -= 1;  if (RQ[bBoard[fr]]) return 1;
	fr = sq + 12; while (!bBoard[fr]) fr += 12; if (RQ[bBoard[fr]]) return 1;
	fr = sq - 12; while (!bBoard[fr]) fr -= 12; if (RQ[bBoard[fr]]) return 1;
	fr = sq + 11; while (!bBoard[fr]) fr += 11; if (BQ[bBoard[fr]]) return 1;
	fr = sq - 11; while (!bBoard[fr]) fr -= 11; if (BQ[bBoard[fr]]) return 1;
	fr = sq + 13; while (!bBoard[fr]) fr += 13; if (BQ[bBoard[fr]]) return 1;
	fr = sq - 13; while (!bBoard[fr]) fr -= 13; if (BQ[bBoard[fr]]) return 1;
	if (Distance(sq, bKings[cx]) == 1) return 1;
	return 0;
}

static int EvalPosition() {
	int mgScore = 0;
	int egScore = 0;
	int mgPhase = 0;
	int phases[] = { 0, 0, 1, 1, 2, 4, 0, 0,
					 0, 0, 1, 1, 2, 4, 0, 0 };
	for (int sq = 0; sq < 64; sq++) {
		const int fr = B88[sq];
		const int piece = bBoard[fr];
		if (!piece)
			continue;
		mgPhase += phases[piece];
		mgScore += pstMg[piece][sq];
		egScore += pstEg[piece][sq];
	}
	if (mgPhase > 24) mgPhase = 24;
	int egPhase = 24 - mgPhase;
	int score = ((mgScore * mgPhase + egScore * egPhase) * (100 - bMove50)) / (24 * 100);
	return bTurn == WHITE ? score : -score;
}

static int FlipSq(int sq) {
	const int m = (143 - sq) / 12;
	return 12 * m + sq % 12;
}

static void InitEval() {
	for (int piece = PAWN; piece <= KING; piece++)
		for (int sq = 0; sq < 64; sq++) {
			pstMg[piece][sq] = mg_value[piece] + mg_pesto_table[piece][sq];
			pstEg[piece][sq] = eg_value[piece] + eg_pesto_table[piece][sq];
			pstMg[piece | BLACK][sq] = -mg_value[piece] - mg_pesto_table[piece][FLIP64(sq)];
			pstEg[piece | BLACK][sq] = -eg_value[piece] - eg_pesto_table[piece][FLIP64(sq)];
		}
}

static void CacheWrite(SCache* c) {
	c->bRights = bRights;
	c->bEP = bEP;
	c->move50 = bMove50;
}

static void CacheRead(SCache* c) {
	bRights = c->bRights;
	bEP = c->bEP;
	bMove50 = c->move50;
}

int moveFromSq(move_t move) {
	return (int)((move & MOVE_FR_MASK) >> MOVE_FR_BITS);
}

int moveToSq(move_t move) {
	return (int)((move & MOVE_TO_MASK) >> MOVE_TO_BITS);
}

int moveToObj(move_t move) {
	return (int)((move & MOVE_TOOBJ_MASK) >> MOVE_TOOBJ_BITS);
}

int moveFromObj(move_t move) {
	return (int)((move & MOVE_FROBJ_MASK) >> MOVE_FROBJ_BITS);
}

int movePromotePiece(move_t move) {
	return (int)(((move & MOVE_PROMAS_MASK) >> MOVE_PROMAS_BITS) + 2);
}

static int AddTT(MoveList* ml, move_t move) {
	for (int n = 0; n < ml->movesNum; n++)
		if (ml->moves[n] == move) {
			ml->ranks[n] = 32000;
			return 1;
		}
	return 0;
}

static void AddQuiet(MoveList* ml, move_t move) {
	if (ml->noisy)
		return;
	ml->moves[ml->movesNum] = move;
	ml->ranks[ml->movesNum] = 100 + bCentre[moveToSq(move)] - bCentre[moveFromSq(move)];
	ml->movesNum++;
}

const int RANK_ATTACKER[] = { 0,    600,  500,  400,  300,  200,  100, 0, 0,    600,  500,  400,  300,  200,  100 };
const int RANK_DEFENDER[] = { 2000, 1000, 3000, 3000, 5000, 9000, 0,   0, 2000, 1000, 3000, 3000, 5000, 9000, 0 };

static void AddNoisy(MoveList* ml, move_t move) {
	ml->moves[ml->movesNum] = move;
	ml->ranks[ml->movesNum] = RANK_ATTACKER[moveFromObj(move)] + RANK_DEFENDER[moveToObj(move)];
	ml->movesNum++;
}

static move_t GetNextMove(MoveList* ml) {
	if (ml->moveNext >= ml->movesNum)
		return 0;
	int bstIndex = ml->moveNext;
	int bstRank = ml->ranks[bstIndex];
	for (int n = ml->moveNext + 1; n < ml->movesNum; n++)
		if (ml->ranks[n] > bstRank) {
			bstIndex = n;
			bstRank = ml->ranks[n];
		}
	int bstMove = ml->moves[bstIndex];
	ml->moves[bstIndex] = ml->moves[ml->moveNext];
	ml->ranks[bstIndex] = ml->ranks[ml->moveNext];
	//ml->moves[ml->moveNext] = bstMove;
	//ml->ranks[ml->moveNext] = bstRank;
	ml->moveNext++;
	return bstMove;
}

static void GenWhiteCastlingMoves(MoveList* ml) {

	int* b = bBoard;

	if ((bRights & WHITE_RIGHTS_KING) && !b[SQ_F1]
		&& !b[SQ_G1]
		&& b[SQ_G2] != B_KING
		&& b[SQ_H2] != B_KING
		&& !IsSquareAttacked(SQ_E1, BLACK)
		&& !IsSquareAttacked(SQ_F1, BLACK)) {
		AddQuiet(ml, MOVE_E1G1);
	}

	if ((bRights & WHITE_RIGHTS_QUEEN) && !b[SQ_B1]
		&& !b[SQ_C1]
		&& !b[SQ_D1]
		&& b[SQ_B2] != B_KING
		&& b[SQ_C2] != B_KING
		&& !IsSquareAttacked(SQ_E1, BLACK)
		&& !IsSquareAttacked(SQ_D1, BLACK)) {
		AddQuiet(ml, MOVE_E1C1);
	}
}

static void GenBlackCastlingMoves(MoveList* ml) {

	int* b = bBoard;

	if ((bRights & BLACK_RIGHTS_KING) && b[SQ_F8] == 0
		&& b[SQ_G8] == 0
		&& b[SQ_G7] != W_KING
		&& b[SQ_H7] != W_KING
		&& !IsSquareAttacked(SQ_E8, WHITE)
		&& !IsSquareAttacked(SQ_F8, WHITE)) {
		AddQuiet(ml, MOVE_E8G8);
	}

	if ((bRights & BLACK_RIGHTS_QUEEN) && b[SQ_B8] == 0
		&& b[SQ_C8] == 0
		&& b[SQ_D8] == 0
		&& b[SQ_B7] != W_KING
		&& b[SQ_C7] != W_KING
		&& !IsSquareAttacked(SQ_E8, WHITE)
		&& !IsSquareAttacked(SQ_D8, WHITE)) {
		AddQuiet(ml, MOVE_E8C8);
	}
}

static void GenPawnMoves(MoveList* ml, move_t frMove) {
	const int fr = moveFromSq(frMove);
	const int cx = ColourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];
	const int OFFSET_ORTH = WB_OFFSET_ORTH[cx];
	const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];
	int to, toObj;
	to = fr + OFFSET_ORTH;
	if (!bBoard[to])
		AddQuiet(ml, frMove | to);
	to = fr + OFFSET_DIAG1;
	toObj = bBoard[to];
	if (CAN_CAPTURE[toObj])
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
	to = fr + OFFSET_DIAG2;
	toObj = bBoard[to];
	if (CAN_CAPTURE[toObj])
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
}

static void GenEnPassPawnMoves(MoveList* ml, move_t frMove) {
	const int fr = moveFromSq(frMove);
	const int cx = ColourIndex(bTurn);
	const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];
	int to;
	to = fr + OFFSET_DIAG1;
	if (to == bEP && !bBoard[to])
		AddNoisy(ml, frMove | to | MOVE_EPTAKE_MASK);
	to = fr + OFFSET_DIAG2;
	if (to == bEP && !bBoard[to])
		AddNoisy(ml, frMove | to | MOVE_EPTAKE_MASK);
}

static void GenHomePawnMoves(MoveList* ml, move_t frMove) {
	const int fr = moveFromSq(frMove);
	const int cx = ColourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];
	const int OFFSET_ORTH = WB_OFFSET_ORTH[cx];
	const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];
	int to, toObj;
	to = fr + OFFSET_ORTH;
	if (!bBoard[to]) {
		AddQuiet(ml, frMove | to);
		to += OFFSET_ORTH;
		if (!bBoard[to])
			AddQuiet(ml, frMove | to | MOVE_EPMAKE_MASK);
	}
	to = fr + OFFSET_DIAG1;
	toObj = bBoard[to];
	if (CAN_CAPTURE[toObj])
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
	to = fr + OFFSET_DIAG2;
	toObj = bBoard[to];
	if (CAN_CAPTURE[toObj])
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
}

static void GenPromotePawnMoves(MoveList* ml, move_t frMove) {
	const int fr = moveFromSq(frMove);
	const int cx = ColourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];
	const int OFFSET_ORTH = WB_OFFSET_ORTH[cx];
	const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

	int to, toObj;

	to = fr + OFFSET_ORTH;
	if (!bBoard[to]) {
		AddQuiet(ml, frMove | to | QPRO);
		AddQuiet(ml, frMove | to | RPRO);
		AddQuiet(ml, frMove | to | BPRO);
		AddQuiet(ml, frMove | to | NPRO);
	}

	to = fr + OFFSET_DIAG1;
	toObj = bBoard[to];
	if (CAN_CAPTURE[toObj]) {
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | QPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | RPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | BPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | NPRO);
	}

	to = fr + OFFSET_DIAG2;
	toObj = bBoard[to];
	if (CAN_CAPTURE[toObj]) {
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | QPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | RPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | BPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | NPRO);
	}
}

static void GenKingMoves(MoveList* ml, move_t frMove) {
	const int fr = moveFromSq(frMove);
	const int cx = ColourIndex(bTurn);
	const int cy = ColourIndex(ColourToggle(bTurn));
	const int* can_capture = WB_CAN_CAPTURE[cx];
	for (int dir = 0; dir < 8; dir++) {
		const int to = fr + KING_OFFSETS[dir];
		const int toObj = bBoard[to];
		if (!toObj)
			AddQuiet(ml, frMove | to | MOVE_KINGMOVE_MASK);
		else if (can_capture[toObj])
			AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | MOVE_KINGMOVE_MASK);
	}
}

static void GenKnightMoves(MoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = ColourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];

	int dir = 0;

	while (dir < 8) {

		const int to = fr + KNIGHT_OFFSETS[dir++];
		const int toObj = b[to];

		if (!toObj)
			AddQuiet(ml, frMove | to);

		else if (CAN_CAPTURE[toObj])
			AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
	}
}

void GenBishopMoves(MoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = ColourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];

	int dir = 0;

	while (dir < 4) {

		const int offset = BISHOP_OFFSETS[dir++];

		int to = fr + offset;
		while (!b[to]) {
			AddQuiet(ml, frMove | to);
			to += offset;
		}

		const int toObj = b[to];
		if (CAN_CAPTURE[toObj])
			AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
	}
}

static void GenRookMoves(MoveList* ml, move_t frMove) {
	const int fr = moveFromSq(frMove);
	const int cx = ColourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];

	int dir = 0;

	while (dir < 4) {

		const int offset = ROOK_OFFSETS[dir++];

		int to = fr + offset;
		while (!bBoard[to]) {
			AddQuiet(ml, frMove | to);
			to += offset;
		}

		const int toObj = bBoard[to];
		if (CAN_CAPTURE[toObj])
			AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
	}
}

static void GenQueenMoves(MoveList* ml, move_t frMove) {
	const int fr = moveFromSq(frMove);
	const int cx = ColourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];

	int dir = 0;

	while (dir < 8) {

		const int offset = QUEEN_OFFSETS[dir++];

		int to = fr + offset;
		while (!bBoard[to]) {
			AddQuiet(ml, frMove | to);
			to += offset;
		}

		const int toObj = bBoard[to];
		if (CAN_CAPTURE[toObj])
			AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
	}
}

static void GenMoves(MoveList* ml) {
	const int cx = ColourIndex(bTurn);
	const int* OUR_PIECE = WB_OUR_PIECE[cx];
	const int HOME_RANK = WB_HOME_RANK[cx];
	const int PROMOTE_RANK = WB_PROMOTE_RANK[cx];
	const int EP_RANK = WB_EP_RANK[cx];
	if (bTurn == WHITE)
		GenWhiteCastlingMoves(ml);
	else
		GenBlackCastlingMoves(ml);
	for (int i = 0; i < 64; i++) {
		const int fr = B88[i];
		const int frObj = bBoard[fr];
		if (!OUR_PIECE[frObj])
			continue;
		const int frPiece = objPiece(frObj);
		const int frRank = bRank[fr];
		const move_t frMove = (move_t)((frObj << MOVE_FROBJ_BITS) | (fr << MOVE_FR_BITS));
		switch (frPiece) {
		case KING:
			GenKingMoves(ml, frMove);
			break;
		case PAWN:
			if (frRank == HOME_RANK)
				GenHomePawnMoves(ml, frMove);
			else if (frRank == PROMOTE_RANK) {
				GenPromotePawnMoves(ml, frMove);
			}
			else if (frRank == EP_RANK) {
				GenPawnMoves(ml, frMove);
				if (bEP)
					GenEnPassPawnMoves(ml, frMove);
			}
			else
				GenPawnMoves(ml, frMove);
			break;

		case ROOK:
			GenRookMoves(ml, frMove);
			break;

		case KNIGHT:
			GenKnightMoves(ml, frMove);
			break;

		case BISHOP:
			GenBishopMoves(ml, frMove);
			break;

		case QUEEN:
			GenQueenMoves(ml, frMove);
			break;
		}
	}
}

static void InitMoveList(MoveList* ml, int noisy) {
	ml->noisy = noisy;
	ml->movesNum = 0;
	ml->moveNext = 0;
	GenMoves(ml);
}

static char* MoveToUci(move_t move) {
	static char fm[6];
	const int fr = moveFromSq(move);
	const int to = moveToSq(move);
	const char* frCoord = COORDS[fr];
	const char* toCoord = COORDS[to];
	fm[0] = frCoord[0];
	fm[1] = frCoord[1];
	fm[2] = toCoord[0];
	fm[3] = toCoord[1];
	if (move & MOVE_PROMOTE_MASK) {
		fm[4] = OBJ_CHAR[movePromotePiece(move) | BLACK];
		fm[5] = '\0';
	}
	else
		fm[4] = '\0';
	return fm;
}

static void MakeSpecialMove(move_t move) {
	int* b = bBoard;
	const int to = moveToSq(move);
	const int frObj = moveFromObj(move);
	const int frCol = objColour(frObj);
	if (frCol == WHITE) {
		if (move & MOVE_KINGMOVE_MASK)
			bKings[0] = to;
		if (move & MOVE_EPMAKE_MASK)
			bEP = to + 12;
		else if (move & MOVE_EPTAKE_MASK)
			b[to + 12] = 0;
		else if (move & MOVE_PROMOTE_MASK)
			b[to] = movePromotePiece(move) | WHITE;
		else if (move == MOVE_E1G1) {
			b[SQ_H1] = 0;
			b[SQ_F1] = W_ROOK;
		}
		else if (move == MOVE_E1C1) {
			b[SQ_A1] = 0;
			b[SQ_D1] = W_ROOK;
		}
	}
	else {
		if (move & MOVE_KINGMOVE_MASK)
			bKings[1] = to;
		if (move & MOVE_EPMAKE_MASK)
			bEP = to - 12;
		else if (move & MOVE_EPTAKE_MASK)
			b[to - 12] = 0;
		else if (move & MOVE_PROMOTE_MASK)
			b[to] = movePromotePiece(move) | BLACK;
		else if (move == MOVE_E8G8) {
			b[SQ_H8] = 0;
			b[SQ_F8] = B_ROOK;
		}
		else if (move == MOVE_E8C8) {
			b[SQ_A8] = 0;
			b[SQ_D8] = B_ROOK;
		}
	}
}

static void UnmakeSpecialMove(move_t move) {
	int* b = bBoard;
	const int fr = moveFromSq(move);
	const int to = moveToSq(move);
	const int frObj = moveFromObj(move);
	const int frCol = objColour(frObj);
	if (frCol == WHITE) {
		if (move & MOVE_KINGMOVE_MASK)
			bKings[0] = fr;
		if (move & MOVE_EPTAKE_MASK)
			b[to + 12] = B_PAWN;
		else if (move == MOVE_E1G1) {
			b[SQ_H1] = W_ROOK;
			b[SQ_F1] = 0;
		}
		else if (move == MOVE_E1C1) {
			b[SQ_A1] = W_ROOK;
			b[SQ_D1] = 0;
		}
	}
	else {
		if (move & MOVE_KINGMOVE_MASK)
			bKings[1] = fr;
		if (move & MOVE_EPTAKE_MASK)
			b[to - 12] = W_PAWN;
		else if (move == MOVE_E8G8) {
			b[SQ_H8] = B_ROOK;
			b[SQ_F8] = 0;
		}
		else if (move == MOVE_E8C8) {
			b[SQ_A8] = B_ROOK;
			b[SQ_D8] = 0;
		}
	}
}

static void MakeMove(move_t move) {
	const int fr = moveFromSq(move);
	const int to = moveToSq(move);
	const int frObj = moveFromObj(move);
	const int toObj = moveToObj(move);
	bBoard[fr] = 0;
	bBoard[to] = frObj;
	bTurn = ColourToggle(bTurn);
	bRights &= MASK_RIGHTS[fr] & MASK_RIGHTS[to];
	bEP = 0;
	bMove50++;
	if (move & MOVE_IKKY_MASK)
		MakeSpecialMove(move);
	if ((move & MOVE_DRAW_MASK) || objPiece(frObj) == PAWN)
		bMove50 = 0;
}

static void UnmakeMove(move_t move) {
	const int fr = moveFromSq(move);
	const int to = moveToSq(move);
	const int toObj = moveToObj(move);
	const int frObj = moveFromObj(move);
	bBoard[fr] = frObj;
	bBoard[to] = toObj;
	if (move & MOVE_IKKY_MASK)
		UnmakeSpecialMove(move);
	bTurn = ColourToggle(bTurn);
}

static void PlayUciMove(char* uciMove) {
	MoveList ml;
	InitMoveList(&ml, ALL_MOVES);
	move_t move = 0;
	while ((move = GetNextMove(&ml))) {
		if (!strcmp(MoveToUci(move), uciMove)) {
			MakeMove(move);
			return;
		}
	}
	fprintf(stderr, "cannot play uci move %s\n", uciMove);
}

static void ResetInfo() {
	info.timeStart = GetTimeMs();
	info.timeLimit = 0;
	info.depthLimit = MAX_PLY;
	info.nodesLimit = 0;
	info.nodes = 0;
	info.stop = FALSE;
	info.post = TRUE;
}

static U32 PerftDriver(int depth) {
	const int turn = bTurn;
	const int nextTurn = ColourToggle(turn);
	const int cx = ColourIndex(turn);
	U32 count = 0;
	move_t move;
	SCache sc;
	MoveList ml;
	CacheWrite(&sc);
	InitMoveList(&ml, ALL_MOVES);
	while ((move = GetNextMove(&ml))) {
		MakeMove(move);
		if (IsSquareAttacked(bKings[cx], nextTurn)) {
			UnmakeMove(move);
			CacheRead(&sc);
			continue;
		}
		if (depth)
			PerftDriver(depth - 1);
		else
			info.nodes++;
		UnmakeMove(move);
		CacheRead(&sc);
	}
	return count;
}

static int ShrinkNumber(U64 n) {
	if (n < 10000)
		return 0;
	if (n < 10000000)
		return 1;
	if (n < 10000000000)
		return 2;
	return 3;
}

static void PrintSummary(U64 time, U64 nodes) {
	U64 nps = (nodes * 1000) / max(time, 1);
	const char* units[] = { "", "k", "m", "g" };
	int sn = ShrinkNumber(nps);
	int p = pow(10, sn * 3);
	int b = pow(10, 3);
	printf("-----------------------------\n");
	printf("Time        : %llu\n", time);
	printf("Nodes       : %llu\n", nodes);
	printf("Nps         : %llu (%llu%s/s)\n", nps, nps / p, units[sn]);
	printf("-----------------------------\n");
}

void PrintPerformanceHeader() {
	printf("-----------------------------\n");
	printf("ply      time        nodes\n");
	printf("-----------------------------\n");
}

static void PrintPv(move_t move) {
	SCache sc;
	MoveList ml;
	CacheWrite(&sc);
	InitMoveList(&ml, ALL_MOVES);
	if (!AddTT(&ml, move))
		return;
	const int cx = ColourIndex(bTurn);
	const int nextTurn = ColourToggle(bTurn);
	MakeMove(move);
	if (IsSquareAttacked(bKings[cx], nextTurn)) {
		UnmakeMove(move);
		CacheRead(&sc);
		return;
	}
	printf(" %s", MoveToUci(move));
	const U64 hash = GetHash();
	TTEntry* tt_entry = tt + (hash % TT_MASK);
	if (tt_entry->hash != hash || tt_entry->flag != EXACT  || IsRepetition(hash)) {
		UnmakeMove(move);
		CacheRead(&sc);
		return;
	}
	historyHash[historyCount++] = hash;
	PrintPv(tt_entry->move);
	historyCount--;
	UnmakeMove(move);
	CacheRead(&sc);
}

void PrintInfo(int depth, int score) {
	printf("info depth %d score ", depth);
	if (abs(score) < MATE - MAX_PLY)
		printf("cp %d", score);
	else
		printf("mate %d", (score > 0 ? (MATE - score + 1) >> 1 : -(MATE + score) >> 1));
	printf(" time %lld", GetTimeMs() - info.timeStart);
	printf(" nodes %lld", info.nodes);
	printf(" hashfull %d pv", Permill());
	PrintPv(tBestMove);
	printf("\n");
}

static int SearchQuiescence(int alpha, int beta, int depth, int ply) {
	if (CheckUp())
		return 0;
	if (ply >= MAX_PLY)
		return EvalPosition();
	U64 hash = GetHash();
	TTEntry* tt_entry = tt + (hash % TT_MASK);
	int tt_move = 0;
	if (tt_entry->hash == hash) {
		tt_move = tt_entry->move;
		if (tt_entry->flag == EXACT || (tt_entry->flag == UPPER && tt_entry->score >= beta) || (tt_entry->flag == LOWER && tt_entry->score <= alpha))
			return tt_entry->score;
	}
	const int staticEval = EvalPosition();
	if (alpha < staticEval)
		alpha = staticEval;
	if (alpha >= beta)
		return beta;
	int bestMove = tt_move;
	U8 tt_flag = LOWER;
	const int turn = bTurn;
	const int nextTurn = ColourToggle(turn);
	const int cx = ColourIndex(turn);
	move_t move;
	int score = 0;
	int played = 0;
	SCache sc;
	MoveList ml;
	CacheWrite(&sc);
	InitMoveList(&ml, NOISY_MOVES);
	while ((move = GetNextMove(&ml))) {
		MakeMove(move);
		if (IsSquareAttacked(bKings[cx], nextTurn)) {
			UnmakeMove(move);
			CacheRead(&sc);
			continue;
		}
		score = -SearchQuiescence(-beta, -alpha, depth - 1, ply + 1);
		UnmakeMove(move);
		CacheRead(&sc);
		if (info.stop)
			return 0;
		if (alpha < score) {
			bestMove = move;
			tt_flag = EXACT;
			alpha = score;
		}
		if (alpha >= beta) {
			tt_flag = UPPER;
			break;
		}
	}
	tt_entry->hash = hash;
	tt_entry->move = bestMove;
	tt_entry->depth = depth;
	tt_entry->score = alpha;
	tt_entry->flag = tt_flag;
	return alpha;
}

static int SearchAlpha(int alpha, int beta, int depth, int ply) {
	if (ply >= MAX_PLY)
		return 0;
	const int turn = bTurn;
	const int nextTurn = ColourToggle(turn);
	const int cx = ColourIndex(turn);
	const int inCheck = IsSquareAttacked(bKings[cx], nextTurn);
	if (inCheck)
		depth = max(1, depth + 1);
	if (depth <= 0)
		return SearchQuiescence(alpha, beta, depth, ply);
	if (CheckUp())
		return 0;
	const int pvNode = alpha != (beta - 1);
	U64 hash = GetHash();
	if (IsRepetition(hash) || bMove50 >= 100)
		return 0;
	int tt_move = 0;
	TTEntry* tt_entry = tt + (hash % TT_MASK);
	if (tt_entry->hash == hash) {
		tt_move = tt_entry->move;
		if (tt_entry->depth >= depth)
			if (tt_entry->flag == EXACT || (tt_entry->flag == UPPER && tt_entry->score >= beta) || (tt_entry->flag == LOWER && tt_entry->score <= alpha))
				return tt_entry->score;
	}
	historyHash[historyCount++] = hash;
	U8 tt_flag = LOWER;
	const int rootNode = ply == 0;
	move_t move;
	int bestScore = -MATE;
	int bestMove = tt_move;
	int score = 0;
	int legalMoves = 0;
	SCache sc;
	MoveList ml;
	CacheWrite(&sc);
	InitMoveList(&ml, ALL_MOVES);
	if (tt_move)
		AddTT(&ml, tt_move);
	while ((move = GetNextMove(&ml))) {
		MakeMove(move);
		if (IsSquareAttacked(bKings[cx], nextTurn)) {
			UnmakeMove(move);
			CacheRead(&sc);
			continue;
		}
		legalMoves++;
		score = -SearchAlpha(-beta, -alpha, depth - 1, ply + 1);
		UnmakeMove(move);
		CacheRead(&sc);
		if (info.stop)
			break;
		if (score > bestScore) {
			bestScore = score;
			bestMove = move;
			if (bestScore > alpha) {
				alpha = bestScore;
				tt_flag = EXACT;
				if (rootNode) {
					tBestMove = bestMove;
					PrintInfo(depth, bestScore);
				}
				if (bestScore >= beta) {
					tt_flag = UPPER;
					break;
				}
			}
		}
	}
	historyCount--;
	if (info.stop)
		return 0;
	if (!legalMoves)
		return inCheck ? ply - MATE : 0;
	tt_entry->hash = hash;
	tt_entry->move = bestMove;
	tt_entry->depth = depth;
	tt_entry->score = bestScore;
	tt_entry->flag = tt_flag;
	return bestScore;
}

static void SearchIteratively() {
	TTClear();
	tBestMove = 0;
	int score = 0;
	int alpha = 0;
	int beta = 0;
	int delta = 0;
	for (int depth = 1; depth <= info.depthLimit; depth++) {
		alpha = -MATE;
		beta = MATE;
		delta = 10;
		if (depth >= 4) { alpha = MAX(-MATE, score - delta); beta = MIN(MATE, score + delta); }
		while (TRUE) {
			score = SearchAlpha(alpha, beta, depth, 0);
			if (info.stop)
				break;
			delta += delta / 2;
			if (score <= alpha)
				alpha = MAX(-MATE, score - delta);
			else if (score >= beta) 
				beta = MIN(MATE, score + delta);
			else
				break;
		}
		if (info.stop)
			break;
		if (info.timeLimit && GetTimeMs() - info.timeStart > info.timeLimit / 2)
			break;
	}
	if (info.post)
		printf("bestmove %s\n", MoveToUci(tBestMove));
	fflush(stdout);
}

//performance test
static inline void UciPerformance() {
	ResetInfo();
	PrintPerformanceHeader();
	info.depthLimit = 0;
	U64 elapsed = 0;
	while (elapsed < 3000) {
		PerftDriver(info.depthLimit++);
		elapsed = GetTimeMs() - info.timeStart;
		printf(" %2d. %8llu %12llu\n", info.depthLimit, elapsed, info.nodes);
	}
	PrintSummary(elapsed, info.nodes);
}

//start benchmark
static void UciBench() {
	ResetInfo();
	PrintPerformanceHeader();
	info.depthLimit = 0;
	info.post = FALSE;
	U64 elapsed = 0;
	while (elapsed < 3000) {
		++info.depthLimit;
		SearchIteratively();
		elapsed = GetTimeMs() - info.timeStart;
		printf(" %2d. %8llu %12llu\n", info.depthLimit, elapsed, info.nodes);
	}
	PrintSummary(elapsed, info.nodes);
}

//new game
static void UciNewGame() {
	TTClear();
}

static void SetFen(char* fen) {
	for (int i = 0; i < 144; i++)
		bBoard[i] = EDGE;
	for (int i = 0; i < 64; i++)
		bBoard[B88[i]] = 0;
	bRights = 0;
	bEP = 0;
	bMove50 = 0;
	int sq = 0;
	while (*fen && *fen != ' ') {
		int i = B88[sq];
		switch (*fen) {
		case '1': sq += 1; break;
		case '2': sq += 2; break;
		case '3': sq += 3; break;
		case '4': sq += 4; break;
		case '5': sq += 5; break;
		case '6': sq += 6; break;
		case '7': sq += 7; break;
		case '8': sq += 8; break;
		case 'P': sq++; bBoard[i] = W_PAWN; break;
		case 'N': sq++; bBoard[i] = W_KNIGHT; break;
		case 'B': sq++; bBoard[i] = W_BISHOP; break;
		case 'R': sq++; bBoard[i] = W_ROOK; break;
		case 'Q': sq++; bBoard[i] = W_QUEEN; break;
		case 'K': sq++; bBoard[i] = W_KING; bKings[0] = i; break;
		case 'p': sq++; bBoard[i] = B_PAWN; break;
		case 'n': sq++; bBoard[i] = B_KNIGHT; break;
		case 'b': sq++; bBoard[i] = B_BISHOP; break;
		case 'r': sq++; bBoard[i] = B_ROOK; break;
		case 'q': sq++; bBoard[i] = B_QUEEN; break;
		case 'k': sq++; bBoard[i] = B_KING; bKings[1] = i; break;
		}
		fen++;
	}
	fen++;
	bTurn = *fen == 'w' ? WHITE : BLACK;
	while (*fen && *fen != ' ') fen++; fen++;
	while (*fen && *fen != ' ') {
		switch (*fen) {
		case 'K': bRights |= WHITE_RIGHTS_KING; break;
		case 'Q': bRights |= WHITE_RIGHTS_QUEEN; break;
		case 'k': bRights |= BLACK_RIGHTS_KING; break;
		case 'q': bRights |= BLACK_RIGHTS_QUEEN; break;
		case '-': break;
		}
		fen++;
	}
	fen++;
	if (*fen != '-')
		bEP = B88[fen[0] - 'a' + 8 * (7 - (fen[1] - '1'))];
	while (*fen && *fen != ' ') fen++; fen++;
	bMove50 = atoi(fen);
}

static void ParsePosition(char* ptr) {
	historyCount = 0;
	char token[80], fen[80];
	ptr = ParseToken(ptr, token);
	if (strcmp(token, "fen") == 0) {
		fen[0] = '\0';
		while (1) {
			ptr = ParseToken(ptr, token);
			if (*token == '\0' || strcmp(token, "moves") == 0)
				break;
			strcat(fen, token);
			strcat(fen, " ");
		}
		SetFen(fen);
	}
	else {
		ptr = ParseToken(ptr, token);
		SetFen(START_FEN);
	}
	if (strcmp(token, "moves") == 0)
		while (1) {
			ptr = ParseToken(ptr, token);
			if (*token == '\0')
				break;
			PlayUciMove(token);
		}
}

static void ParseGo(char* command) {
	ResetInfo();
	int wtime = 0;
	int btime = 0;
	int winc = 0;
	int binc = 0;
	int movestogo = 32;
	char* argument = NULL;
	if (argument = strstr(command, "binc"))
		binc = atoi(argument + 5);
	if (argument = strstr(command, "winc"))
		winc = atoi(argument + 5);
	if (argument = strstr(command, "wtime"))
		wtime = atoi(argument + 6);
	if (argument = strstr(command, "btime"))
		btime = atoi(argument + 6);
	if ((argument = strstr(command, "movestogo")))
		movestogo = atoi(argument + 10);
	if ((argument = strstr(command, "movetime")))
		info.timeLimit = atoi(argument + 9);
	if ((argument = strstr(command, "depth")))
		info.depthLimit = atoi(argument + 6);
	if (argument = strstr(command, "nodes"))
		info.nodesLimit = atoi(argument + 5);
	int time = bTurn == WHITE ? wtime : btime;
	int inc = bTurn == WHITE ? winc : binc;
	if (time)
		info.timeLimit = min(time / movestogo + inc, time / 2);
	SearchIteratively();
}

static void UciCommand(char* line) {
	if (strncmp(line, "ucinewgame", 10) == 0)
		UciNewGame();
	else if (!strncmp(line, "uci", 3)) {
		printf("id name %s\nuciok\n", NAME);
		fflush(stdout);
	}
	else if (!strncmp(line, "isready", 7)) {
		printf("readyok\n");
		fflush(stdout);
	}
	else if (!strncmp(line, "go", 2))
		ParseGo(line + 2);
	else if (!strncmp(line, "position", 8))
		ParsePosition(line + 8);
	else if (!strncmp(line, "print", 5))
		PrintBoard();
	else if (!strncmp(line, "perft", 5))
		UciPerformance();
	else if (!strncmp(line, "bench", 5))
		UciBench();
	else if (!strncmp(line, "exit", 4))
		exit(0);
}

static void UciLoop() {
	//UciCommand("position fen 5k2/8/1p1N4/1P1P4/2n1P3/2q2B1P/8/1K6 b - - 2 47");
	//UciCommand("go movetime 1000");
	char line[4000];
	while (fgets(line, sizeof(line), stdin) != NULL)
		UciCommand(line);
}

int main(int argc, char** argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	InitHash();
	InitEval();
	printf("%s %s\n", NAME, VERSION);
	SetFen(START_FEN);
	UciLoop();
	return 0;
}