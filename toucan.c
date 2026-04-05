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
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MATE 32000
#define INF 32001
#define MAX_PLY 1024
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
#define TT_EXACT 0x01
#define TT_ALPHA 0x02
#define TT_BETA  0x04

#define A1 110
#define B1 111
#define C1 112
#define D1 113
#define E1 114
#define F1 115
#define G1 116
#define H1 117
#define B2 99
#define C2 100
#define G2 10
#define H2 105
#define B7 39
#define C7 40
#define G7 44
#define H7 45
#define A8 26
#define B8 27
#define C8 28
#define D8 29
#define E8 30
#define F8 31
#define G8 32
#define H8 33

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

#define MOVE_E1G1 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (W_KING << MOVE_FROBJ_BITS) | (E1 << MOVE_FR_BITS) | G1)
#define MOVE_E1C1 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (W_KING << MOVE_FROBJ_BITS) | (E1 << MOVE_FR_BITS) | C1)
#define MOVE_E8G8 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (B_KING << MOVE_FROBJ_BITS) | (E8 << MOVE_FR_BITS) | G8)
#define MOVE_E8C8 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (B_KING << MOVE_FROBJ_BITS) | (E8 << MOVE_FR_BITS) | C8)

#define QPRO (((QUEEN-2)  << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)
#define RPRO (((ROOK-2)   << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)
#define BPRO (((BISHOP-2) << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)
#define NPRO (((KNIGHT-2) << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)

typedef struct {
	move_t quietMoves[MAX_MOVES];
	move_t noisyMoves[MAX_MOVES];
	int quietRanks[MAX_MOVES];
	int noisyRanks[MAX_MOVES];
	int quietNum;
	int noisyNum;
	int nextMove;
	int noisy;
	int stage;
}SMoveList;

typedef struct {
	U64 hash;
	int score;
	int depth;
	U32 flag;
}TT_Entry;

typedef struct {
	int post;
	int stop;
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

const char OBJ_CHAR[] = { ' ','A','N','B','R','Q','K','x','y','a','n','b','r','q','k','z' };

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


const int ADJACENT[144] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0 };

const int B88[] = { 26, 27, 28, 29, 30, 31, 32, 33,
				   38, 39, 40, 41, 42, 43, 44, 45,
				   50, 51, 52, 53, 54, 55, 56, 57,
				   62, 63, 64, 65, 66, 67, 68, 69,
				   74, 75, 76, 77, 78, 79, 80, 81,
				   86, 87, 88, 89, 90, 91, 92, 93,
				   98, 99, 100,101,102,103,104,105,
				   110,111,112,113,114,115,116,117 };

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

const int RANK[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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

const int FYLE[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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

const int CENTRE[] = { 0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0,
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

int MATERIAL[] = { 100,320,330,500,900,20000 };

int WPAWN_PST[] = { 0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
				   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
				   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
				   0, 0, 50, 50, 50,  50,  50,  50,  50, 50, 0, 0,
				   0, 0, 10, 10, 20,  30,  30,  20,  10, 10, 0, 0,
				   0, 0, 5,  5,  10,  25,  25,  10,  5,  5,  0, 0,
				   0, 0, 0,  0,  0,   20,  20,  0,   0,  0,  0, 0,
				   0, 0, 5,  -5, -10, 0,   0,   -10, -5, 5,  0, 0,
				   0, 0, 5,  10, 10,  -20, -20, 10,  10, 5,  0, 0,
				   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
				   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
				   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0 };

int WKNIGHT_PST[] = { 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					 0, 0, -50, -40, -30, -30, -30, -30, -40, -50, 0, 0,
					 0, 0, -40, -20, 0,   0,   0,   0,   -20, -40, 0, 0,
					 0, 0, -30, 0,   10,  15,  15,  10,   0,  -30, 0, 0,
					 0, 0, -30, 5,   15,  20,  20,  15,   5,  -30, 0, 0,
					 0, 0, -30, 0,   15,  20,  20,  15,   0,  -30, 0, 0,
					 0, 0, -30, 5,   10,  15,  15,  10,   5,  -30, 0, 0,
					 0, 0, -40, -20, 0,   5,   5,   0,   -20, -40, 0, 0,
					 0, 0, -50, -40, -30, -30, -30, -30, -40, -50, 0, 0,
					 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0 };

int WBISHOP_PST[] = { 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					 0, 0, -20, -10, -10, -10, -10, -10, -10, -20, 0, 0,
					 0, 0, -10, 0,   0,   0,   0,   0,    0,  -10, 0, 0,
					 0, 0, -10, 0,   5,   10,  10,  5,    0,  -10, 0, 0,
					 0, 0, -10, 5,   5,   10,  10,  5,    5,  -10, 0, 0,
					 0, 0, -10, 0,   10,  10,  10,  10,   0,  -10, 0, 0,
					 0, 0, -10, 10,  10,  10,  10,  10,   10, -10, 0, 0,
					 0, 0, -10, 5 ,   0,   0,   0,   0,   5,  -10, 0, 0,
					 0, 0, -20, -10, -10, -10, -10, -10, -10, -20, 0, 0,
					 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0 };
int WROOK_PST[] = { 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
				   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
				   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
				   0, 0, 5,   10,  10,  10,  10,  10,  10,  5,  0, 0,
				   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
				   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
				   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
				   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
				   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
				   0, 0, 0,   0,   0,   5,   5,   0,   0,   0,  0, 0,
				   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
				   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0 };

int WQUEEN_PST[] = { 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					0, 0, -20, -10, -10, -5,  -5,  -10, -10, -20, 0, 0,
					0, 0, -10, 0,   0,   0,   0,   0,    0,  -10, 0, 0,
					0, 0, -10, 0,   5,   5,   5,   5,    0,  -10, 0, 0,
					0, 0, -5,  0,   5,   5,   5,   5,    0,  -5,  0, 0,
					0, 0,  0,  0,   5,   5,   5,   5,    0,  -5,  0, 0,
					0, 0, -10, 5,   5,   5,   5,   5,    0,  -10, 0, 0,
					0, 0, -10, 0,   5,   0,   0,   0,    0,  -10, 0, 0,
					0, 0, -20, -10, -10, -5,  -5,  -10, -10, -20, 0, 0,
					0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0 };

int WKING_MID_PST[] = { 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					   0, 0, -30, -40, -40, -50, -50, -40, -40, -30, 0, 0,
					   0, 0, -30, -40, -40, -50, -50, -40, -40, -30, 0, 0,
					   0, 0, -30, -40, -40, -50, -50, -40, -40, -30, 0, 0,
					   0, 0, -30, -40, -40, -50, -50, -40, -40, -30, 0, 0,
					   0, 0, -20, -30, -30, -40, -40, -30, -30, -20, 0, 0,
					   0, 0, -10, -20, -20, -20, -20, -20, -20, -10, 0, 0,
					   0, 0, 20,  20,  0,   0,   0,   0,   20,  20,  0, 0,
					   0, 0, 20,  30,  10,  0,   0,   10,  30,  20,  0, 0,
					   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
					   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0 };

int WKING_END_PST[] = { 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
					   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
					   0, 0, -50, -40, -30, -20, -20, -30, -40, -50,0, 0,
					   0, 0, -30, -20, -10, 0,   0,   -10, -20, -30,0, 0,
					   0, 0, -30, -10, 20,  30,  30,  20,  -10, -30,0, 0,
					   0, 0, -30, -10, 30,  40,  40,  30,  -10, -30,0, 0,
					   0, 0, -30, -10, 30,  40,  40,  30,  -10, -30,0, 0,
					   0, 0, -30, -10, 20,  30,  30,  20,  -10, -30,0, 0,
					   0, 0, -30, -30, 0,   0,   0,   0,   -30, -30,0, 0,
					   0, 0, -50, -30, -30, -30, -30, -30, -30, -50,0, 0,
					   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
					   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0 };

int BPAWN_PST[144];
int BKNIGHT_PST[144];
int BBISHOP_PST[144];
int BROOK_PST[144];
int BQUEEN_PST[144];
int BKING_MID_PST[144];
int BKING_END_PST[144];

int* WHITE_MID_PST[] = { WPAWN_PST, WKNIGHT_PST, WBISHOP_PST, WROOK_PST, WQUEEN_PST, WKING_MID_PST };
int* WHITE_END_PST[] = { WPAWN_PST, WKNIGHT_PST, WBISHOP_PST, WROOK_PST, WQUEEN_PST, WKING_END_PST };
int* BLACK_MID_PST[] = { BPAWN_PST, BKNIGHT_PST, BBISHOP_PST, BROOK_PST, BQUEEN_PST, BKING_MID_PST };
int* BLACK_END_PST[] = { BPAWN_PST, BKNIGHT_PST, BBISHOP_PST, BROOK_PST, BQUEEN_PST, BKING_END_PST };

int** WB_MID_PST[] = { WHITE_MID_PST, BLACK_MID_PST };
int** WB_END_PST[] = { WHITE_END_PST, BLACK_END_PST };

int bBoard[144];
int bTurn = 0;
int bRights = 0;
int bEP = 0;
int bMove50 = 0;
int bKings[2] = { 0,0 };
U64 bHash = 0;

U64 keys[16 * 144];
int historyCount = 0;
U64 historyHash[1024];
TT_Entry  tt[TT_SIZE];
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

static void GetHash() {
	bHash = bTurn;
	for (int i = 0; i < 64; i++) {
		const int sq = B88[i];
		const int piece = bBoard[sq];
		if (piece)
			bHash ^= keys[piece * 144 + sq];
	}
	bHash ^= keys[bRights];
	bHash ^= keys[7 * 144 + bEP];
}

static int Permill() {
	int pm = 0;
	for (int n = 0; n < 1000; n++)
		if (tt[n].hash)
			pm++;
	return pm;
}

static int IsRepetition() {
	for (int n = historyCount - 4; n >= historyCount - bMove50; n -= 2) {
		if (n < 0)
			return FALSE;
		if (historyHash[n] == bHash)
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

int colourIndex(int c) {
	return c >> 3;
}

int colourtoggleIndex(int i) {
	return abs(i - 1);
}

int colourMultiplier(int c) {
	return (-c >> 31) | 1;
}

int colourToggle(int c) {
	return ~c & COLOUR_MASK;
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

	printf("hash : %16llx\n", bHash);
}

static int IsSquareAttacked(int to, int byCol) {
	int* b = bBoard;
	const int cx = colourIndex(byCol);
	const int OFFSET_DIAG1 = -WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = -WB_OFFSET_DIAG2[cx];
	const int BY_PAWN = WB_PAWN[cx];
	const int* RQ = WB_RQ[cx];
	const int* BQ = WB_BQ[cx];
	const int N = KNIGHT | byCol;
	int fr = 0;
	if (b[to + OFFSET_DIAG1] == BY_PAWN || b[to + OFFSET_DIAG2] == BY_PAWN)
		return 1;
	if ((b[to + -10] == N) ||
		(b[to + -23] == N) ||
		(b[to + -14] == N) ||
		(b[to + -25] == N) ||
		(b[to + 10] == N) ||
		(b[to + 23] == N) ||
		(b[to + 14] == N) ||
		(b[to + 25] == N)) return 1;
	fr = to + 1;  while (!b[fr]) fr += 1;  if (RQ[b[fr]]) return 1;
	fr = to - 1;  while (!b[fr]) fr -= 1;  if (RQ[b[fr]]) return 1;
	fr = to + 12; while (!b[fr]) fr += 12; if (RQ[b[fr]]) return 1;
	fr = to - 12; while (!b[fr]) fr -= 12; if (RQ[b[fr]]) return 1;
	fr = to + 11; while (!b[fr]) fr += 11; if (BQ[b[fr]]) return 1;
	fr = to - 11; while (!b[fr]) fr -= 11; if (BQ[b[fr]]) return 1;
	fr = to + 13; while (!b[fr]) fr += 13; if (BQ[b[fr]]) return 1;
	fr = to - 13; while (!b[fr]) fr -= 13; if (BQ[b[fr]]) return 1;
	return 0;
}

static void TTPut(U64 hash, int flags, int depth, int score) {
	const U64 i = hash & TT_MASK;
	tt[i].hash = hash;
	tt[i].flag = flags;
	tt[i].depth = depth;
	tt[i].score = score;
}

static int EvalPosition() {
	int* b = bBoard;
	const int cx = colourMultiplier(bTurn);
	int e = 10 * cx;
	int pst_mid = 0;
	int pst_end = 0;
	int q = 0;
	for (int sq = 0; sq < 64; sq++) {
		const int fr = B88[sq];
		const int frObj = b[fr];
		if (!frObj)
			continue;
		const int frPiece = objPiece(frObj) - 1;
		const int frColour = objColour(frObj);
		const int frIndex = colourIndex(frColour);
		const int frMult = colourMultiplier(frColour);
		e += MATERIAL[frPiece] * frMult;
		pst_mid += WB_MID_PST[frIndex][frPiece][fr] * frMult;
		pst_end += WB_END_PST[frIndex][frPiece][fr] * frMult;
		q += IS_Q[frObj];
	}
	if (q)
		return (e + pst_mid) * cx;
	else
		return (e + pst_end) * cx;
}

static int FlipSq(int sq) {
	const int m = (143 - sq) / 12;
	return 12 * m + sq % 12;
}

static void InitEval() {
	for (int i = 0; i < 144; i++) {
		const int j = FlipSq(i);
		BPAWN_PST[j] = WPAWN_PST[i];
		BKNIGHT_PST[j] = WKNIGHT_PST[i];
		BBISHOP_PST[j] = WBISHOP_PST[i];
		BROOK_PST[j] = WROOK_PST[i];
		BQUEEN_PST[j] = WQUEEN_PST[i];
		BKING_MID_PST[j] = WKING_MID_PST[i];
		BKING_END_PST[j] = WKING_END_PST[i];
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

static void InitMoveList(SMoveList* ml, int noisy) {
	ml->stage = 0;
	ml->noisy = noisy;
}

static void AddQuiet(SMoveList* ml, move_t move) {
	ml->quietMoves[ml->quietNum] = move;
	ml->quietRanks[ml->quietNum] = 100 + CENTRE[moveToSq(move)] - CENTRE[moveFromSq(move)];
	ml->quietNum = ml->quietNum + 1;
}

const int RANK_ATTACKER[] = { 0,    600,  500,  400,  300,  200,  100, 0, 0,    600,  500,  400,  300,  200,  100 };
const int RANK_DEFENDER[] = { 2000, 1000, 3000, 3000, 5000, 9000, 0,   0, 2000, 1000, 3000, 3000, 5000, 9000, 0 };

static void AddNoisy(SMoveList* ml, move_t move) {
	ml->noisyMoves[ml->noisyNum] = move;
	ml->noisyRanks[ml->noisyNum] = RANK_ATTACKER[moveFromObj(move)] + RANK_DEFENDER[moveToObj(move)];
	ml->noisyNum = ml->noisyNum + 1;
}

static void GenWhiteCastlingMoves(SMoveList* ml) {

	int* b = bBoard;

	if ((bRights & WHITE_RIGHTS_KING) && !b[F1]
		&& !b[G1]
		&& b[G2] != B_KING
		&& b[H2] != B_KING
		&& !IsSquareAttacked(E1, BLACK)
		&& !IsSquareAttacked(F1, BLACK)) {
		AddQuiet(ml, MOVE_E1G1);
	}

	if ((bRights & WHITE_RIGHTS_QUEEN) && !b[B1]
		&& !b[C1]
		&& !b[D1]
		&& b[B2] != B_KING
		&& b[C2] != B_KING
		&& !IsSquareAttacked(E1, BLACK)
		&& !IsSquareAttacked(D1, BLACK)) {
		AddQuiet(ml, MOVE_E1C1);
	}
}

static void GenBlackCastlingMoves(SMoveList* ml) {

	int* b = bBoard;

	if ((bRights & BLACK_RIGHTS_KING) && b[F8] == 0
		&& b[G8] == 0
		&& b[G7] != W_KING
		&& b[H7] != W_KING
		&& !IsSquareAttacked(E8, WHITE)
		&& !IsSquareAttacked(F8, WHITE)) {
		AddQuiet(ml, MOVE_E8G8);
	}

	if ((bRights & BLACK_RIGHTS_QUEEN) && b[B8] == 0
		&& b[C8] == 0
		&& b[D8] == 0
		&& b[B7] != W_KING
		&& b[C7] != W_KING
		&& !IsSquareAttacked(E8, WHITE)
		&& !IsSquareAttacked(D8, WHITE)) {
		AddQuiet(ml, MOVE_E8C8);
	}
}

void GenPawnMoves(SMoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = colourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];
	const int OFFSET_ORTH = WB_OFFSET_ORTH[cx];
	const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

	int to, toObj;

	to = fr + OFFSET_ORTH;
	if (!b[to])
		AddQuiet(ml, frMove | to);

	to = fr + OFFSET_DIAG1;
	toObj = b[to];
	if (CAN_CAPTURE[toObj])
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);

	to = fr + OFFSET_DIAG2;
	toObj = b[to];
	if (CAN_CAPTURE[toObj])
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
}

void GenEnPassPawnMoves(SMoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = colourIndex(bTurn);
	const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

	int to;

	to = fr + OFFSET_DIAG1;
	if (to == bEP && !b[to])
		AddNoisy(ml, frMove | to | MOVE_EPTAKE_MASK);

	to = fr + OFFSET_DIAG2;
	if (to == bEP && !b[to])
		AddNoisy(ml, frMove | to | MOVE_EPTAKE_MASK);
}

void GenHomePawnMoves(SMoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = colourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];
	const int OFFSET_ORTH = WB_OFFSET_ORTH[cx];
	const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

	int to, toObj;

	to = fr + OFFSET_ORTH;
	if (!b[to]) {
		AddQuiet(ml, frMove | to);
		to += OFFSET_ORTH;
		if (!b[to])
			AddQuiet(ml, frMove | to | MOVE_EPMAKE_MASK);
	}

	to = fr + OFFSET_DIAG1;
	toObj = b[to];
	if (CAN_CAPTURE[toObj])
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);

	to = fr + OFFSET_DIAG2;
	toObj = b[to];
	if (CAN_CAPTURE[toObj])
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to);
}

void GenPromotePawnMoves(SMoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = colourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];
	const int OFFSET_ORTH = WB_OFFSET_ORTH[cx];
	const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
	const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

	int to, toObj;

	to = fr + OFFSET_ORTH;
	if (!b[to]) {
		AddQuiet(ml, frMove | to | QPRO);
		AddQuiet(ml, frMove | to | RPRO);
		AddQuiet(ml, frMove | to | BPRO);
		AddQuiet(ml, frMove | to | NPRO);
	}

	to = fr + OFFSET_DIAG1;
	toObj = b[to];
	if (CAN_CAPTURE[toObj]) {
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | QPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | RPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | BPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | NPRO);
	}

	to = fr + OFFSET_DIAG2;
	toObj = b[to];
	if (CAN_CAPTURE[toObj]) {
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | QPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | RPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | BPRO);
		AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | NPRO);
	}
}

/*}}}*/
/*{{{  genKingMoves*/

void GenKingMoves(SMoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = colourIndex(bTurn);
	const int cy = colourIndex(colourToggle(bTurn));
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];
	const int theirKingSq = bKings[cy];

	int dir = 0;

	while (dir < 8) {

		const int to = fr + KING_OFFSETS[dir++];

		if (!ADJACENT[abs(to - theirKingSq)]) {

			const int toObj = b[to];

			if (!toObj)
				AddQuiet(ml, frMove | to | MOVE_KINGMOVE_MASK);

			else if (CAN_CAPTURE[toObj])
				AddNoisy(ml, frMove | (toObj << MOVE_TOOBJ_BITS) | to | MOVE_KINGMOVE_MASK);
		}
	}
}

void GenKnightMoves(SMoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = colourIndex(bTurn);
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

void GenBishopMoves(SMoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = colourIndex(bTurn);
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

void GenRookMoves(SMoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = colourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];

	int dir = 0;

	while (dir < 4) {

		const int offset = ROOK_OFFSETS[dir++];

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

void GenQueenMoves(SMoveList* ml, move_t frMove) {

	int* b = bBoard;

	const int fr = moveFromSq(frMove);
	const int cx = colourIndex(bTurn);
	const int* CAN_CAPTURE = WB_CAN_CAPTURE[cx];

	int dir = 0;

	while (dir < 8) {

		const int offset = QUEEN_OFFSETS[dir++];

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

static void GenMoves(SMoveList* ml) {
	const int cx = colourIndex(bTurn);
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
		const int frRank = RANK[fr];
		const move_t frMove = (move_t)((frObj << MOVE_FROBJ_BITS) | (fr << MOVE_FR_BITS));
		switch (frPiece) {
		case KING:
			GenKingMoves(ml, frMove);
			break;
		case PAWN:
			if (frRank == HOME_RANK) {
				GenHomePawnMoves(ml, frMove);
			}
			else if (frRank == PROMOTE_RANK) {
				GenPromotePawnMoves(ml, frMove);
			}
			else if (frRank == EP_RANK) {
				GenPawnMoves(ml, frMove);
				if (bEP)
					GenEnPassPawnMoves(ml, frMove);
			}
			else {
				GenPawnMoves(ml, frMove);
			}
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

static move_t NextStagedMove(int next, int num, move_t* moves, int* ranks) {

	int maxR = -10000;
	int maxI = 0;

	for (int i = next; i < num; i++) {
		if (ranks[i] > maxR) {
			maxR = ranks[i];
			maxI = i;
		}
	}

	const move_t maxM = moves[maxI];
	moves[maxI] = moves[next];
	ranks[maxI] = ranks[next];
	return maxM;
}

static move_t GetNextMove(SMoveList* ml) {
	switch (ml->stage) {
	case 0:
		ml->noisyNum = 0;
		ml->quietNum = 0;
		GenMoves(ml);
		ml->nextMove = 0;
		ml->stage++;
	case 1:
		if (ml->nextMove < ml->noisyNum)
			return NextStagedMove(ml->nextMove++, ml->noisyNum, ml->noisyMoves, ml->noisyRanks);
		if (ml->noisy)
			return 0;
		ml->nextMove = 0;
		ml->stage++;
	case 2:
		if (ml->nextMove < ml->quietNum)
			return NextStagedMove(ml->nextMove++, ml->quietNum, ml->quietMoves, ml->quietRanks);
		return 0;
	default:
		fprintf(stderr, "get next move stage is %d\n", ml->stage);
		return 0;
	}
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
			b[H1] = 0;
			b[F1] = W_ROOK;
		}
		else if (move == MOVE_E1C1) {
			b[A1] = 0;
			b[D1] = W_ROOK;
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
			b[H8] = 0;
			b[F8] = B_ROOK;
		}
		else if (move == MOVE_E8C8) {
			b[A8] = 0;
			b[D8] = B_ROOK;
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
			b[H1] = W_ROOK;
			b[F1] = 0;
		}
		else if (move == MOVE_E1C1) {
			b[A1] = W_ROOK;
			b[D1] = 0;
		}
	}
	else {
		if (move & MOVE_KINGMOVE_MASK)
			bKings[1] = fr;
		if (move & MOVE_EPTAKE_MASK)
			b[to - 12] = W_PAWN;
		else if (move == MOVE_E8G8) {
			b[H8] = B_ROOK;
			b[F8] = 0;
		}
		else if (move == MOVE_E8C8) {
			b[A8] = B_ROOK;
			b[D8] = 0;
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
	bTurn = colourToggle(bTurn);
	bRights &= MASK_RIGHTS[fr] & MASK_RIGHTS[to];
	bEP = 0;
	bMove50++;
	if (move & MOVE_IKKY_MASK)
		MakeSpecialMove(move);
	if ((move & MOVE_DRAW_MASK) || objPiece(frObj) == PAWN)
		bMove50 = 0;
	GetHash();
	historyHash[historyCount++] = bHash;
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
	bTurn = colourToggle(bTurn);
	bHash = historyHash[--historyCount];
}

static void PlayUciMove(char* uciMove) {
	SMoveList ml;
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
	const int nextTurn = colourToggle(turn);
	const int cx = colourIndex(turn);
	U32 count = 0;
	move_t move;
	SCache sc;
	SMoveList ml;
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

static int SearchQuiescence(int alpha, int beta, int depth, int ply) {
	if (CheckUp())
		return 0;
	if (ply >= MAX_PLY)
		return EvalPosition();
	TT_Entry* tt_entry = tt + (bHash % TT_MASK);
	if (tt->hash == bHash)
		if (tt->flag == TT_EXACT || (tt->flag == TT_BETA && tt->score >= beta) || (tt->flag == TT_ALPHA && tt->score <= alpha))
			return tt->score;
	const int staticEval = EvalPosition();
	if (alpha < staticEval)
		alpha = staticEval;
	if (alpha >= beta)
		return beta;
	const int turn = bTurn;
	const int nextTurn = colourToggle(turn);
	const int cx = colourIndex(turn);
	move_t move;
	int score = 0;
	int played = 0;
	SCache sc;
	SMoveList ml;
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
		if (alpha < score)
			alpha = score;
		if (alpha >= beta)
			return score;
	}
	return alpha;
}

static int SearchAlpha(int alpha, int beta, int depth, int ply) {
	if (ply >= MAX_PLY)
		return 0;
	const int turn = bTurn;
	const int nextTurn = colourToggle(turn);
	const int cx = colourIndex(turn);
	const int inCheck = IsSquareAttacked(bKings[cx], nextTurn);
	if (inCheck)
		depth = max(1, depth + 1);
	if (depth <= 0)
		return SearchQuiescence(alpha, beta, depth, ply);
	if (CheckUp())
		return 0;
	const int pvNode = alpha != (beta - 1);
	if (IsRepetition() || bMove50 >= 100)
		return 0;
	TT_Entry* tt_entry = tt + (bHash % TT_MASK);
	if (tt->hash == bHash)
		if (tt->flag == TT_EXACT || (tt->flag == TT_BETA && tt->score >= beta) || (tt->flag == TT_ALPHA && tt->score <= alpha))
			return tt->score;
	const int oAlpha = alpha;
	const int rootNode = ply == 0;
	move_t move;
	int bestScore = -MATE;
	int bestMove = 0;
	int score = 0;
	int legalMoves = 0;
	U64 hash = bHash;
	SCache sc;
	SMoveList ml;
	CacheWrite(&sc);
	InitMoveList(&ml, ALL_MOVES);
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
			return 0;
		if (score > bestScore) {
			bestScore = score;
			bestMove = move;
			if (bestScore > alpha) {
				alpha = bestScore;
				if (rootNode)
					tBestMove = bestMove;
				if (bestScore >= beta) {
					TTPut(hash, TT_BETA, depth, bestScore);
					return bestScore;
				}
			}
		}
	}
	if (!legalMoves)
		return inCheck ? ply - MATE : 0;
	TTPut(hash, alpha > oAlpha ? TT_EXACT : TT_ALPHA, depth, bestScore);
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
		if (depth >= 4) {
			alpha = MAX(-MATE, score - delta);
			beta = MIN(MATE, score + delta);
		}
		while (TRUE) {
			score = SearchAlpha(alpha, beta, depth, 0);
			if (info.stop)
				break;
			if (info.post) {
				printf("info depth %d score ", depth);
				if (abs(score) < MATE - MAX_PLY)
					printf("cp %d", score);
				else
					printf("mate %d", (score > 0 ? (MATE - score + 1) >> 1 : -(MATE + score) >> 1));
				printf(" time %lld", GetTimeMs() - info.timeStart);
				printf(" nodes %lld", info.nodes);
				printf(" hashfull %d pv", Permill());
				printf(" pv %s\n", MoveToUci(tBestMove));
			}
			int scoreOk = TRUE;
			delta += delta / 2;
			if (score <= alpha) {
				scoreOk = FALSE;
				beta = MIN(MATE, ((alpha + beta) / 2));
				alpha = MAX(-MATE, score - delta);
			}
			else if (score >= beta) {
				scoreOk = FALSE;
				alpha = MAX(-MATE, ((alpha + beta) / 2));
				beta = MIN(MATE, score + delta);
			}
			if (scoreOk)
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
	GetHash();
	historyCount = 0;
	historyHash[historyCount++] = bHash;
}

static void ParsePosition(char* ptr) {
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