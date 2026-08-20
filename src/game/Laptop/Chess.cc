// chach.com: Grunty's daily chess puzzle, run off a box in his apartment.
//
// One puzzle per campaign day, five attempts, a streak that breaks if you skip
// a day. The layout is chess.com's current one - nav rail, board, right-hand
// panel - reproduced at the 502x400 the laptop's web area gives us.
//
// All rules live in the engine-free ChessGame class and the corpus in the
// generated ChessPuzzles.cc; this file is the laptop page wrapper.

#include "Chess.h"

#include "ChessGame.h"
#include "ChessPuzzles.h"

#include "Button_System.h"
#include "Cursors.h"
#include "Directories.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "HImage.h"
#include "Laptop.h"
#include "MouseSystem.h"
#include "Soldier_Profile.h"
#include "Timer_Control.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <string_theory/format>
#include <string_theory/string>

#define CH_X(x) ((INT32)(LAPTOP_SCREEN_UL_X + (x)))
#define CH_Y(y) ((INT32)(LAPTOP_SCREEN_WEB_UL_Y + (y)))

// --- layout ---------------------------------------------------------------
// 68 rail + 6 gutter + 272 board + 6 gutter + 150 panel = 502
#define CH_NAV_W        68
#define CH_SQ           34
#define CH_BOARD_X      74
#define CH_BOARD_Y      24
#define CH_BOARD_SIZE   (8 * CH_SQ)
#define CH_BOARD_BOTTOM (CH_BOARD_Y + CH_BOARD_SIZE)
#define CH_PANEL_X      352
#define CH_PANEL_W      150
#define CH_PAGE_H       400
#define CH_HEART_Y      (CH_BOARD_BOTTOM + 12)
#define CH_HEART_W      13

#define CH_MAX_HEARTS   5
#define CH_REPLY_DELAY  650  // ms before the scripted reply lands

// chess.com's palette, sampled from the live site
#define CH_RGB_LIGHT       FROMRGB(235, 236, 208)
#define CH_RGB_DARK        FROMRGB(115, 149,  82)
#define CH_RGB_HL_LIGHT    FROMRGB(245, 246, 130)
#define CH_RGB_HL_DARK     FROMRGB(185, 202,  67)
#define CH_RGB_DOT_LIGHT   FROMRGB(200, 201, 178)
#define CH_RGB_DOT_DARK    FROMRGB( 98, 127,  70)
#define CH_RGB_CHROME      FROMRGB( 48,  46,  43)
#define CH_RGB_PANEL       FROMRGB( 38,  37,  34)
#define CH_RGB_PANEL_UP    FROMRGB( 60,  59,  57)
#define CH_RGB_CTA         FROMRGB(129, 182,  76)
#define CH_RGB_RULE        FROMRGB( 62,  60,  57)
#define CH_RGB_HEART       FROMRGB(201,  70,  70)
#define CH_RGB_HEART_SPENT FROMRGB( 70,  68,  65)

namespace
{
	enum ChessUiState
	{
		CHUI_PUZZLE,  // the player is solving
		CHUI_SOLVED,
		CHUI_FAILED,  // out of hearts; the solution is on the board
	};

	// Session-lifetime, rebuilt on every visit from the persisted daily state.
	ChessUiState             gChessState = CHUI_PUZZLE;
	ChessGame                gChessGame;
	int                      giChessPuzzle = 0;
	std::vector<std::string> gChessSolution;  // the line after the setup move
	std::size_t              guiChessPly = 0; // index of the next expected move
	ChessGame::Color         gChessSolver = ChessGame::White;

	INT8   gbChessSelected  = -1;    // 0x88 square, -1 when nothing is picked up
	UINT8  gubChessHearts   = CH_MAX_HEARTS;
	UINT32 guiChessReplyDue = 0;     // when the scripted reply lands, 0 if none
	UINT8  gubChessLastFrom = ChessGame::NO_SQUARE;
	UINT8  gubChessLastTo   = ChessGame::NO_SQUARE;
	bool   gfChessHintShown = false;
	ST::string gChessStatus;

	// persisted
	UINT16 gusChessDay        = 0;
	UINT16 gusChessLastSolved = 0;
	UINT8  gubChessStreak     = 0;
	UINT8  gubChessBestStreak = 0;
	UINT8  gubChessFlags      = 0;

	constexpr UINT8 CH_FLAG_SOLVED     = 0x01;
	constexpr UINT8 CH_FLAG_FAILED     = 0x02;
	constexpr UINT8 CH_FLAG_HINT_USED  = 0x04;
	constexpr UINT8 CH_FLAG_DISCOVERED = 0x08;

	SGPVObject* guiChessPieces = nullptr;  // 12 frames, 34x34
	SGPVObject* guiChessCoach  = nullptr;  // Grunty, 29x33

	MOUSE_REGION gChessSquare[64];
	MOUSE_REGION gChessHintRegion;
	bool         gfChessRegionsUp = false;

	// The solvers list is padded, obviously, and everyone knows it.
	const char* const CHESS_SOLVERS[] = {
		"@ivan_d", "@grunty", "@the_house", "@e11iot",
		"@no_refunds", "@shady_lady", "@ringside_d", "@666",
	};

	UINT32 ChessNow() { return GetJA2Clock(); }

	UINT16 ChessToday() { return static_cast<UINT16>(GetWorldDay()); }

	std::vector<std::string> SplitMoves(const char* moves)
	{
		std::vector<std::string> out;
		std::istringstream in(moves ? moves : "");
		std::string move;
		while (in >> move) out.push_back(move);
		return out;
	}
}

ChessPersist ChessGetPersist()
{
	ChessPersist p;
	p.usDay           = gusChessDay;
	p.usLastSolvedDay = gusChessLastSolved;
	p.ubStreak        = gubChessStreak;
	p.ubBestStreak    = gubChessBestStreak;
	p.ubHearts        = gubChessHearts;
	p.ubFlags         = gubChessFlags;
	return p;
}

void ChessSetPersist(const ChessPersist& p)
{
	gusChessDay        = p.usDay;
	gusChessLastSolved = p.usLastSolvedDay;
	gubChessStreak     = p.ubStreak;
	gubChessBestStreak = p.ubBestStreak;
	gubChessHearts     = std::min<UINT8>(p.ubHearts, CH_MAX_HEARTS);
	gubChessFlags      = p.ubFlags;
}

// --- board geometry -------------------------------------------------------

namespace
{
	// The board is drawn from the solver's side, so a puzzle where Black moves
	// arrives rotated - same as chess.com.
	void SquareToScreen(UINT8 sq, INT32& x, INT32& y)
	{
		const int file = ChessGame::FileOf(sq);
		const int rank = ChessGame::RankOf(sq);
		const int col  = gChessSolver == ChessGame::White ? file : 7 - file;
		const int row  = gChessSolver == ChessGame::White ? 7 - rank : rank;
		x = CH_BOARD_X + col * CH_SQ;
		y = CH_BOARD_Y + row * CH_SQ;
	}

	UINT8 ScreenToSquare(int col, int row)
	{
		const int file = gChessSolver == ChessGame::White ? col : 7 - col;
		const int rank = gChessSolver == ChessGame::White ? 7 - row : row;
		return ChessGame::MakeSquare(file, rank);
	}

	bool IsLightSquare(UINT8 sq)
	{
		return ((ChessGame::FileOf(sq) + ChessGame::RankOf(sq)) & 1) != 0;
	}

	void FillRect(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb)
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, CH_X(x), CH_Y(y), CH_X(x + w), CH_Y(y + h),
		                          Get16BPPColor(rgb));
	}

	void PrintAt(SGPFont font, UINT8 colour, INT32 x, INT32 y, const ST::string& text)
	{
		SetFontAttributes(font, colour, FONT_MCOLOR_BLACK, 0);
		MPrint(CH_X(x), CH_Y(y), text);
	}

	void PrintCentred(SGPFont font, UINT8 colour, INT32 cx, INT32 y, const ST::string& text)
	{
		PrintAt(font, colour, cx - StringPixLength(text, font) / 2, y, text);
	}
}

// --- puzzle setup ---------------------------------------------------------

namespace
{
	// Play the whole recorded line out, used to show the answer once the day is
	// over one way or the other.
	void ChessPlayOutSolution()
	{
		while (guiChessPly < gChessSolution.size())
		{
			const ChessGame::Move m = gChessGame.ParseUci(gChessSolution[guiChessPly]);
			if (m.IsNull()) break;  // the corpus test guarantees this cannot happen
			gubChessLastFrom = m.from;
			gubChessLastTo   = m.to;
			gChessGame.MakeMove(m);
			++guiChessPly;
		}
	}

	void ChessLoadPuzzleForToday()
	{
		const UINT16 today = ChessToday();
		giChessPuzzle = NUM_CHESS_PUZZLES > 0
			? int((today == 0 ? 0 : today - 1) % UINT16(NUM_CHESS_PUZZLES)) : 0;

		const ChessPuzzle& puzzle = CHESS_PUZZLES[giChessPuzzle];
		gChessGame.SetFen(puzzle.fen);

		const std::vector<std::string> all = SplitMoves(puzzle.moves);
		gChessSolution.clear();
		if (all.size() >= 2)
		{
			// Lichess stores the position before an opponent setup move: play it
			// so the board opens on what just happened, and the solver is then
			// whichever side did not make it.
			gChessSolver = ChessGame::Color(gChessGame.SideToMove() ^ 1);
			const ChessGame::Move setup = gChessGame.ParseUci(all[0]);
			if (!setup.IsNull())
			{
				gubChessLastFrom = setup.from;
				gubChessLastTo   = setup.to;
				gChessGame.MakeMove(setup);
			}
			gChessSolution.assign(all.begin() + 1, all.end());
		}

		guiChessPly      = 0;
		gbChessSelected  = -1;
		guiChessReplyDue = 0;
		gfChessHintShown = false;
	}

	// Roll the daily state over when the campaign has moved on to a new day.
	void ChessRollOverDay()
	{
		const UINT16 today = ChessToday();
		if (gusChessDay == today) return;
		gusChessDay     = today;
		gubChessHearts  = CH_MAX_HEARTS;
		gubChessFlags  &= CH_FLAG_DISCOVERED;
	}

	void ChessBeginSession()
	{
		ChessRollOverDay();
		ChessLoadPuzzleForToday();

		if (gubChessFlags & CH_FLAG_SOLVED)
		{
			ChessPlayOutSolution();
			gChessState = CHUI_SOLVED;
			gChessStatus = "solved. come back tomorrow.";
		}
		else if (gubChessFlags & CH_FLAG_FAILED)
		{
			ChessPlayOutSolution();
			gChessState = CHUI_FAILED;
			gChessStatus = "out of tries. ze solution is on ze board.";
		}
		else
		{
			gChessState = CHUI_PUZZLE;
			gChessStatus = gChessSolver == ChessGame::White ? "white to move." : "black to move.";
		}
	}

	void ChessRedraw()
	{
		fPausedReDrawScreenFlag = TRUE;
	}

	void ChessRecordSolved()
	{
		const UINT16 today = ChessToday();
		// a day missed breaks the run; solving on consecutive days extends it
		gubChessStreak = (gusChessLastSolved != 0 && gusChessLastSolved + 1 == today)
			? UINT8(std::min<int>(gubChessStreak + 1, 255)) : 1;
		gubChessBestStreak = std::max(gubChessBestStreak, gubChessStreak);
		gusChessLastSolved = today;
		gubChessFlags |= CH_FLAG_SOLVED;
		gChessState  = CHUI_SOLVED;
		gChessStatus = "correct. ze position is resolved.";
	}

	void ChessRecordFailed()
	{
		gubChessFlags |= CH_FLAG_FAILED;
		gubChessStreak = 0;
		ChessPlayOutSolution();
		gChessState  = CHUI_FAILED;
		gChessStatus = "no tries left. ze solution is on ze board.";
	}

	void ChessSpendHeart()
	{
		if (gubChessHearts > 0) --gubChessHearts;
		if (gubChessHearts == 0) ChessRecordFailed();
	}

	// The player picked a square to move to. Only the recorded solution counts;
	// an illegal move is simply refused, the way a real board refuses it.
	void ChessTryMove(UINT8 from, UINT8 to)
	{
		if (guiChessPly >= gChessSolution.size()) return;

		const std::string& expected = gChessSolution[guiChessPly];
		const ChessGame::Move want = gChessGame.ParseUci(expected);

		if (!want.IsNull() && want.from == from && want.to == to)
		{
			gubChessLastFrom = want.from;
			gubChessLastTo   = want.to;
			gChessGame.MakeMove(want);
			++guiChessPly;
			gbChessSelected = -1;

			if (guiChessPly >= gChessSolution.size())
			{
				ChessRecordSolved();
			}
			else
			{
				gChessStatus     = "correct. he answers...";
				guiChessReplyDue = ChessNow() + CH_REPLY_DELAY;
			}
			return;
		}

		// legal but not the answer costs a heart; illegal costs nothing
		std::string uci;
		uci += char('a' + ChessGame::FileOf(from));
		uci += char('1' + ChessGame::RankOf(from));
		uci += char('a' + ChessGame::FileOf(to));
		uci += char('1' + ChessGame::RankOf(to));
		if (!gChessGame.ParseUci(uci).IsNull() || !gChessGame.ParseUci(uci + "q").IsNull())
		{
			ChessSpendHeart();
			if (gChessState == CHUI_PUZZLE) gChessStatus = "no. zat is not ze move.";
		}
		gbChessSelected = -1;
	}

	void ChessSquareCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gChessState != CHUI_PUZZLE || guiChessReplyDue != 0) return;

		const UINT8 sq = UINT8(MSYS_GetRegionUserData(region, 0));

		const bool ours = !gChessGame.IsEmpty(sq) && gChessGame.ColorAt(sq) == gChessSolver;
		if (gbChessSelected < 0)
		{
			if (ours) gbChessSelected = INT8(sq);
		}
		else if (UINT8(gbChessSelected) == sq)
		{
			gbChessSelected = -1;
		}
		else if (ours)
		{
			gbChessSelected = INT8(sq);
		}
		else
		{
			ChessTryMove(UINT8(gbChessSelected), sq);
		}
		ChessRedraw();
	}

	void ChessHintCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gChessState != CHUI_PUZZLE || guiChessReplyDue != 0) return;
		if (gubChessFlags & CH_FLAG_HINT_USED) return;
		if (guiChessPly >= gChessSolution.size()) return;

		gubChessFlags |= CH_FLAG_HINT_USED;
		gfChessHintShown = true;
		gChessStatus = "zis piece. ze rest is yours.";
		ChessSpendHeart();
		ChessRedraw();
	}

	void ChessPlaceRegions()
	{
		for (int row = 0; row < 8; ++row)
		{
			for (int col = 0; col < 8; ++col)
			{
				MOUSE_REGION& r = gChessSquare[row * 8 + col];
				const UINT16 x = UINT16(CH_X(CH_BOARD_X + col * CH_SQ));
				const UINT16 y = UINT16(CH_Y(CH_BOARD_Y + row * CH_SQ));
				MSYS_DefineRegion(&r, x, y, UINT16(x + CH_SQ), UINT16(y + CH_SQ),
				                  MSYS_PRIORITY_NORMAL, CURSOR_WWW, MSYS_NO_CALLBACK,
				                  ChessSquareCallback);
				MSYS_SetRegionUserData(&r, 0, ScreenToSquare(col, row));
			}
		}

		MSYS_DefineRegion(&gChessHintRegion,
		                  UINT16(CH_X(CH_PANEL_X + 10)), UINT16(CH_Y(CH_PAGE_H - 46)),
		                  UINT16(CH_X(CH_PANEL_X + CH_PANEL_W - 10)), UINT16(CH_Y(CH_PAGE_H - 24)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, ChessHintCallback);
		gChessHintRegion.SetFastHelpText("Costs one attempt");
		gfChessRegionsUp = true;
	}

	void ChessRemoveRegions()
	{
		if (!gfChessRegionsUp) return;
		for (MOUSE_REGION& r : gChessSquare) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessHintRegion);
		gfChessRegionsUp = false;
	}
}

// --- rendering ------------------------------------------------------------

namespace
{
	void ChessRenderNav()
	{
		FillRect(0, 0, CH_NAV_W, CH_PAGE_H, CH_RGB_PANEL);

		// masthead: the z is the odd letter, so it gets the accent
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, 6, 8, "chach");
		PrintAt(FONT10ARIAL, FONT_MCOLOR_LTGREEN, 6 + StringPixLength("chach", FONT10ARIALBOLD), 8, ".com");

		static const char* const items[] = { "Play", "Puzzles", "Learn", "Watch", "Community" };
		for (int i = 0; i < 5; ++i)
		{
			const bool active = i == 1;  // only Puzzles exists so far
			if (active) FillRect(0, 34 + i * 22 - 3, CH_NAV_W, 20, CH_RGB_PANEL_UP);
			PrintAt(FONT10ARIAL, active ? FONT_MCOLOR_LTGREEN : FONT_GRAY2,
			        8, 34 + i * 22, items[i]);
		}

		PrintAt(FONT10ARIAL, FONT_GRAY4, 8, CH_PAGE_H - 30, "Suche");
	}

	void ChessRenderBoard()
	{
		for (int row = 0; row < 8; ++row)
		{
			for (int col = 0; col < 8; ++col)
			{
				const UINT8 sq = ScreenToSquare(col, row);
				const bool light = IsLightSquare(sq);
				const bool lit = sq == gubChessLastFrom || sq == gubChessLastTo ||
				                 (gbChessSelected >= 0 && sq == UINT8(gbChessSelected));
				const UINT32 rgb = lit ? (light ? CH_RGB_HL_LIGHT : CH_RGB_HL_DARK)
				                       : (light ? CH_RGB_LIGHT : CH_RGB_DARK);
				FillRect(CH_BOARD_X + col * CH_SQ, CH_BOARD_Y + row * CH_SQ, CH_SQ, CH_SQ, rgb);
			}
		}

		// the hint marks the piece that has to move, nothing more
		if (gfChessHintShown && guiChessPly < gChessSolution.size())
		{
			const ChessGame::Move want = gChessGame.ParseUci(gChessSolution[guiChessPly]);
			if (!want.IsNull())
			{
				INT32 x, y;
				SquareToScreen(want.from, x, y);
				FillRect(x, y, CH_SQ, CH_SQ,
				         IsLightSquare(want.from) ? CH_RGB_HL_LIGHT : CH_RGB_HL_DARK);
			}
		}

		// legal destinations for the piece in hand, chess.com's dot and ring
		if (gbChessSelected >= 0)
		{
			ChessGame::Move moves[ChessGame::MAX_MOVES];
			const int count = gChessGame.GenerateLegal(moves);
			for (int i = 0; i < count; ++i)
			{
				if (moves[i].from != UINT8(gbChessSelected)) continue;
				INT32 x, y;
				SquareToScreen(moves[i].to, x, y);
				const UINT32 dot = IsLightSquare(moves[i].to) ? CH_RGB_DOT_LIGHT : CH_RGB_DOT_DARK;
				if (gChessGame.IsEmpty(moves[i].to))
				{
					FillRect(x + CH_SQ / 2 - 5, y + CH_SQ / 2 - 5, 10, 10, dot);
				}
				else
				{
					// a ring around an occupied square, drawn as four edges
					FillRect(x, y, CH_SQ, 3, dot);
					FillRect(x, y + CH_SQ - 3, CH_SQ, 3, dot);
					FillRect(x, y, 3, CH_SQ, dot);
					FillRect(x + CH_SQ - 3, y, 3, CH_SQ, dot);
				}
			}
		}

		// coordinates sit inside the corner squares, in the opposite colour
		for (int i = 0; i < 8; ++i)
		{
			const UINT8 rankSq = ScreenToSquare(0, i);
			const UINT8 fileSq = ScreenToSquare(i, 7);
			PrintAt(FONT10ARIAL, IsLightSquare(rankSq) ? FONT_GRAY7 : FONT_WHITE,
			        CH_BOARD_X + 2, CH_BOARD_Y + i * CH_SQ + 1,
			        ST::format("{}", char('1' + ChessGame::RankOf(rankSq))));
			PrintAt(FONT10ARIAL, IsLightSquare(fileSq) ? FONT_GRAY7 : FONT_WHITE,
			        CH_BOARD_X + i * CH_SQ + CH_SQ - 8, CH_BOARD_BOTTOM - 13,
			        ST::format("{}", char('a' + ChessGame::FileOf(fileSq))));
		}

		if (!guiChessPieces) return;
		for (int row = 0; row < 8; ++row)
		{
			for (int col = 0; col < 8; ++col)
			{
				const UINT8 sq = ScreenToSquare(col, row);
				if (gChessGame.IsEmpty(sq)) continue;
				const UINT8 type = gChessGame.PieceAt(sq);
				const UINT16 frame = UINT16((type - 1) +
					(gChessGame.ColorAt(sq) == ChessGame::Black ? 6 : 0));
				BltVideoObject(FRAME_BUFFER, guiChessPieces, frame,
				               CH_X(CH_BOARD_X + col * CH_SQ), CH_Y(CH_BOARD_Y + row * CH_SQ));
			}
		}
	}

	void ChessRenderHearts()
	{
		for (int i = 0; i < CH_MAX_HEARTS; ++i)
		{
			const bool spent = i >= gubChessHearts;
			const INT32 x = CH_BOARD_X + i * (CH_HEART_W + 5);
			// a blocky heart: two shoulders over a body
			const UINT32 rgb = spent ? CH_RGB_HEART_SPENT : CH_RGB_HEART;
			FillRect(x, CH_HEART_Y, 5, 4, rgb);
			FillRect(x + 8, CH_HEART_Y, 5, 4, rgb);
			FillRect(x, CH_HEART_Y + 3, 13, 4, rgb);
			FillRect(x + 2, CH_HEART_Y + 7, 9, 2, rgb);
			FillRect(x + 4, CH_HEART_Y + 9, 5, 2, rgb);
		}
		PrintAt(FONT10ARIAL, FONT_GRAY4, CH_BOARD_X + 5 * (CH_HEART_W + 5) + 8, CH_HEART_Y,
		        ST::format("{} VERSUCHE", gubChessHearts));
	}

	void ChessRenderPanel()
	{
		FillRect(CH_PANEL_X, 0, CH_PANEL_W, CH_PAGE_H, CH_RGB_PANEL);
		const INT32 cx = CH_PANEL_X + CH_PANEL_W / 2;
		const ChessPuzzle& puzzle = CHESS_PUZZLES[giChessPuzzle];

		PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, 8, ST::format("TAG {}", ChessToday()));
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, 24, "TAGESRATSEL");
		PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, 38, "daily puzzle");
		FillRect(CH_PANEL_X + 10, 56, CH_PANEL_W - 20, 1, CH_RGB_RULE);

		// who moves, with the proprietor standing in as the coach
		if (guiChessCoach)
		{
			BltVideoObject(FRAME_BUFFER, guiChessCoach, 0, CH_X(CH_PANEL_X + 10), CH_Y(66));
		}
		FillRect(CH_PANEL_X + 46, 66, CH_PANEL_W - 56, 33, CH_RGB_PANEL_UP);
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, CH_PANEL_X + 52, 76,
		        gChessSolver == ChessGame::White ? "WEISS ZIEHT" : "SCHWARZ ZIEHT");

		PrintAt(FONT10ARIAL, FONT_GRAY4, CH_PANEL_X + 10, 108,
		        ST::format("WERTUNG {}", puzzle.rating));

		// the running commentary, wrapped by hand at this width
		PrintAt(FONT10ARIAL, gChessState == CHUI_SOLVED ? FONT_MCOLOR_LTGREEN
		        : gChessState == CHUI_FAILED ? FONT_MCOLOR_LTRED : FONT_GRAY2,
		        CH_PANEL_X + 10, 126, gChessStatus);

		PrintAt(FONT10ARIAL, FONT_GRAY4, CH_PANEL_X + 10, 156, "GELOST VON:");
		const int shown = int(sizeof(CHESS_SOLVERS) / sizeof(CHESS_SOLVERS[0]));
		for (int i = 0; i < shown; ++i)
		{
			PrintAt(FONT10ARIAL, FONT_GRAY7, CH_PANEL_X + 14, 172 + i * 13, CHESS_SOLVERS[i]);
		}
		if (gubChessFlags & CH_FLAG_SOLVED)
		{
			PrintAt(FONT10ARIAL, FONT_MCOLOR_LTGREEN, CH_PANEL_X + 14, 172 + shown * 13, "und Sie.");
		}

		FillRect(CH_PANEL_X + 10, CH_PAGE_H - 96, CH_PANEL_W - 20, 1, CH_RGB_RULE);
		PrintAt(FONT10ARIAL, FONT_GRAY2, CH_PANEL_X + 10, CH_PAGE_H - 88,
		        ST::format("SERIE: {} TAGE", gubChessStreak));
		PrintAt(FONT10ARIAL, FONT_GRAY4, CH_PANEL_X + 10, CH_PAGE_H - 74,
		        ST::format("BESTE: {}", gubChessBestStreak));

		// the hint button greys out once it has been spent
		const bool hintLive = gChessState == CHUI_PUZZLE && !(gubChessFlags & CH_FLAG_HINT_USED);
		FillRect(CH_PANEL_X + 10, CH_PAGE_H - 46, CH_PANEL_W - 20, 22,
		         hintLive ? CH_RGB_PANEL_UP : CH_RGB_PANEL);
		PrintCentred(FONT10ARIAL, hintLive ? FONT_MCOLOR_WHITE : FONT_GRAY7,
		             cx, CH_PAGE_H - 40, "TIPP");
	}

	void ChessRenderFooter()
	{
		PrintAt(FONT10ARIAL, FONT_GRAY7, CH_BOARD_X, CH_PAGE_H - 16,
		        "Beste Ansicht 800x600 - Losung morgen");
	}
}

// --- laptop page hooks ----------------------------------------------------

void EnterChess()
{
	gubChessFlags |= CH_FLAG_DISCOVERED;

	guiChessPieces = nullptr;
	guiChessCoach  = nullptr;
	try
	{
		guiChessPieces = AddVideoObjectFromFile("sti/laptop/chesspieces.sti");
	}
	catch (...)
	{
		// sheet missing: the board still plays, just without pieces drawn
	}
	try
	{
		guiChessCoach = AddVideoObjectFromFile(
			ST::format(FACESDIR "/33face/b{02d}.sti", GetProfile(GRUNTY).ubFaceIndex));
	}
	catch (...)
	{
	}

	ChessBeginSession();
	ChessPlaceRegions();
}

void ExitChess()
{
	ChessRemoveRegions();
	if (guiChessPieces) { DeleteVideoObject(guiChessPieces); guiChessPieces = nullptr; }
	if (guiChessCoach)  { DeleteVideoObject(guiChessCoach);  guiChessCoach  = nullptr; }
}

void RenderChess()
{
	FillRect(0, 0, LAPTOP_SCREEN_WIDTH, CH_PAGE_H, CH_RGB_CHROME);
	ChessRenderNav();
	ChessRenderBoard();
	ChessRenderHearts();
	ChessRenderPanel();
	ChessRenderFooter();

	MarkButtonsDirty();
	RenderWWWProgramTitleBar();
	InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
	                 LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
}

void HandleChess()
{
	if (guiChessReplyDue == 0 || ChessNow() < guiChessReplyDue) return;
	guiChessReplyDue = 0;

	if (guiChessPly < gChessSolution.size())
	{
		const ChessGame::Move reply = gChessGame.ParseUci(gChessSolution[guiChessPly]);
		if (!reply.IsNull())
		{
			gubChessLastFrom = reply.from;
			gubChessLastTo   = reply.to;
			gChessGame.MakeMove(reply);
			++guiChessPly;
		}
	}
	gChessStatus = guiChessPly >= gChessSolution.size() ? "solved." : "your move again.";
	if (guiChessPly >= gChessSolution.size() && gChessState == CHUI_PUZZLE) ChessRecordSolved();
	ChessRedraw();
}
