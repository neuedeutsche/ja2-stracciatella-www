#include "ChessDaily.h"

#include <algorithm>

namespace ChessDaily
{
	int PuzzleIndexForDay(int day, int puzzleCount)
	{
		if (puzzleCount <= 0) return 0;
		const int ordinal = (day <= 0 ? 1 : day) - 1;
		return ordinal % puzzleCount;
	}

	bool RollOverDay(State& state, int today)
	{
		const std::uint16_t stamp = std::uint16_t(today < 0 ? 0 : today);
		if (state.day == stamp) return false;
		state.day    = stamp;
		state.hearts = MAX_HEARTS;
		// FLAG_DISCOVERED and FLAG_INVITED describe the player, not the day
		state.flags &= std::uint8_t(FLAG_DISCOVERED | FLAG_INVITED);
		return true;
	}

	void RecordSolved(State& state, int today)
	{
		const std::uint16_t stamp = std::uint16_t(today < 0 ? 0 : today);
		const bool consecutive = state.lastSolvedDay != 0 &&
		                         state.lastSolvedDay + 1 == stamp;
		state.streak = consecutive
			? std::uint8_t(std::min(int(state.streak) + 1, 255))
			: std::uint8_t(1);
		state.bestStreak    = std::max(state.bestStreak, state.streak);
		state.lastSolvedDay = stamp;
		state.flags        |= FLAG_SOLVED;
	}

	void RecordFailed(State& state)
	{
		state.flags |= FLAG_FAILED;
		state.streak = 0;
	}

	bool SpendHeart(State& state)
	{
		if (state.hearts == 0) return false;
		--state.hearts;
		return state.hearts == 0;
	}

	bool IsFinished(const State& state)
	{
		return (state.flags & (FLAG_SOLVED | FLAG_FAILED)) != 0;
	}
}
