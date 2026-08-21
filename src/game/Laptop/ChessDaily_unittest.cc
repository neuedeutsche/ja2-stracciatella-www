#include "gtest/gtest.h"

#include "ChessDaily.h"
#include "ChessPuzzles.h"

using ChessDaily::State;

TEST(ChessDaily, PuzzleIndexIsOneBasedAndWraps)
{
	EXPECT_EQ(0, ChessDaily::PuzzleIndexForDay(1, 100));
	EXPECT_EQ(7, ChessDaily::PuzzleIndexForDay(8, 100));
	EXPECT_EQ(99, ChessDaily::PuzzleIndexForDay(100, 100));
	// the run wraps rather than walking off the end of the corpus
	EXPECT_EQ(0, ChessDaily::PuzzleIndexForDay(101, 100));
	EXPECT_EQ(49, ChessDaily::PuzzleIndexForDay(150, 100));
	// day 0 can be reported by a save loaded before the clock starts
	EXPECT_EQ(0, ChessDaily::PuzzleIndexForDay(0, 100));
	EXPECT_EQ(0, ChessDaily::PuzzleIndexForDay(-5, 100));
	// and an empty corpus must not divide by zero
	EXPECT_EQ(0, ChessDaily::PuzzleIndexForDay(8, 0));
}

TEST(ChessDaily, EveryCorpusDayResolvesToARealPuzzle)
{
	for (int day = 1; day <= NUM_CHESS_PUZZLES * 2 + 3; ++day)
	{
		const int index = ChessDaily::PuzzleIndexForDay(day, NUM_CHESS_PUZZLES);
		ASSERT_GE(index, 0) << "day " << day;
		ASSERT_LT(index, NUM_CHESS_PUZZLES) << "day " << day;
	}
}

TEST(ChessDaily, RollOverResetsTheDayButNotThePlayer)
{
	State s;
	s.day    = 8;
	s.hearts = 1;
	s.flags  = ChessDaily::FLAG_SOLVED | ChessDaily::FLAG_HINT_USED |
	           ChessDaily::FLAG_DISCOVERED | ChessDaily::FLAG_INVITED |
	           ChessDaily::FLAG_DOWN_NOTED | ChessDaily::FLAG_SIGNED |
	           ChessDaily::FLAG_CROWN_ASKED;

	EXPECT_FALSE(ChessDaily::RollOverDay(s, 8));  // same day is a no-op
	EXPECT_EQ(1, s.hearts);

	EXPECT_TRUE(ChessDaily::RollOverDay(s, 9));
	EXPECT_EQ(9, s.day);
	EXPECT_EQ(ChessDaily::MAX_HEARTS, s.hearts);
	EXPECT_FALSE(s.flags & ChessDaily::FLAG_SOLVED);
	EXPECT_FALSE(s.flags & ChessDaily::FLAG_HINT_USED);
	// these describe the player and have to survive the night
	EXPECT_TRUE(s.flags & ChessDaily::FLAG_DISCOVERED);
	EXPECT_TRUE(s.flags & ChessDaily::FLAG_INVITED);
	EXPECT_TRUE(s.flags & ChessDaily::FLAG_DOWN_NOTED)
		<< "an outage spans days, so its notice must not re-send every morning";
	EXPECT_TRUE(s.flags & ChessDaily::FLAG_SIGNED)
		<< "a guestbook signature is forever";
	EXPECT_TRUE(s.flags & ChessDaily::FLAG_CROWN_ASKED)
		<< "he only explains the crown once";
}

TEST(ChessDaily, StreakCountsConsecutiveDaysOnly)
{
	State s;

	ChessDaily::RecordSolved(s, 4);
	EXPECT_EQ(1, s.streak);
	EXPECT_EQ(1, s.bestStreak);

	ChessDaily::RecordSolved(s, 5);
	EXPECT_EQ(2, s.streak);

	ChessDaily::RecordSolved(s, 6);
	EXPECT_EQ(3, s.streak);
	EXPECT_EQ(3, s.bestStreak);

	// day 7 skipped: the run restarts at one, and the best is remembered
	ChessDaily::RecordSolved(s, 8);
	EXPECT_EQ(1, s.streak);
	EXPECT_EQ(3, s.bestStreak);

	// solving the same day twice must not inflate the streak
	ChessDaily::RecordSolved(s, 8);
	EXPECT_EQ(1, s.streak);
}

TEST(ChessDaily, StreakSurvivesTheDayOneEdge)
{
	State s;
	// lastSolvedDay 0 means never solved, so day 1 must not read as a run
	ChessDaily::RecordSolved(s, 1);
	EXPECT_EQ(1, s.streak);
	ChessDaily::RecordSolved(s, 2);
	EXPECT_EQ(2, s.streak);
}

TEST(ChessDaily, StreakSaturatesRatherThanWrapping)
{
	State s;
	s.streak        = 255;
	s.bestStreak    = 255;
	s.lastSolvedDay = 300;
	ChessDaily::RecordSolved(s, 301);
	EXPECT_EQ(255, s.streak) << "a byte-wide streak must clamp, not roll to zero";
}

TEST(ChessDaily, FailingBreaksTheStreak)
{
	State s;
	ChessDaily::RecordSolved(s, 4);
	ChessDaily::RecordSolved(s, 5);
	EXPECT_EQ(2, s.streak);

	ChessDaily::RecordFailed(s);
	EXPECT_EQ(0, s.streak);
	EXPECT_EQ(2, s.bestStreak) << "the best stands even after a loss";
	EXPECT_TRUE(ChessDaily::IsFinished(s));
}

TEST(ChessDaily, HeartsRunOutExactlyOnce)
{
	State s;
	for (int i = 1; i < ChessDaily::MAX_HEARTS; ++i)
	{
		EXPECT_FALSE(ChessDaily::SpendHeart(s)) << "try " << i;
		EXPECT_EQ(ChessDaily::MAX_HEARTS - i, s.hearts);
	}
	EXPECT_TRUE(ChessDaily::SpendHeart(s)) << "the last try ends the day";
	EXPECT_EQ(0, s.hearts);
	// spending past zero must not report a second ending
	EXPECT_FALSE(ChessDaily::SpendHeart(s));
	EXPECT_EQ(0, s.hearts);
}

TEST(ChessDaily, IsFinishedTracksBothOutcomes)
{
	State s;
	EXPECT_FALSE(ChessDaily::IsFinished(s));
	ChessDaily::RecordSolved(s, 3);
	EXPECT_TRUE(ChessDaily::IsFinished(s));

	State t;
	ChessDaily::RecordFailed(t);
	EXPECT_TRUE(ChessDaily::IsFinished(t));
}

// A full run through a campaign week, which is the behaviour a player actually
// experiences: solve, solve, miss a day, fail, recover.
TEST(ChessDaily, AWeekOfPlay)
{
	State s;
	s.flags |= ChessDaily::FLAG_DISCOVERED;

	ChessDaily::RollOverDay(s, 1);
	ChessDaily::RecordSolved(s, 1);
	EXPECT_EQ(1, s.streak);

	ChessDaily::RollOverDay(s, 2);
	EXPECT_EQ(ChessDaily::MAX_HEARTS, s.hearts);
	ChessDaily::RecordSolved(s, 2);
	EXPECT_EQ(2, s.streak);

	// day 3 never visited; day 4 is opened and lost
	ChessDaily::RollOverDay(s, 4);
	for (int i = 0; i < ChessDaily::MAX_HEARTS; ++i) ChessDaily::SpendHeart(s);
	ChessDaily::RecordFailed(s);
	EXPECT_EQ(0, s.streak);
	EXPECT_EQ(2, s.bestStreak);

	ChessDaily::RollOverDay(s, 5);
	EXPECT_FALSE(ChessDaily::IsFinished(s)) << "a new day is playable again";
	ChessDaily::RecordSolved(s, 5);
	EXPECT_EQ(1, s.streak) << "day 4 was failed, not solved, so this is a fresh run";
	EXPECT_TRUE(s.flags & ChessDaily::FLAG_DISCOVERED);
}

TEST(ChessDaily, LapsedStreakOnlyFiresWhenADayWentBy)
{
	State s;
	ChessDaily::RecordSolved(s, 5);
	ChessDaily::RecordSolved(s, 6);
	ChessDaily::RecordSolved(s, 7);
	EXPECT_EQ(3, s.streak);

	// still day 7, or arriving on day 8 with yesterday solved: the run stands
	EXPECT_EQ(0, ChessDaily::LapsedStreak(s, 7));
	EXPECT_EQ(0, ChessDaily::LapsedStreak(s, 8));

	// arriving on day 9 with nothing solved since day 7: the run is dead
	EXPECT_EQ(3, ChessDaily::LapsedStreak(s, 9));
	EXPECT_EQ(3, ChessDaily::LapsedStreak(s, 40));

	ChessDaily::ClearStreak(s);
	EXPECT_EQ(0, s.streak);
	EXPECT_EQ(0, ChessDaily::LapsedStreak(s, 40));
	EXPECT_EQ(3, s.bestStreak) << "clearing the run leaves the record alone";
}

TEST(ChessDaily, NeverSolvedHasNoRunToLose)
{
	State s;
	EXPECT_EQ(0, ChessDaily::LapsedStreak(s, 50));
}
