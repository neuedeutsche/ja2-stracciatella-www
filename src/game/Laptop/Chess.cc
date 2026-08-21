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

#include <SDL_keycode.h>
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
#include <cstring>
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
#define CH_INSET        8
#define CH_RADIUS       4
// the side panels round more generously than the board
#define CH_PANEL_RADIUS 8
// The rail is the one exception: it runs the full height of the page and sits
// flush to the left edge, so no chrome shows around it.
#define CH_NAV_X        0
#define CH_NAV_W        70
#define CH_SQ           34
#define CH_BOARD_X      76
#define CH_BOARD_Y      40
#define CH_BOARD_SIZE   (8 * CH_SQ)
#define CH_BOARD_BOTTOM (CH_BOARD_Y + CH_BOARD_SIZE)
#define CH_PANEL_X      352
#define CH_PANEL_W      142
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
#define CH_FOOT_Y       (CH_PAGE_H - CH_INSET - 34)
#define CH_HINT_Y       (CH_FOOT_Y + 6)
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
// player rows for the live views, above and below the board. The faces come
// from the big A.I.M. portraits, cropped square and downsampled a few pixels
// shy of a board cell so they stay crisp.
#define CH_ROW_FACE     30
// guestbook geometry: the book fills the page height, each entry carries an
// avatar, and the sidebar hosts the skyscraper ad
#define CH_GB_TOP       26
// the composer card, centred over the book
#define CH_GBM_W        260
#define CH_GBM_H        170
#define CH_GBM_X        (CH_BOARD_X + (CH_BOARD_SIZE - CH_GBM_W) / 2)
#define CH_GBM_Y        80
#define CH_GB_FACE      26
#define CH_ROW_TOP_Y    6
#define CH_ROW_BOT_Y    (CH_BOARD_BOTTOM + 3)

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
#define CH_RGB_CHROME      FROMRGB( 58,  53,  47)
#define CH_RGB_PANEL       FROMRGB( 36,  33,  30)
#define CH_RGB_PANEL_UP    FROMRGB( 60,  55,  50)
// Sections are separated by a shift in ground tone rather than by rules, and
// the shift goes down: a sunk band, never a brighter one.
#define CH_RGB_PANEL_SUNK  FROMRGB( 29,  26,  24)
// the guestbook's row separator: a hairline, read more than seen
#define CH_RGB_ROW_SEP     FROMRGB( 48,  44,  40)
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
	// Each regular plays at their own strength: search depth, blunder rate,
	// and how often they grab a capture just because it is there. The ratings
	// are invented, but they rank the same way the parameters do.
	struct ChessSeat
	{
		ProfileID   pid;
		const char* handle;
		const char* title;  // "IM"-style chess title; "" for the untitled
		int         rating;
		int         depth;
		int         err;    // percent of moves played at random
		int         greed;  // percent chance any available capture is taken
	};
	static const ChessSeat CHESS_SEATS[6] =
	{
		{ IVAN,   "@ivan_d",  "IM", 2145, 3,  4, 25 },
		{ BUNS,   "@buns",    "FM", 1994, 3, 10, 40 },
		{ SCOPE,  "@scope",   "CM", 1873, 2,  8, 50 },
		{ FOX,    "@foxtrot", "",   1731, 2, 16, 65 },
		{ IGOR,   "@igor_k",  "",   1677, 2, 24, 70 },
		{ SPIDER, "@spider",  "",   1512, 1, 30, 80 },
	};

	// the guestbook looks a signer's title up by their handle
	const char* ChessTitleForHandle(const ST::string& handle)
	{
		for (const ChessSeat& seat : CHESS_SEATS)
		{
			if (handle == seat.handle && seat.title[0]) return seat.title;
		}
		return nullptr;
	}
	int gWatchSeat[2] = { 0, 1 };            // [0] plays White, [1] Black
	SGPVSurface* gWatchFaceHalf[2] = { nullptr, nullptr };
	std::vector<ST::string> gWatchSan;       // the exhibition's move list, SAN
	std::vector<ST::string> gPlaySan;        // yours
	// Play: a live game against the proprietor. You are White; he is not in a
	// hurry.
	ChessGame gPlayGame;
	int    giPlayState   = 3;   // 0 your move, 1 thinking, 2 over, 3 seeking
	int    giPlaySeat    = -1;  // index into CHESS_SEATS once paired
	UINT32 guiPlaySeekDue = 0;
	SGPVSurface* gPlayOppFace = nullptr;
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
	SGPVObject* guiChessAdDragon = nullptr; // the Parlour's medallion, borrowed for its ad
	SGPVObject* guiChessAdTiles  = nullptr; // and two of its tiles: the ring and the bird
	SGPVObject* guiChessSelf   = nullptr;  // the player's I.M.P. portrait
	SGPVSurface* guiChessSelfHalf  = nullptr;  // 16bpp bakes for half-size rows
	SGPVSurface* guiChessCoachHalf = nullptr;
	ST::string  gChessSelfNick;

	// which campaign day is on screen; past days are archive, view only
	int giChessViewDay = 1;

	// One move animating at a time: the game state has already made the move,
	// so the piece is suppressed on its destination square and drawn gliding
	// between the two centres instead.
	struct ChessAnim
	{
		bool             active = false;
		const ChessGame* game   = nullptr;
		UINT8            from   = ChessGame::NO_SQUARE;
		UINT8            to     = ChessGame::NO_SQUARE;
		UINT16           frame  = 0;
		UINT32           start  = 0;
		UINT32           dur    = 180;
	};
	ChessAnim gChessAnim;
	// a drag lands where the pointer put it; sliding it afterwards reads wrong
	bool gfChessDropMove = false;

	MOUSE_REGION gChessSquare[64];
	MOUSE_REGION gChessHintRegion;
	MOUSE_REGION gChessNavRegion[5];
	MOUSE_REGION gChessBannerRegion;
	MOUSE_REGION gChessAdRegion;
	MOUSE_REGION gChessSignRegion;
	MOUSE_REGION gChessGbPrevRegion;
	MOUSE_REGION gChessGbNextRegion;
	MOUSE_REGION gChessGbPostRegion;
	MOUSE_REGION gChessGbCloseRegion;
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
		CHS_GB_TITLE, CHS_GB_PROMPT, CHS_GB_SIGN, CHS_GB_YOURS, CHS_GB_WRITE,
		CHS_GB_MOD,
		CHS_LEARN_PAGE, CHS_WATCH_LIVE, CHS_WATCH_OVER,
		CHS_PLAY_OPP, CHS_PLAY_YOUR, CHS_PLAY_THINK, CHS_PLAY_WIN,
		CHS_PLAY_LOSS, CHS_PLAY_DRAW, CHS_PLAY_NEW, CHS_LEARN_NEXT, CHS_PLAY_SEEK,
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
			"was here. solved some.", "write in ze book...",
			"ze moderation reads everything.",
			"LESSON {} OF 3", "LIVE - ze house plays itself",
			"game over. ze next one starts alone.",
			"GRUNTY", "your move.", "he is thinking. he does zis.",
			"you beat ze proprietor.", "ze proprietor wins. again.",
			"a draw. nobody is happy.", "NEW GAME", "NEXT LESSON",
			"seeking opponent...",
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
			"war hier. hat einiges geloest.", "schreiben Sie in ze Buch...",
			"ze Moderation liest alles.",
			"LEKTION {} VON 3", "LIVE - das Haus spielt gegen sich",
			"Partie vorbei. die naechste beginnt allein.",
			"GRUNTY", "Sie sind am Zug.", "er denkt. das macht er so.",
			"Sie haben den Betreiber geschlagen.", "der Betreiber gewinnt. wieder.",
			"Remis. niemand ist gluecklich.", "NEUE PARTIE", "NAECHSTE LEKTION",
			"suche Gegner...",
		},
	};

	bool gfChessGerman = false;

	// The guestbook: cast regulars, and the accidental traffic a typo domain
	// earns. Entries are user content, so they stay in whatever language
	// their author typed.
	struct ChessGuestEntry
	{
		const char* handle;
		const char* name;  // fallback display name; a merc's profile overrides
		const char* date;  // when they signed, 1999 style
		UINT8       lines; // wrapped message lines; the row is sized from this
		ProfileID   pid;   // CH_NO_PID: not a merc, the tinted disc stands in
		UINT32      tint;  // disc colour for the portraitless
		const char* line;
	};
	constexpr ProfileID CH_NO_PID = 0xFF;
	// Four pages of five: the regulars, the accidental traffic a typo domain
	// earns, Arulco reading along, and the complaints department. The dates
	// run back into the '80s: ze book predates the site, the web, and one
	// wall - Grunty migrated it from paper and no one asks how.
	const ChessGuestEntry CHESS_GUESTBOOK[] =
	{
		{ "@grunty", "Helmut Grunther", "11/09/1987",        3, GRUNTY,    FROMRGB( 90, 130,  70),
		  "please sign properly. no links. no recipes. ze book is for chess and chess-adjacent feelings only." },
		{ "@ivan_d", "Ivan Dolvich", "03/17/1989",        1, IVAN,      FROMRGB( 70,  90, 140),
		  "good puzzles. no nonsense." },
		{ "@buns", "Mary Beth Wilkens", "06/24/1992",          3, BUNS,      FROMRGB(120, 100,  60),
		  "I show the lessons to my students. the German translation is correct. I checked it twice and found nothing. annoyed." },
		{ "@scope", "Sheila Sterling", "10/02/1995",         3, SCOPE,     FROMRGB( 70, 110, 100),
		  "solved day 12 in one attempt from a moving jeep, laptop balanced on a crate of ammunition. do not tell me about your office." },
		{ "@steroid", "Bobby Gontarski", "04/18/1997",       3, STEROID,   FROMRGB(100,  90, 130),
		  "in Poland we play with no clock and no mercy. site is good. pieces could be more bigger." },
		{ "@dorothy_1938", "Dorothy Cummins", "02/11/1998",  3, CH_NO_PID, FROMRGB(164,  84, 124),
		  "is this the crochet ring?? the button said NEXT SITE. i have been here four days now. the horse one is my favourite." },
		{ "@chachtourism", "Chach Tourism Board", "06/30/1998",  3, CH_NO_PID, FROMRGB( 60, 110, 170),
		  "we are the OFFICIAL page of Chach, Slovakia. we get your e-mails. we do not want your e-mails. stop." },
		{ "@webring_admin", "The WebRing", "09/14/1998", 3, CH_NO_PID, FROMRGB( 90,  90,  90),
		  "removed from the Chess WebRing pending review: excessive load times, suspicious hit counter. reapply in 30 days." },
		{ "@no_refunds", "Marty", "11/23/1998",    2, CH_NO_PID, FROMRGB(190, 120,  40),
		  "nice traffic numbers. ever consider sponsorship? call me" },
		{ "@prague_cc", "Prague Chess Club", "01/07/1999",     3, CH_NO_PID, FROMRGB(140,  70,  60),
		  "you are not affiliated with any federation we recognize. remove the flag or we write again." },
		{ "@the_house", "The House", "01/28/1999",     2, CH_NO_PID, FROMRGB(150,  40,  40),
		  "we also run games. ours pay out. mostly to us. no crocodiles in ours either." },
		{ "@skyrider", "Skyrider", "02/19/1999",      3, SKYRIDER,  FROMRGB( 70, 120, 140),
		  "GOOD reading while WAITING at the landing pad for someone to remember FUEL is not free. hint. hint." },
		{ "@pablo_airport", "Pablo", "03/06/1999", 3, PABLO,     FROMRGB(110, 110,  60),
		  "packages sometimes arrive with pieces missing. this is normal, is customs, is not my fault. good puzzles." },
		{ "@micky_o", "Micky O'Brien", "03/22/1999",       3, MICKY,     FROMRGB(150, 100,  50),
		  "fine site. if anyone here buys crocodile skins, i am in the bar in Estoni until thursday. also elephant." },
		{ "@biff_m", "Biff Apscott", "04/09/1999",        3, BIFF,      FROMRGB( 90, 120,  90),
		  "is there a way to practice being braver before the pieces start taking each other. asking for chess reasons." },
		{ "@igor_k", "Igor Dolvich", "04/17/1999",        2, IGOR,      FROMRGB( 80, 100, 120),
		  "uncle Ivan says the puzzles are good. so they are good." },
		{ "@foxtrot", "Cynthia Guzzman", "04/25/1999",       1, FOX,       FROMRGB(160,  90,  90),
		  "the knight one on day 9 was rude." },
		{ "@spider", "Spider", "05/02/1999",        3, SPIDER,    FROMRGB( 60, 100,  90),
		  "lost to the proprietor eleven games in a row. I am a nurse. I have seen things die slower." },
		{ "@a_free_arulco", "A Patriot", "05/11/1999", 3, CH_NO_PID, FROMRGB( 80, 110,  60),
		  "a liberated Arulco will have a chess club in every town. mark these words, friend. and tell no one you read them." },
		{ "@flo_m", "Flo Malone", "05/19/1999",         3, FLO,       FROMRGB(170, 110, 140),
		  "traded my queen for a horse because the horse looked nicer. no regrets. the site said MISTAKE which felt personal." },
	};
	constexpr size_t CH_GB_COUNT = sizeof(CHESS_GUESTBOOK) / sizeof(CHESS_GUESTBOOK[0]);
	constexpr int CH_GB_PER_PAGE = 5;
	constexpr int CH_GB_PAGES = int((CH_GB_COUNT + CH_GB_PER_PAGE - 1) / CH_GB_PER_PAGE);
	int giChessGbPage = 0;
	MOUSE_REGION gChessGbNumRegion[CH_GB_PAGES];
	// the composer: a modal the sign button opens; what you type is yours,
	// survives the save, and ze moderation reads it either way
	bool gfChessGbCompose = false;
	std::string gChessGbInput;
	std::string gChessGuestLine;  // the posted line; empty means the standard one
	constexpr size_t CH_GB_LINE_MAX = 120;
	SGPVSurface* gGuestFace[CH_GB_COUNT] = {};
	ST::string gGuestName[CH_GB_COUNT];
	SGPVSurface* gGuestSelfFace = nullptr;
	ST::string gChessSelfName;

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

	// Bake a face into a 16bpp surface once, at load, scaled to one board
	// cell (34px tall, nearest neighbour) so the row avatars measure an
	// eighth of the board. The ETRLE data is decoded directly and pushed
	// through the object's own palette; the engine's half blitter cannot be
	// used here - it reads 8bpp sources, which was the Watch crash.
	// The book shows first names only - the full A.I.M. billing is too wide
	// for a row. A short title ("Dr.") drags the given name along with it.
	ST::string ChessFirstName(const ST::string& full)
	{
		const char* z = full.c_str();
		const char* sp = std::strchr(z, ' ');
		if (!sp) return full;
		if (sp - z <= 3 && sp[-1] == '.')
		{
			const char* sp2 = std::strchr(sp + 1, ' ');
			return sp2 ? ST::string(z, sp2 - z) : full;
		}
		return ST::string(z, sp - z);
	}

	// The big A.I.M. portrait when the profile has one; the 33 thumbnail
	// upscales into mush, the big face downsamples cleanly.
	SGPVObject* ChessLoadPortrait(MERCPROFILESTRUCT const& profile)
	{
		try { return LoadBigPortrait(profile); }
		catch (...) { return Load33Portrait(profile); }
	}

	SGPVSurface* ChessBakeFace(SGPVObject* face, INT32 size = CH_SQ)
	{
		if (!face) return nullptr;
		const ETRLEObject& e = face->SubregionProperties(0);
		const INT32 w = e.usWidth, h = e.usHeight;
		if (w <= 0 || h <= 0) return nullptr;

		std::vector<UINT8> pixels(size_t(w) * h, 0);
		const UINT8* in = face->PixData(e);
		for (INT32 y = 0; y < h; ++y)
		{
			INT32 x = 0;
			while (*in != 0)
			{
				const UINT8 code = *in++;
				const UINT8 run  = code & 0x7F;
				if (code & 0x80) x += run;
				else for (UINT8 k = 0; k < run && x < w; ++k) pixels[size_t(y) * w + x++] = *in++;
			}
			++in;  // row terminator
		}

		const UINT16* pal = face->Palette16();
		if (!pal) return nullptr;

		// crop to a square biased toward the top - the big portraits carry
		// shoulders the avatar has no room for - then downsample
		const INT32 side = w < h ? w : h;
		const INT32 sx = (w - side) / 2;
		const INT32 sy = (h - side) / 4;
		SGPVSurface* surf = AddVideoSurface(UINT16(size), UINT16(size), PIXEL_DEPTH);
		surf->Fill(Get16BPPColor(CH_RGB_CHROME));
		{
			SGPVSurface::Lock lock(surf);
			UINT16* out = lock.Buffer<UINT16>();
			const UINT32 pitch = lock.Pitch() / 2;
			for (INT32 y = 0; y < size; ++y)
			{
				for (INT32 x = 0; x < size; ++x)
				{
					const UINT8 v = pixels[size_t(sy + y * side / size) * w
					                       + size_t(sx + x * side / size)];
					if (v) out[y * pitch + x] = pal[v];
				}
			}
		}
		return surf;
	}

	// How the regulars actually play: if something can be taken, it usually
	// is, whether or not taking it is any good. Greed first, thought second.
	ChessGame::Move ChessWatchPickMove()
	{
		const ChessSeat& seat = CHESS_SEATS[
			gWatchSeat[gWatchGame.SideToMove() == ChessGame::White ? 0 : 1]];
		guiWatchSeed = guiWatchSeed * 1103515245u + 12345u;
		ChessGame::Move captures[ChessGame::MAX_MOVES];
		const int nCaps = gWatchGame.GenerateLegalCaptures(captures);
		if (nCaps > 0 && int((guiWatchSeed >> 16) % 100) < seat.greed)
		{
			return captures[(guiWatchSeed >> 8) % unsigned(nCaps)];
		}
		return gWatchGame.Search(seat.depth, seat.err, guiWatchSeed);
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
				SGPVObject* face = ChessLoadPortrait(GetProfile(CHESS_SEATS[gWatchSeat[i]].pid));
				gWatchFaceHalf[i] = ChessBakeFace(face, CH_ROW_FACE);
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
			const ChessGame::Move m = ChessWatchPickMove();
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
	std::memset(p.szLine, 0, sizeof(p.szLine));
	std::strncpy(p.szLine, gChessGuestLine.c_str(), sizeof(p.szLine) - 1);
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
	gChessGuestLine.clear();
	for (size_t i = 0; i < sizeof(p.szLine) && p.szLine[i]; ++i)
	{
		// old saves hold zeroes here; anything unprintable is not ours
		const char c = p.szLine[i];
		if (c >= 32 && c < 127) gChessGuestLine += c;
	}
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

	// Rounded fill that never paints outside its own outline - for cards
	// floating over variegated ground (the dimmed board), where there is no
	// single colour to hand the corners back to.
	void FillRoundedOnly(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb, int radius)
	{
		for (int row = 0; row < radius; ++row)
		{
			const int inset = CornerInset(row, radius);
			FillRect(x + inset, y + row, w - 2 * inset, 1, rgb);
			FillRect(x + inset, y + h - 1 - row, w - 2 * inset, 1, rgb);
		}
		FillRect(x, y + radius, w, h - 2 * radius, rgb);
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

	// Call just after MakeMove: the landed piece slides from its old square.
	void ChessAnimateMove(const ChessGame& game, const ChessGame::Move& m, UINT32 dur)
	{
		if (gfChessDropMove) return;
		gChessAnim.active = true;
		gChessAnim.game   = &game;
		gChessAnim.from   = m.from;
		gChessAnim.to     = m.to;
		gChessAnim.frame  = UINT16((game.PieceAt(m.to) - 1) +
		                           (game.ColorAt(m.to) == ChessGame::Black ? 6 : 0));
		gChessAnim.start  = ChessNow();
		gChessAnim.dur    = dur;
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
			ChessAnimateMove(gChessGame, want, 110);
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
		ChessAnimateMove(gPlayGame, m, 110);
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
		giPlayState = 3;
		giPlaySeat  = -1;
		giPlaySaid  = CHS_PLAY_SEEK;
		guiPlaySeekDue = ChessNow() + 1400 + Random(2000);
		guiPlayDue  = 0;
		gubPlayFrom = gubPlayTo = ChessGame::NO_SQUARE;
		gbChessSelected = -1;
		if (gPlayOppFace) { DeleteVideoSurface(gPlayOppFace); gPlayOppFace = nullptr; }
	}

	// Your opponent plays like themselves: their depth, their blunders, their
	// greed - the same triple the exhibition seats use.
	ChessGame::Move ChessPlayPickMove()
	{
		const ChessSeat& seat = CHESS_SEATS[giPlaySeat < 0 ? 0 : giPlaySeat];
		guiPlaySeed = guiPlaySeed * 1103515245u + 12345u;
		ChessGame::Move captures[ChessGame::MAX_MOVES];
		const int nCaps = gPlayGame.GenerateLegalCaptures(captures);
		if (nCaps > 0 && int((guiPlaySeed >> 16) % 100) < seat.greed)
		{
			return captures[(guiPlaySeed >> 8) % unsigned(nCaps)];
		}
		return gPlayGame.Search(seat.depth, seat.err, guiPlaySeed);
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
			gfChessDropMove = true;
			ChessPlayTryMove(from, to);
			gfChessDropMove = false;
		}
		else
		{
			gfChessDropMove = true;
			ChessTryMove(from, to);
			gfChessDropMove = false;
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

	void ChessSyncPageRegions();

	void ChessNavCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const int item = int(MSYS_GetRegionUserData(region, 0));
		// Puzzles is the site; everything else is a page he has not written
		const int want = (item == 1) ? -1 : item;
		if (want != giChessStub) ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		giChessStub = want;
		gChessHintRegion.SetFastHelpText(want < 0 ? "Costs one attempt" : ST::string());
		ChessSyncPageRegions();
		if (want == 0 && giPlaySeat < 0 && guiPlaySeekDue == 0)
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
		// the guestbook's sidebar ad answers too; the other pages' banner does not
		if (gfChessModal || (giChessStub >= 0 && giChessStub != 4)) return;
		// only the crown creative answers, and he answers exactly once
		if (giChessViewDay % 3 != 1) return;
		if (gChessDay.flags & ChessDaily::FLAG_CROWN_ASKED) return;
		gChessDay.flags |= ChessDaily::FLAG_CROWN_ASKED;
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		ChessMail(5, 0);
	}

	void ChessGbPageCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giChessStub != 4) return;
		const int want = giChessGbPage + int(MSYS_GetRegionUserData(region, 0));
		if (want < 0 || want >= CH_GB_PAGES) return;
		giChessGbPage = want;
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	void ChessGbPageSetCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giChessStub != 4) return;
		const int want = int(MSYS_GetRegionUserData(region, 0));
		if (want == giChessGbPage) return;
		giChessGbPage = want;
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	void ChessSignCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giChessStub != 4) return;
		if (gChessDay.flags & ChessDaily::FLAG_SIGNED) return;
		// the button does not sign; it opens the composer
		gfChessGbCompose = true;
		gChessGbInput.clear();
		ChessSyncPageRegions();
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	void ChessGbPost()
	{
		while (!gChessGbInput.empty() && gChessGbInput.back() == ' ') gChessGbInput.pop_back();
		gChessGuestLine = gChessGbInput;  // empty posts the standard line
		gChessDay.flags |= ChessDaily::FLAG_SIGNED;
		gfChessGbCompose = false;
		giChessGbPage = CH_GB_PAGES - 1;  // the book opens where you signed it
		ChessSyncPageRegions();
		ChessPlay(CH_SND_CLICK);
		ChessRedraw();
	}

	void ChessGbPostCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfChessGbCompose) return;
		ChessGbPost();
	}

	void ChessGbCloseCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfChessGbCompose) return;
		gfChessGbCompose = false;
		ChessSyncPageRegions();
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	// The guestbook trades the bottom banner for the sidebar skyscraper and
	// grows a pager; every mode switch resolves the swap here.
	void ChessSyncPageRegions()
	{
		if (!gfChessRegionsUp) return;
		const bool gb = giChessStub == 4;
		if (gb) gChessBannerRegion.Disable(); else gChessBannerRegion.Enable();
		const bool list = gb && !gfChessGbCompose;
		MOUSE_REGION* book[] = { &gChessAdRegion, &gChessSignRegion,
		                         &gChessGbPrevRegion, &gChessGbNextRegion };
		for (MOUSE_REGION* r : book) { if (list) r->Enable(); else r->Disable(); }
		for (MOUSE_REGION& r : gChessGbNumRegion) { if (list) r.Enable(); else r.Disable(); }
		const bool compose = gb && gfChessGbCompose;
		if (compose) { gChessGbPostRegion.Enable();  gChessGbCloseRegion.Enable(); }
		else         { gChessGbPostRegion.Disable(); gChessGbCloseRegion.Disable(); }
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
		                  UINT16(CH_X(CH_BOARD_X)), UINT16(CH_Y(CH_PAGE_H - CH_INSET - 48)),
		                  UINT16(CH_X(CH_BOARD_X + CH_BOARD_SIZE)), UINT16(CH_Y(CH_PAGE_H - CH_INSET)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessBannerCallback);
		// the guestbook's sidebar skyscraper; sits over the panel furniture, so
		// it only exists while that page is up
		MSYS_DefineRegion(&gChessAdRegion,
		                  UINT16(CH_X(CH_PANEL_X)), UINT16(CH_Y(CH_INSET + 14)),
		                  UINT16(CH_X(CH_PANEL_X + CH_PANEL_W)), UINT16(CH_Y(CH_PAGE_H - CH_INSET)),
		                  MSYS_PRIORITY_HIGH + 1, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessBannerCallback);
		MSYS_DefineRegion(&gChessSignRegion,
		                  UINT16(CH_X(CH_BOARD_X + 8)), UINT16(CH_Y(CH_PAGE_H - CH_INSET - 30)),
		                  UINT16(CH_X(CH_BOARD_X + 118)), UINT16(CH_Y(CH_PAGE_H - CH_INSET - 8)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessSignCallback);
		// the composer's two buttons, live only while it is open
		MSYS_DefineRegion(&gChessGbPostRegion,
		                  UINT16(CH_X(CH_GBM_X + CH_GBM_W - 100)), UINT16(CH_Y(CH_GBM_Y + CH_GBM_H - 34)),
		                  UINT16(CH_X(CH_GBM_X + CH_GBM_W - 12)),  UINT16(CH_Y(CH_GBM_Y + CH_GBM_H - 12)),
		                  MSYS_PRIORITY_HIGHEST, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessGbPostCallback);
		MSYS_DefineRegion(&gChessGbCloseRegion,
		                  UINT16(CH_X(CH_GBM_X + CH_GBM_W - 26)), UINT16(CH_Y(CH_GBM_Y + 4)),
		                  UINT16(CH_X(CH_GBM_X + CH_GBM_W - 4)),  UINT16(CH_Y(CH_GBM_Y + 26)),
		                  MSYS_PRIORITY_HIGHEST, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessGbCloseCallback);
		// the book's pager, either side of the page count at the foot
		const INT32 pgX0 = CH_BOARD_X + CH_BOARD_SIZE - 8 - (CH_GB_PAGES + 2) * 22 + 4;
		const UINT16 pgY0 = UINT16(CH_Y(CH_PAGE_H - CH_INSET - 28));
		const UINT16 pgY1 = UINT16(CH_Y(CH_PAGE_H - CH_INSET - 8));
		MSYS_DefineRegion(&gChessGbPrevRegion,
		                  UINT16(CH_X(pgX0)), pgY0, UINT16(CH_X(pgX0 + 18)), pgY1,
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessGbPageCallback);
		MSYS_SetRegionUserData(&gChessGbPrevRegion, 0, -1);
		MSYS_DefineRegion(&gChessGbNextRegion,
		                  UINT16(CH_X(pgX0 + 22 + CH_GB_PAGES * 22)), pgY0,
		                  UINT16(CH_X(pgX0 + 22 + CH_GB_PAGES * 22 + 18)), pgY1,
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessGbPageCallback);
		MSYS_SetRegionUserData(&gChessGbNextRegion, 0, 1);
		for (int i = 0; i < CH_GB_PAGES; ++i)
		{
			const INT32 nx = pgX0 + 22 + i * 22;
			MSYS_DefineRegion(&gChessGbNumRegion[i],
			                  UINT16(CH_X(nx)), pgY0, UINT16(CH_X(nx + 18)), pgY1,
			                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
			                  ChessGbPageSetCallback);
			MSYS_SetRegionUserData(&gChessGbNumRegion[i], 0, i);
		}
		gfChessRegionsUp = true;
		ChessSyncPageRegions();
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
		MSYS_RemoveRegion(&gChessAdRegion);
		MSYS_RemoveRegion(&gChessGbPrevRegion);
		MSYS_RemoveRegion(&gChessGbNextRegion);
		for (MOUSE_REGION& r : gChessGbNumRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessGbPostRegion);
		MSYS_RemoveRegion(&gChessGbCloseRegion);
		MSYS_RemoveRegion(&gChessSignRegion);
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
			// tips land at cx-4 and cx+4, so the two glyphs mirror optically
			const INT32 x = left ? cx - 4 + step : cx + 2 - step;
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

	void ChessRenderAnimPiece(const ChessGame& game)
	{
		if (!gChessAnim.active || gChessAnim.game != &game || !guiChessPieces) return;
		const UINT32 elapsed = ChessNow() - gChessAnim.start;
		float t = gChessAnim.dur ? float(elapsed) / float(gChessAnim.dur) : 1.0f;
		if (t > 1.0f) t = 1.0f;
		t = 1.0f - (1.0f - t) * (1.0f - t);  // ease out
		INT32 fx, fy, tx, ty;
		SquareToScreen(gChessAnim.from, fx, fy);
		SquareToScreen(gChessAnim.to, tx, ty);
		BltVideoObject(FRAME_BUFFER, guiChessPieces,
		               UINT16(gChessAnim.frame + (gfChessModal ? 12 : 0)),
		               CH_X(fx + INT32((tx - fx) * t)), CH_Y(fy + INT32((ty - fy) * t)));
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
				if (gChessAnim.active && gChessAnim.game == &game && sq == gChessAnim.to) continue;
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
		ChessRenderAnimPiece(game);

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
	void ChessDrawCTAButton(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 bg);
	INT32 ChessRenderSectionPanel(UINT16 icon, ChessStr title);
	void ChessDrawPagerButton(INT32 x, INT32 y, INT32 s, bool lit);
	INT32 ChessDrawTitleBadge(INT32 x, INT32 y, const char* title, UINT32 bg);

	// The right panel while a live game runs: the opponent, the state of
	// play, and the one button.
	void ChessRenderPlayPanel()
	{
		const INT32 cx = ChessRenderSectionPanel(CH_ICON_PLAY, CHS_NAV_PLAY);
		// the NEW GAME button gets the same sunk footer as the puzzle's hint
		FillRect(CH_PANEL_X, CH_FOOT_Y, CH_PANEL_W, 34, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_PANEL_RADIUS, CH_RGB_CHROME);

		// the opponent card follows the pairing; a grey pawn holds the seat
		const INT32 faceX = CH_PANEL_X + 8;
		if (giPlaySeat >= 0 && gPlayOppFace)
		{
			BltVideoSurface(FRAME_BUFFER, gPlayOppFace, CH_X(faceX + 2), CH_Y(CH_COACH_Y + 2), NULL);
			const ChessSeat& opp = CHESS_SEATS[giPlaySeat];
			INT32 hx = faceX + CH_SQ + 6;
			hx += ChessDrawTitleBadge(hx, CH_COACH_Y + 5, opp.title, CH_RGB_PANEL);
			PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, hx, CH_COACH_Y + 6, opp.handle);
			PrintAt(FONT10ARIAL, FONT_GRAY4,
			        hx + 6 + StringPixLength(opp.handle, FONT10ARIALBOLD),
			        CH_COACH_Y + 6, ST::format("({})", opp.rating));
		}
		else
		{
			FillRounded(faceX, CH_COACH_Y, CH_SQ, CH_SQ, CH_RGB_PANEL_SUNK, 3, CH_RGB_PANEL);
			if (guiChessPieces)
			{
				BltVideoObject(FRAME_BUFFER, guiChessPieces, 12, CH_X(faceX), CH_Y(CH_COACH_Y));
			}
			PrintAt(FONT10ARIALBOLD, FONT_GRAY4, faceX + CH_SQ + 6, CH_COACH_Y + 6, T(CHS_PLAY_SEEK));
		}

		const UINT8 colour = giPlaySaid == CHS_PLAY_WIN  ? FONT_MCOLOR_LTGREEN
		                   : giPlaySaid == CHS_PLAY_LOSS ? FONT_MCOLOR_LTRED
		                                                 : FONT_GRAY2;
		PrintAt(FONT10ARIAL, colour, CH_PANEL_X + 10, CH_COACH_Y + 44, T(ChessStr(giPlaySaid)));

		ChessRenderMoveList(gPlaySan, CH_COACH_Y + 62, CH_FOOT_Y - 4);

		// the hint button doubles as NEW GAME here; same box, same region
		ChessDrawCTAButton(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20, CH_HINT_H,
		                   CH_RGB_PANEL);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx,
		             CH_HINT_Y + (CH_HINT_H - GetFontHeight(FONT10ARIALBOLD)) / 2,
		             T(CHS_PLAY_NEW));
	}

	void ChessRenderPanel()
	{
		FillRounded(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		            CH_RGB_PANEL, CH_PANEL_RADIUS, CH_RGB_CHROME);
		const INT32 cx = CH_PANEL_X + CH_PANEL_W / 2;
		const ChessPuzzle& puzzle = CHESS_PUZZLES[giChessPuzzle];

		// header band: snug around the title, the stepper and the day's name
		FillRect(CH_PANEL_X, CH_INSET, CH_PANEL_W, 66, CH_RGB_PANEL_SUNK);
		// and its mirror at the foot: the hint button sits padded inside
		FillRect(CH_PANEL_X, CH_FOOT_Y, CH_PANEL_W, 34, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_PANEL_RADIUS, CH_RGB_CHROME);

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

		FillRoundedOnly(x - 1, y - 1, w + 2, h + 2, CH_RGB_PANEL_UP, 6);
		FillRoundedOnly(x, y, w, h, CH_RGB_PANEL, 5);

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

		ChessDrawCTAButton(x + 12, y + 46, w - 24, 22, CH_RGB_PANEL);
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
		// Code-drawn creatives in the guestbook skyscraper's dress: a flat
		// card, one hairline border, each ad in its target site's colours.
		// Pages without player rows give the banner the extra height.
		const bool tall = giChessStub != 0 && giChessStub != 3;
		const INT32 bh = tall ? 48 : 30;
		const INT32 bx = CH_BOARD_X, bw = CH_BOARD_SIZE;
		const INT32 by = CH_PAGE_H - CH_INSET - bh;
		const INT32 cx = bx + bw / 2;
		const int slot = giChessViewDay % 3;
		FillRect(bx, by, bw, bh, FROMRGB(0, 0, 0));
		if (slot == 0)
		{
			FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(214, 213, 206));
			PrintCentred(FONT10ARIALBOLD, FONT_RED, cx, by + (tall ? 8 : 4),
			             "BOBBY RAY'S - GUNS AND MORE");
			PrintCentred(FONT10ARIAL, FONT_MCOLOR_BLACK, cx, by + (tall ? 22 : 16),
			             "always cheap. always stocked. always legal*");
			if (tall) PrintCentred(FONT10ARIAL, FONT_GRAY6, cx, by + 35, "*mostly");
		}
		else if (slot == 1)
		{
			FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(24, 20, 12));
			PrintCentred(FONT10ARIALBOLD, FONT_YELLOW, cx, by + (tall ? 8 : 4),
			             "CHACH.COM GOLD CROWN");
			PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, cx, by + (tall ? 22 : 16),
			             "MEMBERSHIP - COMING SOON");
			if (tall) PrintCentred(FONT10ARIAL, FONT_GRAY6, cx, by + 35, "do not ask");
		}
		else
		{
			FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(94, 26, 26));
			// tall: the copy stacks between the flanking tiles, so the slogan
			// breaks in two and stays inside the column
			PrintCentred(FONT10ARIALBOLD, FONT_YELLOW, cx, by + (tall ? 6 : 4),
			             "SAN MONA MAHJONG PARLOUR");
			if (tall)
			{
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, by + 20,
				             "GAMES ARE FAIR BECAUSE");
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, by + 33,
				             "MR. KLAUS SAYS SO");
			}
			else
			{
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, by + 16,
				             "GAMES ARE FAIR BECAUSE MR. KLAUS SAYS SO");
			}
			if (tall && guiChessAdTiles)
			{
				// the homepage's pair flanks the copy
				BltVideoObject(FRAME_BUFFER, guiChessAdTiles, 9,
				               CH_X(bx + 8), CH_Y(by + 4));
				BltVideoObject(FRAME_BUFFER, guiChessAdTiles, 18,
				               CH_X(bx + bw - 38), CH_Y(by + 4));
			}
		}
		// A hit counter nobody has ever believed. Derived from the campaign
		// clock rather than stored, so it climbs without costing save bytes.
		if (tall)
		{
			const int hits = 148299 + giChessViewDay * 17 + gChessDay.bestStreak * 3;
			PrintCentred(FONT10ARIAL, FONT_GRAY7, cx, by - 25,
			             ST::format("{} {}", T(CHS_VISITOR), hits));
		}
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
	// A tiny disc for the capture tallies - a plus-shape reads round at 4px.
	void ChessDrawDot(INT32 x, INT32 y, UINT32 rgb)
	{
		FillRect(x + 1, y,     2, 4, rgb);
		FillRect(x,     y + 1, 4, 2, rgb);
	}

	// One player row: avatar, handle with grey rating, the pieces they have
	// taken as coloured discs beneath the name, and a clock on the right edge.
	// The green button, with a little body to it: a darker foot edge, the
	// fill graded lighter toward the top, and a highlight line under the rim.
	void ChessDrawCTAButton(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 bg)
	{
		const INT32 band = (h - 2) / 3;
		FillRounded(x, y, w, h, FROMRGB(86, 128, 45), 3, bg);
		FillRounded(x, y, w, h - 2, CH_RGB_CTA, 3, bg);
		FillRect(x + 2, y + 2, w - 4, band, FROMRGB(150, 199, 88));
		FillRect(x + 3, y + 1, w - 6, 1, FROMRGB(184, 221, 130));
		// the gradient steps down through two dithered rows - checkerboarded
		// pixels of the lighter green over the body, the 1999 blend
		for (INT32 row = 0; row < 2; ++row)
		{
			const INT32 dy = y + 2 + band + row;
			for (INT32 dx = x + 2 + ((row + 1) & 1); dx < x + w - 2; dx += 2)
			{
				FillRect(dx, dy, 1, 1, row == 0 ? FROMRGB(150, 199, 88)
				                                : FROMRGB(140, 191, 82));
			}
		}
	}

	// A chess title, chess.com style: a small crimson chip, white letters.
	// Returns the width it consumed, zero for the untitled.
	INT32 ChessDrawTitleBadge(INT32 x, INT32 y, const char* title, UINT32 bg)
	{
		if (!title || !*title) return 0;
		const INT32 w = StringPixLength(title, TINYFONT1) + 4;
		FillRounded(x, y, w, 9, FROMRGB(167, 45, 45), 2, bg);
		PrintCentred(TINYFONT1, FONT_MCOLOR_WHITE, x + w / 2, y, title);
		return w + 5;
	}

	void ChessRenderPlayerRow(SGPVSurface* face, const ST::string& handle,
	                          const ST::string& rating, INT32 y,
	                          const ChessGame* game, ChessGame::Color side,
	                          int plies, bool clockActive,
	                          const char* title = nullptr)
	{
		INT32 nameX = CH_BOARD_X;
		if (face)
		{
			BltVideoSurface(FRAME_BUFFER, face,
			                CH_X(CH_BOARD_X + (CH_SQ - CH_ROW_FACE) / 2),
			                CH_Y(y + (CH_SQ - CH_ROW_FACE) / 2), NULL);
			nameX += CH_SQ + 8;
		}
		nameX += ChessDrawTitleBadge(nameX, y + 5, title, CH_RGB_CHROME);
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, nameX, y + 6, handle);
		if (!rating.empty())
		{
			PrintAt(FONT10ARIAL, FONT_GRAY4,
			        nameX + StringPixLength(handle, FONT10ARIALBOLD) + 6, y + 6, rating);
		}

		if (game)
		{
			// what this side has captured: the opponent's starting material
			// minus what is still on the board
			const ChessGame::Color prey = ChessGame::Color(side ^ 1);
			int onBoard[7] = {};
			for (int sq = 0; sq < 128; ++sq)
			{
				if (sq & 0x88) continue;
				if (game->IsEmpty(UINT8(sq))) continue;
				if (game->ColorAt(UINT8(sq)) != prey) continue;
				++onBoard[game->PieceAt(UINT8(sq))];
			}
			static const int START[7] = { 0, 8, 2, 2, 2, 1, 1 };
			const UINT32 disc = prey == ChessGame::White ? FROMRGB(245, 245, 243)
			                                             : FROMRGB(30, 28, 26);
			INT32 dx = nameX;
			for (int type = ChessGame::Pawn; type <= ChessGame::Queen; ++type)
			{
				for (int k = onBoard[type]; k < START[type]; ++k)
				{
					ChessDrawDot(dx, y + 21, disc);
					dx += 6;
				}
			}

			// the clock: ten minutes each, bleeding a few seconds per move -
			// nobody has ever believed a hit counter either
			const int mine = side == ChessGame::White ? (plies + 1) / 2 : plies / 2;
			int secs = 600 - mine * 8;
			if (secs < 4) secs = 4;
			const ST::string time = ST::format("{}:{02d}", secs / 60, secs % 60);
			const INT32 boxW = 40;
			const INT32 boxX = CH_BOARD_X + CH_BOARD_SIZE - boxW;
			FillRounded(boxX, y + 7, boxW, 18,
			            clockActive ? CH_RGB_LIGHT : CH_RGB_PANEL_SUNK, 3, CH_RGB_CHROME);
			PrintCentred(FONT10ARIAL,
			             clockActive ? FONT_NEARBLACK : FONT_GRAY4,
			             boxX + boxW / 2, y + 11, time);
		}
	}

	// The last stretch of a move list, numbered SAN pairs, newest at the foot.
	void ChessRenderMoveList(const std::vector<ST::string>& san, INT32 y0, INT32 y1)
	{
		const int pairs = int(san.size() + 1) / 2;
		const int fit   = (y1 - y0) / 13;
		const int first = pairs > fit ? pairs - fit : 0;
		INT32 y = y0;
		for (int pn = first; pn < pairs; ++pn)
		{
			// zebra rows: every second move-pair sits on a sunk band
			if (pn % 2 == 0)
			{
				FillRect(CH_PANEL_X + 6, y - 2, CH_PANEL_W - 12, 13, CH_RGB_PANEL_SUNK);
			}
			PrintAt(FONT10ARIAL, FONT_GRAY7, CH_PANEL_X + 10, y, ST::format("{}.", pn + 1));
			PrintAt(FONT10ARIAL, FONT_GRAY2, CH_PANEL_X + 32, y, san[pn * 2]);
			if (pn * 2 + 1 < int(san.size()))
			{
				PrintAt(FONT10ARIAL, FONT_GRAY2, CH_PANEL_X + 86, y, san[pn * 2 + 1]);
			}
			y += 13;
		}
	}

	// A right panel scaffold shared by the live views: rounded ground and a
	// sunk header band carrying the section's icon and name.
	INT32 ChessRenderSectionPanel(UINT16 icon, ChessStr title)
	{
		FillRounded(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		            CH_RGB_PANEL, CH_PANEL_RADIUS, CH_RGB_CHROME);
		// the band sits snug around the title alone
		FillRect(CH_PANEL_X, CH_INSET, CH_PANEL_W, 34, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_PANEL_RADIUS, CH_RGB_CHROME);
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
		ChessDrawCTAButton(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20, CH_HINT_H,
		                   CH_RGB_PANEL);
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
		ChessRenderAnimPiece(gWatchGame);

		const ChessSeat& white = CHESS_SEATS[gWatchSeat[0]];
		const ChessSeat& black = CHESS_SEATS[gWatchSeat[1]];
		const int wPlies = int(gWatchSan.size());
		const bool wTurn = gWatchGame.SideToMove() == ChessGame::White && !giWatchResult;
		const bool bTurn = gWatchGame.SideToMove() == ChessGame::Black && !giWatchResult;
		ChessRenderPlayerRow(gWatchFaceHalf[1], black.handle,
		                     ST::format("({})", black.rating), CH_ROW_TOP_Y,
		                     &gWatchGame, ChessGame::Black, wPlies, bTurn,
		                     black.title);
		ChessRenderPlayerRow(gWatchFaceHalf[0], white.handle,
		                     ST::format("({})", white.rating), CH_ROW_BOT_Y,
		                     &gWatchGame, ChessGame::White, wPlies, wTurn,
		                     white.title);

		// the LIVE chip rides the top row's right end
		if (!giWatchResult)
		{
			FillRounded(CH_BOARD_X + CH_BOARD_SIZE - 84, CH_ROW_TOP_Y + 11, 8, 8,
			            FROMRGB(196, 36, 36), 2, CH_RGB_CHROME);
		}
		PrintAt(FONT10ARIAL, giWatchResult ? FONT_GRAY4 : FONT_MCOLOR_WHITE,
		        CH_BOARD_X + CH_BOARD_SIZE - 72, CH_ROW_TOP_Y + 10,
		        giWatchResult ? "END" : "LIVE");
	}

	void ChessRenderWatchPanel()
	{
		ChessRenderSectionPanel(CH_ICON_WATCH, CHS_NAV_WATCH);
		PrintCentred(FONT10ARIAL, FONT_GRAY4, CH_PANEL_X + CH_PANEL_W / 2, 48,
		             T(giWatchResult ? CHS_WATCH_OVER : CHS_WATCH_LIVE));
		ChessRenderMoveList(gWatchSan, 66, CH_PAGE_H - CH_INSET - 8);
	}

	// One guestbook entry: a sunk row with the avatar - the merc portrait if
	// the handle belongs to one, a tinted initial disc if it does not.
	INT32 ChessRenderGuestRow(INT32 y, SGPVSurface* face, UINT32 tint,
	                          const ST::string& name, const ST::string& handle,
	                          const char* date, const char* line, int lines,
	                          bool separated)
	{
		const INT32 rowH = 24 + 13 * lines;
		// rows sit on the book's own ground; a hairline divides neighbours
		if (separated) FillRect(CH_BOARD_X + 8, y, CH_BOARD_SIZE - 16, 1, CH_RGB_ROW_SEP);
		const INT32 ax = CH_BOARD_X + 14;
		const INT32 ay = y + 8;
		if (face)
		{
			BltVideoSurface(FRAME_BUFFER, face, CH_X(ax), CH_Y(ay), NULL);
		}
		else
		{
			FillRounded(ax, ay, CH_GB_FACE, CH_GB_FACE, tint, 4, CH_RGB_PANEL_SUNK);
			char c = handle.size() > 1 ? handle[1] : '?';
			if (c >= 'a' && c <= 'z') c -= 32;
			const char ini[2] = { c, 0 };
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, ax + CH_GB_FACE / 2,
			             ay + (CH_GB_FACE - GetFontHeight(FONT10ARIALBOLD)) / 2, ini);
		}
		INT32 tx = CH_BOARD_X + 52;
		// the date owns the right edge on every row; the handle only prints
		// when it fits in the space a long name leaves before it
		const INT32 dx = CH_BOARD_X + CH_BOARD_SIZE - 12 - StringPixLength(date, TINYFONT1);
		PrintAt(TINYFONT1, FONT_GRAY6, dx, y + 7, date);
		tx += ChessDrawTitleBadge(tx, y + 7, ChessTitleForHandle(handle), CH_RGB_PANEL);
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, tx, y + 8, name);
		const INT32 nameEnd = tx + StringPixLength(name, FONT10ARIALBOLD) + 6;
		if (nameEnd + StringPixLength(handle, FONT10ARIAL) < dx - 8)
		{
			PrintAt(FONT10ARIAL, FONT_GRAY4, nameEnd, y + 8, handle);
		}
		DisplayWrappedString(UINT16(CH_X(tx)), UINT16(CH_Y(y + 20)),
		                     UINT16(CH_BOARD_X + CH_BOARD_SIZE - 12 - tx), 3,
		                     FONT10ARIAL, FONT_GRAY2, line,
		                     FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		return rowH;
	}

	// The guestbook fills the page: the title sits above the book in the
	// daily header's lockup, the entries carry avatars, and signing is forever.
	void ChessRenderGuestbook()
	{
		ChessIconLabel(CH_ICON_COMMUNITY, CH_BOARD_X + 2, 15, FONT10ARIALBOLD,
		               FONT_MCOLOR_WHITE, T(CHS_GB_TITLE));
		PrintAt(FONT10ARIAL, FONT_GRAY4,
		        CH_BOARD_X + 2 + ChessIconLabelWidth(FONT10ARIALBOLD, T(CHS_GB_TITLE)) + 10,
		        15 - GetFontHeight(FONT10ARIAL) / 2, T(CHS_GB_PROMPT));

		FillRounded(CH_BOARD_X, CH_GB_TOP, CH_BOARD_SIZE,
		            CH_PAGE_H - CH_INSET - CH_GB_TOP,
		            CH_RGB_PANEL, CH_PANEL_RADIUS, CH_RGB_CHROME);

		INT32 y = CH_GB_TOP + 8;
		const size_t first = size_t(giChessGbPage) * CH_GB_PER_PAGE;
		size_t shown = 0;
		for (size_t i = first; i < first + CH_GB_PER_PAGE && i < CH_GB_COUNT; ++i, ++shown)
		{
			y += ChessRenderGuestRow(y, gGuestFace[i], CHESS_GUESTBOOK[i].tint,
			                         gGuestName[i], CHESS_GUESTBOOK[i].handle,
			                         CHESS_GUESTBOOK[i].date,
			                         CHESS_GUESTBOOK[i].line,
			                         CHESS_GUESTBOOK[i].lines, shown > 0);
		}

		// your signature closes the book, zebra parity and all
		if ((gChessDay.flags & ChessDaily::FLAG_SIGNED) && giChessGbPage == CH_GB_PAGES - 1)
		{
			const ST::string handle = gChessSelfNick.empty()
				? ST::string("@commander") : ST::format("@{}", gChessSelfNick);
			const char* mine = gChessGuestLine.empty() ? T(CHS_GB_YOURS)
			                                           : gChessGuestLine.c_str();
			const int mlines = int(std::min<size_t>(3, std::strlen(mine) / 39 + 1));
			const ST::string name = gChessSelfName.empty() ? ST::string("Commander")
			                                               : gChessSelfName;
			ChessRenderGuestRow(y, gGuestSelfFace, 0, name, handle, "today",
			                    mine, mlines, shown > 0);
		}

		// the foot, one shared centreline at the book's bottom edge: the sign
		// button bottom-left, the numbered pager to its right
		const INT32 fy = CH_PAGE_H - CH_INSET - 30;
		if (!(gChessDay.flags & ChessDaily::FLAG_SIGNED))
		{
			ChessDrawCTAButton(CH_BOARD_X + 8, fy, 110, 22, CH_RGB_PANEL);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, CH_BOARD_X + 63,
			             fy + 6, T(CHS_GB_SIGN));
		}
		const INT32 px0 = CH_BOARD_X + CH_BOARD_SIZE - 8 - (CH_GB_PAGES + 2) * 22 + 4;
		const INT32 by = fy + 2;
		ChessDrawPagerButton(px0, by, 18, false);
		ChessDrawChevron(px0 + 9, by + 8, true,
		                 giChessGbPage > 0 ? FROMRGB(180, 175, 168) : FROMRGB(104, 98, 91));
		for (int i = 0; i < CH_GB_PAGES; ++i)
		{
			const INT32 nx = px0 + 22 + i * 22;
			const bool cur = i == giChessGbPage;
			ChessDrawPagerButton(nx, by, 18, cur);
			PrintCentred(FONT10ARIALBOLD, cur ? FONT_NEARBLACK : FONT_GRAY2,
			             nx + 9, by + 4, ST::format("{}", i + 1));
		}
		const INT32 nxe = px0 + 22 + CH_GB_PAGES * 22;
		ChessDrawPagerButton(nxe, by, 18, false);
		ChessDrawChevron(nxe + 9, by + 8, false,
		                 giChessGbPage < CH_GB_PAGES - 1 ? FROMRGB(180, 175, 168)
		                                                 : FROMRGB(104, 98, 91));
	}

	// The CTA button's construction in grey miniature, for the pager: a foot
	// edge, a body, a lighter top band and a highlight line. The lit variant
	// is the current page.
	void ChessDrawPagerButton(INT32 x, INT32 y, INT32 s, bool lit)
	{
		const UINT32 body = lit ? FROMRGB(124, 119, 112) : FROMRGB( 70,  64,  59);
		const UINT32 band = lit ? FROMRGB(138, 133, 126) : FROMRGB( 80,  74,  68);
		const UINT32 high = lit ? FROMRGB(152, 147, 140) : FROMRGB( 90,  84,  77);
		FillRounded(x, y, s, s, FROMRGB(28, 25, 22), 3, CH_RGB_PANEL);
		FillRounded(x, y, s, s - 2, body, 3, CH_RGB_PANEL);
		FillRect(x + 2, y + 2, s - 4, (s - 2) / 3, band);
		FillRect(x + 3, y + 1, s - 6, 1, high);
	}

	// The composer: your avatar, your handle, a box you can actually type in.
	// The page dims behind it, the way the board dims under the result card.
	void ChessRenderGuestCompose()
	{
		if (!gfChessGbCompose) return;
		FRAME_BUFFER->ShadowRect(CH_X(0), CH_Y(0),
		                         CH_X(LAPTOP_SCREEN_WIDTH) - 1, CH_Y(CH_PAGE_H) - 1);

		const INT32 mx = CH_GBM_X, my = CH_GBM_Y;
		const INT32 mw = CH_GBM_W, mh = CH_GBM_H;
		FillRounded(mx - 2, my - 2, mw + 4, mh + 4, CH_RGB_CHROME, 6, CH_RGB_PANEL);
		FillRounded(mx, my, mw, mh, CH_RGB_PANEL_UP, 5, CH_RGB_CHROME);

		// title row, closed by a hairline
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, mx + 12, my + 10, T(CHS_GB_SIGN));
		PrintAt(FONT10ARIALBOLD, FONT_GRAY4, mx + mw - 18, my + 10, "X");
		FillRect(mx + 12, my + 28, mw - 24, 1, CH_RGB_ROW_SEP);

		// you, as the book will show you
		const ST::string handle = gChessSelfNick.empty()
			? ST::string("@commander") : ST::format("@{}", gChessSelfNick);
		if (gGuestSelfFace)
		{
			BltVideoSurface(FRAME_BUFFER, gGuestSelfFace, CH_X(mx + 12), CH_Y(my + 38), NULL);
		}
		const ST::string name = gChessSelfName.empty() ? ST::string("Commander")
		                                               : gChessSelfName;
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, mx + 46, my + 46, name);
		PrintAt(FONT10ARIAL, FONT_GRAY4,
		        mx + 52 + StringPixLength(name, FONT10ARIALBOLD), my + 46, handle);

		// the input: typed text wraps, the caret blinks, the limit is ze disk
		FillRounded(mx + 12, my + 72, mw - 24, 44, FROMRGB(22, 20, 18), 3, CH_RGB_PANEL_UP);
		if (gChessGbInput.empty())
		{
			PrintAt(FONT10ARIAL, FONT_GRAY6, mx + 18, my + 79, T(CHS_GB_WRITE));
		}
		const bool caret = (ChessNow() / 400) & 1;
		const std::string typed = gChessGbInput + (caret ? "_" : " ");
		DisplayWrappedString(UINT16(CH_X(mx + 18)), UINT16(CH_Y(my + 79)),
		                     UINT16(mw - 36), 1, FONT10ARIAL, FONT_GRAY1,
		                     ST::string(typed.c_str()), FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		// the counter and the small print share a line above the button row
		PrintAt(FONT10ARIAL, FONT_GRAY4, mx + 12, my + 122, T(CHS_GB_MOD));
		PrintAt(FONT10ARIAL, FONT_GRAY6, mx + mw - 46, my + 122,
		        ST::format("{}/{}", gChessGbInput.size(), CH_GB_LINE_MAX));

		ChessDrawCTAButton(mx + mw - 100, my + mh - 34, 88, 22, CH_RGB_PANEL_UP);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, mx + mw - 56, my + mh - 28, "POST");
	}

	// With the book at full height the ad moves to the sidebar and grows into
	// the 1999 skyscraper: white card, hard border, foreign to the site.
	void ChessRenderGuestAd()
	{
		const INT32 x = CH_PANEL_X, w = CH_PANEL_W;
		const INT32 cx = x + w / 2;
		PrintCentred(FONT10ARIAL, FONT_GRAY7, cx, 10, "- ADVERTISEMENT -");
		// the card runs level with the book beside it
		const INT32 y = CH_GB_TOP;
		const INT32 h = CH_PAGE_H - CH_INSET - y;
		const int slot = giChessViewDay % 3;
		FillRect(x, y, w, h, FROMRGB(0, 0, 0));

		if (slot == 0)
		{
			// Bobby Ray's, in his storefront's dusty grey and red
			FillRect(x + 1, y + 1, w - 2, h - 2, FROMRGB(214, 213, 206));
			FillRect(x + 8, y + 10, w - 16, 70, FROMRGB(52, 48, 44));
			PrintCentred(FONT12ARIAL, FONT_RED, cx, y + 22, "BOBBY RAY'S");
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, y + 46, "GUNS AND MORE");
			PrintCentred(FONT10ARIAL, FONT_MCOLOR_BLACK, cx, y + 110, "ALWAYS CHEAP.");
			PrintCentred(FONT10ARIAL, FONT_MCOLOR_BLACK, cx, y + 128, "ALWAYS STOCKED.");
			PrintCentred(FONT10ARIAL, FONT_MCOLOR_BLACK, cx, y + 146, "ALWAYS LEGAL*");
			PrintCentred(FONT10ARIAL, FONT_GRAY6, cx, y + 166, "*mostly");
			FillRect(x + 24, y + h - 60, w - 48, 24, FROMRGB(178, 24, 24));
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, y + h - 53, "SHOP NOW");
		}
		else if (slot == 1)
		{
			// the crown, coming soon forever; clicking asks, and he answers once
			FillRect(x + 1, y + 1, w - 2, h - 2, FROMRGB(24, 20, 12));
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, y + 24, "CHACH.COM");
			const UINT32 gold = FROMRGB(212, 175, 55);
			const INT32 cy = y + 66;
			FillRect(cx - 16, cy + 10, 32, 8, gold);
			FillRect(cx - 16, cy, 6, 10, gold);
			FillRect(cx - 3,  cy - 4, 6, 14, gold);
			FillRect(cx + 10, cy, 6, 10, gold);
			PrintCentred(FONT12ARIAL, FONT_YELLOW, cx, cy + 34, "GOLD CROWN");
			PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, cx, cy + 56, "MEMBERSHIP");
			PrintCentred(FONT10ARIALBOLD, FONT_YELLOW, cx, cy + 84, "COMING SOON");
			PrintCentred(FONT10ARIAL, FONT_GRAY6, cx, y + h - 30, "do not ask");
		}
		else
		{
			// the Parlour, in the Parlour's own maroon and gold homepage dress
			FillRect(x + 1, y + 1, w - 2, h - 2, FROMRGB(94, 26, 26));
			FillRect(x + 8, y + 10, w - 16, 84, FROMRGB(58, 14, 14));
			PrintCentred(FONT12ARIAL, FONT_YELLOW, cx, y + 22, "SAN MONA");
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, y + 44, "MAHJONG PARLOUR");
			FillRect(x + 22, y + 62, w - 44, 1, FROMRGB(200, 160, 40));
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, y + 72, "a Kingpin establishment");
			if (guiChessAdTiles)
			{
				// the homepage's pair: the one-ring and the bird
				BltVideoObject(FRAME_BUFFER, guiChessAdTiles, 9,
				               CH_X(cx - 34), CH_Y(y + 104));
				BltVideoObject(FRAME_BUFFER, guiChessAdTiles, 18,
				               CH_X(cx + 4), CH_Y(y + 104));
			}
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, y + 160, "GAMES ARE FAIR");
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, y + 176, "BECAUSE MR. KLAUS");
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, y + 192, "SAYS SO");
			if (guiChessAdDragon)
			{
				// the gold medallion from the Parlour's own masthead
				BltVideoObject(FRAME_BUFFER, guiChessAdDragon, 3,
				               CH_X(cx - 44), CH_Y(y + 208));
			}
			FillRect(x + 20, y + h - 56, w - 40, 26, FROMRGB(58, 14, 14));
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, y + h - 48, "Play the Table");
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, y + h - 20, "est. 1999");
		}
	}

	void ChessRenderFooter()
	{
		PrintAt(FONT10ARIAL, FONT_GRAY7, CH_BOARD_X, CH_PAGE_H - CH_INSET - 60, T(CHS_FOOTER));
	}
}

// --- laptop page hooks ----------------------------------------------------

// The composer eats the keyboard while it is open - the parlour chat's
// pattern. Returns true when the key was ours.
bool ChessHandleTypedKey(UINT32 usParam, UINT16 usKeyState)
{
	if (!gfChessGbCompose) return false;
	if (usParam == SDLK_ESCAPE)
	{
		gfChessGbCompose = false;
		ChessSyncPageRegions();
		ChessRedraw();
		return true;
	}
	if (usParam == SDLK_RETURN || usParam == SDLK_KP_ENTER)
	{
		ChessGbPost();
		return true;
	}
	if (usParam == SDLK_BACKSPACE)
	{
		if (!gChessGbInput.empty())
		{
			gChessGbInput.pop_back();
			ChessRedraw();
		}
		return true;
	}
	// printable characters arrive as TEXT_INPUT events, which know the
	// keyboard layout; here they are only swallowed so no shortcut fires
	return true;
}

// Layout-aware typing: the engine's TEXT_INPUT events carry the character
// the keyboard actually produced, shift, dead keys and all.
bool ChessHandleTextInput(const ST::utf32_buffer& codepoints)
{
	if (!gfChessGbCompose) return false;
	bool changed = false;
	for (char32_t cp : codepoints)
	{
		if (cp < 32 || cp > 126) continue;  // ze book is ASCII
		if (gChessGbInput.size() >= CH_GB_LINE_MAX) break;
		gChessGbInput += char(cp);
		changed = true;
	}
	if (changed) ChessRedraw();
	return true;
}

void EnterChess()
{
	gChessDay.flags |= ChessDaily::FLAG_DISCOVERED;

	guiChessPieces = nullptr;
	guiChessPiecesSmall = nullptr;
	guiChessCoach  = nullptr;
	guiChessIcons  = nullptr;
	guiChessLogo   = nullptr;
	guiChessAdDragon = nullptr;
	guiChessAdTiles  = nullptr;
	guiChessSelf   = nullptr;
	giChessStub    = -1;
	giChessGbPage  = 0;
	gfChessGbCompose = false;
	gChessGbInput.clear();
	gChessSelfNick = ST::string();
	gChessSelfName = ST::string();
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
		guiChessAdDragon = AddVideoObjectFromFile("sti/laptop/mahjongdragon.sti");
		guiChessAdTiles  = AddVideoObjectFromFile("sti/laptop/mahjongtiles.sti");
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
			guiChessSelf = Load33Portrait(imp);
			SGPVObject* big = ChessLoadPortrait(imp);
			guiChessSelfHalf = ChessBakeFace(big, CH_ROW_FACE);
			gGuestSelfFace   = ChessBakeFace(big, CH_GB_FACE);
			DeleteVideoObject(big);
			gChessSelfNick   = imp.zNickname;
			gChessSelfName   = ChessFirstName(imp.zName);
		}
		catch (...)
		{
		}
	}

	// the guestbook regulars: whoever has a portrait gets it by their entry
	for (size_t i = 0; i < CH_GB_COUNT; ++i)
	{
		gGuestFace[i] = nullptr;
		gGuestName[i] = CHESS_GUESTBOOK[i].name;
		if (CHESS_GUESTBOOK[i].pid == CH_NO_PID) continue;
		try
		{
			MERCPROFILESTRUCT const& profile = GetProfile(CHESS_GUESTBOOK[i].pid);
			if (!profile.zName.empty()) gGuestName[i] = ChessFirstName(profile.zName);
			SGPVObject* f = ChessLoadPortrait(profile);
			gGuestFace[i] = ChessBakeFace(f, CH_GB_FACE);
			DeleteVideoObject(f);
		}
		catch (...)
		{
			// no portrait on disk: the tinted initial disc stands in
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

	// Dev shortcut leg four: JA2_DEV_CHESS=0/2/3/4 lands on Play, Learn,
	// Watch or Groups; any other value stays on the puzzle.
	if (const char* dev = getenv("JA2_DEV_CHESS"))
	{
		const int want = dev[0] == '0' ? 0
		               : dev[0] == '2' ? 2
		               : dev[0] == '3' ? 3
		               : dev[0] == '4' ? 4 : -1;
		if (want >= 0)
		{
			giChessStub = want;
			if (want == 0 && giPlaySeat < 0 && guiPlaySeekDue == 0) ChessPlayNewGame();
			if (want == 2) gLearnGame.SetFen(CHESS_LESSONS[giChessLesson].fen);
			if (want == 3 && guiWatchNextMove == 0)
			{
				ChessWatchNewGame();
				guiWatchNextMove = ChessNow() + 900;
			}
			ChessSyncPageRegions();
		}
	}
}

void ExitChess()
{
	ChessRemoveRegions();
	if (guiChessPieces) { DeleteVideoObject(guiChessPieces); guiChessPieces = nullptr; }
	if (guiChessPiecesSmall) { DeleteVideoObject(guiChessPiecesSmall); guiChessPiecesSmall = nullptr; }
	if (guiChessCoach)  { DeleteVideoObject(guiChessCoach);  guiChessCoach  = nullptr; }
	if (guiChessIcons)  { DeleteVideoObject(guiChessIcons);  guiChessIcons  = nullptr; }
	if (guiChessLogo)   { DeleteVideoObject(guiChessLogo);   guiChessLogo   = nullptr; }
	if (guiChessAdDragon) { DeleteVideoObject(guiChessAdDragon); guiChessAdDragon = nullptr; }
	if (guiChessAdTiles)  { DeleteVideoObject(guiChessAdTiles);  guiChessAdTiles  = nullptr; }
	if (guiChessSelf)   { DeleteVideoObject(guiChessSelf);   guiChessSelf   = nullptr; }
	if (guiChessSelfHalf)  { DeleteVideoSurface(guiChessSelfHalf);  guiChessSelfHalf  = nullptr; }
	if (gPlayOppFace)      { DeleteVideoSurface(gPlayOppFace);      gPlayOppFace      = nullptr; }
	if (guiChessCoachHalf) { DeleteVideoSurface(guiChessCoachHalf); guiChessCoachHalf = nullptr; }
	if (gGuestSelfFace)    { DeleteVideoSurface(gGuestSelfFace);    gGuestSelfFace    = nullptr; }
	for (SGPVSurface*& f : gGuestFace)
	{
		if (f) { DeleteVideoSurface(f); f = nullptr; }
	}
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
		ChessRenderGuestAd();
		ChessRenderGuestCompose();
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
	}
	else if (giChessStub == 0)
	{
		ChessRenderBoard();
		const int pPlies = int(gPlaySan.size());
		if (giPlayState == 3 || giPlaySeat < 0)
		{
			// still seeking: a grey pawn stands in where the opponent will be
			FillRounded(CH_BOARD_X, CH_ROW_TOP_Y, CH_SQ, CH_SQ,
			            CH_RGB_PANEL_SUNK, 3, CH_RGB_CHROME);
			if (guiChessPieces)
			{
				BltVideoObject(FRAME_BUFFER, guiChessPieces, 12,
				               CH_X(CH_BOARD_X), CH_Y(CH_ROW_TOP_Y));
			}
			PrintAt(FONT10ARIALBOLD, FONT_GRAY4, CH_BOARD_X + CH_SQ + 8,
			        CH_ROW_TOP_Y + 6, T(CHS_PLAY_SEEK));
		}
		else
		{
			const ChessSeat& opp = CHESS_SEATS[giPlaySeat];
			ChessRenderPlayerRow(gPlayOppFace, opp.handle,
			                     ST::format("({})", opp.rating), CH_ROW_TOP_Y,
			                     &gPlayGame, ChessGame::Black, pPlies, giPlayState == 1,
			                     opp.title);
		}
		ChessRenderPlayerRow(guiChessSelfHalf,
		                     gChessSelfNick.empty() ? ST::string("@you")
		                                            : ST::format("@{}", gChessSelfNick),
		                     ST::string(), CH_ROW_BOT_Y,
		                     &gPlayGame, ChessGame::White, pPlies, giPlayState == 0);
		ChessRenderBanner();
		ChessRenderPlayPanel();
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
	// the composer's caret blinks on the strategic clock
	if (gfChessGbCompose)
	{
		static bool sLastCaret = false;
		const bool caret = (ChessNow() / 400) & 1;
		if (caret != sLastCaret)
		{
			sLastCaret = caret;
			ChessRedraw();
		}
	}

	// a piece in flight repaints every frame until it lands
	if (gChessAnim.active)
	{
		if (ChessNow() - gChessAnim.start >= gChessAnim.dur) gChessAnim.active = false;
		ChessRedraw();
	}

	// matchmaking: the seek resolves into a random regular
	if (giChessStub == 0 && giPlayState == 3 && ChessNow() >= guiPlaySeekDue)
	{
		giPlaySeat = int(Random(6));
		try
		{
			SGPVObject* face = ChessLoadPortrait(GetProfile(CHESS_SEATS[giPlaySeat].pid));
			gPlayOppFace = ChessBakeFace(face, CH_ROW_FACE);
			DeleteVideoObject(face);
		}
		catch (...) {}
		giPlayState = 0;
		giPlaySaid  = CHS_PLAY_YOUR;
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	// his move in the live game lands on his own clock
	if (giChessStub == 0 && giPlayState == 1 && ChessNow() >= guiPlayDue)
	{
		const ChessGame::Move m = ChessPlayPickMove();
		if (!m.IsNull())
		{
			gPlaySan.push_back(gPlayGame.San(m));
			gubPlayFrom = m.from; gubPlayTo = m.to;
			gPlayGame.MakeMove(m);
			ChessAnimateMove(gPlayGame, m, 140);
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
			const ChessGame::Move m = ChessWatchPickMove();
			if (!m.IsNull())
			{
				gWatchSan.push_back(gWatchGame.San(m));
				gubWatchFrom = m.from; gubWatchTo = m.to;
				gWatchGame.MakeMove(m);
				ChessAnimateMove(gWatchGame, m, 150);
				// the exhibition speaks at full board volume: move, capture,
				// check - the same cues as your own games
				ChessPlay(ChessMoveSound(m, gWatchGame.IsInCheck(gWatchGame.SideToMove()), false));
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
			ChessAnimateMove(gChessGame, reply, 140);
			ChessPlay(ChessMoveSound(reply, gChessGame.IsInCheck(gChessGame.SideToMove()), false));
			++guiChessPly;
		}
	}
	if (guiChessPly < gChessSolution.size()) giChessSaid = CHS_ST_YOUR_MOVE;
	if (guiChessPly >= gChessSolution.size() && gChessState == CHUI_PUZZLE) ChessRecordSolved();
	ChessRedraw();
}
