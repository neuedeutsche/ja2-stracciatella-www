#include "gtest/gtest.h"

#include "ChessGame.h"

#include <string>
#include <vector>

namespace
{
	using Move = ChessGame::Move;

	// Perft is the standard move-generator proof: walk the tree to a fixed
	// depth and count leaves. Every legality rule - pins, en passant, castling
	// through check, promotion - shows up as a wrong count if it is broken.
	struct PerftCase
	{
		const char*   name;
		const char*   fen;
		int           depth;
		std::uint64_t nodes;
	};

	// Positions 1-6 from the Chess Programming Wiki's perft results page.
	const PerftCase PERFT_CASES[] = {
		{ "start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20 },
		{ "start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400 },
		{ "start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902 },
		{ "start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281 },
		{ "start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609 },

		{ "kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48 },
		{ "kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039 },
		{ "kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862 },
		{ "kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603 },

		{ "position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1, 14 },
		{ "position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2, 191 },
		{ "position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 3, 2812 },
		{ "position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238 },
		{ "position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624 },

		{ "position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 1, 6 },
		{ "position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2, 264 },
		{ "position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467 },
		{ "position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333 },

		{ "position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 1, 44 },
		{ "position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 2, 1486 },
		{ "position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379 },
		{ "position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487 },

		{ "position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 1, 46 },
		{ "position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 2, 2079 },
		{ "position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890 },
	};
}

TEST(ChessGame, PerftMatchesKnownCounts)
{
	for (const PerftCase& c : PERFT_CASES)
	{
		ChessGame game;
		ASSERT_TRUE(game.SetFen(c.fen)) << c.name;
		EXPECT_EQ(c.nodes, game.Perft(c.depth))
			<< c.name << " at depth " << c.depth;
	}
}

TEST(ChessGame, FenRoundTrips)
{
	const char* const fens[] = {
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
		"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
		"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
	};
	for (const char* fen : fens)
	{
		ChessGame game;
		ASSERT_TRUE(game.SetFen(fen)) << fen;
		EXPECT_EQ(std::string(fen), game.Fen());
	}
}

TEST(ChessGame, RejectsMalformedFen)
{
	ChessGame game;
	const std::string before = game.Fen();
	EXPECT_FALSE(game.SetFen(""));
	EXPECT_FALSE(game.SetFen("not a fen at all"));
	EXPECT_FALSE(game.SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1"));
	EXPECT_FALSE(game.SetFen("8/8/8/8/8/8/8/8 w - - 0 1"));            // no kings
	EXPECT_FALSE(game.SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w - - 0 1"));  // 7 ranks
	// a rejected FEN must leave the previous position untouched
	EXPECT_EQ(before, game.Fen());
}

TEST(ChessGame, MakeUnmakeRestoresPositionExactly)
{
	ChessGame game;
	ASSERT_TRUE(game.SetFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"));
	const std::string fen = game.Fen();
	const std::uint64_t key = game.ZobristKey();

	Move moves[ChessGame::MAX_MOVES];
	const int count = game.GenerateLegal(moves);
	ASSERT_GT(count, 0);
	for (int i = 0; i < count; ++i)
	{
		ASSERT_TRUE(game.MakeMove(moves[i]));
		game.Unmake();
		EXPECT_EQ(fen, game.Fen()) << "after move " << i;
		EXPECT_EQ(key, game.ZobristKey()) << "after move " << i;
	}
}

TEST(ChessGame, DetectsMateStalemateAndDraws)
{
	ChessGame game;

	// fool's mate, Black has delivered mate
	ASSERT_TRUE(game.SetFen("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3"));
	EXPECT_EQ(ChessGame::Result::BlackMates, game.GetResult());

	// classic stalemate: Black to move, no legal move, not in check
	ASSERT_TRUE(game.SetFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1"));
	EXPECT_EQ(ChessGame::Result::Stalemate, game.GetResult());

	// bare kings
	ASSERT_TRUE(game.SetFen("8/8/4k3/8/8/3K4/8/8 w - - 0 1"));
	EXPECT_EQ(ChessGame::Result::DrawInsufficient, game.GetResult());

	// king and one bishop cannot mate
	ASSERT_TRUE(game.SetFen("8/8/4k3/8/8/3K1B2/8/8 w - - 0 1"));
	EXPECT_EQ(ChessGame::Result::DrawInsufficient, game.GetResult());

	// the halfmove clock alone ends it
	ASSERT_TRUE(game.SetFen("8/8/4k3/8/8/3K4/4R3/8 w - - 100 80"));
	EXPECT_EQ(ChessGame::Result::DrawFiftyMove, game.GetResult());

	ASSERT_TRUE(game.SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
	EXPECT_EQ(ChessGame::Result::Ongoing, game.GetResult());
}

TEST(ChessGame, DetectsThreefoldRepetition)
{
	ChessGame game;
	ASSERT_TRUE(game.SetFen("4k3/8/8/8/8/8/8/R3K2R w - - 0 1"));
	// shuffle both rooks back and forth until the start position appears a
	// third time
	const char* const shuffle[] = {
		"a1b1", "e8d8", "b1a1", "d8e8",
		"a1b1", "e8d8", "b1a1", "d8e8",
	};
	for (const char* uci : shuffle)
	{
		const Move m = game.ParseUci(uci);
		ASSERT_FALSE(m.IsNull()) << uci;
		ASSERT_TRUE(game.MakeMove(m)) << uci;
	}
	EXPECT_EQ(ChessGame::Result::DrawRepetition, game.GetResult());
}

TEST(ChessGame, SanNotation)
{
	ChessGame game;

	ASSERT_TRUE(game.SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
	EXPECT_EQ("e4", game.San(game.ParseUci("e2e4")));
	EXPECT_EQ("Nf3", game.San(game.ParseUci("g1f3")));

	// castling
	ASSERT_TRUE(game.SetFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"));
	EXPECT_EQ("O-O", game.San(game.ParseUci("e1g1")));
	EXPECT_EQ("O-O-O", game.San(game.ParseUci("e1c1")));

	// mate suffix
	ASSERT_TRUE(game.SetFen("3rk2r/pp1n1pp1/2q1p2p/4P3/1b1P4/P1N2Q2/1P4PP/R1B2RK1 b k - 0 16"));
	ASSERT_TRUE(game.MakeMove(game.ParseUci("b4c3")));
	EXPECT_EQ("Qxf7#", game.San(game.ParseUci("f3f7")));

	// file disambiguation between two knights that both reach d2
	ASSERT_TRUE(game.SetFen("4k3/8/8/8/8/8/8/1N1K1N2 w - - 0 1"));
	EXPECT_EQ("Nbd2", game.San(game.ParseUci("b1d2")));
	EXPECT_EQ("Nfd2", game.San(game.ParseUci("f1d2")));

	// pawn capture and promotion (the new queen does not see h1, so no suffix)
	ASSERT_TRUE(game.SetFen("3r4/4P3/8/8/8/8/8/4K2k w - - 0 1"));
	EXPECT_EQ("exd8=Q", game.San(game.ParseUci("e7d8q")));
	EXPECT_EQ("exd8=N", game.San(game.ParseUci("e7d8n")));
}

TEST(ChessGame, EnPassantIsGeneratedAndUndone)
{
	ChessGame game;
	ASSERT_TRUE(game.SetFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2"));
	const Move ep = game.ParseUci("e5d6");
	ASSERT_FALSE(ep.IsNull());
	EXPECT_TRUE(ep.flags & ChessGame::MF_EN_PASSANT);

	const std::string before = game.Fen();
	ASSERT_TRUE(game.MakeMove(ep));
	EXPECT_TRUE(game.IsEmpty(ChessGame::MakeSquare(3, 4)));  // the captured pawn is gone
	game.Unmake();
	EXPECT_EQ(before, game.Fen());
}

TEST(ChessGame, SearchFindsMateInOne)
{
	ChessGame game;
	std::uint32_t seed = 12345;

	// back-rank mate
	ASSERT_TRUE(game.SetFen("6k1/5ppp/8/8/8/8/8/R3K3 w - - 0 1"));
	EXPECT_EQ("Ra8#", game.San(game.Search(3, 0, seed)));

	// Qxf7 smothered by the bishop pair
	ASSERT_TRUE(game.SetFen("r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4"));
	EXPECT_EQ(ChessGame::Result::WhiteMates, game.GetResult());
}

TEST(ChessGame, SearchAlwaysRestoresThePosition)
{
	ChessGame game;
	ASSERT_TRUE(game.SetFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"));
	const std::string before = game.Fen();
	std::uint32_t seed = 99;
	const Move m = game.Search(3, 0, seed);
	EXPECT_FALSE(m.IsNull());
	EXPECT_EQ(before, game.Fen());
}
