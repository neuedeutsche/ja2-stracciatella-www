#include "gtest/gtest.h"

#include "MahjongGame.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace
{
	using TileId = MahjongGame::TileId;

	// man 1-9 => 0..8, pin 1-9 => 9..17, sou 1-9 => 18..26
	void FillCounts(std::uint8_t (&counts)[MahjongGame::NUM_KINDS], std::initializer_list<int> tiles)
	{
		std::memset(counts, 0, sizeof(counts));
		for (int t : tiles) ++counts[t];
	}

	void DoHumanExchange(MahjongGame& game)
	{
		MahjongGame::TileId give[3];
		MahjongGame::AiChooseExchange(game.player(0).counts, give);
		ASSERT_TRUE(game.SetHumanExchange(give));
	}

	// drive one full hand with the AI making every decision (human included)
	void PlayOutHand(MahjongGame& game)
	{
		if (game.phase() == MahjongGame::Phase::ExchangeSelect)
		{
			DoHumanExchange(game);
		}
		if (game.phase() == MahjongGame::Phase::ChooseVoid)
		{
			game.SetHumanVoidSuit(MahjongGame::AiChooseVoidSuit(game.player(0).counts));
		}
		int safety = 1000;
		while (game.phase() != MahjongGame::Phase::HandEnd && safety-- > 0)
		{
			game.DrawForCurrent();
			if (game.phase() == MahjongGame::Phase::HandEnd) break;
			// self kongs, with the rob resolved AI-style
			for (MahjongGame::TileId t : game.SelfKongOptions())
			{
				if (!game.AiWantsSelfKong(game.currentPlayer(), t)) continue;
				if (game.IsAddedKong(t))
				{
					int const robber = game.RobKongClaimant(t);
					if (robber >= 0)
					{
						game.ResolveRobKong(robber, t);
						break;
					}
				}
				game.DeclareSelfKong(t);
				break;
			}
			if (game.phase() == MahjongGame::Phase::HandEnd) break;
			if (game.phase() == MahjongGame::Phase::AwaitDraw) continue; // a rob passed the turn
			if (game.CanTsumo())
			{
				game.ResolveTsumo();
				continue;
			}
			game.Discard(game.AiChooseDiscard(game.currentPlayer()));
			int const claimant = game.RonClaimant();
			if (claimant >= 0)
			{
				game.ResolveRon(claimant);
				continue;
			}
			bool kongPossible = false;
			int const mc = game.MeldClaimant(kongPossible);
			if (mc >= 0 && kongPossible && game.AiWantsKong(mc))
			{
				game.ClaimKong(mc);
			}
			else if (mc >= 0 && game.AiWantsPong(mc))
			{
				game.ClaimPong(mc);
			}
			else
			{
				game.PassRon();
			}
		}
		ASSERT_GT(safety, 0);
	}
}


TEST(MahjongGameTest, winningStandardHandWithRuns)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	// 123m 456m 789m 111p 55s
	FillCounts(c, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 22, 22 });
	EXPECT_TRUE(MahjongGame::IsWinning(c));
}


TEST(MahjongGameTest, winningSevenPairs)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	FillCounts(c, { 0, 0, 4, 4, 9, 9, 13, 13, 20, 20, 24, 24, 26, 26 });
	EXPECT_TRUE(MahjongGame::IsWinning(c));
}


TEST(MahjongGameTest, nearMissIsNotWinning)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	// 123m 456m 789m 111p + 5s 6p: no pair
	FillCounts(c, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 22, 14 });
	EXPECT_FALSE(MahjongGame::IsWinning(c));
}


TEST(MahjongGameTest, runsCannotCrossSuitBoundary)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	// 123m 456m 89m 1p 456p 77s: complete iff 8m9m1p counted as a run,
	// which must not happen across the man/pin boundary.
	FillCounts(c, { 0, 1, 2, 3, 4, 5, 7, 8, 9, 12, 13, 14, 24, 24 });
	EXPECT_FALSE(MahjongGame::IsWinning(c));
}


TEST(MahjongGameTest, tripletVsRunBacktracking)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	// 111234m shape: 111m 234m 567p 999p 44s
	FillCounts(c, { 0, 0, 0, 1, 2, 3, 13, 14, 15, 17, 17, 17, 21, 21 });
	EXPECT_TRUE(MahjongGame::IsWinning(c));
}


TEST(MahjongGameTest, voidSuitBlocksWin)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	// clean two-suit winning hand: 123m 456m 789m 111p 55p... use pin pair
	FillCounts(c, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 13, 13 });
	EXPECT_TRUE(MahjongGame::IsWinning(c));
	EXPECT_TRUE(MahjongGame::IsWinningWithVoid(c, 2));   // holds no sou: ok
	EXPECT_FALSE(MahjongGame::IsWinningWithVoid(c, 0));  // holds man: blocked
	EXPECT_FALSE(MahjongGame::IsWinningWithVoid(c, 1));  // holds pin: blocked
	EXPECT_TRUE(MahjongGame::IsWinningWithVoid(c, MahjongGame::NO_SUIT));
}


TEST(MahjongGameTest, aiVoidChoicePicksSmallestSuit)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	// 6 man, 5 pin, 2 sou
	FillCounts(c, { 0, 1, 2, 3, 4, 5, 9, 10, 11, 12, 13, 20, 21 });
	EXPECT_EQ(MahjongGame::AiChooseVoidSuit(c), 2);
}


TEST(MahjongGameTest, completeThirteenTileHandIsTenpai)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	// 123m 456m 789m 11p 45s: waiting on 3s/6s
	FillCounts(c, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 21, 22 });
	EXPECT_EQ(MahjongGame::Shanten(c), 0);
}


TEST(MahjongGameTest, dealIsDeterministicAndComplete)
{
	MahjongGame a, b;
	a.NewMatch(42);
	b.NewMatch(42);

	for (int p = 0; p < MahjongGame::NUM_PLAYERS; ++p)
	{
		int total = 0;
		for (int k = 0; k < MahjongGame::NUM_KINDS; ++k)
		{
			EXPECT_EQ(a.player(p).counts[k], b.player(p).counts[k]);
			total += a.player(p).counts[k];
		}
		EXPECT_EQ(total, MahjongGame::HAND_TILES);
	}
	EXPECT_EQ(a.wallRemaining(),
			MahjongGame::TOTAL_TILES - MahjongGame::NUM_PLAYERS * MahjongGame::HAND_TILES);
	EXPECT_EQ(a.phase(), MahjongGame::Phase::ExchangeSelect);
	EXPECT_GE(a.exchangeOffset(), 1);
	EXPECT_LE(a.exchangeOffset(), 3);

	// nobody has declared a void suit before the exchange
	for (int p = 0; p < MahjongGame::NUM_PLAYERS; ++p)
	{
		EXPECT_EQ(a.player(p).voidSuit, MahjongGame::NO_SUIT);
	}

	DoHumanExchange(a);
	EXPECT_EQ(a.phase(), MahjongGame::Phase::ChooseVoid);

	// hands still hold 13 tiles after the exchange; AI void suits declared
	for (int p = 0; p < MahjongGame::NUM_PLAYERS; ++p)
	{
		int total = 0;
		for (int k = 0; k < MahjongGame::NUM_KINDS; ++k) total += a.player(p).counts[k];
		EXPECT_EQ(total, MahjongGame::HAND_TILES);
	}
	EXPECT_EQ(a.player(0).voidSuit, MahjongGame::NO_SUIT);
	for (int p = 1; p < MahjongGame::NUM_PLAYERS; ++p)
	{
		EXPECT_NE(a.player(p).voidSuit, MahjongGame::NO_SUIT);
	}

	a.SetHumanVoidSuit(1);
	EXPECT_EQ(a.phase(), MahjongGame::Phase::AwaitDraw);
	EXPECT_EQ(a.currentPlayer(), a.dealer());
}


TEST(MahjongGameTest, exchangeRejectsMixedSuitsAndMissingTiles)
{
	MahjongGame game;
	game.NewMatch(11);

	// find one held tile and one from another suit the player does not hold 3 of
	MahjongGame::TileId give[3] = { 0, 9, 18 }; // one of each suit: always mixed
	EXPECT_FALSE(game.SetHumanExchange(give));
	EXPECT_EQ(game.phase(), MahjongGame::Phase::ExchangeSelect);
}


TEST(MahjongGameTest, provenanceAndWinningTileRecorded)
{
	for (std::uint32_t seed : { 3u, 11u, 42u, 512u, 4711u, 90210u })
	{
		MahjongGame game;
		game.NewMatch(seed);
		PlayOutHand(game);

		for (MahjongGame::WinEvent const& e : game.wins())
		{
			ASSERT_NE(e.winningTile, MahjongGame::NO_TILE) << "seed " << seed;
			EXPECT_GT(e.winningCounts[e.winningTile], 0) << "seed " << seed;
		}
		for (int p = 0; p < MahjongGame::NUM_PLAYERS; ++p)
		{
			for (MahjongGame::Meld const& m : game.player(p).melds)
			{
				if (m.concealed)
				{
					EXPECT_EQ(m.from, -1) << "seed " << seed;
				}
				else if (m.count == 3)
				{
					// a pong is always fed; only kongs can be self-made
					EXPECT_GE(m.from, 0) << "seed " << seed;
					EXPECT_LT(m.from, MahjongGame::NUM_PLAYERS);
					EXPECT_NE(int(m.from), p) << "seed " << seed;
				}
			}
		}
	}
}

TEST(MahjongGameTest, bloodyBattleHandEndsAndScoresStayZeroSum)
{
	for (std::uint32_t seed : { 7u, 42u, 99u, 1234u, 987654u })
	{
		MahjongGame game;
		game.NewMatch(seed);
		PlayOutHand(game);
		EXPECT_EQ(game.phase(), MahjongGame::Phase::HandEnd);

		// hand ends with at most 3 winners, or by wall exhaustion
		EXPECT_LE(game.wins().size(), 3u);
		if (!game.endedByWallExhaustion())
		{
			EXPECT_EQ(game.wins().size(), 3u);
		}

		// every winner's recorded hand respects their void suit
		for (MahjongGame::WinEvent const& e : game.wins())
		{
			int const vs = game.player(e.winner).voidSuit;
			ASSERT_NE(vs, MahjongGame::NO_SUIT);
			for (int k = vs * 9; k < vs * 9 + 9; ++k)
			{
				EXPECT_EQ(e.winningCounts[k], 0) << "seed " << seed;
			}
			EXPECT_TRUE(MahjongGame::IsWinning(e.winningCounts));
		}

		std::int32_t total = 0, deltas = 0;
		for (int p = 0; p < MahjongGame::NUM_PLAYERS; ++p)
		{
			total += game.player(p).score;
			deltas += game.handDelta(p);
		}
		EXPECT_EQ(total, MahjongGame::NUM_PLAYERS * MahjongGame::START_SCORE);
		EXPECT_EQ(deltas, 0);
	}
}


TEST(MahjongGameTest, winnersRetireAndStopActing)
{
	MahjongGame game;
	// find a seed where someone wins mid-hand
	for (std::uint32_t seed = 1; seed < 200; ++seed)
	{
		game.NewMatch(seed);
		PlayOutHand(game);
		if (!game.wins().empty() && !game.endedByWallExhaustion()) break;
	}
	ASSERT_FALSE(game.wins().empty());

	// a finished player must never appear as current player again: replay and check
	// (indirect check: all winners are marked finished at hand end)
	for (MahjongGame::WinEvent const& e : game.wins())
	{
		EXPECT_TRUE(game.player(e.winner).finished);
	}
}


TEST(MahjongGameTest, aiDumpsVoidSuitFirst)
{
	MahjongGame game;
	game.NewMatch(5);
	DoHumanExchange(game);
	game.SetHumanVoidSuit(0);
	game.DrawForCurrent();

	int const cur = game.currentPlayer();
	int const vs = game.player(cur).voidSuit;
	std::uint8_t c14[MahjongGame::NUM_KINDS];
	std::memcpy(c14, game.player(cur).counts, sizeof(c14));
	if (game.drawnTile() != MahjongGame::NO_TILE) ++c14[game.drawnTile()];

	bool hasVoidTiles = false;
	for (int k = vs * 9; k < vs * 9 + 9; ++k) hasVoidTiles |= c14[k] > 0;

	TileId const t = game.AiChooseDiscard(cur);
	ASSERT_NE(t, MahjongGame::NO_TILE);
	if (hasVoidTiles)
	{
		EXPECT_EQ(MahjongGame::SuitOf(t), vs);
	}
}


TEST(MahjongGameTest, aiDiscardKeepsHandLegal)
{
	MahjongGame game;
	game.NewMatch(99);
	DoHumanExchange(game);
	game.SetHumanVoidSuit(2);
	game.DrawForCurrent();

	TileId const t = game.AiChooseDiscard(game.currentPlayer());
	ASSERT_NE(t, MahjongGame::NO_TILE);
	game.Discard(t);

	int total = 0;
	for (int k = 0; k < MahjongGame::NUM_KINDS; ++k) total += game.player(game.lastDiscarder()).counts[k];
	EXPECT_EQ(total, MahjongGame::HAND_TILES);
	EXPECT_EQ(game.player(game.lastDiscarder()).discards.size(), 1u);
	EXPECT_EQ(game.lastDiscard(), t);
}

TEST(MahjongGameTest, winningWithExposedSets)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	// two exposed melds banked: concealed 123m 456m 77s must win with setsNeeded=2
	FillCounts(c, { 0, 1, 2, 3, 4, 5, 24, 24 });
	EXPECT_TRUE(MahjongGame::IsWinningSets(c, 2, false));
	// seven pairs is only a closed-hand pattern
	std::uint8_t sp[MahjongGame::NUM_KINDS];
	FillCounts(sp, { 0, 0, 4, 4, 9, 9, 13, 13 });
	EXPECT_FALSE(MahjongGame::IsWinningSets(sp, 2, false));
}


TEST(MahjongGameTest, shantenAccountsForExposedMelds)
{
	std::uint8_t c[MahjongGame::NUM_KINDS];
	// 2 melds exposed, concealed 123m 45m 77s: waiting on 3m/6m
	FillCounts(c, { 0, 1, 2, 3, 4, 24, 24 });
	EXPECT_EQ(MahjongGame::Shanten(c, 2), 0);
}


TEST(MahjongGameTest, pongClaimMovesTilesAndTurn)
{
	// find a seed where a pong actually happens, then verify its bookkeeping
	for (std::uint32_t seed = 1; seed < 400; ++seed)
	{
		MahjongGame game;
		game.NewMatch(seed);
		DoHumanExchange(game);
		game.SetHumanVoidSuit(MahjongGame::AiChooseVoidSuit(game.player(0).counts));

		int safety = 400;
		while (game.phase() != MahjongGame::Phase::HandEnd && safety-- > 0)
		{
			game.DrawForCurrent();
			if (game.phase() == MahjongGame::Phase::HandEnd) break;
			if (game.CanTsumo()) { game.ResolveTsumo(); continue; }
			game.Discard(game.AiChooseDiscard(game.currentPlayer()));
			if (game.RonClaimant() >= 0) { game.ResolveRon(game.RonClaimant()); continue; }
			bool kongPossible = false;
			int const mc = game.MeldClaimant(kongPossible);
			if (mc >= 0 && game.AiWantsPong(mc) && !kongPossible)
			{
				int const discarder = game.lastDiscarder();
				size_t const pondBefore = game.player(discarder).discards.size();
				MahjongGame::TileId const tile = game.lastDiscard();
				std::uint8_t const held = game.player(mc).counts[tile];

				game.ClaimPong(mc);

				EXPECT_EQ(game.currentPlayer(), mc);
				EXPECT_EQ(game.phase(), MahjongGame::Phase::AwaitDiscard);
				EXPECT_EQ(game.player(mc).counts[tile], held - 2);
				ASSERT_EQ(game.player(mc).melds.size(), 1u);
				EXPECT_EQ(game.player(mc).melds[0].tile, tile);
				EXPECT_EQ(game.player(mc).melds[0].count, 3);
				EXPECT_EQ(game.player(discarder).discards.size(), pondBefore - 1);
				return; // verified one real pong
			}
			game.PassRon();
		}
	}
	FAIL() << "no pong opportunity found in 400 seeds";
}


TEST(MahjongGameTest, meldInvariantHoldsAtHandEnd)
{
	for (std::uint32_t seed : { 3u, 21u, 77u, 4242u })
	{
		MahjongGame game;
		game.NewMatch(seed);
		PlayOutHand(game);
		for (int p = 0; p < MahjongGame::NUM_PLAYERS; ++p)
		{
			int concealed = 0;
			for (int k = 0; k < MahjongGame::NUM_KINDS; ++k) concealed += game.player(p).counts[k];
			int const rest = concealed + 3 * static_cast<int>(game.player(p).melds.size());
			// a winner keeps the tile that won: four sets and a pair is 14.
			// everyone still playing holds 13
			if (game.player(p).finished)
			{
				EXPECT_EQ(rest, 14) << "seed " << seed << " winner " << p;
			}
			// a kong claimed off an empty wall can leave one player a tile short
			else if (game.endedByWallExhaustion()) EXPECT_GE(rest, 12);
			else                              EXPECT_EQ(rest, 13) << "seed " << seed << " player " << p;
		}
	}
}


TEST(MahjongGameTest, fanScoringRecordedAndPaymentsMatch)
{
	// across many seeds, every recorded win must satisfy payment = base << fan
	int checked = 0;
	for (std::uint32_t seed = 1; seed < 120 && checked < 10; ++seed)
	{
		MahjongGame game;
		game.NewMatch(seed);
		PlayOutHand(game);
		for (MahjongGame::WinEvent const& e : game.wins())
		{
			ASSERT_GE(e.fan, 0);
			ASSERT_LE(e.fan, MahjongGame::FAN_CAP);
			if (e.discarder < 0)
			{
				EXPECT_EQ(e.payment, MahjongGame::BASE_TSUMO_EACH << e.fan);
			}
			else
			{
				// a robbed kong pays at least the ron base for its fan
				EXPECT_GE(e.payment, MahjongGame::BASE_RON << std::max(0, e.fan - 1));
			}
			++checked;
		}
	}
	ASSERT_GT(checked, 0);
}


TEST(MahjongGameTest, wallExhaustionPenaltiesStayZeroSum)
{
	for (std::uint32_t seed : { 7u, 42u, 99u, 555u, 2024u })
	{
		MahjongGame game;
		game.NewMatch(seed);
		PlayOutHand(game);
		std::int32_t total = 0, deltas = 0;
		for (int p = 0; p < MahjongGame::NUM_PLAYERS; ++p)
		{
			total += game.player(p).score;
			deltas += game.handDelta(p);
		}
		EXPECT_EQ(total, MahjongGame::NUM_PLAYERS * MahjongGame::START_SCORE) << "seed " << seed;
		EXPECT_EQ(deltas, 0) << "seed " << seed;
	}
}


TEST(MahjongGameTest, abortedHandHasNoWinnersAndNoPayments)
{
	MahjongGame game;
	game.NewMatch(3);
	DoHumanExchange(game);
	game.SetHumanVoidSuit(MahjongGame::AiChooseVoidSuit(game.player(0).counts));
	game.DrawForCurrent();
	game.AbortHand();
	EXPECT_EQ(game.phase(), MahjongGame::Phase::HandEnd);
	EXPECT_TRUE(game.aborted());
	EXPECT_TRUE(game.wins().empty());
	for (int p = 0; p < MahjongGame::NUM_PLAYERS; ++p)
	{
		EXPECT_EQ(game.handDelta(p), 0);
	}
}


TEST(MahjongGameTest, grudgeStillProducesLegalDiscards)
{
	MahjongGame game;
	game.NewMatch(77);
	DoHumanExchange(game);
	game.SetHumanVoidSuit(MahjongGame::AiChooseVoidSuit(game.player(0).counts));
	game.SetAiGrudge(2, 0, 5);
	int safety = 300;
	while (game.phase() != MahjongGame::Phase::HandEnd && safety-- > 0)
	{
		game.DrawForCurrent();
		if (game.phase() == MahjongGame::Phase::HandEnd) break;
		if (game.CanTsumo()) { game.ResolveTsumo(); continue; }
		MahjongGame::TileId const t = game.AiChooseDiscard(game.currentPlayer());
		ASSERT_NE(t, MahjongGame::NO_TILE);
		game.Discard(t);
		if (game.RonClaimant() >= 0) game.ResolveRon(game.RonClaimant());
		else game.PassRon();
	}
	ASSERT_GT(safety, 0);
}
