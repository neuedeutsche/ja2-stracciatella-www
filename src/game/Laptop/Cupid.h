#ifndef CUPID_H
#define CUPID_H

#include "Types.h"

#include <string_theory/string>

// Mercs & Kisses - "Where the tough get tender." Speck T. Kline's dating site
// for the A.I.M. and M.E.R.C. rosters, powered (without permission) by the
// I.M.P. personality questionnaire. One member at a time, photo first: the
// deck deals you a card, and you either pass or spend a like. Mutual likes
// become matches. Speck sells you more likes. The matchmaking core lives in
// the engine-free DatingGame; this header is the laptop page API plus the
// save persistence.

void EnterCupid(void);
void ExitCupid(void);
void RenderCupid(void);
void HandleCupid(void);
bool CupidHandleTypedKey(UINT32 usParam, UINT16 usKeyState);
// layout-aware typing for the lounge chat line, via TEXT_INPUT events
bool CupidHandleTextInput(const ST::utf32_buffer& codepoints);

// save-game persistence: 37 bytes carved from the laptop blob's reserve
struct CupidPersist
{
	// the player's 16 I.M.P. quiz answers, two nibbles per byte, low nibble
	// first; 0xF = unanswered. I.M.P. has always thrown these away after
	// compiling the character - this is where they finally get kept.
	UINT8  ubAnswers[8];
	UINT8  ubFlags;        // CUPID_FLAG_* below
	UINT8  ubStreak;       // consecutive days the member logged on
	UINT16 usLastVisitDay;
	UINT16 usViews;        // lifetime profile views (feeds the hit counter)
	INT32  iSpent;         // lifetime dollars into Speck's pocket
	UINT8  ubLiked[8];     // bitmask by profile id: likes the member gave
	UINT8  ubPassed[8];    // bitmask by profile id: cards swiped away
	UINT8  ubLikesLeft;    // free likes remaining today
	UINT16 usDeckDay;      // the day the like allowance belongs to
	// profile "editing", 1999 style: pick your spin from the site's list.
	// low nibble = headline choice + 1, high nibble = summary choice + 1;
	// 0 means the questionnaire's default (and is what old saves carry)
	UINT8  ubSpin;
	// the MARRY verdict: who has been proposed to, and the day of the
	// last proposal (one per day; composure is a virtue)
	UINT8  ubProposed[8];
	UINT16 usProposeDay;
};

#define CUPID_FLAG_SPAM_STAGED 0x01 // the ad campaign has been scheduled
#define CUPID_FLAG_VISITED     0x02 // the member has seen the site
#define CUPID_FLAG_PROFILE     0x04 // a member profile exists
#define CUPID_FLAG_IMP_ANSWERS 0x08 // the answers came from the real I.M.P. quiz
#define CUPID_FLAG_WELCOMED    0x10 // Speck's personal welcome has been sent
#define CUPID_FLAG_FEMALE      0x20 // the site quiz asked; I.M.P. members use their profile
#define CUPID_FLAG_GOLD        0x40 // MERCS & KISSES GOLD: winks never run out
#define CUPID_FLAG_CONDOLED    0x80 // the one condolence letter has been sent

CupidPersist CupidGetPersist();
void CupidSetPersist(const CupidPersist& p);

// Called from I.M.P. the moment a character is compiled: banks the 16 raw quiz
// answers before CompileQuestionsInStatsAndWhatNot's tally throws them away.
void CupidRecordImpAnswers(const INT32 (&answers)[16]);

#endif
