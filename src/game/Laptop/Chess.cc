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
#include "GameRes.h"
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
#define CH_MODAL_H      120
#define CH_MODAL_X      (CH_BOARD_X + (CH_BOARD_SIZE - CH_MODAL_W) / 2)
#define CH_MODAL_Y      (CH_BOARD_Y + (CH_BOARD_SIZE - CH_MODAL_H) / 2)

#define CH_DATE_Y       28
#define CH_COACH_Y      78
#define CH_COACH_TILE   36
#define CH_FOOT_Y       (CH_PAGE_H - CH_INSET - 38)
#define CH_HINT_Y       (CH_FOOT_Y + 5)
#define CH_HINT_H       28

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
// the move list's zebra: the one band that goes up rather than down, because
// the reference's alternating rows sit a shade above their panel
#define CH_RGB_ROW_ALT     FROMRGB( 47,  43,  39)
#define CH_RGB_CTA         FROMRGB(129, 182,  76)
#define CH_RGB_HEART_SPENT FROMRGB( 70,  68,  65)
// the account row: the nickname is suggested rather than set, two grey bars
// standing in for text the way a low-res mock would
#define CH_RGB_NICK        FROMRGB(154, 152, 147)
#define CH_RGB_NICK_DIM    FROMRGB( 92,  90,  87)
// the coach speaks from a white bubble, as on the live site
#define CH_RGB_BUBBLE      FROMRGB(255, 255, 255)
// a king in check tints his square red, the analog of the move highlights
#define CH_RGB_CHK_LIGHT   FROMRGB(235, 125, 106)
#define CH_RGB_CHK_DARK    FROMRGB(199,  98,  78)

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
	bool   gfLearnSolved    = false;      // the lesson's move has been played
	const char* gzLearnSay  = nullptr;    // coach override: praise, deny, hint
	UINT8  gubLearnFrom     = 0x7F;       // the solved move, for the highlight
	UINT8  gubLearnTo       = 0x7F;

	void ChessLearnTryMove(UINT8 from, UINT8 to);

	UINT8 gubLearnTarget = 0x7F; // the answer's destination, kept lit
	bool  gfLearnModal   = false; // the last lesson's graduation card

	void ChessLearnReset();
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
	// frames in chessflags.sti, in the order the generator writes them
	enum ChessFlag { CH_FLAG_NONE = -1, CH_FLAG_RU = 0, CH_FLAG_DK, CH_FLAG_GB,
	                 CH_FLAG_US, CH_FLAG_DE, CH_FLAG_AR };

	struct ChessSeat
	{
		ProfileID   pid;
		const char* handle;
		const char* title;  // "IM"-style chess title; "" for the untitled
		int         rating;
		int         depth;  // iteration cap: the weak seats bind on this
		int         ms;     // think budget: the strong seats bind on this
		int         err;    // percent of moves played at random
		int         greed;  // percent chance any available capture is taken
		int         flag;   // where their A.I.M. bio puts them
	};
	// Flags follow the bios, not guesswork: Ivan and Igor Dolvich are Red Army,
	// Monica "Buns" Sonderguard shot for Denmark at the Atlanta games, and
	// A.I.M. printed a correction to say Scope's service was the S.A.S. Fox and
	// Spider have no nationality on file, which in 1999 meant American.
	static const ChessSeat CHESS_SEATS[6] =
	{
		{ IVAN,   "@ivan_d",  "IM", 2145, 64, 350,  2, 10, CH_FLAG_RU },
		{ BUNS,   "@buns",    "FM", 1994,  8, 200,  6, 30, CH_FLAG_DK },
		{ SCOPE,  "@scope",   "CM", 1873,  6, 120,  8, 45, CH_FLAG_GB },
		{ FOX,    "@foxtrot", "",   1731,  4,  80, 16, 60, CH_FLAG_US },
		{ IGOR,   "@igor_k",  "",   1677,  3,  60, 24, 70, CH_FLAG_RU },
		{ SPIDER, "@spider",  "",   1512,  2,  30, 30, 80, CH_FLAG_US },
	};

	// daily chess is played against one man, by letter, one move a day. He
	// signs the guestbook the same way, under the flag he is fighting for.
	static const ChessSeat CHESS_SEAT_ENRICO =
		{ ENRICO, "@a_free_arulco", "", 1899, 7, 150, 5, 15, CH_FLAG_AR };

	// the guestbook looks a signer's title up by their handle
	const char* ChessTitleForHandle(const ST::string& handle)
	{
		for (const ChessSeat& seat : CHESS_SEATS)
		{
			if (handle == seat.handle && seat.title[0]) return seat.title;
		}
		return nullptr;
	}

	// and their flag the same way. The proprietor is German, which is why the
	// site has a German column at all.
	int ChessFlagForHandle(const ST::string& handle)
	{
		for (const ChessSeat& seat : CHESS_SEATS)
		{
			if (handle == seat.handle) return seat.flag;
		}
		if (handle == CHESS_SEAT_ENRICO.handle) return CHESS_SEAT_ENRICO.flag;
		if (handle == "@grunty" || handle == "@sirFER") return CH_FLAG_DE;
		return CH_FLAG_NONE;
	}

	// The badge itself: thirteen by nine, drawn where chess.com puts it -
	// after the rating, before whatever the account is boasting about.
	INT32 ChessDrawFlag(INT32 x, INT32 y, int flag);
	int gWatchSeat[2] = { 0, 1 };            // [0] plays White, [1] Black
	// both live sidebars carry two tabs, like the reference: moves and chat.
	// Watch is a public room; Play is just the two of you at the table.
	int giWatchTab = 0;
	int giPlayTab  = 0;
	// scroll state, measured from the bottom: 0 follows the live end
	int giMoveScroll = 0;
	int giChatScroll = 0;
	struct WatchChatLine { ST::string who; ST::string text; };
	std::vector<WatchChatLine> gWatchChat;
	std::vector<WatchChatLine> gPlayChat;
	UINT32 guiPlayChatReplyDue = 0;
	SGPVSurface* gWatchFaceHalf[2] = { nullptr, nullptr };
	std::vector<ST::string> gWatchSan;       // the exhibition's move list, SAN
	std::vector<ST::string> gPlaySan;        // yours
	// the scrubber: every position of the running game, so played moves can
	// be stepped through; [0] is the start, back() is live
	std::vector<ChessGame> gWatchHist;
	std::vector<ChessGame> gPlayHist;
	int giWatchView = -1;   // -1 live, else an index into the history
	int giPlayView  = -1;
	// Play: a live game against the proprietor. You are White; he is not in a
	// hurry.
	ChessGame gPlayGame;
	int    giPlayState   = 4;   // 0 your move, 1 thinking, 2 over, 3 seeking,
	                            // 4 the lobby: pick a time, press start
	int    giPlayMinutes = 10;  // the chosen control; 0 is daily chess
	int    giPlayControl = 10;  // the control the running game was started at
	UINT32 guiPlayDailyDay = 0; // campaign day of your last daily move
	int    giPlayEndReason = 0;      // 0 on the board, 1 by resignation
	bool   gfPlayModal = false;      // the finished-game card
	UINT32 guiResignArmUntil = 0;    // the flag asks twice
	int    giPlayRematchSeat = -1;   // a rematch keeps the chair
	struct PlayControl { int mins; const char* name; const char* time; };
	const PlayControl CH_PLAY_TIMES[4] =
	{
		{ 1, "BULLET", "1 min" },
		{ 3, "BLITZ",  "3 min" },
		{ 10, "RAPID", "10 min" },
		{ 0, "DAILY",  "1 day" },
	};
	int    giPlaySeat    = -1;  // index into CHESS_SEATS once paired

	// -2 seats the letter-writer; -1 is nobody; 0.. indexes the regulars
	const ChessSeat& ChessOpponent()
	{
		if (giPlaySeat == -2) return CHESS_SEAT_ENRICO;
		return CHESS_SEATS[giPlaySeat < 0 ? 0 : giPlaySeat];
	}

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

	// the profile ledger: your rating and the games the server remembers,
	// persisted with the daily state behind the 0xC6 save marker
	UINT16 gusProfRating = 0;   // 0 until the first live game finishes
	UINT16 gusProfWins = 0, gusProfLosses = 0, gusProfDraws = 0;
	UINT8  gubProfCount = 0;
	ChessGameRec gProfHist[CHESS_HIST_MAX] = {};

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
	SGPVObject* guiChessFlags  = nullptr;  // 6 roster flags, 11x7
	SGPVObject* guiChessLogo   = nullptr;  // green pawn, 22 and 14
	SGPVObject* guiChessAdDragon = nullptr; // the Parlour's medallion, borrowed for its ad
	SGPVObject* guiChessAdTiles  = nullptr; // and two of its tiles: the ring and the bird
	// ad impressions: every page load or page change serves the next creative
	UINT32 guiChessAdImpression = 0;
	// real art from the advertisers' own pages, baked to banner size
	SGPVSurface* gAdArtAim     = nullptr;  // the A.I.M. medallion
	SGPVSurface* gAdArtBobby   = nullptr;  // Bobby Ray's own banner creative
	SGPVSurface* gAdArtFuneral = nullptr;  // McGillicutty's, likewise
	SGPVSurface* gAdArtFlower  = nullptr;  // United Floral, likewise
	SGPVSurface* gAdArtIns     = nullptr;  // Malleus, Incus & Stapes

	// a stride-5 walk through the eight creatives: reads as random, never
	// repeats back-to-back, and serves all eight before any comes again;
	// each full cycle starts from a different point
	int ChessAdSlot()
	{
		return int((guiChessAdImpression * 5 + guiChessAdImpression / 9) % 9);
	}
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
	MOUSE_REGION gChessMeRegion;      // the account row at the rail's foot
	MOUSE_REGION gChessFaceRegion[2]; // the row portraits, top and bottom
	int giChessProfSeat = -1;         // -1 you, 0..5 the ladder, -2 the daily man
	SGPVSurface* gProfFace = nullptr; // baked portrait for the open member page
	int giProfFaceFor = -3;
	MOUSE_REGION gChessProfRowRegion[10]; // the feed rows link onward
	int giProfRowTarget[10] = { -3, -3, -3, -3, -3, -3, -3, -3, -3, -3 };
	MOUSE_REGION gChessChallengeRegion;
	bool gfChalModal = false;         // the clock picker over a member page
	MOUSE_REGION gChessChalRegion[6]; // 4 controls, START GAME, nevermind
	SGPVSurface* gSeatChip[7] = {};   // 14px row chips: the ladder + the daily man
	SGPVSurface* ChessSeatChip(int seat);
	MOUSE_REGION gChessBannerRegion;
	MOUSE_REGION gChessAdRegion;
	MOUSE_REGION gChessSignRegion;
	MOUSE_REGION gChessGbPrevRegion;
	MOUSE_REGION gChessGbNextRegion;
	MOUSE_REGION gChessGbPostRegion;
	MOUSE_REGION gChessGbCloseRegion;
	MOUSE_REGION gChessHistRegion[4];  // |< < > >| under the live move lists
	MOUSE_REGION gChessWatchTabRegion[2];  // MOVES | CHAT
	MOUSE_REGION gChessListWheelRegion;    // wheel scroll over the lists
	MOUSE_REGION gChessTimeRegion[4];      // the lobby's time controls
	MOUSE_REGION gChessResignRegion;       // the little flag by the controls
	MOUSE_REGION gChessPostRegion[3];      // new / lobby / rematch, game over
	MOUSE_REGION gChessPModalCloseRegion;  // its X
	MOUSE_REGION gChessPrevDayRegion;
	MOUSE_REGION gChessNextDayRegion;
	MOUSE_REGION gChessModalCloseRegion;
	MOUSE_REGION gChessModalArchiveRegion;
	MOUSE_REGION gChessLearnCtaRegion; // the graduation card's lower button
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
			"PERFECT!", "SOLVED", "OUT OF TRIES", "SEE YOU TOMORROW",
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
			"LESSON {} OF 8", "LIVE - ze house plays itself",
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
			"LEKTION {} VON 8", "LIVE - das Haus spielt gegen sich",
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
	constexpr int CH_COACH_LINES = 6;
	const char* const CHESS_COACH_GOOD[2][CH_COACH_LINES] =
	{
		{ "Ja! That is it.", "Good. Keep going.", "Correct. Do not stop.",
		  "Yes. You see it now.", "So. Not hopeless after all.",
		  "Ja. Ze book agrees with you." },
		{ "Ja! Das ist es.", "Gut. Weiter so.", "Richtig. Nicht aufhoeren.",
		  "Ja. Jetzt sehen Sie es.", "So. Doch nicht hoffnungslos.",
		  "Ja. Das Buch stimmt zu." },
	};
	const char* const CHESS_COACH_BAD[2][CH_COACH_LINES] =
	{
		{ "Nein. Look again.", "That one loses. Think.", "No. Not this piece.",
		  "You are rushing. Stop.", "I saw it too. It is wrong.",
		  "Ze board forgives. I remember." },
		{ "Nein. Schauen Sie nochmal.", "Der verliert. Denken Sie.",
		  "Nein. Nicht diese Figur.", "Sie hetzen. Aufhoeren.",
		  "Ich sah es auch. Es ist falsch.",
		  "Das Brett verzeiht. Ich merke es mir." },
	};
	// said once, when the last heart is all that is left
	const char* const CHESS_COACH_LAST[2] =
	{
		"one heart. breathe. zen move.",
		"ein Herz. atmen. dann ziehen.",
	};

	// -1 asks for an encouraging line, -2 a corrective one; anything else is a
	// fixed string id. Storing the id rather than the text means the language
	// switch re-renders whatever he last said.
	void ChessCoachSay(int what)
	{
		if (what < 0) giChessVariant = (giChessVariant + 1) % CH_COACH_LINES;
		giChessSaid = what;
	}

	UINT32 ChessNow();

	// The bubble types itself: when the line changes, letters appear one by
	// one on a 24ms clock. The full line is cached so a change restarts it.
	ST::string gCoachTyping;
	UINT32 guiCoachTypeStart = 0;

	ST::string ChessCoachTypedLine(const char* full)
	{
		if (gCoachTyping != full)
		{
			gCoachTyping = full;
			guiCoachTypeStart = ChessNow();
		}
		const size_t vis = size_t((ChessNow() - guiCoachTypeStart) / 24) + 1;
		if (vis >= gCoachTyping.size()) return gCoachTyping;
		return ST::string(gCoachTyping.c_str(), vis);
	}

	bool ChessCoachStillTyping()
	{
		return !gCoachTyping.empty() &&
			ChessNow() - guiCoachTypeStart < UINT32(gCoachTyping.size()) * 24;
	}

	// Where he claims each puzzle came from. One line per day, deterministic,
	// in his voice - the sourcing is the flavour, the war is the context.
	const char* const CHESS_COACH_LORE[] =
	{
		"from a book of mates, Leipzig 1887. ze cover is gone.",
		"Ivan showed me zis one. he calls it easy. he lies.",
		"found in a Soviet army magazine, 1974. ze rest was propaganda.",
		"from a newspaper zat no longer exists. like ze country.",
		"scratched into a barracks table in '85. I kept ze table.",
		"a monastery bulletin printed zis. ze monks were not messing around.",
		"clipped from a Metavira paper. do not ask how I got it.",
		"from Madame Layla's waiting room. best magazine in San Mona.",
		"a correspondence game I lost in 1991. now it is your problem.",
		"ze radio read zis position out loud once. I wrote fast.",
		"traded a can of coffee for zis one. good trade.",
		"from a Polish pamphlet Bobby sent. ze spelling was worse than mine.",
		"zis was in my army notebook. ze notebook survived. ze army did not.",
		"a tourist left ze book on ze bus to Chach. finders keepers.",
		"printed on ze back of a ration coupon. hard times, good puzzles.",
		"I do not remember where zis is from. it followed me home.",
		"zis one had me thinking all night. ze night was long anyway.",
		"after solving zis I could finally sleep.",
		"I solved zis in a trench. you have a chair.",
		"took me three coffees. I do not drink coffee.",
		"I showed zis to Ivan. he went quiet. zat never happens.",
		"ze first time I saw zis I resigned. do not resign.",
		"solved it on ze bus to Chach. missed my stop.",
		"I keep zis one for bad days.",
		"I lost a bet on zis position. pay attention.",
		"my notes say 'easy'. my notes were written by a younger man.",
	};
	constexpr int CH_COACH_LORE_COUNT =
		int(sizeof(CHESS_COACH_LORE) / sizeof(CHESS_COACH_LORE[0]));

	const char* ChessCoachLine()
	{
		const int lang = gfChessGerman ? 1 : 0;
		if (giChessSaid == -1) return CHESS_COACH_GOOD[lang][giChessVariant];
		if (giChessSaid == -2) return CHESS_COACH_BAD[lang][giChessVariant];
		if (giChessSaid == -3) return CHESS_COACH_LAST[lang];
		// every day opens on that puzzle's story - provenance or the night it
		// cost him - fresh and archive alike. The notice bar already covers
		// the contract, so the bubble stays his own words.
		if (giChessSaid == CHS_ST_WHITE || giChessSaid == CHS_ST_BLACK ||
		    giChessSaid == CHS_ST_ARCHIVE || giChessSaid == CHS_ST_ALREADY)
		{
			return CHESS_COACH_LORE[(giChessPuzzle * 7 + 3) % CH_COACH_LORE_COUNT];
		}
		return CHESS_TEXT[lang][giChessSaid];
	}

	// chess.com's own cue set, externalized alongside the art
	#define CH_SND_MOVE     SOUNDSDIR "/laptop/chach-move-self.mp3"
	#define CH_SND_OPPONENT SOUNDSDIR "/laptop/chach-move-opponent.mp3"
	#define CH_SND_CAPTURE  SOUNDSDIR "/laptop/chach-capture.mp3"
	#define CH_SND_CHECK    SOUNDSDIR "/laptop/chach-move-check.mp3"
	#define CH_SND_GAMESTART SOUNDSDIR "/laptop/chach-game-start.mp3"
	#define CH_SND_GAMEEND   SOUNDSDIR "/laptop/chach-game-end.mp3"
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
	// A mating move gives check, so it carries the check cue; the game-end
	// sample answers separately when the result card lands.
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

	// Any site's art, baked for a banner: one frame, scaled to a height,
	// composited on the creative's own ground colour.
	SGPVSurface* ChessBakeArtObj(SGPVObject* art, UINT16 frame, INT32 dh, INT32 maxW, UINT32 bg)
	{
		const ETRLEObject& e = art->SubregionProperties(frame);
		const INT32 w = e.usWidth, h = e.usHeight;
		if (w <= 0 || h <= 0) { DeleteVideoObject(art); return nullptr; }
		std::vector<UINT8> pixels(size_t(w) * h, 0);
		const UINT8* in = art->PixData(e);
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
			++in;
		}
		const UINT16* pal = art->Palette16();
		if (!pal) { DeleteVideoObject(art); return nullptr; }
		// scale to the requested height; anything wider than maxW is centre-
		// cropped rather than shrunk, so every creative fills the same frame
		const INT32 fullW = std::max<INT32>(1, w * dh / h);
		const INT32 dw = (maxW > 0 && fullW > maxW) ? maxW : fullW;
		const INT32 cropX = (fullW - dw) / 2;
		SGPVSurface* surf = AddVideoSurface(UINT16(dw), UINT16(dh), PIXEL_DEPTH);
		surf->Fill(Get16BPPColor(bg));
		{
			SGPVSurface::Lock lock(surf);
			UINT16* out = lock.Buffer<UINT16>();
			const UINT32 pitch = lock.Pitch() / 2;
			for (INT32 y = 0; y < dh; ++y)
			{
				for (INT32 x = 0; x < dw; ++x)
				{
					const UINT8 v = pixels[size_t(y * h / dh) * w
					                       + size_t((x + cropX) * w / fullW)];
					if (v) out[y * pitch + x] = pal[v];
				}
			}
		}
		DeleteVideoObject(art);
		return surf;
	}

	SGPVSurface* ChessBakeArt(const char* path, UINT16 frame, INT32 dh, INT32 maxW, UINT32 bg)
	{
		return ChessBakeArtObj(AddVideoObjectFromFile(path), frame, dh, maxW, bg);
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

	// How the regulars actually play. Greed makes them grab a capture just
	// because it is there - but a rated player still sees that the pawn is
	// defended, so the grab comes from the captures that survive the
	// recapture, best first. Blunders are a separate habit: `err` plays a
	// move the search did not choose, drawn from moves that do not simply
	// hand material over. How much material a seat will overlook scales with
	// its rating, which is what stops a 1994 from playing Qxb7??.
	ChessGame::Move ChessPickMoveFor(ChessGame& game, const ChessSeat& seat, UINT32& seed)
	{
		const auto roll = [&seed]() -> UINT32
		{
			seed = seed * 1103515245u + 12345u;
			return seed;
		};

		if (int((roll() >> 16) % 100) < seat.greed)
		{
			ChessGame::Move captures[ChessGame::MAX_MOVES];
			const int nCaps = game.GenerateLegalCaptures(captures);
			ChessGame::Move best;
			int bestSee = 0;
			for (int i = 0; i < nCaps; ++i)
			{
				const int see = game.See(captures[i]);
				if (see < 0) continue;   // the pawn was defended after all
				if (best.IsNull() || see > bestSee) { best = captures[i]; bestSee = see; }
			}
			if (!best.IsNull()) return best;
		}

		ChessGame::SearchParams params;
		params.maxDepth = seat.depth;
		params.msBudget = seat.ms;
		const ChessGame::Move thought = game.SearchTimed(params, seed).move;
		if (seat.err > 0 && int((roll() >> 16) % 100) < seat.err)
		{
			// a pawn to the titled seats, a whole knight to the bottom of
			// the ladder: what the seat is capable of not noticing
			const int tolerance = seat.rating >= 2000 ? 90
			                    : seat.rating >= 1800 ? 150
			                    : seat.rating >= 1600 ? 250
			                                          : 330;
			ChessGame::Move moves[ChessGame::MAX_MOVES];
			const int n = game.GenerateLegal(moves);
			ChessGame::Move pool[ChessGame::MAX_MOVES];
			int nPool = 0;
			for (int i = 0; i < n; ++i)
			{
				if (moves[i] == thought) continue;
				if (game.LosesMaterial(moves[i], tolerance)) continue;
				pool[nPool++] = moves[i];
			}
			if (nPool > 0) return pool[(roll() >> 8) % unsigned(nPool)];
		}
		return thought;
	}

	ChessGame::Move ChessWatchPickMove()
	{
		// ambient chess must never hitch the page: the exhibition plays
		// the same personas with their budgets clamped
		ChessSeat seat = CHESS_SEATS[
			gWatchSeat[gWatchGame.SideToMove() == ChessGame::White ? 0 : 1]];
		if (seat.ms > 150) seat.ms = 150;
		return ChessPickMoveFor(gWatchGame, seat, guiWatchSeed);
	}

	// the peanut gallery: watchers pulled from the site's cast
	const char* const CHESS_CHAT_WHO[] =
	{
		"@dorothy_1938", "@no_refunds", "@the_house", "@e11iot", "@biff_m",
		"@prague_cc", "@chachtourism", "@flo_m", "@a_free_arulco",
	};
	const char* const CHESS_CHAT_IDLE[] =
	{
		"good game so far", "ze center is a mess", "i could beat both of them",
		"slow down, i am taking notes", "zzz", "??", "!!",
		"is this ze crochet stream", "we are STILL not affiliated with this",
		"my money is on ze angry one", "quiet please. thinking along.",
		"i am doing cardio while watching", "anyone else hear shelling",
		"any 1200s here", "hi hi", "wsp", "hello from prague",
		"does ze winner get paid? asking for me",
		"left my laundry in san mona. long story",
		"first time here. last time also.", "who is streaming zis",
		"my rooster is watching too. he disapproves",
		"gg already", "add me for unrated. or rated. anything",
	};
	const char* const CHESS_CHAT_REPLY[] =
	{
		"lol", "true", "agreed", "who asked", "shh. they are thinking.",
		"finally somebody says it", "ok commander", "no",
	};
	const char* const CHESS_CHAT_CAPTURE[] =
	{
		"ouch.", "he will miss zat one", "free material??", "there it goes",
		"house always collects", "i felt that from here",
	};
	const char* const CHESS_CHAT_CHECK[] =
	{
		"check!", "careful now", "run, king, run",
	};
	const char* const CHESS_CHAT_OVER[] =
	{
		"gg", "gg wp", "rematch!", "pay up.", "i called it. nobody heard me.",
	};

	UINT32 guiWatchChatDue  = 0;   // next ambient message
	UINT32 guiWatchReplyDue = 0;   // a pending reply to something you said
	std::string gWatchChatInput;
	constexpr size_t CH_WCHAT_MAX = 60;

	bool ChessWatchChatFocused()
	{
		return (giChessStub == 3 && giWatchTab == 1) ||
		       (giChessStub == 0 && giPlayTab == 1);
	}

	void ChessWatchChatSay(const ST::string& who, const ST::string& text)
	{
		gWatchChat.push_back({ who, text });
		while (gWatchChat.size() > 24) gWatchChat.erase(gWatchChat.begin());
	}

	// what a bot says at its own table: not much, and mostly about moving
	const char* const CHESS_CHAT_OPP[] =
	{
		"hm.", "you talk. i play.", "less typing, more moving.",
		"*nods*", "ze position speaks for itself.", "is zat a threat?",
		"i am thinking. loudly.", "gg soon.",
	};

	void ChessPlayChatSay(const ST::string& who, const ST::string& text)
	{
		gPlayChat.push_back({ who, text });
		while (gPlayChat.size() > 24) gPlayChat.erase(gPlayChat.begin());
	}

	template <size_t N>
	void ChessWatchChatFrom(const char* const (&pool)[N])
	{
		ChessWatchChatSay(CHESS_CHAT_WHO[Random(lengthof(CHESS_CHAT_WHO))],
		                  pool[Random(N)]);
	}

	// A fresh exhibition: new seats, a clean move list, and the game already a
	// few moves in the way a live table should be.
	void ChessWatchNewGame()
	{
		guiWatchSeed = guiWatchSeed * 1103515245u + 12345u;
		guiWatchSeed ^= GetJA2Clock() ^ (Random(0xFFFF) << 12);
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
		gWatchHist.clear();
		gWatchHist.push_back(gWatchGame);
		giWatchView = -1;
		gubWatchFrom = gubWatchTo = ChessGame::NO_SQUARE;
		giWatchResult = 0;
		gWatchChat.clear();
		ChessWatchChatSay(ST::string(),
		                  ST::format("OBSERVING: {} ({}) vs {} ({}) - 10 min",
		                             CHESS_SEATS[gWatchSeat[0]].handle, CHESS_SEATS[gWatchSeat[0]].rating,
		                             CHESS_SEATS[gWatchSeat[1]].handle, CHESS_SEATS[gWatchSeat[1]].rating));
		ChessWatchChatSay(CHESS_CHAT_WHO[Random(lengthof(CHESS_CHAT_WHO))], "new game!");
		ChessWatchChatSay(CHESS_CHAT_WHO[Random(lengthof(CHESS_CHAT_WHO))],
		                  ST::format("my money is on {}", CHESS_SEATS[gWatchSeat[Random(2)]].handle));
		for (int i = 0; i < 6; ++i)
		{
			const ChessGame::Move m = ChessWatchPickMove();
			if (m.IsNull()) break;
			gWatchSan.push_back(gWatchGame.San(m));
			gubWatchFrom = m.from; gubWatchTo = m.to;
			gWatchGame.MakeMove(m);
			gWatchHist.push_back(gWatchGame);
		}
	}

	bool ChessPlayReviewing()
	{
		return giChessStub == 0 && giPlayView >= 0
			&& size_t(giPlayView) < gPlayHist.size();
	}

	ChessGame& ChessActiveGame()
	{
		if (ChessPlayReviewing()) return gPlayHist[size_t(giPlayView)];
		if (giChessStub == 2) return gLearnGame;
		return giChessStub == 0 ? gPlayGame : gChessGame;
	}
	ChessGame::Color ChessActiveSolver()
	{
		if (giChessStub == 2) return gLearnGame.SideToMove();
		return giChessStub == 0 ? ChessGame::White : gChessSolver;
	}
	UINT8 ChessActiveFrom()
	{
		if (ChessPlayReviewing()) return ChessGame::NO_SQUARE;
		return giChessStub == 0 ? gubPlayFrom : gubChessLastFrom;
	}
	UINT8 ChessActiveTo()
	{
		if (ChessPlayReviewing()) return ChessGame::NO_SQUARE;
		return giChessStub == 0 ? gubPlayTo : gubChessLastTo;
	}

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
	p.usRating    = gusProfRating;
	p.usWins      = gusProfWins;
	p.usLosses    = gusProfLosses;
	p.usDraws     = gusProfDraws;
	p.ubHistCount = gubProfCount;
	std::copy(std::begin(gProfHist), std::end(gProfHist), std::begin(p.aHist));
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
	gusProfRating = p.usRating;
	gusProfWins   = p.usWins;
	gusProfLosses = p.usLosses;
	gusProfDraws  = p.usDraws;
	gubProfCount  = std::min<UINT8>(p.ubHistCount, CHESS_HIST_MAX);
	std::copy(std::begin(p.aHist), std::end(p.aHist), std::begin(gProfHist));
	// old saves hold zeroes or noise here; a seat off the ladder is not ours
	for (int i = 0; i < int(gubProfCount); ++i)
	{
		if (gProfHist[i].ubSeat != 0xFE && gProfHist[i].ubSeat >= lengthof(CHESS_SEATS))
		{
			gubProfCount = UINT8(i);
			break;
		}
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

	// A disc and its hollow twin, drawn a scanline at a time off the circle
	// equation. The half-pixel offsets put the centre between pixels, which is
	// what keeps a small disc from coming out square-shouldered.
	void FillDisc(INT32 cx, INT32 cy, double radius, UINT32 rgb)
	{
		const INT32 r = INT32(std::ceil(radius));
		for (INT32 dy = -r; dy < r; ++dy)
		{
			const double y = dy + 0.5;
			if (std::fabs(y) > radius) continue;
			const INT32 half = INT32(std::sqrt(radius * radius - y * y) + 0.5);
			if (half <= 0) continue;
			FillRect(cx - half, cy + dy, half * 2, 1, rgb);
		}
	}

	// The ring chess.com puts around a piece you can take: same circle, with
	// the middle left alone so the piece still reads through it.
	void FillRing(INT32 cx, INT32 cy, double outer, double inner, UINT32 rgb)
	{
		const INT32 r = INT32(std::ceil(outer));
		for (INT32 dy = -r; dy < r; ++dy)
		{
			const double y = dy + 0.5;
			if (std::fabs(y) > outer) continue;
			const INT32 half = INT32(std::sqrt(outer * outer - y * y) + 0.5);
			if (half <= 0) continue;
			const INT32 hole = std::fabs(y) > inner
				? 0 : INT32(std::sqrt(inner * inner - y * y) + 0.5);
			if (hole <= 0)
			{
				FillRect(cx - half, cy + dy, half * 2, 1, rgb);
				continue;
			}
			FillRect(cx - half, cy + dy, half - hole, 1, rgb);
			FillRect(cx + hole, cy + dy, half - hole, 1, rgb);
		}
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
			// the graduation card seats its button lower, under the
			// coach; the puzzle card keeps the original spot
			if (gfLearnModal)
			{
				gChessLearnCtaRegion.Enable();
				gChessModalArchiveRegion.Disable();
			}
			else
			{
				gChessModalArchiveRegion.Enable();
				gChessLearnCtaRegion.Disable();
			}
		}
		else
		{
			gChessModalCloseRegion.Disable();
			gChessModalArchiveRegion.Disable();
			gChessLearnCtaRegion.Disable();
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
			if (gChessState == CHUI_PUZZLE)
			{
				ChessCoachSay(gChessDay.hearts == 1 ? -3 : -2);
			}
		}
		// an illegal drop is silent, as it is on the live site: the piece just
		// goes back
		gbChessSelected = -1;
	}

	void ChessSyncPageRegions();

	UINT32 guiPlayModalDue = 0; // the result modal waits a beat

	// The ledger takes the finished game and moves the rating: honest Elo,
	// with a provisional K while the sample is thin. outcome: 0 loss, 1
	// draw, 2 win, always from your side of the board.
	void ChessRecordGame(int outcome, bool resigned)
	{
		if (giPlaySeat == -1) return;
		const ChessSeat& opp = ChessOpponent();
		const int games = gusProfWins + gusProfLosses + gusProfDraws;
		if (gusProfRating == 0) gusProfRating = 1200;
		const double expected = 1.0 /
			(1.0 + std::pow(10.0, (opp.rating - int(gusProfRating)) / 400.0));
		const int k = games < 10 ? 40 : 16;
		const int r = int(gusProfRating) +
			int(std::lround(k * (outcome * 0.5 - expected)));
		gusProfRating = UINT16(std::clamp(r, 400, 2400));
		if      (outcome == 2) ++gusProfWins;
		else if (outcome == 1) ++gusProfDraws;
		else                   ++gusProfLosses;
		for (int i = std::min<int>(gubProfCount, CHESS_HIST_MAX - 1); i > 0; --i)
		{
			gProfHist[i] = gProfHist[i - 1];
		}
		ChessGameRec& rec = gProfHist[0];
		rec.usDay     = UINT16(GetWorldDay());
		rec.ubSeat    = giPlaySeat == -2 ? 0xFE : UINT8(giPlaySeat);
		rec.ubResult  = UINT8(outcome) | (resigned ? 4 : 0);
		rec.ubMoves   = UINT8(std::min<size_t>(255, (gPlaySan.size() + 1) / 2));
		rec.ubControl = UINT8(giPlayControl);
		if (gubProfCount < CHESS_HIST_MAX) ++gubProfCount;
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
		giPlayEndReason = 0;
		ChessRecordGame(giPlaySaid == CHS_PLAY_WIN  ? 2
		              : giPlaySaid == CHS_PLAY_LOSS ? 0 : 1, false);
		if (giPlaySeat != -1)
		{
			ChessPlayChatSay(ST::string(), "GAME OVER");
			ChessPlayChatSay(ChessOpponent().handle,
			                 giPlaySaid == CHS_PLAY_LOSS ? "gg." : "gg. rematch?");
			// let the final position breathe before the card covers it
			guiPlayModalDue = ChessNow() + 900;
		}
		ChessSyncPageRegions();
	}

	void ChessPlayResign()
	{
		if (giPlayState != 0 && giPlayState != 1 && giPlayState != 5) return;
		giPlaySaid = CHS_PLAY_LOSS;
		giPlayState = 2;
		giPlayEndReason = 1;
		ChessRecordGame(0, true);
		ChessPlayChatSay(ST::string(), "GAME OVER - you resigned");
		if (giPlaySeat != -1) ChessPlayChatSay(ChessOpponent().handle, "gg.");
		gfPlayModal = true;
		ChessPlay(CH_SND_GAMEEND);
		ChessSyncPageRegions();
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
		gPlayHist.push_back(gPlayGame);
		ChessAnimateMove(gPlayGame, m, 110);
		ChessPlay(ChessMoveSound(m, gPlayGame.IsInCheck(gPlayGame.SideToMove()), true));
		ChessPlayFinish();
		if (giPlayState != 2)
		{
			if (giPlayControl == 0)
			{
				giPlayState = 5;
				guiPlayDailyDay = GetWorldDay();
				ChessPlayChatSay(ST::string(), "your move is in. ze reply comes tomorrow.");
			}
			else
			{
				giPlayState = 1;
				giPlaySaid  = CHS_PLAY_THINK;
				guiPlayDue  = ChessNow() + 700 + Random(1400);
			}
		}
	}

	void ChessPlayReset()
	{
		gfPlayModal = false;
		guiResignArmUntil = 0;
		gPlayGame.SetStartPosition();
		gPlaySan.clear();
		gPlayChat.clear();
		guiPlayChatReplyDue = 0;
		gPlayHist.clear();
		gPlayHist.push_back(gPlayGame);
		giPlayView = -1;
		giPlaySeat  = -1;
		guiPlayDue  = 0;
		gubPlayFrom = gubPlayTo = ChessGame::NO_SQUARE;
		gbChessSelected = -1;
		if (gPlayOppFace) { DeleteVideoSurface(gPlayOppFace); gPlayOppFace = nullptr; }
	}

	// back to the lobby: a clean table, a time to choose, a button to press
	void ChessPlayToLobby()
	{
		ChessPlayReset();
		giPlayState = 4;
		guiPlaySeekDue = 0;
		ChessSyncPageRegions();
	}

	void ChessPlayNewGame()
	{
		ChessPlayReset();
		giPlayControl = giPlayMinutes;
		giPlayState = 3;
		giPlaySaid  = CHS_PLAY_SEEK;
		// the letter is answered fast; the pools take a moment
		guiPlaySeekDue = ChessNow() + (giPlayControl == 0 ? 600
		                                                  : 1400 + Random(2000));
		ChessSyncPageRegions();
	}

	// Your opponent plays like themselves: their depth, their blunders, their
	// greed - the same triple the exhibition seats use.
	ChessGame::Move ChessPlayPickMove()
	{
		return ChessPickMoveFor(gPlayGame, ChessOpponent(), guiPlaySeed);
	}

	void ChessSquareCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (ChessPlayReviewing() && (reason & (MSYS_CALLBACK_REASON_POINTER_DWN | MSYS_CALLBACK_REASON_POINTER_UP)))
		{
			giPlayView = -1;
			ChessRedraw();
			return;
		}
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_DWN)) return;
		if (gfChessModal) return;
		const bool playMode  = giChessStub == 0;
		const bool learnMode = giChessStub == 2;
		if (!playMode && !learnMode && giChessStub >= 0) return;
		if (playMode)
		{
			if (giPlayState != 0) return;
		}
		else if (learnMode)
		{
			if (gfLearnSolved) return;
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
			if (playMode)       ChessPlayTryMove(UINT8(gbChessSelected), sq);
			else if (learnMode) ChessLearnTryMove(UINT8(gbChessSelected), sq);
			else                ChessTryMove(UINT8(gbChessSelected), sq);
		}
		ChessRedraw();
	}

	void ChessLearnReset()
	{
		gfLearnSolved = false;
		gzLearnSay    = nullptr;
		gubLearnFrom  = 0x7F;
		gubLearnTo    = 0x7F;
		const ChessGame::Move want =
			gLearnGame.ParseUci(CHESS_LESSONS[giChessLesson].answer);
		gubLearnTarget = want.IsNull() ? 0x7F : want.to;
	}

	void ChessLearnTryMove(UINT8 from, UINT8 to)
	{
		const ChessLesson& lesson = CHESS_LESSONS[giChessLesson];
		ChessGame::Move want = gLearnGame.ParseUci(lesson.answer);
		if (!(want.from == from && want.to == to) && lesson.answer2)
		{
			// some lessons have two equally correct doors
			want = gLearnGame.ParseUci(lesson.answer2);
		}
		gbChessSelected = -1;
		if (!want.IsNull() && want.from == from && want.to == to)
		{
			gLearnGame.MakeMove(want);
			gfLearnSolved = true;
			gubLearnFrom  = from;
			gubLearnTo    = to;
			gzLearnSay    = "ja. exactly zat.";
			ChessPlay(ChessMoveSound(want,
					gLearnGame.IsInCheck(gLearnGame.SideToMove()), true));
			if (giChessLesson == CHESS_LESSON_COUNT - 1)
			{
				gfLearnModal = true;
				ChessSetModal(true);
				ChessPlay(CH_SND_GAMEEND);
			}
		}
		else
		{
			gzLearnSay = lesson.deny;
			ChessPlay(CH_SND_WRONG, LOWVOLUME);
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
		else if (giChessStub == 2)
		{
			ChessLearnTryMove(from, to);
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
			if (giPlayState == 4)      ChessPlayNewGame();   // START GAME
			else if (giPlayState == 2) ChessPlayToLobby();   // NEW GAME
			else return;                                     // seeking: wait
			ChessPlay(CH_SND_CLICK);
			ChessRedraw();
			return;
		}
		if (giChessStub == 2)
		{
			if (gfChessModal) return; // the graduation card owns the click
			if (!gfLearnSolved)
			{
				// the hint: the coach repeats her middle line and the
				// piece in question lights up
				const ChessGame::Move want = gLearnGame.ParseUci(
						CHESS_LESSONS[giChessLesson].answer);
				if (!want.IsNull()) gbChessSelected = INT8(want.from);
				gzLearnSay = CHESS_LESSONS[giChessLesson].lines[1];
				ChessPlay(CH_SND_CLICK);
				ChessRedraw();
				return;
			}
			giChessLesson = (giChessLesson + 1) % CHESS_LESSON_COUNT;
			gLearnGame.SetFen(CHESS_LESSONS[giChessLesson].fen);
			ChessLearnReset();
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
		++guiChessAdImpression;  // stepping either archive is a page load too
		const int step = int(MSYS_GetRegionUserData(region, 0));
		if (giChessStub == 2)
		{
			// same chevrons, different book: lesson to lesson
			const int wantLesson = giChessLesson + step;
			if (wantLesson < 0 || wantLesson >= CHESS_LESSON_COUNT) return;
			giChessLesson = wantLesson;
			gLearnGame.SetFen(CHESS_LESSONS[giChessLesson].fen);
			ChessLearnReset();
			ChessPlay(CH_SND_CLICK, LOWVOLUME);
			ChessRedraw();
			return;
		}
		if (giChessStub >= 0) return;
		const int want = giChessViewDay + step;
		if (want < 1 || want > int(ChessToday())) return;
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		ChessShowDay(want);
		ChessRedraw();
	}

	// releasing anywhere resolves the drop from the pointer's position: a
	// square lands the move, anywhere else snaps the piece home but keeps
	// it selected. Touch delivers its button-up through this region, so a
	// cancel here used to kill every touch drag.
	void ChessDropCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfChessDragging) return;
		ChessResolveDrop();
	}

	void ChessModalCloseCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfChessModal) return;
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		gfLearnModal = false;
		ChessSetModal(false);
		ChessRedraw();
	}

	void ChessModalArchiveCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfChessModal) return;
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		ChessSetModal(false);
		if (gfLearnModal)
		{
			// graduation: the card takes its bow and the book stays open
			gfLearnModal = false;
			ChessRedraw();
			return;
		}
		// a plain dismissal: the next puzzle arrives with tomorrow, and
		// the archive chevrons are right there for the impatient
		ChessRedraw();
	}

	void ChessSyncPageRegions();

	void ChessNavCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const int item = int(MSYS_GetRegionUserData(region, 0));
		// Puzzles is the site; everything else is a page he has not written
		const int want = (item == 1) ? -1 : item;
		if (want != giChessStub)
		{
			ChessPlay(CH_SND_CLICK2, LOWVOLUME);
			++guiChessAdImpression;
		}
		if (gfLearnModal)
		{
			gfLearnModal = false;
			ChessSetModal(false);
		}
		giChessStub = want;
		gChessHintRegion.SetFastHelpText(want < 0 ? "Costs one attempt" : ST::string());
		giMoveScroll = 0;
		giChatScroll = 0;
		ChessSyncPageRegions();

		if (want == 2)
		{
			gLearnGame.SetFen(CHESS_LESSONS[giChessLesson].fen);
			ChessLearnReset();
		}
		if (want == 3 && guiWatchNextMove == 0)
		{
			ChessWatchNewGame();
			guiWatchNextMove = ChessNow() + 1600 + Random(1800);
		}
		ChessRedraw();
	}

	SGPVSurface* ChessSeatChip(int seat)
	{
		const int idx = seat == -2 ? 6 : seat;
		if (idx < 0 || idx > 6) return nullptr;
		if (!gSeatChip[idx])
		{
			const ChessSeat& who = seat == -2 ? CHESS_SEAT_ENRICO
			                                  : CHESS_SEATS[seat];
			try
			{
				SGPVObject* face = ChessLoadPortrait(GetProfile(who.pid));
				gSeatChip[idx] = ChessBakeFace(face, 14);
				DeleteVideoObject(face);
			}
			catch (...) {}
		}
		return gSeatChip[idx];
	}

	// one door to any member page: yours (-1), a ladder seat, or the
	// daily man (-2). Seat portraits bake lazily and stick around.
	void ChessOpenProfile(int seat)
	{
		gfChalModal = false;
		if (seat != -1 && giProfFaceFor != seat)
		{
			if (gProfFace) { DeleteVideoSurface(gProfFace); gProfFace = nullptr; }
			const ChessSeat& who = seat == -2 ? CHESS_SEAT_ENRICO
			                                  : CHESS_SEATS[seat];
			try
			{
				SGPVObject* face = ChessLoadPortrait(GetProfile(who.pid));
				gProfFace = ChessBakeFace(face, CH_GB_FACE);
				DeleteVideoObject(face);
			}
			catch (...) { gProfFace = nullptr; }
			giProfFaceFor = seat;
		}
		giChessProfSeat = seat;
		if (giChessStub != 5)
		{
			ChessPlay(CH_SND_CLICK2, LOWVOLUME);
			++guiChessAdImpression;
		}
		if (gfLearnModal)
		{
			gfLearnModal = false;
			ChessSetModal(false);
		}
		giChessStub = 5;
		gChessHintRegion.SetFastHelpText(ST::string());
		giMoveScroll = 0;
		giChatScroll = 0;
		ChessSyncPageRegions();
		ChessRedraw();
	}

	// the account row at the rail's foot is the door to your own page
	void ChessMeCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		ChessOpenProfile(-1);
	}

	void ChessProfRowCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giChessStub != 5 || gfChalModal) return;
		const int t = giProfRowTarget[MSYS_GetRegionUserData(region, 0)];
		if (t < -2) return;
		ChessOpenProfile(t);
	}

	void ChessPlayNewGame();

	void ChessChallengeCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giChessStub != 5 || gfChalModal) return;
		if (giChessProfSeat < 0) return; // the daily man plays by post
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		gfChalModal = true;
		ChessSyncPageRegions();
		ChessRedraw();
	}

	void ChessChalOptCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfChalModal || giChessStub != 5) return;
		const int i = int(MSYS_GetRegionUserData(region, 0));
		if (i < 4)
		{
			// picking a control mirrors the lobby: select, stay open
			ChessPlay(CH_SND_CLICK2, LOWVOLUME);
			giPlayMinutes = CH_PLAY_TIMES[i].mins;
			ChessRedraw();
			return;
		}
		gfChalModal = false;
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		if (i == 5)
		{
			// nevermind: the page comes back up
			ChessSyncPageRegions();
			ChessRedraw();
			return;
		}
		giPlayRematchSeat = giChessProfSeat;
		giChessStub = 0;
		ChessPlayNewGame();
		ChessSyncPageRegions();
		ChessRedraw();
	}

	// the row portraits open whoever is sitting there
	void ChessFaceCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gfChessModal || gfPlayModal) return;
		const int row = int(MSYS_GetRegionUserData(region, 0)); // 0 top, 1 bottom
		if (giChessStub == 0)
		{
			if (row == 1) { ChessOpenProfile(-1); return; }
			if (giPlayState == 3 || giPlayState == 4 || giPlaySeat == -1) return;
			ChessOpenProfile(giPlaySeat);
			return;
		}
		if (giChessStub == 3)
		{
			ChessOpenProfile(gWatchSeat[row == 0 ? 1 : 0]);
		}
	}

	void ChessBannerCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		// the guestbook's sidebar ad answers too; the other pages' banner does not
		if (gfChessModal || (giChessStub >= 0 && giChessStub != 4)) return;
		// only the crown creative answers, and he answers exactly once
		if (ChessAdSlot() != 1) return;
		if (gChessDay.flags & ChessDaily::FLAG_CROWN_ASKED) return;
		gChessDay.flags |= ChessDaily::FLAG_CROWN_ASKED;
		ChessPlay(CH_SND_CLICK, LOWVOLUME);
		ChessMail(5, 0);
	}

	void ChessPlayResign();
	void ChessPlayToLobby();
	void ChessPlayNewGame();

	void ChessResignCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giChessStub != 0) return;
		if (giPlayState != 0 && giPlayState != 1 && giPlayState != 5) return;
		if (ChessNow() < guiResignArmUntil)
		{
			guiResignArmUntil = 0;
			ChessPlayResign();
			ChessRedraw();
			return;
		}
		// the flag asks twice: arm it, and it stays lit a moment
		guiResignArmUntil = ChessNow() + 2500;
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	void ChessPostCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giChessStub != 0 || giPlayState != 2 || !gfPlayModal) return;
		const int what = int(MSYS_GetRegionUserData(region, 0));
		gfPlayModal = false;
		if (what == 1)
		{
			ChessPlayToLobby();
		}
		else
		{
			if (what == 2 && giPlaySeat != -1) giPlayRematchSeat = giPlaySeat;
			giPlayMinutes = giPlayControl;
			ChessPlayNewGame();
		}
		ChessPlay(CH_SND_CLICK);
		ChessRedraw();
	}

	void ChessPModalCloseCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfPlayModal) return;
		gfPlayModal = false;
		ChessSyncPageRegions();
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	void ChessTimeCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giChessStub != 0 || giPlayState != 4) return;
		giPlayMinutes = CH_PLAY_TIMES[MSYS_GetRegionUserData(region, 0)].mins;
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	void ChessListWheelCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & (MSYS_CALLBACK_REASON_WHEEL_UP | MSYS_CALLBACK_REASON_WHEEL_DOWN))) return;
		const int dir = (reason & MSYS_CALLBACK_REASON_WHEEL_UP) ? 2 : -2;
		int* scroll = ChessWatchChatFocused() ? &giChatScroll : &giMoveScroll;
		*scroll += dir;
		if (*scroll < 0) *scroll = 0;  // the render clamps the far end
		ChessRedraw();
	}

	void ChessWatchTabCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		int* tab;
		if      (giChessStub == 3) tab = &giWatchTab;
		else if (giChessStub == 0) tab = &giPlayTab;
		else return;
		const int want = int(MSYS_GetRegionUserData(region, 0));
		if (want == *tab) return;
		*tab = want;
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	void ChessHistCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		int* view;
		const std::vector<ChessGame>* hist;
		if      (giChessStub == 0) { view = &giPlayView;  hist = &gPlayHist; }
		else if (giChessStub == 3) { view = &giWatchView; hist = &gWatchHist; }
		else return;
		const int last = int(hist->size()) - 1;
		if (last < 0) return;
		int cur = (*view < 0 || *view > last) ? last : *view;
		switch (int(MSYS_GetRegionUserData(region, 0)))
		{
			case 0: cur = 0; break;
			case 1: if (cur > 0) --cur; break;
			case 2: if (cur < last) ++cur; break;
			default: cur = last; break;
		}
		*view = cur >= last ? -1 : cur;
		gbChessSelected = -1;  // a piece in hand has no meaning in the past
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
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
		if (gb || giChessStub == 5) gChessBannerRegion.Disable();
		else gChessBannerRegion.Enable();
		const bool list = gb && !gfChessGbCompose;
		MOUSE_REGION* book[] = { &gChessSignRegion,
		                         &gChessGbPrevRegion, &gChessGbNextRegion };
		for (MOUSE_REGION* r : book) { if (list) r->Enable(); else r->Disable(); }
		// the profile pages share the guestbook's ad column
		if (list || giChessStub == 5) gChessAdRegion.Enable();
		else gChessAdRegion.Disable();
		for (MOUSE_REGION& r : gChessGbNumRegion) { if (list) r.Enable(); else r.Disable(); }
		const bool compose = gb && gfChessGbCompose;
		if (compose) { gChessGbPostRegion.Enable();  gChessGbCloseRegion.Enable(); }
		else         { gChessGbPostRegion.Disable(); gChessGbCloseRegion.Disable(); }
		const bool playOngoing = giPlayState == 0 || giPlayState == 1 ||
		                         giPlayState == 5;
		const bool scrub = (giChessStub == 0 && playOngoing) || giChessStub == 3;
		for (MOUSE_REGION& r : gChessHistRegion) { if (scrub) r.Enable(); else r.Disable(); }
		for (MOUSE_REGION& r : gChessWatchTabRegion)
		{
			if (giChessStub == 3 || giChessStub == 0) r.Enable(); else r.Disable();
		}
		if (giChessStub == 0 || giChessStub == 3) gChessListWheelRegion.Enable();
		else gChessListWheelRegion.Disable();
		for (MOUSE_REGION& r : gChessTimeRegion)
		{
			if (giChessStub == 0 && giPlayState == 4) r.Enable(); else r.Disable();
		}
		if (giChessStub == 0 && playOngoing) gChessResignRegion.Enable();
		else gChessResignRegion.Disable();
		const bool card = giChessStub == 0 && giPlayState == 2 && gfPlayModal;
		for (MOUSE_REGION& r : gChessPostRegion) { if (card) r.Enable(); else r.Disable(); }
		if (card) gChessPModalCloseRegion.Enable(); else gChessPModalCloseRegion.Disable();
		// the same strip is either the controls or the button, never both
		if (giChessStub == 0 && playOngoing) gChessHintRegion.Disable();
		else gChessHintRegion.Enable();
		const bool faces = giChessStub == 0 || giChessStub == 3;
		for (MOUSE_REGION& r : gChessFaceRegion)
		{
			if (faces) r.Enable(); else r.Disable();
		}
		for (MOUSE_REGION& r : gChessProfRowRegion)
		{
			if (giChessStub == 5) r.Enable(); else r.Disable();
		}
		if (giChessStub != 5) gfChalModal = false;
		if (giChessStub == 5 && giChessProfSeat >= 0 && !gfChalModal)
		{
			gChessChallengeRegion.Enable();
		}
		else
		{
			gChessChallengeRegion.Disable();
		}
		for (MOUSE_REGION& r : gChessChalRegion)
		{
			if (giChessStub == 5 && gfChalModal) r.Enable(); else r.Disable();
		}
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

		MSYS_DefineRegion(&gChessMeRegion,
		                  UINT16(CH_X(CH_NAV_X)), UINT16(CH_Y(CH_PAGE_H - 40)),
		                  UINT16(CH_X(CH_NAV_X + CH_NAV_W)), UINT16(CH_Y(CH_PAGE_H)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessMeCallback);
		gChessMeRegion.SetFastHelpText("your page");

		for (int i = 0; i < 2; ++i)
		{
			const INT32 fx = CH_BOARD_X + (CH_SQ - CH_ROW_FACE) / 2;
			const INT32 fy = (i == 0 ? CH_ROW_TOP_Y : CH_ROW_BOT_Y) +
			                 (CH_SQ - CH_ROW_FACE) / 2;
			MSYS_DefineRegion(&gChessFaceRegion[i],
			                  UINT16(CH_X(fx)), UINT16(CH_Y(fy)),
			                  UINT16(CH_X(fx + CH_ROW_FACE)),
			                  UINT16(CH_Y(fy + CH_ROW_FACE)),
			                  MSYS_PRIORITY_HIGH + 1, CURSOR_WWW,
			                  MSYS_NO_CALLBACK, ChessFaceCallback);
			MSYS_SetRegionUserData(&gChessFaceRegion[i], 0, i);
			gChessFaceRegion[i].SetFastHelpText("view profile");
			gChessFaceRegion[i].Disable();
		}

		for (int i = 0; i < 10; ++i)
		{
			// the feed rows: parked at a dummy spot, the renderer moves
			// them onto whatever it draws
			MSYS_DefineRegion(&gChessProfRowRegion[i],
			                  UINT16(CH_X(CH_BOARD_X)), UINT16(CH_Y(0)),
			                  UINT16(CH_X(CH_BOARD_X + 1)), UINT16(CH_Y(1)),
			                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
			                  ChessProfRowCallback);
			MSYS_SetRegionUserData(&gChessProfRowRegion[i], 0, i);
			gChessProfRowRegion[i].SetFastHelpText("view profile");
			gChessProfRowRegion[i].Disable();
		}
		MSYS_DefineRegion(&gChessChallengeRegion,
		                  UINT16(CH_X(CH_BOARD_X + 12)),
		                  UINT16(CH_Y(CH_GB_TOP + 44)),
		                  UINT16(CH_X(CH_BOARD_X + 12 + 110)),
		                  UINT16(CH_Y(CH_GB_TOP + 66)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessChallengeCallback);
		gChessChallengeRegion.Disable();
		{
			// the picker: four control rows, the start button, the way out
			const INT32 mx = CH_BOARD_X + (CH_BOARD_SIZE - 160) / 2;
			for (int i = 0; i < 6; ++i)
			{
				const INT32 oy = i < 4 ? 122 + i * 30
				               : i == 4 ? 246 : 282;
				const INT32 oh = i < 4 ? 24 : i == 4 ? CH_HINT_H : 14;
				MSYS_DefineRegion(&gChessChalRegion[i],
				                  UINT16(CH_X(mx + 16)), UINT16(CH_Y(oy)),
				                  UINT16(CH_X(mx + 144)),
				                  UINT16(CH_Y(oy + oh)),
				                  MSYS_PRIORITY_HIGH + 2, CURSOR_WWW,
				                  MSYS_NO_CALLBACK, ChessChalOptCallback);
				MSYS_SetRegionUserData(&gChessChalRegion[i], 0, i);
				gChessChalRegion[i].Disable();
			}
		}

		MSYS_DefineRegion(&gChessModalCloseRegion,
		                  UINT16(CH_X(CH_MODAL_X + CH_MODAL_W - 20)), UINT16(CH_Y(CH_MODAL_Y + 2)),
		                  UINT16(CH_X(CH_MODAL_X + CH_MODAL_W - 2)),  UINT16(CH_Y(CH_MODAL_Y + 20)),
		                  MSYS_PRIORITY_HIGHEST, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessModalCloseCallback);
		MSYS_DefineRegion(&gChessModalArchiveRegion,
		                  UINT16(CH_X(CH_MODAL_X + 12)), UINT16(CH_Y(CH_MODAL_Y + 46)),
		                  UINT16(CH_X(CH_MODAL_X + CH_MODAL_W - 12)), UINT16(CH_Y(CH_MODAL_Y + 74)),
		                  MSYS_PRIORITY_HIGHEST, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessModalArchiveCallback);
		MSYS_DefineRegion(&gChessLearnCtaRegion,
		                  UINT16(CH_X(CH_MODAL_X + 12)), UINT16(CH_Y(CH_MODAL_Y + 66)),
		                  UINT16(CH_X(CH_MODAL_X + CH_MODAL_W - 12)), UINT16(CH_Y(CH_MODAL_Y + 94)),
		                  MSYS_PRIORITY_HIGHEST, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessModalArchiveCallback);
		gChessModalCloseRegion.Disable();
		gChessModalArchiveRegion.Disable();
		gChessLearnCtaRegion.Disable();

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
		// the resign flag, left of the scrubber in the footer band
		MSYS_DefineRegion(&gChessResignRegion,
		                  UINT16(CH_X(CH_PANEL_X + 8)), UINT16(CH_Y(CH_FOOT_Y + 9)),
		                  UINT16(CH_X(CH_PANEL_X + 28)), UINT16(CH_Y(CH_FOOT_Y + 29)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessResignCallback);
		// the finished-game card's three actions and its X
		for (int i = 0; i < 3; ++i)
		{
			const INT32 mx2 = CH_MODAL_X, my2 = CH_BOARD_Y + (CH_BOARD_SIZE - 148) / 2;
			UINT16 x0, y0, x1, y1;
			if (i == 0) { x0 = UINT16(mx2 + 12); y0 = UINT16(my2 + 92); x1 = UINT16(mx2 + 192); y1 = UINT16(my2 + 114); }
			else
			{
				x0 = UINT16(mx2 + 12 + (i - 1) * 94); y0 = UINT16(my2 + 120);
				x1 = UINT16(x0 + 86); y1 = UINT16(my2 + 140);
			}
			MSYS_DefineRegion(&gChessPostRegion[i],
			                  UINT16(CH_X(x0)), UINT16(CH_Y(y0)),
			                  UINT16(CH_X(x1)), UINT16(CH_Y(y1)),
			                  MSYS_PRIORITY_HIGHEST, CURSOR_WWW, MSYS_NO_CALLBACK,
			                  ChessPostCallback);
			MSYS_SetRegionUserData(&gChessPostRegion[i], 0, i);
		}
		{
			const INT32 mx2 = CH_MODAL_X, my2 = CH_BOARD_Y + (CH_BOARD_SIZE - 148) / 2;
			MSYS_DefineRegion(&gChessPModalCloseRegion,
			                  UINT16(CH_X(mx2 + 182)), UINT16(CH_Y(my2 + 2)),
			                  UINT16(CH_X(mx2 + 202)), UINT16(CH_Y(my2 + 22)),
			                  MSYS_PRIORITY_HIGHEST, CURSOR_WWW, MSYS_NO_CALLBACK,
			                  ChessPModalCloseCallback);
		}
		for (int i = 0; i < 4; ++i)
		{
			const INT32 bx2 = CH_PANEL_X + 8;
			const INT32 by2 = 60 + i * 30;
			MSYS_DefineRegion(&gChessTimeRegion[i],
			                  UINT16(CH_X(bx2)), UINT16(CH_Y(by2)),
			                  UINT16(CH_X(bx2 + CH_PANEL_W - 16)), UINT16(CH_Y(by2 + 24)),
			                  MSYS_PRIORITY_HIGH + 1, CURSOR_WWW, MSYS_NO_CALLBACK,
			                  ChessTimeCallback);
			MSYS_SetRegionUserData(&gChessTimeRegion[i], 0, i);
		}
		MSYS_DefineRegion(&gChessListWheelRegion,
		                  UINT16(CH_X(CH_PANEL_X + 4)), UINT16(CH_Y(56)),
		                  UINT16(CH_X(CH_PANEL_X + CH_PANEL_W - 4)), UINT16(CH_Y(CH_FOOT_Y - 2)),
		                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
		                  ChessListWheelCallback);
		for (int i = 0; i < 2; ++i)
		{
			const INT32 tx = CH_PANEL_X + i * (CH_PANEL_W / 2);
			MSYS_DefineRegion(&gChessWatchTabRegion[i],
			                  UINT16(CH_X(tx)), UINT16(CH_Y(CH_INSET + 24)),
			                  UINT16(CH_X(tx + CH_PANEL_W / 2)), UINT16(CH_Y(CH_INSET + 42)),
			                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
			                  ChessWatchTabCallback);
			MSYS_SetRegionUserData(&gChessWatchTabRegion[i], 0, i);
		}
		for (int i = 0; i < 4; ++i)
		{
			const INT32 hx = CH_PANEL_X + CH_PANEL_W / 2 - 41 + i * 28;
			MSYS_DefineRegion(&gChessHistRegion[i],
			                  UINT16(CH_X(hx)), UINT16(CH_Y(CH_FOOT_Y + 9)),
			                  UINT16(CH_X(hx + 22)), UINT16(CH_Y(CH_FOOT_Y + 29)),
			                  MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
			                  ChessHistCallback);
			MSYS_SetRegionUserData(&gChessHistRegion[i], 0, i);
		}
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
		MSYS_RemoveRegion(&gChessLearnCtaRegion);
		for (MOUSE_REGION& r : gChessNavRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessMeRegion);
		for (MOUSE_REGION& r : gChessFaceRegion) MSYS_RemoveRegion(&r);
		for (MOUSE_REGION& r : gChessProfRowRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessChallengeRegion);
		for (MOUSE_REGION& r : gChessChalRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessBannerRegion);
		MSYS_RemoveRegion(&gChessAdRegion);
		MSYS_RemoveRegion(&gChessGbPrevRegion);
		MSYS_RemoveRegion(&gChessGbNextRegion);
		for (MOUSE_REGION& r : gChessGbNumRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessGbPostRegion);
		MSYS_RemoveRegion(&gChessGbCloseRegion);
		for (MOUSE_REGION& r : gChessHistRegion) MSYS_RemoveRegion(&r);
		for (MOUSE_REGION& r : gChessWatchTabRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessListWheelRegion);
		for (MOUSE_REGION& r : gChessTimeRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessResignRegion);
		for (MOUSE_REGION& r : gChessPostRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gChessPModalCloseRegion);
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
		if (giChessStub == 5 && giChessProfSeat == -1)
		{
			FillRounded(CH_NAV_X + 2, faceY - 4, CH_NAV_W - 4, faceH + 8,
			            CH_RGB_PANEL_UP, 3, CH_RGB_PANEL);
		}
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
		// a king standing in check gets his square tinted red
		UINT8 checkSq = ChessGame::NO_SQUARE;
		if (game.IsInCheck(game.SideToMove()))
		{
			checkSq = game.KingSquare(game.SideToMove());
		}
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
				if (sq == checkSq) rgb = light ? CH_RGB_CHK_LIGHT : CH_RGB_CHK_DARK;
				if (gfChessModal)
				{
					rgb = lit ? (light ? CH_RGB_HL_LIGHT_DIM : CH_RGB_HL_DARK_DIM)
					          : (light ? CH_RGB_LIGHT_DIM : CH_RGB_DARK_DIM);
				}
				FillRect(CH_BOARD_X + col * CH_SQ, CH_BOARD_Y + row * CH_SQ, CH_SQ, CH_SQ, rgb);
			}
		}

		// the man under the pointer wears a ring when he has a move:
		// the affordance that says this one is yours to pick up
		if (&game == &ChessActiveGame() && !gfChessModal &&
		    !ChessPlayReviewing() && !gfChessDragging &&
		    game.SideToMove() == ChessActiveSolver() &&
		    (gChessState != CHUI_PUZZLE || guiChessReplyDue == 0))
		{
			const UINT8 hov = ChessSquareUnderPointer();
			if (hov < 64)
			{
				ChessGame::Move moves[256];
				const int n =
					const_cast<ChessGame&>(game).GenerateLegal(moves);
				bool can = false;
				for (int i = 0; i < n && !can; ++i)
				{
					can = moves[i].from == hov;
				}
				if (can)
				{
					INT32 x, y;
					SquareToScreen(hov, x, y);
					const UINT32 ring = FROMRGB(252, 252, 250);
					FillRect(x, y, CH_SQ, 2, ring);
					FillRect(x, y + CH_SQ - 2, CH_SQ, 2, ring);
					FillRect(x, y, 2, CH_SQ, ring);
					FillRect(x + CH_SQ - 2, y, 2, CH_SQ, ring);
				}
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
				const INT32 cx = x + CH_SQ / 2;
				const INT32 cy = y + CH_SQ / 2;
				if (game.IsEmpty(moves[i].to))
				{
					// an empty square takes the dot, a sixth of the square wide
					FillDisc(cx, cy, CH_SQ / 6.0, dot);
				}
				else
				{
					// an occupied one takes the ring, hugging the square edge
					FillRing(cx, cy, CH_SQ / 2.0, CH_SQ / 2.0 - 3.0, dot);
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

	// The coach's portrait, decoded by hand with a channel lift: the
	// sample's palette runs dark against the panel, so r, g and b each
	// gain thirty percent, clamped, on the way to the screen.
	void ChessBltCoachFace(INT32 px, INT32 py)
	{
		if (!guiChessCoach || guiChessCoach->SubregionCount() == 0) return;
		const ETRLEObject& e = guiChessCoach->SubregionProperties(0);
		const UINT16* pal = guiChessCoach->Palette16();
		if (!pal) return;
		const UINT8* in = guiChessCoach->PixData(e);
		SGPVSurface::Lock lock(FRAME_BUFFER);
		UINT16* buf = lock.Buffer<UINT16>();
		const UINT32 pitch = lock.Pitch() / 2;
		for (INT32 y = 0; y < e.usHeight; ++y)
		{
			INT32 x = 0;
			while (*in != 0)
			{
				const UINT8 code = *in++;
				const UINT8 run = code & 0x7F;
				if (code & 0x80) { x += run; continue; }
				for (UINT8 k = 0; k < run; ++k, ++x)
				{
					const UINT16 c = pal[*in++];
					INT32 r = ((c >> 11) & 31) * 13 / 10;
					INT32 g = ((c >> 5) & 63) * 13 / 10;
					INT32 b = (c & 31) * 13 / 10;
					if (r > 31) r = 31;
					if (g > 63) g = 63;
					if (b > 31) b = 31;
					buf[UINT32(CH_Y(py + y)) * pitch +
							UINT32(CH_X(px + x))] =
						UINT16((r << 11) | (g << 5) | b);
				}
			}
			++in;
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
			ChessBltCoachFace(faceX, y);
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
		                     ChessCoachTypedLine(ChessCoachLine()), FONT_MCOLOR_WHITE, LEFT_JUSTIFIED);
	}

	void ChessRenderMoveList(const std::vector<ST::string>& san, INT32 y0, INT32 y1,
	                         int* scroll, int view = -1);
	void ChessDrawCTAButton(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 bg);
	INT32 ChessRenderSectionPanel(UINT16 icon, ChessStr title);
	void ChessDrawPagerButton(INT32 x, INT32 y, INT32 s, bool lit);
	void ChessDrawDot(INT32 x, INT32 y, UINT32 rgb);
	void ChessRenderPanelTabs(int active);
	void ChessRenderChatPanel(const std::vector<WatchChatLine>& log,
	                          INT32 inY = CH_FOOT_Y - 16);
	INT32 ChessDrawTitleBadge(INT32 x, INT32 y, const char* title, UINT32 bg);
	void ChessDrawGreyButton(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 bg, bool live);

	// one TIME CONTROL row, badge and all - the lobby and the challenge
	// picker draw the same furniture
	void ChessDrawControlRow(int i, INT32 bx2, INT32 by2, INT32 bw2, UINT32 bg)
	{
		const PlayControl& pc = CH_PLAY_TIMES[i];
		const bool on = giPlayMinutes == pc.mins;
		if (on) FillRounded(bx2 - 1, by2 - 1, bw2 + 2, 26, CH_RGB_CTA, 4, bg);
		ChessDrawGreyButton(bx2, by2, bw2, 24, bg, on);
		// the badge: bullet streak, blitz bolt, rapid watch, daily sun
		const INT32 ix = bx2 + 8, iy = by2 + 7;
		switch (i)
		{
			case 0:
				FillRect(ix + 5, iy, 4, 3, FROMRGB(214, 140, 60));
				FillRect(ix + 2, iy + 3, 4, 3, FROMRGB(214, 140, 60));
				FillRect(ix, iy + 6, 3, 3, FROMRGB(170, 108, 44));
				break;
			case 1:
				// a proper bolt: head high right, wide jag, tail point
				FillRect(ix + 5, iy - 1, 4, 3, FROMRGB(240, 210, 70));
				FillRect(ix + 3, iy + 2, 4, 2, FROMRGB(240, 210, 70));
				FillRect(ix, iy + 4, 7, 2, FROMRGB(240, 210, 70));
				FillRect(ix + 2, iy + 6, 3, 2, FROMRGB(214, 180, 52));
				FillRect(ix, iy + 8, 2, 2, FROMRGB(214, 180, 52));
				break;
			case 2:
				FillRounded(ix, iy + 1, 9, 9, FROMRGB(129, 182, 76), 4,
				            FROMRGB(66, 61, 56));
				FillRect(ix + 3, iy - 1, 3, 2, FROMRGB(129, 182, 76));
				FillRect(ix + 3, iy + 4, 3, 3, FROMRGB(36, 33, 30));
				break;
			default:
				FillRect(ix + 2, iy + 2, 5, 5, FROMRGB(226, 186, 60));
				FillRect(ix + 4, iy, 1, 2, FROMRGB(226, 186, 60));
				FillRect(ix + 4, iy + 7, 1, 2, FROMRGB(226, 186, 60));
				FillRect(ix, iy + 4, 2, 1, FROMRGB(226, 186, 60));
				FillRect(ix + 7, iy + 4, 2, 1, FROMRGB(226, 186, 60));
				break;
		}
		// the duration is the decision, chess.com fashion: it leads; the
		// format name trails as the detail
		PrintAt(FONT10ARIALBOLD, on ? FONT_MCOLOR_WHITE : FONT_GRAY4,
		        bx2 + 24, by2 + 7, pc.time);
		const ST::string t = pc.name;
		PrintAt(FONT10ARIAL, on ? FONT_MCOLOR_WHITE : FONT_GRAY6,
		        bx2 + bw2 - 8 - StringPixLength(t, FONT10ARIAL), by2 + 7, t);
	}

	// The right panel while a live game runs: the opponent, the state of
	// play, and the one button.
	// chess.com's board controls: |< < > >| in the pager buttons' dress.
	// Arrows grey out at the live end and at the start of the game.
	void ChessRenderScrubber(const std::vector<ChessGame>& hist, int view)
	{
		const INT32 cx = CH_PANEL_X + CH_PANEL_W / 2;
		const INT32 y = CH_FOOT_Y + 10;
		const int last = int(hist.size()) - 1;
		const int cur = (view < 0 || view > last) ? last : view;
		for (int i = 0; i < 4; ++i)
		{
			const INT32 bx = cx - 41 + i * 28;
			const bool en = (i < 2) ? cur > 0 : cur < last;
			ChessDrawGreyButton(bx, y - 1, 22, 20, CH_RGB_PANEL_SUNK, en);
			const UINT32 col = en ? FROMRGB(190, 185, 178) : FROMRGB(96, 90, 83);
			switch (i)
			{
				case 0:
					FillRect(bx + 5, y + 4, 2, 9, col);
					ChessDrawChevron(bx + 14, y + 8, true, col);
					break;
				case 1: ChessDrawChevron(bx + 11, y + 8, true, col); break;
				case 2: ChessDrawChevron(bx + 11, y + 8, false, col); break;
				default:
					ChessDrawChevron(bx + 8, y + 8, false, col);
					FillRect(bx + 15, y + 4, 2, 9, col);
					break;
			}
		}
	}

	void ChessRenderPlayPanel()
	{
		const INT32 cx = ChessRenderSectionPanel(CH_ICON_PLAY, CHS_NAV_PLAY);
		FillRect(CH_PANEL_X, CH_FOOT_Y, CH_PANEL_W, 38, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_PANEL_RADIUS, CH_RGB_CHROME);

		if (giPlayState == 4)
		{
			// the lobby: the ladder stacked in rows, each with its badge
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, 46, "TIME CONTROL");
			for (int i = 0; i < 4; ++i)
			{
				ChessDrawControlRow(i, CH_PANEL_X + 8, 60 + i * 30,
				                    CH_PANEL_W - 16, CH_RGB_PANEL);
			}
		}
		else if (giPlayState == 3)
		{
			// a ring of dots, one bright and running - the 1999 spinner
			const INT32 scy = (CH_INSET + 24 + CH_FOOT_Y) / 2;  // centred in the panel body
			static const INT32 ring[8][2] =
			{
				{ 0, -12 }, { 8, -8 }, { 12, 0 }, { 8, 8 },
				{ 0, 12 }, { -8, 8 }, { -12, 0 }, { -8, -8 },
			};
			const int phase = int((ChessNow() / 120) % 8);
			for (int i = 0; i < 8; ++i)
			{
				const int back = (phase - i + 8) % 8;
				const UINT32 col = back == 0 ? FROMRGB(200, 195, 188)
				                 : back == 1 ? FROMRGB(140, 134, 127)
				                 : back == 2 ? FROMRGB(96, 90, 83)
				                             : FROMRGB(58, 53, 47);
				ChessDrawDot(cx + ring[i][0] - 3, scy + ring[i][1] - 3, col);
			}
		}
		else
		{
			// a game on the table: the same tab pair as Watch; the chat is
			// just the two of you
			ChessRenderPanelTabs(giPlayTab);
			if (giPlayTab == 0)
			{
				ChessRenderMoveList(gPlaySan, 58, CH_FOOT_Y - 4, &giMoveScroll, giPlayView);
			}
			else
			{
				ChessRenderChatPanel(gPlayChat);
			}
		}

		// the footer is contextual: controls during the game, START GAME in
		// the lobby, NEW GAME over a finished board, patience while seeking
		if (giPlayState == 0 || giPlayState == 1 || giPlayState == 5)
		{
			ChessRenderScrubber(gPlayHist, giPlayView);
			// the little flag; armed, it turns red and asks you to mean it
			const INT32 rx = CH_PANEL_X + 8, ry = CH_FOOT_Y + 9;
			const bool armed = ChessNow() < guiResignArmUntil;
			if (armed)
			{
				FillRounded(rx, ry, 20, 20, FROMRGB(167, 45, 45), 3, CH_RGB_PANEL_SUNK);
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, rx + 10, ry + 5, "!");
			}
			else
			{
				ChessDrawGreyButton(rx, ry, 20, 20, CH_RGB_PANEL_SUNK, true);
				const UINT32 fc = FROMRGB(190, 185, 178);
				FillRect(rx + 6, ry + 4, 2, 12, fc);
				FillRect(rx + 8, ry + 4, 7, 3, fc);
				FillRect(rx + 8, ry + 7, 5, 2, fc);
			}
		}
		else if (giPlayState == 3)
		{
			ChessDrawGreyButton(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20,
			                    CH_HINT_H, CH_RGB_PANEL_SUNK, false);
			PrintCentred(FONT14ARIAL, FONT_GRAY6, cx,
			             CH_HINT_Y + 8,
			             "SEEKING...");
		}
		else
		{
			ChessDrawCTAButton(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20, CH_HINT_H,
			                   CH_RGB_PANEL_SUNK);
			PrintCentred(FONT14ARIAL, FONT_MCOLOR_WHITE, cx,
			             CH_HINT_Y + 8,
			             giPlayState == 4 ? ST::string("START GAME") : ST::string(T(CHS_PLAY_NEW)));
		}
	}

	// The finished-game card: verdict, how, a word from the coach, and the
	// three ways forward - the live site's own layout.
	void ChessRenderPlayModal()
	{
		if (!gfPlayModal || giPlayState != 2) return;
		FRAME_BUFFER->ShadowRect(CH_X(CH_BOARD_X), CH_Y(CH_BOARD_Y),
		                         CH_X(CH_BOARD_X + CH_BOARD_SIZE) - 1,
		                         CH_Y(CH_BOARD_Y + CH_BOARD_SIZE) - 1);
		const INT32 mx = CH_MODAL_X;
		const INT32 my = CH_BOARD_Y + (CH_BOARD_SIZE - 148) / 2;
		const INT32 mw = CH_MODAL_W, mh = 148;
		FillRoundedOnly(mx - 1, my - 1, mw + 2, mh + 2, CH_RGB_PANEL_UP, 6);
		FillRoundedOnly(mx, my, mw, mh, CH_RGB_PANEL, 5);
		PrintAt(FONT10ARIALBOLD, FONT_GRAY4, mx + mw - 16, my + 6, "X");

		const bool won  = giPlaySaid == CHS_PLAY_WIN;
		const bool draw = giPlaySaid == CHS_PLAY_DRAW;
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, mx + mw / 2, my + 10,
		             won ? "YOU WON!" : draw ? "A DRAW" : "YOU LOST");
		PrintCentred(FONT10ARIAL, FONT_GRAY4, mx + mw / 2, my + 24,
		             giPlayEndReason == 1 ? "by resignation"
		                                  : draw ? "nobody is happy"
		                                         : "by checkmate");

		// the coach has a word either way
		const char* line = won  ? "you beat one of ze regulars. zey will remember."
		                 : draw ? "half a point. half a feeling."
		                 : giPlayEndReason == 1 ? "a resigned game is still a lesson."
		                                        : "zey are ruthless. i told you.";
		INT32 faceW = 26;
		if (guiChessCoach)
		{
			faceW = guiChessCoach->SubregionProperties(0).usWidth;
			ChessBltCoachFace(mx + 10, my + 42);
		}
		const INT32 bx2 = mx + 10 + faceW + 4;
		const INT32 bw2 = mx + mw - 10 - bx2;
		FillRounded(bx2, my + 42, bw2, 40, CH_RGB_BUBBLE, 3, CH_RGB_PANEL);
		DisplayWrappedString(UINT16(CH_X(bx2 + 5)), UINT16(CH_Y(my + 47)),
		                     UINT16(bw2 - 10), 1, FONT10ARIAL, FONT_MCOLOR_BLACK,
		                     line, FONT_MCOLOR_WHITE, LEFT_JUSTIFIED);

		ChessDrawCTAButton(mx + 12, my + 92, mw - 24, 22, CH_RGB_PANEL);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, mx + mw / 2, my + 98,
		             giPlayControl == 0 ? ST::string("NEW DAILY")
		                                : ST::format("NEW {} MIN", giPlayControl));
		for (int i = 0; i < 2; ++i)
		{
			const INT32 hx2 = mx + 12 + i * 94;
			ChessDrawGreyButton(hx2, my + 120, 86, 20, CH_RGB_PANEL, true);
			PrintCentred(FONT10ARIALBOLD, FONT_GRAY2, hx2 + 43, my + 125,
			             i == 0 ? "LOBBY" : "REMATCH");
		}
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
		FillRect(CH_PANEL_X, CH_FOOT_Y, CH_PANEL_W, 38, CH_RGB_PANEL_SUNK);
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
		// left-anchored: the word DAY holds still while the number grows
		ChessIconLabel(CH_ICON_CALENDAR, CH_CHIP_X + 8,
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

		// the streak, at the foot: a flame and a number, nothing else.
		// At zero the flame goes cold - decoded to grey by luma.
		if (guiChessIcons)
		{
			if (gChessDay.streak > 0)
			{
				BltVideoObject(FRAME_BUFFER, guiChessIcons, CH_ICON_FLAME,
				               CH_X(CH_PANEL_X + 10), CH_Y(CH_PAGE_H - 76));
			}
			else
			{
				const ETRLEObject& e =
					guiChessIcons->SubregionProperties(CH_ICON_FLAME);
				const UINT16* pal = guiChessIcons->Palette16();
				const UINT8* in = guiChessIcons->PixData(e);
				SGPVSurface::Lock lock(FRAME_BUFFER);
				UINT16* buf = lock.Buffer<UINT16>();
				const UINT32 pitch = lock.Pitch() / 2;
				for (INT32 iy = 0; pal && iy < e.usHeight; ++iy)
				{
					INT32 ix = 0;
					while (*in != 0)
					{
						const UINT8 code = *in++;
						const UINT8 run = code & 0x7F;
						if (code & 0x80) { ix += run; continue; }
						for (UINT8 k = 0; k < run; ++k, ++ix)
						{
							const UINT16 c = pal[*in++];
							const INT32 luma =
								(((c >> 11) & 31) * 2 * 30 +
								 ((c >> 5) & 63) * 59 +
								 (c & 31) * 2 * 11) / 200;
							const INT32 g5 = std::min(31, 6 + luma / 2);
							buf[UINT32(CH_Y(CH_PAGE_H - 76 + iy)) * pitch +
									UINT32(CH_X(CH_PANEL_X + 10 + ix))] =
								UINT16((g5 << 11) | (g5 * 2 << 5) | g5);
						}
					}
					++in;
				}
			}
		}
		PrintAt(FONT10ARIALBOLD, gChessDay.streak > 0 ? FONT_MCOLOR_WHITE : FONT_GRAY7,
		        CH_PANEL_X + 28, CH_PAGE_H - 75, ST::format("{}", gChessDay.streak));

		// the hint button greys out once it has been spent
		const bool hintLive = gChessState == CHUI_PUZZLE && !(gChessDay.flags & ChessDaily::FLAG_HINT_USED);
		ChessDrawGreyButton(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20, CH_HINT_H,
		                    CH_RGB_PANEL_SUNK, hintLive);
		PrintCentred(FONT14ARIAL, hintLive ? FONT_MCOLOR_WHITE : FONT_GRAY7,
		             cx, CH_HINT_Y + 8, T(CHS_HINT));
	}

	// The result card, over a scanline-dimmed board. Square corners on purpose:
	// rounding it would need the board colour behind each corner, and the board
	// is not one colour.
	// What the site is when its one man is in the field: still serving, because
	// the daily puzzle is automated and he is not. Only he is missing.
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

		if (gfLearnModal)
		{
			const INT32 w = CH_MODAL_W, h = 140;
			const INT32 x = CH_MODAL_X, y = CH_MODAL_Y;
			const INT32 cx2 = x + w / 2;
			FillRoundedOnly(x - 1, y - 1, w + 2, h + 2, CH_RGB_PANEL_UP, 6);
			FillRoundedOnly(x, y, w, h, CH_RGB_PANEL, 5);
			PrintAt(FONT10ARIAL, FONT_GRAY4, x + w - 14, y + 4, "X");
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx2, y + 12,
			             "ZE BOOK IS FINISHED");
			// the coach speaks beneath the heading, above the button
			INT32 faceW = 26;
			if (guiChessCoach)
			{
				faceW = guiChessCoach->SubregionProperties(0).usWidth;
				ChessBltCoachFace(x + 10, y + 28);
			}
			const INT32 bubbleX = x + 10 + faceW + 4;
			const INT32 bubbleW = x + w - 10 - bubbleX;
			FillRounded(bubbleX, y + 26, bubbleW, 34, CH_RGB_BUBBLE, 3,
			            CH_RGB_PANEL);
			for (int i = 0; i < 4; ++i)
			{
				const INT32 th = 8 - 2 * i;
				if (th <= 0) break;
				FillRect(bubbleX - 1 - i, y + 41 - th / 2, 1, th,
				         CH_RGB_BUBBLE);
			}
			DisplayWrappedString(UINT16(CH_X(bubbleX + 4)),
			                     UINT16(CH_Y(y + 31)), UINT16(bubbleW - 8), 1,
			                     FONT10ARIAL, FONT_MCOLOR_BLACK,
			                     "eight lessons. no refunds. go play.",
			                     FONT_MCOLOR_WHITE, LEFT_JUSTIFIED);
			ChessDrawCTAButton(x + 12, y + 66, w - 24, 28, CH_RGB_PANEL);
			PrintCentred(FONT14ARIAL, FONT_MCOLOR_WHITE, cx2, y + 74,
			             "DANKE, COACH");
			const INT32 boxW = (w - 30) / 2;
			const INT32 boxY = y + 102;
			static const char* const vals[2] = { "8", "$0" };
			static const char* const tags[2] = { "LESSONS", "TUITION" };
			for (int i = 0; i < 2; ++i)
			{
				const INT32 bx = x + 12 + i * (boxW + 6);
				FillRect(bx, boxY, boxW, 28, CH_RGB_PANEL_SUNK);
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
				             bx + boxW / 2, boxY + 3, vals[i]);
				PrintCentred(FONT10ARIAL, FONT_GRAY4, bx + boxW / 2,
				             boxY + 15, tags[i]);
			}
			return;
		}

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

		ChessDrawCTAButton(x + 12, y + 46, w - 24, 28, CH_RGB_PANEL);
		// the title bar's 14pt cut: heavy by nature, no faking needed
		PrintCentred(FONT14ARIAL, FONT_MCOLOR_WHITE, cx, y + 54,
		             T(CHS_MODAL_ARCHIVE));

		// streak and best, side by side on sunk ground
		const INT32 boxW = (w - 30) / 2;
		const INT32 boxY = y + 80;
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
		const INT32 bh = tall ? 48 : 36;
		const INT32 bx = CH_BOARD_X, bw = CH_BOARD_SIZE;
		const INT32 by = CH_PAGE_H - CH_INSET - bh;
		const INT32 cx = bx + bw / 2;
		// the strip is inventory: every site on ze web has bought it at least
		// once. A fresh creative on every page view, each in its owner's colours.
		const int slot = ChessAdSlot();
		FillRect(bx, by, bw, bh, FROMRGB(0, 0, 0));
		const INT32 l1 = by + (tall ? 10 : 8);
		const INT32 l2 = by + (tall ? 20 : 18);
		const INT32 l3 = by + 30;
		switch (slot)
		{
			case 0:  // Bobby Ray's: his own creative, from his own homepage
			{
				FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(214, 213, 206));
				FillRect(bx + 1, by + 1, 96, bh - 2, FROMRGB(178, 24, 24));
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, bx + 49, by + (tall ? 12 : 8), "BOBBY");
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, bx + 49, by + (tall ? 24 : 18), "RAY'S");
				PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_BLACK, bx + 108, by + (tall ? 11 : 8), "GUNS AND MORE");
				PrintAt(FONT10ARIAL, FONT_GRAY6, bx + 108, by + (tall ? 21 : 18), "always cheap. always stocked.");
				if (tall) PrintAt(FONT10ARIAL, FONT_GRAY6, bx + 108, by + 31, "always legal*   *mostly");
				FillRounded(bx + bw - 44, by + (tall ? 14 : 7), 36, 18, FROMRGB(24, 24, 24), 3,
				            FROMRGB(214, 213, 206));
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, bx + bw - 26, by + (tall ? 19 : 12), "SALE");
				break;
			}
			case 1:  // the house ad: gold hairlines, a little crown, dark velvet
			{
				const UINT32 gold = FROMRGB(212, 175, 55);
				FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(24, 20, 12));
				FillRect(bx + 8, by + 3, bw - 16, 1, gold);
				FillRect(bx + 8, by + bh - 4, bw - 16, 1, gold);
				// two crowns flank the copy, each set close to its edge
				const INT32 ky = by + (tall ? 15 : 11);
				for (const INT32 kx : { bx + 16, bx + bw - 36 })
				{
					for (int t = 0; t < 3; ++t)
					{
						const INT32 sx = kx + t * 7;
						FillRect(sx + 2, ky, 2, 3, gold);
						FillRect(sx + 1, ky + 3, 4, 3, gold);
						FillRect(sx, ky + 6, 6, 2, gold);
					}
					FillRect(kx, ky + 8, 20, 3, gold);
				}
				PrintCentred(FONT10ARIALBOLD, FONT_YELLOW, cx, by + (tall ? 10 : 8), "CHACH.COM GOLD CROWN");
				PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, cx, by + (tall ? 20 : 19),
				             tall ? "MEMBERSHIP - COMING SOON" : "COMING SOON. do not ask.");
				if (tall) PrintCentred(FONT10ARIAL, FONT_GRAY6, cx, by + 30, "do not ask ze proprietor");
				break;
			}
			case 2:  // the Parlour, the reference creative
				FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(94, 26, 26));
				PrintCentred(FONT10ARIALBOLD, FONT_YELLOW, cx, by + (tall ? 6 : 7), "SAN MONA MAHJONG PARLOUR");
				if (tall)
				{
					PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, by + 20, "GAMES ARE FAIR BECAUSE");
					PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, by + 33, "MR. KLAUS SAYS SO");
					if (guiChessAdTiles)
					{
						BltVideoObject(FRAME_BUFFER, guiChessAdTiles, 9, CH_X(bx + 8), CH_Y(by + 4));
						BltVideoObject(FRAME_BUFFER, guiChessAdTiles, 18, CH_X(bx + bw - 38), CH_Y(by + 4));
					}
				}
				else
				{
					PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTRED, cx, by + 19,
					             "GAMES ARE FAIR BECAUSE MR. KLAUS SAYS SO");
				}
				break;
			case 3:  // A.I.M.: their own masthead carries the brand; the copy
			         // stands clear of it to the right
			{
				FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(46, 34, 26));
				if (tall && gAdArtAim)
				{
					BltVideoSurface(FRAME_BUFFER, gAdArtAim, CH_X(bx + 6),
					                CH_Y(by + (bh - gAdArtAim->Height()) / 2), NULL);
					const INT32 tx3 = bx + 6 + gAdArtAim->Width() + 10;
					PrintAt(FONT10ARIAL, FONT_GRAY2, tx3, by + 12, "ELITE MERCENARIES FOR HIRE");
					PrintAt(FONT10ARIAL, FONT_GRAY6, tx3, by + 26, "some are even polite.");
				}
				else
				{
					PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, bx + 12, by + 5, "A.I.M.");
					PrintAt(FONT10ARIAL, FONT_GRAY2, bx + 58, by + 5, "ELITE MERCENARIES FOR HIRE");
					PrintAt(FONT10ARIAL, FONT_GRAY6, bx + 58, by + 17, "references available");
				}
				break;
			}
			case 4:  // I.M.P.: navy, and the test sheet with its little ticks
			{
				FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(26, 34, 48));
				PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, bx + 12, by + (tall ? 10 : 8), "I.M.P.");
				PrintAt(FONT10ARIAL, FONT_GRAY2, bx + 12, by + (tall ? 20 : 18), "KNOW THYSELF");
				if (tall) PrintAt(FONT10ARIAL, FONT_GRAY6, bx + 12, by + 30, "$3000. worth every session.");
				// the profile sheet
				const INT32 px = bx + bw - 92, pw = 82;
				FillRect(px, by + 4, pw, bh - 8, FROMRGB(228, 226, 216));
				for (int i = 0; i < (tall ? 3 : 5); ++i)
				{
					const INT32 ly = by + 9 + i * (tall ? 11 : 10);
					if (i == 1)
					{
						// the ticked answer is a plain solid square
						FillRect(px + 6, ly, 6, 6, FROMRGB(40, 40, 44));
					}
					else
					{
						FillRect(px + 6, ly, 6, 6, FROMRGB(150, 148, 140));
						FillRect(px + 7, ly + 1, 4, 4, FROMRGB(228, 226, 216));
					}
					FillRect(px + 18, ly + 2, pw - 30, 2, FROMRGB(150, 148, 140));
				}
				break;
			}
			case 5:  // M.E.R.C.: a photocopied flyer, hand-underlined
			{
				FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(224, 224, 224));
				PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_BLACK, bx + 12, by + (tall ? 9 : 7), "M.E.R.C.");
				// the underline misses, twice - it is that kind of operation
				FillRect(bx + 12, by + (tall ? 19 : 18), 58, 2, FROMRGB(178, 24, 24));
				FillRect(bx + 16, by + (tall ? 22 : 21), 58, 1, FROMRGB(178, 24, 24));
				PrintAt(FONT10ARIAL, FONT_MCOLOR_BLACK, bx + 92, by + (tall ? 9 : 7), "affordable manpower.");
				PrintAt(FONT10ARIAL, FONT_MCOLOR_BLACK, bx + 92, by + (tall ? 20 : 18), "flexible standards.");
				if (tall) PrintAt(FONT10ARIALBOLD, FONT_RED, bx + 92, by + 31, "NO REFUNDS. ask for Marty.");
				break;
			}
			case 6:  // United Floral: their own creative, else the drawn card
			{
				FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(240, 242, 232));
				for (const INT32 fxp : { bx + 18, bx + bw - 18 })
				{
					const INT32 fy = by + bh / 2 - 3;
					const UINT32 petal = FROMRGB(214, 120, 150);
					FillRect(fxp - 2, fy - 5, 4, 4, petal);
					FillRect(fxp - 2, fy + 3, 4, 4, petal);
					FillRect(fxp - 6, fy - 1, 4, 4, petal);
					FillRect(fxp + 2, fy - 1, 4, 4, petal);
					FillRect(fxp - 1, fy, 3, 3, FROMRGB(226, 186, 60));
				}
				PrintCentred(FONT10ARIALBOLD, FONT_DKGREEN, cx, by + (tall ? 8 : 7), "UNITED FLORAL SERVICE");
				for (INT32 dxp = cx - 66; dxp < cx + 66; dxp += 8)
				{
					FillRect(dxp, by + (tall ? 22 : 19), 4, 1, FROMRGB(96, 138, 82));
				}
				PrintCentred(FONT10ARIAL, FONT_GRAY6, cx, by + (tall ? 27 : 22),
				             tall ? "flowers delivered anywhere on ze planet. yes, even there."
				                  : "delivered anywhere. even there.");
				break;
			}
			case 7: // McGillicutty's: their own creative, else the drawn card
			{
				FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(22, 22, 24));
				const INT32 ix = bx + 5, iy = by + 4, iw = bw - 10, ih = bh - 8;
				FillRect(ix, iy, iw, 1, FROMRGB(96, 96, 100));
				FillRect(ix, iy + ih - 1, iw, 1, FROMRGB(96, 96, 100));
				FillRect(ix, iy, 1, ih, FROMRGB(96, 96, 100));
				FillRect(ix + iw - 1, iy, 1, ih, FROMRGB(96, 96, 100));
				for (const INT32 tx2 : { ix + 3, ix + iw - 9 })
				{
					FillRect(tx2, iy + 3, 6, 1, FROMRGB(140, 140, 144));
					FillRect(tx2, iy + ih - 4, 6, 1, FROMRGB(140, 140, 144));
				}
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, by + (tall ? 9 : 8), "McGILLICUTTY'S MORTUARY");
				PrintCentred(FONT10ARIAL, FONT_GRAY6, cx, by + (tall ? 21 : 19),
				             "for when ze flowers were not enough");
				if (tall) PrintCentred(FONT10ARIAL, FONT_GRAY7, cx, by + 33, "discreet. experienced. open late.");
				break;
			}
			default: // Malleus, Incus & Stapes: umbrella drawn by hand,
			         // like every other site carries this advertiser
			{
				FillRect(bx + 1, by + 1, bw - 2, bh - 2, FROMRGB(30, 38, 34));
				// the umbrellas flank the copy: canopy, stem, hook
				for (const INT32 ux : { bx + 18, bx + bw - 34 })
				{
					const INT32 uy = by + (tall ? 12 : 8);
					const UINT32 pale = FROMRGB(148, 178, 196);
					for (int r = 0; r < 5; ++r)
					{
						const INT32 half = 2 + 2 * r;
						FillRect(ux + 8 - half, uy + r, 2 * half, 1, pale);
					}
					FillRect(ux + 7, uy + 5, 2, 12, pale);
					FillRect(ux + 5, uy + 16, 3, 2, pale);
				}
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx, by + (tall ? 8 : 7),
				             "MALLEUS, INCUS & STAPES");
				PrintCentred(FONT10ARIAL, FONT_GRAY2, cx, by + (tall ? 21 : 19),
				             "insurance for ze working merc");
				if (tall) PrintCentred(FONT10ARIAL, FONT_GRAY6, cx, by + 33,
				                       "premiums reflect ze profession");
				break;
			}
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
		FillRounded(x, y, w, h, FROMRGB(86, 128, 45), 5, bg);
		FillRounded(x, y, w, h - 2, CH_RGB_CTA, 5, bg);
		FillRect(x + 3, y + 2, w - 6, band, FROMRGB(150, 199, 88));
		FillRect(x + 5, y + 1, w - 10, 1, FROMRGB(184, 221, 130));
		// the gradient steps down through three loud dither rows: full
		// checker, offset checker, then a sparse tail into the body
		for (INT32 row = 0; row < 3; ++row)
		{
			const INT32 dy = y + 2 + band + row;
			const INT32 step = row == 2 ? 4 : 2;
			for (INT32 dx = x + 2 + ((row + 1) & 1); dx < x + w - 2; dx += step)
			{
				FillRect(dx, dy, 1, 1, FROMRGB(150, 199, 88));
			}
		}
	}

	// A chess title, chess.com style: a small crimson chip, white letters.
	// Returns the width it consumed, zero for the untitled.
	INT32 ChessDrawFlag(INT32 x, INT32 y, int flag)
	{
		if (flag < 0 || !guiChessFlags) return 0;
		BltVideoObject(FRAME_BUFFER, guiChessFlags, UINT16(flag), CH_X(x), CH_Y(y));
		return 11 + 4;
	}

	INT32 ChessDrawTitleBadge(INT32 x, INT32 y, const char* title, UINT32 bg)
	{
		if (!title || !*title) return 0;
		const INT32 w = StringPixLength(title, TINYFONT1) + 2;
		FillRounded(x, y, w, 8, FROMRGB(167, 45, 45), 2, bg);
		PrintCentred(TINYFONT1, FONT_MCOLOR_WHITE, x + w / 2, y - 1, title);
		return w + 5;
	}

	// The green CTA's construction in dark grey: foot edge, body, band, top
	// highlight and the two dithered gradient rows. The pager's palette at
	// full button size; `live` dims the whole thing for a spent button.
	void ChessDrawGreyButton(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 bg, bool live)
	{
		const INT32 band = (h - 2) / 3;
		const UINT32 foot = live ? FROMRGB(24, 21, 18) : FROMRGB(24, 22, 20);
		const UINT32 body = live ? FROMRGB(64, 59, 54) : FROMRGB(42, 39, 36);
		const UINT32 bnd  = live ? FROMRGB(86, 80, 73) : FROMRGB(50, 46, 42);
		const UINT32 high = live ? FROMRGB(102, 95, 87) : FROMRGB(56, 52, 48);
		FillRounded(x, y, w, h, foot, 3, bg);
		FillRounded(x, y, w, h - 2, body, 3, bg);
		FillRect(x + 2, y + 2, w - 4, band, bnd);
		FillRect(x + 3, y + 1, w - 6, 1, high);
		// the gradient steps down through three loud dither rows: full
		// checker, then sparser, band colour over the darker body
		for (INT32 row = 0; row < 3; ++row)
		{
			const INT32 dy = y + 2 + band + row;
			const INT32 step = row == 2 ? 4 : 2;
			for (INT32 dx = x + 2 + ((row + 1) & 1); dx < x + w - 2; dx += step)
			{
				FillRect(dx, dy, 1, 1, bnd);
			}
		}
	}

	// The capture tally: seven-pixel silhouettes of the board's own pieces -
	// head-and-bell pawn, battlemented rook, leaning knight, egg bishop,
	// spiked queen - so the row reads like the set it counts.
	void ChessDrawCaptureGlyph(INT32 x, INT32 y, int type, UINT32 rgb)
	{
		switch (type)
		{
			case ChessGame::Rook:
				FillRect(x,     y, 2, 2, rgb);
				FillRect(x + 4, y, 2, 2, rgb);
				FillRect(x + 1, y + 2, 4, 3, rgb);
				FillRect(x,     y + 5, 6, 2, rgb);
				break;
			case ChessGame::Knight:
				FillRect(x + 2, y, 2, 1, rgb);
				FillRect(x + 1, y + 1, 5, 2, rgb);
				FillRect(x + 1, y + 3, 3, 2, rgb);
				FillRect(x,     y + 5, 6, 2, rgb);
				break;
			case ChessGame::Bishop:
				FillRect(x + 2, y, 2, 1, rgb);
				FillRect(x + 1, y + 1, 4, 4, rgb);
				FillRect(x,     y + 5, 6, 2, rgb);
				break;
			case ChessGame::Queen:
				FillRect(x,     y, 1, 3, rgb);
				FillRect(x + 3, y, 1, 3, rgb);
				FillRect(x + 6, y, 1, 3, rgb);
				FillRect(x,     y + 3, 7, 2, rgb);
				FillRect(x + 1, y + 5, 5, 2, rgb);
				break;
			case ChessGame::King:
				// never captured, but the move list needs him: cross, then
				// the crown, then the same foot as everyone else
				FillRect(x + 3, y, 1, 3, rgb);
				FillRect(x + 2, y + 1, 3, 1, rgb);
				FillRect(x + 1, y + 3, 5, 2, rgb);
				FillRect(x,     y + 5, 7, 2, rgb);
				break;
			default:  // pawn
				FillRect(x + 2, y, 3, 3, rgb);
				FillRect(x + 1, y + 4, 5, 1, rgb);
				FillRect(x,     y + 5, 6, 2, rgb);
				break;
		}
	}

	void ChessRenderPlayerRow(SGPVSurface* face, const ST::string& handle,
	                          const ST::string& rating, INT32 y,
	                          const ChessGame* game, ChessGame::Color side,
	                          int plies, bool clockActive,
	                          const char* title = nullptr,
	                          int baseSecs = 600, int bleedPerMove = 8)
	{
		INT32 nameX = CH_BOARD_X;
		if (face)
		{
			BltVideoSurface(FRAME_BUFFER, face,
			                CH_X(CH_BOARD_X + (CH_SQ - CH_ROW_FACE) / 2),
			                CH_Y(y + (CH_SQ - CH_ROW_FACE) / 2), NULL);
			nameX += CH_SQ + 2;
		}
		const INT32 dotX = nameX;  // captures stay left-aligned under the name
		nameX += ChessDrawTitleBadge(nameX, y + 3, title, CH_RGB_CHROME);
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, nameX, y + 3, handle);
		INT32 tailX = nameX + StringPixLength(handle, FONT10ARIALBOLD) + 6;
		if (!rating.empty())
		{
			PrintAt(FONT10ARIAL, FONT_GRAY4, tailX, y + 3, rating);
			tailX += StringPixLength(rating, FONT10ARIAL) + 2;
		}
		// the flag rides after the rating, as it does on the reference site,
		// sitting on the cap line rather than the baseline
		ChessDrawFlag(tailX, y + 3, ChessFlagForHandle(handle));

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
			INT32 dx = dotX;
			for (int type = ChessGame::Pawn; type <= ChessGame::Queen; ++type)
			{
				// same-type captures huddle into one overlapping cluster;
				// a breath of air separates the type groups
				bool any = false;
				for (int k = onBoard[type]; k < START[type]; ++k)
				{
					ChessDrawCaptureGlyph(dx, y + 19, type, disc);
					dx += 4;
					any = true;
				}
				if (any) dx += 3 + 4;
			}

			// the clock: ten minutes each, bleeding a few seconds per move -
			// nobody has ever believed a hit counter either
			if (baseSecs == 0)
			{
				// correspondence: the chip names the control instead
				const INT32 boxW = 40;
				const INT32 boxX = CH_BOARD_X + CH_BOARD_SIZE - boxW;
				FillRounded(boxX, y + 2, boxW, 18, CH_RGB_PANEL_SUNK, 3, CH_RGB_CHROME);
				PrintCentred(FONT10ARIAL, FONT_GRAY4, boxX + boxW / 2, y + 6, "1 day");
				return;
			}
			const int mine = side == ChessGame::White ? (plies + 1) / 2 : plies / 2;
			int secs = baseSecs - mine * bleedPerMove;
			if (secs < 4) secs = 4;
			const ST::string time = ST::format("{}:{02d}", secs / 60, secs % 60);
			const INT32 boxW = 40;
			const INT32 boxX = CH_BOARD_X + CH_BOARD_SIZE - boxW;
			FillRounded(boxX, y + 2, boxW, 18,
			            clockActive ? CH_RGB_LIGHT : CH_RGB_PANEL_SUNK, 3, CH_RGB_CHROME);
			PrintCentred(FONT10ARIAL,
			             clockActive ? FONT_NEARBLACK : FONT_GRAY4,
			             boxX + boxW / 2, y + 6, time);
		}
	}

	// A slim scrollbar: track on the panel's right edge, thumb proportional.
	void ChessDrawScrollbar(INT32 y0, INT32 y1, int total, int fit, int offBottom)
	{
		if (total <= fit) return;
		const INT32 x = CH_PANEL_X + CH_PANEL_W - 5;
		FillRect(x, y0, 3, y1 - y0, CH_RGB_PANEL_SUNK);
		const INT32 trackH = y1 - y0;
		INT32 thumbH = trackH * fit / total;
		if (thumbH < 10) thumbH = 10;
		const int maxOff = total - fit;
		const INT32 ty = y0 + (trackH - thumbH) * (maxOff - offBottom) / maxOff;
		FillRect(x, ty, 3, thumbH, FROMRGB(80, 74, 68));
	}

	// The visible stretch of a move list, numbered SAN pairs, newest at the
	// foot; the scroll offset counts pairs back from the live end.
	// How wide ChessPrintSan will draw a move: the glyph stands in for the
	// piece letter, everything else is text.
	INT32 ChessMoveWidth(const ST::string& san)
	{
		switch (san.empty() ? '\0' : san.c_str()[0])
		{
			case 'N': case 'B': case 'R': case 'Q': case 'K':
				return 10 + StringPixLength(san.substr(1), FONT10ARIAL);
			default:
				return StringPixLength(san, FONT10ARIAL);
		}
	}

	// One move in the reference's dress: the piece letter drawn as the piece,
	// the square in text after it. Pawn moves and castling have no letter to
	// replace and print as they stand.
	INT32 ChessPrintSan(INT32 x, INT32 y, const ST::string& san, UINT8 colour, UINT32 glyph)
	{
		int type = 0;
		switch (san.empty() ? '\0' : san.c_str()[0])
		{
			case 'N': type = ChessGame::Knight; break;
			case 'B': type = ChessGame::Bishop; break;
			case 'R': type = ChessGame::Rook;   break;
			case 'Q': type = ChessGame::Queen;  break;
			case 'K': type = ChessGame::King;   break;
			default: break;
		}
		if (type == 0)
		{
			PrintAt(FONT10ARIAL, colour, x, y, san);
			return StringPixLength(san, FONT10ARIAL);
		}
		// on the text's own cap line, not slung under it
		ChessDrawCaptureGlyph(x, y - 1, type, glyph);
		const ST::string rest = san.substr(1);
		PrintAt(FONT10ARIAL, colour, x + 10, y, rest);
		return 10 + StringPixLength(rest, FONT10ARIAL);
	}

	// `view` is the scrubber's position: an index into the history, where 0 is
	// the starting board and -1 means the live end. The move that produced the
	// board on screen is the ply before it, and that is the one the list marks.
	void ChessRenderMoveList(const std::vector<ST::string>& san, INT32 y0, INT32 y1,
	                         int* scroll, int view)
	{
		const int pairs = int(san.size() + 1) / 2;
		const int fit   = (y1 - y0) / 16;
		const int maxOff = pairs > fit ? pairs - fit : 0;
		const int plies  = int(san.size());
		const int cur    = (view < 0 || view > plies) ? plies - 1 : view - 1;

		// follow the scrubber: when the marked move is walked out of sight,
		// bring it back, and otherwise leave the reader's scroll alone
		static int sLastCur = -2;
		if (cur != sLastCur && cur >= 0 && maxOff > 0)
		{
			const int wanted = cur / 2;
			const int firstNow = maxOff - *scroll;
			if (wanted < firstNow || wanted >= firstNow + fit)
			{
				const int top = wanted - fit / 2;
				const int clamped = top < 0 ? 0 : top > maxOff ? maxOff : top;
				*scroll = maxOff - clamped;
			}
		}
		sLastCur = cur;

		if (*scroll > maxOff) *scroll = maxOff;
		if (*scroll < 0) *scroll = 0;
		const int first = maxOff - *scroll;
		const int last  = first + fit < pairs ? first + fit : pairs;
		INT32 y = y0;
		for (int pn = first; pn < last; ++pn)
		{
			// zebra rows: every second move-pair sits on a lighter band that
			// runs the full width of the sidebar, edge to edge
			if (pn % 2 == 0)
			{
				FillRect(CH_PANEL_X, y - 3, CH_PANEL_W, 16, CH_RGB_ROW_ALT);
			}
			PrintAt(FONT10ARIAL, FONT_GRAY7, CH_PANEL_X + 10, y + 2, ST::format("{}.", pn + 1));
			const UINT32 glyph = FROMRGB(196, 193, 188);
			for (int half = 0; half < 2; ++half)
			{
				const int ply = pn * 2 + half;
				if (ply >= plies) break;
				const INT32 mx = CH_PANEL_X + (half == 0 ? 32 : 86);
				const bool here = ply == cur;
				if (here)
				{
					// the move on the board wears the badge's dress: a chip
					// under it, and its own text and piece brightened
					const INT32 w = ChessMoveWidth(san[ply]);
					FillRoundedOnly(mx - 4, y - 1, w + 8, 14, CH_RGB_PANEL_UP, 3);
				}
				ChessPrintSan(mx, y + 2, san[ply], here ? FONT_MCOLOR_WHITE : FONT_GRAY2,
				              here ? FROMRGB(245, 244, 242) : glyph);
			}
			y += 16;
		}
		// last, so it rides over the zebra rather than under them: the bands
		// run the full width of the panel and would paint it out
		ChessDrawScrollbar(y0, y1, pairs, fit, *scroll);
	}

	// A right panel scaffold shared by the live views: rounded ground and a
	// sunk header band carrying the section's icon and name.
	INT32 ChessRenderSectionPanel(UINT16 icon, ChessStr title)
	{
		FillRounded(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		            CH_RGB_PANEL, CH_PANEL_RADIUS, CH_RGB_CHROME);
		// the band sits snug around the title alone
		FillRect(CH_PANEL_X, CH_INSET, CH_PANEL_W, 24, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_PANEL_RADIUS, CH_RGB_CHROME);
		const INT32 cx = CH_PANEL_X + CH_PANEL_W / 2;
		const ST::string text = ST::string(T(title)).to_upper();
		const INT32 w = ChessIconLabelWidth(FONT10ARIALBOLD, text);
		ChessIconLabel(icon, cx - w / 2, 19, FONT10ARIALBOLD, FONT_MCOLOR_WHITE, text);
		return cx;
	}

	// Learn: the lesson position occupies the full board; the teaching happens
	// in the sidebar, where the coach explains it from her bubble.
	void ChessRenderLearn()
	{
		// the door blinks: base colour and highlight take turns
		const bool blinkOn = (ChessNow() / 450) % 2 == 0;
		ChessRenderBoardCore(gLearnGame,
		                     gfLearnSolved ? gubLearnFrom
		                                   : ChessGame::NO_SQUARE,
		                     gfLearnSolved ? gubLearnTo
		                     : blinkOn     ? gubLearnTarget
		                                   : ChessGame::NO_SQUARE,
		                     gbChessSelected);
		ChessRenderBoardCoreLate(gLearnGame);
		// the lifted piece follows the pointer here too
		if (gfChessDragging && !gLearnGame.IsEmpty(gubChessDragFrom) &&
		    guiChessPieces)
		{
			const UINT8 type = gLearnGame.PieceAt(gubChessDragFrom);
			const UINT16 frame = UINT16((type - 1) +
				(gLearnGame.ColorAt(gubChessDragFrom) == ChessGame::Black
					? 6 : 0));
			BltVideoObject(FRAME_BUFFER, guiChessPieces, frame,
			               INT32(gusMouseXPos) - CH_SQ / 2,
			               INT32(gusMouseYPos) - CH_SQ / 2);
		}
	}

	void ChessRenderLearnPanel()
	{
		// the puzzle header's construction: lockup, stepper chip, title -
		// the chevrons walk the lessons the way the calendar walks the days
		FillRounded(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		            CH_RGB_PANEL, CH_PANEL_RADIUS, CH_RGB_CHROME);
		FillRect(CH_PANEL_X, CH_INSET, CH_PANEL_W, 66, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_PANEL_RADIUS, CH_RGB_CHROME);
		FillRect(CH_PANEL_X, CH_FOOT_Y, CH_PANEL_W, 38, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_PANEL_RADIUS, CH_RGB_CHROME);
		const INT32 cx = CH_PANEL_X + CH_PANEL_W / 2;
		const ChessLesson& lesson = CHESS_LESSONS[giChessLesson];

		{
			const ST::string head = ST::string(T(CHS_NAV_LEARN)).to_upper();
			const INT32 hw = ChessIconLabelWidth(FONT10ARIALBOLD, head);
			ChessIconLabel(CH_ICON_LEARN, cx - hw / 2, 20, FONT10ARIALBOLD,
			               FONT_MCOLOR_WHITE, head);
		}
		FillRounded(CH_CHIP_X, CH_DATE_Y, CH_CHIP_W, 20, CH_RGB_PANEL_UP, 3,
		            CH_RGB_PANEL_SUNK);
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, CH_CHIP_X + CH_CHIP_W / 2,
		             CH_DATE_Y + CH_ARROW_H / 2 - GetFontHeight(FONT10ARIAL) / 2,
		             ST::format("LESSON {}", giChessLesson + 1));
		ChessDrawChevron(CH_PREV_X + CH_ARROW_W / 2, CH_DATE_Y + 10, true,
		                 giChessLesson > 0 ? FROMRGB(148, 142, 136) : FROMRGB(74, 69, 64));
		ChessDrawChevron(CH_NEXT_X + CH_ARROW_W / 2, CH_DATE_Y + 10, false,
		                 giChessLesson < CHESS_LESSON_COUNT - 1 ? FROMRGB(148, 142, 136)
		                                                        : FROMRGB(74, 69, 64));
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, cx, CH_DATE_Y + 26,
		             lesson.title);

		// the coach carries the first line; the rest follows as body text.
		// Face, bubble and tail sit exactly where the daily puzzle puts
		// them, so flipping between the two tabs moves nothing.
		const INT32 faceX = CH_PANEL_X + 4;
		INT32 faceW = 26;
		if (guiChessCoach)
		{
			faceW = guiChessCoach->SubregionProperties(0).usWidth;
			ChessBltCoachFace(faceX, CH_COACH_Y);
		}
		const INT32 bubbleX = faceX + faceW + 4;
		const INT32 bubbleW = CH_PANEL_X + CH_PANEL_W - 4 - bubbleX;
		FillRounded(bubbleX, CH_COACH_Y, bubbleW, CH_COACH_TILE, CH_RGB_BUBBLE, 3,
		            CH_RGB_PANEL);
		for (int i = 0; i < 4; ++i)
		{
			const INT32 h = 8 - 2 * i;
			if (h <= 0) break;
			FillRect(bubbleX - 1 - i, CH_COACH_Y + 13 - h / 2, 1, h,
			         CH_RGB_BUBBLE);
		}
		DisplayWrappedString(UINT16(CH_X(bubbleX + 4)), UINT16(CH_Y(CH_COACH_Y + 5)),
		                     UINT16(bubbleW - 8), 1, FONT10ARIAL, FONT_MCOLOR_BLACK,
		                     gzLearnSay ? ST::string(gzLearnSay)
		                                : ST::string(lesson.lines[0]),
		                     FONT_MCOLOR_WHITE, LEFT_JUSTIFIED);

		DisplayWrappedString(UINT16(CH_X(CH_PANEL_X + 10)), UINT16(CH_Y(128)),
		                     UINT16(CH_PANEL_W - 20), 2, FONT10ARIAL, FONT_GRAY2,
		                     ST::format("{} {}", lesson.lines[1], lesson.lines[2]),
		                     FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		// the footer is contextual: a quiet HINT while the move is owed,
		// the green NEXT LESSON once it has been played
		FillRect(CH_PANEL_X, CH_FOOT_Y, CH_PANEL_W, 38, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_PANEL_RADIUS, CH_RGB_CHROME);
		if (gfLearnSolved)
		{
			ChessDrawCTAButton(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20,
			                   CH_HINT_H, CH_RGB_PANEL_SUNK);
			PrintCentred(FONT14ARIAL, FONT_MCOLOR_WHITE, cx,
			             CH_HINT_Y + 8,
			             T(CHS_LEARN_NEXT));
		}
		else
		{
			ChessDrawGreyButton(CH_PANEL_X + 10, CH_HINT_Y, CH_PANEL_W - 20,
			                    CH_HINT_H, CH_RGB_PANEL_SUNK, true);
			PrintCentred(FONT14ARIAL, FONT_GRAY2, cx,
			             CH_HINT_Y + 8,
			             "HINT");
		}
	}

	// Watch: the exhibition board between its two player rows, the move list
	// in the sidebar.
	void ChessRenderWatch()
	{
		const bool reviewing = giWatchView >= 0 && size_t(giWatchView) < gWatchHist.size();
		ChessGame& shown = reviewing ? gWatchHist[size_t(giWatchView)] : gWatchGame;
		ChessRenderBoardCore(shown,
		                     reviewing ? ChessGame::NO_SQUARE : gubWatchFrom,
		                     reviewing ? ChessGame::NO_SQUARE : gubWatchTo, -1);
		ChessRenderBoardCoreLate(shown);
		ChessRenderAnimPiece(shown);

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

	// The tab strip both live sidebars share: one full-width row under the
	// header, split in two; the resting half continues the sunk tone.
	void ChessRenderPanelTabs(int active)
	{
		const INT32 cx = CH_PANEL_X + CH_PANEL_W / 2;
		for (int i = 0; i < 2; ++i)
		{
			const bool on = active == i;
			const INT32 tx = CH_PANEL_X + i * (CH_PANEL_W / 2);
			const INT32 tw = i == 0 ? CH_PANEL_W / 2 : CH_PANEL_W - CH_PANEL_W / 2;
			if (!on) FillRect(tx, CH_INSET + 24, tw, 18, CH_RGB_PANEL_SUNK);
			PrintCentred(FONT10ARIAL, on ? FONT_MCOLOR_WHITE : FONT_GRAY6,
			             tx + CH_PANEL_W / 4, CH_INSET + 29, i == 0 ? "Moves" : "Chat");
		}
		(void)cx;
	}

	// The chat surface both sidebars share: the log stacks up from the
	// always-focused input line, newest at the bottom.
	void ChessRenderChatPanel(const std::vector<WatchChatLine>& log, INT32 inY)
	{
		const INT32 lx = CH_PANEL_X + 8;
		const INT32 lw = CH_PANEL_W - 16;
		if (giChatScroll > int(log.size()) - 3)
		{
			giChatScroll = int(log.size()) - 3;
			if (giChatScroll < 0) giChatScroll = 0;
		}
		int shown = 0;
		INT32 y = inY - 6;
		for (size_t i = log.size() - size_t(giChatScroll); i-- > 0 && y > 62; )
		{
			++shown;
			const WatchChatLine& line = log[i];
			const ST::string full = line.who.empty()
				? line.text : ST::format("{}: {}", line.who, line.text);
			// word wrap loses a little width per row; pad the estimate
			const int rows = 1 + (int(StringPixLength(full, TINYFONT1)) * 10 / 9) / lw;
			y -= rows * 9;
			if (y <= 58) break;
			DisplayWrappedString(UINT16(CH_X(lx)), UINT16(CH_Y(y)), UINT16(lw), 1,
			                     TINYFONT1,
			                     line.who.empty() ? FONT_GRAY4 : FONT_GRAY2,
			                     full, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
			if (!line.who.empty())
			{
				// the handle re-printed in the accent, same font, same spot
				PrintAt(TINYFONT1, FONT_MCOLOR_LTGREEN, lx, y,
				        ST::format("{}:", line.who));
			}
			y -= 4;
		}
		ChessDrawScrollbar(58, inY - 6, int(log.size()), shown, giChatScroll);
		// the input line: always focused, the parlour way
		FillRect(lx - 2, inY - 3, lw + 4, 1, CH_RGB_ROW_SEP);
		if (gWatchChatInput.empty())
		{
			PrintAt(TINYFONT1, FONT_GRAY6, lx, inY, "send a message...");
		}
		else
		{
			std::string typed = gWatchChatInput;
			if ((ChessNow() / 400) & 1) typed += "_";
			while (typed.size() > 1 &&
			       StringPixLength(ST::string(typed.c_str()), TINYFONT1) > lw)
			{
				typed.erase(typed.begin());
			}
			PrintAt(TINYFONT1, FONT_GRAY1, lx, inY, ST::string(typed.c_str()));
		}
	}

	void ChessRenderWatchPanel()
	{
		ChessRenderSectionPanel(CH_ICON_WATCH, CHS_NAV_WATCH);
		FillRect(CH_PANEL_X, CH_FOOT_Y, CH_PANEL_W, 38, CH_RGB_PANEL_SUNK);
		RoundCorners(CH_PANEL_X, CH_INSET, CH_PANEL_W, CH_PAGE_H - 2 * CH_INSET,
		             CH_PANEL_RADIUS, CH_RGB_CHROME);

		ChessRenderPanelTabs(giWatchTab);
		if (giWatchTab == 0)
		{
			ChessRenderMoveList(gWatchSan, 58, CH_FOOT_Y - 4, &giMoveScroll, giWatchView);
		}
		else
		{
			ChessRenderChatPanel(gWatchChat);
		}
		ChessRenderScrubber(gWatchHist, giWatchView);
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
		const INT32 tx = CH_BOARD_X + 52;
		// the date owns the right edge on every row; the handle only prints
		// when it fits in the space a long name leaves before it
		const INT32 dx = CH_BOARD_X + CH_BOARD_SIZE - 12 - StringPixLength(date, TINYFONT1);
		PrintAt(TINYFONT1, FONT_GRAY6, dx, y + 7, date);
		// the badge is part of the username lockup; the message below keeps
		// the column
		INT32 nameX = tx;
		nameX += ChessDrawTitleBadge(nameX, y + 7, ChessTitleForHandle(handle), CH_RGB_PANEL);
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, nameX, y + 8, name);
		const INT32 nameEnd = nameX + StringPixLength(name, FONT10ARIALBOLD) + 6;
		if (nameEnd + StringPixLength(handle, FONT10ARIAL) < dx - 8)
		{
			PrintAt(FONT10ARIAL, FONT_GRAY4, nameEnd, y + 8, handle);
			// signers who are on the roster sign under their flag
			const INT32 flagX = nameEnd + StringPixLength(handle, FONT10ARIAL) + 5;
			if (flagX + 11 < dx - 6)
			{
				ChessDrawFlag(flagX, y + 10, ChessFlagForHandle(handle));
			}
		}
		DisplayWrappedString(UINT16(CH_X(tx)), UINT16(CH_Y(y + 20)),
		                     UINT16(CH_BOARD_X + CH_BOARD_SIZE - 12 - tx), 3,
		                     FONT10ARIAL, FONT_GRAY2, line,
		                     FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		return rowH;
	}

	// The guestbook fills the page: the title sits above the book in the
	// daily header's lockup, the entries carry avatars, and signing is forever.
	// A ladder member's page: invented site totals in their voice, and the
	// real head-to-head ledger against you.
	void ChessRenderMemberProfile()
	{
		const ChessSeat& seat = giChessProfSeat == -2 ? CHESS_SEAT_ENRICO
		                                              : CHESS_SEATS[giChessProfSeat];
		const INT32 px = CH_BOARD_X;
		const INT32 pw = CH_BOARD_SIZE;
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, px + 2, 10, "PROFILE");
		PrintAt(FONT10ARIAL, FONT_GRAY4,
		        px + 2 + StringPixLength("PROFILE", FONT10ARIALBOLD) + 10, 10,
		        "every member, on ze record.");
		const INT32 top = CH_GB_TOP;
		const INT32 pageBot = CH_BANNER_Y - 6;
		// the page is a stack of separate cards on the chrome: header,
		// the stat boxes, then the ledger - each its own container
		FillRounded(px, top, pw, 78, CH_RGB_PANEL, CH_PANEL_RADIUS,
		            CH_RGB_CHROME);

		if (gProfFace)
		{
			BltVideoSurface(FRAME_BUFFER, gProfFace, CH_X(px + 12),
			                CH_Y(top + 10), NULL);
		}
		else
		{
			FillRounded(px + 12, top + 10, CH_GB_FACE, CH_GB_FACE,
			            FROMRGB(74, 69, 63), 4, CH_RGB_PANEL);
		}
		INT32 hx = px + 46;
		if (seat.title[0])
		{
			const INT32 tw = StringPixLength(seat.title, TINYFONT1) + 2;
			FillRounded(hx, top + 14, tw, 8, FROMRGB(146, 44, 44), 2,
			            CH_RGB_PANEL);
			PrintAt(TINYFONT1, FONT_MCOLOR_WHITE, hx + 1, top + 14, seat.title);
			hx += tw + 4;
		}
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, hx, top + 12, seat.handle);
		hx += StringPixLength(seat.handle, FONT10ARIALBOLD) + 5;
		ChessDrawFlag(hx, top + 13, seat.flag);
		// the bios follow the ladder's order; the daily man closes the list
		static const char* const bios[6] = {
			"answers only in moves.",
			"shot for denmark in '96.",
			"ex s.a.s. plays ze english.",
			"moves fast, reasons later.",
			"ze family opening, inherited.",
			"hangs ze queen. calls it a web.",
		};
		PrintAt(FONT10ARIAL, FONT_GRAY4, px + 46, top + 25,
		        giChessProfSeat == -2 ? "one move a day, by post."
		                              : bios[giChessProfSeat]);
		{
			const ST::string rate = ST::format("{}", seat.rating);
			PrintAt(FONT14ARIAL, FONT_MCOLOR_WHITE,
			        px + pw - 14 - StringPixLength(rate, FONT14ARIAL),
			        top + 10, rate);
			PrintAt(TINYFONT1, FONT_GRAY4,
			        px + pw - 14 - StringPixLength("rating", TINYFONT1),
			        top + 24, "rating");
		}

		// the challenge sits right under the name - or the postal notice
		if (giChessProfSeat >= 0)
		{
			ChessDrawCTAButton(px + 12, top + 44, 110, 22, CH_RGB_PANEL);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, px + 67,
			             top + 50, "CHALLENGE");
		}
		else
		{
			PrintAt(FONT10ARIAL, FONT_GRAY4, px + 12, top + 48,
			        "accepts challenges by post only. write ze mail.");
		}

		// the stat cards: their site totals (invented, but consistently),
		// and the head-to-head that is entirely real
		const INT32 bandY = top + 84;
		const INT32 boxW = (pw - 16) / 3;
		const INT32 boxH = 40;
		static const char* const caps[3] = { "RECORD", "VS YOU", "STREAK" };
		for (int b = 0; b < 3; ++b)
		{
			const INT32 bx = px + b * (boxW + 8);
			FillRounded(bx, bandY, boxW, boxH, CH_RGB_PANEL, 6,
			            CH_RGB_CHROME);
			PrintAt(TINYFONT1, FONT_GRAY4, bx + 8, bandY + 5, caps[b]);
		}
		{
			const int games = 96 + (int(seat.pid) * 37) % 320;
			const int wpct = std::clamp(32 + (seat.rating - 1500) / 18, 22, 72);
			const int w = games * wpct / 100;
			const int d = games * 11 / 100;
			PrintAt(FONT14ARIAL, FONT_MCOLOR_WHITE, px + 8, bandY + 17,
			        ST::format("{}-{}-{}", w, games - w - d, d));
		}
		{
			int vw = 0, vl = 0, vd = 0;
			const UINT8 want = giChessProfSeat == -2 ? 0xFE
			                                         : UINT8(giChessProfSeat);
			for (int i = 0; i < int(gubProfCount); ++i)
			{
				if (gProfHist[i].ubSeat != want) continue;
				const int r = gProfHist[i].ubResult & 3;
				if (r == 2) ++vw; else if (r == 0) ++vl; else ++vd;
			}
			const INT32 bx = px + boxW + 8;
			if (vw + vl + vd == 0)
			{
				// the blank state is a dash, not a sentence
				PrintAt(FONT14ARIAL, FONT_GRAY4, bx + 8, bandY + 17, "-");
			}
			else
			{
				PrintAt(FONT14ARIAL, FONT_MCOLOR_WHITE, bx + 8, bandY + 17,
				        ST::format("{}-{}-{}", vw, vl, vd));
			}
		}
		PrintAt(FONT14ARIAL, FONT_MCOLOR_WHITE, px + 2 * (boxW + 8) + 8,
		        bandY + 17,
		        ST::format("{}", 3 + (int(seat.pid) * 5) % 19));

		// recent games: your real games against them, woven into a feed of
		// invented ones against the other regulars - reseeded daily, so the
		// ladder looks alive whether or not you ever sit down
		const INT32 histTop = bandY + boxH + 6;
		FillRounded(px, histTop, pw, pageBot - histTop, CH_RGB_PANEL,
		            CH_PANEL_RADIUS, CH_RGB_CHROME);
		INT32 y = histTop + 8;
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, px + 12, y,
		        "RECENT GAMES");
		y += 16;
		struct ProfRow
		{
			ST::string opp;    // handle, with rating for the regulars
			const char* title; // their chip, or nullptr
			int flag;
			int res;           // 0 they lost, 1 draw, 2 they won
			bool resigned;     // only ever true on your games
			int moves;
			int control;       // minutes, 0 = daily
			ST::string date;   // campaign days for your games, the site's
			                   // own calendar for everything older
			int seatIdx;       // -1 you, else the opponent's own page
		};
		ProfRow rows[10];
		int n = 0;
		const UINT8 want = giChessProfSeat == -2 ? 0xFE
		                                         : UINT8(giChessProfSeat);
		const ST::string you = gChessSelfNick.empty()
			? ST::string("@commander") : ST::format("@{}", gChessSelfNick);
		for (int i = 0; i < int(gubProfCount) && n < 4; ++i)
		{
			if (gProfHist[i].ubSeat != want) continue;
			const ChessGameRec& r = gProfHist[i];
			const int yours = r.ubResult & 3;
			rows[n++] = ProfRow{ you, nullptr, CH_FLAG_NONE,
					yours == 2 ? 0 : yours == 0 ? 2 : 1,
					(r.ubResult & 4) != 0, r.ubMoves, r.ubControl,
					ST::format("day {}", r.usDay), -1 };
		}
		{
			UINT32 st = UINT32(seat.pid) * 2654435761u + GetWorldDay() * 97u;
			auto roll = [&st](int mod)
			{
				st = st * 1103515245u + 12345u;
				return int((st >> 16) % UINT32(mod));
			};
			// the ledger predates you: dates walk back through the site's
			// own history, guestbook-fashion, well before day 1
			int ym = 1999 * 12 + 3;  // april '99, and only downhill from there
			int dom = 1 + roll(27);
			while (n < 8)
			{
				// an opponent from the rest of the room, the daily man
				// included; never themselves
				int pick = roll(6);
				const ChessSeat* opp = pick == 5 ? &CHESS_SEAT_ENRICO
				                                 : &CHESS_SEATS[pick];
				if (opp->handle == seat.handle)
				{
					opp = &CHESS_SEATS[(pick + 1) % 6];
				}
				const int edge = std::clamp(
						50 + (seat.rating - opp->rating) / 8, 15, 85);
				const int r2 = roll(100);
				const int res = r2 < edge ? 2 : r2 < edge + 14 ? 1 : 0;
				static const int clocks[3] = { 1, 3, 10 };
				dom -= 4 + roll(18);
				while (dom < 1) { dom += 28; --ym; }
				if (roll(100) < 18) ym -= 5; // a quiet half year, now and then
				rows[n++] = ProfRow{ ST::string(opp->handle),
						opp->title[0] ? opp->title : nullptr, opp->flag,
						res, false, 14 + roll(48),
						opp == &CHESS_SEAT_ENRICO ? 0 : clocks[roll(3)],
						ST::format("{02d}/{02d}/{02d}", ym % 12 + 1, dom,
								(ym / 12) % 100),
						opp == &CHESS_SEAT_ENRICO
							? -2 : int(opp - CHESS_SEATS) };
			}
		}
		const INT32 cOpp = px + 12;
		const INT32 axRes = px + 164;
		const INT32 axMov = px + 202;
		const INT32 cDay = px + pw - 12;
		PrintAt(TINYFONT1, FONT_GRAY4, cOpp, y, "OPPONENT");
		PrintCentred(TINYFONT1, FONT_GRAY4, axRes, y, "RES");
		PrintCentred(TINYFONT1, FONT_GRAY4, axMov, y, "MOV");
		PrintAt(TINYFONT1, FONT_GRAY4,
		        cDay - StringPixLength("DATE", TINYFONT1), y, "DATE");
		y += 14;
		for (int i = 0; i < n; ++i)
		{
			if (y > pageBot - 18) break;
			const ProfRow& r = rows[i];
			if (i % 2 == 0)
			{
				// zebra flush with the card, move-list fashion
				FillRect(px, y - 3, pw, 20, CH_RGB_ROW_ALT);
			}
			// the chat-line dress: a small face leads the row
			SGPVSurface* chip = r.seatIdx == -1 ? nullptr
			                                    : ChessSeatChip(r.seatIdx);
			if (chip)
			{
				BltVideoSurface(FRAME_BUFFER, chip, CH_X(cOpp), CH_Y(y - 1),
				                NULL);
			}
			else if (r.seatIdx == -1 && gGuestSelfFace)
			{
				SGPBox const src = { 0, 0, CH_GB_FACE, CH_GB_FACE };
				SGPBox const dst = { UINT16(CH_X(cOpp)), UINT16(CH_Y(y - 1)),
				                     14, 14 };
				BltStretchVideoSurface(FRAME_BUFFER, gGuestSelfFace, &src,
				                       &dst);
			}
			INT32 tx = cOpp + 18;
			if (r.title)
			{
				const INT32 tw = StringPixLength(r.title, TINYFONT1) + 2;
				FillRounded(tx, y + 3, tw, 8, FROMRGB(146, 44, 44), 2,
				            i % 2 == 0 ? CH_RGB_ROW_ALT : CH_RGB_PANEL);
				PrintAt(TINYFONT1, FONT_MCOLOR_WHITE, tx + 1, y + 3, r.title);
				tx += tw + 4;
			}
			PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, tx, y + 2, r.opp);
			tx += StringPixLength(r.opp, FONT10ARIALBOLD) + 5;
			ChessDrawFlag(tx, y + 3, r.flag);
			const UINT8 rc = r.res == 2 ? FONT_MCOLOR_LTGREEN
			               : r.res == 0 ? FONT_MCOLOR_LTRED : FONT_GRAY2;
			const char* rs = r.res == 2 ? "1-0" : r.res == 0 ? "0-1" : "1/2";
			PrintCentred(FONT10ARIALBOLD, rc, axRes, y + 2, rs);
			PrintCentred(FONT10ARIAL, FONT_GRAY2, axMov, y + 2,
			             ST::format("{}", r.moves));
			PrintAt(FONT10ARIAL, FONT_GRAY2,
			        cDay - StringPixLength(r.date, FONT10ARIAL), y + 2, r.date);
			MOUSE_REGION& rr = gChessProfRowRegion[i];
			rr.RegionTopLeftX = INT16(CH_X(cOpp));
			rr.RegionTopLeftY = INT16(CH_Y(y - 3));
			rr.RegionBottomRightX = INT16(CH_X(axRes - 24));
			rr.RegionBottomRightY = INT16(CH_Y(y + 16));
			giProfRowTarget[i] = r.seatIdx;
			y += 20;
		}

		// the clock picker: the lobby's TIME CONTROL sidebar, verbatim,
		// floated over the dimmed page with a start button beneath
		if (gfChalModal)
		{
			FRAME_BUFFER->ShadowRect(CH_X(0), CH_Y(0),
			                         CH_X(LAPTOP_SCREEN_WIDTH) - 1,
			                         CH_Y(CH_PAGE_H) - 1);
			const INT32 mx = px + (pw - 160) / 2;
			const INT32 my = 92;
			FillRounded(mx - 2, my - 2, 164, 212, CH_RGB_CHROME, 6,
			            CH_RGB_PANEL);
			FillRounded(mx, my, 160, 208, CH_RGB_PANEL, 5, CH_RGB_CHROME);
			PrintCentred(FONT10ARIAL, FONT_GRAY4, mx + 80, my + 10,
			             "TIME CONTROL");
			for (int i = 0; i < 4; ++i)
			{
				ChessDrawControlRow(i, mx + 16, 122 + i * 30, 128,
				                    CH_RGB_PANEL);
			}
			// the START GAME cut: full CTA height, title-bar type
			ChessDrawCTAButton(mx + 16, 246, 128, CH_HINT_H, CH_RGB_PANEL);
			const ST::string play = ST::format("PLAY {}", seat.handle);
			if (StringPixLength(play, FONT14ARIAL) <= 120)
			{
				PrintCentred(FONT14ARIAL, FONT_MCOLOR_WHITE, mx + 80, 254,
				             play);
			}
			else
			{
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, mx + 80, 256,
				             play);
			}
			PrintCentred(FONT10ARIAL, FONT_GRAY4, mx + 80, 284, "nevermind");
		}
	}

	// Your member page, as every 1999 chess site kept one: the rating with
	// its provisional asterisk, the record, the form line, and a ledger of
	// games the server refuses to forget.
	void ChessRenderProfile()
	{
		for (int& t : giProfRowTarget) t = -3;
		if (giChessProfSeat != -1)
		{
			ChessRenderMemberProfile();
			return;
		}
		const INT32 px = CH_BOARD_X;
		const INT32 pw = CH_BOARD_SIZE;
		const ST::string handle = gChessSelfNick.empty()
			? ST::string("@commander") : ST::format("@{}", gChessSelfNick);
		const ST::string name = gChessSelfName.empty() ? ST::string("Commander")
		                                               : gChessSelfName;
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, px + 2, 10, "PROFILE");
		PrintAt(FONT10ARIAL, FONT_GRAY4,
		        px + 2 + StringPixLength("PROFILE", FONT10ARIALBOLD) + 10, 10,
		        "ze server remembers every game. apologies.");

		const INT32 top = CH_GB_TOP;
		const INT32 pageBot = CH_BANNER_Y - 6;
		FillRounded(px, top, pw, 78, CH_RGB_PANEL, CH_PANEL_RADIUS,
		            CH_RGB_CHROME);

		// the header: who you are, and the number the site holds over you
		if (gGuestSelfFace)
		{
			BltVideoSurface(FRAME_BUFFER, gGuestSelfFace, CH_X(px + 12),
			                CH_Y(top + 10), NULL);
		}
		else
		{
			FillRounded(px + 12, top + 10, CH_GB_FACE, CH_GB_FACE,
			            FROMRGB(74, 69, 63), 4, CH_RGB_PANEL);
		}
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, px + 46, top + 12, name);
		PrintAt(FONT10ARIAL, FONT_GRAY4,
		        px + 52 + StringPixLength(name, FONT10ARIALBOLD), top + 12,
		        handle);
		PrintAt(FONT10ARIAL, FONT_GRAY4, px + 46, top + 25,
		        "member no. 2");

		const int games = gusProfWins + gusProfLosses + gusProfDraws;
		{
			const ST::string rate =
				gusProfRating == 0 ? ST::string("unrated")
				: games < 10 ? ST::format("{}*", gusProfRating)
				             : ST::format("{}", gusProfRating);
			PrintAt(FONT14ARIAL, FONT_MCOLOR_WHITE,
			        px + pw - 14 - StringPixLength(rate, FONT14ARIAL),
			        top + 10, rate);
			const char* lbl = gusProfRating == 0 ? "play to earn one"
			                : games < 10 ? "rating (provisional)" : "rating";
			PrintAt(TINYFONT1, FONT_GRAY4,
			        px + pw - 14 - StringPixLength(lbl, TINYFONT1), top + 24,
			        lbl);
		}

		// the site reads your form back to you, inside the header card
		if (gubProfCount > 0)
		{
			const int kind = gProfHist[0].ubResult & 3;
			int run = 0;
			while (run < int(gubProfCount) &&
			       (gProfHist[run].ubResult & 3) == kind)
			{
				++run;
			}
			ST::string say;
			if (run >= 2 && kind == 0)
			{
				say = ST::format("{} losses running. ze other members have "
				                 "noticed.", run);
			}
			else if (run >= 2 && kind == 2)
			{
				say = ST::format("{} wins running. ze site suspects nothing, "
				                 "yet.", run);
			}
			else if (run >= 2)
			{
				say = ST::format("{} draws running. decisive as always.", run);
			}
			else
			{
				say = "form: inconclusive. ze data is thin.";
			}
			PrintAt(FONT10ARIAL, FONT_GRAY2, px + 12, top + 48, say);
		}

		// the stat cards: record, form, the puzzle habit
		const INT32 bandY = top + 84;
		const INT32 boxW = (pw - 16) / 3;
		const INT32 boxH = 40;
		static const char* const caps[3] = { "RECORD", "FORM", "STREAK" };
		for (int b = 0; b < 3; ++b)
		{
			const INT32 bx = px + b * (boxW + 8);
			FillRounded(bx, bandY, boxW, boxH, CH_RGB_PANEL, 6,
			            CH_RGB_CHROME);
			PrintAt(TINYFONT1, FONT_GRAY4, bx + 8, bandY + 5, caps[b]);
		}
		PrintAt(FONT14ARIAL, FONT_MCOLOR_WHITE, px + 8, bandY + 17,
		        ST::format("{}-{}-{}", gusProfWins, gusProfLosses,
		                   gusProfDraws));
		{
			// the form chips: one square per remembered game, newest left
			const INT32 fx0 = px + boxW + 8 + 8;
			for (int i = 0; i < std::min<int>(gubProfCount, 5); ++i)
			{
				const int r = gProfHist[i].ubResult & 3;
				const UINT32 c = r == 2 ? CH_RGB_CTA
				               : r == 1 ? FROMRGB(104, 98, 91)
				                        : CH_RGB_CHK_DARK;
				FillRounded(fx0 + i * 12, bandY + 19, 9, 9, c, 3,
				            CH_RGB_PANEL);
			}
			if (gubProfCount == 0)
			{
				PrintAt(FONT10ARIAL, FONT_GRAY4, fx0, bandY + 19, "-");
			}
		}
		{
			const INT32 sx = px + 2 * (boxW + 8) + 8;
			const ST::string sv = ST::format("{}", gChessDay.streak);
			PrintAt(FONT14ARIAL, FONT_MCOLOR_WHITE, sx, bandY + 17, sv);
			PrintAt(FONT10ARIAL, FONT_GRAY4,
			        sx + StringPixLength(sv, FONT14ARIAL) + 8, bandY + 21,
			        ST::format("best {}", gChessDay.bestStreak));
		}

		const INT32 histTop = bandY + boxH + 6;
		FillRounded(px, histTop, pw, pageBot - histTop, CH_RGB_PANEL,
		            CH_PANEL_RADIUS, CH_RGB_CHROME);
		INT32 y = histTop + 8;
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, px + 12, y,
		        "GAME HISTORY");
		y += 16;
		if (gubProfCount == 0)
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY4, px + pw / 2, y + 20,
			             "no games on record. ze board at PLAY is patient.");
			return;
		}
		const INT32 cOpp = px + 12;
		const INT32 axRes = px + 164;
		const INT32 axMov = px + 202;
		const INT32 cDay = px + pw - 12;
		PrintAt(TINYFONT1, FONT_GRAY4, cOpp, y, "OPPONENT");
		PrintCentred(TINYFONT1, FONT_GRAY4, axRes, y, "RES");
		PrintCentred(TINYFONT1, FONT_GRAY4, axMov, y, "MOV");
		PrintAt(TINYFONT1, FONT_GRAY4,
		        cDay - StringPixLength("DAY", TINYFONT1), y, "DAY");
		y += 14;
		const int maxRows = std::max<INT32>(0, (pageBot - 8 - y) / 20);
		const int rows = std::min<int>(gubProfCount, maxRows);
		for (int i = 0; i < rows; ++i)
		{
			const ChessGameRec& r = gProfHist[i];
			if (i % 2 == 0) FillRect(px, y - 3, pw, 20, CH_RGB_ROW_ALT);
			const int seatIdx = r.ubSeat == 0xFE ? -2 : int(r.ubSeat);
			const ChessSeat& opp = seatIdx == -2 ? CHESS_SEAT_ENRICO
			                                     : CHESS_SEATS[seatIdx];
			if (SGPVSurface* chip = ChessSeatChip(seatIdx))
			{
				BltVideoSurface(FRAME_BUFFER, chip, CH_X(cOpp), CH_Y(y - 1),
				                NULL);
			}
			INT32 tx = cOpp + 18;
			if (opp.title[0])
			{
				// the crimson title chip, chess.com fashion
				const INT32 tw = StringPixLength(opp.title, TINYFONT1) + 2;
				FillRounded(tx, y + 3, tw, 8, FROMRGB(146, 44, 44), 2,
				            i % 2 == 0 ? CH_RGB_ROW_ALT : CH_RGB_PANEL);
				PrintAt(TINYFONT1, FONT_MCOLOR_WHITE, tx + 1, y + 3,
				        opp.title);
				tx += tw + 4;
			}
			PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, tx, y + 2, opp.handle);
			tx += StringPixLength(opp.handle, FONT10ARIALBOLD) + 5;
			ChessDrawFlag(tx, y + 3, opp.flag);
			const int res = r.ubResult & 3;
			const UINT8 rc = res == 2 ? FONT_MCOLOR_LTGREEN
			               : res == 0 ? FONT_MCOLOR_LTRED : FONT_GRAY2;
			const char* rs = res == 2 ? "1-0" : res == 0 ? "0-1" : "1/2";
			PrintCentred(FONT10ARIALBOLD, rc, axRes, y + 2, rs);
			if (r.ubResult & 4)
			{
				PrintAt(TINYFONT1, FONT_GRAY4,
				        axRes + StringPixLength(rs, FONT10ARIALBOLD) / 2 + 3,
				        y + 5, "res.");
			}
			PrintCentred(FONT10ARIAL, FONT_GRAY2, axMov, y + 2,
			             ST::format("{}", r.ubMoves));
			const ST::string dd = ST::format("day {}", r.usDay);
			PrintAt(FONT10ARIAL, FONT_GRAY2,
			        cDay - StringPixLength(dd, FONT10ARIAL), y + 2, dd);
			MOUSE_REGION& rr = gChessProfRowRegion[i];
			rr.RegionTopLeftX = INT16(CH_X(cOpp));
			rr.RegionTopLeftY = INT16(CH_Y(y - 3));
			rr.RegionBottomRightX = INT16(CH_X(axRes - 24));
			rr.RegionBottomRightY = INT16(CH_Y(y + 16));
			giProfRowTarget[i] = seatIdx;
			y += 20;
		}
	}

	void ChessRenderGuestbook()
	{
		ChessIconLabel(CH_ICON_COMMUNITY, CH_BOARD_X + 2, 15, FONT10ARIALBOLD,
		               FONT_MCOLOR_WHITE, T(CHS_GB_TITLE));
		PrintAt(FONT10ARIAL, FONT_GRAY4,
		        CH_BOARD_X + 2 + ChessIconLabelWidth(FONT10ARIALBOLD, T(CHS_GB_TITLE)) + 10,
		        15 - GetFontHeight(FONT10ARIAL) / 2, T(CHS_GB_PROMPT));
		// the hit counter lives where hit counters lived: in the guestbook,
		// in the accent green, believable to nobody
		{
			const int hits = 148299 + giChessViewDay * 17 + gChessDay.bestStreak * 3;
			const ST::string count = ST::format("no. {}", hits);
			PrintAt(TINYFONT1, FONT_MCOLOR_LTGREEN,
			        CH_BOARD_X + CH_BOARD_SIZE - StringPixLength(count, TINYFONT1), 12, count);
		}

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
		PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, 10, "- ADVERTISEMENT -");
		// the card runs level with the book beside it
		const INT32 y = CH_GB_TOP;
		const INT32 h = CH_PAGE_H - CH_INSET - y;
		// the skyscraper serves from the same impression counter as the strip,
		// so the crown (and its one-click letter) is always where the click
		// handler thinks it is; without a creative of its own the big two
		// alternate
		int slot = ChessAdSlot();
		if (slot != 1) slot = (slot % 2 != 0) ? 2 : 0;
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
	if (ChessWatchChatFocused())
	{
		if (usParam == SDLK_RETURN || usParam == SDLK_KP_ENTER)
		{
			while (!gWatchChatInput.empty() && gWatchChatInput.back() == ' ')
				gWatchChatInput.pop_back();
			if (!gWatchChatInput.empty())
			{
				const ST::string me = gChessSelfNick.empty()
					? ST::string("@you") : ST::format("@{}", gChessSelfNick);
				if (giChessStub == 0)
				{
					ChessPlayChatSay(me, ST::string(gWatchChatInput.c_str()));
					// he answers eventually. He is mid-game, after all.
					if (Random(3) != 0) guiPlayChatReplyDue = ChessNow() + 2000 + Random(5000);
				}
				else
				{
					ChessWatchChatSay(me, ST::string(gWatchChatInput.c_str()));
					// somebody always has something to say about it
					if (Random(2) == 0) guiWatchReplyDue = ChessNow() + 1500 + Random(3500);
				}
				gWatchChatInput.clear();
				ChessPlay(CH_SND_CLICK2, LOWVOLUME);
				ChessRedraw();
			}
			return true;
		}
		if (usParam == SDLK_BACKSPACE)
		{
			if (!gWatchChatInput.empty())
			{
				gWatchChatInput.pop_back();
				ChessRedraw();
			}
			return true;
		}
		if (usParam >= 32 && usParam < 127) return true;  // arrives as TEXT_INPUT
		return false;  // shortcuts still work from the chat
	}
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
	if (ChessWatchChatFocused())
	{
		bool changed = false;
		for (char32_t cp : codepoints)
		{
			if (cp < 32 || cp > 126) continue;
			if (gWatchChatInput.size() >= CH_WCHAT_MAX) break;
			gWatchChatInput += char(cp);
			changed = true;
		}
		if (changed) ChessRedraw();
		return true;
	}
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
	guiChessFlags  = nullptr;
	guiChessLogo   = nullptr;
	guiChessAdDragon = nullptr;
	guiChessAdTiles  = nullptr;
	guiChessSelf   = nullptr;
	giChessStub    = -1;
	giChessGbPage  = 0;
	// a fresh visit reseeds the ad server; every page change advances it
	guiChessAdImpression = ChessToday();
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
		// on its own, so an old install without the sheet still gets its
		// icons, logo and ads - the flags are the one decoration nothing
		// else depends on
		guiChessFlags  = AddVideoObjectFromFile("sti/laptop/chessflags.sti");
	}
	catch (...)
	{
		// no flags: handles simply stand without one
	}
	try
	{
		guiChessIcons  = AddVideoObjectFromFile("sti/laptop/chessicons.sti");
		guiChessLogo   = AddVideoObjectFromFile("sti/laptop/chesslogo.sti");
		guiChessAdDragon = AddVideoObjectFromFile("sti/laptop/mahjongdragon.sti");
		guiChessAdTiles  = AddVideoObjectFromFile("sti/laptop/mahjongtiles.sti");
		gAdArtAim  = ChessBakeArtObj(AddVideoObjectFromFile(MLG_AIMSYMBOL), 0, 28, 104,
		                             FROMRGB(46, 34, 26));
		gAdArtBobby   = ChessBakeArtObj(AddVideoObjectFromFile(MLG_BOBBYRAYAD21), 0, 46, 266,
		                                FROMRGB(0, 0, 0));
		gAdArtFuneral = ChessBakeArtObj(AddVideoObjectFromFile(MLG_FUNERALAD9), 0, 46, 266,
		                                FROMRGB(0, 0, 0));
		gAdArtFlower  = ChessBakeArt(LAPTOPDIR "/flowerad_16.sti", 0, 46, 266, FROMRGB(0, 0, 0));
		gAdArtIns     = ChessBakeArtObj(AddVideoObjectFromFile(MLG_INSURANCEAD10), 0, 46, 266,
		                                FROMRGB(0, 0, 0));
	}
	catch (...)
	{
		// chrome only: labels stand on their own without the icons
	}
	try
	{
		// Buns coaches: a kindergarten teacher before A.I.M., and a Danish
		// sharpshooter before that. The language switch is the proprietor's
		// doing, not hers. Merc faces have to come through
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
			if (want == 2)
			{
				gLearnGame.SetFen(CHESS_LESSONS[giChessLesson].fen);
				ChessLearnReset();
			}
			if (want == 3 && guiWatchNextMove == 0)
			{
				ChessWatchNewGame();
				guiWatchNextMove = ChessNow() + 1600 + Random(1800);
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
	if (guiChessFlags)  { DeleteVideoObject(guiChessFlags);  guiChessFlags  = nullptr; }
	if (guiChessLogo)   { DeleteVideoObject(guiChessLogo);   guiChessLogo   = nullptr; }
	if (guiChessAdDragon) { DeleteVideoObject(guiChessAdDragon); guiChessAdDragon = nullptr; }
	if (guiChessAdTiles)  { DeleteVideoObject(guiChessAdTiles);  guiChessAdTiles  = nullptr; }
	for (SGPVSurface** art : { &gAdArtAim, &gAdArtBobby,
	                           &gAdArtFuneral, &gAdArtFlower, &gAdArtIns })
	{
		if (*art) { DeleteVideoSurface(*art); *art = nullptr; }
	}
	if (guiChessSelf)   { DeleteVideoObject(guiChessSelf);   guiChessSelf   = nullptr; }
	if (guiChessSelfHalf)  { DeleteVideoSurface(guiChessSelfHalf);  guiChessSelfHalf  = nullptr; }
	if (gPlayOppFace)      { DeleteVideoSurface(gPlayOppFace);      gPlayOppFace      = nullptr; }
	if (guiChessCoachHalf) { DeleteVideoSurface(guiChessCoachHalf); guiChessCoachHalf = nullptr; }
	if (gGuestSelfFace)    { DeleteVideoSurface(gGuestSelfFace);    gGuestSelfFace    = nullptr; }
	if (gProfFace)         { DeleteVideoSurface(gProfFace);         gProfFace         = nullptr; }
	for (SGPVSurface*& c : gSeatChip)
	{
		if (c) { DeleteVideoSurface(c); c = nullptr; }
	}
	giProfFaceFor = -3;
	giChessProfSeat = -1;
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
	else if (giChessStub == 5)
	{
		ChessRenderProfile();
		ChessRenderGuestAd();
	}
	else if (giChessStub == 2)
	{
		ChessRenderLearn();
		ChessRenderBanner();
		ChessRenderLearnPanel();
		ChessRenderFooter();
		ChessRenderModal(); // the graduation card, on the last lesson
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
		if (giPlayState == 3 || giPlayState == 4 || giPlaySeat == -1)
		{
			// no opponent yet: a grey tile with a dark bust, cut to the
			// exact footprint a real face chip occupies in the row
			const INT32 tileX = CH_BOARD_X + (CH_SQ - CH_ROW_FACE) / 2;
			const INT32 tileY = CH_ROW_TOP_Y + (CH_SQ - CH_ROW_FACE) / 2;
			FillRounded(tileX, tileY, CH_ROW_FACE, CH_ROW_FACE,
			            FROMRGB(74, 69, 63), 3, CH_RGB_CHROME);
			const UINT32 bust = FROMRGB(40, 37, 33);
			FillRounded(tileX + 11, tileY + 5, 8, 8, bust, 3,
			            FROMRGB(74, 69, 63));
			FillRounded(tileX + 5, tileY + 15, 20, 13, bust, 6,
			            FROMRGB(74, 69, 63));
			PrintAt(FONT10ARIALBOLD, FONT_GRAY4, CH_BOARD_X + CH_SQ + 2,
			        CH_ROW_TOP_Y + 3, "Opponent");
			// the clock chip already shows the control you picked
			const INT32 boxW = 40;
			const INT32 boxX = CH_BOARD_X + CH_BOARD_SIZE - boxW;
			FillRounded(boxX, CH_ROW_TOP_Y + 2, boxW, 18, CH_RGB_PANEL_SUNK, 3,
			            CH_RGB_CHROME);
			PrintCentred(FONT10ARIAL, FONT_GRAY4, boxX + boxW / 2, CH_ROW_TOP_Y + 6,
			             giPlayMinutes == 0 ? ST::string("1 day")
			                                : ST::format("{}:00", giPlayMinutes));
		}
		else
		{
			const ChessSeat& opp = ChessOpponent();
			ChessRenderPlayerRow(gPlayOppFace, opp.handle,
			                     ST::format("({})", opp.rating), CH_ROW_TOP_Y,
			                     &gPlayGame, ChessGame::Black, pPlies, giPlayState == 1,
			                     opp.title, giPlayControl * 60,
			                     giPlayControl >= 5 ? giPlayControl : 1);
		}
		ChessRenderPlayerRow(guiChessSelfHalf,
		                     gChessSelfNick.empty() ? ST::string("@you")
		                                            : ST::format("@{}", gChessSelfNick),
		                     ST::string(), CH_ROW_BOT_Y,
		                     &gPlayGame, ChessGame::White, pPlies, giPlayState == 0,
		                     nullptr, giPlayControl * 60,
		                     giPlayControl >= 5 ? giPlayControl : 1);
		ChessRenderBanner();
		ChessRenderPlayPanel();
		ChessRenderPlayModal();
	}
	else if (giChessStub >= 0)
	{
		ChessRenderStub();
		ChessRenderBanner();
		ChessRenderFooter();
		ChessRenderModal();
	}
	else
	{
		ChessRenderBoard();
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
	// the result card arrives a beat after the final move
	if (guiPlayModalDue != 0 && ChessNow() >= guiPlayModalDue)
	{
		guiPlayModalDue = 0;
		gfPlayModal = true;
		ChessPlay(CH_SND_GAMEEND);
		ChessSyncPageRegions();
		ChessRedraw();
	}

	// the lesson's target square blinks while the move is owed
	if (giChessStub == 2 && !gfLearnSolved)
	{
		static UINT32 sBlink = 0;
		const UINT32 blink = ChessNow() / 450;
		if (blink != sBlink)
		{
			sBlink = blink;
			ChessRedraw();
		}
	}

	// the hover ring follows the pointer across the board
	{
		static UINT8 sHoverSq = ChessGame::NO_SQUARE;
		const UINT8 hov = ChessSquareUnderPointer();
		if (hov != sHoverSq)
		{
			sHoverSq = hov;
			ChessRedraw();
		}
	}

	// the seek spinner turns on its own clock
	if (giChessStub == 0 && giPlayState == 3)
	{
		static int sSpinPhase = -1;
		const int phase = int((ChessNow() / 120) % 8);
		if (phase != sSpinPhase)
		{
			sSpinPhase = phase;
			ChessRedraw();
		}
	}

	// the coach is mid-sentence: repaint until the line is out
	if (giChessStub < 0 && ChessCoachStillTyping())
	{
		ChessRedraw();
	}

	// the chat caret blinks while the tab is up and something is typed
	if (ChessWatchChatFocused() && !gWatchChatInput.empty())
	{
		static bool sChatCaret = false;
		const bool caret = (ChessNow() / 400) & 1;
		if (caret != sChatCaret) { sChatCaret = caret; ChessRedraw(); }
	}

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
		giPlaySeat = giPlayRematchSeat != -1 ? giPlayRematchSeat
		           : giPlayControl == 0     ? -2 : int(Random(6));
		giPlayRematchSeat = -1;
		ChessPlay(CH_SND_GAMESTART); // the connect: an opponent is found
		try
		{
			const ChessSeat& opp = ChessOpponent();
			SGPVObject* face = ChessLoadPortrait(GetProfile(opp.pid));
			gPlayOppFace = ChessBakeFace(face, CH_ROW_FACE);
			DeleteVideoObject(face);
			if (giPlayControl == 0)
			{
				ChessPlayChatSay(ST::string(),
				                 ST::format("DAILY: {} ({}) - one move a day",
				                            opp.handle, opp.rating));
				ChessPlayChatSay(opp.handle,
				                 "a game by letter, friend. take your day.");
			}
			else
			{
				ChessPlayChatSay(ST::string(),
				                 ST::format("PLAYING: {} ({}) - {} min",
				                            opp.handle, opp.rating, giPlayControl));
				ChessPlayChatSay(opp.handle, "gl.");
			}
		}
		catch (...) {}
		giPlayState = 0;
		giPlaySaid  = CHS_PLAY_YOUR;
		ChessSyncPageRegions();
		ChessPlay(CH_SND_CLICK2, LOWVOLUME);
		ChessRedraw();
	}

	// the letter-writer answers when the campaign day turns
	if (giChessStub == 0 && giPlayState == 5 && GetWorldDay() > guiPlayDailyDay)
	{
		giPlayState = 1;
		giPlaySaid  = CHS_PLAY_THINK;
		guiPlayDue  = ChessNow() + 900 + Random(1200);
		ChessSyncPageRegions();
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
			gPlayHist.push_back(gPlayGame);
			ChessAnimateMove(gPlayGame, m, 140);
			ChessPlay(ChessMoveSound(m, gPlayGame.IsInCheck(gPlayGame.SideToMove()), false));
		}
		giPlayState = 0;
		giPlaySaid  = CHS_PLAY_YOUR;
		ChessPlayFinish();
		ChessSyncPageRegions();
		ChessRedraw();
	}

	// the room talks on its own clock, in little bursts
	if (giChessStub == 3 && ChessNow() >= guiWatchChatDue)
	{
		guiWatchChatDue = ChessNow() + 3000 + Random(9000);
		ChessWatchChatFrom(CHESS_CHAT_IDLE);
		if (Random(3) == 0) ChessWatchChatFrom(CHESS_CHAT_IDLE);
		if (giWatchTab == 1) ChessRedraw();
	}
	if (giChessStub == 0 && giPlaySeat != -1 && guiPlayChatReplyDue &&
		ChessNow() >= guiPlayChatReplyDue)
	{
		guiPlayChatReplyDue = 0;
		ChessPlayChatSay(ChessOpponent().handle,
		                 CHESS_CHAT_OPP[Random(lengthof(CHESS_CHAT_OPP))]);
		if (giPlayTab == 1) ChessRedraw();
	}

	if (giChessStub == 3 && guiWatchReplyDue && ChessNow() >= guiWatchReplyDue)
	{
		guiWatchReplyDue = 0;
		ChessWatchChatFrom(CHESS_CHAT_REPLY);
		if (giWatchTab == 1) ChessRedraw();
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
				const ST::string san = gWatchGame.San(m);
				gWatchSan.push_back(san);
				gubWatchFrom = m.from; gubWatchTo = m.to;
				gWatchGame.MakeMove(m);
				gWatchHist.push_back(gWatchGame);
				ChessAnimateMove(gWatchGame, m, 150);
				// the exhibition speaks at full board volume: move, capture,
				// check - the same cues as your own games
				const bool check = gWatchGame.IsInCheck(gWatchGame.SideToMove());
				ChessPlay(ChessMoveSound(m, check, false));
				// the room reacts to the loud moves, sometimes
				if (check && Random(2) == 0) ChessWatchChatFrom(CHESS_CHAT_CHECK);
				else if (std::strchr(san.c_str(), 'x') && Random(3) == 0)
				{
					ChessWatchChatFrom(CHESS_CHAT_CAPTURE);
				}
			}
			if (m.IsNull() || gWatchGame.GetResult() != ChessGame::Result::Ongoing)
			{
				giWatchResult = 1;
				ChessWatchChatSay(ST::string(), "GAME OVER");
				ChessWatchChatFrom(CHESS_CHAT_OVER);
				ChessWatchChatFrom(CHESS_CHAT_OVER);
			}
		}
		// ten-minute chess is played at a ten-minute pace: a few seconds of
		// thought as a rule, and every so often somebody sinks into a tank
		guiWatchNextMove = ChessNow() + (giWatchResult ? 3600
			: Random(10) == 0 ? 8000 + Random(6000)
			                  : 2200 + Random(3200));
		ChessRedraw();
	}

	// a piece in hand has to be repainted every frame to keep up with the
	// pointer, and the drop is resolved here rather than in a region
	// callback. Touch input never reads as a held mouse button, so the
	// main finger counts as holding the piece too.
	if (gfChessDragging)
	{
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMainFingerDown())
		{
			ChessRedraw();
		}
		else
		{
			ChessResolveDrop();
		}
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
