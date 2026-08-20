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
	UINT8  ubFlags;         // bit0 solved, bit1 failed, bit2 hint used, bit3 discovered
};
ChessPersist ChessGetPersist();
void ChessSetPersist(const ChessPersist& p);

#endif
