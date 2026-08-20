#ifndef MAHJONGGAME_H
#define MAHJONGGAME_H

// Engine-free Sichuan-rules mahjong core for the laptop minigame.
//
// Sichuan mahjong in brief: 108 tiles (suits only, no honours), pong and
// kong but no chi, every player passes 3 tiles to a neighbour after the
// deal (huan san zhang), declares one void suit (que yi men) and cannot win
// while holding tiles of it, and the hand is played "bloody to the end"
// (xue zhan dao di): winners retire and play continues until three players
// have won or the wall is exhausted.
//
// No JA2 headers on purpose: everything here is deterministic given the
// seed passed to NewMatch(), so the whole game is unit-testable.

#include <cstdint>
#include <random>
#include <vector>

class MahjongGame
{
public:
	// 0..8 man (wan), 9..17 pin (tong), 18..26 sou (tiao)
	using TileId = std::uint8_t;

	static constexpr int NUM_KINDS       = 27;
	static constexpr int NUM_PLAYERS     = 4;
	static constexpr int HAND_TILES      = 13;
	static constexpr int TOTAL_TILES     = 108;
	static constexpr int HANDS_PER_MATCH = 4;
	static constexpr TileId NO_TILE      = 0xFF;
	static constexpr int NO_SUIT         = -1;

	static constexpr std::int32_t START_SCORE    = 25000;
	// fan scoring: base payments double per fan, capped
	static constexpr std::int32_t BASE_TSUMO_EACH = 1000;
	static constexpr std::int32_t BASE_RON        = 3000;
	static constexpr int FAN_CAP                  = 4;
	// kong bonuses (gua feng xia yu): instant payments on declaration
	static constexpr std::int32_t KONG_EXPOSED_BONUS   = 500; // discarder pays claimer
	static constexpr std::int32_t KONG_CONCEALED_EACH  = 500; // everyone pays
	static constexpr std::int32_t KONG_ADDED_EACH      = 250; // everyone pays
	// wall-exhaustion settlements (cha jiao / hua zhu)
	static constexpr std::int32_t TENPAI_PENALTY   = 1000; // not-ready pays each ready
	static constexpr std::int32_t PIG_PENALTY_EACH = 2000; // void-suit holdout pays all
	// fan flags recorded on a win
	static constexpr std::uint8_t FAN_PURE_SUIT    = 0x01;
	static constexpr std::uint8_t FAN_ALL_TRIPLETS = 0x02;
	static constexpr std::uint8_t FAN_SEVEN_PAIRS  = 0x04;

	enum class Phase : std::uint8_t
	{
		NotStarted,     // before NewMatch / between matches
		ExchangeSelect, // hand dealt, waiting for the human's 3-tile exchange pick
		ChooseVoid,     // exchange done, waiting for the human's void-suit choice
		AwaitDraw,      // current player must draw
		AwaitDiscard,   // current player must discard (after draw, pong or kong)
		RonWindow,      // a discard is on the table, others may claim it
		HandEnd,        // hand resolved (see wins()/endedByWallExhaustion())
		MatchEnd,       // all hands played
	};

	// an exposed (or concealed-kong) meld on the table
	struct Meld
	{
		TileId tile;
		std::uint8_t count;  // 3 = pong, 4 = kong
		bool concealed;      // concealed kong
	};

	struct Player
	{
		std::uint8_t counts[NUM_KINDS]; // concealed hand as per-kind counts
		std::vector<Meld> melds;        // face-up sets
		std::vector<TileId> discards;   // pond, in discard order
		std::int32_t score;
		int voidSuit;                   // 0..2 once declared, NO_SUIT before
		bool finished;                  // already won this hand (retired)
	};

	// one win during the bloody battle; discarder == -1 means self-draw
	struct WinEvent
	{
		int winner    = -1;
		int discarder = -1;
		int fan       = 0;
		std::uint8_t fanFlags = 0;                  // FAN_* bits
		std::uint8_t roots    = 0;                  // four-of-a-kind count
		std::int32_t payment  = 0;                  // per payer
		std::uint8_t winningCounts[NUM_KINDS] = {}; // 14-tile display hand (kongs shown as 3)
	};

	// --- match / hand flow ------------------------------------------------
	void NewMatch(std::uint32_t seed);
	void NewHand();                  // deals; phase -> ExchangeSelect

	// huan san zhang: every player passes 3 same-suit tiles to the player
	// `exchangeOffset()` seats away (1 = right, 2 = across, 3 = left).
	bool SetHumanExchange(const TileId give[3]); // phase -> ChooseVoid
	int  exchangeOffset() const { return exchangeOffset_; }

	void SetHumanVoidSuit(int suit); // 0..2; phase -> AwaitDraw
	TileId DrawForCurrent();         // AwaitDraw -> AwaitDiscard (or HandEnd on empty wall)
	bool CanTsumo() const;           // valid in AwaitDiscard; enforces void suit
	void ResolveTsumo();             // winner retires; hand continues or ends
	void Discard(TileId t);          // phase -> RonWindow
	int  RonClaimant() const;        // human first, then turn order; -1 if none
	void ResolveRon(int who);        // winner retires; hand continues or ends
	void PassRon();                  // nobody claims: next unfinished player draws

	// --- pong / kong ------------------------------------------------------
	// who could meld the tile on the table (unique: only one player can hold
	// two copies once the discarder used theirs). kongPossible: they hold 3.
	// fan evaluation for a prospective win (concealed+winning tile + melds)
	int FanFor(int player, TileId winningTile, std::uint8_t& flags, std::uint8_t& roots) const;

	int  MeldClaimant(bool& kongPossible) const; // valid in RonWindow
	void ClaimPong(int who);         // exposed triplet; claimer must discard
	void ClaimKong(int who);         // exposed kong; replacement draw, then discard
	std::vector<TileId> SelfKongOptions() const; // current player, AwaitDiscard
	bool DeclareSelfKong(TileId t);  // concealed or added kong; replacement draw

	// robbing the kong: an added kong's 4th tile can be claimed as a win
	bool IsAddedKong(TileId t) const;            // would DeclareSelfKong(t) promote a pong?
	int  RobKongClaimant(TileId t) const;        // human first; -1 if nobody can rob
	void ResolveRobKong(int who, TileId t);      // declarer pays as discarder

	// the scandal: void the hand, no payments, no winners
	void AbortHand();
	bool aborted() const { return aborted_; }

	// is this (unfinished) player one tile from winning?
	bool IsTenpai(int player) const;
	// still holding void-suit tiles (the flower pig)
	bool IsFlowerPig(int player) const;

	bool AiWantsPong(int player) const; // pong only if it advances the hand
	bool AiWantsKong(int player) const; // kong if it does not set the hand back

	// --- AI ---------------------------------------------------------------
	static int AiChooseVoidSuit(const std::uint8_t counts[NUM_KINDS]);
	static void AiChooseExchange(const std::uint8_t counts[NUM_KINDS], TileId out[3]);
	TileId AiChooseDiscard(int player) const; // void tiles first, then shanten

	// human-like fallibility: with errorRate% probability the AI discards a
	// tile that genuinely sets its hand back one step
	void SetAiErrorRate(int player, int percent);
	TileId AiChooseDiscardSkilled(int player);

	// the grudge: this AI avoids feeding a specific player's waits when a
	// same-quality alternative exists (the Queen keeps count)
	void SetAiGrudge(int player, int target, int level);
	bool AiWantsSelfKong(int player, TileId t) const;

	// --- pure helpers (unit-test targets) ---------------------------------
	// setsNeeded sets + a pair; seven pairs allowed only for a full closed hand
	static bool IsWinningSets(const std::uint8_t counts[NUM_KINDS], int setsNeeded, bool allowSevenPairs);
	static bool IsWinning(const std::uint8_t counts[NUM_KINDS]); // 14 tiles, 4 sets
	static int  Shanten(const std::uint8_t counts[NUM_KINDS], int exposedMelds = 0);
	static int  SuitOf(TileId t) { return t / 9; }
	static bool IsWinningWithVoid(const std::uint8_t counts[NUM_KINDS], int voidSuit);

	// tiles that would complete the player's concealed hand (their waits)
	std::vector<TileId> WinningTilesFor(int player) const;
	int ShantenFor(int player) const;

	// --- accessors --------------------------------------------------------
	Phase phase() const               { return phase_; }
	int currentPlayer() const         { return current_; }
	int dealer() const                { return dealer_; }
	int handNumber() const            { return handNumber_; } // 0-based
	const Player& player(int i) const { return players_[i]; }
	TileId drawnTile() const          { return drawn_; }      // NO_TILE if none
	TileId lastDiscard() const        { return lastDiscard_; }
	int lastDiscarder() const         { return lastDiscarder_; }
	int wallRemaining() const;
	int finishedCount() const;
	const std::vector<WinEvent>& wins() const { return wins_; }
	bool endedByWallExhaustion() const        { return wallExhausted_; }
	std::int32_t handDelta(int i) const       { return handDelta_[i]; }

	// concealed tiles of a hand, ascending (for UI slots)
	static std::vector<TileId> SortedHand(const Player& p);

private:
	bool handCanWin(int player, const std::uint8_t counts14[NUM_KINDS]) const;
	void recordWinningHand(WinEvent& e, int player, TileId winningTile) const;
	void retireWinner(int winner, int discarder, TileId winningTile,
				int fan, std::uint8_t fanFlags, std::uint8_t roots, std::int32_t payment);
	void advanceToNextUnfinished(int after);
	void endHand();

	Phase phase_ = Phase::NotStarted;
	Player players_[NUM_PLAYERS] = {};
	std::vector<TileId> wall_;
	std::size_t wallPos_ = 0;
	int current_ = 0;
	int dealer_ = 0;
	int handNumber_ = 0;
	TileId drawn_ = NO_TILE;
	TileId lastDiscard_ = NO_TILE;
	int lastDiscarder_ = -1;
	std::vector<WinEvent> wins_;
	bool wallExhausted_ = false;
	bool aborted_ = false;
	int aiGrudgeTarget_[NUM_PLAYERS] = { -1, -1, -1, -1 };
	int aiGrudgeLevel_[NUM_PLAYERS] = {};
	int exchangeOffset_ = 1;
	std::int32_t handDelta_[NUM_PLAYERS] = {};
	std::uint8_t aiErrorRate_[NUM_PLAYERS] = {};
	std::mt19937 rng_;
};

#endif
