#ifndef FELINECAT_H
#define FELINECAT_H

#include <cstdint>

// The Arulco Feline Society's foster cat - the Tamagotchi that lives in the
// back room of Brenda's store and on one web page. Engine-free on the
// MahjongGame / ChessDaily / DatingGame precedent: campaign days go in,
// hunger, mood and poses come out, and every rule is unit-testable.
namespace FelineCat
{
	// the whole animal, sized for the laptop save blob
	struct State
	{
		uint8_t  hunger       = 0; // 0 fed .. 100 empty
		uint8_t  supplies     = 0; // tins on Brenda's shelf
		uint16_t lastFedDay   = 0;
		uint16_t lastVisitDay = 0;
		bool     away         = false; // "staying with a member in the country"
	};

	enum Mood
	{
		MOOD_PLAYFUL, // fed, seen often: the page's best self
		MOOD_CONTENT, // the default cat
		MOOD_LONELY,  // you have not visited in days, and it noticed
		MOOD_HUNGRY,  // the shelf is bare
		MOOD_THIN,    // the polite word the updates use
		MOOD_HIDING,  // fighting nearby: under the counter, ears flat
		MOOD_AWAY,    // not on the page at all
		MOOD_COUNT
	};

	enum Pose
	{
		POSE_LOAF, POSE_SIT, POSE_SLEEP, POSE_STRETCH, POSE_BAT,
		POSE_WALK, POSE_GROOM, POSE_CROUCH, POSE_TAIL, POSE_STARE,
		POSE_COUNT
	};

	// hunger rises 12 points per unfed day. While tins last, Brenda feeds
	// from the shelf every second day without being asked. A cat left at
	// 100 for twelve days is quietly rehomed: away, recoverable, worse.
	void RollDay(State& s, uint16_t today);

	// one tin, by hand, today. False when the shelf is empty or the cat
	// is not there to feed.
	bool Feed(State& s, uint16_t today);

	// what the page says the cat is feeling. warNearby wins over hunger:
	// artillery outranks appetite.
	Mood MoodOf(const State& s, uint16_t today, bool warNearby);

	// a pose the current mood permits, stable for a given seed
	Pose PoseFor(Mood m, uint32_t seed);

	// the letter from Brenda that does not quite explain; the cat is back
	void Recover(State& s, uint16_t today);
}

#endif
