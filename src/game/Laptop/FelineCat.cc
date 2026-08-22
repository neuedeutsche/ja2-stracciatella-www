#include "FelineCat.h"

namespace FelineCat
{
	void RollDay(State& s, uint16_t today)
	{
		if (s.away) return;
		if (today < s.lastFedDay) return; // clock oddities feed nobody

		// Brenda works the shelf first: one tin every second day
		while (s.supplies > 0 && today >= uint16_t(s.lastFedDay + 2))
		{
			s.lastFedDay = uint16_t(s.lastFedDay + 2);
			--s.supplies;
		}

		const uint32_t days = uint32_t(today - s.lastFedDay);
		const uint32_t h = days * 12;
		s.hunger = h > 100 ? 100 : uint8_t(h);

		// twelve days past empty and the updates change their wording
		if (s.hunger >= 100 && days >= 12)
		{
			s.away = true;
		}
	}

	bool Feed(State& s, uint16_t today)
	{
		if (s.away || s.supplies == 0) return false;
		--s.supplies;
		s.lastFedDay = today;
		s.hunger = 0;
		return true;
	}

	Mood MoodOf(const State& s, uint16_t today, bool warNearby)
	{
		if (s.away) return MOOD_AWAY;
		if (warNearby) return MOOD_HIDING;
		if (s.hunger >= 80) return MOOD_THIN;
		if (s.hunger >= 45) return MOOD_HUNGRY;
		if (today >= uint16_t(s.lastVisitDay + 5)) return MOOD_LONELY;
		if (s.hunger <= 15) return MOOD_PLAYFUL;
		return MOOD_CONTENT;
	}

	Pose PoseFor(Mood m, uint32_t seed)
	{
		// each mood owns a vocabulary; the seed leafs through it
		static const Pose banks[MOOD_COUNT][4] =
		{
			/* PLAYFUL */ { POSE_BAT,    POSE_WALK,   POSE_TAIL,   POSE_STRETCH },
			/* CONTENT */ { POSE_LOAF,   POSE_SIT,    POSE_GROOM,  POSE_SLEEP   },
			/* LONELY  */ { POSE_SIT,    POSE_STARE,  POSE_SLEEP,  POSE_LOAF    },
			/* HUNGRY  */ { POSE_STARE,  POSE_WALK,   POSE_SIT,    POSE_TAIL    },
			/* THIN    */ { POSE_SLEEP,  POSE_LOAF,   POSE_STARE,  POSE_SIT     },
			/* HIDING  */ { POSE_CROUCH, POSE_CROUCH, POSE_STARE,  POSE_CROUCH  },
			/* AWAY    */ { POSE_SLEEP,  POSE_SLEEP,  POSE_SLEEP,  POSE_SLEEP   },
		};
		return banks[m][seed % 4];
	}

	void Recover(State& s, uint16_t today)
	{
		s.away = false;
		s.lastFedDay = today;
		s.hunger = 0;
	}
}
