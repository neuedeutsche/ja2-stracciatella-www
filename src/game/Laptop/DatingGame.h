#ifndef DATINGGAME_H
#define DATINGGAME_H

// Mercs & Kisses: the engine-free matchmaking core.
//
// No JA2 headers on purpose - everything here is plain C++ so the unit tests
// build without the engine, exactly like MahjongGame and ChessGame. The bridge
// to the game's enums is a set of static_asserts in Cupid.cc.
//
// The heart of it is the I.M.P. personality quiz. The game has always thrown
// the 16 raw answers away after collapsing them to one attitude and one trait;
// we keep them (see CupidPersist) and match on them, which is exactly what
// SparkMatch was doing with its own personality test in the real 1999.

#include <cstdint>

namespace DatingGame
{

// --- mirrored game enums ---------------------------------------------------
// Numeric twins of the Soldier_Profile_Type.h values; Cupid.cc static_asserts
// they stay in step.

enum Attitude : int8_t
{
	ATT_NORMAL = 0,
	ATT_FRIENDLY,
	ATT_LONER,
	ATT_OPTIMIST,
	ATT_PESSIMIST,
	ATT_AGGRESSIVE,
	ATT_ARROGANT,
	ATT_BIG_SHOT,
	ATT_ASSHOLE,
	ATT_COWARD,
	NUM_ATTITUDES
};

enum Trait : int8_t
{
	TRAIT_NONE = 0,
	TRAIT_HEAT_INTOLERANT,
	TRAIT_NERVOUS,
	TRAIT_CLAUSTROPHOBIC,
	TRAIT_NONSWIMMER,
	TRAIT_FEAR_OF_INSECTS,
	TRAIT_FORGETFUL,
	TRAIT_PSYCHO,
	NUM_TRAITS
};

enum Sexist : int8_t
{
	SX_NOT_SEXIST = 0,
	SX_SOMEWHAT_SEXIST,
	SX_VERY_SEXIST,
	SX_GENTLEMAN
};

enum Sex : int8_t { SEX_MALE = 0, SEX_FEMALE = 1 };

// --- the quiz --------------------------------------------------------------

constexpr int NUM_QUESTIONS = 16;
constexpr uint8_t NO_ANSWER = 0x0F; // nibble sentinel, matches the save format

// answers per question, from IMP_Personality_Quiz.cc's button counts
extern const uint8_t ANSWER_COUNT[NUM_QUESTIONS];

// What each answer votes for when I.M.P. compiles a character, transcribed
// from CompileQuestionsInStatsAndWhatNot(). -1 / TRAIT_NONE = no vote (the
// answer votes for a skill instead, or for nothing).
int8_t AttitudeVote(int question, int answer); // Attitude or -1
int8_t TraitVote(int question, int answer);    // Trait or TRAIT_NONE

// --- profiles --------------------------------------------------------------

struct Profile
{
	int8_t  attitude   = ATT_NORMAL;
	int8_t  trait      = TRAIT_NONE;
	int8_t  sexist     = SX_NOT_SEXIST;
	int8_t  sex        = SEX_MALE;
	uint8_t answers[NUM_QUESTIONS] = { NO_ANSWER, NO_ANSWER, NO_ANSWER,
		NO_ANSWER, NO_ANSWER, NO_ANSWER, NO_ANSWER, NO_ANSWER, NO_ANSWER,
		NO_ANSWER, NO_ANSWER, NO_ANSWER, NO_ANSWER, NO_ANSWER, NO_ANSWER,
		NO_ANSWER };

	bool HasAnswers() const;
};

// Deterministically fill in the 16 answers a merc "gave" from their canonical
// attitude and trait, seeded by profile id so the sheet never changes between
// sessions. Guarantees the answers tally back to the given attitude under
// I.M.P.'s own vote-counting (the plurality is forced, every rival attitude is
// kept strictly below it), so the data cannot drift from the game.
void DeriveAnswers(uint32_t seed, int8_t attitude, int8_t trait,
			uint8_t answers[NUM_QUESTIONS]);

// Re-run I.M.P.'s tally over an answer sheet: returns the attitude a plurality
// of votes picks, ATT_NORMAL when nothing got two votes (mirroring
// CreatePlayerAttitude's behaviour for an empty list), or -1 on a tie that
// I.M.P. would have diced for. Used by the validation test.
int8_t TallyAttitude(const uint8_t answers[NUM_QUESTIONS]);

// --- matching --------------------------------------------------------------

enum Dealbreaker : int8_t
{
	DEAL_NONE = 0,
	DEAL_PSYCHO,      // one of them is PSYCHO and the other is not
	DEAL_SEXIST,      // a VERY_SEXIST member matched against a woman
};

struct Match
{
	int    percent     = 0;   // 1..99; nobody is 100, the site says so
	int    uncertainty = 0;   // +/- band; 0 when both sheets are complete
	int    agree       = 0;   // questions both answered identically
	int    answered    = 0;   // questions both answered at all
	int    bestQ       = -1;  // an agreed question, for the check-mark line
	int    worstQ      = -1;  // a disagreed question, for the cross line
	int8_t dealbreaker = DEAL_NONE;
};

// Symmetric: Compute(a, b) == Compute(b, a) with the same seed. The seed
// stands in for chemistry and should hash both identities the same way in
// either order.
Match Compute(const Profile& a, const Profile& b, uint32_t chemistrySeed);

// Order-independent seed helper for two profile ids.
uint32_t ChemistrySeed(uint32_t idA, uint32_t idB);

} // namespace DatingGame

#endif
