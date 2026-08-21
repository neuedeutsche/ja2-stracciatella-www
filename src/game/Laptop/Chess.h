#ifndef CHESS_H
#define CHESS_H

#include "Types.h"

void EnterChess(void);
void ExitChess(void);
void RenderChess(void);
void HandleChess(void);

// save-game persistence: 8 bytes carved from the laptop blob's reserve
struct ChessPersist
{
	UINT16 usDay;           // campaign day the daily state below belongs to
	UINT16 usLastSolvedDay; // for the streak; 0 means never solved
	UINT8  ubStreak;
	UINT8  ubBestStreak;
	UINT8  ubHearts;        // attempts left on usDay
	UINT8  ubFlags;         // see the CHESS_FLAG_* bits below
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
