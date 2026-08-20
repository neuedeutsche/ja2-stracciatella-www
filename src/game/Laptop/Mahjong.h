#ifndef MAHJONG_H
#define MAHJONG_H

#include "Types.h"

void EnterMahjong(void);
void ExitMahjong(void);
void RenderMahjong(void);
void HandleMahjong(void);

// chat input: returns true if the key was consumed by the chat line
bool MahjongHandleTypedKey(UINT32 usParam, UINT16 usKeyState);

// save-game persistence: 16 bytes carved from the laptop blob's reserve
struct MahjongPersist
{
	UINT16 usMatches;
	UINT16 usMatchesWon;
	UINT16 usHandsWon;
	INT32 iBiggestHand;
	INT32 iDollarsNet;
	UINT8 ubFlags; // bit0 elliot-secret sent, bits1-3 elliot good nights
	UINT8 ubGrudge;
};
MahjongPersist MahjongGetPersist();
void MahjongSetPersist(const MahjongPersist& p);

#endif
