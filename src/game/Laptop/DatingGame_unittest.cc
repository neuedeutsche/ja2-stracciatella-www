// Mercs & Kisses: unit tests for the engine-free matchmaking core.

#include "DatingGame.h"

#include <gtest/gtest.h>

#include <set>

using namespace DatingGame;

namespace
{
	Profile MakeProfile(int8_t att, int8_t trait, uint32_t seed,
				int8_t sex = SEX_MALE, int8_t sexist = SX_NOT_SEXIST)
	{
		Profile p;
		p.attitude = att;
		p.trait    = trait;
		p.sex      = sex;
		p.sexist   = sexist;
		DeriveAnswers(seed, att, trait, p.answers);
		return p;
	}
}

TEST(DatingGame, voteTablesMatchAnswerCounts)
{
	// every vote index must be inside the question's real answer range, and
	// out-of-range lookups must be inert
	for (int q = 0; q < NUM_QUESTIONS; ++q)
	{
		ASSERT_GE(ANSWER_COUNT[q], 4);
		ASSERT_LE(ANSWER_COUNT[q], 8);
		EXPECT_EQ(AttitudeVote(q, ANSWER_COUNT[q]), -1);
		EXPECT_EQ(TraitVote(q, ANSWER_COUNT[q]), TRAIT_NONE);
		EXPECT_EQ(AttitudeVote(q, -1), -1);
	}
	EXPECT_EQ(AttitudeVote(-1, 0), -1);
	EXPECT_EQ(AttitudeVote(NUM_QUESTIONS, 0), -1);
}

TEST(DatingGame, everyImpAttitudeHasTwoVotingQuestions)
{
	// the derivation's plurality guarantee rests on this property; if a mod
	// ever rewrites the quiz, this is the test that says so
	for (int8_t att = 0; att < NUM_ATTITUDES; ++att)
	{
		if (att == ATT_BIG_SHOT) continue; // I.M.P. cannot produce it either
		int voters = 0;
		for (int q = 0; q < NUM_QUESTIONS; ++q)
		{
			for (int a = 0; a < ANSWER_COUNT[q]; ++a)
			{
				if (AttitudeVote(q, a) == att) { ++voters; break; }
			}
		}
		EXPECT_GE(voters, 2) << "attitude " << int(att);
	}
}

TEST(DatingGame, derivedAnswersAreInRange)
{
	for (uint32_t seed = 0; seed < 60; ++seed)
	{
		uint8_t answers[NUM_QUESTIONS];
		DeriveAnswers(seed, ATT_NORMAL, TRAIT_NONE, answers);
		for (int q = 0; q < NUM_QUESTIONS; ++q)
		{
			EXPECT_LT(answers[q], ANSWER_COUNT[q]);
		}
	}
}

TEST(DatingGame, derivationIsDeterministic)
{
	uint8_t a[NUM_QUESTIONS], b[NUM_QUESTIONS];
	DeriveAnswers(14, ATT_LONER, TRAIT_PSYCHO, a);
	DeriveAnswers(14, ATT_LONER, TRAIT_PSYCHO, b);
	for (int q = 0; q < NUM_QUESTIONS; ++q) EXPECT_EQ(a[q], b[q]);
}

TEST(DatingGame, derivationVariesBySeed)
{
	// different mercs with the same temperament must not share a sheet
	std::set<uint32_t> sheets;
	for (uint32_t seed = 0; seed < 40; ++seed)
	{
		uint8_t answers[NUM_QUESTIONS];
		DeriveAnswers(seed, ATT_NORMAL, TRAIT_NONE, answers);
		uint32_t hash = 0;
		for (int q = 0; q < NUM_QUESTIONS; ++q) hash = hash * 31 + answers[q];
		sheets.insert(hash);
	}
	EXPECT_GT(sheets.size(), 30u);
}

TEST(DatingGame, derivedAnswersTallyToTheCanonAttitude)
{
	// the doc's validation requirement: a merc's sheet must compile back to
	// their actual attitude under I.M.P.'s own vote counting, every time
	for (int8_t att = 0; att < NUM_ATTITUDES; ++att)
	{
		if (att == ATT_BIG_SHOT) continue;
		for (int8_t trait = 0; trait < NUM_TRAITS; ++trait)
		{
			for (uint32_t seed = 0; seed < 25; ++seed)
			{
				uint8_t answers[NUM_QUESTIONS];
				DeriveAnswers(seed, att, trait, answers);
				EXPECT_EQ(TallyAttitude(answers), att)
					<< "att " << int(att) << " trait " << int(trait)
					<< " seed " << seed;
			}
		}
	}
}

TEST(DatingGame, derivedSheetCarriesTheTraitWherePossible)
{
	// traits with a voting answer must appear on the sheet; NONSWIMMER and
	// FEAR_OF_INSECTS have no quiz question and are excused
	const int8_t votable[] = { TRAIT_HEAT_INTOLERANT, TRAIT_NERVOUS,
		TRAIT_CLAUSTROPHOBIC, TRAIT_FORGETFUL, TRAIT_PSYCHO };
	for (int8_t trait : votable)
	{
		uint8_t answers[NUM_QUESTIONS];
		DeriveAnswers(7, ATT_NORMAL, trait, answers);
		bool found = false;
		for (int q = 0; q < NUM_QUESTIONS && !found; ++q)
		{
			found = TraitVote(q, answers[q]) == trait;
		}
		EXPECT_TRUE(found) << "trait " << int(trait);
	}
}

TEST(DatingGame, matchIsSymmetric)
{
	for (uint32_t s = 0; s < 20; ++s)
	{
		const Profile a = MakeProfile(ATT_LONER, TRAIT_NONE, s, SEX_MALE);
		const Profile b = MakeProfile(ATT_FRIENDLY, TRAIT_NERVOUS, s + 100,
						SEX_FEMALE);
		const uint32_t seed = ChemistrySeed(s, s + 100);
		const Match ab = Compute(a, b, seed);
		const Match ba = Compute(b, a, seed);
		EXPECT_EQ(ab.percent, ba.percent);
		EXPECT_EQ(ab.agree, ba.agree);
		EXPECT_EQ(ab.dealbreaker, ba.dealbreaker);
	}
}

TEST(DatingGame, chemistrySeedIsOrderIndependent)
{
	EXPECT_EQ(ChemistrySeed(3, 41), ChemistrySeed(41, 3));
	EXPECT_NE(ChemistrySeed(3, 41), ChemistrySeed(3, 42));
}

TEST(DatingGame, percentStaysOnTheDial)
{
	for (uint32_t s = 0; s < 200; ++s)
	{
		const Profile a = MakeProfile(int8_t(s % NUM_ATTITUDES),
						int8_t(s % NUM_TRAITS), s);
		const Profile b = MakeProfile(int8_t((s + 3) % NUM_ATTITUDES),
						int8_t((s + 5) % NUM_TRAITS), s + 999);
		const Match m = Compute(a, b, ChemistrySeed(s, s + 999));
		EXPECT_GE(m.percent, 1);
		EXPECT_LE(m.percent, 99); // nobody is 100; the site says so
	}
}

TEST(DatingGame, identicalSheetsScoreHigh)
{
	Profile a = MakeProfile(ATT_OPTIMIST, TRAIT_NONE, 5);
	Profile b = a;
	const Match m = Compute(a, b, ChemistrySeed(1, 2));
	EXPECT_EQ(m.agree, NUM_QUESTIONS);
	EXPECT_GE(m.percent, 90);
	EXPECT_EQ(m.uncertainty, 0);
}

TEST(DatingGame, onesidedPsychoIsADealbreaker)
{
	const Profile a = MakeProfile(ATT_NORMAL, TRAIT_PSYCHO, 1);
	const Profile b = MakeProfile(ATT_NORMAL, TRAIT_NONE, 2);
	EXPECT_EQ(Compute(a, b, 9).dealbreaker, DEAL_PSYCHO);

	// two of them at least know what they are getting
	const Profile c = MakeProfile(ATT_NORMAL, TRAIT_PSYCHO, 3);
	EXPECT_EQ(Compute(a, c, 9).dealbreaker, DEAL_NONE);
}

TEST(DatingGame, verySexistVersusWomanIsADealbreaker)
{
	const Profile him = MakeProfile(ATT_NORMAL, TRAIT_NONE, 1, SEX_MALE,
					SX_VERY_SEXIST);
	const Profile her = MakeProfile(ATT_NORMAL, TRAIT_NONE, 2, SEX_FEMALE);
	const Match m = Compute(him, her, 4);
	EXPECT_EQ(m.dealbreaker, DEAL_SEXIST);
	// and it costs him: same pairing without the attitude scores higher
	const Profile decent = MakeProfile(ATT_NORMAL, TRAIT_NONE, 1, SEX_MALE);
	EXPECT_GT(Compute(decent, her, 4).percent, m.percent);
}

TEST(DatingGame, missingAnswersWidenTheErrorBars)
{
	Profile full = MakeProfile(ATT_NORMAL, TRAIT_NONE, 8);
	Profile blank;
	blank.attitude = ATT_NORMAL;
	EXPECT_FALSE(blank.HasAnswers());
	EXPECT_TRUE(full.HasAnswers());

	const Match partial = Compute(full, blank, 3);
	EXPECT_EQ(partial.answered, 0);
	EXPECT_EQ(partial.uncertainty, 12); // the 60%-complete nag is honest
	const Match complete = Compute(full, MakeProfile(ATT_NORMAL, TRAIT_NONE, 9), 3);
	EXPECT_EQ(complete.uncertainty, 0);
}

TEST(DatingGame, agreeAndDisagreeLinesAreReported)
{
	Profile a = MakeProfile(ATT_NORMAL, TRAIT_NONE, 11);
	Profile b = a;
	b.answers[4] = uint8_t(b.answers[4] == 0 ? 1 : 0); // force one disagreement
	const Match m = Compute(a, b, 6);
	EXPECT_GE(m.bestQ, 0);
	EXPECT_EQ(m.worstQ, 4);
	EXPECT_EQ(m.agree, NUM_QUESTIONS - 1);
}

TEST(DatingGame, tallyOfBlankSheetIsNormal)
{
	uint8_t answers[NUM_QUESTIONS];
	for (int q = 0; q < NUM_QUESTIONS; ++q) answers[q] = NO_ANSWER;
	// mirrors CreatePlayerAttitude: an empty vote list means ATT_NORMAL
	EXPECT_EQ(TallyAttitude(answers), ATT_NORMAL);
}
