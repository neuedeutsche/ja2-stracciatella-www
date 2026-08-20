#ifndef CHESSDAILY_H
#define CHESSDAILY_H

#include <cstdint>

// Daily-puzzle bookkeeping for chach.com: which puzzle a campaign day gets,
// what a try costs, and when a streak survives.
//
// No JA2 headers on purpose. Every function here is a pure transform of the
// state handed to it, so the rules can be tested directly instead of being
// inferred from what the page happens to draw.
namespace ChessDaily
{
	constexpr int MAX_HEARTS = 5;

	constexpr std::uint8_t FLAG_SOLVED     = 0x01;  // today's puzzle is done
	constexpr std::uint8_t FLAG_FAILED     = 0x02;  // today's tries are spent
	constexpr std::uint8_t FLAG_HINT_USED  = 0x04;  // the hint is gone for today
	constexpr std::uint8_t FLAG_DISCOVERED = 0x08;  // the player has been here
	constexpr std::uint8_t FLAG_INVITED    = 0x10;  // the invitation has gone out

	// Eight bytes, matching the laptop save blob exactly.
	struct State
	{
		std::uint16_t day           = 0;  // the campaign day the flags describe
		std::uint16_t lastSolvedDay = 0;  // 0 means never solved
		std::uint8_t  streak        = 0;
		std::uint8_t  bestStreak    = 0;
		std::uint8_t  hearts        = MAX_HEARTS;
		std::uint8_t  flags         = 0;
	};

	// Day 1 gets puzzle 0, and the run wraps once the corpus is exhausted.
	// Day 0 is treated as day 1: the campaign clock is one-based, but a save
	// loaded before the clock starts can still report zero.
	int PuzzleIndexForDay(int day, int puzzleCount);

	// Move the state on to `today` if it is not already there. Tries and the
	// per-day flags reset; the flags that describe the player rather than the
	// day survive. Returns true when the day actually changed.
	bool RollOverDay(State& state, int today);

	// A streak counts consecutive days solved, so it continues only when the
	// previous day was the one solved last, and a day missed ends it.
	void RecordSolved(State& state, int today);

	void RecordFailed(State& state);

	// Spend one try. Returns true when that was the last one, which is the
	// caller's cue to end the day.
	bool SpendHeart(State& state);

	// Today is over, one way or the other.
	bool IsFinished(const State& state);
}

#endif
