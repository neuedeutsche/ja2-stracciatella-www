// chach.com: Grunty's daily chess puzzle, run off a box in his apartment.
//
// One puzzle per campaign day, five attempts, a streak that breaks if you skip
// a day. The layout is chess.com's current one - nav rail, board, right-hand
// panel - reproduced at the 502x400 the laptop's web area gives us.
//
// All rules live in the engine-free ChessGame class and the corpus in the
// generated ChessPuzzles.cc; this file is the laptop page wrapper.

#include "Chess.h"

#include "ChessDaily.h"
#include "ChessGame.h"
#include "ChessPuzzles.h"

#include "Button_System.h"
#include "Cursors.h"
#include "Directories.h"
#include "EMail.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "Game_Event_Hook.h"
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

#define CH_DATE_Y       28
#define CH_COACH_Y      78
#define CH_COACH_TILE   36
#define CH_HINT_Y       (CH_PAGE_H - 34)
#define CH_HINT_H       22

// The day stepper's arrows sit at fixed points on the panel edges while the
// chip between them is centred and changes width with the day number. Glyphs
// and hit regions are both derived from these, so they cannot drift apart.
// The chip is a fixed width so the arrows can sit hard against it without the
// two drifting apart as the day number grows a digit.
#define CH_CHIP_W       66
#define CH_CHIP_X       (CH_PANEL_X + (CH_PANEL_W - CH_CHIP_W) / 2)
#define CH_ARROW_W      16
#define CH_ARROW_H      20
#define CH_PREV_X       (CH_CHIP_X - CH_ARROW_W - 2)
#define CH_NEXT_X       (CH_CHIP_X + CH_CHIP_W + 2)

// rail furniture, measured down from the page foot
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

// the ad slot and the counter fill the dead run under the board
#define CH_BANNER_Y     (CH_BOARD_BOTTOM + 12)
#define CH_BANNER_H     30
#define CH_COUNTER_Y    (CH_BANNER_Y + CH_BANNER_H + 8)

#define CH_REPLY_DELAY  650  // ms before the scripted reply lands

// chess.com's palette, sampled from the live site
#define CH_RGB_LIGHT       FROMRGB(235, 236, 208)
#define CH_RGB_DARK        FROMRGB(115, 149,  82)
#define CH_RGB_HL_LIGHT    FROMRGB(245, 246, 130)
#define CH_RGB_HL_DARK     FROMRGB(185, 202,  67)
// the board under the result card: every square colour pulled halfway to the
// chrome grey, in place of the scanline dim
#define CH_RGB_LIGHT_DIM   FROMRGB(140, 139, 123)
#define CH_RGB_DARK_DIM    FROMRGB( 80,  95,  60)
#define CH_RGB_HL_LIGHT_DIM FROMRGB(145, 144,  84)
#define CH_RGB_HL_DARK_DIM  FROMRGB(115, 122,  52)
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
	UINT32 guiChessReplyDue = 0;     // when the scripted reply lands, 0 if none
	UINT8  gubChessLastFrom = ChessGame::NO_SQUARE;
	UINT8  gubChessLastTo   = ChessGame::NO_SQUARE;
	bool   gfChessHintShown = false;
	// the result card, raised on the move that ends the day and dismissed by
	// hand; revisiting a finished day does not raise it again
	bool   gfChessModal     = false;
	// the proprietor is on one of your contracts, so the site runs unattended
	bool   gfChessOffline   = false;
	// which nav section is showing its unfinished page, or -1 for the puzzle
	int    giChessStub      = -1;
	// what the coach is currently saying, kept as an id so the language switch
	// re-renders it rather than freezing whatever was said last
	int    giChessSaid      = 0;   // ChessStr, or -1/-2 for a good/bad variant
	int    giChessVariant   = 0;

	// persisted; the rules that move it live in ChessDaily, where they are
	// tested without a laptop or a save game in sight
	ChessDaily::State gChessDay;

	// Chess.h repeats these bits so the laptop can read them without pulling
	// the daily module in. They must not drift.
	static_assert(CHESS_FLAG_SOLVED     == ChessDaily::FLAG_SOLVED,     "flag drift");
	static_assert(CHESS_FLAG_FAILED     == ChessDaily::FLAG_FAILED,     "flag drift");
	static_assert(CHESS_FLAG_HINT_USED  == ChessDaily::FLAG_HINT_USED,  "flag drift");
	static_assert(CHESS_FLAG_DISCOVERED == ChessDaily::FLAG_DISCOVERED, "flag drift");
	static_assert(CHESS_FLAG_INVITED    == ChessDaily::FLAG_INVITED,    "flag drift");
	static_assert(CHESS_FLAG_DOWN_NOTED == ChessDaily::FLAG_DOWN_NOTED, "flag drift");

	SGPVObject* guiChessPieces = nullptr;  // 12 frames, 34x34
	SGPVObject* guiChessCoach  = nullptr;  // Grunty, 29x33
	SGPVObject* guiChessIcons  = nullptr;  // 7 nav and panel icons, 14x14
	SGPVObject* guiChessLogo   = nullptr;  // green pawn, 22 and 14
	SGPVObject* guiChessBanner = nullptr;  // 3 ad slots, 272x30
	SGPVObject* guiChessSelf   = nullptr;  // the player's I.M.P. portrait
	ST::string  gChessSelfNick;

	// which campaign day is on screen; past days are archive, view only
	int giChessViewDay = 1;

	MOUSE_REGION gChessSquare[64];
	MOUSE_REGION gChessHintRegion;
	MOUSE_REGION gChessNavRegion[5];
	MOUSE_REGION gChessPrevDayRegion;
	MOUSE_REGION gChessNextDayRegion;
	MOUSE_REGION gChessModalCloseRegion;
	MOUSE_REGION gChessModalArchiveRegion;
	// sits behind everything and catches a piece released off the board, which
	// would otherwise leave it stuck to the cursor
	MOUSE_REGION gChessDropRegion;
	bool         gfChessRegionsUp = false;

	// The site is English. The German column is kept because the proprietor is
	// German and it costs nothing, but nothing reaches it now that the rail
	// switch is gone. Umlauts are spelled out: the laptop fonts are ASCII.
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
		CHS_DOWN_TITLE, CHS_DOWN_1, CHS_DOWN_2, CHS_DOWN_3,
		CHS_STUB_TITLE, CHS_STUB_PLAY, CHS_STUB_LEARN, CHS_STUB_WATCH,
		CHS_STUB_GROUPS, CHS_STUB_BACK, CHS_VISITOR,
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
			"UNATTENDED",
			"ze proprietor is on contract. yours.",
			"ze puzzle runs without me. it always did.",
			"do not break anything. - G.",
			"UNDER CONSTRUCTION",
			"ze server cannot hold two games at once.",
			"I will write it when I understand it myself.",
			"zere is nothing to watch. zis is a puzzle site.",
			"a group needs two people.",
			"back to ze puzzle",
			"you are visitor no.",
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
			"UNBEAUFSICHTIGT",
			"der Betreiber ist im Einsatz. bei Ihnen.",
			"das Raetsel laeuft ohne mich. immer schon.",
			"machen Sie nichts kaputt. - G.",
			"IM AUFBAU",
			"der Server haelt keine zwei Partien.",
			"ich schreibe es, wenn ich es selbst verstehe.",
			"es gibt nichts zu sehen. dies ist ein Raetsel.",
			"eine Gruppe braucht zwei Leute.",
			"zurueck zum Raetsel",
			"Sie sind Besucher Nr.",
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
		// away on contract: the site still answers, he does not
		if (gfChessOffline) return CHESS_TEXT[lang][CHS_DOWN_2];
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
	#define CH_SND_CLICK    SOUNDSDIR "/very small switch 01 in.wav"
	#define CH_SND_CLICK2   SOUNDSDIR "/very small switch 02 in.wav"

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

	// The whole operation is one man and one modem, and the modem is in his
	// apartment. Hire him and the site goes with him.
	bool ChessProprietorAway()
	{
		return FindSoldierByProfileIDOnPlayerTeam(GRUNTY) != NULL;
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
	p.usDay           = gChessDay.day;
	p.usLastSolvedDay = gChessDay.lastSolvedDay;
	p.ubStreak        = gChessDay.streak;
	p.ubBestStreak    = gChessDay.bestStreak;
	p.ubHearts        = gChessDay.hearts;
	p.ubFlags         = gChessDay.flags;
	return p;
}

void ChessSetPersist(const ChessPersist& p)
{
	gChessDay.day           = p.usDay;
	gChessDay.lastSolvedDay = p.usLastSolvedDay;
	gChessDay.streak        = p.ubStreak;
	gChessDay.bestStreak    = p.ubBestStreak;
	gChessDay.hearts        = std::min<UINT8>(p.ubHearts, ChessDaily::MAX_HEARTS);
	gChessDay.flags         = p.ubFlags;
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

	// A 14px icon and a label packed as one group, both centred on the same
	// line. Eyeballing the two separately is what left them out of step.
	INT32 ChessIconLabelWidth(SGPFont font, const ST::string& text)
	{
		return 14 + 4 + StringPixLength(text, font);
	}

	void ChessIconLabel(UINT16 icon, INT32 x, INT32 midY, SGPFont font, UINT8 colour,
	                    const ST::string& text)
	{
		if (guiChessIcons)
		{
			BltVideoObject(FRAME_BUFFER, guiChessIcons, icon, CH_X(x), CH_Y(midY - 7));
		}
		PrintAt(font, colour, x + 18, midY - GetFontHeight(font) / 2, text);
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
		giChessPuzzle = ChessDaily::PuzzleIndexForDay(day, NUM_CHESS_PUZZLES);

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
	// He answers on the strategic clock, not the instant you click - which is
	// the only honest way for a man with one modem to reply.
	void ChessMail(int kind, int streak)
	{
		AddStrategicEvent(EVENT_CHESS_GRUNTY_EMAIL,
		                  GetWorldTotalMin() + 90 + Random(240),
		                  (UINT32(kind) << 16) | UINT32(streak & 0xFFFF));
	}

	void ChessRollOverDay()
	{
		const int today  = ChessToday();
		const int lapsed = ChessDaily::LapsedStreak(gChessDay, today);
		if (lapsed > 0)
		{
			// only a run worth remarking on earns a letter
			if (lapsed >= 3) ChessMail(2, lapsed);
			ChessDaily::ClearStreak(gChessDay);
		}
		ChessDaily::RollOverDay(gChessDay, today);
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
		else if (gChessDay.flags & ChessDaily::FLAG_SOLVED)
		{
			ChessPlayOutSolution();
			gChessState = CHUI_SOLVED;
			giChessSaid = CHS_ST_ALREADY;
		}
		else if (gChessDay.flags & ChessDaily::FLAG_FAILED)
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

	// The card's regions sit above the board, so they are only live while it is
	// up - otherwise they would swallow clicks on the squares underneath.
	void ChessSetModal(bool up)
	{
		gfChessModal = up;
		if (!gfChessRegionsUp) return;
		if (up)
		{
			gChessModalCloseRegion.Enable();
			gChessModalArchiveRegion.Enable();
		}
		else
		{
			gChessModalCloseRegion.Disable();
			gChessModalArchiveRegion.Disable();
		}
	}

	void ChessRecordSolved()
	{
		ChessDaily::RecordSolved(gChessDay, ChessToday());
		// he notices a run at three, and again every week it survives
		const int streak = gChessDay.streak;
		if (streak == 3 || (streak >= 7 && streak % 7 == 0)) ChessMail(1, streak);
		ChessSetModal(true);
		gChessState  = CHUI_SOLVED;
		giChessSaid = CHS_ST_DONE;
	}

	void ChessRecordFailed()
	{
		ChessDaily::RecordFailed(gChessDay);
		ChessSetModal(true);
		ChessPlayOutSolution();
		gChessState  = CHUI_FAILED;
		giChessSaid = CHS_ST_OUT;
	}

	void ChessSpendHeart()
	{
		if (ChessDaily::SpendHeart(gChessDay)) ChessRecordFailed();
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
		if (gfChessModal || giChessStub >= 0) return;
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
		if (gChessDay.flags & ChessDaily::FLAG_HINT_USED) return;
		if (guiChessPly >= gChessSolution.size()) return;

		gChessDay.flags |= ChessDaily::FLAG_HINT_USED;
		ChessPlay(CH_SND_CLICK);
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
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		ChessShowDay(want);
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

	void ChessModalCloseCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfChessModal) return;
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		ChessSetModal(false);
		ChessRedraw();
	}

	void ChessModalArchiveCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfChessModal) return;
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		ChessSetModal(false);
		ChessShowDay(giChessViewDay - 1);
		ChessRedraw();
	}

	void ChessNavCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const int item = int(MSYS_GetRegionUserData(region, 0));
		// Puzzles is the site; everything else is a page he has not written
		const int want = (item == 1) ? -1 : item;
		if (want != giChessStub) ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		giChessStub = want;
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
		                  UINT16(CH_X(CH_PANEL_X + 10)), UINT16(CH_Y(CH_HINT_Y)),
		                  UINT16(CH_X(CH_PANEL_X + CH_PANEL_W - 10)), UINT16(CH_Y(CH_HINT_Y + CH_HINT_H)),
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

		for (int i = 0; i < 5; ++i)
		{
			const UINT16 y = UINT16(CH_Y(32 + i * 20 - 3));
			MSYS_DefineRegion(&gChessNavRegion[i],
			                  UINT16(CH_X(CH_NAV_X)), y,
			                  UINT16(CH_X(CH_NAV_X + CH_NAV_W)), UINT16(y + 18),
			                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
			                  ChessNavCallback);
			MSYS_SetRegionUserData(&gChessNavRegion[i], 0, i);
		}

		MSYS_DefineRegion(&gChessModalCloseRegion,
		                  UINT16(CH_X(CH_MODAL_X + CH_MODAL_W - 20)), UINT16(CH_Y(CH_MODAL_Y + 2)),
		                  UINT16(CH_X(CH_MODAL_X + CH_MODAL_W - 2)),  UINT16(CH_Y(CH_MODAL_Y + 20)),
		                  MSYS_PRIORITY_HIGHEST, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessModalCloseCallback);
		MSYS_DefineRegion(&gChessModalArchiveRegion,
		                  UINT16(CH_X(CH_MODAL_X + 12)), UINT16(CH_Y(CH_MODAL_Y + 46)),
		                  UINT16(CH_X(CH_MODAL_X + CH_MODAL_W - 12)), UINT16(CH_Y(CH_MODAL_Y + 68)),
		                  MSYS_PRIORITY_HIGHEST, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessModalArchiveCallback);
		gChessModalCloseRegion.Disable();
		gChessModalArchiveRegion.Disable();

		gfChessRegionsUp = true;
		if (gfChessModal) ChessSetModal(true);
	}

	// Everything clickable, in one switch: used when the site is down.
	void ChessEnableRegions(bool on)
	{
		if (!gfChessRegionsUp) return;
		for (MOUSE_REGION& r : gChessSquare) { if (on) r.Enable(); else r.Disable(); }
		if (on) gChessHintRegion.Enable();    else gChessHintRegion.Disable();
		if (on) gChessPrevDayRegion.Enable(); else gChessPrevDayRegion.Disable();
		if (on) gChessNextDayRegion.Enable(); else gChessNextDayRegion.Disable();
	}

	void ChessRemoveRegions()
	{
		if (!gfChessRegionsUp) return;
		for (MOUSE_REGION& r : gChessSquare) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessDropRegion);
		MSYS_RemoveRegion(&gChessHintRegion);
		MSYS_RemoveRegion(&gChessModalCloseRegion);
		MSYS_RemoveRegion(&gChessModalArchiveRegion);
		for (MOUSE_REGION& r : gChessNavRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessPrevDayRegion);
		MSYS_RemoveRegion(&gChessNextDayRegion);
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

	// A spent heart: grey, and cracked - a zigzag gap splits it in two.
	void ChessDrawBrokenHeart(INT32 x, INT32 y, UINT32 rgb, UINT32 bg, INT32 s = 1)
	{
		ChessDrawHeart(x, y, rgb, s);
		FillRect(x + 3 * s, y,         1 * s, 2 * s, bg);
		FillRect(x + 4 * s, y + 2 * s, 1 * s, 1 * s, bg);
		FillRect(x + 3 * s, y + 3 * s, 1 * s, 1 * s, bg);
		FillRect(x + 4 * s, y + 4 * s, 1 * s, 1 * s, bg);
		FillRect(x + 3 * s, y + 5 * s, 1 * s, 1 * s, bg);
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
		const INT32 markW = StringPixLength("Chach", FONT10ARIALBOLD) +
		                    StringPixLength(".com", FONT10ARIAL);
		const INT32 lockW = 14 + 2 + markW;
		const INT32 lockX = std::max(2, CH_NAV_X + (CH_NAV_W - lockW) / 2);
		if (guiChessLogo)
		{
			// frame 1 is the 14px pawn; the 22px one will not sit on one line
			BltVideoObject(FRAME_BUFFER, guiChessLogo, 1, CH_X(lockX + 1), CH_Y(8));
		}
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, lockX + 15, 13, "Chach");
		PrintAt(FONT10ARIAL, FONT_GRAY2,
		        lockX + 15 + StringPixLength("Chach", FONT10ARIALBOLD), 13, ".com");

		static const ChessStr items[5] = {
			CHS_NAV_PLAY, CHS_NAV_PUZZLES, CHS_NAV_LEARN, CHS_NAV_WATCH, CHS_NAV_COMMUNITY
		};
		for (int i = 0; i < 5; ++i)
		{
			const INT32 rowY = 32 + i * 20;
			const bool active = (giChessStub < 0) ? (i == 1) : (i == giChessStub);
			if (active)
			{
				FillRounded(CH_NAV_X + 2, rowY - 3, CH_NAV_W - 4, 18,
				            CH_RGB_PANEL_UP, 3, CH_RGB_PANEL);
			}
			if (guiChessIcons)
			{
				// two up from the row origin: the 14px icon otherwise hangs
				// below the 10px label's line
				BltVideoObject(FRAME_BUFFER, guiChessIcons, UINT16(i),
				               CH_X(CH_NAV_X + 4), CH_Y(rowY - 2));
			}
			PrintAt(FONT10ARIAL, active ? FONT_MCOLOR_WHITE : FONT_GRAY2,
			        CH_NAV_X + 20, rowY + 1, T(items[i]));
		}

		// The account row: your I.M.P. portrait as the site avatar, with the
		// nickname suggested by two grey bars rather than set in type. The
		// portrait's real size is read off the sub-image - assuming 29x33 put
		// the bars a whole portrait's width away from it.
		INT32 faceW = 26, faceH = 30;
		if (guiChessSelf)
		{
			const ETRLEObject& e = guiChessSelf->SubregionProperties(0);
			faceW = e.usWidth;
			faceH = e.usHeight;
		}
		// anchored to the rail's bottom-left corner off the portrait's real
		// height, so a shorter image does not leave it floating
		const INT32 faceX = CH_NAV_X + 3;
		const INT32 faceY = CH_PAGE_H - faceH - 5;
		if (guiChessSelf)
		{
			BltVideoObject(FRAME_BUFFER, guiChessSelf, 0, CH_X(faceX), CH_Y(faceY));
		}
		if (!gChessSelfNick.empty())
		{
			const INT32 barX = faceX + faceW + 4;
			const INT32 mid  = faceY + faceH / 2;
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
				UINT32 rgb = lit ? (light ? CH_RGB_HL_LIGHT : CH_RGB_HL_DARK)
				                 : (light ? CH_RGB_LIGHT : CH_RGB_DARK);
				if (gfChessModal)
				{
					rgb = lit ? (light ? CH_RGB_HL_LIGHT_DIM : CH_RGB_HL_DARK_DIM)
					          : (light ? CH_RGB_LIGHT_DIM : CH_RGB_DARK_DIM);
				}
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
			const UINT8 onLight = gfChessModal ? FONT_GRAY6 : FONT_GRAY7;
			const UINT8 onDark  = gfChessModal ? FONT_GRAY4 : FONT_MCOLOR_WHITE;
			PrintAt(FONT10ARIAL, IsLightSquare(rankSq) ? onLight : onDark,
			        CH_BOARD_X + 3, CH_BOARD_Y + i * CH_SQ + 4, rankGlyph);
			PrintAt(FONT10ARIAL, IsLightSquare(fileSq) ? onLight : onDark,
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
				// frames 12-23 are the dimmed twins, used while the card is up
				const UINT16 frame = UINT16((type - 1) +
					(gChessGame.ColorAt(sq) == ChessGame::Black ? 6 : 0) +
					(gfChessModal ? 12 : 0));
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
		// Her portrait's real width, not an assumed 29: guessing it is what left
		// the bubble sitting a gap clear of her.
		const INT32 faceX = CH_PANEL_X + 4;
		INT32 faceW = 26;
		if (guiChessCoach)
		{
			faceW = guiChessCoach->SubregionProperties(0).usWidth;
			BltVideoObject(FRAME_BUFFER, guiChessCoach, 0, CH_X(faceX), CH_Y(y));
		}

		const INT32 bubbleX = faceX + faceW + 4;
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
		// the real bold cut: faking bold by double-printing 12pt crushed the
		// letter spacing, and oversize beat bold anyway
		const INT32 titleW = ChessIconLabelWidth(FONT10ARIALBOLD, title);
		ChessIconLabel(CH_ICON_PUZZLEMARK, cx - titleW / 2, 20,
		               FONT10ARIALBOLD, FONT_MCOLOR_WHITE, title);

		// date stepper: < [calendar] DAY n >
		const ST::string day = ST::format("{} {}", T(CHS_DAY), giChessViewDay);
		FillRounded(CH_CHIP_X, CH_DATE_Y, CH_CHIP_W, CH_ARROW_H,
		            CH_RGB_PANEL_UP, 3, CH_RGB_PANEL_SUNK);
		// icon and label centred in the chip as one group
		const INT32 groupW = ChessIconLabelWidth(FONT10ARIAL, day);
		ChessIconLabel(CH_ICON_CALENDAR, CH_CHIP_X + (CH_CHIP_W - groupW) / 2,
		               CH_DATE_Y + CH_ARROW_H / 2, FONT10ARIAL, FONT_MCOLOR_WHITE, day);
		// arrows grey out at the ends of the run; centred in their own hit
		// regions rather than hung off the chip, which changes width
		ChessDrawChevron(CH_PREV_X + CH_ARROW_W / 2, CH_DATE_Y + 10, true,
		                 giChessViewDay > 1 ? FROMRGB(148, 142, 136) : FROMRGB( 74,  69,  64));
		ChessDrawChevron(CH_NEXT_X + CH_ARROW_W / 2, CH_DATE_Y + 10, false,
		                 giChessViewDay < ChessToday() ? FROMRGB(148, 142, 136) : FROMRGB( 74,  69,  64));

		// the day's title, under the stepper. Titles run with the rating sort,
		// so they escalate from contract work to the war itself.
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, CH_DATE_Y + 26,
		             puzzle.title);

		ChessRenderCoach(CH_COACH_Y);

		// tries, directly under the coach: she is the one keeping count. The
		// hearts are the label.
		const INT32 heartY = CH_COACH_Y + CH_COACH_TILE + 7;
		for (int i = 0; i < ChessDaily::MAX_HEARTS; ++i)
		{
			if (i >= gChessDay.hearts)
			{
				ChessDrawBrokenHeart(CH_PANEL_X + 10 + i * CH_HEART_PITCH, heartY,
				                     CH_RGB_HEART_SPENT, CH_RGB_PANEL);
			}
			else
			{
				ChessDrawHeart(CH_PANEL_X + 10 + i * CH_HEART_PITCH, heartY, CH_RGB_CTA);
			}
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
		PrintAt(FONT10ARIALBOLD, gChessDay.streak > 0 ? FONT_MCOLOR_WHITE : FONT_GRAY7,
		        CH_PANEL_X + 28, CH_PAGE_H - 75, ST::format("{}", gChessDay.streak));

		// the hint button greys out once it has been spent
		const bool hintLive = gChessState == CHUI_PUZZLE && !(gChessDay.flags & ChessDaily::FLAG_HINT_USED);
		FillRounded(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20, CH_HINT_H,
		            hintLive ? CH_RGB_PANEL_UP : CH_RGB_PANEL_SUNK, 3, CH_RGB_PANEL);
		PrintCentred(FONT10ARIAL, hintLive ? FONT_MCOLOR_WHITE : FONT_GRAY7,
		             cx, CH_HINT_Y + (CH_HINT_H - GetFontHeight(FONT10ARIAL)) / 2, T(CHS_HINT));
	}

	// The result card, over a scanline-dimmed board. Square corners on purpose:
	// rounding it would need the board colour behind each corner, and the board
	// is not one colour.
	// What the site is when its one man is in the field: still serving, because
	// the daily puzzle is automated and he is not. Only he is missing.
	void ChessRenderUnattendedNotice()
	{
		FillRect(CH_BOARD_X, CH_BOARD_Y - 17, CH_BOARD_SIZE, 15, CH_RGB_PANEL_SUNK);
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_LTYELLOW,
		             CH_BOARD_X + CH_BOARD_SIZE / 2, CH_BOARD_Y - 16, T(CHS_DOWN_1));
	}

	// The result card, over a scanline-dimmed board. Square corners on purpose:
	// rounding it would need the board colour behind each corner, and the board
	// is not one colour.
	// What the site is when its one man is in the field.
	void ChessRenderOffline()
	{
		FillRounded(CH_BOARD_X, CH_BOARD_Y, CH_BOARD_SIZE, CH_BOARD_SIZE,
		            CH_RGB_PANEL, CH_RADIUS, CH_RGB_CHROME);
		const INT32 cx = CH_BOARD_X + CH_BOARD_SIZE / 2;
		const INT32 top = CH_BOARD_Y + CH_BOARD_SIZE / 2 - 46;

		if (guiChessCoach)
		{
			// his own portrait, since it is his apartment
			BltVideoObject(FRAME_BUFFER, guiChessSelf ? guiChessSelf : guiChessCoach, 0,
			               CH_X(cx - 13), CH_Y(top));
		}
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, top + 44, T(CHS_DOWN_TITLE));
		PrintCentred(FONT10ARIAL, FONT_GRAY2, cx, top + 62, T(CHS_DOWN_1));
		PrintCentred(FONT10ARIAL, FONT_GRAY2, cx, top + 74, T(CHS_DOWN_2));
		PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, top + 92, T(CHS_DOWN_3));
	}

	void ChessRenderModal()
	{
		if (!gfChessModal) return;

		const INT32 w = CH_MODAL_W, h = CH_MODAL_H;
		const INT32 x = CH_MODAL_X, y = CH_MODAL_Y;
		const INT32 cx = x + w / 2;

		FillRect(x - 1, y - 1, w + 2, h + 2, CH_RGB_PANEL_UP);
		FillRect(x, y, w, h, CH_RGB_PANEL);

		PrintAt(FONT10ARIAL, FONT_GRAY4, x + w - 14, y + 4, "X");

		const bool won = gChessState == CHUI_SOLVED;
		const ChessStr title = !won                          ? CHS_MODAL_FAILED
		                     : gChessDay.hearts == ChessDaily::MAX_HEARTS ? CHS_MODAL_PERFECT
		                                                       : CHS_MODAL_SOLVED;
		// white on a win: the green hearts below already carry that. Red is kept
		// for the loss, which has nothing else saying so.
		PrintCentred(FONT10ARIALBOLD, won ? FONT_MCOLOR_WHITE : FONT_MCOLOR_LTRED,
		             cx, y + 12, T(title));

		// hearts left standing, in the CTA green rather than the counter's red
		const INT32 heartsW = ChessDaily::MAX_HEARTS * 18 - 4;
		for (int i = 0; i < ChessDaily::MAX_HEARTS; ++i)
		{
			if (i < gChessDay.hearts)
			{
				ChessDrawHeart(cx - heartsW / 2 + i * 18, y + 28, CH_RGB_CTA, 2);
			}
			else
			{
				ChessDrawBrokenHeart(cx - heartsW / 2 + i * 18, y + 28,
				                     CH_RGB_HEART_SPENT, CH_RGB_PANEL, 2);
			}
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
			             ST::format("{}", i == 0 ? gChessDay.streak : gChessDay.bestStreak));
			PrintCentred(FONT10ARIAL, FONT_GRAY4, bx + boxW / 2, boxY + 15,
			             T(i == 0 ? CHS_MODAL_STREAK : CHS_MODAL_BEST));
		}
	}

	// The ad slot: three creatives, rotating on the campaign clock, one of them
	// a house ad and one a plug for the other site in the collection.
	void ChessRenderBanner()
	{
		if (guiChessBanner)
		{
			const UINT16 slot = UINT16(giChessViewDay % 3);
			BltVideoObject(FRAME_BUFFER, guiChessBanner, slot,
			               CH_X(CH_BOARD_X), CH_Y(CH_BANNER_Y));
		}
		// A hit counter nobody has ever believed. Derived from the campaign
		// clock rather than stored, so it climbs without costing save bytes.
		const int hits = 148299 + giChessViewDay * 17 + gChessDay.bestStreak * 3;
		PrintCentred(FONT10ARIAL, FONT_GRAY7, CH_BOARD_X + CH_BOARD_SIZE / 2,
		             CH_COUNTER_Y, ST::format("{} {}", T(CHS_VISITOR), hits));
	}

	// Every other nav entry leads here, which is the honest state of them.
	void ChessRenderStub()
	{
		FillRounded(CH_BOARD_X, CH_BOARD_Y, CH_BOARD_SIZE, CH_BOARD_SIZE,
		            CH_RGB_PANEL, CH_RADIUS, CH_RGB_CHROME);
		const INT32 cx = CH_BOARD_X + CH_BOARD_SIZE / 2;
		const INT32 top = CH_BOARD_Y + 96;

		static const ChessStr excuse[5] = {
			CHS_STUB_PLAY, CHS_STUB_PLAY, CHS_STUB_LEARN, CHS_STUB_WATCH, CHS_STUB_GROUPS
		};
		if (guiChessIcons && giChessStub >= 0)
		{
			BltVideoObject(FRAME_BUFFER, guiChessIcons, UINT16(giChessStub),
			               CH_X(cx - 7), CH_Y(top - 26));
		}
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTYELLOW, cx, top, T(CHS_STUB_TITLE));
		PrintCentred(FONT10ARIAL, FONT_GRAY2, cx, top + 20,
		             T(excuse[giChessStub < 0 ? 0 : giChessStub]));
		PrintCentred(FONT10ARIAL, FONT_GRAY7, cx, top + 44, T(CHS_STUB_BACK));
	}

	void ChessRenderFooter()
	{
		PrintAt(FONT10ARIAL, FONT_GRAY7, CH_BOARD_X, CH_PAGE_H - 16, T(CHS_FOOTER));
	}
}

// --- laptop page hooks ----------------------------------------------------

void EnterChess()
{
	gChessDay.flags |= ChessDaily::FLAG_DISCOVERED;

	guiChessPieces = nullptr;
	guiChessCoach  = nullptr;
	guiChessIcons  = nullptr;
	guiChessLogo   = nullptr;
	guiChessBanner = nullptr;
	guiChessSelf   = nullptr;
	giChessStub    = -1;
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
		guiChessIcons  = AddVideoObjectFromFile("sti/laptop/chessicons.sti");
		guiChessLogo   = AddVideoObjectFromFile("sti/laptop/chesslogo.sti");
		guiChessBanner = AddVideoObjectFromFile("sti/laptop/chessbanner.sti");
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

	// a card left up when the player navigated away does not greet them on the
	// way back in
	gfChessModal = false;
	gfChessOffline = ChessProprietorAway();
	// he explains the outage once per contract, and can explain it again the
	// next time you hire him
	if (gfChessOffline && !(gChessDay.flags & ChessDaily::FLAG_DOWN_NOTED))
	{
		gChessDay.flags |= ChessDaily::FLAG_DOWN_NOTED;
		ChessMail(3, 0);
	}
	else if (!gfChessOffline)
	{
		gChessDay.flags &= UINT8(~ChessDaily::FLAG_DOWN_NOTED);
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
	if (guiChessBanner) { DeleteVideoObject(guiChessBanner); guiChessBanner = nullptr; }
	if (guiChessSelf)   { DeleteVideoObject(guiChessSelf);   guiChessSelf   = nullptr; }
}

void RenderChess()
{
	FillRect(0, 0, LAPTOP_SCREEN_WIDTH, CH_PAGE_H, CH_RGB_CHROME);
	ChessRenderNav();
	if (giChessStub >= 0)
	{
		ChessRenderStub();
		ChessRenderBanner();
		ChessRenderFooter();
	}
	else
	{
		ChessRenderBoard();
		if (gfChessOffline) ChessRenderUnattendedNotice();
		ChessRenderBanner();
		ChessRenderPanel();
		ChessRenderFooter();
		ChessRenderModal();
	}

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
