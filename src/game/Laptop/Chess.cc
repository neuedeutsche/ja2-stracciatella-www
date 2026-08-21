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
#include "ChessLessons.h"
#include "ChessPuzzles.h"

#include "Button_System.h"
#include "Cursors.h"
#include "Directories.h"
#include "EMail.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "Game_Event_Hook.h"
#include "History.h"
#include "HImage.h"
#include "IMP_Compile_Character.h"
#include "Input.h"
#include "Laptop.h"
#include "LaptopSave.h"
#include "Local.h"
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

// the ad slot sits at the site's bottom edge, above the footer line, and
// the hit counter rides just above it
#define CH_BANNER_H     30
#define CH_BANNER_Y     (CH_PAGE_H - 20 - CH_BANNER_H)
#define CH_COUNTER_Y    (CH_BANNER_Y - 13)
// player rows for the live views, above and below the board
#define CH_ROW_TOP_Y    3
#define CH_ROW_BOT_Y    (CH_BOARD_BOTTOM + 5)

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
	// which nav section is showing, or -1 for the daily puzzle:
	// 0 Play, 2 Learn, 3 Watch, 4 Guestbook; 1 routes back to the puzzle
	int    giChessStub      = -1;
	// Learn: which of the lessons is open, and its position on the board
	int    giChessLesson    = 0;
	ChessGame gLearnGame;
	// Watch: the house plays itself while nobody minds the shop
	ChessGame gWatchGame;
	UINT32 guiWatchNextMove = 0;
	UINT8  gubWatchFrom = ChessGame::NO_SQUARE;
	UINT8  gubWatchTo   = ChessGame::NO_SQUARE;
	UINT32 guiWatchSeed = 7;
	int    giWatchResult = 0;   // 0 running, else display-result pause
	// who is seated at the exhibition: two accounts from the site's regulars
	struct ChessSeat { ProfileID pid; const char* handle; int rating; };
	static const ChessSeat CHESS_SEATS[6] =
	{
		{ IVAN,   "@ivan_d",    2145 },
		{ BUNS,   "@buns",      1994 },
		{ SCOPE,  "@scope",     1873 },
		{ FOX,    "@foxtrot",   1731 },
		{ IGOR,   "@igor_k",    1677 },
		{ SPIDER, "@spider",    1512 },
	};
	int gWatchSeat[2] = { 0, 1 };            // [0] plays White, [1] Black
	SGPVSurface* gWatchFaceHalf[2] = { nullptr, nullptr };
	std::vector<ST::string> gWatchSan;       // the exhibition's move list, SAN
	std::vector<ST::string> gPlaySan;        // yours
	// Play: a live game against the proprietor. You are White; he is not in a
	// hurry.
	ChessGame gPlayGame;
	int    giPlayState   = 0;   // 0 your move, 1 he is thinking, 2 over
	UINT32 guiPlayDue    = 0;
	UINT8  gubPlayFrom   = ChessGame::NO_SQUARE;
	UINT8  gubPlayTo     = ChessGame::NO_SQUARE;
	UINT32 guiPlaySeed   = 3;
	int    giPlaySaid    = 0;   // a CHS_PLAY_* status id
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
	static_assert(CHESS_FLAG_SIGNED     == ChessDaily::FLAG_SIGNED,     "flag drift");
	static_assert(CHESS_FLAG_CROWN      == ChessDaily::FLAG_CROWN_ASKED, "flag drift");

	SGPVObject* guiChessPieces = nullptr;      // 24 frames, 34x34 (12 live, 12 dim)
	SGPVObject* guiChessPiecesSmall = nullptr; // 24 frames, 20x20, for diagrams
	SGPVObject* guiChessCoach  = nullptr;  // Grunty, 29x33
	SGPVObject* guiChessIcons  = nullptr;  // 7 nav and panel icons, 14x14
	SGPVObject* guiChessLogo   = nullptr;  // green pawn, 22 and 14
	SGPVObject* guiChessBanner = nullptr;  // 3 ad slots, 272x30
	SGPVObject* guiChessSelf   = nullptr;  // the player's I.M.P. portrait
	SGPVSurface* guiChessSelfHalf  = nullptr;  // 16bpp bakes for half-size rows
	SGPVSurface* guiChessCoachHalf = nullptr;
	ST::string  gChessSelfNick;

	// which campaign day is on screen; past days are archive, view only
	int giChessViewDay = 1;

	MOUSE_REGION gChessSquare[64];
	MOUSE_REGION gChessHintRegion;
	MOUSE_REGION gChessNavRegion[5];
	MOUSE_REGION gChessBannerRegion;
	MOUSE_REGION gChessSignRegion;
	MOUSE_REGION gChessLangRegion;
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
		CHS_HINT, CHS_FOOTER,
		CHS_NAV_PLAY, CHS_NAV_PUZZLES, CHS_NAV_LEARN, CHS_NAV_WATCH, CHS_NAV_COMMUNITY,
		CHS_ST_WHITE, CHS_ST_BLACK, CHS_ST_HINT,
		CHS_ST_ALREADY, CHS_ST_OUT, CHS_ST_DONE, CHS_ST_YOUR_MOVE, CHS_ST_ARCHIVE,
		CHS_MODAL_PERFECT, CHS_MODAL_SOLVED, CHS_MODAL_FAILED, CHS_MODAL_ARCHIVE,
		CHS_MODAL_STREAK, CHS_MODAL_BEST,
		CHS_DOWN_TITLE, CHS_DOWN_1, CHS_DOWN_2, CHS_DOWN_3,
		CHS_STUB_TITLE, CHS_STUB_PLAY, CHS_STUB_LEARN, CHS_STUB_WATCH,
		CHS_STUB_GROUPS, CHS_STUB_BACK, CHS_VISITOR,
		CHS_GB_TITLE, CHS_GB_PROMPT, CHS_GB_SIGN, CHS_GB_YOURS,
		CHS_LEARN_PAGE, CHS_WATCH_LIVE, CHS_WATCH_OVER,
		CHS_PLAY_OPP, CHS_PLAY_YOUR, CHS_PLAY_THINK, CHS_PLAY_WIN,
		CHS_PLAY_LOSS, CHS_PLAY_DRAW, CHS_PLAY_NEW, CHS_LEARN_NEXT,
		CHS_COUNT
	};

	const char* const CHESS_TEXT[2][CHS_COUNT] =
	{
		{
			"DAILY PUZZLE", "DAY", "RATING", "WHITE TO MOVE", "BLACK TO MOVE",
			"HINT", "best viewed at 800x600 - solution tomorrow",
			"Play", "Puzzles", "Learn", "Watch", "Groups",
			"white to move.", "black to move.", "this piece. the rest is yours.",
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
			"GUESTBOOK", "sign it. everyone signs it.", "SIGN ZE BOOK",
			"was here. solved some.",
			"LESSON {} OF 3", "LIVE - ze house plays itself",
			"game over. ze next one starts alone.",
			"GRUNTY - 1850", "your move.", "he is thinking. he does zis.",
			"you beat ze proprietor.", "ze proprietor wins. again.",
			"a draw. nobody is happy.", "NEW GAME", "NEXT LESSON",
		},
		{
			"TAGESRAETSEL", "TAG", "WERTUNG", "WEISS ZIEHT", "SCHWARZ ZIEHT",
			"TIPP", "Beste Ansicht 800x600 - Loesung morgen",
			"Spielen", "Raetsel", "Lernen", "Zusehen", "Forum",
			"Weiss ist am Zug.", "Schwarz ist am Zug.", "diese Figur. der Rest ist Ihrer.",
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
			"GAESTEBUCH", "tragen Sie sich ein. jeder tut es.", "INS BUCH",
			"war hier. hat einiges geloest.",
			"LEKTION {} VON 3", "LIVE - das Haus spielt gegen sich",
			"Partie vorbei. die naechste beginnt allein.",
			"GRUNTY - 1850", "Sie sind am Zug.", "er denkt. das macht er so.",
			"Sie haben den Betreiber geschlagen.", "der Betreiber gewinnt. wieder.",
			"Remis. niemand ist gluecklich.", "NEUE PARTIE", "NAECHSTE LEKTION",
		},
	};

	bool gfChessGerman = false;

	// The guestbook: cast regulars, and the accidental traffic a typo domain
	// earns. Entries are user content, so they stay in whatever language
	// their author typed.
	struct ChessGuestEntry { const char* handle; const char* line; };
	const ChessGuestEntry CHESS_GUESTBOOK[] =
	{
		{ "@grunty",       "please sign properly. no links. no recipes." },
		{ "@ivan_d",       "good puzzles. no nonsense." },
		{ "@e11iot",       "the rook one made me feel things. day 4 i think" },
		{ "@dorothy_1938", "is this the crochet ring?? the button said NEXT SITE" },
		{ "@no_refunds",   "nice traffic numbers. ever consider sponsorship? call me" },
		{ "@chachtourism", "we are the OFFICIAL page of Chach, Slovakia. stop e-mailing us" },
		{ "@the_house",    "we also run games. ours pay out. mostly to us." },
	};

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

	// Bake a face into a 16bpp surface so BltVideoSurfaceHalf can shrink it to
	// row size - the mahjong chat's mini-avatar trick.
	SGPVSurface* ChessBakeFace(SGPVObject* face)
	{
		if (!face) return nullptr;
		const ETRLEObject& e = face->SubregionProperties(0);
		SGPVSurface* surf = AddVideoSurface(e.usWidth, e.usHeight, PIXEL_DEPTH);
		surf->Fill(Get16BPPColor(CH_RGB_CHROME));
		BltVideoObject(surf, face, 0, 0, 0);
		return surf;
	}

	// A fresh exhibition: new seats, a clean move list, and the game already a
	// few moves in the way a live table should be.
	void ChessWatchNewGame()
	{
		guiWatchSeed = guiWatchSeed * 1103515245u + 12345u;
		gWatchSeat[0] = int((guiWatchSeed >> 16) % 6);
		gWatchSeat[1] = (gWatchSeat[0] + 1 + int((guiWatchSeed >> 8) % 5)) % 6;
		for (int i = 0; i < 2; ++i)
		{
			if (gWatchFaceHalf[i]) { DeleteVideoSurface(gWatchFaceHalf[i]); gWatchFaceHalf[i] = nullptr; }
			try
			{
				SGPVObject* face = Load33Portrait(GetProfile(CHESS_SEATS[gWatchSeat[i]].pid));
				gWatchFaceHalf[i] = ChessBakeFace(face);
				DeleteVideoObject(face);
			}
			catch (...)
			{
				// no portrait: the row shows the handle alone
			}
		}

		gWatchGame.SetStartPosition();
		gWatchSan.clear();
		gubWatchFrom = gubWatchTo = ChessGame::NO_SQUARE;
		giWatchResult = 0;
		for (int i = 0; i < 6; ++i)
		{
			const ChessGame::Move m = gWatchGame.Search(2, 20, guiWatchSeed);
			if (m.IsNull()) break;
			gWatchSan.push_back(gWatchGame.San(m));
			gubWatchFrom = m.from; gubWatchTo = m.to;
			gWatchGame.MakeMove(m);
		}
	}

	ChessGame& ChessActiveGame()          { return giChessStub == 0 ? gPlayGame : gChessGame; }
	ChessGame::Color ChessActiveSolver()  { return giChessStub == 0 ? ChessGame::White : gChessSolver; }
	UINT8 ChessActiveFrom()               { return giChessStub == 0 ? gubPlayFrom : gubChessLastFrom; }
	UINT8 ChessActiveTo()                 { return giChessStub == 0 ? gubPlayTo : gubChessLastTo; }

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
	// The board serves two games: the daily puzzle and the live one. These
	// pick whichever the open section owns.
	ChessGame& ChessActiveGame();
	ChessGame::Color ChessActiveSolver();
	UINT8 ChessActiveFrom();
	UINT8 ChessActiveTo();

	// Which way is up: the puzzle rotates to its solver; every other view -
	// Play, Watch, Learn - shows White at the bottom.
	bool ChessWhiteUp()
	{
		return giChessStub >= 0 || gChessSolver == ChessGame::White;
	}

	void SquareToScreen(UINT8 sq, INT32& x, INT32& y)
	{
		const int file = ChessGame::FileOf(sq);
		const int rank = ChessGame::RankOf(sq);
		const int col  = ChessWhiteUp() ? file : 7 - file;
		const int row  = ChessWhiteUp() ? 7 - rank : rank;
		x = CH_BOARD_X + col * CH_SQ;
		y = CH_BOARD_Y + row * CH_SQ;
	}

	UINT8 ScreenToSquare(int col, int row)
	{
		const int file = ChessWhiteUp() ? col : 7 - col;
		const int rank = ChessWhiteUp() ? 7 - row : row;
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
	void ChessMail(int kind, int data)
	{
		AddStrategicEvent(EVENT_CHESS_GRUNTY_EMAIL,
		                  GetWorldTotalMin() + 90 + Random(240),
		                  (UINT32(kind) << 16) | UINT32(data & 0xFFFF));
	}

	// The game report runs overnight on his one machine, so it arrives next
	// morning, between eight and ten.
	void ChessQueueReview(bool solved)
	{
		UINT32 const now = GetWorldTotalMin();
		UINT32 const due = ((now / 1440) + 1) * 1440 + 480 + Random(120);
		UINT32 const data = UINT32(giChessPuzzle & 0x1FF) |
		                    (UINT32(gChessDay.hearts & 0x7) << 9) |
		                    (solved ? 0x1000u : 0u);
		AddStrategicEvent(EVENT_CHESS_GRUNTY_EMAIL, due, (4u << 16) | data);
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
		ChessPlay(CH_SND_CASTLE);
		ChessQueueReview(true);
		// a week-long run is worth a line in the campaign record
		if (gChessDay.streak == 7)
		{
			AddHistoryToPlayersLog(HISTORY_CHESS_STREAK_WEEK, 0, GetWorldTotalMin(), SGPSector());
		}
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
		ChessQueueReview(false);
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

	void ChessPlayFinish()
	{
		switch (gPlayGame.GetResult())
		{
			case ChessGame::Result::Ongoing:    return;
			case ChessGame::Result::WhiteMates: giPlaySaid = CHS_PLAY_WIN;  break;
			case ChessGame::Result::BlackMates: giPlaySaid = CHS_PLAY_LOSS; break;
			default:                            giPlaySaid = CHS_PLAY_DRAW; break;
		}
		giPlayState = 2;
	}

	// Your move in the live game: any legal move goes, promotion takes the
	// queen without asking, and he answers on his own clock.
	void ChessPlayTryMove(UINT8 from, UINT8 to)
	{
		ChessGame::Move moves[ChessGame::MAX_MOVES];
		const int count = gPlayGame.GenerateLegal(moves);
		const ChessGame::Move* pick = nullptr;
		for (int i = 0; i < count; ++i)
		{
			if (moves[i].from != from || moves[i].to != to) continue;
			if (moves[i].promo == ChessGame::NoPiece || moves[i].promo == ChessGame::Queen)
			{
				pick = &moves[i];
				break;
			}
		}
		gbChessSelected = -1;
		if (!pick) return;

		const ChessGame::Move m = *pick;
		gPlaySan.push_back(gPlayGame.San(m));
		gubPlayFrom = m.from; gubPlayTo = m.to;
		gPlayGame.MakeMove(m);
		ChessPlay(ChessMoveSound(m, gPlayGame.IsInCheck(gPlayGame.SideToMove()), true));
		ChessPlayFinish();
		if (giPlayState != 2)
		{
			giPlayState = 1;
			giPlaySaid  = CHS_PLAY_THINK;
			guiPlayDue  = ChessNow() + 700 + Random(1400);
		}
	}

	void ChessPlayNewGame()
	{
		gPlayGame.SetStartPosition();
		gPlaySan.clear();
		giPlayState = 0;
		giPlaySaid  = CHS_PLAY_YOUR;
		guiPlayDue  = 0;
		gubPlayFrom = gubPlayTo = ChessGame::NO_SQUARE;
		gbChessSelected = -1;
	}

	void ChessSquareCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_DWN)) return;
		if (gfChessModal) return;
		const bool playMode = giChessStub == 0;
		if (!playMode && giChessStub >= 0) return;
		if (playMode)
		{
			if (giPlayState != 0) return;
		}
		else
		{
			if (gChessState != CHUI_PUZZLE || guiChessReplyDue != 0) return;
		}

		const UINT8 sq = UINT8(MSYS_GetRegionUserData(region, 0));
		ChessGame& game = ChessActiveGame();
		const bool ours = !game.IsEmpty(sq) && game.ColorAt(sq) == ChessActiveSolver();

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
			if (playMode) ChessPlayTryMove(UINT8(gbChessSelected), sq);
			else          ChessTryMove(UINT8(gbChessSelected), sq);
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
		ChessGame& game = ChessActiveGame();
		if (!game.IsEmpty(to) && game.ColorAt(to) == ChessActiveSolver())
		{
			gbChessSelected = INT8(to);  // dropped on another of our own men
		}
		else if (giChessStub == 0)
		{
			ChessPlayTryMove(from, to);
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
		if (giChessStub == 0)
		{
			ChessPlay(CH_SND_CLICK);
			ChessPlayNewGame();
			ChessRedraw();
			return;
		}
		if (giChessStub == 2)
		{
			giChessLesson = (giChessLesson + 1) % 3;
			gLearnGame.SetFen(CHESS_LESSONS[giChessLesson].fen);
			ChessPlay(CH_SND_CLICK);
			ChessRedraw();
			return;
		}
		if (giChessStub >= 0) return;
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
		if (want == 0 && guiPlayDue == 0 && gubPlayFrom == ChessGame::NO_SQUARE &&
		    giPlayState == 0)
		{
			ChessPlayNewGame();
		}
		if (want == 2)
		{
			gLearnGame.SetFen(CHESS_LESSONS[giChessLesson].fen);
		}
		if (want == 3 && guiWatchNextMove == 0)
		{
			ChessWatchNewGame();
			guiWatchNextMove = ChessNow() + 900;
		}
		ChessRedraw();
	}

	void ChessBannerCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gfChessModal || giChessStub >= 0) return;
		// only the crown creative answers, and he answers exactly once
		if (giChessViewDay % 3 != 1) return;
		if (gChessDay.flags & ChessDaily::FLAG_CROWN_ASKED) return;
		gChessDay.flags |= ChessDaily::FLAG_CROWN_ASKED;
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		ChessMail(5, 0);
	}

	void ChessSignCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giChessStub != 4) return;
		if (gChessDay.flags & ChessDaily::FLAG_SIGNED) return;
		gChessDay.flags |= ChessDaily::FLAG_SIGNED;
		ChessPlay(CH_SND_CLICK);
		ChessRedraw();
	}

	void ChessLangCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		gfChessGerman = !gfChessGerman;
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
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

		MSYS_DefineRegion(&gChessBannerRegion,
		                  UINT16(CH_X(CH_BOARD_X)), UINT16(CH_Y(CH_BANNER_Y)),
		                  UINT16(CH_X(CH_BOARD_X + CH_BOARD_SIZE)), UINT16(CH_Y(CH_BANNER_Y + CH_BANNER_H)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessBannerCallback);
		MSYS_DefineRegion(&gChessSignRegion,
		                  UINT16(CH_X(CH_BOARD_X + 40)), UINT16(CH_Y(CH_BOARD_BOTTOM - 34)),
		                  UINT16(CH_X(CH_BOARD_X + CH_BOARD_SIZE - 40)), UINT16(CH_Y(CH_BOARD_BOTTOM - 12)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessSignCallback);
		MSYS_DefineRegion(&gChessLangRegion,
		                  UINT16(CH_X(CH_BOARD_X + CH_BOARD_SIZE - 36)), UINT16(CH_Y(CH_PAGE_H - 18)),
		                  UINT16(CH_X(CH_BOARD_X + CH_BOARD_SIZE)), UINT16(CH_Y(CH_PAGE_H - 4)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessLangCallback);
		gChessLangRegion.SetFastHelpText("English / Deutsch");

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
		MSYS_RemoveRegion(&gChessBannerRegion);
		MSYS_RemoveRegion(&gChessSignRegion);
		MSYS_RemoveRegion(&gChessLangRegion);
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

	// The board core: squares, coordinates and pieces for any game. The
	// puzzle-only extras (hint square, target dots, the dragged piece) stay in
	// ChessRenderBoard, which wraps this for the daily page.
	void ChessRenderBoardCore(const ChessGame& game, UINT8 lastFrom, UINT8 lastTo,
	                          INT8 selected)
	{
		for (int row = 0; row < 8; ++row)
		{
			for (int col = 0; col < 8; ++col)
			{
				const UINT8 sq = ScreenToSquare(col, row);
				const bool light = IsLightSquare(sq);
				const bool lit = sq == lastFrom || sq == lastTo ||
				                 (selected >= 0 && sq == UINT8(selected));
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
	}

	void ChessRenderBoardCoreLate(const ChessGame& game)
	{
		// Coordinates sit inside the corner squares, in the opposite colour.
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
				if (game.IsEmpty(sq)) continue;
				if (&game == &ChessActiveGame() && gfChessDragging && sq == gubChessDragFrom) continue;
				const UINT8 type = game.PieceAt(sq);
				const UINT16 frame = UINT16((type - 1) +
					(game.ColorAt(sq) == ChessGame::Black ? 6 : 0) +
					(gfChessModal ? 12 : 0));
				BltVideoObject(FRAME_BUFFER, guiChessPieces, frame,
				               CH_X(CH_BOARD_X + col * CH_SQ), CH_Y(CH_BOARD_Y + row * CH_SQ));
			}
		}
	}

	void ChessRenderBoard()
	{
		ChessGame& game = ChessActiveGame();
		ChessRenderBoardCore(game, ChessActiveFrom(), ChessActiveTo(), gbChessSelected);

		// the hint marks the piece that has to move, nothing more
		if (giChessStub < 0 && gfChessHintShown && guiChessPly < gChessSolution.size())
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
			const int count = game.GenerateLegal(moves);
			for (int i = 0; i < count; ++i)
			{
				if (moves[i].from != UINT8(gbChessSelected)) continue;
				INT32 x, y;
				SquareToScreen(moves[i].to, x, y);
				const UINT32 dot = IsLightSquare(moves[i].to) ? CH_RGB_DOT_LIGHT : CH_RGB_DOT_DARK;
				if (game.IsEmpty(moves[i].to))
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

		ChessRenderBoardCoreLate(game);

		// the lifted piece, centred on the pointer. Mouse coords are already
		// screen space, so these do not go through CH_X/CH_Y.
		if (gfChessDragging && !game.IsEmpty(gubChessDragFrom))
		{
			const UINT8 type = game.PieceAt(gubChessDragFrom);
			const UINT16 frame = UINT16((type - 1) +
				(game.ColorAt(gubChessDragFrom) == ChessGame::Black ? 6 : 0));
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

	void ChessRenderMoveList(const std::vector<ST::string>& san, INT32 y0, INT32 y1);

	// The right panel while a live game runs: the opponent, the state of
	// play, and the one button.
	void ChessRenderPlayPanel()
	{
		FillRounded(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		            CH_RGB_PANEL, CH_RADIUS, CH_RGB_CHROME);
		const INT32 cx = CH_PANEL_X + CH_PANEL_W / 2;

		FillRect(CH_PANEL_X, CH_INSET, CH_PANEL_W, 66, CH_RGB_PANEL_SUNK);
		const ST::string title = T(CHS_NAV_PLAY);
		const INT32 titleW = ChessIconLabelWidth(FONT10ARIALBOLD, title);
		ChessIconLabel(CH_ICON_PLAY, cx - titleW / 2, 21,
		               FONT10ARIALBOLD, FONT_MCOLOR_WHITE, title);

		// the opponent: his portrait and his invented rating
		const INT32 faceX = CH_PANEL_X + 8;
		if (guiChessCoach)
		{
			BltVideoObject(FRAME_BUFFER, guiChessCoach, 0, CH_X(faceX), CH_Y(CH_COACH_Y));
		}
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, faceX + 34, CH_COACH_Y + 4, T(CHS_PLAY_OPP));

		const UINT8 colour = giPlaySaid == CHS_PLAY_WIN  ? FONT_MCOLOR_LTGREEN
		                   : giPlaySaid == CHS_PLAY_LOSS ? FONT_MCOLOR_LTRED
		                                                 : FONT_GRAY2;
		PrintAt(FONT10ARIAL, colour, CH_PANEL_X + 10, CH_COACH_Y + 44, T(ChessStr(giPlaySaid)));

		ChessRenderMoveList(gPlaySan, CH_COACH_Y + 62, CH_HINT_Y - 6);

		// the hint button doubles as NEW GAME here; same box, same region
		FillRounded(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20, CH_HINT_H,
		            CH_RGB_CTA, 3, CH_RGB_PANEL);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx,
		             CH_HINT_Y + (CH_HINT_H - GetFontHeight(FONT10ARIALBOLD)) / 2,
		             T(CHS_PLAY_NEW));
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

	// One player row: half-size avatar, handle, rating - above or below the
	// board, as the live site lays a match out.
	void ChessRenderPlayerRow(SGPVSurface* face, const ST::string& name, INT32 y)
	{
		if (face)
		{
			BltVideoSurfaceHalf(FRAME_BUFFER, face, CH_X(CH_BOARD_X), CH_Y(y), NULL);
		}
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, CH_BOARD_X + 20, y + 4, name);
	}

	// The last stretch of a move list, numbered SAN pairs, newest at the foot.
	void ChessRenderMoveList(const std::vector<ST::string>& san, INT32 y0, INT32 y1)
	{
		const int pairs = int(san.size() + 1) / 2;
		const int fit   = (y1 - y0) / 12;
		const int first = pairs > fit ? pairs - fit : 0;
		INT32 y = y0;
		for (int pn = first; pn < pairs; ++pn)
		{
			PrintAt(FONT10ARIAL, FONT_GRAY7, CH_PANEL_X + 10, y, ST::format("{}.", pn + 1));
			PrintAt(FONT10ARIAL, FONT_GRAY2, CH_PANEL_X + 32, y, san[pn * 2]);
			if (pn * 2 + 1 < int(san.size()))
			{
				PrintAt(FONT10ARIAL, FONT_GRAY2, CH_PANEL_X + 86, y, san[pn * 2 + 1]);
			}
			y += 12;
		}
	}

	// A right panel scaffold shared by the live views: rounded ground and a
	// sunk header band carrying the section's icon and name.
	INT32 ChessRenderSectionPanel(UINT16 icon, ChessStr title)
	{
		FillRounded(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		            CH_RGB_PANEL, CH_RADIUS, CH_RGB_CHROME);
		FillRect(CH_PANEL_X, CH_INSET, CH_PANEL_W, 34, CH_RGB_PANEL_SUNK);
		const INT32 cx = CH_PANEL_X + CH_PANEL_W / 2;
		const ST::string text = T(title);
		const INT32 w = ChessIconLabelWidth(FONT10ARIALBOLD, text);
		ChessIconLabel(icon, cx - w / 2, 21, FONT10ARIALBOLD, FONT_MCOLOR_WHITE, text);
		return cx;
	}

	// Learn: the lesson position occupies the full board; the teaching happens
	// in the sidebar, where the coach explains it from her bubble.
	void ChessRenderLearn()
	{
		ChessRenderBoardCore(gLearnGame, ChessGame::NO_SQUARE, ChessGame::NO_SQUARE, -1);
		ChessRenderBoardCoreLate(gLearnGame);
	}

	void ChessRenderLearnPanel()
	{
		const INT32 cx = ChessRenderSectionPanel(CH_ICON_LEARN, CHS_NAV_LEARN);
		const ChessLesson& lesson = CHESS_LESSONS[giChessLesson];

		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, 48, lesson.title);
		PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, 62,
		             ST::format(T(CHS_LEARN_PAGE), giChessLesson + 1));

		// the coach carries the first line; the rest follows as body text
		const INT32 faceX = CH_PANEL_X + 4;
		INT32 faceW = 26;
		if (guiChessCoach)
		{
			faceW = guiChessCoach->SubregionProperties(0).usWidth;
			BltVideoObject(FRAME_BUFFER, guiChessCoach, 0, CH_X(faceX), CH_Y(80));
		}
		const INT32 bubbleX = faceX + faceW + 4;
		const INT32 bubbleW = CH_PANEL_X + CH_PANEL_W - 4 - bubbleX;
		FillRounded(bubbleX, 80, bubbleW, CH_COACH_TILE, CH_RGB_BUBBLE, 3, CH_RGB_PANEL);
		for (int i = 0; i < 4; ++i)
		{
			const INT32 h = 8 - 2 * i;
			if (h <= 0) break;
			FillRect(bubbleX - 1 - i, 93 - h / 2, 1, h, CH_RGB_BUBBLE);
		}
		DisplayWrappedString(UINT16(CH_X(bubbleX + 4)), UINT16(CH_Y(85)),
		                     UINT16(bubbleW - 8), 1, FONT10ARIAL, FONT_MCOLOR_BLACK,
		                     lesson.lines[0], FONT_MCOLOR_WHITE, LEFT_JUSTIFIED);

		DisplayWrappedString(UINT16(CH_X(CH_PANEL_X + 10)), UINT16(CH_Y(128)),
		                     UINT16(CH_PANEL_W - 20), 2, FONT10ARIAL, FONT_GRAY2,
		                     ST::format("{} {}", lesson.lines[1], lesson.lines[2]),
		                     FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		// NEXT LESSON in the CTA green, on the shared button box
		FillRounded(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20, CH_HINT_H,
		            CH_RGB_CTA, 3, CH_RGB_PANEL);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx,
		             CH_HINT_Y + (CH_HINT_H - GetFontHeight(FONT10ARIALBOLD)) / 2,
		             T(CHS_LEARN_NEXT));
	}

	// Watch: the exhibition board between its two player rows, the move list
	// in the sidebar.
	void ChessRenderWatch()
	{
		ChessRenderBoardCore(gWatchGame, gubWatchFrom, gubWatchTo, -1);
		ChessRenderBoardCoreLate(gWatchGame);

		const ChessSeat& white = CHESS_SEATS[gWatchSeat[0]];
		const ChessSeat& black = CHESS_SEATS[gWatchSeat[1]];
		ChessRenderPlayerRow(gWatchFaceHalf[1],
		                     ST::format("{} ({})", black.handle, black.rating), CH_ROW_TOP_Y);
		ChessRenderPlayerRow(gWatchFaceHalf[0],
		                     ST::format("{} ({})", white.handle, white.rating), CH_ROW_BOT_Y);

		// the LIVE chip rides the top row's right end
		if (!giWatchResult)
		{
			FillRounded(CH_BOARD_X + CH_BOARD_SIZE - 40, CH_ROW_TOP_Y + 4, 8, 8,
			            FROMRGB(196, 36, 36), 2, CH_RGB_CHROME);
		}
		PrintAt(FONT10ARIAL, giWatchResult ? FONT_GRAY4 : FONT_MCOLOR_WHITE,
		        CH_BOARD_X + CH_BOARD_SIZE - 28, CH_ROW_TOP_Y + 3,
		        giWatchResult ? "END" : "LIVE");
	}

	void ChessRenderWatchPanel()
	{
		ChessRenderSectionPanel(CH_ICON_WATCH, CHS_NAV_WATCH);
		PrintCentred(FONT10ARIAL, FONT_GRAY4, CH_PANEL_X + CH_PANEL_W / 2, 48,
		             T(giWatchResult ? CHS_WATCH_OVER : CHS_WATCH_LIVE));
		ChessRenderMoveList(gWatchSan, 66, CH_PAGE_H - CH_INSET - 8);
	}

	// The guestbook. Sign once; the signature is forever.
	void ChessRenderGuestbook()
	{
		FillRounded(CH_BOARD_X, CH_BOARD_Y, CH_BOARD_SIZE, CH_BOARD_SIZE,
		            CH_RGB_PANEL, CH_RADIUS, CH_RGB_CHROME);
		const INT32 cx = CH_BOARD_X + CH_BOARD_SIZE / 2;

		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, CH_BOARD_Y + 10, T(CHS_GB_TITLE));
		PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, CH_BOARD_Y + 24, T(CHS_GB_PROMPT));

		const int shown = int(sizeof(CHESS_GUESTBOOK) / sizeof(CHESS_GUESTBOOK[0]));
		INT32 y = CH_BOARD_Y + 44;
		for (int i = 0; i < shown; ++i)
		{
			FillRect(CH_BOARD_X + 8, y - 2, CH_BOARD_SIZE - 16, 24, CH_RGB_PANEL_SUNK);
			PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_LTGREEN, CH_BOARD_X + 14, y, CHESS_GUESTBOOK[i].handle);
			PrintAt(FONT10ARIAL, FONT_GRAY2, CH_BOARD_X + 14, y + 11, CHESS_GUESTBOOK[i].line);
			y += 26;
		}

		if (gChessDay.flags & ChessDaily::FLAG_SIGNED)
		{
			// your entry, in the site's accent - the one line you control
			FillRect(CH_BOARD_X + 8, y - 2, CH_BOARD_SIZE - 16, 24, CH_RGB_PANEL_SUNK);
			const ST::string handle = gChessSelfNick.empty()
				? ST::string("@commander") : ST::format("@{}", gChessSelfNick);
			PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, CH_BOARD_X + 14, y, handle);
			PrintAt(FONT10ARIAL, FONT_GRAY2, CH_BOARD_X + 14, y + 11, T(CHS_GB_YOURS));
		}
		else
		{
			FillRounded(CH_BOARD_X + 40, CH_BOARD_BOTTOM - 34, CH_BOARD_SIZE - 80, 22,
			            CH_RGB_CTA, 3, CH_RGB_PANEL);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, CH_BOARD_BOTTOM - 28,
			             T(CHS_GB_SIGN));
		}
	}

	void ChessRenderFooter()
	{
		PrintAt(FONT10ARIAL, FONT_GRAY7, CH_BOARD_X, CH_PAGE_H - 16, T(CHS_FOOTER));
		// the language link lives down here now: the proprietor is German and
		// it costs one small footer line
		const INT32 lx = CH_BOARD_X + CH_BOARD_SIZE - 34;
		PrintAt(FONT10ARIAL, gfChessGerman ? FONT_GRAY7 : FONT_MCOLOR_LTGREEN, lx, CH_PAGE_H - 16, "EN");
		PrintAt(FONT10ARIAL, FONT_GRAY7, lx + 14, CH_PAGE_H - 16, "|");
		PrintAt(FONT10ARIAL, gfChessGerman ? FONT_MCOLOR_LTGREEN : FONT_GRAY7, lx + 20, CH_PAGE_H - 16, "DE");
	}
}

// --- laptop page hooks ----------------------------------------------------

void EnterChess()
{
	gChessDay.flags |= ChessDaily::FLAG_DISCOVERED;

	guiChessPieces = nullptr;
	guiChessPiecesSmall = nullptr;
	guiChessCoach  = nullptr;
	guiChessIcons  = nullptr;
	guiChessLogo   = nullptr;
	guiChessBanner = nullptr;
	guiChessSelf   = nullptr;
	giChessStub    = -1;
	gChessSelfNick = ST::string();
	try
	{
		guiChessPieces      = AddVideoObjectFromFile("sti/laptop/chesspieces.sti");
		guiChessPiecesSmall = AddVideoObjectFromFile("sti/laptop/chesspiecessmall.sti");
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
	guiChessCoachHalf = ChessBakeFace(guiChessCoach);

	// the account block shows your own I.M.P. character as the site avatar
	if (LaptopSaveInfo.fIMPCompletedFlag)
	{
		try
		{
			MERCPROFILESTRUCT const& imp = GetProfile(
				static_cast<ProfileID>(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId));
			guiChessSelf     = Load33Portrait(imp);
			guiChessSelfHalf = ChessBakeFace(guiChessSelf);
			gChessSelfNick   = imp.zNickname;
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
	if (guiChessPiecesSmall) { DeleteVideoObject(guiChessPiecesSmall); guiChessPiecesSmall = nullptr; }
	if (guiChessCoach)  { DeleteVideoObject(guiChessCoach);  guiChessCoach  = nullptr; }
	if (guiChessIcons)  { DeleteVideoObject(guiChessIcons);  guiChessIcons  = nullptr; }
	if (guiChessLogo)   { DeleteVideoObject(guiChessLogo);   guiChessLogo   = nullptr; }
	if (guiChessBanner) { DeleteVideoObject(guiChessBanner); guiChessBanner = nullptr; }
	if (guiChessSelf)   { DeleteVideoObject(guiChessSelf);   guiChessSelf   = nullptr; }
	if (guiChessSelfHalf)  { DeleteVideoSurface(guiChessSelfHalf);  guiChessSelfHalf  = nullptr; }
	if (guiChessCoachHalf) { DeleteVideoSurface(guiChessCoachHalf); guiChessCoachHalf = nullptr; }
	for (int i = 0; i < 2; ++i)
	{
		if (gWatchFaceHalf[i]) { DeleteVideoSurface(gWatchFaceHalf[i]); gWatchFaceHalf[i] = nullptr; }
	}
}

void RenderChess()
{
	FillRect(0, 0, LAPTOP_SCREEN_WIDTH, CH_PAGE_H, CH_RGB_CHROME);
	ChessRenderNav();
	if (giChessStub == 4)
	{
		ChessRenderGuestbook();
		ChessRenderBanner();
		ChessRenderFooter();
	}
	else if (giChessStub == 2)
	{
		ChessRenderLearn();
		ChessRenderBanner();
		ChessRenderLearnPanel();
		ChessRenderFooter();
	}
	else if (giChessStub == 3)
	{
		ChessRenderWatch();
		ChessRenderBanner();
		ChessRenderWatchPanel();
		ChessRenderFooter();
	}
	else if (giChessStub == 0)
	{
		ChessRenderBoard();
		ChessRenderPlayerRow(guiChessCoachHalf, T(CHS_PLAY_OPP), CH_ROW_TOP_Y);
		ChessRenderPlayerRow(guiChessSelfHalf,
		                     gChessSelfNick.empty() ? ST::string("@you")
		                                            : ST::format("@{}", gChessSelfNick),
		                     CH_ROW_BOT_Y);
		ChessRenderBanner();
		ChessRenderPlayPanel();
		ChessRenderFooter();
	}
	else if (giChessStub >= 0)
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
	// his move in the live game lands on his own clock
	if (giChessStub == 0 && giPlayState == 1 && ChessNow() >= guiPlayDue)
	{
		const ChessGame::Move m = gPlayGame.Search(3, 8, guiPlaySeed);
		if (!m.IsNull())
		{
			gPlaySan.push_back(gPlayGame.San(m));
			gubPlayFrom = m.from; gubPlayTo = m.to;
			gPlayGame.MakeMove(m);
			ChessPlay(ChessMoveSound(m, gPlayGame.IsInCheck(gPlayGame.SideToMove()), false));
		}
		giPlayState = 0;
		giPlaySaid  = CHS_PLAY_YOUR;
		ChessPlayFinish();
		ChessRedraw();
	}

	// the exhibition ticks while it is on screen
	if (giChessStub == 3 && ChessNow() >= guiWatchNextMove)
	{
		if (giWatchResult)
		{
			// the result hangs on screen a moment, then a fresh game starts
			ChessWatchNewGame();
		}
		else
		{
			const ChessGame::Move m = gWatchGame.Search(2, 15, guiWatchSeed);
			if (!m.IsNull())
			{
				gWatchSan.push_back(gWatchGame.San(m));
				gubWatchFrom = m.from; gubWatchTo = m.to;
				gWatchGame.MakeMove(m);
				ChessPlay(ChessMoveSound(m, gWatchGame.IsInCheck(gWatchGame.SideToMove()), false),
				          LOWVOLUME);
			}
			if (m.IsNull() || gWatchGame.GetResult() != ChessGame::Result::Ongoing)
			{
				giWatchResult = 1;
			}
		}
		guiWatchNextMove = ChessNow() + (giWatchResult ? 3600 : 1100 + Random(700));
		ChessRedraw();
	}

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
