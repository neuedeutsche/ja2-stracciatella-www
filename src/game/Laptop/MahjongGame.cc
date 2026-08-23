#include "MahjongGame.h"

#include <algorithm>
#include <cstring>

namespace
{
	bool RunFits(int kind) { return kind % 9 <= 6; } // all 27 kinds are suit tiles

	bool DecomposeMelds(std::uint8_t* c, int i, int setsLeft)
	{
		while (i < MahjongGame::NUM_KINDS && c[i] == 0) ++i;
		if (i == MahjongGame::NUM_KINDS) return setsLeft == 0;
		if (setsLeft == 0) return false;

		if (c[i] >= 3)
		{
			c[i] -= 3;
			bool const ok = DecomposeMelds(c, i, setsLeft - 1);
			c[i] += 3;
			if (ok) return true;
		}
		if (RunFits(i) && c[i + 1] > 0 && c[i + 2] > 0)
		{
			--c[i]; --c[i + 1]; --c[i + 2];
			bool const ok = DecomposeMelds(c, i, setsLeft - 1);
			++c[i]; ++c[i + 1]; ++c[i + 2];
			if (ok) return true;
		}
		return false;
	}

	// DecomposeMelds, but keeping the receipts: records each set it commits
	// to so a winning hand can be laid out group by group
	bool DecomposeCollect(std::uint8_t* c, int i, int setsLeft,
			MahjongGame::TileId (*groups)[3], int* glen, int& n)
	{
		while (i < MahjongGame::NUM_KINDS && c[i] == 0) ++i;
		if (i == MahjongGame::NUM_KINDS) return setsLeft == 0;
		if (setsLeft == 0) return false;

		if (c[i] >= 3)
		{
			c[i] -= 3;
			groups[n][0] = groups[n][1] = groups[n][2] =
					static_cast<MahjongGame::TileId>(i);
			glen[n] = 3;
			++n;
			if (DecomposeCollect(c, i, setsLeft - 1, groups, glen, n))
			{
				c[i] += 3;
				return true;
			}
			--n;
			c[i] += 3;
		}
		if (RunFits(i) && c[i + 1] > 0 && c[i + 2] > 0)
		{
			--c[i]; --c[i + 1]; --c[i + 2];
			groups[n][0] = static_cast<MahjongGame::TileId>(i);
			groups[n][1] = static_cast<MahjongGame::TileId>(i + 1);
			groups[n][2] = static_cast<MahjongGame::TileId>(i + 2);
			glen[n] = 3;
			++n;
			if (DecomposeCollect(c, i, setsLeft - 1, groups, glen, n))
			{
				++c[i]; ++c[i + 1]; ++c[i + 2];
				return true;
			}
			--n;
			++c[i]; ++c[i + 1]; ++c[i + 2];
		}
		return false;
	}

	// Backtracking search maximising 2*melds + partials + hasPair over a
	// concealed hand; melds + partials capped at the sets still needed.
	void ShantenSearch(std::uint8_t* c, int i, int melds, int partials, bool hasPair, int& best)
	{
		while (i < MahjongGame::NUM_KINDS && c[i] == 0) ++i;
		if (i == MahjongGame::NUM_KINDS)
		{
			best = std::min(best, 8 - 2 * melds - partials - (hasPair ? 1 : 0));
			return;
		}
		// upper-bound prune
		if (8 - 2 * melds - partials - 1 - 2 * (4 - melds - partials) >= best) return;

		if (c[i] >= 3)
		{
			c[i] -= 3;
			ShantenSearch(c, i, melds + 1, partials, hasPair, best);
			c[i] += 3;
		}
		if (RunFits(i) && c[i + 1] > 0 && c[i + 2] > 0)
		{
			--c[i]; --c[i + 1]; --c[i + 2];
			ShantenSearch(c, i, melds + 1, partials, hasPair, best);
			++c[i]; ++c[i + 1]; ++c[i + 2];
		}
		if (c[i] >= 2)
		{
			c[i] -= 2;
			if (!hasPair) ShantenSearch(c, i, melds, partials, true, best);
			if (melds + partials < 4) ShantenSearch(c, i, melds, partials + 1, hasPair, best);
			c[i] += 2;
		}
		if (melds + partials < 4 && i % 9 <= 7 && c[i + 1] > 0)
		{
			--c[i]; --c[i + 1];
			ShantenSearch(c, i, melds, partials + 1, hasPair, best);
			++c[i]; ++c[i + 1];
		}
		if (melds + partials < 4 && RunFits(i) && c[i + 2] > 0)
		{
			--c[i]; --c[i + 2];
			ShantenSearch(c, i, melds, partials + 1, hasPair, best);
			++c[i]; ++c[i + 2];
		}
		// leave one tile of this kind unused
		--c[i];
		ShantenSearch(c, i, melds, partials, hasPair, best);
		++c[i];
	}

	int SevenPairsShanten(const std::uint8_t c[MahjongGame::NUM_KINDS])
	{
		int pairs = 0, kinds = 0;
		for (int i = 0; i < MahjongGame::NUM_KINDS; ++i)
		{
			if (c[i] > 0) ++kinds;
			if (c[i] >= 2) ++pairs;
		}
		int shanten = 6 - pairs;
		if (kinds < 7) shanten += 7 - kinds;
		return shanten;
	}
}


int MahjongGame::DecomposeWin(const std::uint8_t counts[NUM_KINDS],
		TileId groups[7][3], int groupLen[7])
{
	int pairs = 0, total = 0;
	for (int i = 0; i < NUM_KINDS; ++i)
	{
		total += counts[i];
		if (counts[i] == 2) ++pairs;
	}
	if (total != 14) return 0;
	if (pairs == 7)
	{
		int n = 0;
		for (int i = 0; i < NUM_KINDS; ++i)
		{
			if (counts[i] != 2) continue;
			groups[n][0] = groups[n][1] = static_cast<TileId>(i);
			groupLen[n] = 2;
			++n;
		}
		return n;
	}
	std::uint8_t c[NUM_KINDS];
	for (int p = 0; p < NUM_KINDS; ++p)
	{
		if (counts[p] < 2) continue;
		std::memcpy(c, counts, sizeof(c));
		c[p] -= 2;
		int n = 0;
		if (DecomposeCollect(c, 0, 4, groups, groupLen, n))
		{
			groups[n][0] = groups[n][1] = static_cast<TileId>(p);
			groupLen[n] = 2;
			return n + 1;
		}
	}
	return 0;
}

bool MahjongGame::IsWinningSets(const std::uint8_t counts[NUM_KINDS], int setsNeeded, bool allowSevenPairs)
{
	int pairs = 0, total = 0;
	for (int i = 0; i < NUM_KINDS; ++i)
	{
		total += counts[i];
		if (counts[i] == 2) ++pairs;
	}
	if (total != 2 + 3 * setsNeeded) return false;
	if (allowSevenPairs && setsNeeded == 4 && pairs == 7) return true;

	std::uint8_t c[NUM_KINDS];
	for (int p = 0; p < NUM_KINDS; ++p)
	{
		if (counts[p] < 2) continue;
		std::memcpy(c, counts, sizeof(c));
		c[p] -= 2;
		if (DecomposeMelds(c, 0, setsNeeded)) return true;
	}
	return false;
}


bool MahjongGame::IsWinning(const std::uint8_t counts[NUM_KINDS])
{
	return IsWinningSets(counts, 4, true);
}


bool MahjongGame::IsWinningWithVoid(const std::uint8_t counts[NUM_KINDS], int voidSuit)
{
	if (voidSuit != NO_SUIT)
	{
		for (int k = voidSuit * 9; k < voidSuit * 9 + 9; ++k)
		{
			if (counts[k] > 0) return false;
		}
	}
	return IsWinning(counts);
}


bool MahjongGame::handCanWin(int player, const std::uint8_t counts14[NUM_KINDS]) const
{
	Player const& p = players_[player];
	if (p.voidSuit != NO_SUIT)
	{
		for (int k = p.voidSuit * 9; k < p.voidSuit * 9 + 9; ++k)
		{
			if (counts14[k] > 0) return false;
		}
	}
	int const setsNeeded = 4 - static_cast<int>(p.melds.size());
	return IsWinningSets(counts14, setsNeeded, p.melds.empty());
}


int MahjongGame::Shanten(const std::uint8_t counts[NUM_KINDS], int exposedMelds)
{
	std::uint8_t c[NUM_KINDS];
	std::memcpy(c, counts, sizeof(c));
	int best = 8;
	ShantenSearch(c, 0, exposedMelds, 0, false, best);
	if (exposedMelds == 0) best = std::min(best, SevenPairsShanten(counts));
	return best;
}


std::vector<MahjongGame::TileId> MahjongGame::WinningTilesFor(int player) const
{
	std::vector<TileId> waits;
	std::uint8_t c[NUM_KINDS];
	std::memcpy(c, players_[player].counts, sizeof(c));
	for (int k = 0; k < NUM_KINDS; ++k)
	{
		if (c[k] >= 4) continue;
		++c[k];
		if (handCanWin(player, c)) waits.push_back(static_cast<TileId>(k));
		--c[k];
	}
	return waits;
}


int MahjongGame::ShantenFor(int player) const
{
	return Shanten(players_[player].counts, static_cast<int>(players_[player].melds.size()));
}


void MahjongGame::NewMatch(std::uint32_t seed)
{
	rng_.seed(seed);
	for (Player& p : players_)
	{
		std::memset(p.counts, 0, sizeof(p.counts));
		p.melds.clear();
		p.discards.clear();
		p.score = START_SCORE;
		p.voidSuit = NO_SUIT;
		p.finished = false;
	}
	handNumber_ = -1;
	phase_ = Phase::NotStarted;
	NewHand();
}


void MahjongGame::NewHand()
{
	++handNumber_;
	if (handNumber_ >= HANDS_PER_MATCH)
	{
		phase_ = Phase::MatchEnd;
		return;
	}

	wall_.resize(TOTAL_TILES);
	for (int i = 0; i < TOTAL_TILES; ++i) wall_[i] = static_cast<TileId>(i / 4);
	std::shuffle(wall_.begin(), wall_.end(), rng_);
	wallPos_ = 0;

	for (Player& p : players_)
	{
		std::memset(p.counts, 0, sizeof(p.counts));
		p.melds.clear();
		p.discards.clear();
		p.voidSuit = NO_SUIT;
		p.finished = false;
		for (int t = 0; t < HAND_TILES; ++t) ++p.counts[wall_[wallPos_++]];
	}

	exchangeOffset_ = 1 + static_cast<int>(rng_() % 3);

	dealer_ = handNumber_ % NUM_PLAYERS;
	current_ = dealer_;
	drawn_ = NO_TILE;
	lastDiscard_ = NO_TILE;
	lastDiscarder_ = -1;
	wins_.clear();
	wallExhausted_ = false;
	aborted_ = false;
	std::memset(handDelta_, 0, sizeof(handDelta_));
	phase_ = Phase::ExchangeSelect;
}


void MahjongGame::AiChooseExchange(const std::uint8_t counts[NUM_KINDS], TileId out[3])
{
	// give away 3 tiles of the smallest suit holding at least 3, preferring
	// terminals over middle tiles
	int suitCount[3] = {};
	for (int k = 0; k < NUM_KINDS; ++k) suitCount[SuitOf(static_cast<TileId>(k))] += counts[k];
	int suit = -1;
	for (int s = 0; s < 3; ++s)
	{
		if (suitCount[s] < 3) continue;
		if (suit == -1 || suitCount[s] < suitCount[suit]) suit = s;
	}
	if (suit == -1) suit = 0; // cannot happen with 13 tiles over 3 suits

	int picked = 0;
	for (int dist = 0; dist < 5 && picked < 3; ++dist) // rank distance from edge
	{
		for (int rank : { dist, 8 - dist })
		{
			int const k = suit * 9 + rank;
			for (int n = 0; n < counts[k] && picked < 3; ++n)
			{
				out[picked++] = static_cast<TileId>(k);
			}
			if (dist == 4) break; // rank 4 == 8-4, avoid double-counting
		}
	}
}


bool MahjongGame::SetHumanExchange(const TileId give[3])
{
	if (phase_ != Phase::ExchangeSelect) return false;

	// validate: 3 tiles of one suit, all present in the human hand
	int const suit = SuitOf(give[0]);
	std::uint8_t need[NUM_KINDS] = {};
	for (int i = 0; i < 3; ++i)
	{
		if (give[i] >= NUM_KINDS || SuitOf(give[i]) != suit) return false;
		++need[give[i]];
	}
	for (int k = 0; k < NUM_KINDS; ++k)
	{
		if (need[k] > players_[0].counts[k]) return false;
	}

	TileId gives[NUM_PLAYERS][3];
	std::memcpy(gives[0], give, sizeof(gives[0]));
	for (int i = 1; i < NUM_PLAYERS; ++i) AiChooseExchange(players_[i].counts, gives[i]);

	for (int i = 0; i < NUM_PLAYERS; ++i)
	{
		for (int t = 0; t < 3; ++t) --players_[i].counts[gives[i][t]];
	}
	for (int i = 0; i < NUM_PLAYERS; ++i)
	{
		int const to = (i + exchangeOffset_) % NUM_PLAYERS;
		for (int t = 0; t < 3; ++t) ++players_[to].counts[gives[i][t]];
	}

	// with exchanged hands known, AI players declare their void suit
	for (int i = 1; i < NUM_PLAYERS; ++i)
	{
		players_[i].voidSuit = AiChooseVoidSuit(players_[i].counts);
	}
	phase_ = Phase::ChooseVoid;
	return true;
}


void MahjongGame::SetHumanVoidSuit(int suit)
{
	if (phase_ != Phase::ChooseVoid || suit < 0 || suit > 2) return;
	players_[0].voidSuit = suit;
	phase_ = Phase::AwaitDraw;
}


int MahjongGame::FanFor(int player, TileId winningTile, std::uint8_t& flags, std::uint8_t& roots) const
{
	Player const& p = players_[player];
	flags = 0;
	roots = 0;

	// full hand: concealed + winning tile + melds at true counts
	std::uint8_t full[NUM_KINDS];
	std::memcpy(full, p.counts, sizeof(full));
	if (winningTile != NO_TILE) ++full[winningTile];
	std::uint8_t concealed14[NUM_KINDS];
	std::memcpy(concealed14, full, sizeof(concealed14));
	for (Meld const& m : p.melds) full[m.tile] = static_cast<std::uint8_t>(full[m.tile] + m.count);

	// pure suit: every held tile shares one suit
	int suitSeen = -1;
	bool pure = true;
	for (int k = 0; k < NUM_KINDS && pure; ++k)
	{
		if (full[k] == 0) continue;
		if (suitSeen == -1) suitSeen = SuitOf(static_cast<TileId>(k));
		else if (SuitOf(static_cast<TileId>(k)) != suitSeen) pure = false;
	}
	if (pure) flags |= FAN_PURE_SUIT;

	// seven pairs: closed hand of 7 pairs
	if (p.melds.empty())
	{
		int pairs = 0, total = 0;
		for (int k = 0; k < NUM_KINDS; ++k)
		{
			total += concealed14[k];
			if (concealed14[k] == 2) ++pairs;
			if (concealed14[k] == 4) pairs += 2; // a four counts as two pairs
		}
		if (total == 14 && pairs == 7) flags |= FAN_SEVEN_PAIRS;
	}

	// all triplets: melds are always pon/kong, so only the concealed part
	// must decompose into triplets plus the pair (no runs)
	if (!(flags & FAN_SEVEN_PAIRS))
	{
		bool triplets = false;
		for (int pr = 0; pr < NUM_KINDS && !triplets; ++pr)
		{
			if (concealed14[pr] < 2) continue;
			bool ok = true;
			for (int k = 0; k < NUM_KINDS; ++k)
			{
				int n = concealed14[k] - (k == pr ? 2 : 0);
				if (n % 3 != 0) { ok = false; break; }
			}
			triplets = ok;
		}
		if (triplets) flags |= FAN_ALL_TRIPLETS;
	}

	// roots: every four-of-a-kind in the full hand
	for (int k = 0; k < NUM_KINDS; ++k)
	{
		if (full[k] >= 4) ++roots;
	}

	int fan = roots;
	if (flags & FAN_PURE_SUIT) fan += 2;
	if (flags & FAN_ALL_TRIPLETS) fan += 1;
	if (flags & FAN_SEVEN_PAIRS) fan += 2;
	return std::min(fan, FAN_CAP);
}


int MahjongGame::wallRemaining() const
{
	return static_cast<int>(TOTAL_TILES - wallPos_);
}


int MahjongGame::finishedCount() const
{
	int n = 0;
	for (Player const& p : players_)
	{
		if (p.finished) ++n;
	}
	return n;
}


bool MahjongGame::IsTenpai(int player) const
{
	return !WinningTilesFor(player).empty();
}


bool MahjongGame::IsFlowerPig(int player) const
{
	Player const& p = players_[player];
	if (p.voidSuit == NO_SUIT) return false;
	for (int k = p.voidSuit * 9; k < p.voidSuit * 9 + 9; ++k)
	{
		if (p.counts[k] > 0) return true;
	}
	return false;
}


MahjongGame::TileId MahjongGame::DrawForCurrent()
{
	if (phase_ != Phase::AwaitDraw) return NO_TILE;
	if (wallRemaining() <= 0)
	{
		wallExhausted_ = true;
		// cha jiao / hua zhu: settle readiness among the players left standing
		bool pig[NUM_PLAYERS] = {};
		bool ready[NUM_PLAYERS] = {};
		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			if (players_[i].finished) continue;
			pig[i] = IsFlowerPig(i);
			ready[i] = !pig[i] && IsTenpai(i);
		}
		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			if (players_[i].finished) continue;
			for (int j = 0; j < NUM_PLAYERS; ++j)
			{
				if (i == j || players_[j].finished) continue;
				std::int32_t owed = 0;
				if (pig[i]) owed += PIG_PENALTY_EACH;              // pigs pay everyone
				else if (!ready[i] && ready[j]) owed += TENPAI_PENALTY; // not ready pays ready
				if (owed > 0)
				{
					players_[i].score -= owed;
					handDelta_[i] -= owed;
					players_[j].score += owed;
					handDelta_[j] += owed;
				}
			}
		}
		endHand();
		return NO_TILE;
	}
	drawn_ = wall_[wallPos_++];
	phase_ = Phase::AwaitDiscard;
	return drawn_;
}


bool MahjongGame::CanTsumo() const
{
	if (phase_ != Phase::AwaitDiscard || drawn_ == NO_TILE) return false;
	std::uint8_t c[NUM_KINDS];
	std::memcpy(c, players_[current_].counts, sizeof(c));
	++c[drawn_];
	return handCanWin(current_, c);
}


void MahjongGame::ResolveTsumo()
{
	std::uint8_t flags = 0, roots = 0;
	int const fan = FanFor(current_, drawn_, flags, roots);
	std::int32_t const payEach = BASE_TSUMO_EACH << fan;

	// every player still in the hand pays the winner
	for (int i = 0; i < NUM_PLAYERS; ++i)
	{
		if (i == current_ || players_[i].finished) continue;
		players_[i].score -= payEach;
		handDelta_[i] -= payEach;
		players_[current_].score += payEach;
		handDelta_[current_] += payEach;
	}

	int const winner = current_;
	TileId const winning = drawn_;
	drawn_ = NO_TILE;
	retireWinner(winner, -1, winning, fan, flags, roots, payEach);
	if (phase_ == Phase::HandEnd) return;
	advanceToNextUnfinished(winner);
	phase_ = Phase::AwaitDraw;
}


void MahjongGame::Discard(TileId t)
{
	if (phase_ != Phase::AwaitDiscard) return;
	Player& p = players_[current_];
	if (t == drawn_ && drawn_ != NO_TILE)
	{
		// discard the freshly drawn tile as-is
	}
	else
	{
		if (t >= NUM_KINDS || p.counts[t] == 0) return; // not in hand: ignore
		--p.counts[t];
		if (drawn_ != NO_TILE) ++p.counts[drawn_];
	}
	p.discards.push_back(t);
	drawn_ = NO_TILE;
	lastDiscard_ = t;
	lastDiscarder_ = current_;
	phase_ = Phase::RonWindow;
}


int MahjongGame::RonClaimant() const
{
	if (phase_ != Phase::RonWindow) return -1;

	auto const canClaim = [this](int i)
	{
		if (i == lastDiscarder_ || players_[i].finished) return false;
		std::uint8_t c[NUM_KINDS];
		std::memcpy(c, players_[i].counts, sizeof(c));
		++c[lastDiscard_];
		return handCanWin(i, c);
	};

	// house rule: the human player gets claim priority
	if (canClaim(0)) return 0;
	for (int offset = 1; offset < NUM_PLAYERS; ++offset)
	{
		int const i = (lastDiscarder_ + offset) % NUM_PLAYERS;
		if (i != 0 && canClaim(i)) return i;
	}
	return -1;
}


void MahjongGame::ResolveRon(int who)
{
	if (phase_ != Phase::RonWindow) return;

	std::uint8_t flags = 0, roots = 0;
	int const fan = FanFor(who, lastDiscard_, flags, roots);
	std::int32_t const pay = BASE_RON << fan;

	players_[who].score += pay;
	handDelta_[who] += pay;
	players_[lastDiscarder_].score -= pay;
	handDelta_[lastDiscarder_] -= pay;

	retireWinner(who, lastDiscarder_, lastDiscard_, fan, flags, roots, pay);
	if (phase_ == Phase::HandEnd) return;
	advanceToNextUnfinished(lastDiscarder_);
	phase_ = Phase::AwaitDraw;
}


void MahjongGame::PassRon()
{
	if (phase_ != Phase::RonWindow) return;
	advanceToNextUnfinished(lastDiscarder_);
	phase_ = Phase::AwaitDraw;
}


int MahjongGame::MeldClaimant(bool& kongPossible) const
{
	kongPossible = false;
	if (phase_ != Phase::RonWindow) return -1;
	for (int i = 0; i < NUM_PLAYERS; ++i)
	{
		if (i == lastDiscarder_ || players_[i].finished) continue;
		// claiming a tile of your own void suit is pointless and forbidden
		if (players_[i].voidSuit == SuitOf(lastDiscard_)) continue;
		if (players_[i].counts[lastDiscard_] >= 2)
		{
			kongPossible = players_[i].counts[lastDiscard_] >= 3;
			return i;
		}
	}
	return -1;
}


void MahjongGame::ClaimPong(int who)
{
	if (phase_ != Phase::RonWindow || players_[who].counts[lastDiscard_] < 2) return;
	Player& p = players_[who];
	p.counts[lastDiscard_] -= 2;
	p.melds.push_back(Meld{ lastDiscard_, 3, false,
			static_cast<std::int8_t>(lastDiscarder_) });
	// the claimed tile leaves the discarder's pond
	players_[lastDiscarder_].discards.pop_back();
	current_ = who;
	drawn_ = NO_TILE;
	phase_ = Phase::AwaitDiscard;
}


void MahjongGame::ClaimKong(int who)
{
	if (phase_ != Phase::RonWindow || players_[who].counts[lastDiscard_] < 3) return;
	Player& p = players_[who];
	p.counts[lastDiscard_] -= 3;
	p.melds.push_back(Meld{ lastDiscard_, 4, false,
			static_cast<std::int8_t>(lastDiscarder_) });
	players_[lastDiscarder_].discards.pop_back();
	// gua feng: the discarder pays for the exposed kong on the spot
	p.score += KONG_EXPOSED_BONUS;
	handDelta_[who] += KONG_EXPOSED_BONUS;
	players_[lastDiscarder_].score -= KONG_EXPOSED_BONUS;
	handDelta_[lastDiscarder_] -= KONG_EXPOSED_BONUS;
	current_ = who;
	// replacement draw from the wall
	if (wallRemaining() > 0)
	{
		drawn_ = wall_[wallPos_++];
		phase_ = Phase::AwaitDiscard;
	}
	else
	{
		drawn_ = NO_TILE;
		phase_ = Phase::AwaitDiscard;
	}
}


std::vector<MahjongGame::TileId> MahjongGame::SelfKongOptions() const
{
	std::vector<TileId> options;
	if (phase_ != Phase::AwaitDiscard) return options;
	Player const& p = players_[current_];
	for (int k = 0; k < NUM_KINDS; ++k)
	{
		int const held = p.counts[k] + (drawn_ == k ? 1 : 0);
		if (held >= 4)
		{
			options.push_back(static_cast<TileId>(k)); // concealed kong
			continue;
		}
		if (held >= 1)
		{
			for (Meld const& m : p.melds)
			{
				if (m.tile == k && m.count == 3)
				{
					options.push_back(static_cast<TileId>(k)); // added kong
					break;
				}
			}
		}
	}
	return options;
}


bool MahjongGame::DeclareSelfKong(TileId t)
{
	if (phase_ != Phase::AwaitDiscard || t >= NUM_KINDS) return false;
	Player& p = players_[current_];
	int held = p.counts[t] + (drawn_ == t ? 1 : 0);

	bool added = false;
	for (Meld& m : p.melds)
	{
		if (m.tile == t && m.count == 3)
		{
			// added kong: promote the pong with the 4th copy
			if (held < 1) return false;
			m.count = 4;
			if (drawn_ == t) drawn_ = NO_TILE;
			else --p.counts[t];
			added = true;
			break;
		}
	}
	// xia yu / gua feng: everyone still standing pays for the declaration
	std::int32_t const bonus = added ? KONG_ADDED_EACH : KONG_CONCEALED_EACH;
	if (!added)
	{
		if (held < 4) return false;
		// concealed kong: all four copies leave the hand
		int fromHand = 4 - (drawn_ == t ? 1 : 0);
		if (drawn_ == t) drawn_ = NO_TILE;
		p.counts[t] -= static_cast<std::uint8_t>(fromHand);
		p.melds.push_back(Meld{ t, 4, true });
	}
	for (int i = 0; i < NUM_PLAYERS; ++i)
	{
		if (i == current_ || players_[i].finished) continue;
		players_[i].score -= bonus;
		handDelta_[i] -= bonus;
		p.score += bonus;
		handDelta_[current_] += bonus;
	}

	// leftover drawn tile (if any) folds into the hand before the replacement
	if (drawn_ != NO_TILE)
	{
		++p.counts[drawn_];
		drawn_ = NO_TILE;
	}
	if (wallRemaining() > 0)
	{
		drawn_ = wall_[wallPos_++];
		phase_ = Phase::AwaitDiscard;
	}
	else
	{
		// no replacement left: play on without one
		phase_ = Phase::AwaitDiscard;
	}
	return true;
}


bool MahjongGame::IsAddedKong(TileId t) const
{
	Player const& p = players_[current_];
	for (Meld const& m : p.melds)
	{
		if (m.tile == t && m.count == 3) return true;
	}
	return false;
}


int MahjongGame::RobKongClaimant(TileId t) const
{
	auto const canRob = [&](int i)
	{
		if (i == current_ || players_[i].finished) return false;
		std::uint8_t c[NUM_KINDS];
		std::memcpy(c, players_[i].counts, sizeof(c));
		++c[t];
		return handCanWin(i, c);
	};
	if (canRob(0)) return 0;
	for (int offset = 1; offset < NUM_PLAYERS; ++offset)
	{
		int const i = (current_ + offset) % NUM_PLAYERS;
		if (i != 0 && canRob(i)) return i;
	}
	return -1;
}


void MahjongGame::ResolveRobKong(int who, TileId t)
{
	// the 4th tile is stolen mid-air: the declarer pays as a discarder would
	Player& d = players_[current_];
	if (drawn_ == t) drawn_ = NO_TILE;
	else if (d.counts[t] > 0) --d.counts[t];
	if (drawn_ != NO_TILE)
	{
		++d.counts[drawn_];
		drawn_ = NO_TILE;
	}

	std::uint8_t flags = 0, roots = 0;
	int fan = FanFor(who, t, flags, roots);
	fan = std::min(fan + 1, FAN_CAP); // robbing the kong is worth an extra fan
	std::int32_t const pay = BASE_RON << fan;

	players_[who].score += pay;
	handDelta_[who] += pay;
	d.score -= pay;
	handDelta_[current_] -= pay;

	int const declarer = current_;
	retireWinner(who, declarer, t, fan, flags, roots, pay);
	if (phase_ == Phase::HandEnd) return;
	advanceToNextUnfinished(declarer);
	phase_ = Phase::AwaitDraw;
}


void MahjongGame::AbortHand()
{
	aborted_ = true;
	endHand();
}


void MahjongGame::SetAiGrudge(int player, int target, int level)
{
	if (player >= 0 && player < NUM_PLAYERS)
	{
		aiGrudgeTarget_[player] = target;
		aiGrudgeLevel_[player] = level;
	}
}


bool MahjongGame::AiWantsSelfKong(int player, TileId t) const
{
	Player const& p = players_[player];
	int const meldsNow = static_cast<int>(p.melds.size());
	int const before = Shanten(p.counts, meldsNow);
	std::uint8_t c[NUM_KINDS];
	std::memcpy(c, p.counts, sizeof(c));
	int const inHand = c[t];
	c[t] = 0;
	// concealed kong banks a meld; added kong just sheds the copy
	int const after = Shanten(c, meldsNow + (inHand >= 4 ? 1 : 0));
	return after <= before;
}


bool MahjongGame::AiWantsPong(int player) const
{
	Player const& p = players_[player];
	if (p.counts[lastDiscard_] < 2) return false;

	int const meldsNow = static_cast<int>(p.melds.size());
	int const before = Shanten(p.counts, meldsNow);

	std::uint8_t c[NUM_KINDS];
	std::memcpy(c, p.counts, sizeof(c));
	c[lastDiscard_] -= 2;

	// after ponging the AI must discard: take the best resulting shanten
	int after = 99;
	for (int k = 0; k < NUM_KINDS; ++k)
	{
		if (c[k] == 0) continue;
		--c[k];
		after = std::min(after, Shanten(c, meldsNow + 1));
		++c[k];
	}
	return after < before;
}


bool MahjongGame::AiWantsKong(int player) const
{
	Player const& p = players_[player];
	if (p.counts[lastDiscard_] < 3) return false;

	int const meldsNow = static_cast<int>(p.melds.size());
	int const before = Shanten(p.counts, meldsNow);

	std::uint8_t c[NUM_KINDS];
	std::memcpy(c, p.counts, sizeof(c));
	c[lastDiscard_] -= 3;
	// the kong comes with a free replacement draw, so not-worse is enough
	return Shanten(c, meldsNow + 1) <= before;
}


void MahjongGame::recordWinningHand(WinEvent& e, int player, TileId winningTile) const
{
	Player const& p = players_[player];
	std::memcpy(e.winningCounts, p.counts, sizeof(e.winningCounts));
	if (winningTile != NO_TILE) ++e.winningCounts[winningTile];
	for (Meld const& m : p.melds)
	{
		e.winningCounts[m.tile] = static_cast<std::uint8_t>(
				std::min(4, e.winningCounts[m.tile] + 3)); // kongs shown as 3
	}
}


void MahjongGame::retireWinner(int winner, int discarder, TileId winningTile,
				int fan, std::uint8_t fanFlags, std::uint8_t roots, std::int32_t payment)
{
	WinEvent e;
	e.winner = winner;
	e.discarder = discarder;
	e.fan = fan;
	e.fanFlags = fanFlags;
	e.roots = roots;
	e.payment = payment;
	e.winningTile = winningTile;
	recordWinningHand(e, winner, winningTile);
	wins_.push_back(e);
	players_[winner].finished = true;

	// bloody battle ends when only one player is left standing
	if (finishedCount() >= NUM_PLAYERS - 1) endHand();
}


void MahjongGame::advanceToNextUnfinished(int after)
{
	for (int offset = 1; offset <= NUM_PLAYERS; ++offset)
	{
		int const i = (after + offset) % NUM_PLAYERS;
		if (!players_[i].finished)
		{
			current_ = i;
			return;
		}
	}
}


void MahjongGame::endHand()
{
	phase_ = Phase::HandEnd;
}


int MahjongGame::AiChooseVoidSuit(const std::uint8_t counts[NUM_KINDS])
{
	int bestSuit = 0, bestCount = 99;
	for (int s = 0; s < 3; ++s)
	{
		int n = 0;
		for (int k = s * 9; k < s * 9 + 9; ++k) n += counts[k];
		if (n < bestCount)
		{
			bestCount = n;
			bestSuit = s;
		}
	}
	return bestSuit;
}


MahjongGame::TileId MahjongGame::AiChooseDiscard(int player) const
{
	Player const& p = players_[player];
	int const meldsNow = static_cast<int>(p.melds.size());
	std::uint8_t c14[NUM_KINDS];
	std::memcpy(c14, p.counts, sizeof(c14));
	if (player == current_ && drawn_ != NO_TILE) ++c14[drawn_];

	// copies of each kind not visible to this player (own hand + all ponds + melds)
	std::uint8_t unseen[NUM_KINDS];
	for (int k = 0; k < NUM_KINDS; ++k) unseen[k] = 4 - c14[k];
	for (Player const& q : players_)
	{
		for (TileId d : q.discards)
		{
			if (unseen[d] > 0) --unseen[d];
		}
		for (Meld const& m : q.melds)
		{
			if (&q == &p) continue;
			unseen[m.tile] = static_cast<std::uint8_t>(std::max(0, unseen[m.tile] - m.count));
		}
	}

	auto neighbourScore = [&](int k) -> int
	{
		int score = 2 * unseen[k];
		int const rank = k % 9;
		if (rank >= 1) score += unseen[k - 1];
		if (rank >= 2) score += unseen[k - 2];
		if (rank <= 7) score += unseen[k + 1];
		if (rank <= 6) score += unseen[k + 2];
		return score;
	};

	// void-suit tiles can never win: dump them first, most isolated first
	if (p.voidSuit != NO_SUIT)
	{
		TileId bestVoid = NO_TILE;
		int bestNeighbours = 999;
		for (int k = p.voidSuit * 9; k < p.voidSuit * 9 + 9; ++k)
		{
			if (c14[k] == 0) continue;
			int const n = neighbourScore(k);
			if (n < bestNeighbours)
			{
				bestNeighbours = n;
				bestVoid = static_cast<TileId>(k);
			}
		}
		if (bestVoid != NO_TILE) return bestVoid;
	}

	// the grudge: a held target's waits are radioactive if avoidable
	bool avoid[NUM_KINDS] = {};
	if (aiGrudgeTarget_[player] >= 0 && aiGrudgeLevel_[player] > 0 &&
		!players_[aiGrudgeTarget_[player]].finished)
	{
		for (TileId w : WinningTilesFor(aiGrudgeTarget_[player])) avoid[w] = true;
	}

	TileId bestTile = NO_TILE;
	int bestShanten = 99, bestNeighbours = 999;
	bool bestAvoided = true;
	for (int k = 0; k < NUM_KINDS; ++k)
	{
		if (c14[k] == 0) continue;
		--c14[k];
		int const sh = Shanten(c14, meldsNow);
		++c14[k];
		int const n = neighbourScore(k);
		bool const dangerous = avoid[k];
		if (sh < bestShanten ||
			(sh == bestShanten && !dangerous && bestAvoided) ||
			(sh == bestShanten && dangerous == bestAvoided && n < bestNeighbours))
		{
			bestShanten = sh;
			bestNeighbours = n;
			bestTile = static_cast<TileId>(k);
			bestAvoided = dangerous;
		}
	}
	return bestTile;
}


void MahjongGame::SetAiErrorRate(int player, int percent)
{
	if (player >= 0 && player < NUM_PLAYERS)
	{
		aiErrorRate_[player] = static_cast<std::uint8_t>(std::min(percent, 100));
	}
}


MahjongGame::TileId MahjongGame::AiChooseDiscardSkilled(int player)
{
	TileId const best = AiChooseDiscard(player);
	if (aiErrorRate_[player] == 0 || rng_() % 100 >= aiErrorRate_[player]) return best;

	// blunder: discard something that leaves the hand one step worse
	Player const& p = players_[player];
	int const meldsNow = static_cast<int>(p.melds.size());
	std::uint8_t c14[NUM_KINDS];
	std::memcpy(c14, p.counts, sizeof(c14));
	if (player == current_ && drawn_ != NO_TILE) ++c14[drawn_];

	--c14[best];
	int const bestShanten = Shanten(c14, meldsNow);
	++c14[best];

	std::vector<TileId> blunders;
	for (int k = 0; k < NUM_KINDS; ++k)
	{
		if (c14[k] == 0 || k == best) continue;
		--c14[k];
		if (Shanten(c14, meldsNow) == bestShanten + 1) blunders.push_back(static_cast<TileId>(k));
		++c14[k];
	}
	if (blunders.empty()) return best;
	return blunders[rng_() % blunders.size()];
}


std::vector<MahjongGame::TileId> MahjongGame::SortedHand(const Player& p)
{
	std::vector<TileId> hand;
	hand.reserve(HAND_TILES);
	for (int k = 0; k < NUM_KINDS; ++k)
	{
		for (int n = 0; n < p.counts[k]; ++n) hand.push_back(static_cast<TileId>(k));
	}
	return hand;
}
