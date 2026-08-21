// Mercs & Kisses: the engine-free matchmaking core. See DatingGame.h.

#include "DatingGame.h"

#include <algorithm>

namespace DatingGame
{

// --- the quiz vote tables --------------------------------------------------
// Transcribed from CompileQuestionsInStatsAndWhatNot() in
// IMP_Personality_Quiz.cc. An entry of -1 (attitude) or TRAIT_NONE (trait)
// means the answer votes for a skill or for nothing at all. Question 15 is
// pure filler - I.M.P. asks it and ignores every possible reply, which the
// site's fine print is contractually obliged to find romantic.

const uint8_t ANSWER_COUNT[NUM_QUESTIONS] =
	{ 6, 4, 4, 5, 4, 5, 4, 4, 4, 4, 5, 8, 4, 4, 4, 4 };

namespace
{
	constexpr int8_t A_ = -1; // no attitude vote

	const int8_t kAttVote[NUM_QUESTIONS][8] =
	{
		/* q0  */ { A_, ATT_LONER, A_, A_, A_, ATT_OPTIMIST, A_, A_ },
		/* q1  */ { A_, A_, A_, ATT_FRIENDLY, A_, A_, A_, A_ },
		/* q2  */ { A_, ATT_ARROGANT, A_, ATT_NORMAL, A_, A_, A_, A_ },
		/* q3  */ { A_, ATT_FRIENDLY, ATT_NORMAL, ATT_ASSHOLE, ATT_LONER, A_, A_, A_ },
		/* q4  */ { ATT_COWARD, A_, ATT_AGGRESSIVE, A_, A_, A_, A_, A_ },
		/* q5  */ { ATT_COWARD, A_, A_, A_, A_, A_, A_, A_ },
		/* q6  */ { A_, A_, A_, A_, A_, A_, A_, A_ },
		/* q7  */ { A_, A_, ATT_OPTIMIST, A_, A_, A_, A_, A_ },
		/* q8  */ { A_, A_, ATT_PESSIMIST, A_, A_, A_, A_, A_ },
		/* q9  */ { A_, ATT_PESSIMIST, ATT_ASSHOLE, A_, A_, A_, A_, A_ },
		/* q10 */ { A_, A_, ATT_AGGRESSIVE, ATT_NORMAL, A_, A_, A_, A_ },
		/* q11 */ { A_, A_, A_, A_, A_, A_, A_, A_ },
		/* q12 */ { A_, ATT_NORMAL, ATT_NORMAL, A_, A_, A_, A_, A_ },
		/* q13 */ { A_, ATT_NORMAL, A_, A_, A_, A_, A_, A_ },
		/* q14 */ { A_, A_, ATT_ARROGANT, A_, A_, A_, A_, A_ },
		/* q15 */ { A_, A_, A_, A_, A_, A_, A_, A_ },
	};

	const int8_t kTraitVote[NUM_QUESTIONS][8] =
	{
		/* q0  */ { 0, 0, 0, 0, 0, 0, 0, 0 },
		/* q1  */ { 0, 0, TRAIT_PSYCHO, 0, 0, 0, 0, 0 },
		/* q2  */ { 0, 0, 0, 0, 0, 0, 0, 0 },
		/* q3  */ { 0, 0, 0, 0, 0, 0, 0, 0 },
		/* q4  */ { 0, 0, 0, 0, 0, 0, 0, 0 },
		/* q5  */ { 0, 0, TRAIT_CLAUSTROPHOBIC, 0, 0, 0, 0, 0 },
		/* q6  */ { 0, 0, 0, 0, 0, 0, 0, 0 },
		/* q7  */ { 0, 0, 0, TRAIT_PSYCHO, 0, 0, 0, 0 },
		/* q8  */ { TRAIT_FORGETFUL, 0, 0, TRAIT_NERVOUS, 0, 0, 0, 0 },
		/* q9  */ { 0, 0, 0, TRAIT_NERVOUS, 0, 0, 0, 0 },
		/* q10 */ { 0, 0, 0, 0, 0, 0, 0, 0 },
		/* q11 */ { 0, 0, 0, 0, 0, 0, 0, 0 },
		/* q12 */ { TRAIT_FORGETFUL, 0, 0, TRAIT_HEAT_INTOLERANT, 0, 0, 0, 0 },
		/* q13 */ { TRAIT_CLAUSTROPHOBIC, 0, TRAIT_HEAT_INTOLERANT, 0, 0, 0, 0, 0 },
		/* q14 */ { 0, 0, 0, 0, 0, 0, 0, 0 },
		/* q15 */ { 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	// xorshift32: tiny, seedable, good enough for picking questionnaire
	// answers; never returns 0 for a non-zero seed
	struct Rng
	{
		uint32_t s;
		explicit Rng(uint32_t seed) : s(seed ? seed : 0x9E3779B9u) {}
		uint32_t Next()
		{
			s ^= s << 13; s ^= s >> 17; s ^= s << 5;
			return s;
		}
		uint32_t Below(uint32_t n) { return n ? Next() % n : 0; }
	};
}

int8_t AttitudeVote(int question, int answer)
{
	if (question < 0 || question >= NUM_QUESTIONS) return -1;
	if (answer < 0 || answer >= ANSWER_COUNT[question]) return -1;
	return kAttVote[question][answer];
}

int8_t TraitVote(int question, int answer)
{
	if (question < 0 || question >= NUM_QUESTIONS) return TRAIT_NONE;
	if (answer < 0 || answer >= ANSWER_COUNT[question]) return TRAIT_NONE;
	return kTraitVote[question][answer];
}

bool Profile::HasAnswers() const
{
	for (int q = 0; q < NUM_QUESTIONS; ++q)
	{
		if (answers[q] != NO_ANSWER) return true;
	}
	return false;
}

void DeriveAnswers(uint32_t seed, int8_t attitude, int8_t trait,
			uint8_t answers[NUM_QUESTIONS])
{
	// Knuth's multiplicative hash spreads consecutive profile ids apart so
	// neighbouring mercs do not answer in lockstep
	Rng rng(seed * 2654435761u + 1);

	int attCount[NUM_ATTITUDES] = {};

	for (int q = 0; q < NUM_QUESTIONS; ++q) answers[q] = NO_ANSWER;

	// The trait places first - some traits vote on the same questions an
	// attitude does (HEAT_INTOLERANT and ATT_NORMAL share both of theirs),
	// and the attitude can spare a question far more easily than the trait
	// can. The chosen answer never votes for an attitude, so the tally below
	// is untouched.
	if (trait != TRAIT_NONE)
	{
		for (int q = 0; q < NUM_QUESTIONS && trait != TRAIT_NONE; ++q)
		{
			for (int a = 0; a < ANSWER_COUNT[q]; ++a)
			{
				if (kTraitVote[q][a] == trait && kAttVote[q][a] < 0)
				{
					answers[q] = uint8_t(a);
					trait = TRAIT_NONE; // placed; stop looking
					break;
				}
			}
		}
	}

	// Then force the canonical attitude on every still-free question that
	// votes for it. Every attitude keeps at least one voter even after the
	// trait takes a question, and the second pass caps every rival strictly
	// below the canon count, so the plurality always holds; I.M.P.'s dice
	// never get a say. BIG_SHOT has no voting answer at all (I.M.P. cannot
	// produce it either), so it derives like ATT_NORMAL.
	if (attitude >= 0 && attitude < NUM_ATTITUDES)
	{
		for (int q = 0; q < NUM_QUESTIONS; ++q)
		{
			if (answers[q] != NO_ANSWER) continue;
			for (int a = 0; a < ANSWER_COUNT[q]; ++a)
			{
				if (kAttVote[q][a] == attitude)
				{
					answers[q] = uint8_t(a);
					++attCount[attitude];
					break;
				}
			}
		}
	}

	// Second pass: fill the rest with seeded picks that keep every rival
	// attitude strictly below the canonical count.
	const int canon = (attitude >= 0 && attitude < NUM_ATTITUDES)
				? attCount[attitude] : 0;
	for (int q = 0; q < NUM_QUESTIONS; ++q)
	{
		if (answers[q] != NO_ANSWER) continue;

		uint8_t ok[8];
		int nOk = 0;
		for (int a = 0; a < ANSWER_COUNT[q]; ++a)
		{
			const int8_t v = kAttVote[q][a];
			if (v >= 0 && v != attitude && canon > 0 && attCount[v] + 1 >= canon)
			{
				continue; // would threaten the plurality
			}
			ok[nOk++] = uint8_t(a);
		}
		// nOk can never be 0: every question has at least one non-voting
		// answer, and non-voting answers are always acceptable
		const uint8_t pick = ok[rng.Below(uint32_t(nOk))];
		answers[q] = pick;
		const int8_t v = kAttVote[q][pick];
		if (v >= 0) ++attCount[v];
	}
}

int8_t TallyAttitude(const uint8_t answers[NUM_QUESTIONS])
{
	int hits[NUM_ATTITUDES] = {};
	int total = 0;
	for (int q = 0; q < NUM_QUESTIONS; ++q)
	{
		const int8_t v = AttitudeVote(q, answers[q] == NO_ANSWER ? -1 : answers[q]);
		if (v >= 0) { ++hits[v]; ++total; }
	}
	if (total == 0) return ATT_NORMAL;

	int best = 0;
	for (int i = 1; i < NUM_ATTITUDES; ++i)
	{
		if (hits[i] > hits[best]) best = i;
	}
	// a tie is where I.M.P. rolls dice; report it as such
	for (int i = 0; i < NUM_ATTITUDES; ++i)
	{
		if (i != best && hits[i] == hits[best]) return -1;
	}
	return int8_t(best);
}

// --- matching --------------------------------------------------------------

namespace
{
	// how two attitudes get on across a small table; symmetric by construction
	int AttitudeChord(int8_t a, int8_t b)
	{
		if (a > b) std::swap(a, b);

		// anyone with an asshole: bad - except two of them, who at least
		// know what they are getting
		if (b == ATT_ASSHOLE) return a == ATT_ASSHOLE ? -2 : -8;

		if (a == ATT_FRIENDLY || b == ATT_FRIENDLY)
		{
			// friendly smooths everything except a loner's evening
			return (a == ATT_LONER || b == ATT_LONER) ? -4 : 6;
		}

		if (a == b)
		{
			switch (a)
			{
				case ATT_LONER:     return 4; // separate tables, same corner
				case ATT_OPTIMIST:  return 4;
				case ATT_PESSIMIST: return 2; // misery loves company
				case ATT_ARROGANT:  return -8; // one mirror per household
				case ATT_AGGRESSIVE:return -4;
				default:            return 2;
			}
		}

		if ((a == ATT_OPTIMIST  && b == ATT_PESSIMIST)) return -5;
		if ((a == ATT_AGGRESSIVE && b == ATT_COWARD))   return -6;
		if ((a == ATT_ARROGANT  && b == ATT_BIG_SHOT))  return 3; // an audience
		return 0;
	}
}

uint32_t ChemistrySeed(uint32_t idA, uint32_t idB)
{
	if (idA > idB) std::swap(idA, idB);
	return idA * 73856093u ^ idB * 19349663u;
}

Match Compute(const Profile& a, const Profile& b, uint32_t chemistrySeed)
{
	Match m;
	int score = 50;

	// the questionnaire, where both sides answered
	for (int q = 0; q < NUM_QUESTIONS; ++q)
	{
		if (a.answers[q] == NO_ANSWER || b.answers[q] == NO_ANSWER) continue;
		++m.answered;
		if (a.answers[q] == b.answers[q])
		{
			++m.agree;
			score += 4;
			if (m.bestQ < 0) m.bestQ = q;
		}
		else
		{
			score -= 2;
			if (m.worstQ < 0) m.worstQ = q;
		}
	}

	score += AttitudeChord(a.attitude, b.attitude);

	// a shared phobia is a conversation; a shared psychosis is a bond
	if (a.trait != TRAIT_NONE && a.trait == b.trait)
	{
		score += a.trait == TRAIT_PSYCHO ? 8 : 4;
	}

	// dealbreakers dent the score and get named on the page
	if ((a.trait == TRAIT_PSYCHO) != (b.trait == TRAIT_PSYCHO))
	{
		score -= 15;
		m.dealbreaker = DEAL_PSYCHO;
	}
	if ((a.sexist == SX_VERY_SEXIST && b.sex == SEX_FEMALE) ||
	    (b.sexist == SX_VERY_SEXIST && a.sex == SEX_FEMALE))
	{
		score -= 25;
		m.dealbreaker = DEAL_SEXIST;
	}
	if ((a.sexist == SX_GENTLEMAN && b.sex == SEX_FEMALE) ||
	    (b.sexist == SX_GENTLEMAN && a.sex == SEX_FEMALE))
	{
		score += 5;
	}

	// chemistry: the part no questionnaire explains
	Rng rng(chemistrySeed);
	score += int(rng.Below(17)) - 8;

	// the fewer questions in play, the wider the honest error bars
	m.uncertainty = (NUM_QUESTIONS - m.answered) * 12 / NUM_QUESTIONS;

	m.percent = std::clamp(score, 1, 99);
	return m;
}

} // namespace DatingGame
