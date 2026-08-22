#include "gtest/gtest.h"

#include "FelineCat.h"

using FelineCat::State;

TEST(FelineCat, HungerRisesTwelvePointsPerUnfedDay)
{
	State s;
	s.lastFedDay = 10;
	FelineCat::RollDay(s, 10);
	EXPECT_EQ(0, s.hunger);
	FelineCat::RollDay(s, 12);
	EXPECT_EQ(24, s.hunger);
	FelineCat::RollDay(s, 17);
	EXPECT_EQ(84, s.hunger);
	// and it caps rather than wrapping
	FelineCat::RollDay(s, 40);
	EXPECT_EQ(100, s.hunger);
}

TEST(FelineCat, BrendaFeedsFromTheShelfEverySecondDay)
{
	State s;
	s.lastFedDay = 0;
	s.supplies = 3;
	FelineCat::RollDay(s, 7);
	// three tins cover days 2, 4 and 6; one hungry day remains
	EXPECT_EQ(0, s.supplies);
	EXPECT_EQ(6, s.lastFedDay);
	EXPECT_EQ(12, s.hunger);
	EXPECT_FALSE(s.away);
}

TEST(FelineCat, FeedingByHandConsumesATinAndResets)
{
	State s;
	s.lastFedDay = 0;
	s.supplies = 1;
	FelineCat::RollDay(s, 1);
	EXPECT_EQ(12, s.hunger);
	EXPECT_TRUE(FelineCat::Feed(s, 1));
	EXPECT_EQ(0, s.hunger);
	EXPECT_EQ(1, s.lastFedDay);
	EXPECT_EQ(0, s.supplies);
	// the shelf is bare now
	EXPECT_FALSE(FelineCat::Feed(s, 2));
}

TEST(FelineCat, TwelveDaysPastEmptySendsTheCatToTheCountry)
{
	State s;
	s.lastFedDay = 0;
	FelineCat::RollDay(s, 11);
	EXPECT_FALSE(s.away);
	FelineCat::RollDay(s, 12);
	EXPECT_TRUE(s.away);
	// an away cat cannot be fed, only recovered
	s.supplies = 5;
	EXPECT_FALSE(FelineCat::Feed(s, 12));
	FelineCat::Recover(s, 12);
	EXPECT_FALSE(s.away);
	EXPECT_EQ(0, s.hunger);
	EXPECT_TRUE(FelineCat::Feed(s, 13));
}

TEST(FelineCat, MoodLadderInPriorityOrder)
{
	State s;
	s.lastFedDay = 0;
	s.lastVisitDay = 20;

	s.away = true;
	EXPECT_EQ(FelineCat::MOOD_AWAY, FelineCat::MoodOf(s, 20, true));
	s.away = false;

	// artillery outranks appetite
	s.hunger = 100;
	EXPECT_EQ(FelineCat::MOOD_HIDING, FelineCat::MoodOf(s, 20, true));
	EXPECT_EQ(FelineCat::MOOD_THIN, FelineCat::MoodOf(s, 20, false));

	s.hunger = 50;
	EXPECT_EQ(FelineCat::MOOD_HUNGRY, FelineCat::MoodOf(s, 20, false));

	s.hunger = 30;
	s.lastVisitDay = 10;
	EXPECT_EQ(FelineCat::MOOD_LONELY, FelineCat::MoodOf(s, 20, false));
	s.lastVisitDay = 18;
	EXPECT_EQ(FelineCat::MOOD_CONTENT, FelineCat::MoodOf(s, 20, false));

	s.hunger = 5;
	EXPECT_EQ(FelineCat::MOOD_PLAYFUL, FelineCat::MoodOf(s, 20, false));
}

TEST(FelineCat, PosesStayInsideTheMoodVocabulary)
{
	for (int m = 0; m < FelineCat::MOOD_COUNT; ++m)
	{
		for (uint32_t seed = 0; seed < 9; ++seed)
		{
			const FelineCat::Pose p =
				FelineCat::PoseFor(FelineCat::Mood(m), seed);
			ASSERT_GE(p, 0);
			ASSERT_LT(p, FelineCat::POSE_COUNT);
		}
	}
	// a hiding cat crouches; it does not play
	for (uint32_t seed = 0; seed < 9; ++seed)
	{
		const FelineCat::Pose p =
			FelineCat::PoseFor(FelineCat::MOOD_HIDING, seed);
		EXPECT_TRUE(p == FelineCat::POSE_CROUCH ||
				p == FelineCat::POSE_STARE);
	}
	// an away cat is only ever asleep, somewhere else
	for (uint32_t seed = 0; seed < 9; ++seed)
	{
		EXPECT_EQ(FelineCat::POSE_SLEEP,
				FelineCat::PoseFor(FelineCat::MOOD_AWAY, seed));
	}
}
