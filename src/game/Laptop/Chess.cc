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
#include "IMP_Compile_Character.h"
#include "Input.h"
#include "Laptop.h"
#include "LaptopSave.h"
#include "MercPortrait.h"
#include "MouseSystem.h"
#include "Sound_Control.h"
#include "Soldier_Profile.h"
#include "Timer_Control.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"
#include "WordWrap.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <string_theory/format>
#include <string_theory/string>

#define CH_X(x) ((INT32)(LAPTOP_SCREEN_UL_X + (x)))
#define CH_Y(y) ((INT32)(LAPTOP_SCREEN_WEB_UL_Y + (y)))

// --- layout ---------------------------------------------------------------
// The rail and panel are inset from the page edges and rounded, so the dark
// chrome shows as a margin around them.
#define CH_PAGE_H       400
#define CH_INSET        6
#define CH_RADIUS       4
// The rail is the one exception: it runs the full height of the page and sits
// flush to the left edge, so no chrome shows around it.
#define CH_NAV_X        0
#define CH_NAV_W        70
#define CH_SQ           34
#define CH_BOARD_X      74
#define CH_BOARD_Y      24
#define CH_BOARD_SIZE   (8 * CH_SQ)
#define CH_BOARD_BOTTOM (CH_BOARD_Y + CH_BOARD_SIZE)
#define CH_PANEL_X      352
#define CH_PANEL_W      144
// hearts live in the rail beside the avatar, so they are the small variant
#define CH_HEART_PITCH  9

// the result card, centred on the board
#define CH_MODAL_W      204
#define CH_MODAL_H      116
#define CH_MODAL_X      (CH_BOARD_X + (CH_BOARD_SIZE - CH_MODAL_W) / 2)
#define CH_MODAL_Y      (CH_BOARD_Y + (CH_BOARD_SIZE - CH_MODAL_H) / 2)

#define CH_DATE_Y       34
#define CH_COACH_Y      78
#define CH_COACH_TILE   36

// The day stepper's arrows sit at fixed points on the panel edges while the
// chip between them is centred and changes width with the day number. Glyphs
// and hit regions are both derived from these, so they cannot drift apart.
#define CH_PREV_X       (CH_PANEL_X + 8)
#define CH_NEXT_X       (CH_PANEL_X + CH_PANEL_W - 34)
#define CH_ARROW_W      26
#define CH_ARROW_H      16

// rail furniture, measured down from the page foot
#define CH_RAIL_SEARCH_Y  (CH_PAGE_H - 118)
#define CH_RAIL_LANG_Y    (CH_PAGE_H - 102)
#define CH_RAIL_AVATAR_Y  (CH_PAGE_H - 38)
#define CH_AVATAR_SIZE    33

// frame indices into chessicons.sti
#define CH_ICON_PLAY       0
#define CH_ICON_PUZZLES    1
#define CH_ICON_LEARN      2
#define CH_ICON_WATCH      3
#define CH_ICON_COMMUNITY  4
#define CH_ICON_CALENDAR   5
#define CH_ICON_PUZZLEMARK 6
#define CH_ICON_FLAME      7

#define CH_MAX_HEARTS   5
#define CH_REPLY_DELAY  650  // ms before the scripted reply lands

// chess.com's palette, sampled from the live site
#define CH_RGB_LIGHT       FROMRGB(235, 236, 208)
#define CH_RGB_DARK        FROMRGB(115, 149,  82)
#define CH_RGB_HL_LIGHT    FROMRGB(245, 246, 130)
#define CH_RGB_HL_DARK     FROMRGB(185, 202,  67)
#define CH_RGB_DOT_LIGHT   FROMRGB(200, 201, 178)
#define CH_RGB_DOT_DARK    FROMRGB( 98, 127,  70)
// Warm greys: the neutral values read green next to the board, so red leads
// and blue trails in every one of these.
#define CH_RGB_CHROME      FROMRGB( 46,  42,  38)
#define CH_RGB_PANEL       FROMRGB( 36,  33,  30)
#define CH_RGB_PANEL_UP    FROMRGB( 60,  55,  50)
// Sections are separated by a shift in ground tone rather than by rules, and
// the shift goes down: a sunk band, never a brighter one.
#define CH_RGB_PANEL_SUNK  FROMRGB( 29,  26,  24)
#define CH_RGB_CTA         FROMRGB(129, 182,  76)
#define CH_RGB_HEART_SPENT FROMRGB( 70,  68,  65)
// the account row: the nickname is suggested rather than set, two grey bars
// standing in for text the way a low-res mock would
#define CH_RGB_NICK        FROMRGB(154, 152, 147)
#define CH_RGB_NICK_DIM    FROMRGB( 92,  90,  87)
// the coach speaks from a white bubble, as on the live site
#define CH_RGB_BUBBLE      FROMRGB(255, 255, 255)

namespace
{
	enum ChessUiState
	{
		CHUI_PUZZLE,  // the player is solving
		CHUI_SOLVED,
		CHUI_FAILED,  // out of hearts; the solution is on the board
		CHUI_REVIEW,  // an earlier day pulled from the archive, not playable
	};

	// Session-lifetime, rebuilt on every visit from the persisted daily state.
	ChessUiState             gChessState = CHUI_PUZZLE;
	ChessGame                gChessGame;
	int                      giChessPuzzle = 0;
	std::vector<std::string> gChessSolution;  // the line after the setup move
	std::size_t              guiChessPly = 0; // index of the next expected move
	ChessGame::Color         gChessSolver = ChessGame::White;

	INT8   gbChessSelected  = -1;    // 0x88 square, -1 when nothing is picked up
	// Drag state. The piece is lifted on press and dropped on release; a press
	// and release on the same square falls through to the click-to-move path,
	// so both idioms work the way they do on the live site.
	bool   gfChessDragging  = false;
	UINT8  gubChessDragFrom = ChessGame::NO_SQUARE;
	UINT8  gubChessHearts   = CH_MAX_HEARTS;
	UINT32 guiChessReplyDue = 0;     // when the scripted reply lands, 0 if none
	UINT8  gubChessLastFrom = ChessGame::NO_SQUARE;
	UINT8  gubChessLastTo   = ChessGame::NO_SQUARE;
	bool   gfChessHintShown = false;
	// the result card, raised on the move that ends the day and dismissed by
	// hand; revisiting a finished day does not raise it again
	bool   gfChessModal     = false;
	// what the coach is currently saying, kept as an id so the language switch
	// re-renders it rather than freezing whatever was said last
	int    giChessSaid      = 0;   // ChessStr, or -1/-2 for a good/bad variant
	int    giChessVariant   = 0;

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
	SGPVObject* guiChessIcons  = nullptr;  // 7 nav and panel icons, 14x14
	SGPVObject* guiChessLogo   = nullptr;  // green pawn, 22 and 14
	SGPVObject* guiChessSelf   = nullptr;  // the player's I.M.P. portrait
	ST::string  gChessSelfNick;

	// which campaign day is on screen; past days are archive, view only
	int giChessViewDay = 1;

	MOUSE_REGION gChessSquare[64];
	MOUSE_REGION gChessHintRegion;
	MOUSE_REGION gChessPrevDayRegion;
	MOUSE_REGION gChessNextDayRegion;
	MOUSE_REGION gChessLangRegion;
	MOUSE_REGION gChessModalCloseRegion;
	MOUSE_REGION gChessModalArchiveRegion;
	// sits behind everything and catches a piece released off the board, which
	// would otherwise leave it stuck to the cursor
	MOUSE_REGION gChessDropRegion;
	bool         gfChessRegionsUp = false;

	// The site is English by default and the proprietor is not, so the rail
	// carries a switch. Umlauts are spelled out: the laptop fonts are ASCII.
	enum ChessStr
	{
		CHS_TITLE, CHS_DAY, CHS_RATING, CHS_WHITE_MOVES, CHS_BLACK_MOVES,
		CHS_SOLVED_BY, CHS_AND_YOU, CHS_STREAK, CHS_BEST, CHS_HINT, CHS_TRIES,
		CHS_FOOTER, CHS_SEARCH,
		CHS_NAV_PLAY, CHS_NAV_PUZZLES, CHS_NAV_LEARN, CHS_NAV_WATCH, CHS_NAV_COMMUNITY,
		CHS_ST_WHITE, CHS_ST_BLACK, CHS_ST_CORRECT, CHS_ST_WRONG, CHS_ST_HINT,
		CHS_ST_ALREADY, CHS_ST_OUT, CHS_ST_DONE, CHS_ST_YOUR_MOVE, CHS_ST_ARCHIVE,
		CHS_MODAL_PERFECT, CHS_MODAL_SOLVED, CHS_MODAL_FAILED, CHS_MODAL_ARCHIVE,
		CHS_MODAL_STREAK, CHS_MODAL_BEST,
		CHS_COUNT
	};

	const char* const CHESS_TEXT[2][CHS_COUNT] =
	{
		{
			"DAILY PUZZLE", "DAY", "RATING", "WHITE TO MOVE", "BLACK TO MOVE",
			"SOLVED BY:", "and you.", "STREAK: {} DAYS", "BEST: {}", "HINT", "TRIES",
			"best viewed at 800x600 - solution tomorrow", "Search",
			"Play", "Puzzles", "Learn", "Watch", "Groups",
			"white to move.", "black to move.", "correct. he answers...",
			"no. that is not the move.", "this piece. the rest is yours.",
			"solved. come back tomorrow.", "out of tries. the solution is on the board.",
			"correct. the position is resolved.", "your move again.",
			"from the archive. this one is finished.",
			"PERFECT!", "SOLVED", "OUT OF TRIES", "OLDER PUZZLES",
			"STREAK", "BEST",
		},
		{
			"TAGESRAETSEL", "TAG", "WERTUNG", "WEISS ZIEHT", "SCHWARZ ZIEHT",
			"GELOEST VON:", "und Sie.", "SERIE: {} TAGE", "BESTE: {}", "TIPP", "VERSUCHE",
			"Beste Ansicht 800x600 - Loesung morgen", "Suche",
			"Spielen", "Raetsel", "Lernen", "Zusehen", "Forum",
			"Weiss ist am Zug.", "Schwarz ist am Zug.", "richtig. er antwortet...",
			"nein. das ist nicht der Zug.", "diese Figur. der Rest ist Ihrer.",
			"geloest. kommen Sie morgen wieder.", "keine Versuche mehr. die Loesung steht.",
			"richtig. die Stellung ist geklaert.", "Sie sind wieder am Zug.",
			"aus dem Archiv. dieses ist erledigt.",
			"PERFEKT!", "GELOEST", "KEINE VERSUCHE", "AELTERE RAETSEL",
			"SERIE", "BESTE",
		},
	};

	bool gfChessGerman = false;

	const char* T(ChessStr id) { return CHESS_TEXT[gfChessGerman ? 1 : 0][id]; }

	// The coach reacts to each move. He is a mercenary, not a chess teacher,
	// so the encouragement is brisk and the corrections are worse.
	constexpr int CH_COACH_LINES = 4;
	const char* const CHESS_COACH_GOOD[2][CH_COACH_LINES] =
	{
		{ "Ja! That is it.", "Good. Keep going.", "Correct. Do not stop.",
		  "Yes. You see it now." },
		{ "Ja! Das ist es.", "Gut. Weiter so.", "Richtig. Nicht aufhoeren.",
		  "Ja. Jetzt sehen Sie es." },
	};
	const char* const CHESS_COACH_BAD[2][CH_COACH_LINES] =
	{
		{ "Nein. Look again.", "That one loses. Think.", "No. Not this piece.",
		  "You are rushing. Stop." },
		{ "Nein. Schauen Sie nochmal.", "Der verliert. Denken Sie.",
		  "Nein. Nicht diese Figur.", "Sie hetzen. Aufhoeren." },
	};

	// -1 asks for an encouraging line, -2 a corrective one; anything else is a
	// fixed string id. Storing the id rather than the text means the language
	// switch re-renders whatever he last said.
	void ChessCoachSay(int what)
	{
		if (what < 0) giChessVariant = (giChessVariant + 1) % CH_COACH_LINES;
		giChessSaid = what;
	}

	const char* ChessCoachLine()
	{
		const int lang = gfChessGerman ? 1 : 0;
		if (giChessSaid == -1) return CHESS_COACH_GOOD[lang][giChessVariant];
		if (giChessSaid == -2) return CHESS_COACH_BAD[lang][giChessVariant];
		return CHESS_TEXT[lang][giChessSaid];
	}

	// chess.com's own cue set, externalized alongside the art
	#define CH_SND_MOVE     SOUNDSDIR "/laptop/chach-move-self.mp3"
	#define CH_SND_OPPONENT SOUNDSDIR "/laptop/chach-move-opponent.mp3"
	#define CH_SND_CAPTURE  SOUNDSDIR "/laptop/chach-capture.mp3"
	#define CH_SND_CHECK    SOUNDSDIR "/laptop/chach-move-check.mp3"
	#define CH_SND_CASTLE   SOUNDSDIR "/laptop/chach-castle.mp3"
	#define CH_SND_PROMOTE  SOUNDSDIR "/laptop/chach-promote.mp3"
	#define CH_SND_WRONG    SOUNDSDIR "/laptop/chach-incorrect.mp3"

	void ChessPlay(const char* file, UINT32 volume = MIDVOLUME)
	{
		try
		{
			PlayJA2SampleFromFile(file, volume, 1, MIDDLEPAN);
		}
		catch (...)
		{
			// missing sample: stay silent
		}
	}

	// Which cue a move earns, in chess.com's order of precedence: the special
	// cases speak first, and check outranks the capture that delivered it.
	const char* ChessMoveSound(const ChessGame::Move& m, bool givesCheck, bool byUs)
	{
		if (givesCheck)                  return CH_SND_CHECK;
		if (m.flags & ChessGame::MF_CASTLE) return CH_SND_CASTLE;
		if (m.promo != ChessGame::NoPiece)  return CH_SND_PROMOTE;
		if (m.flags & ChessGame::MF_CAPTURE) return CH_SND_CAPTURE;
		return byUs ? CH_SND_MOVE : CH_SND_OPPONENT;
	}

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

	// How far a rounded corner is inset on a given row of the arc.
	int CornerInset(int row, int radius)
	{
		const double dy = radius - row - 0.5;
		return int(radius - std::sqrt(double(radius) * radius - dy * dy) + 0.5);
	}

	// Paint the corner steps of an already-drawn rect in the surrounding colour,
	// which is how both the panels and the board get their rounding.
	void RoundCorners(INT32 x, INT32 y, INT32 w, INT32 h, int radius, UINT32 bg)
	{
		for (int row = 0; row < radius; ++row)
		{
			const int inset = CornerInset(row, radius);
			if (inset <= 0) continue;
			FillRect(x, y + row, inset, 1, bg);
			FillRect(x + w - inset, y + row, inset, 1, bg);
			FillRect(x, y + h - 1 - row, inset, 1, bg);
			FillRect(x + w - inset, y + h - 1 - row, inset, 1, bg);
		}
	}

	void FillRounded(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb, int radius, UINT32 bg)
	{
		FillRect(x, y, w, h, rgb);
		RoundCorners(x, y, w, h, radius, bg);
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

	void ChessLoadPuzzleForDay(int day)
	{
		giChessPuzzle = NUM_CHESS_PUZZLES > 0
			? int((day <= 0 ? 0 : day - 1) % NUM_CHESS_PUZZLES) : 0;

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

	// Show one day. Today is playable; anything earlier is archive, opened with
	// its answer already on the board.
	void ChessShowDay(int day)
	{
		giChessViewDay = std::max(1, std::min<int>(day, ChessToday()));
		ChessLoadPuzzleForDay(giChessViewDay);

		if (giChessViewDay != int(ChessToday()))
		{
			ChessPlayOutSolution();
			gChessState = CHUI_REVIEW;
			giChessSaid = CHS_ST_ARCHIVE;
		}
		else if (gubChessFlags & CH_FLAG_SOLVED)
		{
			ChessPlayOutSolution();
			gChessState = CHUI_SOLVED;
			giChessSaid = CHS_ST_ALREADY;
		}
		else if (gubChessFlags & CH_FLAG_FAILED)
		{
			ChessPlayOutSolution();
			gChessState = CHUI_FAILED;
			giChessSaid = CHS_ST_OUT;
		}
		else
		{
			gChessState = CHUI_PUZZLE;
			giChessSaid = gChessSolver == ChessGame::White ? CHS_ST_WHITE : CHS_ST_BLACK;
		}
	}

	void ChessBeginSession()
	{
		ChessRollOverDay();
		ChessShowDay(ChessToday());
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
		gfChessModal = true;
		gChessState  = CHUI_SOLVED;
		giChessSaid = CHS_ST_DONE;
	}

	void ChessRecordFailed()
	{
		gubChessFlags |= CH_FLAG_FAILED;
		gfChessModal   = true;
		gubChessStreak = 0;
		ChessPlayOutSolution();
		gChessState  = CHUI_FAILED;
		giChessSaid = CHS_ST_OUT;
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
			ChessPlay(ChessMoveSound(want, gChessGame.IsInCheck(gChessGame.SideToMove()), true));
			++guiChessPly;
			gbChessSelected = -1;

			if (guiChessPly >= gChessSolution.size())
			{
				ChessRecordSolved();
			}
			else
			{
				ChessCoachSay(-1);
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
			ChessPlay(CH_SND_WRONG);
			ChessSpendHeart();
			if (gChessState == CHUI_PUZZLE) ChessCoachSay(-2);
		}
		// an illegal drop is silent, as it is on the live site: the piece just
		// goes back
		gbChessSelected = -1;
	}

	void ChessSquareCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_DWN)) return;
		if (gfChessModal) return;
		if (gChessState != CHUI_PUZZLE || guiChessReplyDue != 0) return;

		const UINT8 sq = UINT8(MSYS_GetRegionUserData(region, 0));
		const bool ours = !gChessGame.IsEmpty(sq) && gChessGame.ColorAt(sq) == gChessSolver;

		if (ours)
		{
			// lift it: a click is just a drag that never moved. Silent - only
			// a move that lands on the board makes a sound.
			gbChessSelected  = INT8(sq);
			gfChessDragging  = true;
			gubChessDragFrom = sq;
		}
		else if (gbChessSelected >= 0)
		{
			// click-to-move: second click lands the piece already selected
			ChessTryMove(UINT8(gbChessSelected), sq);
		}
		ChessRedraw();
	}

	// Where the pointer is over the board, or NO_SQUARE if it is off it.
	UINT8 ChessSquareUnderPointer()
	{
		const INT32 col = (INT32(gusMouseXPos) - CH_X(CH_BOARD_X)) / CH_SQ;
		const INT32 row = (INT32(gusMouseYPos) - CH_Y(CH_BOARD_Y)) / CH_SQ;
		if (INT32(gusMouseXPos) < CH_X(CH_BOARD_X) || INT32(gusMouseYPos) < CH_Y(CH_BOARD_Y) ||
		    col < 0 || col > 7 || row < 0 || row > 7)
		{
			return ChessGame::NO_SQUARE;
		}
		return ScreenToSquare(int(col), int(row));
	}

	// MSYS locks the mouse to the region that was pressed and then deliberately
	// withholds the button-up when the pointer has moved elsewhere, so a drop on
	// a different square never reaches a callback. Resolve it from the pointer
	// instead, on the tick the button comes back up.
	void ChessResolveDrop()
	{
		const UINT8 from = gubChessDragFrom;
		gfChessDragging  = false;
		gubChessDragFrom = ChessGame::NO_SQUARE;

		const UINT8 to = ChessSquareUnderPointer();
		// released off the board, or back where it started: it snaps home and
		// stays selected, so the click path can still finish the move
		if (to == ChessGame::NO_SQUARE || to == from)
		{
			ChessRedraw();
			return;
		}
		if (!gChessGame.IsEmpty(to) && gChessGame.ColorAt(to) == gChessSolver)
		{
			gbChessSelected = INT8(to);  // dropped on another of our own men
		}
		else
		{
			ChessTryMove(from, to);
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
		giChessSaid = CHS_ST_HINT;
		ChessSpendHeart();
		ChessRedraw();
	}

	void ChessDayCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const int step = int(MSYS_GetRegionUserData(region, 0));
		const int want = giChessViewDay + step;
		if (want < 1 || want > int(ChessToday())) return;
		ChessShowDay(want);
		ChessRedraw();
	}

	void ChessLangCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		gfChessGerman = !gfChessGerman;
		ChessRedraw();
	}

	// releasing anywhere that is not a square just puts the piece back; it stays
	// selected, so the click-to-move path can still finish the job
	void ChessDropCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfChessDragging) return;
		gfChessDragging  = false;
		gubChessDragFrom = ChessGame::NO_SQUARE;
		ChessRedraw();
	}

	void ChessPlaceRegions()
	{
		MSYS_DefineRegion(&gChessDropRegion,
		                  UINT16(CH_X(0)), UINT16(CH_Y(0)),
		                  UINT16(CH_X(LAPTOP_SCREEN_WIDTH)), UINT16(CH_Y(CH_PAGE_H)),
		                  // above the laptop's own screen region, below the squares
		                  MSYS_PRIORITY_NORMAL + 2, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessDropCallback);

		for (int row = 0; row < 8; ++row)
		{
			for (int col = 0; col < 8; ++col)
			{
				MOUSE_REGION& r = gChessSquare[row * 8 + col];
				const UINT16 x = UINT16(CH_X(CH_BOARD_X + col * CH_SQ));
				const UINT16 y = UINT16(CH_Y(CH_BOARD_Y + row * CH_SQ));
				MSYS_DefineRegion(&r, x, y, UINT16(x + CH_SQ), UINT16(y + CH_SQ),
				                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				                  ChessSquareCallback);
				MSYS_SetRegionUserData(&r, 0, ScreenToSquare(col, row));
			}
		}

		MSYS_DefineRegion(&gChessHintRegion,
		                  UINT16(CH_X(CH_PANEL_X + 10)), UINT16(CH_Y(CH_PAGE_H - 46)),
		                  UINT16(CH_X(CH_PANEL_X + CH_PANEL_W - 10)), UINT16(CH_Y(CH_PAGE_H - 24)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, ChessHintCallback);
		gChessHintRegion.SetFastHelpText("Costs one attempt");

		// date stepper arrows, either side of the day chip
		MSYS_DefineRegion(&gChessPrevDayRegion,
		                  UINT16(CH_X(CH_PREV_X)), UINT16(CH_Y(CH_DATE_Y)),
		                  UINT16(CH_X(CH_PREV_X + CH_ARROW_W)), UINT16(CH_Y(CH_DATE_Y + CH_ARROW_H)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, ChessDayCallback);
		MSYS_SetRegionUserData(&gChessPrevDayRegion, 0, -1);
		MSYS_DefineRegion(&gChessNextDayRegion,
		                  UINT16(CH_X(CH_NEXT_X)), UINT16(CH_Y(CH_DATE_Y)),
		                  UINT16(CH_X(CH_NEXT_X + CH_ARROW_W)), UINT16(CH_Y(CH_DATE_Y + CH_ARROW_H)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, ChessDayCallback);
		MSYS_SetRegionUserData(&gChessNextDayRegion, 0, 1);

		MSYS_DefineRegion(&gChessLangRegion,
		                  UINT16(CH_X(CH_NAV_X + 4)), UINT16(CH_Y(CH_RAIL_LANG_Y - 2)),
		                  UINT16(CH_X(CH_NAV_X + CH_NAV_W - 4)), UINT16(CH_Y(CH_RAIL_LANG_Y + 12)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, ChessLangCallback);
		gChessLangRegion.SetFastHelpText("English / Deutsch");

		gfChessRegionsUp = true;
	}

	void ChessRemoveRegions()
	{
		if (!gfChessRegionsUp) return;
		for (MOUSE_REGION& r : gChessSquare) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessDropRegion);
		MSYS_RemoveRegion(&gChessHintRegion);
		MSYS_RemoveRegion(&gChessPrevDayRegion);
		MSYS_RemoveRegion(&gChessNextDayRegion);
		MSYS_RemoveRegion(&gChessLangRegion);
		gfChessRegionsUp = false;
	}
}

// --- rendering ------------------------------------------------------------

namespace
{
	// A small blocky heart, the tries counter. Drawn rather than blitted so the
	// spent ones can be greyed without a second frame.
	void ChessDrawHeart(INT32 x, INT32 y, UINT32 rgb, INT32 s = 1)
	{
		FillRect(x,         y,         3 * s, 2 * s, rgb);
		FillRect(x + 4 * s, y,         3 * s, 2 * s, rgb);
		FillRect(x,         y + 2 * s, 7 * s, 2 * s, rgb);
		FillRect(x + 1 * s, y + 4 * s, 5 * s, 1 * s, rgb);
		FillRect(x + 2 * s, y + 5 * s, 3 * s, 1 * s, rgb);
	}

	// A blunt chevron: each row is a short bar stepped out from the point, so
	// it reads at 9px where a text glyph reads as a comma.
	void ChessDrawChevron(INT32 cx, INT32 cy, bool left, UINT32 rgb)
	{
		const INT32 half = 4;
		for (INT32 r = -half; r <= half; ++r)
		{
			const INT32 step = (r < 0 ? -r : r);
			const INT32 x = left ? cx - 2 + step : cx + 2 - step - 3;
			FillRect(x, cy + r, 3, 1, rgb);
		}
	}

	void ChessRenderNav()
	{
		// flush left and full height: the rail is page furniture, not a card
		FillRect(CH_NAV_X, 0, CH_NAV_W, CH_PAGE_H, CH_RGB_PANEL);

		// masthead: chess.com's lockup, pawn to the left of the wordmark
		const INT32 markW = StringPixLength("chach", FONT10ARIALBOLD) +
		                    StringPixLength(".com", FONT10ARIAL);
		const INT32 lockW = 14 + 2 + markW;
		const INT32 lockX = std::max(2, CH_NAV_X + (CH_NAV_W - lockW) / 2);
		if (guiChessLogo)
		{
			// frame 1 is the 14px pawn; the 22px one will not sit on one line
			BltVideoObject(FRAME_BUFFER, guiChessLogo, 1, CH_X(lockX + 2), CH_Y(7));
		}
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, lockX + 16, 12, "chach");
		PrintAt(FONT10ARIAL, FONT_GRAY2,
		        lockX + 16 + StringPixLength("chach", FONT10ARIALBOLD), 11, ".com");

		static const ChessStr items[5] = {
			CHS_NAV_PLAY, CHS_NAV_PUZZLES, CHS_NAV_LEARN, CHS_NAV_WATCH, CHS_NAV_COMMUNITY
		};
		for (int i = 0; i < 5; ++i)
		{
			const INT32 rowY = 32 + i * 20;
			const bool active = i == 1;  // only Puzzles exists so far
			if (active)
			{
				FillRounded(CH_NAV_X + 2, rowY - 3, CH_NAV_W - 4, 18,
				            CH_RGB_PANEL_UP, 3, CH_RGB_PANEL);
			}
			if (guiChessIcons)
			{
				BltVideoObject(FRAME_BUFFER, guiChessIcons, UINT16(i),
				               CH_X(CH_NAV_X + 4), CH_Y(rowY));
			}
			PrintAt(FONT10ARIAL, active ? FONT_MCOLOR_WHITE : FONT_GRAY2,
			        CH_NAV_X + 20, rowY + 1, T(items[i]));
		}

		PrintAt(FONT10ARIAL, FONT_GRAY4, CH_NAV_X + 6, CH_RAIL_SEARCH_Y, T(CHS_SEARCH));

		// language switch, because the proprietor is not English
		PrintAt(FONT10ARIAL, gfChessGerman ? FONT_GRAY7 : FONT_MCOLOR_LTGREEN,
		        CH_NAV_X + 6, CH_RAIL_LANG_Y, "EN");
		PrintAt(FONT10ARIAL, FONT_GRAY7, CH_NAV_X + 24, CH_RAIL_LANG_Y, "|");
		PrintAt(FONT10ARIAL, gfChessGerman ? FONT_MCOLOR_LTGREEN : FONT_GRAY7,
		        CH_NAV_X + 32, CH_RAIL_LANG_Y, "DE");

		// The account row: your I.M.P. portrait as the site avatar, with the
		// nickname suggested by two grey bars rather than set in type. The
		// portrait's real size is read off the sub-image - assuming 29x33 put
		// the bars a whole portrait's width away from it.
		INT32 faceW = 29, faceH = 33;
		if (guiChessSelf)
		{
			const ETRLEObject& e = guiChessSelf->SubregionProperties(0);
			faceW = e.usWidth;
			faceH = e.usHeight;
			BltVideoObject(FRAME_BUFFER, guiChessSelf, 0,
			               CH_X(CH_NAV_X + 3), CH_Y(CH_RAIL_AVATAR_Y));
		}
		if (!gChessSelfNick.empty())
		{
			const INT32 barX = CH_NAV_X + 3 + faceW + 4;
			const INT32 mid  = CH_RAIL_AVATAR_Y + faceH / 2;
			FillRect(barX, mid - 6, 22, 4, CH_RGB_NICK);
			FillRect(barX, mid + 1, 15, 3, CH_RGB_NICK_DIM);
		}
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

		// Coordinates sit inside the corner squares, in the opposite colour.
		// These go through a char buffer on purpose: ST::format renders a bare
		// char as its numeric value, which turned the ranks into 49..56.
		for (int i = 0; i < 8; ++i)
		{
			const UINT8 rankSq = ScreenToSquare(0, i);
			const UINT8 fileSq = ScreenToSquare(i, 7);
			const char rankGlyph[2] = { char('1' + ChessGame::RankOf(rankSq)), '\0' };
			const char fileGlyph[2] = { char('a' + ChessGame::FileOf(fileSq)), '\0' };
			PrintAt(FONT10ARIAL, IsLightSquare(rankSq) ? FONT_GRAY7 : FONT_WHITE,
			        CH_BOARD_X + 3, CH_BOARD_Y + i * CH_SQ + 4, rankGlyph);
			PrintAt(FONT10ARIAL, IsLightSquare(fileSq) ? FONT_GRAY7 : FONT_WHITE,
			        CH_BOARD_X + i * CH_SQ + CH_SQ - 8, CH_BOARD_BOTTOM - 11, fileGlyph);
		}

		RoundCorners(CH_BOARD_X, CH_BOARD_Y, CH_BOARD_SIZE, CH_BOARD_SIZE,
		             CH_RADIUS, CH_RGB_CHROME);

		if (!guiChessPieces) return;
		for (int row = 0; row < 8; ++row)
		{
			for (int col = 0; col < 8; ++col)
			{
				const UINT8 sq = ScreenToSquare(col, row);
				if (gChessGame.IsEmpty(sq)) continue;
				// the piece in hand rides the cursor instead of its square
				if (gfChessDragging && sq == gubChessDragFrom) continue;
				const UINT8 type = gChessGame.PieceAt(sq);
				const UINT16 frame = UINT16((type - 1) +
					(gChessGame.ColorAt(sq) == ChessGame::Black ? 6 : 0));
				BltVideoObject(FRAME_BUFFER, guiChessPieces, frame,
				               CH_X(CH_BOARD_X + col * CH_SQ), CH_Y(CH_BOARD_Y + row * CH_SQ));
			}
		}

		// the lifted piece, centred on the pointer. Mouse coords are already
		// screen space, so these do not go through CH_X/CH_Y.
		if (gfChessDragging && !gChessGame.IsEmpty(gubChessDragFrom))
		{
			const UINT8 type = gChessGame.PieceAt(gubChessDragFrom);
			const UINT16 frame = UINT16((type - 1) +
				(gChessGame.ColorAt(gubChessDragFrom) == ChessGame::Black ? 6 : 0));
			BltVideoObject(FRAME_BUFFER, guiChessPieces, frame,
			               INT32(gusMouseXPos) - CH_SQ / 2, INT32(gusMouseYPos) - CH_SQ / 2);
		}
	}

	// The coach: her portrait, bare, with a white bubble beside it. No tile and
	// no caption - the portrait is the label.
	void ChessRenderCoach(INT32 y)
	{
		const INT32 faceX = CH_PANEL_X + 4;
		if (guiChessCoach)
		{
			BltVideoObject(FRAME_BUFFER, guiChessCoach, 0, CH_X(faceX), CH_Y(y));
		}

		const INT32 bubbleX = faceX + 29 + 5;
		const INT32 bubbleW = CH_PANEL_X + CH_PANEL_W - 4 - bubbleX;
		FillRounded(bubbleX, y, bubbleW, CH_COACH_TILE, CH_RGB_BUBBLE, 3, CH_RGB_PANEL);

		// The tail narrows to a point as it travels away from the bubble and
		// towards her: each sliver further left is shorter, not taller.
		const INT32 tailY = y + 13;
		for (int i = 0; i < 4; ++i)
		{
			const INT32 h = 8 - 2 * i;
			if (h <= 0) break;
			FillRect(bubbleX - 1 - i, tailY - h / 2, 1, h, CH_RGB_BUBBLE);
		}

		// on white, the verdict has to read dark
		const UINT8 colour = gChessState == CHUI_SOLVED ? FONT_DKGREEN
		                   : gChessState == CHUI_FAILED ? FONT_DKRED
		                   : FONT_MCOLOR_BLACK;
		DisplayWrappedString(UINT16(CH_X(bubbleX + 4)), UINT16(CH_Y(y + 5)),
		                     UINT16(bubbleW - 8), 1, FONT10ARIAL, colour,
		                     ChessCoachLine(), FONT_MCOLOR_WHITE, LEFT_JUSTIFIED);
	}

	void ChessRenderPanel()
	{
		FillRounded(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		            CH_RGB_PANEL, CH_RADIUS, CH_RGB_CHROME);
		const INT32 cx = CH_PANEL_X + CH_PANEL_W / 2;
		const ChessPuzzle& puzzle = CHESS_PUZZLES[giChessPuzzle];

		// header band: a lighter ground stands in for the rule that used to
		// sit under the date
		FillRect(CH_PANEL_X, CH_INSET, CH_PANEL_W, 66, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_RADIUS, CH_RGB_CHROME);

		const ST::string title = T(CHS_TITLE);
		const INT32 titleW = StringPixLength(title, FONT10ARIALBOLD) + 18;
		if (guiChessIcons)
		{
			BltVideoObject(FRAME_BUFFER, guiChessIcons, CH_ICON_PUZZLEMARK,
			               CH_X(cx - titleW / 2), CH_Y(14));
		}
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx - titleW / 2 + 18, 15, title);

		// date stepper: < [calendar] DAY n >
		const ST::string day = ST::format("{} {}", T(CHS_DAY), giChessViewDay);
		const INT32 stepW = StringPixLength(day, FONT10ARIAL) + 20;
		FillRounded(cx - stepW / 2, CH_DATE_Y, stepW, 16, CH_RGB_PANEL_UP, 3, CH_RGB_PANEL_SUNK);
		if (guiChessIcons)
		{
			BltVideoObject(FRAME_BUFFER, guiChessIcons, CH_ICON_CALENDAR,
			               CH_X(cx - stepW / 2 + 3), CH_Y(CH_DATE_Y + 1));
		}
		PrintAt(FONT10ARIAL, FONT_MCOLOR_WHITE, cx - stepW / 2 + 20, CH_DATE_Y + 3, day);
		// arrows grey out at the ends of the run; centred in their own hit
		// regions rather than hung off the chip, which changes width
		ChessDrawChevron(CH_PREV_X + CH_ARROW_W / 2, CH_DATE_Y + 8, true,
		                 giChessViewDay > 1 ? FROMRGB(232, 230, 227) : FROMRGB(110, 104, 98));
		ChessDrawChevron(CH_NEXT_X + CH_ARROW_W / 2, CH_DATE_Y + 8, false,
		                 giChessViewDay < ChessToday() ? FROMRGB(232, 230, 227) : FROMRGB(110, 104, 98));

		// the day's title, under the stepper. Titles run with the rating sort,
		// so they escalate from contract work to the war itself.
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, CH_DATE_Y + 22,
		             puzzle.title);

		ChessRenderCoach(CH_COACH_Y);

		// tries, directly under the coach: she is the one keeping count. The
		// hearts are the label.
		const INT32 heartY = CH_COACH_Y + CH_COACH_TILE + 16;
		for (int i = 0; i < CH_MAX_HEARTS; ++i)
		{
			ChessDrawHeart(CH_PANEL_X + 10 + i * CH_HEART_PITCH, heartY,
			               i >= gubChessHearts ? CH_RGB_HEART_SPENT : CH_RGB_CTA);
		}

		PrintAt(FONT10ARIAL, FONT_GRAY4, CH_PANEL_X + 10, heartY + 14,
		        ST::format("{} {}", T(CHS_RATING), puzzle.rating));
		PrintAt(FONT10ARIALBOLD, FONT_GRAY2, CH_PANEL_X + 10, heartY + 28,
		        gChessSolver == ChessGame::White ? T(CHS_WHITE_MOVES) : T(CHS_BLACK_MOVES));

		// the streak, at the foot: a flame and a number, nothing else
		if (guiChessIcons)
		{
			BltVideoObject(FRAME_BUFFER, guiChessIcons, CH_ICON_FLAME,
			               CH_X(CH_PANEL_X + 10), CH_Y(CH_PAGE_H - 76));
		}
		PrintAt(FONT10ARIALBOLD, gubChessStreak > 0 ? FONT_MCOLOR_WHITE : FONT_GRAY7,
		        CH_PANEL_X + 28, CH_PAGE_H - 75, ST::format("{}", gubChessStreak));

		// the hint button greys out once it has been spent
		const bool hintLive = gChessState == CHUI_PUZZLE && !(gubChessFlags & CH_FLAG_HINT_USED);
		FillRounded(CH_PANEL_X + 10, CH_PAGE_H - 46, CH_PANEL_W - 20, 22,
		            hintLive ? CH_RGB_PANEL_UP : CH_RGB_PANEL_SUNK, 3, CH_RGB_PANEL);
		PrintCentred(FONT10ARIAL, hintLive ? FONT_MCOLOR_WHITE : FONT_GRAY7,
		             cx, CH_PAGE_H - 40, T(CHS_HINT));
	}

	// The result card, over a scanline-dimmed board. Square corners on purpose:
	// rounding it would need the board colour behind each corner, and the board
	// is not one colour.
	void ChessRenderModal()
	{
		if (!gfChessModal) return;

		// dim by dropping every other scanline to the chrome tone
		for (INT32 row = 0; row < CH_BOARD_SIZE; row += 2)
		{
			FillRect(CH_BOARD_X, CH_BOARD_Y + row, CH_BOARD_SIZE, 1, CH_RGB_CHROME);
		}

		const INT32 w = CH_MODAL_W, h = CH_MODAL_H;
		const INT32 x = CH_MODAL_X, y = CH_MODAL_Y;
		const INT32 cx = x + w / 2;

		FillRect(x - 1, y - 1, w + 2, h + 2, CH_RGB_PANEL_UP);
		FillRect(x, y, w, h, CH_RGB_PANEL);

		PrintAt(FONT10ARIAL, FONT_GRAY4, x + w - 14, y + 4, "X");

		const bool won = gChessState == CHUI_SOLVED;
		const ChessStr title = !won                          ? CHS_MODAL_FAILED
		                     : gubChessHearts == CH_MAX_HEARTS ? CHS_MODAL_PERFECT
		                                                       : CHS_MODAL_SOLVED;
		// white on a win: the green hearts below already carry that. Red is kept
		// for the loss, which has nothing else saying so.
		PrintCentred(FONT10ARIALBOLD, won ? FONT_MCOLOR_WHITE : FONT_MCOLOR_LTRED,
		             cx, y + 12, T(title));

		// hearts left standing, in the CTA green rather than the counter's red
		const INT32 heartsW = CH_MAX_HEARTS * 18 - 4;
		for (int i = 0; i < CH_MAX_HEARTS; ++i)
		{
			ChessDrawHeart(cx - heartsW / 2 + i * 18, y + 28,
			               i < gubChessHearts ? CH_RGB_CTA : CH_RGB_PANEL_SUNK, 2);
		}

		FillRounded(x + 12, y + 46, w - 24, 22, CH_RGB_CTA, 3, CH_RGB_PANEL);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, y + 52, T(CHS_MODAL_ARCHIVE));

		// streak and best, side by side on sunk ground
		const INT32 boxW = (w - 30) / 2;
		const INT32 boxY = y + 76;
		for (int i = 0; i < 2; ++i)
		{
			const INT32 bx = x + 12 + i * (boxW + 6);
			FillRect(bx, boxY, boxW, 28, CH_RGB_PANEL_SUNK);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, bx + boxW / 2, boxY + 3,
			             ST::format("{}", i == 0 ? gubChessStreak : gubChessBestStreak));
			PrintCentred(FONT10ARIAL, FONT_GRAY4, bx + boxW / 2, boxY + 15,
			             T(i == 0 ? CHS_MODAL_STREAK : CHS_MODAL_BEST));
		}
	}

	void ChessRenderFooter()
	{
		PrintAt(FONT10ARIAL, FONT_GRAY7, CH_BOARD_X, CH_PAGE_H - 16, T(CHS_FOOTER));
	}
}

// --- laptop page hooks ----------------------------------------------------

void EnterChess()
{
	gubChessFlags |= CH_FLAG_DISCOVERED;

	guiChessPieces = nullptr;
	guiChessCoach  = nullptr;
	guiChessIcons  = nullptr;
	guiChessLogo   = nullptr;
	guiChessSelf   = nullptr;
	gChessSelfNick = ST::string();
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
		guiChessIcons = AddVideoObjectFromFile("sti/laptop/chessicons.sti");
		guiChessLogo  = AddVideoObjectFromFile("sti/laptop/chesslogo.sti");
	}
	catch (...)
	{
		// chrome only: labels stand on their own without the icons
	}
	try
	{
		// Buns coaches: a schoolteacher by trade, and German, which is why the
		// site has a language switch at all. Merc faces have to come through
		// Load33Portrait - the raw FACESDIR path only resolves for NPCs, which
		// is why the tile was empty.
		guiChessCoach = Load33Portrait(GetProfile(BUNS));
	}
	catch (...)
	{
	}
	// the account block shows your own I.M.P. character as the site avatar
	if (LaptopSaveInfo.fIMPCompletedFlag)
	{
		try
		{
			MERCPROFILESTRUCT const& imp = GetProfile(
				static_cast<ProfileID>(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId));
			guiChessSelf   = Load33Portrait(imp);
			gChessSelfNick = imp.zNickname;
		}
		catch (...)
		{
		}
	}

	ChessBeginSession();
	ChessPlaceRegions();
}

void ExitChess()
{
	ChessRemoveRegions();
	if (guiChessPieces) { DeleteVideoObject(guiChessPieces); guiChessPieces = nullptr; }
	if (guiChessCoach)  { DeleteVideoObject(guiChessCoach);  guiChessCoach  = nullptr; }
	if (guiChessIcons)  { DeleteVideoObject(guiChessIcons);  guiChessIcons  = nullptr; }
	if (guiChessLogo)   { DeleteVideoObject(guiChessLogo);   guiChessLogo   = nullptr; }
	if (guiChessSelf)   { DeleteVideoObject(guiChessSelf);   guiChessSelf   = nullptr; }
}

void RenderChess()
{
	FillRect(0, 0, LAPTOP_SCREEN_WIDTH, CH_PAGE_H, CH_RGB_CHROME);
	ChessRenderNav();
	ChessRenderBoard();
	ChessRenderPanel();
	ChessRenderFooter();
	ChessRenderModal();

	MarkButtonsDirty();
	RenderWWWProgramTitleBar();
	InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
	                 LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
}

void HandleChess()
{
	// a piece in hand has to be repainted every frame to keep up with the
	// pointer, and the drop is resolved here rather than in a region callback
	if (gfChessDragging)
	{
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) ChessRedraw();
		else                                      ChessResolveDrop();
	}

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
			ChessPlay(ChessMoveSound(reply, gChessGame.IsInCheck(gChessGame.SideToMove()), false));
			++guiChessPly;
		}
	}
	if (guiChessPly < gChessSolution.size()) giChessSaid = CHS_ST_YOUR_MOVE;
	if (guiChessPly >= gChessSolution.size() && gChessState == CHUI_PUZZLE) ChessRecordSolved();
	ChessRedraw();
}
