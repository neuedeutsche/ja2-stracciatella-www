#ifndef CHESS_H
#define CHESS_H

#include "Types.h"

#include <string_theory/string>

void EnterChess(void);
bool ChessHandleTypedKey(UINT32 usParam, UINT16 usKeyState);
bool ChessHandleTextInput(const ST::utf32_buffer& codepoints);
void ExitChess(void);
void RenderChess(void);
void HandleChess(void);

// one finished live game, as the profile page remembers it
struct ChessGameRec
{
	UINT16 usDay;     // campaign day it ended
	UINT8  ubSeat;    // index into the seat ladder; 0xFE = the daily man
	UINT8  ubResult;  // bits 0-1: 0 loss, 1 draw, 2 win; bit 2: resigned
	UINT8  ubMoves;   // full moves, capped at 255
	UINT8  ubControl; // minutes; 0 = daily
};
#define CHESS_HIST_MAX 10

// save-game persistence: 8 bytes carved from the laptop blob's reserve.
// The rating and game ledger ride a marker-gated tail behind the fixed
// block (0xC6 in place of the old 0xC5), so old saves read as unrated.
struct ChessPersist
{
	UINT16 usDay;           // campaign day the daily state below belongs to
	UINT16 usLastSolvedDay; // for the streak; 0 means never solved
	UINT8  ubStreak;
	UINT8  ubBestStreak;
	UINT8  ubHearts;        // attempts left on usDay
	UINT8  ubFlags;         // see the CHESS_FLAG_* bits below
	char   szLine[121];     // the signed guestbook line; empty = the stock one
	UINT16 usRating;        // 0 until the first live game finishes
	UINT16 usWins;
	UINT16 usLosses;
	UINT16 usDraws;
	UINT8  ubHistCount;
	ChessGameRec aHist[CHESS_HIST_MAX]; // [0] is the most recent
};

// Mirrors ChessDaily's flag bits, exposed so the laptop can gate discovery
// without pulling the whole daily module in.
#define CHESS_FLAG_SOLVED     0x01
#define CHESS_FLAG_FAILED     0x02
#define CHESS_FLAG_HINT_USED  0x04
#define CHESS_FLAG_DISCOVERED 0x08
#define CHESS_FLAG_INVITED    0x10
#define CHESS_FLAG_DOWN_NOTED 0x20
#define CHESS_FLAG_SIGNED     0x40
#define CHESS_FLAG_CROWN     0x80
ChessPersist ChessGetPersist();
void ChessSetPersist(const ChessPersist& p);

#endif
