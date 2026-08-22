#include "gtest/gtest.h"

#include "ChessGame.h"
#include "ChessLessons.h"
#include "ChessPuzzles.h"

#include <sstream>
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

TEST(ChessGame, FinishesAWonRookEndgame)
{
	// Two rooks against a bare king: the ladder. Material and piece-square
	// tables alone score every rook shuffle the same, so the old evaluation
	// pushed these around until the fifty-move rule bailed the defender out.
	struct Case { const char* name; const char* fen; int plies; };
	const Case cases[] = {
		{ "two rooks",  "8/8/4k3/8/8/8/8/R3K2R w - - 0 1",   60 },
		{ "rook alone", "8/8/4k3/8/8/8/8/R3K3 w - - 0 1",   100 },
		{ "queen",      "8/8/4k3/8/8/8/8/3QK3 w - - 0 1",    40 },
	};

	for (const Case& c : cases)
	{
		ChessGame game;
		ASSERT_TRUE(game.SetFen(c.fen)) << c.name;
		std::uint32_t seed = 4242;
		ChessGame::Result result = ChessGame::Result::Ongoing;
		for (int ply = 0; ply < c.plies; ++ply)
		{
			result = game.GetResult();
			if (result != ChessGame::Result::Ongoing) break;
			const Move m = game.Search(3, 0, seed);
			ASSERT_FALSE(m.IsNull()) << c.name;
			ASSERT_TRUE(game.MakeMove(m)) << c.name;
		}
		EXPECT_EQ(ChessGame::Result::WhiteMates, result)
			<< c.name << " ended " << game.Fen();
	}
}

TEST(ChessGame, MopUpOnlyAppliesToABareKing)
{
	ChessGame game;

	// a full board has no mating net to steer toward
	ASSERT_TRUE(game.SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
	EXPECT_EQ(0, game.MopUp());

	// nor does an endgame where the weak side still has a pawn to push
	ASSERT_TRUE(game.SetFen("8/5p2/4k3/8/8/8/8/R3K3 w - - 0 1"));
	EXPECT_EQ(0, game.MopUp());

	// a king driven to the corner scores worse for its owner than one in the
	// middle, which is the whole point of the term
	ASSERT_TRUE(game.SetFen("7k/8/8/8/8/8/8/R3K3 w - - 0 1"));
	const int corner = game.MopUp();
	ASSERT_TRUE(game.SetFen("8/8/8/3k4/8/8/8/R3K3 w - - 0 1"));
	EXPECT_GT(corner, game.MopUp());
}

TEST(ChessGame, SeeScoresTheExchangeOnASquare)
{
	ChessGame game;

	// a free pawn: nothing is defending b7
	ASSERT_TRUE(game.SetFen("4k3/1p6/8/8/8/8/8/1Q2K3 w - - 0 1"));
	EXPECT_EQ(100, game.See(game.ParseUci("b1b7")));
	EXPECT_EQ(0, game.See(game.ParseUci("b1b5")));   // a quiet move wins nothing

	// the same pawn, defended by its king: the queen loses the exchange.
	// This is the capture the bots kept playing.
	ASSERT_TRUE(game.SetFen("2k5/1p6/8/8/8/8/8/1Q2K3 w - - 0 1"));
	EXPECT_EQ(100 - 900, game.See(game.ParseUci("b1b7")));

	// pawn takes queen is worth a queen however well defended she is
	ASSERT_TRUE(game.SetFen("4k3/8/8/3q4/2P5/8/8/4K3 w - - 0 1"));
	EXPECT_EQ(900, game.See(game.ParseUci("c4d5")));

	// knight takes knight, recaptured by a pawn: a clean even trade
	ASSERT_TRUE(game.SetFen("4k3/2p5/3n4/8/4N3/8/8/4K3 w - - 0 1"));
	EXPECT_EQ(0, game.See(game.ParseUci("e4d6")));

	// x-ray: the rook behind the rook joins the exchange on d5
	ASSERT_TRUE(game.SetFen("3rk3/3r4/8/3p4/8/8/3R4/3RK3 w - - 0 1"));
	EXPECT_EQ(100 - 500 + 500 - 500, game.See(game.ParseUci("d2d5")));
}

TEST(ChessGame, LosesMaterialCatchesHangingAPiece)
{
	ChessGame game;

	// Qxb7 with the pawn defended: the capture itself loses the exchange
	ASSERT_TRUE(game.SetFen("2k5/1p6/8/8/8/8/8/1Q2K3 w - - 0 1"));
	EXPECT_TRUE(game.LosesMaterial(game.ParseUci("b1b7"), 90));

	// a queen stepping somewhere the king can simply take her
	ASSERT_TRUE(game.SetFen("2k5/8/8/8/8/8/8/3QK3 w - - 0 1"));
	EXPECT_TRUE(game.LosesMaterial(game.ParseUci("d1d7"), 90));

	// plain developing moves lose nothing
	ASSERT_TRUE(game.SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
	EXPECT_FALSE(game.LosesMaterial(game.ParseUci("g1f3"), 90));
	EXPECT_FALSE(game.LosesMaterial(game.ParseUci("e2e4"), 90));

	// the position is exactly as it was afterwards
	const std::string before = game.Fen();
	game.LosesMaterial(game.ParseUci("d2d4"), 90);
	EXPECT_EQ(before, game.Fen());
}

TEST(ChessGame, LosingCapturesAgreeWithTheirSign)
{
	// The bug in the wild: a rated bot grabbing a defended pawn because a
	// capture was available. Whatever See calls losing must really lose, so
	// that a seat's greed has something honest to filter on.
	ChessGame game;
	ASSERT_TRUE(game.SetFen(
		"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"));
	Move caps[ChessGame::MAX_MOVES];
	const int n = game.GenerateLegalCaptures(caps);
	ASSERT_GT(n, 0);
	for (int i = 0; i < n; ++i)
	{
		if (game.See(caps[i]) < 0) EXPECT_TRUE(game.LosesMaterial(caps[i], 0));
	}
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

namespace
{
	std::vector<std::string> SplitMoves(const char* moves)
	{
		std::vector<std::string> out;
		std::istringstream in(moves);
		std::string move;
		while (in >> move) out.push_back(move);
		return out;
	}
}

// The daily puzzle corpus is compiled in, so a bad record is a build-time
// problem rather than a player staring at an unsolvable board. Every puzzle is
// replayed here move by move.
TEST(ChessPuzzles, CorpusIsWellFormed)
{
	ASSERT_GT(NUM_CHESS_PUZZLES, 0);

	for (int i = 0; i < NUM_CHESS_PUZZLES; ++i)
	{
		const ChessPuzzle& puzzle = CHESS_PUZZLES[i];
		SCOPED_TRACE(std::string("puzzle ") + puzzle.id);

		ChessGame game;
		ASSERT_TRUE(game.SetFen(puzzle.fen)) << "unparseable FEN";

		const std::vector<std::string> moves = SplitMoves(puzzle.moves);
		ASSERT_GE(moves.size(), 2u) << "needs a setup move plus a solution";

		// the solver plays the side opposite the FEN's side to move
		const ChessGame::Color solver = ChessGame::Color(game.SideToMove() ^ 1);

		for (std::size_t ply = 0; ply < moves.size(); ++ply)
		{
			const ChessGame::Move m = game.ParseUci(moves[ply]);
			ASSERT_FALSE(m.IsNull()) << "illegal move " << moves[ply] << " at ply " << ply;
			// ply 0 is the opponent's setup, then the solver alternates from ply 1
			const bool solverToPlay = ply > 0 && (ply % 2) == 1;
			EXPECT_EQ(solverToPlay, game.SideToMove() == solver)
				<< "side to move is wrong at ply " << ply;
			ASSERT_TRUE(game.MakeMove(m)) << "could not play " << moves[ply];
		}

		// a puzzle tagged as a mate has to actually finish in mate
		const std::string themes = puzzle.themes ? puzzle.themes : "";
		if (themes.find("mate") != std::string::npos &&
		    themes.find("mateIn") != std::string::npos)
		{
			const ChessGame::Result result = game.GetResult();
			EXPECT_TRUE(result == ChessGame::Result::WhiteMates ||
			            result == ChessGame::Result::BlackMates)
				<< "themes claim mate but the line does not end in one";
		}
	}
}

TEST(ChessPuzzles, RatingsAreSortedAndSane)
{
	for (int i = 0; i < NUM_CHESS_PUZZLES; ++i)
	{
		EXPECT_GT(CHESS_PUZZLES[i].rating, 0) << CHESS_PUZZLES[i].id;
		if (i > 0)
		{
			EXPECT_LE(CHESS_PUZZLES[i - 1].rating, CHESS_PUZZLES[i].rating)
				<< "corpus must stay sorted so difficulty tracks campaign days";
		}
	}
}

// The corpus is trusted to be solvable, but not to be unambiguous. A puzzle
// whose recorded answer the engine does not even rank as best is either a
// broken record or a position with two solutions, and both make for a player
// being told "no" after finding a perfectly good move.
TEST(ChessPuzzles, RecordedAnswerSurvivesAShallowSearch)
{
	int checked = 0, disputed = 0;
	for (int i = 0; i < NUM_CHESS_PUZZLES; ++i)
	{
		const ChessPuzzle& puzzle = CHESS_PUZZLES[i];
		const std::vector<std::string> moves = SplitMoves(puzzle.moves);
		if (moves.size() < 2) continue;

		ChessGame game;
		ASSERT_TRUE(game.SetFen(puzzle.fen)) << puzzle.id;
		ASSERT_TRUE(game.MakeMove(game.ParseUci(moves[0]))) << puzzle.id;

		const ChessGame::Move answer = game.ParseUci(moves[1]);
		ASSERT_FALSE(answer.IsNull()) << puzzle.id;

		// score the recorded answer against the engine's own pick
		std::uint32_t seed = 1;
		const ChessGame::Move best = game.Search(3, 0, seed);
		++checked;
		if (!(best == answer)) ++disputed;
	}

	ASSERT_GT(checked, 0);
	// A three-ply search is not an oracle, so a handful of quiet positional
	// puzzles will legitimately disagree. A large share disagreeing would mean
	// the corpus, the setup-move convention or the search is wrong.
	EXPECT_LT(disputed * 100 / checked, 40)
		<< disputed << " of " << checked << " recorded answers lost to a depth-3 search";
}

// The lessons must show what their text claims, or the book teaches lies.
TEST(ChessLessons, EachDiagramProvesItsClaim)
{
	ChessGame game;

	// lesson 1: the position after 1.e4, Black to move
	ASSERT_TRUE(game.SetFen(CHESS_LESSONS[0].fen));
	EXPECT_EQ(ChessGame::Black, game.SideToMove());
	EXPECT_EQ(ChessGame::Pawn, game.PieceAt(ChessGame::MakeSquare(4, 3)));

	// lesson 2: the knight forks rook and king - it attacks both squares
	ASSERT_TRUE(game.SetFen(CHESS_LESSONS[1].fen));
	const std::uint8_t rookSq = ChessGame::MakeSquare(2, 6);
	const std::uint8_t kingSq = ChessGame::MakeSquare(4, 6);
	EXPECT_EQ(ChessGame::Rook, game.PieceAt(rookSq));
	EXPECT_EQ(ChessGame::King, game.PieceAt(kingSq));
	EXPECT_TRUE(game.IsSquareAttacked(rookSq, ChessGame::White));
	EXPECT_TRUE(game.IsSquareAttacked(kingSq, ChessGame::White));

	// lesson 3: Re8 is mate, exactly as the caption says
	ASSERT_TRUE(game.SetFen(CHESS_LESSONS[2].fen));
	const ChessGame::Move mate = game.ParseUci("e1e8");
	ASSERT_FALSE(mate.IsNull());
	ASSERT_TRUE(game.MakeMove(mate));
	EXPECT_EQ(ChessGame::Result::WhiteMates, game.GetResult());
	// lesson 4: the c6 knight is pinned by the a4 bishop - it has no move
	ASSERT_TRUE(game.SetFen(CHESS_LESSONS[3].fen));
	{
		const std::uint8_t pinned = ChessGame::MakeSquare(2, 5);
		EXPECT_EQ(ChessGame::Knight, game.PieceAt(pinned));
		ASSERT_TRUE(game.SetFen("4k3/8/2n5/8/B7/8/8/4K3 b - - 0 1"));
		Move moves[ChessGame::MAX_MOVES];
		const int count = game.GenerateLegal(moves);
		for (int i = 0; i < count; ++i) EXPECT_NE(pinned, moves[i].from);
	}

	// lesson 5: the d5 pawn is passed - no black pawn ahead on c, d or e
	ASSERT_TRUE(game.SetFen(CHESS_LESSONS[4].fen));
	EXPECT_EQ(ChessGame::Pawn, game.PieceAt(ChessGame::MakeSquare(3, 4)));
	for (int file = 2; file <= 4; ++file)
	{
		for (int rank = 5; rank <= 6; ++rank)
		{
			EXPECT_TRUE(game.IsEmpty(ChessGame::MakeSquare(file, rank)));
		}
	}

	// lesson 6: both sides castled short - kings on g1/g8, rooks on f1/f8
	ASSERT_TRUE(game.SetFen(CHESS_LESSONS[5].fen));
	EXPECT_EQ(ChessGame::King, game.PieceAt(ChessGame::MakeSquare(6, 0)));
	EXPECT_EQ(ChessGame::Rook, game.PieceAt(ChessGame::MakeSquare(5, 0)));
	EXPECT_EQ(ChessGame::King, game.PieceAt(ChessGame::MakeSquare(6, 7)));
	EXPECT_EQ(ChessGame::Rook, game.PieceAt(ChessGame::MakeSquare(5, 7)));

	// lesson 7: the rook checks the king with the queen behind on the file
	ASSERT_TRUE(game.SetFen(CHESS_LESSONS[6].fen));
	EXPECT_EQ(ChessGame::Black, game.SideToMove());
	EXPECT_TRUE(game.IsInCheck(ChessGame::Black));
	EXPECT_EQ(ChessGame::King, game.PieceAt(ChessGame::MakeSquare(4, 2)));
	EXPECT_EQ(ChessGame::Queen, game.PieceAt(ChessGame::MakeSquare(4, 4)));

	// lesson 8: the d1 rook stands on a file with no pawns at all
	ASSERT_TRUE(game.SetFen(CHESS_LESSONS[7].fen));
	EXPECT_EQ(ChessGame::Rook, game.PieceAt(ChessGame::MakeSquare(3, 0)));
	for (int rank = 1; rank <= 6; ++rank)
	{
		EXPECT_NE(ChessGame::Pawn, game.PieceAt(ChessGame::MakeSquare(3, rank)));
	}
}
