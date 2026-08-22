// C.U.P.I.D. - the Certified Union of Professionals In Dating. "Where
// the tough get tender." A Speck T. Kline company (formerly Mercs & Kisses)
// (not affiliated with M.E.R.C. - it is).
//
// A 1999 dating site for the A.I.M. and M.E.R.C. rosters, dealt one member at
// a time: big photo, name, a number that claims to be science. Drag the card
// (or use the buttons, or the arrow keys) - left is no, right is a like, and
// likes run out, because Speck sells refills. Mutual likes become matches.
//
// The questionnaire is I.M.P.'s own - the site licenses it ("used with
// permission"; it is not) and matches members on their 16 raw answers, which
// I.M.P. has thrown away after every compile since 1999 and which
// CupidRecordImpAnswers() now banks. Matching lives in the engine-free
// DatingGame core; this file is the laptop page wrapper.

#include "Cupid.h"

#include "DatingGame.h"

#include "Button_System.h"
#include "ContentManager.h"
#include "Cursors.h"
#include "Directories.h"
#include "GameRes.h"
#include "EMail.h"
#include "Finances.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "Game_Event_Hook.h"
#include "Random.h"
#include "GameInstance.h"
#include "HImage.h"
#include "IMP_Compile_Character.h"
#include "Input.h"
#include "Laptop.h"
#include "LaptopSave.h"
#include "MERCListingModel.h"
#include "MercPortrait.h"
#include "MercProfile.h"
#include "Mercs.h"
#include "MouseSystem.h"
#include "Quests.h"
#include "Soldier_Add.h"
#include "Soldier_Profile.h"
#include "Timer_Control.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"
#include "WordWrap.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <string_theory/format>
#include <string_theory/string>

#include <SDL_keycode.h>

// the mirrored enums in the engine-free core must track the game's
static_assert(int(DatingGame::ATT_NORMAL) == int(ATT_NORMAL));
static_assert(int(DatingGame::ATT_FRIENDLY) == int(ATT_FRIENDLY));
static_assert(int(DatingGame::ATT_LONER) == int(ATT_LONER));
static_assert(int(DatingGame::ATT_OPTIMIST) == int(ATT_OPTIMIST));
static_assert(int(DatingGame::ATT_PESSIMIST) == int(ATT_PESSIMIST));
static_assert(int(DatingGame::ATT_AGGRESSIVE) == int(ATT_AGGRESSIVE));
static_assert(int(DatingGame::ATT_ARROGANT) == int(ATT_ARROGANT));
static_assert(int(DatingGame::ATT_BIG_SHOT) == int(ATT_BIG_SHOT));
static_assert(int(DatingGame::ATT_ASSHOLE) == int(ATT_ASSHOLE));
static_assert(int(DatingGame::ATT_COWARD) == int(ATT_COWARD));
static_assert(int(DatingGame::NUM_ATTITUDES) == int(NUM_ATTITUDES));
static_assert(int(DatingGame::TRAIT_NONE) == int(NO_PERSONALITYTRAIT));
static_assert(int(DatingGame::TRAIT_HEAT_INTOLERANT) == int(HEAT_INTOLERANT));
static_assert(int(DatingGame::TRAIT_NERVOUS) == int(NERVOUS));
static_assert(int(DatingGame::TRAIT_CLAUSTROPHOBIC) == int(CLAUSTROPHOBIC));
static_assert(int(DatingGame::TRAIT_NONSWIMMER) == int(NONSWIMMER));
static_assert(int(DatingGame::TRAIT_FEAR_OF_INSECTS) == int(FEAR_OF_INSECTS));
static_assert(int(DatingGame::TRAIT_FORGETFUL) == int(FORGETFUL));
static_assert(int(DatingGame::TRAIT_PSYCHO) == int(PSYCHO));
static_assert(int(DatingGame::SX_NOT_SEXIST) == int(NOT_SEXIST));
static_assert(int(DatingGame::SX_SOMEWHAT_SEXIST) == int(SOMEWHAT_SEXIST));
static_assert(int(DatingGame::SX_VERY_SEXIST) == int(VERY_SEXIST));
static_assert(int(DatingGame::SX_GENTLEMAN) == int(GENTLEMAN));
static_assert(int(DatingGame::SEX_MALE) == int(MALE));
static_assert(int(DatingGame::SEX_FEMALE) == int(FEMALE));

#define CP_X(x) ((INT32)(LAPTOP_SCREEN_UL_X + (x)))
#define CP_Y(y) ((INT32)(LAPTOP_SCREEN_WEB_UL_Y + (y)))

// --- layout: one stage, one card ------------------------------------------
#define CP_PAGE_W       LAPTOP_SCREEN_WIDTH
#define CP_PAGE_H       400
#define CP_RADIUS       5

#define CP_TOPBAR_H     24

// the card, dealt sideways: photo on the left, dossier on the right,
// the whole stage wide
#define CP_CARD_W       366
#define CP_CARD_H       302
#define CP_CARD_X       128
#define CP_CARD_Y       8
#define CP_PHOTO_W      106
#define CP_PHOTO_H      122
// the leaderboard ad beneath the card - the era's one true banner shape
#define CP_AD_Y         318
#define CP_AD_H         60

// swipe thresholds and animation speed, in pixels
#define CP_SWIPE_COMMIT 55
#define CP_FLY_STEP     34

// the two verdict circles sit under the photo at the card's edges, the
// name held between them
#define CP_BTN_SIZE     36
// the action rail: a vertical toolbar inside the card's right edge -
// pager on top, then the three verdicts, so controls never fight text
#define CP_RAIL_X       (CP_CARD_X + 318)
#define CP_BTN_X        (CP_CARD_X + 322)
#define CP_BTN_KILL_Y   (CP_CARD_Y + 68)
#define CP_BTN_KISS_Y   (CP_CARD_Y + 114)
#define CP_BTN_MARRY_Y  (CP_CARD_Y + 174)

// the "small" face is the A.I.M. mugshot (faces/NN.sti frame 0), measured
// from the game data: 48x43. The 33face files are 14x15 tactical heads and
// the merc 65faces are 31x27 - neither survives a layout.
#define CP_FACE_SM_W    48
#define CP_FACE_SM_H    43

// the flank columns: 112 wide at the page edges, two mugshots abreast
#define CP_COL_W        112
#define CP_LCOL_X       8
#define CP_RCOL_X       (502 - 8 - CP_COL_W)

// the content column for the rail pages (ME, MATCHES, the quiz): centred
// in the void between the menu rail and the skyscraper, so CP_CONT_CX
// lands on the same 251 the deck card is dealt around
#define CP_CONT_X       137
#define CP_CONT_W       228
#define CP_CONT_CX      (CP_CONT_X + CP_CONT_W / 2)
// a member card dealt inside the content column (the ME and DETAIL pages)
#define CP_CONT_CARD_X  (CP_CONT_X + (CP_CONT_W - CP_CARD_W) / 2)

// the lounge goes full immersion: no banner, the channel takes the whole
// stage from the rail to the page edge
#define CPL_X           128
#define CPL_W           (502 - 8 - CPL_X)
#define CPL_H           352
#define CPL_SAY_Y       366
// the chat column, parlour fashion: a 14px chip rail, then one fixed
// text column; every log entry is one pre-wrapped 14px row
#define CPL_TEXT_X      30
#define CPL_TEXT_W      (CPL_W - CPL_TEXT_X - 16)
#define CPL_ROW_H       14
#define CPL_CLUSTER_GAP 4

// the match splash, demoted from takeover to popup
#define CP_SPL_W        330
#define CP_SPL_H        254
#define CP_SPL_X        (CP_CARD_X + (CP_CARD_W - CP_SPL_W) / 2)
#define CP_SPL_Y        60
#define CP_SPL_BTN_Y    (CP_SPL_Y + CP_SPL_H - 34)

// the private line: the chat card reuses the deck's footprint
#define CPC_X           CPL_X
#define CPC_Y           8
#define CPC_H           302
#define CPC_HEAD_H      40
#define CPC_LOG_Y       (CPC_Y + 48)
#define CPC_LOG_H       206
#define CPC_SAY_Y       (CPC_Y + CPC_H - 34)
#define CPC_TEXT_W      258

#define CP_FREE_LIKES_A_DAY 10
#define CP_RETAKE_PRICE     25
#define CP_GOLD_PRICE       10

// cupidicons.sti frame order
#define CP_ICON_VERIFIED 5
#define CP_ICON_HEART    6

// The palette: powder blue and hot pink on off-white, dark ink for the type.
// A 1999 dating site did not do subtle, and neither does Speck.
// after dark: plum-black night, maroon panels, pale pink type
#define CP_RGB_BG        FROMRGB( 34,  26,  36)
#define CP_RGB_BLUE      FROMRGB( 58,  46,  66)
#define CP_RGB_BLUE_DK   FROMRGB(150, 120, 168)
#define CP_RGB_BLUE_PALE FROMRGB( 44,  36,  50)
#define CP_RGB_CARD      FROMRGB( 74,  20,  28)
#define CP_RGB_CARD_DIM  FROMRGB( 96,  34,  42)
#define CP_RGB_INK       FROMRGB( 14,  10,  16)
#define CP_RGB_PINK      FROMRGB(230,  62, 118)
#define CP_RGB_PINK_DK   FROMRGB(178,  30,  82)
#define CP_RGB_GOLD      FROMRGB(202, 158,  74)
#define CP_RGB_LIKE      FROMRGB( 78, 176,  86)
#define CP_RGB_NOPE      FROMRGB(216,  64,  58)
#define CP_RGB_GREY      FROMRGB(128, 122, 128)
#define CP_RGB_SHADOW    FROMRGB( 10,   6,  12)
#define CP_RGB_PINK_PALE FROMRGB( 86,  40,  58)
#define CP_RGB_BLUE_WALL FROMRGB(120, 104, 136)
#define CP_RGB_MAT       FROMRGB(240, 232, 238)
// the aqua-gel tints: every control gets a lit top half and a shaded foot
#define CP_RGB_PINK_LITE FROMRGB(244, 136, 172)
#define CP_RGB_BLUE_LITE FROMRGB( 66,  56,  76)
#define CP_RGB_GOLD_LITE FROMRGB(228, 196, 128)
#define CP_RGB_GLOSS     FROMRGB(222, 206, 224)
#define CP_RGB_CARD_LITE FROMRGB( 92,  30,  40)

namespace
{
	enum CupidPage
	{
		CPP_DECK,
		CPP_MATCHES,
		CPP_ME,      // profile: import / take / retake the questionnaire
		CPP_DETAIL,  // one member's dossier, from a match or the deck photo
		CPP_SPLASH,  // IT'S A MATCH
		CPP_LOUNGE,  // #tender: the open flirt channel; lurkers welcome
		CPP_CHAT,    // a private line to one match, no audience
		CPP_ABOUT,   // the site's own story, via the webmaster link
	};

	// --- persistence -------------------------------------------------------
	CupidPersist gCupidPersist = { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF }, 0, 0, 0, 0, 0, {}, {}, 0, 0, 0, {}, 0 };

	UINT8 GetAnswer(int q)
	{
		const UINT8 b = gCupidPersist.ubAnswers[q / 2];
		return (q & 1) ? (b >> 4) : (b & 0x0F);
	}

	void SetAnswer(int q, UINT8 a)
	{
		UINT8& b = gCupidPersist.ubAnswers[q / 2];
		if (q & 1) b = UINT8((b & 0x0F) | (a << 4));
		else       b = UINT8((b & 0xF0) | (a & 0x0F));
	}

	// profile editing, 1999 style: your headline and summary are picked
	// from the site's list; 0 means the questionnaire's own choice
	int SelfHeadlineIdx(int att)
	{
		const int pick = gCupidPersist.ubSpin & 0x0F;
		return pick >= 1 && pick <= NUM_ATTITUDES ? pick - 1 : att;
	}

	int SelfBioIdx(int att)
	{
		const int pick = (gCupidPersist.ubSpin >> 4) & 0x0F;
		return pick >= 1 && pick <= NUM_ATTITUDES ? pick - 1 : att;
	}

	bool HaveStoredAnswers()
	{
		for (int q = 0; q < DatingGame::NUM_QUESTIONS; ++q)
		{
			if (GetAnswer(q) != DatingGame::NO_ANSWER) return true;
		}
		return false;
	}

	bool BitGet(const UINT8 (&mask)[8], ProfileID pid)
	{
		return pid < 64 && (mask[pid / 8] & (1 << (pid % 8))) != 0;
	}

	void BitSet(UINT8 (&mask)[8], ProfileID pid)
	{
		if (pid < 64) mask[pid / 8] |= UINT8(1 << (pid % 8));
	}

	void BitClear(UINT8 (&mask)[8], ProfileID pid)
	{
		if (pid < 64) mask[pid / 8] &= UINT8(~(1 << (pid % 8)));
	}

	bool IsGold() { return (gCupidPersist.ubFlags & CUPID_FLAG_GOLD) != 0; }

	// --- session state -----------------------------------------------------
	CupidPage gCupidPage = CPP_DECK;
	INT8 gCupidRailWho = -1; // lounge: roster index whose card borrows the rail
	CupidPage gCupidDetailFrom = CPP_MATCHES; // where BACK goes
	ProfileID gCupidDetailPid = 0;
	ProfileID gCupidSplashPid = 0;
	bool      gfCupidGerman = false;
	bool      gfCupidRegionsUp = false;

	// the questionnaire (the ME page): idle, or live at the sex question (-1)
	// or at question 0..15
	bool gfCupidQuizLive = false;
	int  giCupidQuizQ = -1;
	// measured per question: the rows are as tall as their text
	INT16 gsCupidQuizTop = 96;
	INT16 gsCupidAnsY[8] = {};
	INT16 gsCupidAnsH[8] = {};
	int   giCupidAnsCount = 0;
	bool  gfCupidAnswerRegionsLive = false;

	// the deck: real members interleaved with Speck's own material
	enum CardKind { CARD_MEMBER, CARD_AD_GOLD, CARD_AD_TESTIMONIAL,
			CARD_AD_NEWMEMBERS, CARD_END };
	struct Card
	{
		CardKind  kind;
		ProfileID pid; // CARD_MEMBER only
	};

	// who is looking at a member card changes what the card admits to
	enum CardView
	{
		CV_DECK,   // dealt on the deck: photo downloads, match maths shown
		CV_DETAIL, // re-dealt on the dossier page, photo already cached
		CV_SELF,   // the member's own card, exactly as others receive it
	};
	std::vector<Card> gCupidDeck;
	int gCupidDeckPos = 0;

	// who the member is browsing for; the era called this "seeking a".
	// Defaults to the opposite of your own sheet, cycles MEN/WOMEN/EVERYONE.
	enum CupidSeek { SEEK_MEN, SEEK_WOMEN, SEEK_EVERYONE };
	int giCupidSeek = SEEK_EVERYONE;
	INT32 gsCupidSeekHit[3][2] = {}; // radio hit spans, set by the render

	// the 1,000,000th-visitor popup: one per fresh day, closable only by
	// its own little X, as was the law of the era
	bool  gfCupidPopupUp = false;

	// the GOLD offer: opened from Speck's house banner, closed by NO
	// THANKS - it borrows the card stage without touching the deck
	bool  gfCupidGoldOffer = false;

	// the site's little dialog: one message, one optional purchase.
	// -1 = no notice; the CTA is 1 for the day pass, 0 for none
	int giCupidNotice = -1;
	int giCupidNoticeCta = 0;

	void CupidNotice(int str, int cta)
	{
		giCupidNotice = str;
		giCupidNoticeCta = cta;
	}

	// the card photo downloads over a 28.8k line: rows revealed so far,
	// when the next batch lands, and where the line chokes this time
	INT32  giCupidPhotoReveal = 0;
	UINT32 guiCupidRevealNext = 0;
	INT32  giCupidRevealChoke = 60;
	bool   gfCupidChokePending = false;

	// how the wire feels this time: sometimes the photo just arrives,
	// sometimes the modem has opinions
	bool gfCupidFastLine = false;

	void StartPhotoLoad()
	{
		giCupidPhotoReveal = 0;
		gfCupidFastLine = Random(10) < 3; // one line in three is a good one
		if (gfCupidFastLine)
		{
			guiCupidRevealNext = GetJA2Clock() + Random(250);
			gfCupidChokePending = false;
		}
		else
		{
			guiCupidRevealNext = GetJA2Clock() + 200 + Random(1100);
			giCupidRevealChoke = 20 + INT32(Random(80));
			gfCupidChokePending = Random(10) < 6; // most lines choke once
		}
	}

	// how far the dossier under the photo has been scrolled, and how far
	// it can go (measured by the last render)
	INT32 giCupidCardScroll = 0;
	INT32 giCupidCardScrollMax = 0x7FFF;
	int   giCupidCardTab = 0; // THE AD / MATCH / STARS

	// every page that deals a card starts it unscrolled
	void ResetCardScroll()
	{
		giCupidCardScroll = 0;
		giCupidCardScrollMax = 0x7FFF;
		giCupidCardTab = 0; // a fresh card opens on the ad
	}

	// the card on the table: a verdict button flies it off the stage.
	// No dragging - this is 1999, and pages have buttons.
	INT32 giCupidCardDx = 0;
	int   giCupidFlyDir = 0; // -1 flying left, +1 flying right, 0 still

	struct Member
	{
		ProfileID pid;
		bool      merc;    // M.E.R.C., unverified, oversharing
		bool      locked;  // not yet unlocked on the M.E.R.C. site
		bool      fresh;   // a later unlock: wears the NEW!! tag forever,
		                   // because nobody ever took those down
		DatingGame::Profile prof;
	};
	std::vector<Member> gCupidRoster;

	SGPVObject* guiCupidLogo  = nullptr;
	SGPVObject* guiCupidNight  = nullptr; // grainy night wallpaper
	SGPVObject* guiCupidTiles  = nullptr; // the parlour's own tile faces
	SGPVObject* guiCupidDragon = nullptr; // and its dragon, for watermarks
	std::vector<SGPVSurface*> gCupidChipSurf; // 16bpp bakes for chat chips
	SGPVSurface* guiCupidSelfChip = nullptr;
	SGPVObject* guiCupidPanels = nullptr; // riveted plates: rail + 3 card sizes
	SGPVObject* guiCupidIcons = nullptr;
	SGPVObject* guiCupidSelf  = nullptr;   // the member's own mugshot, 48x43
	SGPVObject* guiCupidSelfBig = nullptr; // and the full photo, for the splash
	SGPVObject* guiCupidBig   = nullptr;   // the dealt card's 106x122 photo
	ProfileID   gCupidBigPid  = 0xFF;
	SGPVObject* guiCupidFace  = nullptr;   // 65-face for the splash
	ProfileID   gCupidFacePid = 0xFF;
	std::vector<SGPVObject*> gCupidFaces33; // parallel to gCupidRoster

	MOUSE_REGION gCupidTabRegion[4];
	MOUSE_REGION gCupidLoungeRegion;    // wheel catcher over the channel log
	MOUSE_REGION gCupidLoungeSayRegion; // the client's one input
	MOUSE_REGION gCupidRoomRegion[4];   // the channel tabs
	MOUSE_REGION gCupidLoungeUpRegion;   // the rail's top arrow
	MOUSE_REGION gCupidLoungeDownRegion; // and its bottom one
	MOUSE_REGION gCupidLoungeFaceRegion; // the avatar column, clickable
	MOUSE_REGION gCupidRailBackRegion;   // leaves the borrowed rail card
	MOUSE_REGION gCupidEditRegion[2];    // ME: headline and summary pickers
	MOUSE_REGION gCupidCardTabRegion[3]; // the dossier's own tabs
	MOUSE_REGION gCupidSkipRegion;       // next ad >> - no verdict given
	MOUSE_REGION gCupidPrevRegion;       // << prev ad
	MOUSE_REGION gCupidAdCloseRegion;    // the [x] on an interstitial
	MOUSE_REGION gCupidAdCtaRegion;      // and its one golden promise
	MOUSE_REGION gCupidAdPlatRegion;     // the same promise, but prouder
	MOUSE_REGION gCupidMarryRegion;      // the ring: the third verdict
	MOUSE_REGION gCupidNoticeOkRegion;   // the little dialog's OK
	MOUSE_REGION gCupidNoticeCtaRegion;  // and its day-pass upsell
	MOUSE_REGION gCupidCardRegion;
	MOUSE_REGION gCupidPassRegion;
	MOUSE_REGION gCupidLikeRegion;
	MOUSE_REGION gCupidAnswerRegion[8];
	MOUSE_REGION gCupidActionRegion[2]; // ME landing / detail / splash slots
	MOUSE_REGION gCupidMatchRegion[7];  // clickable rows on the matches page
	MOUSE_REGION gCupidSideAdRegion[1]; // today's skyscraper, every page
	MOUSE_REGION gCupidScrollRegion;    // wheel catcher for the column cards
	MOUSE_REGION gCupidPopupXRegion;    // the popup's only honest exit
	MOUSE_REGION gCupidPopupCtaRegion;  // and its entire reason to exist
	MOUSE_REGION gCupidSeekRegion;      // "I am seeking", the ME page widget
	MOUSE_REGION gCupidSplashBtnRegion[2]; // the match popup's two choices
	MOUSE_REGION gCupidRailBtnRegion[3];   // detail/chat rail: back + two circles
	MOUSE_REGION gCupidChatLogRegion;      // wheel catcher over the private line
	MOUSE_REGION gCupidChatSayRegion;      // its input row
	MOUSE_REGION gCupidChatFaceRegion;     // rail photo: the full dossier
	MOUSE_REGION gCupidChatFlowerRegion;   // the florist link in the band
	MOUSE_REGION gCupidWebmasterRegion;    // the mailbox line opens ABOUT

	bool Hover(const MOUSE_REGION& r)
	{
		return (r.uiFlags & MSYS_MOUSE_IN_AREA) != 0;
	}

	// Every control belongs to one page; everything else is switched off so
	// an invisible region can never eat a click meant for the page on show.
	bool PlayerHasProfile(); // defined with the profile helpers below
	bool RoomLocked();       // defined with the lounge below

	void SyncRegions()
	{
		if (!gfCupidRegionsUp) return;
		const bool popup   = gfCupidPopupUp && gCupidPage == CPP_DECK;
		const bool deck    = gCupidPage == CPP_DECK && !popup;
		const bool actions = gCupidPage == CPP_ME && !gfCupidQuizLive;
		const bool matches = gCupidPage == CPP_MATCHES;

		auto set = [](MOUSE_REGION& r, bool on)
		{
			if (on) r.Enable(); else r.Disable();
		};
		const bool column = gCupidPage == CPP_DETAIL ||
				(gCupidPage == CPP_ME && !gfCupidQuizLive &&
				 PlayerHasProfile() && HaveStoredAnswers());
		set(gCupidCardRegion, deck);
		set(gCupidPassRegion, deck);
		set(gCupidLikeRegion, deck);
		set(gCupidMarryRegion, deck);
		set(gCupidNoticeOkRegion, deck && giCupidNotice >= 0);
		set(gCupidNoticeCtaRegion,
				deck && giCupidNotice >= 0 && giCupidNoticeCta == 1);
		set(gCupidScrollRegion, column);
		set(gCupidLoungeRegion, gCupidPage == CPP_LOUNGE);
		set(gCupidLoungeSayRegion, gCupidPage == CPP_LOUNGE);
		for (MOUSE_REGION& r : gCupidRoomRegion)
		{
			set(r, gCupidPage == CPP_LOUNGE);
		}
		set(gCupidLoungeUpRegion, gCupidPage == CPP_LOUNGE);
		set(gCupidLoungeDownRegion, gCupidPage == CPP_LOUNGE);
		set(gCupidLoungeFaceRegion,
				gCupidPage == CPP_LOUNGE && !RoomLocked());
		set(gCupidRailBackRegion,
				(gCupidPage == CPP_LOUNGE && gCupidRailWho >= 0) ||
				gCupidPage == CPP_CHAT);
		const bool editable = gCupidPage == CPP_ME && !gfCupidQuizLive &&
				PlayerHasProfile() && HaveStoredAnswers();
		for (MOUSE_REGION& r : gCupidEditRegion) set(r, editable);
		const bool cardTabs = (deck && PlayerHasProfile()) || column;
		for (MOUSE_REGION& r : gCupidCardTabRegion) set(r, cardTabs);
		set(gCupidSkipRegion, deck && PlayerHasProfile());
		set(gCupidPrevRegion, deck && PlayerHasProfile());
		set(gCupidAdCloseRegion, deck && PlayerHasProfile());
		set(gCupidAdCtaRegion, deck && PlayerHasProfile());
		set(gCupidAdPlatRegion, deck && PlayerHasProfile());
		// while a member's card borrows the rail, the menu is not there
		for (MOUSE_REGION& r : gCupidTabRegion)
		{
			set(r, !(gCupidPage == CPP_LOUNGE && gCupidRailWho >= 0) &&
					gCupidPage != CPP_CHAT);
		}
		for (MOUSE_REGION& r : gCupidSideAdRegion)
		{
			set(r, deck || gCupidPage == CPP_DETAIL ||
					gCupidPage == CPP_CHAT ||
					gCupidPage == CPP_ABOUT);
		}
		for (MOUSE_REGION& r : gCupidActionRegion) set(r, actions);
		for (MOUSE_REGION& r : gCupidMatchRegion)  set(r, matches);
		set(gCupidPopupXRegion, popup);
		set(gCupidPopupCtaRegion, popup);
		set(gCupidSeekRegion, gCupidPage == CPP_ME && !gfCupidQuizLive &&
				PlayerHasProfile() && HaveStoredAnswers());
		const bool splash = gCupidPage == CPP_SPLASH;
		for (MOUSE_REGION& r : gCupidSplashBtnRegion) set(r, splash);
		for (MOUSE_REGION& r : gCupidRailBtnRegion)
		{
			set(r, gCupidPage == CPP_DETAIL);
		}
		set(gCupidChatLogRegion, gCupidPage == CPP_CHAT);
		set(gCupidChatSayRegion, gCupidPage == CPP_CHAT);
		set(gCupidChatFaceRegion, gCupidPage == CPP_CHAT);
		set(gCupidChatFlowerRegion, gCupidPage == CPP_CHAT);
		set(gCupidWebmasterRegion, !splash &&
				gCupidPage != CPP_CHAT &&
				!(gCupidPage == CPP_LOUNGE && gCupidRailWho >= 0));
	}

	// --- text, EN/DE -------------------------------------------------------
	enum CupidStr
	{
		CPS_TAB_DECK, CPS_TAB_MATCHES, CPS_TAB_ME,
		CPS_LIKES_LEFT, CPS_LIKES_GOLD, CPS_OUT_OF_LIKES,
		CPS_STAMP_LIKE, CPS_STAMP_NOPE,
		CPS_STATUS_ONLINE, CPS_STATUS_CONTRACT, CPS_STATUS_PAYROLL,
		CPS_STATUS_GONE, CPS_STATUS_MARRIED,
		CPS_VERIFIED, CPS_UNVERIFIED,
		CPS_MATCH, CPS_MATCH_RANGE,
		CPS_SPLASH_TITLE, CPS_SPLASH_SUB, CPS_SPLASH_KEEP, CPS_SPLASH_VIEW,
		CPS_MATCHES_TITLE, CPS_MATCHES_NONE, CPS_MATCHES_HINT,
		CPS_LIKED_YOU, CPS_LIKED_YOU_GOLD,
		CPS_AD_GOLD_HEAD, CPS_AD_GOLD_BODY, CPS_AD_GOLD_BTN, CPS_AD_GOLD_OWNED,
		CPS_AD_TESTI_HEAD, CPS_AD_TESTI_BODY, CPS_AD_TESTI_BY,
		CPS_AD_NEW_HEAD, CPS_AD_NEW_BODY,
		CPS_END_HEAD, CPS_END_BODY,
		CPS_AD_HINT,
		CPS_ME_TITLE, CPS_ME_TITLE_MINE, CPS_ME_PREVIEW, CPS_ME_STATS,
		CPS_ME_IMPORT, CPS_ME_TAKE, CPS_ME_RETAKE, CPS_ME_UPGRADE,
		CPS_ME_COMPLETE, CPS_ME_PARTIAL, CPS_ME_HINT, CPS_ME_POWERED,
		CPS_QUIZ_SEX, CPS_QUIZ_MALE, CPS_QUIZ_FEMALE, CPS_QUIZ_PROGRESS,
		CPS_NO_PHOTO, CPS_NO_PROFILE_CARD,
		CPS_NOTICE_FIRST, CPS_LOOKING_FOR, CPS_SUMMARY,
		CPS_DEAL_BREAKERS, CPS_BLOCKED_BY,
		CPS_YOU_AGREED, CPS_YOU_DIFFER, CPS_AGREE_ON,
		CPS_SEND_FLOWERS, CPS_BACK,
		CPS_TICKER_DEFAULT, CPS_TICKER_NO_PROFILE, CPS_TICKER_NO_LIKES,
		CPS_TICKER_GOLD, CPS_TICKER_BROKE,
		CPS_TICKER_DEBT, CPS_TICKER_THANKS, CPS_TICKER_SUNDAY,
		CPS_TICKER_ATTRITION,
		CPS_POPUP_TITLE, CPS_POPUP_HEAD, CPS_POPUP_BODY, CPS_POPUP_CTA,
		CPS_FEATURED, CPS_LOVERS, CPS_LOVERS_WATCH,
		CPS_SEEK_LINE, CPS_SEEK_MEN, CPS_SEEK_WOMEN, CPS_SEEK_ALL,
		CPS_ACTIVE_24, CPS_ACTIVE_DAYS, CPS_ACTIVE_LONG,
		CPS_BLOCKED_WARN, CPS_UNDER_CONSTRUCTION, CPS_CONDOLENCE_ROW,
		CPS_TICKER_SAFETY, CPS_TICKER_CREDO, CPS_TICKER_VERDICT,
		CPS_TICKER_PLATINUM, CPS_TICKER_PROPOSE_OK,
		CPS_TICKER_PROPOSE_NEED_MATCH, CPS_TICKER_PROPOSE_ONE_A_DAY,
		CPS_TICKER_SWITCHED, CPS_NOTICE_OUT, CPS_TICKER_DAYPASS,
		CPS_NOTICE_WAIT_KISS, CPS_NOTICE_PROPOSED_WAIT,
		CPS_SPLASH_CHAT, CPS_NOTICE_CHAT_MATCH, CPS_CHAT_PRIVATE_SYS,
		CPS_COUNT
	};

	const char* const CUPID_TEXT[2][CPS_COUNT] =
	{
		{
			"BROWSE", "MATCHES", "ME",
			"{} kisses left today", "GOLD - kisses never run out",
			"OUT OF KISSES",
			"KISS", "PASS",
			"ONLINE NOW", "AWAY - ON CONTRACT", "ON YOUR PAYROLL",
			"LAST LOGIN: a long time ago", "MARRIED (a satisfied customer)",
			"A.I.M. VERIFIED", "no papers",
			"{}% MATCH", "{}-{}% MATCH",
			"MUTUAL INTEREST ALERT!!!", "{} blew a kiss back. The algorithm saw it coming.", "KEEP BROWSING", "VIEW PROFILE",
			"YOUR MATCHES", "No matches yet. The deck is waiting.",
			"Click a match to open a private line.",
			"{} members already blew you a KISS.", "They kissed. Now you know who.",
			"C.U.P.I.D. GOLD", "Unlimited kisses. See who kissed you "
			"FIRST. Prestige beyond measure. One payment of ${}, to me, "
			"Speck T. Kline.", "GET GOLD - ${}", "You are a GOLD member. "
			"Everything I promised is now true.",
			"\"He said he'd never settle down.\"", "\"We are MARRIED now. "
			"Thank you Mercs && Kisses!!!\"", "- Flo, satisfied member",
			"NEW MEMBERS COMING", "The roster grows as M.E.R.C. grows. "
			"Spend generously and love will follow. That is just science.",
			"THAT'S EVERYONE", "You have seen every professional in Arulco. "
			"I am in talks with several other war zones. - S.T.K.",
			"(no refunds. store credit, maybe.)",
			"THE QUESTIONNAIRE", "MY PROFILE",
			"HOW YOU APPEAR IN THE DECK",
			"visits: {}   -   kisses today: {}",
			"IMPORT MY I.M.P. PROFILE (free)",
			"TAKE THE QUESTIONNAIRE (free)", "RETAKE THE QUESTIONNAIRE (${})",
			"COMPLETE MY PROFILE (${})",
			"PROFILE: 100% COMPLETE", "PROFILE: 60% COMPLETE",
			"Members with complete profiles receive 3x more responses!!",
			"Any resemblance to the I.M.P. questionnaire is coincidental "
			"and, to date, unlitigated.",
			"First: the questionnaire needs to know. You are...",
			"MALE", "FEMALE", "QUESTION {} OF {}",
			"NO PHOTO", "Take the questionnaire to start browsing - Speck",
			"First thing people notice: {}", "Looking for: {}",
			"Self-summary:",
			"DEAL BREAKERS: {}", "Blocked by {} member(s)",
			"You both answered:", "You differ on:",
			"You agree on {} of {} shared answers.",
			"SEND FLOWERS", "< BACK",
			"where the tough get tender - a Speck T. Kline company",
			"No profile, no romance. The ME tab is right there. - Speck",
			"Out of kisses. GOLD never runs out. Just saying. - Speck",
			"Thank you for going GOLD. You complete me. - Speck",
			"Your card was declined. It happens. Not to me. - Speck",
			"Your M.E.R.C. account is overdue. Romance can wait. - Speck",
			"My greatest appreciation for your payment. Thank you. - Speck",
			"SUNDAY SPECIAL!! Double kisses all day, per science.",
			"Membership attrition remains within industry norms. - mgmt",
			"advertisement - mercsandkisses.com",
			"!!! CONGRATULATIONS !!!",
			"You are the 1,000,000th visitor to this page!! You have won: "
			"eligibility to purchase C.U.P.I.D. GOLD.",
			"CLAIM MY PRIZE",
			"FEATURED PROFILES",
			"LOVERS OF THE MONTH", "watch this space",
			"I am seeking: [ {} ] (click to change)",
			"MEN", "WOMEN", "EVERYONE",
			"Active during the last 24 hours",
			"Active {} days ago",
			"Active a long, long time ago",
			"has BLOCKED {}",
			"TESTIMONIALS: UNDER CONSTRUCTION",
			"in loving memory - profile retained",
			"SAFETY FIRST: never reveal your real sector. Speck will "
			"never ask for your grid coordinates. 'kill' is a figure of "
			"speech. C.U.P.I.D. brokers no contracts.",
			"C.U.P.I.D. - the Certified Union of Professionals In Dating "
			"- people worth dying for - a Speck T. Kline company - est. "
			"tuesday",
			"That verdict is final. No refunds. - Speck",
			"PLATINUM acquired. It is GOLD, but you paid more. "
			"I respect that. - Speck",
			"A proposal!! Flowers are customary. Conveniently sold "
			"nearby. - Speck",
			"Proposals are for matches. Kiss first - if they kiss back, you are a match. - Speck",
			"One proposal a day. Compose yourself. - Speck",
			"New verdict recorded. The kiss is not refunded. - Speck",
			"Out of kisses for today. A DAY PASS refills them for $5. "
			"GOLD, of course, never runs out.",
			"DAY PASS active. Kiss responsibly. - Speck",
			"Your kiss is out there. Until they kiss back, there is no "
			"match to propose to. Patience. Or flowers. - Speck",
			"The proposal is filed, kiss included at no extra charge. "
			"They are thinking it over. This is normal. - Speck",
			"SAY HI!!",
			"Private lines are for matches. Kiss first - if they kiss "
			"back, the line opens. - Speck",
			"this is a private line. C.U.P.I.D. reads it only for "
			"quality assurance.",
		},
		{
			"STOEBERN", "MATCHES", "ICH",
			"noch {} Kuesse heute", "GOLD - Kuesse gehen nie aus",
			"KEINE KUESSE MEHR",
			"KUSS", "PASS",
			"JETZT ONLINE", "ABWESEND - IM EINSATZ", "AUF IHRER GEHALTSLISTE",
			"LETZTER LOGIN: vor langer Zeit", "VERHEIRATET (zufriedene "
			"Kundin)",
			"A.I.M.-GEPRUEFT", "ungeprueft",
			"{}% PASSUNG", "{}-{}% PASSUNG",
			"BEIDERSEITIGES INTERESSE!!!", "{} haucht einen Kuss zurueck. "
			"Der Algorithmus wusste es vorher.", "WEITER STOEBERN",
			"PROFIL ANSEHEN",
			"IHRE MATCHES", "Noch keine Matches. Das Deck wartet.",
			"Klicken Sie ein Match fuer eine private Leitung.",
			"{} Mitglieder haben Ihnen bereits einen KUSS zugeworfen.",
			"Sie kuessten. Jetzt wissen Sie, wer.",
			"C.U.P.I.D. GOLD", "Unbegrenzte Kuesse. Sehen Sie ZUERST, "
			"wer Sie kuesst. Unermessliches Prestige. Eine Zahlung von "
			"{} $, an mich, Speck T. Kline.", "GOLD HOLEN - {} $",
			"Sie sind GOLD-Mitglied. Alles, was ich versprach, ist jetzt "
			"wahr.",
			"\"Er wollte sich nie binden.\"", "\"Wir sind jetzt VERHEIRATET. "
			"Danke Mercs && Kisses!!!\"", "- Flo, zufriedenes Mitglied",
			"NEUE MITGLIEDER KOMMEN", "Die Liste waechst mit M.E.R.C. Geben "
			"Sie grosszuegig aus, die Liebe folgt. Das ist Wissenschaft.",
			"DAS WAREN ALLE", "Sie haben jeden Profi in Arulco gesehen. Ich "
			"verhandle mit weiteren Kriegsgebieten. - S.T.K.",
			"(keine Rueckerstattung. Gutschein, vielleicht.)",
			"DER FRAGEBOGEN", "MEIN PROFIL",
			"SO ERSCHEINEN SIE IM DECK",
			"Besuche: {}   -   Kuesse heute: {}",
			"MEIN I.M.P.-PROFIL IMPORTIEREN (gratis)",
			"FRAGEBOGEN AUSFUELLEN (gratis)", "FRAGEBOGEN WIEDERHOLEN ({} $)",
			"PROFIL VERVOLLSTAENDIGEN ({} $)",
			"PROFIL: 100% VOLLSTAENDIG",
			"PROFIL: 60% VOLLSTAENDIG",
			"Mitglieder mit vollstaendigem Profil erhalten 3x mehr "
			"Antworten!!",
			"Aehnlichkeiten mit dem I.M.P.-Fragebogen sind zufaellig und "
			"bislang unverklagt.",
			"Zuerst: der Fragebogen muss es wissen. Sie sind...",
			"MANN", "FRAU", "FRAGE {} VON {}",
			"KEIN FOTO", "Ohne Fragebogen kein Stoebern - Speck",
			"Was zuerst auffaellt: {}", "Sucht: {}",
			"Selbstbeschreibung:",
			"AUSSCHLUSSKRITERIEN: {}", "Von {} Mitglied(ern) blockiert",
			"Sie antworteten beide:", "Sie unterscheiden sich bei:",
			"{} von {} gemeinsamen Antworten gleich.",
			"BLUMEN SENDEN", "< ZURUECK",
			"wo die Harten zaertlich werden - eine Speck T. Kline Firma",
			"Kein Profil, keine Romantik. Der ICH-Tab wartet. - Speck",
			"Keine Kuesse mehr. GOLD geht nie aus. Nur so. - Speck",
			"Danke fuer GOLD. Sie vervollstaendigen mich. - Speck",
			"Karte abgelehnt. Passiert jedem. Mir nicht. - Speck",
			"Ihr M.E.R.C.-Konto ist ueberfaellig. Amor wartet. - Speck",
			"Meine groesste Wertschaetzung fuer Ihre Zahlung. - Speck",
			"SONNTAGS-SPEZIAL!! Doppelte Kuesse, laut Wissenschaft.",
			"Mitgliederschwund im Branchenrahmen. - Verwaltung",
			"werbung - mercsandkisses.com",
			"!!! HERZLICHEN GLUECKWUNSCH !!!",
			"Sie sind der 1.000.000ste Besucher dieser Seite!! Sie haben "
			"gewonnen: die Berechtigung, C.U.P.I.D. GOLD zu kaufen.",
			"PREIS ABHOLEN",
			"AUSGEWAEHLTE PROFILE",
			"LIEBESPAAR DES MONATS", "demnaechst hier",
			"Ich suche: [ {} ] (klicken zum Aendern)",
			"MAENNER", "FRAUEN", "ALLE",
			"Aktiv in den letzten 24 Stunden",
			"Aktiv vor {} Tagen",
			"Aktiv vor sehr, sehr langer Zeit",
			"hat {} BLOCKIERT",
			"REFERENZEN: IM AUFBAU",
			"in liebevoller Erinnerung - Profil bleibt bestehen",
			"SICHERHEIT ZUERST: nennen Sie nie Ihren echten Sektor. Speck "
			"fragt nie nach Ihren Gitterkoordinaten. 'kill' ist eine "
			"Redewendung. C.U.P.I.D. vermittelt keine Auftraege.",
			"C.U.P.I.D. - der zertifizierte bund professioneller im dating "
			"- menschen, fuer die sich das sterben lohnt - eine Speck T. "
			"Kline Firma - gegr. dienstag",
			"Das Urteil ist endgueltig. Keine Rueckerstattung. - Speck",
			"PLATIN erworben. Es ist GOLD, aber Sie zahlten mehr. "
			"Respekt. - Speck",
			"Ein Antrag!! Blumen sind ueblich. Gibt es praktischerweise "
			"nebenan. - Speck",
			"Antraege sind fuer Matches. Erst kuessen - kuesst man zurueck, sind Sie ein Match. - Speck",
			"Ein Antrag pro Tag. Fassen Sie sich. - Speck",
			"Neues Urteil vermerkt. Der Kuss wird nicht erstattet. - Speck",
			"Keine Kuesse mehr fuer heute. Ein TAGESPASS fuellt sie fuer "
			"5 $ auf. GOLD geht natuerlich nie aus.",
			"TAGESPASS aktiv. Kuessen Sie verantwortungsvoll. - Speck",
			"Ihr Kuss ist unterwegs. Bis zurueckgekuesst wird, gibt es "
			"kein Match fuer einen Antrag. Geduld. Oder Blumen. - Speck",
			"Der Antrag ist eingereicht, Kuss inklusive. Es wird noch "
			"nachgedacht. Das ist normal. - Speck",
			"SAG HALLO!!",
			"Private Leitungen sind fuer Matches. Erst kuessen - wird "
			"zurueckgekuesst, oeffnet sich die Leitung. - Speck",
			"dies ist eine private leitung. C.U.P.I.D. liest nur zur "
			"qualitaetssicherung mit.",
		},
	};

	const char* T(CupidStr id) { return CUPID_TEXT[gfCupidGerman ? 1 : 0][id]; }

	// what Speck's footer ticker currently says
	int giCupidTicker = CPS_TICKER_DEFAULT;

	// the trait, spun charming, indexed by PersonalityTrait
	const char* const CUPID_TRAIT_SPIN[2][8] =
	{
		{ "pro", "cool", "sensitive", "outdoorsy", "landlubber",
		  "thorough", "carefree", "impulsive!!" },
		{ "profi", "cool", "einfuehlsam", "outdoor-fan", "landratte",
		  "gruendlich", "sorglos", "spontan!!" },
	};

	// every personals ad of the era ran under a headline; these come with
	// the temperament, indexed by Attitudes
	// three headlines per temperament: which one a member runs is theirs
	// alone, so no two neighbours in the deck read the same
	const char* const CUPID_HEADLINE[2][NUM_ATTITUDES][3] =
	{
		{
		  { "\"Steady Hands, Steady Heart\"",
		    "\"Reliable. Ask Anyone Still Around.\"",
		    "\"Boring In The Good Way\"" },
		  { "\"Your New Best Friend (And Then Some?)\"",
		    "\"Free Hugs, Some Conditions Apply\"",
		    "\"I Already Like You\"" },
		  { "\"Not Looking. And Yet Here I Am.\"",
		    "\"Leave A Message After The Silence\"",
		    "\"Alone. On Purpose. Mostly.\"" },
		  { "\"The One Could Be Reading This RIGHT NOW!!\"",
		    "\"Today Feels Lucky!!\"",
		    "\"Everything Happens For A Reason :)\"" },
		  { "\"This Will Probably Not Work Out\"",
		    "\"Lower Your Expectations, Then Write\"",
		    "\"Prepared For The Worst. You?\"" },
		  { "\"No Games. Unless You Start One.\"",
		    "\"First Move? Already Made It.\"",
		    "\"Catch Me If You Can Keep Up\"" },
		  { "\"You Have Excellent Taste Already\"",
		    "\"Congratulations On Finding Me\"",
		    "\"The Standard Others Are Held To\"" },
		  { "\"You May Have Heard Of Me\"",
		    "\"As Seen In Several Contracts\"",
		    "\"Success Seeks Same\"" },
		  { "\"Frankly, You Could Do Worse\"",
		    "\"I'm The Honest One Here\"",
		    "\"Warning Label Included Free\"" },
		  { "\"Seeking Somewhere Quiet, Together\"",
		    "\"Gentle Souls Only, Please Knock\"",
		    "\"Slow To Warm. Worth The Wait?\"" },
		},
		{
		  { "\"Ruhige Haende, ruhiges Herz\"",
		    "\"Zuverlaessig. Fragen Sie die Ueberlebenden.\"",
		    "\"Langweilig auf die gute Art\"" },
		  { "\"Ihr neuer bester Freund (und mehr?)\"",
		    "\"Gratis Umarmungen, kleine Bedingungen\"",
		    "\"Ich mag Sie jetzt schon\"" },
		  { "\"Suche nicht. Und doch bin ich hier.\"",
		    "\"Nachricht nach der Stille hinterlassen\"",
		    "\"Allein. Absichtlich. Meistens.\"" },
		  { "\"Vielleicht liest DER RICHTIGE genau JETZT!!\"",
		    "\"Heute fuehlt sich gluecklich an!!\"",
		    "\"Alles passiert aus einem Grund :)\"" },
		  { "\"Das wird vermutlich nichts\"",
		    "\"Erwartungen senken, dann schreiben\"",
		    "\"Aufs Schlimmste vorbereitet. Sie?\"" },
		  { "\"Keine Spielchen. Ausser Sie fangen an.\"",
		    "\"Erster Schritt? Schon gemacht.\"",
		    "\"Fang mich, wenn du mithaeltst\"" },
		  { "\"Sie haben bereits exzellenten Geschmack\"",
		    "\"Glueckwunsch, Sie haben mich gefunden\"",
		    "\"Der Massstab fuer die anderen\"" },
		  { "\"Sie haben sicher von mir gehoert\"",
		    "\"Bekannt aus mehreren Vertraegen\"",
		    "\"Erfolg sucht seinesgleichen\"" },
		  { "\"Ehrlich: es gibt Schlimmeres\"",
		    "\"Ich bin hier der Ehrliche\"",
		    "\"Warnhinweis gratis dazu\"" },
		  { "\"Suche einen ruhigen Ort, zu zweit\"",
		    "\"Sanfte Seelen bitte anklopfen\"",
		    "\"Braucht Anlauf. Das Warten lohnt?\"" },
		},
	};

	const char* HeadlineFor(int att, ProfileID pid)
	{
		return CUPID_HEADLINE[gfCupidGerman ? 1 : 0][att][pid % 3];
	}

	// what each attitude is "looking for", indexed by Attitudes
	const char* const CUPID_LOOKING[2][NUM_ATTITUDES][2] =
	{
		{
		  { "ISO LTR-ish. short contract first, refs avail.",
		    "ISO the quiet kind of steady. coffee first" },
		  { "ISO new friends!! poss. more. D&&D-free",
		    "ISO pen pals, then more. stamps provided" },
		  { "ISO quiet type. long silences a plus. write short",
		    "ISO one (1) person. that is the maximum" },
		  { "ISO The One :) no time-wasters (ok, some)",
		    "ISO co-author for a great story :)" },
		  { "ISO whatever. expect little, bring bandages",
		    "ISO someone to be wrong with" },
		  { "ISO sparring partner w/ LTR potential. flinchers need not apply",
		    "ISO equal. bring your best, keep your teeth" },
		  { "ISO admirer who can keep up. most cant",
		    "ISO an audience of one, standing ovation" },
		  { "ISO plus-one for VIP lounges. NDA on request",
		    "ISO arm candy w/ security clearance" },
		  { "ISO none of your business. no photo, no reply",
		    "ISO thick skin. compliments extra" },
		  { "ISO patient soul, someplace safe. sudden moves discouraged",
		    "ISO someone brave enough for both of us" },
		},
		{
		  { "su. Bekanntschaft. erst Kurzvertrag, evtl. mehr. NR bevorzugt",
		    "su. das ruhige bestaendige. erst Kaffee" },
		  { "su. neue Freunde!! evtl. mehr. f. gem. Unternehmungen",
		    "su. Brieffreunde, dann mehr. Marken vorhanden" },
		  { "su. ruhigen Typ. lange Pausen ein Plus. kurz schreiben",
		    "su. eine (1) Person. das ist das Maximum" },
		  { "su. DIE grosse Liebe :) Bildzuschrift erwuenscht",
		    "su. Co-Autor fuer eine grosse Geschichte :)" },
		  { "su. was auch immer. wenig erwarten, Verbandszeug mitbringen",
		    "su. jemanden zum Gemeinsam-Danebenliegen" },
		  { "su. Sparringspartner m. Zukunft. wer zuckt, faellt raus",
		    "su. Ebenbuertige. alles geben, Zaehne behalten" },
		  { "su. Bewunderer, der mithaelt. die wenigsten koennen",
		    "su. Publikum von eins, stehende Ovationen" },
		  { "su. Begleitung f. VIP-Lounges. Diskretion Ehrensache",
		    "su. Begleitung m. Sicherheitsfreigabe" },
		  { "su. geht Sie nichts an. ohne Bild keine Antwort",
		    "su. dickes Fell. Komplimente kosten extra" },
		  { "su. geduldige Seele, sicherer Ort. keine schnellen Bewegungen",
		    "su. jemanden, der fuer uns beide mutig ist" },
		},
	};

	const char* LookingFor(int att, ProfileID pid)
	{
		return CUPID_LOOKING[gfCupidGerman ? 1 : 0][att][pid % 2];
	}

	// A.I.M. members answer in four words; M.E.R.C. members overshare
	const char* const CUPID_SUMMARY_AIM[2][NUM_ATTITUDES] =
	{
		{ "I work. I sleep.", "Ask me anything :)", "No.",
		  "Every day, a gift.", "Read the file.", "Try to keep up.",
		  "You already know me.", "You have seen my work.",
		  "Next question.", "Prefer quiet postings." },
		{ "Arbeit. Schlaf. Fertig.", "Fragen Sie ruhig :)", "Nein.",
		  "Jeder Tag ein Geschenk.", "Lesen Sie die Akte.",
		  "Halten Sie mit.", "Sie kennen mich schon.",
		  "Sie kennen meine Arbeit.", "Naechste Frage.",
		  "Bevorzuge ruhige Posten." },
	};

	const char* const CUPID_SUMMARY_MERC[2][NUM_ATTITUDES] =
	{
		{ "Looking to connect with someone real!! I have SO much to give "
		  "and my last three references will confirm it, please do not "
		  "contact the fourth.",
		  "Hi!!! I love meeting people, animals, weather, everything "
		  "really!! Message me!!! I always reply, usually twice!!",
		  "I am not on here by choice, my colleague made the profile. But "
		  "since you are reading it anyway, hello, I suppose.",
		  "Life is AMAZING and it keeps getting better!! Also I am between "
		  "postings right now which is honestly a blessing!!",
		  "My last relationship ended and so will this one probably, but "
		  "the site had a free trial so here we are.",
		  "I like intensity. In everything. If that scares you we already "
		  "know it is not a match, and whose fault is that.",
		  "Frankly the algorithm undersells me. Whatever number you are "
		  "seeing, add ten. Then message me and thank me later.",
		  "You have probably heard of me. If not, ask around. I will wait "
		  "right here, being extremely notable.",
		  "I read the terms of service so believe me when I say nothing on "
		  "this site is my fault, including this paragraph.",
		  "Just looking for somebody nice who lives somewhere with very "
		  "thick walls and no history of incidents." },
		{ "Suche eine ECHTE Verbindung!! Ich habe SO viel zu geben, meine "
		  "letzten drei Referenzen bestaetigen das, die vierte bitte nicht "
		  "kontaktieren.",
		  "Hallo!!! Ich liebe Menschen, Tiere, Wetter, eigentlich alles!! "
		  "Schreiben Sie mir!!! Ich antworte immer, meistens zweimal!!",
		  "Ich bin nicht freiwillig hier, ein Kollege hat das Profil "
		  "erstellt. Aber da Sie schon lesen: hallo, schaetze ich.",
		  "Das Leben ist GROSSARTIG und wird immer besser!! Ich bin gerade "
		  "zwischen zwei Einsaetzen, was ehrlich gesagt ein Segen ist!!",
		  "Meine letzte Beziehung ging zu Ende und diese vermutlich auch, "
		  "aber die Probezeit war gratis, also bitte.",
		  "Ich mag Intensitaet. In allem. Wenn Sie das schreckt, wissen wir "
		  "beide, dass es nicht passt, und wessen Schuld ist das.",
		  "Offen gesagt unterschaetzt mich der Algorithmus. Was immer da "
		  "steht: zehn dazu. Dann schreiben Sie mir und danken mir spaeter.",
		  "Sie haben sicher von mir gehoert. Falls nicht, fragen Sie herum. "
		  "Ich warte hier und bin bemerkenswert.",
		  "Ich habe die AGB gelesen, glauben Sie mir also: nichts auf "
		  "dieser Seite ist meine Schuld, auch dieser Absatz nicht.",
		  "Suche einfach jemand Nettes mit sehr dicken Waenden und ohne "
		  "Vorgeschichte von Zwischenfaellen." },
	};

	// Soldier_Profile.h names only part of the roster; these two are pinned
	// against mercs-profile-info.json
	constexpr ProfileID BUZZ     = 23;
	constexpr ProfileID MELTDOWN = 39;

	// hand-tuned interests for the members whose canon demands it
	struct FlavorLine { ProfileID pid; const char* en; const char* de; };
	const FlavorLine CUPID_FLAVOR[] =
	{
		{ GRUNTY,  "Interests: chess. chach.com. You have maybe seen it. "
		           "Ze coach on ze site is not based on anyone.",
		           "Interessen: Schach. chach.com. Vielleicht kennen Sie es. "
		           "Ze Trainerin dort basiert auf niemandem." },
		{ BUNS,    "Interests: marksmanship, grammar, punctuality. I have "
		           "never visited a chess website and will not start.",
		           "Interessen: Schiessen, Grammatik, Puenktlichkeit. Ich "
		           "besuche keine Schach-Webseiten und fange nicht an." },
		{ FOX,     "Interests: medicine, cosmetics, and knowing exactly what "
		           "I am doing. Some of you already know. Ask Wolf. Or don't.",
		           "Interessen: Medizin, Kosmetik, und genau wissen, was ich "
		           "tue. Manche wissen es schon. Fragen Sie Wolf. Oder nicht." },
		{ BUZZ,    "Interests: none of them are Lynx.",
		           "Interessen: keines davon ist Lynx." },
		{ LYNX,    "Interests: the outdoors, fine rifles, moving forward in "
		           "life. Some people should try it.",
		           "Interessen: die Natur, gute Gewehre, im Leben nach vorn "
		           "schauen. Sollten manche auch mal versuchen." },
		{ MELTDOWN,"Interests: my own business, which you should mind. The "
		           "testimonial in the deck is a lie by the way.",
		           "Interessen: meine Angelegenheiten, kuemmern Sie sich um "
		           "Ihre. Das Zitat im Deck ist uebrigens gelogen." },
		{ FLO,     "Interests: my HUSBAND Daryl!!! Thank you Mercs && Kisses "
		           "for everything!!! (profile kept for the memories)",
		           "Interessen: mein EHEMANN Daryl!!! Danke Mercs && Kisses "
		           "fuer alles!!! (Profil bleibt der Erinnerung wegen)" },
		{ VICKY,   "Interests: engines, reggae, patience. Yes, this is really "
		           "my photo. Yes, I have seen the visitor numbers.",
		           "Interessen: Motoren, Reggae, Geduld. Ja, das Foto ist "
		           "echt. Ja, ich habe die Besucherzahlen gesehen." },
		{ IVAN,    "Interests: work.",
		           "Interessen: Arbeit." },
		{ DR_Q,    "Interests: balance in all things. The eye sees what the "
		           "heart brings. I swipe in moderation.",
		           "Interessen: Gleichgewicht in allem. Das Auge sieht, was "
		           "das Herz mitbringt. Ich wische mit Mass." },
	};

	// --- drawing kit --------------------------------------------------------
	void FillRect(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb)
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, CP_X(x), CP_Y(y),
					CP_X(x + w), CP_Y(y + h), Get16BPPColor(rgb));
	}

	int CornerInset(int row, int radius)
	{
		const double dy = radius - row - 0.5;
		return int(radius - std::sqrt(double(radius) * radius - dy * dy) + 0.5);
	}

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

	void FillRounded(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb, int radius,
				UINT32 bg)
	{
		FillRect(x, y, w, h, rgb);
		RoundCorners(x, y, w, h, radius, bg);
	}

	// shift a FROMRGB triple, clamped - the chisel bevel derives its lit
	// and shaded edge from whatever the plate is made of
	UINT32 Lighten(UINT32 rgb, int d)
	{
		const int r = std::min(255, int(rgb & 0xFF) + d);
		const int g = std::min(255, int((rgb >> 8) & 0xFF) + d);
		const int b = std::min(255, int((rgb >> 16) & 0xFF) + d);
		return FROMRGB(r, g, b);
	}

	UINT32 Darken(UINT32 rgb, int d)
	{
		const int r = std::max(0, int(rgb & 0xFF) - d);
		const int g = std::max(0, int((rgb >> 8) & 0xFF) - d);
		const int b = std::max(0, int((rgb >> 16) & 0xFF) - d);
		return FROMRGB(r, g, b);
	}

	// a 3x3 stud with one glint pixel - panels are bolted on, not floated
	void Rivet(INT32 x, INT32 y)
	{
		FillRect(x, y, 3, 3, CP_RGB_INK);
		FillRect(x, y, 1, 1, FROMRGB(170, 150, 180));
	}

	// a sparse fleck pass, deterministic, so flat inset slabs pick up
	// the same grain the baked plates carry
	void Stipple(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 lite,
			UINT32 dark)
	{
		uint32_t s = uint32_t(x * 73 + y * 149 + w * 31);
		for (INT32 yy = 2; yy < h - 2; yy += 3)
		{
			INT32 xx = 2;
			for (;;)
			{
				s = s * 1103515245u + 12345u;
				xx += 6 + INT32((s >> 16) % 13);
				if (xx >= w - 3) break;
				FillRect(x + xx, y + yy, 1, 1,
						(s & 0x400) ? lite : dark);
			}
		}
	}

	// a plate with an outline and a hard chisel bevel: lit top and left,
	// shaded foot and right, riveted when it is big enough to be structure
	void FillCard(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb, UINT32 edge,
				UINT32 bg)
	{
		FillRounded(x, y, w, h, edge, 3, bg);
		FillRounded(x + 1, y + 1, w - 2, h - 2, rgb, 3, edge);
		FillRect(x + 3, y + 1, w - 6, 1, Lighten(rgb, 34));
		FillRect(x + 1, y + 3, 1, h - 6, Lighten(rgb, 22));
		FillRect(x + 3, y + h - 2, w - 6, 1, Darken(rgb, 26));
		FillRect(x + w - 2, y + 3, 1, h - 6, Darken(rgb, 18));
		if (w >= 100 && h >= 100)
		{
			Rivet(x + 4, y + 4);
			Rivet(x + w - 7, y + 4);
			Rivet(x + 4, y + h - 7);
			Rivet(x + w - 7, y + h - 7);
		}
	}

	// a true disc, scanline by scanline - the only honest circle in 16bpp
	void DrawDisc(INT32 cx, INT32 cy, INT32 r, UINT32 rgb)
	{
		for (INT32 dy = -r; dy <= r; ++dy)
		{
			const double span = std::sqrt(double(r) * r - double(dy) * dy);
			const INT32 hw = INT32(span + 0.5);
			if (hw > 0) FillRect(cx - hw, cy + dy, hw * 2, 1, rgb);
		}
	}

	// the verdict disc: painted steel - shadow, ring, flat face, and one
	// thin lit arc where the shop light catches the rim
	void GelCircle(INT32 x, INT32 y, INT32 d, UINT32 base, UINT32 lite,
				UINT32 ring, UINT32 bg)
	{
		(void)bg;
		const INT32 r  = d / 2;
		const INT32 cx = x + r;
		const INT32 cy = y + r;
		DrawDisc(cx + 2, cy + 2, r, CP_RGB_SHADOW);
		DrawDisc(cx, cy, r, ring);
		DrawDisc(cx, cy, r - 2, base);
		FillRect(cx - r / 3, y + 3, (2 * r) / 3, 1, lite);
		FillRect(cx - r / 4, y + d - 4, r / 2, 1, Darken(base, 22));
	}

	// the press-plate button: flat face, hard chisel bevel, outline ring.
	// The aqua gel went back to 2001 where it came from.
	void GelPill(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 base, UINT32 lite,
				UINT32 ring, UINT32 bg)
	{
		FillRounded(x, y, w, h, ring, 3, bg);
		FillRounded(x + 1, y + 1, w - 2, h - 2, base, 3, ring);
		FillRect(x + 3, y + 1, w - 6, 1, lite);
		FillRect(x + 1, y + 3, 1, h - 6, lite);
		FillRect(x + 3, y + h - 2, w - 6, 1, Darken(base, 30));
		FillRect(x + w - 2, y + 3, 1, h - 6, Darken(base, 20));
	}

	void PrintAt(SGPFont font, UINT8 colour, INT32 x, INT32 y,
				const ST::string& text)
	{
		SetFontAttributes(font, colour, FONT_MCOLOR_BLACK, 0);
		MPrint(CP_X(x), CP_Y(y), text);
	}

	void PrintCentred(SGPFont font, UINT8 colour, INT32 cx, INT32 y,
				const ST::string& text)
	{
		PrintAt(font, colour, cx - StringPixLength(text, font) / 2, y, text);
	}

	void CupidRedraw()
	{
		SyncRegions();
		fPausedReDrawScreenFlag = TRUE;
	}

	// lettering pressed into the plate: dark strike one pixel low, the
	// face colour on top - the dark-theme cousin of the funeral home's
	// engraved marble
	void PrintStamped(SGPFont font, UINT8 colour, INT32 cx, INT32 y,
				const ST::string& text)
	{
		PrintCentred(font, FONT_NEARBLACK, cx, y + 1, text);
		PrintCentred(font, colour, cx, y, text);
	}

	// a 5x5 heart, small enough to punctuate a wordmark
	void DrawHeartDot(INT32 x, INT32 y, UINT32 rgb)
	{
		FillRect(x,     y,     2, 2, rgb);
		FillRect(x + 3, y,     2, 2, rgb);
		FillRect(x,     y + 1, 5, 2, rgb);
		FillRect(x + 1, y + 3, 3, 1, rgb);
		FillRect(x + 2, y + 4, 1, 1, rgb);
	}

	// a blocky heart at 1x (7x6) or scaled up
	void DrawHeart(INT32 x, INT32 y, int s, UINT32 rgb)
	{
		FillRect(x,         y,         3 * s, 2 * s, rgb);
		FillRect(x + 4 * s, y,         3 * s, 2 * s, rgb);
		FillRect(x,         y + 2 * s, 7 * s, 2 * s, rgb);
		FillRect(x + 1 * s, y + 4 * s, 5 * s, 1 * s, rgb);
		FillRect(x + 2 * s, y + 5 * s, 3 * s, 1 * s, rgb);
	}

	// a solid pager triangle: full column at the base, a point at the tip
	void DrawTri(INT32 x, INT32 y, INT32 h, bool right, UINT32 rgb)
	{
		const INT32 w = h / 2 + 1;
		for (INT32 c = 0; c < w; ++c)
		{
			const INT32 col = right ? x + c : x + w - 1 - c;
			FillRect(col, y + c, 1, h - 2 * c, rgb);
		}
	}

	// a 15px skull for the KILL verdict: cranium, sockets, teeth
	void DrawSkull(INT32 x, INT32 y, UINT32 ink, UINT32 base)
	{
		FillRounded(x, y, 15, 11, ink, 4, base);
		FillRect(x + 4, y + 11, 7, 4, ink);
		FillRect(x + 3, y + 4, 3, 3, base);  // sockets
		FillRect(x + 9, y + 4, 3, 3, base);
		FillRect(x + 7, y + 8, 1, 2, base);  // nose
		FillRect(x + 6, y + 12, 1, 3, base); // tooth gaps
		FillRect(x + 8, y + 12, 1, 3, base);
	}

	// a ring with a stone, for the proposal that costs real money
	void DrawRing(INT32 x, INT32 y, UINT32 gold, UINT32 base, UINT32 glint)
	{
		DrawDisc(x + 8, y + 10, 7, gold);
		DrawDisc(x + 8, y + 10, 4, base);
		FillRect(x + 7, y - 1, 3, 1, glint); // the stone, stepped
		FillRect(x + 6, y,     7, 2, glint);
		FillRect(x + 7, y + 2, 5, 1, glint);
		FillRect(x + 8, y + 3, 3, 1, glint);
	}

	// the astrological shorthand the whole trade ran on: a circle with an
	// arrow for the gentlemen, a cross for the ladies, both for everyone
	void DrawGender(INT32 x, INT32 y, bool arrow, bool cross, UINT32 rgb,
				UINT32 base)
	{
		DrawDisc(x + 8, y + 8, 5, rgb);
		DrawDisc(x + 8, y + 8, 3, base);
		if (arrow)
		{
			for (int i = 0; i < 4; ++i)
			{
				FillRect(x + 11 + i, y + 5 - i, 2, 2, rgb);
			}
			FillRect(x + 13, y, 4, 2, rgb);
			FillRect(x + 15, y, 2, 4, rgb);
		}
		if (cross)
		{
			FillRect(x + 7, y + 13, 2, 6, rgb);
			FillRect(x + 4, y + 15, 8, 2, rgb);
		}
	}

	// a speech bubble with three thinking dots and a tail
	void DrawBubble(INT32 x, INT32 y, UINT32 rgb, UINT32 base)
	{
		FillRounded(x, y, 17, 12, rgb, 3, base);
		FillRect(x + 4, y + 12, 4, 2, rgb);
		FillRect(x + 4, y + 14, 2, 1, rgb);
		FillRect(x + 4, y + 5, 2, 2, base);
		FillRect(x + 8, y + 5, 2, 2, base);
		FillRect(x + 12, y + 5, 2, 2, base);
	}

	// a shop flower: three petals, a core, a stem with one leaf
	void DrawFlower(INT32 x, INT32 y, UINT32 petal, UINT32 core, UINT32 stem)
	{
		DrawDisc(x + 8, y + 4, 3, petal);
		DrawDisc(x + 4, y + 8, 3, petal);
		DrawDisc(x + 12, y + 8, 3, petal);
		DrawDisc(x + 8, y + 8, 2, core);
		FillRect(x + 7, y + 12, 2, 8, stem);
		FillRect(x + 3, y + 15, 4, 2, stem);
	}

	// a miniature member card: photo square and two lines of dossier
	void DrawDossier(INT32 x, INT32 y, UINT32 rgb, UINT32 base)
	{
		FillRounded(x, y + 2, 17, 13, rgb, 2, base);
		FillRect(x + 2, y + 4, 5, 6, base);
		FillRect(x + 9, y + 5, 6, 1, base);
		FillRect(x + 9, y + 8, 6, 1, base);
	}

	// a chunky X for the pass button
	void DrawCross(INT32 x, INT32 y, int size, int thick, UINT32 rgb)
	{
		for (int i = 0; i < size; ++i)
		{
			FillRect(x + i, y + i, thick, thick, rgb);
			FillRect(x + size - 1 - i, y + i, thick, thick, rgb);
		}
	}

	// wallpaper: the baked night-sky sheet - grain, stars, hearts adrift.
	// If the STI is missing, the old procedural starfield fills in.
	void RenderWallpaper()
	{
		if (guiCupidNight)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidNight, 0, CP_X(0), CP_Y(0));
			return;
		}
		FillRect(0, 0, CP_PAGE_W, CP_PAGE_H, CP_RGB_BG);
		uint32_t seed = 77;
		for (int i = 0; i < 90; ++i)
		{
			seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
			const INT32 x = 4 + INT32(seed % 494);
			const INT32 y = 4 + INT32((seed >> 10) % 392);
			if (i % 9 == 0)
			{
				DrawHeart(x, y, 1, CP_RGB_PINK_PALE);
			}
			else
			{
				const INT32 d = 1 + int((seed >> 20) % 2);
				FillRect(x, y, d, d, CP_RGB_BLUE_WALL);
			}
		}
	}

	// the cheap depth every 1999 site faked with a table border
	void DropShadow(INT32 x, INT32 y, INT32 w, INT32 h)
	{
		FillRounded(x + 3, y + 3, w, h, CP_RGB_SHADOW, 3, CP_RGB_BG);
	}

	// the match percentage as a segmented instrument meter - the site
	// reads compatibility the way a charging handle reads a magazine
	void DrawMeter(INT32 x, INT32 y, INT32 w, int pct, UINT32 fill)
	{
		FillRect(x - 1, y - 1, w + 2, 9, CP_RGB_INK);
		const INT32 lit = w * std::clamp(pct, 0, 100) / 100;
		for (INT32 sx = 0; sx + 4 <= w; sx += 5)
		{
			FillRect(x + sx, y, 4, 7,
					sx < lit ? fill : CP_RGB_CARD_DIM);
		}
	}

	// the 1999 censor bar: chunky checkerboard over a face, clipped to it
	void BlurOver(INT32 x, INT32 y, INT32 w, INT32 h)
	{
		for (INT32 yy = 0; yy < h; yy += 3)
		{
			const INT32 bh = std::min<INT32>(3, h - yy);
			for (INT32 xx = ((yy / 3) % 2) * 3; xx < w; xx += 6)
			{
				FillRect(x + xx, y + yy, std::min<INT32>(3, w - xx), bh,
						CP_RGB_BLUE_DK);
			}
		}
	}

	// --- profiles and matching ---------------------------------------------
	bool PlayerHasProfile()
	{
		return (gCupidPersist.ubFlags & CUPID_FLAG_PROFILE) != 0;
	}

	ProfileID PlayerImpPid()
	{
		return static_cast<ProfileID>(PLAYER_GENERATED_CHARACTER_ID +
						LaptopSaveInfo.iVoiceId);
	}

	DatingGame::Profile BuildPlayerProfile()
	{
		DatingGame::Profile p;
		for (int q = 0; q < DatingGame::NUM_QUESTIONS; ++q)
		{
			p.answers[q] = GetAnswer(q);
		}
		if (LaptopSaveInfo.fIMPCompletedFlag)
		{
			MERCPROFILESTRUCT const& imp = GetProfile(PlayerImpPid());
			p.attitude = imp.bAttitude;
			p.trait    = imp.bPersonalityTrait;
			p.sex      = imp.bSex;
			p.sexist   = imp.bSexist;
		}
		else
		{
			const int8_t tallied = DatingGame::TallyAttitude(p.answers);
			p.attitude = tallied >= 0 ? tallied : DatingGame::ATT_NORMAL;
			p.sex = (gCupidPersist.ubFlags & CUPID_FLAG_FEMALE)
					? DatingGame::SEX_FEMALE : DatingGame::SEX_MALE;
		}
		return p;
	}

	DatingGame::Profile BuildMercProfile(ProfileID pid)
	{
		MERCPROFILESTRUCT const& m = GetProfile(pid);
		DatingGame::Profile p;
		p.attitude = m.bAttitude < NUM_ATTITUDES ? m.bAttitude
							 : DatingGame::ATT_NORMAL;
		p.trait  = m.bPersonalityTrait;
		p.sex    = m.bSex;
		p.sexist = INT8(m.bSexist);
		// the site "found" their old I.M.P. answer sheet; really it derives
		// one from their canon temperament, seeded so it never changes
		DatingGame::DeriveAnswers(pid, p.attitude, p.trait, p.answers);
		return p;
	}

	int RosterIndexOf(ProfileID pid)
	{
		for (int i = 0; i < int(gCupidRoster.size()); ++i)
		{
			if (gCupidRoster[i].pid == pid) return i;
		}
		return -1;
	}

	DatingGame::Match MatchWith(ProfileID pid)
	{
		const int idx = RosterIndexOf(pid);
		const DatingGame::Profile self = BuildPlayerProfile();
		const DatingGame::Profile other = idx >= 0
			? gCupidRoster[size_t(idx)].prof : BuildMercProfile(pid);
		return DatingGame::Compute(self, other,
			DatingGame::ChemistrySeed(PlayerImpPid(), pid));
	}

	ST::string MatchLabel(const DatingGame::Match& m)
	{
		if (m.uncertainty > 0)
		{
			const int lo = std::max(1, m.percent - m.uncertainty);
			const int hi = std::min(99, m.percent + m.uncertainty);
			return ST::format(T(CPS_MATCH_RANGE), lo, hi);
		}
		return ST::format(T(CPS_MATCH), m.percent);
	}

	UINT8 MatchColour(int percent)
	{
		return percent >= 75 ? FONT_LTGREEN
		     : percent >= 50 ? FONT_MCOLOR_LTYELLOW : FONT_LTRED;
	}

	bool SeekAllows(ProfileID pid)
	{
		if (giCupidSeek == SEEK_EVERYONE) return true;
		const INT8 sex = GetProfile(pid).bSex;
		return giCupidSeek == SEEK_MEN ? sex == MALE : sex == FEMALE;
	}

	bool MemberIsDead(ProfileID pid)
	{
		return GetProfile(pid).bMercStatus == MERC_IS_DEAD;
	}

	bool MemberIsMarried(ProfileID pid)
	{
		return pid == FLO && gubFact[FACT_PC_MARRYING_DARYL_IS_FLO];
	}

	CupidStr MemberStatus(ProfileID pid)
	{
		if (MemberIsDead(pid))    return CPS_STATUS_GONE;
		if (MemberIsMarried(pid)) return CPS_STATUS_MARRIED;
		MERCPROFILESTRUCT const& p = GetProfile(pid);
		if (IsMercOnTeam(pid))    return CPS_STATUS_PAYROLL;
		if (p.bMercStatus == MERC_OK) return CPS_STATUS_ONLINE;
		return CPS_STATUS_CONTRACT;
	}

	int CountBlockedBy(ProfileID pid)
	{
		int n = 0;
		for (const Member& m : gCupidRoster)
		{
			if (m.pid == pid || m.locked) continue;
			if (HATED_MERC(GetProfile(m.pid), INT8(pid))) ++n;
		}
		return n;
	}

	const char* FlavorFor(ProfileID pid)
	{
		for (const FlavorLine& f : CUPID_FLAVOR)
		{
			if (f.pid == pid) return gfCupidGerman ? f.de : f.en;
		}
		return nullptr;
	}

	// Does this member like you back? Deterministic, weighted by the match
	// percentage - the algorithm knows, it always knew.
	bool LikesYouBack(ProfileID pid)
	{
		const DatingGame::Match m = MatchWith(pid);
		const uint32_t roll =
			(DatingGame::ChemistrySeed(PlayerImpPid(), pid) >> 7) % 100;
		// a proposal on file flatters: the ring is worth 25 points
		const int sway =
			BitGet(gCupidPersist.ubProposed, pid) ? 25 : 0;
		return int(roll) < m.percent + sway;
	}

	bool IsMatched(ProfileID pid)
	{
		return BitGet(gCupidPersist.ubLiked, pid) && LikesYouBack(pid);
	}

	std::vector<ProfileID> AllMatches()
	{
		std::vector<ProfileID> out;
		for (const Member& m : gCupidRoster)
		{
			if (!m.locked && IsMatched(m.pid)) out.push_back(m.pid);
		}
		return out;
	}

	// the GOLD tease: members who would like you back but whom you have not
	// liked yet
	std::vector<ProfileID> SecretAdmirers()
	{
		std::vector<ProfileID> out;
		if (!PlayerHasProfile()) return out;
		for (const Member& m : gCupidRoster)
		{
			if (m.locked || MemberIsDead(m.pid)) continue;
			if (BitGet(gCupidPersist.ubLiked, m.pid)) continue;
			if (BitGet(gCupidPersist.ubPassed, m.pid)) continue;
			if (!SeekAllows(m.pid)) continue;
			if (LikesYouBack(m.pid)) out.push_back(m.pid);
			if (out.size() >= 5) break;
		}
		return out;
	}

	// --- the deck ----------------------------------------------------------
	void BuildRoster()
	{
		gCupidRoster.clear();
		for (ProfileID pid = 0; pid < BUBBA + 1; ++pid)
		{
			const MercProfile prof(pid);
			if (!prof.isAIMMerc() && !prof.isMERCMerc()) continue;
			Member m;
			m.pid    = pid;
			m.merc   = prof.isMERCMerc();
			m.locked = false;
			m.fresh  = false;
			m.prof   = BuildMercProfile(pid);
			gCupidRoster.push_back(m);
		}
		for (Member& m : gCupidRoster)
		{
			if (!m.merc) continue;
			for (const MERCListingModel* l : GCM->getMERCListings())
			{
				if (GetProfileIDFromMERCListing(l) != m.pid) continue;
				m.locked = l->index > LaptopSaveInfo.gubLastMercIndex;
				m.fresh  = !m.locked && !l->isAvailableAtStart();
				break;
			}
		}
	}

	bool AnyLockedMembers()
	{
		for (const Member& m : gCupidRoster)
		{
			if (m.locked) return true;
		}
		return false;
	}

	void BuildDeck()
	{
		gCupidDeck.clear();
		gCupidDeckPos = 0;
		giCupidCardDx = 0;
		giCupidFlyDir = 0;

		std::vector<ProfileID> pool;
		for (const Member& m : gCupidRoster)
		{
			if (m.locked || MemberIsDead(m.pid) || MemberIsMarried(m.pid))
			{
				continue;
			}
			if (BitGet(gCupidPersist.ubLiked, m.pid)) continue;
			if (BitGet(gCupidPersist.ubPassed, m.pid)) continue;
			if (!SeekAllows(m.pid)) continue;
			pool.push_back(m.pid);
		}

		// nobody left unswiped: the passes come back around, as they do
		if (pool.empty())
		{
			bool anyPassed = false;
			for (UINT8 b : gCupidPersist.ubPassed) anyPassed |= b != 0;
			if (anyPassed)
			{
				std::fill(std::begin(gCupidPersist.ubPassed),
					std::end(gCupidPersist.ubPassed), UINT8(0));
				for (const Member& m : gCupidRoster)
				{
					if (m.locked || MemberIsDead(m.pid) ||
					    MemberIsMarried(m.pid)) continue;
					if (BitGet(gCupidPersist.ubLiked, m.pid)) continue;
					pool.push_back(m.pid);
				}
			}
		}

		// seeded day shuffle: the deck holds still within a day and
		// reshuffles overnight
		uint32_t seed = uint32_t(GetWorldDay()) * 2654435761u + 17;
		for (int i = int(pool.size()) - 1; i > 0; --i)
		{
			seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
			std::swap(pool[size_t(i)], pool[seed % uint32_t(i + 1)]);
		}

		// members only: a pager UI browses people, not interstitials.
		// Speck sells GOLD from his banner slot instead.
		for (size_t i = 0; i < pool.size(); ++i)
		{
			gCupidDeck.push_back({ CARD_MEMBER, pool[i] });
		}
		gCupidDeck.push_back({ CARD_END, 0 });
	}

	const Card& CurrentCard()
	{
		static const Card end = { CARD_END, 0 };
		if (gCupidDeckPos < 0 || gCupidDeckPos >= int(gCupidDeck.size()))
		{
			return end;
		}
		return gCupidDeck[size_t(gCupidDeckPos)];
	}

	// Speck read the April 1999 research: Sundays are the big day for
	// personals. He responded with a promotion.
	bool IsSiteSunday() { return GetWorldDay() % 7 == 0; }

	// the other ledger: an overdue M.E.R.C. account follows you here
	bool SpeckHasGrudge()
	{
		switch (LaptopSaveInfo.gubPlayersMercAccountStatus)
		{
			case MERC_ACCOUNT_SUSPENDED:
			case MERC_ACCOUNT_INVALID:
			case MERC_ACCOUNT_VALID_FIRST_WARNING:
				return true;
			default:
				return false;
		}
	}

	void RefreshDailyLikes()
	{
		const UINT16 today = UINT16(GetWorldDay());
		if (gCupidPersist.usDeckDay != today)
		{
			gCupidPersist.usDeckDay   = today;
			gCupidPersist.ubLikesLeft = UINT8(IsSiteSunday()
					? CP_FREE_LIKES_A_DAY * 2 : CP_FREE_LIKES_A_DAY);
		}
	}

	bool ChargeSpeck(INT32 amount); // defined with the money below

	// one GOLD purchase path for the ad card and the popup: Speck will not
	// sell prestige to a member whose other account with him is overdue
	void TryBuyGold()
	{
		if (IsGold()) return;
		if (SpeckHasGrudge())
		{
			giCupidTicker = CPS_TICKER_DEBT;
			return;
		}
		if (ChargeSpeck(CP_GOLD_PRICE))
		{
			gCupidPersist.ubFlags |= CUPID_FLAG_GOLD;
			// his canon settle-up line, and he means it
			giCupidTicker = CPS_TICKER_THANKS;
		}
	}

	bool CanLike()
	{
		return IsGold() || gCupidPersist.ubLikesLeft > 0;
	}

	// --- money -------------------------------------------------------------
	bool ChargeSpeck(INT32 amount)
	{
		if (LaptopSaveInfo.iCurrentBalance < amount)
		{
			giCupidTicker = CPS_TICKER_BROKE;
			return false;
		}
		AddTransactionToPlayersBook(PAYMENT_TO_NPC, SPECK, GetWorldTotalMin(),
					-amount);
		gCupidPersist.iSpent += amount;
		return true;
	}

	// --- the questionnaire ---------------------------------------------------
	// Not I.M.P.'s. Legally. Speck could not license the Institute's
	// instrument, so he rewrote it as sixteen romance questions - and every
	// answer lands in the same slot as its I.M.P. twin, votes and all, which
	// is why the matching still works and why the Institute's lawyers keep
	// writing. The vote each answer mirrors is tagged so the mapping never
	// drifts: edit an answer, keep its temperament.

	const char* const CUPID_QUIZ_Q[2][DatingGame::NUM_QUESTIONS] =
	{
		{
			"Your ideal first date:",
			"Your new flame cannot field-strip a rifle. You:",
			"Your date's diary is on the table, and they are not. You:",
			"At their family dinner, you:",
			"Their ex walks into the bar. Twice your size. Angry. You:",
			"The perfect romantic evening:",
			"The best gift somebody could give you:",
			"Holding hands. Thoughts?",
			"Anniversaries:",
			"Your love letters would read like:",
			"Your partner wants to learn to shoot. You:",
			"Pick the dream couples activity:",
			"Honeymoon destination:",
			"Moving in together. The one thing the apartment must not have:",
			"The bouquet is in the air at a friend's wedding. You:",
			"And finally: do you believe in love at first sight?",
		},
		{
			"Ihr ideales erstes Date:",
			"Ihre neue Flamme kann kein Gewehr zerlegen. Sie:",
			"Das Tagebuch Ihres Dates liegt auf dem Tisch. Sie:",
			"Beim Familienessen der Angebeteten:",
			"Der Ex betritt die Bar. Doppelt so gross. Wuetend. Sie:",
			"Der perfekte romantische Abend:",
			"Das beste Geschenk fuer Sie:",
			"Haendchenhalten. Gedanken dazu?",
			"Jahrestage:",
			"Ihre Liebesbriefe laesen sich wie:",
			"Ihr Partner will schiessen lernen. Sie:",
			"Waehlen Sie die Traum-Paaraktivitaet:",
			"Flitterwochen-Ziel:",
			"Zusammenziehen. Was die Wohnung keinesfalls haben darf:",
			"Der Brautstrauss fliegt auf der Hochzeit. Sie:",
			"Und zuletzt: glauben Sie an Liebe auf den ersten Blick?",
		},
	};

	const char* const CUPID_QUIZ_A[2][DatingGame::NUM_QUESTIONS][8] =
	{
		{
			{ // q0
				"Sparring practice. You learn a lot about someone in a clinch.", // martial arts / ambidext
				"Two tables, same restaurant. Wave occasionally.",               // ATT_LONER
				"A bar with a reputation. If trouble starts, you'll see what they're made of.", // hand-to-hand
				"Somewhere locked, with a view. Doors have never stopped you.",  // lockpicking
				"Darts night. Winner plans the second date. You will plan the second date.", // throwing
				"Anywhere!! Every date is the best date until proven otherwise!!", // ATT_OPTIMIST
				nullptr, nullptr },
			{ // q1
				"Teach them. Slowly. Patiently. Blindfolded by week three. This is romance.", // teaching
				"Say nothing and watch from a polite distance. They'll manage, or not.", // stealthy
				"Do it for them, very fast, while maintaining eye contact.",     // PSYCHO
				"Invite the whole squad over and make it a party. Rifles optional!", // ATT_FRIENDLY
				nullptr, nullptr, nullptr, nullptr },
			{ // q2
				"Locked? That has never once been a problem.",                   // lockpicking
				"Skip it. Anything worth reading in there is about you anyway.", // ATT_ARROGANT
				"Read it, replace it to the millimetre. No one will ever know.", // stealthy
				"Leave it. People are allowed a private page.",                  // ATT_NORMAL
				nullptr, nullptr, nullptr, nullptr },
			{ // q3
				"Bring the biggest dish on the table. Overwhelming firepower applies to casseroles.", // auto-weapons
				"Know everyone's name by dessert. Including the dog's.",         // ATT_FRIENDLY
				"Compliment the cooking, help with the dishes, survive.",        // ATT_NORMAL
				"Rank the cooking. Honestly. Somebody had to.",                  // ATT_ASSHOLE
				"Step outside for air that lasts ninety minutes.",               // ATT_LONER
				nullptr, nullptr, nullptr },
			{ // q4
				"Remember an urgent appointment somewhere that is not this bar.", // ATT_COWARD
				"Buy him a beer. Exes are just people with good taste and bad luck.", // none
				"Stand up first. Ask questions later. Preferably never.",        // ATT_AGGRESSIVE
				"Let your date decide how this goes. It is, after all, their ex.", // none
				nullptr, nullptr, nullptr, nullptr },
			{ // q5
				"Ends before dark. Nothing good happens after dark.",            // ATT_COWARD
				"STARTS at midnight. That's when you're at your best.",          // night ops
				"Happens outside. Open sky. Anywhere without walls, honestly.",  // CLAUSTROPHOBIC
				"Ends with dancing, once the band gives up before you do.",      // none
				"Ends with a story worth the ride home.",                        // none
				nullptr, nullptr, nullptr },
			{ // q6
				"Anything with a circuit board. You'll take it apart. Lovingly.", // electronics
				"A blade with history. Balanced. Personal. Quiet.",              // knifing
				"Night-vision goggles. For stargazing. Obviously stargazing.",   // night ops
				"Flowers. The classics survived this country for a reason.",     // none
				nullptr, nullptr, nullptr, nullptr },
			{ // q7
				"Either hand. Both hands. You genuinely have no preference.",    // ambidext
				"Nice, in the right weather.",                                   // none
				"The first step on a long and beautiful road!!",                 // ATT_OPTIMIST
				"Grip strength is a love language.",                             // PSYCHO
				nullptr, nullptr, nullptr, nullptr },
			{ // q8
				"Fall on a date you will remember next week at the latest.",     // FORGETFUL
				"Deserve a nice dinner and an early night.",                     // none
				"Are countdown markers. Everything ends; this is how you check the schedule.", // ATT_PESSIMIST
				"Are SO much pressure. The reservation, the gift, the TIMING. Sweating already.", // NERVOUS
				nullptr, nullptr, nullptr, nullptr },
			{ // q9
				"A field report. Short. Accurate. Signed.",                      // none
				"A weather forecast. Cloudy. Chance of leaving.",                // ATT_PESSIMIST
				"A performance review, areas for improvement included.",         // ATT_ASSHOLE
				"Three drafts, none sent, each apologising for the last.",       // NERVOUS
				nullptr, nullptr, nullptr, nullptr },
			{ // q10
				"Book them a professional. Some things shouldn't stay in the family.", // none
				"Clear a weekend. Draw diagrams. Start with breathing. You've waited for this.", // teaching
				"Live fire, day one. They'll learn or they'll flinch. Either answer is useful.", // ATT_AGGRESSIVE
				"Range on Saturday, ear protection, pizza after.",               // ATT_NORMAL
				"Gently suggest fishing.",                                       // none
				nullptr, nullptr, nullptr },
			{ // q11
				"Couples martial arts. Falling for each other, technically.",    // martial arts / ambidext
				"Knife throwing. Trust exercises, with consequences.",           // knifing
				"A cooking class.",                                              // none
				"The range's full-auto rental hour. Romance is a volume business.", // auto-weapons
				"Boxing lessons. Nothing says trust like a taped fist.",         // hand-to-hand
				"Building a radio from parts. Two soldering irons, one dream.",  // electronics
				"Watching them attempt any of the above. Adorable.",             // none (with attitude)
				"A quiet drive. Scenic route. No agenda.",                       // none
				},
			{ // q12
				"Wherever the tickets say. You definitely booked something.",    // FORGETFUL
				"The coast. Nice hotel. Nothing exploding.",                     // ATT_NORMAL
				"A city with museums and working plumbing.",                     // ATT_NORMAL
				"Somewhere COLD. Snow. An ice hotel. You melt above room temperature.", // HEAT_INTOLERANT
				nullptr, nullptr, nullptr, nullptr },
			{ // q13
				"No windows. Absolutely not. You need exits and sky.",           // CLAUSTROPHOBIC
				"If the rent is fair and the roof holds, you're flexible.",      // ATT_NORMAL
				"No air conditioning. In THIS climate? Non-negotiable.",         // HEAT_INTOLERANT
				"Thin walls. The neighbours do not need a radio play.",          // none
				nullptr, nullptr, nullptr, nullptr },
			{ // q14
				"Assess the arc mid-flight. You could return it with better rotation.", // throwing
				"Catch it. Either hand. You weren't even looking.",              // ambidext
				"Don't bother catching it. Destiny knows your address.",         // ATT_ARROGANT
				"Step aside. Married friends are insufferable enough already.",  // none
				nullptr, nullptr, nullptr, nullptr },
			{ // q15 - the algorithm ignores all four, which is the joke
				"Yes.",
				"No.",
				"Only in good light.",
				"Ask my next of kin.",
				nullptr, nullptr, nullptr, nullptr },
		},
		{
			{ // q0
				"Sparring. Im Clinch lernt man einen Menschen kennen.",
				"Zwei Tische, gleiches Restaurant. Gelegentlich winken.",
				"Eine Bar mit Ruf. Wenn was losgeht, sieht man, was in ihnen steckt.",
				"Irgendwo Abgeschlossenes mit Aussicht. Tueren waren nie ein Hindernis.",
				"Dartabend. Der Sieger plant das zweite Date. Sie planen das zweite Date.",
				"Egal wo!! Jedes Date ist das beste, bis das Gegenteil bewiesen ist!!",
				nullptr, nullptr },
			{ // q1
				"Es beibringen. Langsam. Geduldig. Ab Woche drei mit Augenbinde. Das ist Romantik.",
				"Nichts sagen und aus hoeflicher Distanz zusehen. Wird schon. Oder nicht.",
				"Es fuer sie erledigen, sehr schnell, mit Blickkontakt.",
				"Den ganzen Trupp einladen und ein Fest daraus machen. Gewehre optional!",
				nullptr, nullptr, nullptr, nullptr },
			{ // q2
				"Abgeschlossen? War noch nie ein Problem.",
				"Weiterblaettern? Wozu. Alles Lesenswerte darin handelt ohnehin von Ihnen.",
				"Lesen und millimetergenau zuruecklegen. Niemand wird es je wissen.",
				"Liegen lassen. Eine private Seite steht jedem zu.",
				nullptr, nullptr, nullptr, nullptr },
			{ // q3
				"Die groesste Schuessel mitbringen. Ueberlegene Feuerkraft gilt auch fuer Auflaeufe.",
				"Bis zum Nachtisch jeden Namen kennen. Auch den des Hundes.",
				"Das Essen loben, abwaschen helfen, ueberleben.",
				"Das Essen benoten. Ehrlich. Einer musste es tun.",
				"Kurz Luft schnappen gehen. Neunzig Minuten lang.",
				nullptr, nullptr, nullptr },
			{ // q4
				"Sich an einen dringenden Termin erinnern, weit weg von dieser Bar.",
				"Ihm ein Bier ausgeben. Exen sind nur Menschen mit gutem Geschmack und Pech.",
				"Zuerst aufstehen. Fragen spaeter. Am besten nie.",
				"Das Date entscheiden lassen. Es ist schliesslich deren Ex.",
				nullptr, nullptr, nullptr, nullptr },
			{ // q5
				"Endet vor Einbruch der Dunkelheit. Nach Einbruch passiert nichts Gutes.",
				"BEGINNT um Mitternacht. Da sind Sie in Bestform.",
				"Findet draussen statt. Offener Himmel. Hauptsache keine Waende.",
				"Endet mit Tanzen, wenn die Band vor Ihnen aufgibt.",
				"Endet mit einer Geschichte, die die Heimfahrt wert ist.",
				nullptr, nullptr, nullptr },
			{ // q6
				"Alles mit Platine. Sie nehmen es auseinander. Liebevoll.",
				"Eine Klinge mit Geschichte. Ausbalanciert. Persoenlich. Leise.",
				"Nachtsichtgeraet. Zum Sternegucken. Offensichtlich Sternegucken.",
				"Blumen. Die Klassiker haben dieses Land nicht umsonst ueberlebt.",
				nullptr, nullptr, nullptr, nullptr },
			{ // q7
				"Beide Haende. Egal welche. Sie haben wirklich keine Praeferenz.",
				"Nett, bei passendem Wetter.",
				"Der erste Schritt auf einem langen, wunderschoenen Weg!!",
				"Griffkraft ist eine Liebessprache.",
				nullptr, nullptr, nullptr, nullptr },
			{ // q8
				"Fallen auf ein Datum, das Ihnen spaetestens naechste Woche einfaellt.",
				"Verdienen ein gutes Essen und einen fruehen Abend.",
				"Sind Countdown-Marken. Alles endet; so prueft man den Fahrplan.",
				"Sind SO viel Druck. Die Reservierung, das Geschenk, das TIMING. Schon nass geschwitzt.",
				nullptr, nullptr, nullptr, nullptr },
			{ // q9
				"Ein Feldbericht. Kurz. Praezise. Unterschrieben.",
				"Ein Wetterbericht. Bewoelkt. Aussicht auf Abschied.",
				"Eine Leistungsbeurteilung, Verbesserungspotential inklusive.",
				"Drei Entwuerfe, keiner verschickt, jeder entschuldigt sich fuer den letzten.",
				nullptr, nullptr, nullptr, nullptr },
			{ // q10
				"Einen Profi buchen. Manches gehoert nicht in die Familie.",
				"Ein Wochenende freiraeumen. Diagramme zeichnen. Mit der Atmung beginnen. Sie haben darauf gewartet.",
				"Scharfer Schuss, Tag eins. Lernen oder zucken. Beide Antworten sind nuetzlich.",
				"Samstag Schiessstand, Gehoerschutz, danach Pizza.",
				"Behutsam Angeln vorschlagen.",
				nullptr, nullptr, nullptr },
			{ // q11
				"Paar-Kampfsport. Sich fallen lassen, im technischen Sinn.",
				"Messerwerfen. Vertrauensuebungen mit Konsequenzen.",
				"Ein Kochkurs.",
				"Die Vollautomatik-Stunde am Schiessstand. Romantik ist ein Mengengeschaeft.",
				"Boxstunden. Nichts sagt Vertrauen wie eine bandagierte Faust.",
				"Ein Radio aus Einzelteilen bauen. Zwei Loetkolben, ein Traum.",
				"Ihnen bei alldem zusehen. Entzueckend.",
				"Eine ruhige Fahrt. Panoramastrecke. Kein Plan.",
				},
			{ // q12
				"Wohin die Tickets sagen. Sie haben definitiv etwas gebucht.",
				"Die Kueste. Gutes Hotel. Nichts, das explodiert.",
				"Eine Stadt mit Museen und funktionierender Kanalisation.",
				"Irgendwo KALTES. Schnee. Ein Eishotel. Ueber Zimmertemperatur schmelzen Sie.",
				nullptr, nullptr, nullptr, nullptr },
			{ // q13
				"Keine Fenster. Ausgeschlossen. Sie brauchen Ausgaenge und Himmel.",
				"Wenn die Miete stimmt und das Dach haelt, sind Sie flexibel.",
				"Keine Klimaanlage. In DIESEM Klima? Nicht verhandelbar.",
				"Duenne Waende. Die Nachbarn brauchen kein Hoerspiel.",
				nullptr, nullptr, nullptr, nullptr },
			{ // q14
				"Die Flugbahn analysieren. Sie koennten ihn mit besserer Rotation zurueckwerfen.",
				"Fangen. Mit irgendeiner Hand. Sie haben nicht mal hingesehen.",
				"Nicht fangen. Das Schicksal kennt Ihre Adresse.",
				"Beiseitetreten. Verheiratete Freunde sind schon unertraeglich genug.",
				nullptr, nullptr, nullptr, nullptr },
			{ // q15
				"Ja.",
				"Nein.",
				"Nur bei gutem Licht.",
				"Fragen Sie meine Hinterbliebenen.",
				nullptr, nullptr, nullptr, nullptr },
		},
	};

	ST::string QuizQuestion(int q)
	{
		if (q < 0 || q >= DatingGame::NUM_QUESTIONS) return ST::string();
		return CUPID_QUIZ_Q[gfCupidGerman ? 1 : 0][q];
	}

	ST::string QuizAnswer(int q, int a)
	{
		if (q < 0 || q >= DatingGame::NUM_QUESTIONS) return ST::string();
		if (a < 0 || a >= DatingGame::ANSWER_COUNT[q]) return ST::string();
		const char* const text = CUPID_QUIZ_A[gfCupidGerman ? 1 : 0][q][a];
		return text ? text : "";
	}

	void SendWelcomeOnce()
	{
		if (gCupidPersist.ubFlags & CUPID_FLAG_WELCOMED) return;
		gCupidPersist.ubFlags |= CUPID_FLAG_WELCOMED;
		// Speck reviews every profile personally; personally takes a while
		AddStrategicEvent(EVENT_CUPID_SPECK_EMAIL,
				GetWorldTotalMin() + 45 + Random(90), 0x10);
	}

	void BuildDeckAndGoSwiping()
	{
		giCupidTicker = CPS_TICKER_DEFAULT;
		gfCupidQuizLive = false;
		giCupidQuizQ = -1;
		gCupidPage = CPP_DECK;
		BuildDeck();
	}

	void FinishQuiz()
	{
		gCupidPersist.ubFlags |= CUPID_FLAG_PROFILE;
		gCupidPersist.ubFlags &= UINT8(~CUPID_FLAG_IMP_ANSWERS);
		SendWelcomeOnce();
		BuildDeckAndGoSwiping();
	}

	// --- swiping -----------------------------------------------------------
	void AdvanceCard()
	{
		giCupidCardDx = 0;
		giCupidFlyDir = 0;
		ResetCardScroll();
		StartPhotoLoad(); // the next photo arrives at 28.8kbps, at best
		if (gCupidDeckPos < int(gCupidDeck.size()) - 1) ++gCupidDeckPos;
	}

	// flicking back a page: nothing judged, nothing forgotten
	void RetreatCard()
	{
		if (gCupidDeckPos <= 0) return;
		giCupidCardDx = 0;
		giCupidFlyDir = 0;
		ResetCardScroll();
		StartPhotoLoad();
		--gCupidDeckPos;
	}

	// what happens once the card has flown off the edge
	void CommitSwipe(int dir)
	{
		const Card card = CurrentCard();

		if (card.kind != CARD_MEMBER)
		{
			// the heart accepts an offer; the X declines it
			if (dir > 0 && card.kind == CARD_AD_GOLD) TryBuyGold();
			if (card.kind != CARD_END) AdvanceCard();
			else { giCupidCardDx = 0; giCupidFlyDir = 0; }
			CupidRedraw();
			return;
		}

		if (dir > 0)
		{
			// flicking back and kissing again is one kiss, not two
			if (!IsGold() && gCupidPersist.ubLikesLeft > 0 &&
			    !BitGet(gCupidPersist.ubLiked, card.pid))
			{
				--gCupidPersist.ubLikesLeft;
			}
			BitSet(gCupidPersist.ubLiked, card.pid);
			if (LikesYouBack(card.pid))
			{
				gCupidSplashPid = card.pid;
				AdvanceCard();
				gCupidPage = CPP_SPLASH;
				CupidRedraw();
				return;
			}
		}
		else
		{
			BitSet(gCupidPersist.ubPassed, card.pid);
		}
		AdvanceCard();
		CupidRedraw();
	}

	// a kiss is a verdict, not a page-turn: the card stays where it is,
	// stamped, and the pager remains the only way to move
	void KissCard()
	{
		const Card& card = CurrentCard();
		if (card.kind != CARD_MEMBER) return;
		if (BitGet(gCupidPersist.ubProposed, card.pid) &&
		    IsMatched(card.pid))
		{
			// an engagement binds; everything else is revisable
			giCupidTicker = CPS_TICKER_VERDICT;
			CupidRedraw();
			return;
		}
		if (BitGet(gCupidPersist.ubLiked, card.pid)) return;
		if (!CanLike())
		{
			giCupidTicker = CPS_TICKER_NO_LIKES;
			CupidNotice(CPS_NOTICE_OUT, 1);
			CupidRedraw();
			return;
		}
		if (BitGet(gCupidPersist.ubPassed, card.pid))
		{
			// changed your mind: allowed, noted, not free
			BitClear(gCupidPersist.ubPassed, card.pid);
			giCupidTicker = CPS_TICKER_SWITCHED;
		}
		if (!IsGold() && gCupidPersist.ubLikesLeft > 0)
		{
			--gCupidPersist.ubLikesLeft;
		}
		BitSet(gCupidPersist.ubLiked, card.pid);
		if (LikesYouBack(card.pid))
		{
			gCupidSplashPid = card.pid;
			gCupidPage = CPP_SPLASH;
		}
		CupidRedraw();
	}

	void DeclineCard()
	{
		const Card& card = CurrentCard();
		if (card.kind != CARD_MEMBER) return;
		if (BitGet(gCupidPersist.ubProposed, card.pid) &&
		    IsMatched(card.pid))
		{
			giCupidTicker = CPS_TICKER_VERDICT;
			CupidRedraw();
			return;
		}
		if (BitGet(gCupidPersist.ubPassed, card.pid)) return;
		if (BitGet(gCupidPersist.ubLiked, card.pid))
		{
			// the kiss is not refunded. site policy since tuesday.
			BitClear(gCupidPersist.ubLiked, card.pid);
			giCupidTicker = CPS_TICKER_SWITCHED;
		}
		BitSet(gCupidPersist.ubPassed, card.pid);
		CupidRedraw();
	}

	// a proposal: one a day, matches only, twenty-five dollars, and
	// Speck sells the flowers next door
	void MarryCard()
	{
		const Card& card = CurrentCard();
		if (card.kind != CARD_MEMBER) return;
		if (BitGet(gCupidPersist.ubProposed, card.pid))
		{
			giCupidTicker = CPS_TICKER_VERDICT;
			CupidRedraw();
			return;
		}
		if (gCupidPersist.usProposeDay == UINT16(GetWorldDay()))
		{
			CupidNotice(CPS_TICKER_PROPOSE_ONE_A_DAY, 0);
			CupidRedraw();
			return;
		}
		if (SpeckHasGrudge())
		{
			CupidNotice(CPS_TICKER_DEBT, 0);
			CupidRedraw();
			return;
		}
		if (ChargeSpeck(25))
		{
			// the super-kiss: the ring includes the like, gratis, and
			// carries twenty-five points of pure persuasion
			BitClear(gCupidPersist.ubPassed, card.pid);
			BitSet(gCupidPersist.ubLiked, card.pid);
			BitSet(gCupidPersist.ubProposed, card.pid);
			gCupidPersist.usProposeDay = UINT16(GetWorldDay());
			if (LikesYouBack(card.pid))
			{
				giCupidTicker = CPS_TICKER_PROPOSE_OK;
				gCupidSplashPid = card.pid;
				gCupidPage = CPP_SPLASH;
			}
			else
			{
				CupidNotice(CPS_NOTICE_PROPOSED_WAIT, 0);
			}
		}
		else
		{
			CupidNotice(CPS_TICKER_BROKE, 0);
		}
		CupidRedraw();
	}

	void StartFly(int dir)
	{
		if (giCupidFlyDir != 0) return;
		const Card& card = CurrentCard();
		if (card.kind == CARD_END) return;
		// a like that cannot be paid for never leaves the hand
		if (dir > 0 && card.kind == CARD_MEMBER && !CanLike())
		{
			giCupidTicker = CPS_TICKER_NO_LIKES;
			giCupidCardDx = 0;
			CupidRedraw();
			return;
		}
		giCupidFlyDir = dir;
		CupidRedraw();
	}

	// --- the lounge: ArulcoNet IRC ------------------------------------------
	// Open channels, one per appetite. Anyone may lurk - the whole funnel
	// is that you can read every room without a profile - but speaking
	// requires membership, and one door has a velvet rope.

	#define LNG_SYS   INT8(-1) // *** channel notices
	#define LNG_SPECK INT8(-2) // the operator, selling
	#define LNG_YOU   INT8(-3) // your local echo
	#define CPL_ROOMS 4

	struct LoungeLine    { INT8 who; ST::string text; };
	struct LoungePending { INT8 who; ST::string text; UINT32 due; };
	struct LoungeRoom
	{
		std::vector<LoungeLine>    log;
		std::vector<LoungePending> pend;
		UINT32 tail      = 0; // when the queued talk runs dry
		UINT32 next      = 0; // the next spontaneous event
		UINT32 seen      = 0; // when you last had this room open
		INT32  scroll    = 0; // px above the newest line; 0 = pinned
		INT32  scrollMax = 0;
		bool   seeded    = false;
		std::vector<ST::string> recent; // what the room said lately
	};
	LoungeRoom gCupidRooms[CPL_ROOMS];
	int    giCupidRoom = 0;
	UINT32 guiCupidLoungeSaid = 0;
	UINT32 guiCupidLoungeRoll = 1;
	bool   gfCupidLoungeMourned = false;

	const char* const CPL_NAME[CPL_ROOMS] =
		{ "#tender", "#roughlove", "#ballads", "#goldhearts" };

	LoungeRoom& Room() { return gCupidRooms[giCupidRoom]; }
	bool RoomLocked() { return giCupidRoom == 3 && !IsGold(); }

	// where the last render put the clickable mugshot chips
	struct LoungeChipAt { INT32 top; INT8 who; };
	std::vector<LoungeChipAt> gCupidLoungeChipsAt;
	std::string gCupidLoungeInput; // what you have typed, parlour fashion

	UINT32 LoungeRoll()
	{
		guiCupidLoungeRoll = guiCupidLoungeRoll * 1103515245u + 12345u;
		return guiCupidLoungeRoll >> 16;
	}

	// who drinks where: the rough crowd, the slow hearts, everyone else
	bool RoomAdmits(const Member& m, int room)
	{
		if (room == 1)
		{
			return m.prof.attitude == DatingGame::ATT_AGGRESSIVE ||
			       m.prof.attitude == DatingGame::ATT_ARROGANT ||
			       m.prof.attitude == DatingGame::ATT_ASSHOLE ||
			       m.prof.trait == DatingGame::TRAIT_PSYCHO;
		}
		if (room == 2)
		{
			return m.prof.attitude == DatingGame::ATT_FRIENDLY ||
			       m.prof.attitude == DatingGame::ATT_OPTIMIST ||
			       m.prof.attitude == DatingGame::ATT_LONER ||
			       m.prof.attitude == DatingGame::ATT_NORMAL;
		}
		return true;
	}

	// the rooms' material: written like 1999 typed it
	const char* const CUPID_LOUNGE_IDLE[2][24] =
	{
		{ "anyone else bring a trauma kit on first dates or is that just me",
		  "my ideal evening: sunset, campfire, field-stripping something together",
		  "he said he liked my profile. i said i liked his muzzle velocity. silence since",
		  "romance is dead. i have seen the invoice",
		  "does VERIFIED mean anything here or did speck just like the photo",
		  "long walks on the beach. the beach is mined. keeps it interesting",
		  "my last relationship ended over a claymore. long story",
		  "if you can suppress a hallway you can hold a conversation. dm me",
		  "who else is only here for the cosmic chemistry column",
		  "the stars said proceed with caution. i proceeded anyway",
		  "34 / merc / anywhere with an airstrip",
		  "a/s/l? alive / scarred / classified",
		  "compliment their kit. works every time. worked once",
		  "brb, contract",
		  "my therapist says stop dating coworkers. we are ALL coworkers",
		  "profile says adventurous. means i have been legally dead twice",
		  "you havent lived until someone reloads for you without being asked",
		  "swiped past my own cousin today. small industry",
		  "i can cook. field rations count. they count",
		  "green flags: owns a whetstone. red flags: quotes their own kills",
		  "met someone from this site once. we still nod at checkpoints",
		  "my bunker has room for two. thats the whole ad",
		  "speck if you read this the algorithm gave me my ex",
		  "somebody winked at me mid-firefight. respect. terrible timing" },
		{ "bringt noch jemand ein erste-hilfe-set zum ersten date mit oder nur ich",
		  "mein traumabend: sonnenuntergang, lagerfeuer, gemeinsam etwas zerlegen",
		  "er mochte mein profil. ich mochte seine muendungsgeschwindigkeit. seitdem funkstille",
		  "die romantik ist tot. ich habe die rechnung gesehen",
		  "heisst GEPRUEFT hier irgendwas oder fand speck nur das foto gut",
		  "lange strandspaziergaenge. der strand ist vermint. haelt wach",
		  "meine letzte beziehung endete wegen einer claymore. lange geschichte",
		  "wer einen flur sichern kann, kann auch ein gespraech fuehren. dm an mich",
		  "wer ist noch nur wegen der kosmischen chemie hier",
		  "die sterne rieten zur vorsicht. ich hab trotzdem",
		  "34 / soeldner / ueberall mit landebahn",
		  "a/s/l? lebendig / vernarbt / geheim",
		  "lobt die ausruestung. klappt immer. hat einmal geklappt",
		  "brb, auftrag",
		  "mein therapeut sagt, keine kollegen daten. wir sind ALLE kollegen",
		  "im profil steht abenteuerlustig. heisst: zweimal amtlich tot gewesen",
		  "man hat nicht gelebt, bis jemand ungefragt fuer einen nachlaedt",
		  "heute am eigenen cousin vorbeigewischt. kleine branche",
		  "ich kann kochen. feldrationen zaehlen. sie zaehlen",
		  "gruene flaggen: eigener schleifstein. rote flaggen: zitiert eigene treffer",
		  "hab hier mal jemanden getroffen. wir nicken uns an checkpoints zu",
		  "mein bunker hat platz fuer zwei. das ist die ganze anzeige",
		  "speck, falls du das liest: der algorithmus gab mir meinen ex",
		  "jemand hat mir mitten im feuergefecht zugezwinkert. respekt. mieses timing" },
	};

	const char* const CUPID_LOUNGE_ROUGH[2][14] =
	{
		{ "won an argument with a bear once. looking for round two",
		  "my love language is suppressive fire",
		  "date idea: you, me, the sparring pit. loser buys dinner",
		  "gentle is for bandages",
		  "i dont want flowers i want a worthy opponent",
		  "left my last date in better shape than i found them. barely",
		  "HIT ME UP. i mean that in both ways",
		  "if you flinch we cant be friends",
		  "bring your own tourniquet. thats the date",
		  "roses have thorns. i respect that about them",
		  "arm wrestle me for the last word",
		  "my type: can carry me out of a hot zone. or at least drag",
		  "first one to apologize does the dishes forever",
		  "i said WHAT. i said it with love. loudly" },
		{ "hab mal einen streit mit einem baeren gewonnen. suche runde zwei",
		  "meine liebessprache ist sperrfeuer",
		  "date-idee: du, ich, die sparringsgrube. verlierer zahlt das essen",
		  "sanft ist was fuer verbaende",
		  "ich will keine blumen, ich will einen wuerdigen gegner",
		  "hab mein letztes date in besserem zustand hinterlassen als vorgefunden. knapp",
		  "SCHREIB MICH AN. in beiderlei hinsicht",
		  "wer zuckt, kann nicht mein freund sein",
		  "eigenes tourniquet mitbringen. das ist das date",
		  "rosen haben dornen. das respektiere ich an ihnen",
		  "armdruecken um das letzte wort",
		  "mein typ: kann mich aus der gefahrenzone tragen. oder wenigstens ziehen",
		  "wer sich zuerst entschuldigt, spuelt fuer immer",
		  "ich sagte WAS. mit liebe. laut" },
	};

	const char* const CUPID_LOUNGE_BALLAD[2][14] =
	{
		{ "i still write letters. on paper. with a pen i cleaned",
		  "somewhere out there is someone whose silences fit mine",
		  "candlelight is just controlled fire. i am very good with fire",
		  "i memorized a poem for this channel. nobody asked. it waits",
		  "slow is not weak. slow is aimed",
		  "my heart is a safehouse. the password is patience",
		  "tell me about your scars. take your time",
		  "the moon over grumm was beautiful tonight. that is all",
		  "i pressed a flower in my field manual. chapter 9. mines",
		  "someone here hums when they clean their rifle. marry me",
		  "wrote a haiku about extraction day. it does not rhyme. they dont",
		  "i keep the last letter. everyone keeps the last letter",
		  "dance with me at the safehouse. the floor creaks in waltz time",
		  "your watch ticks. mine ticks. somewhere they agree" },
		{ "ich schreibe noch briefe. auf papier. mit einem gereinigten stift",
		  "irgendwo da draussen passt jemandes schweigen zu meinem",
		  "kerzenlicht ist nur kontrolliertes feuer. ich bin sehr gut mit feuer",
		  "ich habe ein gedicht fuer diesen kanal auswendig gelernt. niemand fragte. es wartet",
		  "langsam ist nicht schwach. langsam ist gezielt",
		  "mein herz ist ein unterschlupf. das passwort ist geduld",
		  "erzaehl mir von deinen narben. lass dir zeit",
		  "der mond ueber grumm war heute schoen. das ist alles",
		  "ich habe eine blume im feldhandbuch gepresst. kapitel 9. minen",
		  "jemand hier summt beim waffenreinigen. heirate mich",
		  "hab ein haiku ueber den abzugstag geschrieben. es reimt sich nicht. die reimen nie",
		  "ich behalte den letzten brief. jeder behaelt den letzten brief",
		  "tanz mit mir im unterschlupf. der boden knarrt im walzertakt",
		  "deine uhr tickt. meine tickt. irgendwo sind sie sich einig" },
	};

	const char* const CUPID_LOUNGE_GOLDCHAT[2][10] =
	{
		{ "the champagne in here is real. speck invoices by the sip",
		  "gold members only. finally, quality people",
		  "my portfolio is diversified. my heart is not. one slot open",
		  "a toast: to those who paid for love and received ambience",
		  "someone in here smells like fresh contracts",
		  "we do not discuss the exchange rate in this room",
		  "the caviar is surplus but the intentions are premium",
		  "i tip the algorithm. it knows",
		  "yes the robe is complimentary. no you cannot keep it",
		  "quarterly earnings call, then candlelight. balance" },
		{ "der champagner hier ist echt. speck rechnet pro schluck ab",
		  "nur gold-mitglieder. endlich niveau",
		  "mein portfolio ist breit gestreut. mein herz nicht. ein platz frei",
		  "ein toast: auf alle, die fuer liebe zahlten und ambiente bekamen",
		  "hier riecht jemand nach frischen vertraegen",
		  "ueber den wechselkurs sprechen wir in diesem raum nicht",
		  "der kaviar ist ueberschuss, aber die absichten sind premium",
		  "ich gebe dem algorithmus trinkgeld. er weiss es",
		  "ja, der bademantel ist inklusive. nein, behalten geht nicht",
		  "erst quartalszahlen, dann kerzenlicht. balance" },
	};

	const char* const CUPID_LOUNGE_EMOTE[2][12] =
	{
		{ "polishes a scope, slowly",
		  "slides a ration bar across the channel",
		  "sighs in encrypted",
		  "checks the door again",
		  "blushes in olive drab",
		  "sharpens something off-screen",
		  "reloads, romantically",
		  "lights a cigarette on a thermite match",
		  "counts the exits, then smiles",
		  "folds a love letter into a paper plane. it flies badly",
		  "raises a canteen to the room",
		  "adjusts the claymore. FRONT TOWARD LOVE" },
		{ "poliert langsam ein zielfernrohr",
		  "schiebt einen verpflegungsriegel durch den kanal",
		  "seufzt verschluesselt",
		  "prueft nochmal die tuer",
		  "erroetet in olivgruen",
		  "schaerft etwas ausserhalb des bildes",
		  "laedt nach, romantisch",
		  "zuendet eine zigarette an einem thermitstab an",
		  "zaehlt die ausgaenge, laechelt dann",
		  "faltet einen liebesbrief zum papierflieger. er fliegt schlecht",
		  "hebt eine feldflasche auf den raum",
		  "richtet die claymore aus. VORDERSEITE ZUR LIEBE" },
	};

	const char* const CUPID_LOUNGE_SPECK[2][10] =
	{
		{ "friendly reminder: GOLD members appear 40% more desirable. this is science. ask me",
		  "keep it tasteful, people. this is a family establishment with a body count",
		  "todays special: retake the questionnaire, $25. become someone better",
		  "no refunds on broken hearts. site policy since tuesday",
		  "somebody ordered flowers today. THAT is the spirit. united floral. tell them speck sent you",
		  "lurkers: the room cannot see you, but the algorithm believes in you",
		  "our matching is I.M.P. licensed* (*pending. do not ask the institute)",
		  "the funeral home banner is a coincidence, not a warning",
		  "flowers say what mortars cannot. usually 'sorry'",
		  "every wink is a small investment in yourself. also in me" },
		{ "zur erinnerung: GOLD-mitglieder wirken 40% begehrenswerter. das ist wissenschaft. fragt mich",
		  "bleibt geschmackvoll, leute. das hier ist ein familienbetrieb mit opferzahlen",
		  "tagesangebot: fragebogen wiederholen, 25 $. werdet jemand besseres",
		  "keine rueckerstattung bei gebrochenen herzen. hausregel seit dienstag",
		  "jemand hat heute blumen bestellt. DAS ist der geist. united floral. sagt, speck schickt euch",
		  "an die stillen mitleser: der raum sieht euch nicht, aber der algorithmus glaubt an euch",
		  "unser matching ist I.M.P.-lizenziert* (*beantragt. fragt nicht beim institut nach)",
		  "das banner vom bestattungsinstitut ist zufall, keine warnung",
		  "blumen sagen, was moerser nicht koennen. meistens 'tut mir leid'",
		  "jedes zwinkern ist eine kleine investition in euch selbst. und in mich" },
	};

	const char* const CUPID_LOUNGE_TOPIC[2][CPL_ROOMS] =
	{
		{ "topic for #tender: where the tough get tender. no live ordnance in the channel. -@speck",
		  "topic for #roughlove: consent first, helmets recommended. -@speck",
		  "topic for #ballads: slow hearts, long letters. no shouting. -@speck",
		  "topic for #goldhearts: you paid for this. enjoy responsibly. -@speck" },
		{ "topic fuer #tender: wo die harten zaertlich werden. keine scharfe munition im kanal. -@speck",
		  "topic fuer #roughlove: erst einwilligung, helm empfohlen. -@speck",
		  "topic fuer #ballads: langsame herzen, lange briefe. nicht schreien. -@speck",
		  "topic fuer #goldhearts: sie haben dafuer bezahlt. geniessen sie verantwortungsvoll. -@speck" },
	};

	const char* const CUPID_LOUNGE_EXQ[2][7] =
	{
		{ "serious question. flowers or ammunition",
		  "what counts as a first date here",
		  "does anyone actually meet up irl",
		  "whats your dealbreaker",
		  "whats a green flag then",
		  "long distance: yes or no",
		  "kids someday?" },
		{ "ernste frage. blumen oder munition",
		  "was zaehlt hier als erstes date",
		  "trifft sich hier irgendwer wirklich im rl",
		  "was ist euer ausschlusskriterium",
		  "und was ist dann eine gruene flagge",
		  "fernbeziehung: ja oder nein",
		  "irgendwann kinder?" },
	};
	const char* const CUPID_LOUNGE_EXA[2][7] =
	{
		{ "why is this a choice",
		  "an extraction counts if you hold hands",
		  "define irl. define meet. define anyone",
		  "snoring. also war crimes",
		  "carries two tourniquets. one is for you",
		  "define distance. i have air support",
		  "i can barely keep a plant alive. the plant is plastic" },
		{ "wieso ist das eine entweder-oder-frage",
		  "eine evakuierung zaehlt, wenn man haendchen haelt",
		  "definiere rl. definiere treffen. definiere irgendwer",
		  "schnarchen. und kriegsverbrechen",
		  "hat zwei tourniquets dabei. eins davon fuer dich",
		  "definiere entfernung. ich habe luftunterstuetzung",
		  "ich halte kaum eine pflanze am leben. die pflanze ist aus plastik" },
	};

	const char* const CUPID_LOUNGE_SAY[2][6] =
	{
		{ "hi room",
		  "so. anyone here come to this channel often",
		  "nice topic",
		  "im new. be gentle",
		  "the topic is right. we are tough. we are tender",
		  "lurked for an hour. good room" },
		{ "hallo raum",
		  "und. kommt hier jemand oefter in diesen kanal",
		  "schoenes topic",
		  "bin neu. seid sanft",
		  "das topic stimmt. wir sind hart. wir sind zaertlich",
		  "eine stunde still mitgelesen. guter raum" },
	};
	const char* const CUPID_LOUNGE_REPLY[2][7] =
	{
		{ "a wild {} appears",
		  "hello {}. state your caliber",
		  "{} finally speaks. the lurker walks among us",
		  "hi {}. read your profile. bold of you",
		  "{}!! fresh mea- i mean. welcome",
		  "{} said something nice. screenshot it",
		  "welcome {}. mind the claymore in the corner" },
		{ "ein wildes {} erscheint",
		  "hallo {}. kaliber angeben bitte",
		  "{} spricht. der stille mitleser wandelt unter uns",
		  "hi {}. hab dein profil gelesen. mutig",
		  "{}!! frischflei- ich meine. willkommen",
		  "{} hat etwas nettes gesagt. macht einen screenshot",
		  "willkommen {}. vorsicht mit der claymore in der ecke" },
	};

	// what this particular temperament would actually type - four lines
	// per canon attitude, spoken only by members who carry it
	const char* const CUPID_LOUNGE_PERSONA[2][10][4] =
	{
		{
		  { "im normal. profile says so. twice",
		    "looking for something steady. steady hands a plus",
		    "no drama please. i get enough at work",
		    "i like my coffee and my extractions clean" },
		  { "free hugs at the safehouse. helmet optional",
		    "i just think everyone here deserves somebody. even the snipers",
		    "made cookies for the whole squad once. no regrets. some regrets",
		    "your smile could disarm me. thats a compliment AND a warning" },
		  { "i work alone. dating seems like a two person job. hence the problem",
		    "my ideal date leaves me alone together",
		    "typing this from a rooftop. good signal. no neighbors",
		    "if i reply within a week, thats keen" },
		  { "todays the day. i can feel it. felt it yesterday too",
		    "every miss is just a hit that hasnt happened yet",
		    "this site WORKS people. flo proved it",
		    "cloudy with a chance of romance. bring a poncho" },
		  { "this will end badly. dm me anyway",
		    "love is a minefield. i say that professionally",
		    "my glass is half empty and probably poisoned",
		    "expect nothing. thats my secret. im never disappointed" },
		  { "i type in caps because i CARE",
		    "fastest reply in the room. try me",
		    "i dont do slow burns. i do burns",
		    "my patience died in a firefight. what about yours" },
		  { "youre all lucky im even logged in",
		    "my profile photo doesnt do me justice. nothing does",
		    "i dont match with people. people match with me",
		    "yes thats my real kill count. the modest one" },
		  { "just closed a six figure contract. anyway hi",
		    "i know the owner. speck owes me, actually",
		    "my other profile is on a premium site",
		    "villa in san mona. helicopter pad pending" },
		  { "rating profiles 1-10 in my head. youre welcome",
		    "im not rude. im efficient. same thing to you people",
		    "someone winked at me. blocked. dont be desperate",
		    "this room was better before all of you joined" },
		  { "is it safe to post here. asking first",
		    "i like someone. no i wont say who. or wave. or breathe",
		    "ran from my last date. literally. she understood. i hope",
		    "bravery is overrated. alive is underrated" },
		},
		{
		  { "ich bin normal. steht so im profil. zweimal",
		    "suche etwas bestaendiges. ruhige haende ein plus",
		    "bitte kein drama. davon hab ich auf der arbeit genug",
		    "ich mag meinen kaffee und meine evakuierungen sauber" },
		  { "gratis umarmungen im unterschlupf. helm optional",
		    "ich finde einfach, hier verdient jeder jemanden. sogar die scharfschuetzen",
		    "hab mal kekse fuer den ganzen trupp gebacken. keine reue. etwas reue",
		    "dein laecheln koennte mich entwaffnen. das ist kompliment UND warnung" },
		  { "ich arbeite allein. daten wirkt wie ein job fuer zwei. daher das problem",
		    "mein traumdate laesst mich gemeinsam in ruhe",
		    "schreibe das von einem dach. guter empfang. keine nachbarn",
		    "wenn ich innerhalb einer woche antworte, ist das stuermisch" },
		  { "heute ist der tag. ich spuere es. gestern auch gespuert",
		    "jeder fehlschuss ist nur ein treffer, der noch nicht passiert ist",
		    "diese seite FUNKTIONIERT leute. flo hat es bewiesen",
		    "bewoelkt mit aussicht auf romantik. poncho mitbringen" },
		  { "das endet schlecht. schreibt mir trotzdem",
		    "liebe ist ein minenfeld. ich sage das beruflich",
		    "mein glas ist halb leer und vermutlich vergiftet",
		    "nichts erwarten. das ist mein geheimnis. ich werde nie enttaeuscht" },
		  { "ich schreibe in grossbuchstaben, weil es mir WICHTIG ist",
		    "schnellste antwort im raum. probier mich",
		    "ich mach keine langsamen flammen. ich mach verbrennungen",
		    "meine geduld fiel im feuergefecht. und deine" },
		  { "ihr koennt froh sein, dass ich ueberhaupt eingeloggt bin",
		    "mein profilfoto wird mir nicht gerecht. nichts wird das",
		    "ich matche nicht mit leuten. leute matchen mit mir",
		    "ja, das ist meine echte trefferzahl. die bescheidene" },
		  { "gerade einen sechsstelligen vertrag abgeschlossen. wie auch immer, hi",
		    "ich kenne den besitzer. speck schuldet MIR was",
		    "mein anderes profil liegt auf einer premium-seite",
		    "villa in san mona. hubschrauberlandeplatz beantragt" },
		  { "ich bewerte eure profile im kopf von 1-10. gern geschehen",
		    "ich bin nicht unhoeflich. ich bin effizient. fuer euch dasselbe",
		    "jemand hat mir zugezwinkert. blockiert. nicht so verzweifelt sein",
		    "der raum war besser, bevor ihr alle beigetreten seid" },
		  { "ist es sicher, hier zu posten. frage vorsichtshalber",
		    "ich mag jemanden. nein, ich sage nicht wen. winke auch nicht. atme kaum",
		    "bin vom letzten date weggerannt. woertlich. sie hatte verstaendnis. hoffe ich",
		    "mut wird ueberschaetzt. lebendig wird unterschaetzt" },
		},
	};

	// the trait that overrides everything else about a person
	const char* const CUPID_LOUNGE_PSYCHO[2][3] =
	{
		{ "i named all my knives after exes. running out of exes",
		  "my love is like a frag pattern. wide. indiscriminate",
		  "the voices say youre cute" },
		{ "ich habe alle meine messer nach exen benannt. die exen gehen aus",
		  "meine liebe ist wie ein splitterkegel. breit. wahllos",
		  "die stimmen sagen, du bist suess" },
	};

	// the one-word school of correspondence
	const char* const CUPID_LOUNGE_SHORT[2][12] =
	{
		{ "lol", "same", "mood", "no", "ok but no", "hard agree",
		  "brb", "this room lol", "real", "yes.", "hm", "felt that" },
		{ "lol", "same", "mood", "nein", "ok aber nein", "voll dafuer",
		  "brb", "dieser raum lol", "real", "ja.", "hm", "kenn ich" },
	};

	// and the memoirists: long, lore-heavy, typed with both hands
	const char* const CUPID_LOUNGE_LORE[2][6] =
	{
		{ "my grandmother worked the cambria mines before the queen closed "
		  "them. she said love is like the head-frame elevator: go slow, "
		  "hold the rail, never trust the man who owns it. anyway. thats "
		  "why i vet",
		  "i was in omerta the week it started. somebody kept playing a "
		  "wedding song on the radio between the announcements. i think "
		  "about that more than i think about the announcements",
		  "metavira was supposed to be about the trees. everyone i know "
		  "came back talking about one nurse. make of that what you will",
		  "deidranna banned this site last spring. speck mirrored it on six "
		  "servers over a weekend. say what you want about the man, that "
		  "is commitment to commerce",
		  "my father sold ice cream in san mona before the casinos came. "
		  "he said every heart is a tourist. i did not understand him "
		  "until my third contract",
		  "the drassen tower plays love songs after midnight if you know "
		  "the frequency. i am not giving out the frequency. earn it" },
		{ "meine grossmutter arbeitete in den minen von cambria, bevor die "
		  "koenigin sie schloss. sie sagte, liebe ist wie der foerderkorb: "
		  "langsam fahren, festhalten, und trau nie dem mann, dem er "
		  "gehoert. deshalb pruefe ich",
		  "ich war in omerta, in der woche, als es losging. jemand spielte "
		  "im radio zwischen den durchsagen immer ein hochzeitslied. daran "
		  "denke ich oefter als an die durchsagen",
		  "bei metavira sollte es um die baeume gehen. alle, die ich kenne, "
		  "kamen zurueck und redeten von einer krankenschwester. macht "
		  "daraus, was ihr wollt",
		  "deidranna hat die seite letztes fruehjahr verboten. speck hat "
		  "sie uebers wochenende auf sechs server gespiegelt. sagt ueber "
		  "den mann, was ihr wollt: das ist hingabe zum geschaeft",
		  "mein vater verkaufte eis in san mona, bevor die casinos kamen. "
		  "er sagte, jedes herz ist ein tourist. verstanden habe ich ihn "
		  "erst beim dritten vertrag",
		  "der tower von drassen spielt nach mitternacht liebeslieder, "
		  "wenn man die frequenz kennt. ich verrate die frequenz nicht. "
		  "verdient sie euch" },
	};

	// the neighbourhood comes up: the parlour, the chess site
	const char* const CUPID_LOUNGE_CROSS[2][8] =
	{
		{ "took a date to the mahjong parlour. lost the date to the ladder. worth it",
		  "anyone else's chach.com rating better than their match rate here",
		  "the parlour's back room has better lighting than my profile photo",
		  "played chess with someone from #ballads. they resigned. romantically",
		  "if we match, first date is mahjong at san mona. house rules",
		  "K. from the parlour says hi. K. says a lot of things",
		  "my chach.com coach says i blunder queens and relationships the same way",
		  "speck, the chess site has a guestbook. where is OUR guestbook" },
		{ "date in den mahjong-salon mitgenommen. date an die rangliste verloren. hat sich gelohnt",
		  "ist noch bei wem die chach.com-wertung besser als die trefferquote hier",
		  "das hinterzimmer im salon hat besseres licht als mein profilfoto",
		  "mit jemandem aus #ballads schach gespielt. aufgegeben. romantisch",
		  "wenn wir matchen: erstes date mahjong in san mona. hausregeln",
		  "K. aus dem salon laesst gruessen. K. laesst vieles",
		  "mein chach.com-coach sagt, ich verspiele damen und beziehungen gleich",
		  "speck, die schachseite hat ein gaestebuch. wo ist UNSERES" },
	};

	// what each temperament cannot stand, for the rail dossier
	const char* const CUPID_DISLIKE[2][10] =
	{
		{ "surprises, paperwork errors", "cold shoulders, empty mess halls",
		  "crowds, follow-up questions", "quitters, cloudy forecasts",
		  "hope, and being right about it", "waiting, whispering",
		  "competition, per se", "economy class, small talk",
		  "most of you, honestly", "loud noises, brave plans" },
		{ "ueberraschungen, formularfehler",
		  "kalte schultern, leere kantinen",
		  "menschenmengen, nachfragen", "aufgeber, schlechte prognosen",
		  "hoffnung, und recht zu behalten", "warten, gefluester",
		  "konkurrenz, an sich", "economy class, smalltalk",
		  "die meisten von euch, ehrlich", "laute geraeusche, mutige plaene" },
	};

	// IRC handles: the nickname, lower-cased, in one of the era's costumes
	ST::string LoungeNick(int who)
	{
		if (who == LNG_SYS)   return "***";
		if (who == LNG_SPECK) return "@speck";
		ST::string base;
		ProfileID pid = 0;
		if (who == LNG_YOU)
		{
			if (LaptopSaveInfo.fIMPCompletedFlag)
			{
				pid  = PlayerImpPid();
				base = GetProfile(pid).zNickname;
			}
			else
			{
				return ST::format("guest{}", 2000 + int(GetWorldDay()) * 41 % 800);
			}
		}
		else
		{
			if (who < 0 || who >= int(gCupidRoster.size())) return "someone";
			pid  = gCupidRoster[size_t(who)].pid;
			base = GetProfile(pid).zNickname;
		}
		std::string low = base.to_std_string();
		for (char& c : low)
		{
			if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
			else if (c == ' ') c = '_';
		}
		switch (pid % 5)
		{
			case 1:  return ST::format("{}2000", low.c_str());
			case 2:  return ST::format("{}{}", low.c_str(), 61 + pid % 38);
			case 3:  return ST::format("xX{}Xx", low.c_str());
			case 4:  return ST::format("{}^", low.c_str());
			default: return ST::string(low);
		}
	}

	// the mugshot beside a cluster of lines, exactly like the parlour bar
	SGPVObject* LoungeFace(INT8 who)
	{
		if (who >= 0 && who < INT8(gCupidFaces33.size()))
		{
			return gCupidFaces33[size_t(who)];
		}
		if (who == LNG_YOU) return guiCupidSelf;
		return nullptr; // speck and the system carry their own colours
	}

	// a line lands in the current room, pre-wrapped into fixed rows the
	// way the parlour bar does it; the reader's place survives the growth
	void LoungePush(INT8 who, const ST::string& text)
	{
		LoungeRoom& R = Room();
		INT32 budget = CPL_TEXT_W;
		INT32 cap = budget;
		if (who != LNG_SYS)
		{
			cap -= StringPixLength(ST::format("{}:", LoungeNick(who)),
					FONT10ARIALBOLD) + 4;
		}
		std::vector<ST::string> rows;
		for (auto const& para : text.split('\n'))
		{
			ST::string cur;
			for (auto const& word : para.split(' '))
			{
				const ST::string cand =
					cur.empty() ? word : cur + " " + word;
				if (!cur.empty() &&
				    StringPixLength(cand, FONT10ARIAL) > cap)
				{
					rows.push_back(cur);
					cur = word;
					cap = budget; // continuations get the full column
				}
				else cur = cand;
			}
			rows.push_back(cur);
			cap = budget;
		}
		// no widows: a lone word on the last row pulls company down
		if (rows.size() >= 2 &&
		    rows.back().to_std_string().find(' ') == std::string::npos)
		{
			std::string prev = rows[rows.size() - 2].to_std_string();
			const size_t cut = prev.rfind(' ');
			if (cut != std::string::npos && cut > 0)
			{
				rows.back() = ST::format("{} {}",
						prev.substr(cut + 1).c_str(), rows.back());
				rows[rows.size() - 2] = ST::string(prev.substr(0, cut));
			}
		}
		for (const ST::string& row : rows)
		{
			R.log.push_back(LoungeLine{ who, row });
		}
		while (R.log.size() > 96) R.log.erase(R.log.begin());
		if (R.scroll > 0)
		{
			R.scroll += INT32(rows.size()) * CPL_ROW_H;
		}
		if (gCupidPage == CPP_LOUNGE) CupidRedraw();
	}

	// the mini avatar chip: square-cropped and stretch-blitted to 14px,
	// exactly the parlour recipe. Tokens for the voices without faces.
	void LoungeChip(INT8 who, INT32 x, INT32 y)
	{
		SGPVSurface* surf = nullptr;
		if (who >= 0 && who < INT8(gCupidChipSurf.size()))
		{
			surf = gCupidChipSurf[size_t(who)];
		}
		else if (who == LNG_YOU) surf = guiCupidSelfChip;
		if (surf)
		{
			FillRect(x - 1, y - 1, 16, 16, CP_RGB_CARD_DIM);
			SGPBox const srcBox = { 8, 2, 32, 32 };
			SGPBox const dstBox = { UINT16(CP_X(x)), UINT16(CP_Y(y)),
						14, 14 };
			BltStretchVideoSurface(FRAME_BUFFER, surf, &srcBox, &dstBox);
			return;
		}
		FillRect(x - 1, y - 1, 16, 16, CP_RGB_INK);
		if (who == LNG_SPECK)
		{
			// the operator wears the house mark, in gold
			DrawHeart(x + 3, y + 4, 1, CP_RGB_GOLD);
		}
		else
		{
			FillRect(x + 3, y + 3, 8, 8, CP_RGB_BLUE_DK);
		}
	}

	// somebody has to sit down and type it first
	void LoungeSay(INT8 who, const ST::string& text, UINT32 lead = 0)
	{
		LoungeRoom& R = Room();
		const UINT32 now = GetJA2Clock();
		UINT32 due = std::max(now, std::min(R.tail, now + 9000));
		due += lead;
		due += who == LNG_SYS ? 250
			: who == LNG_SPECK ? 500 + UINT32(text.size()) * 18
			: 700 + LoungeRoll() % 900 + UINT32(text.size()) * 55;
		R.pend.push_back(LoungePending{ who, text, due });
		R.tail = due;
	}

	// nobody repeats the room's recent material - silence beats reruns
	bool LoungeFresh(const ST::string& text)
	{
		for (const ST::string& prev : Room().recent)
		{
			if (prev == text) return false;
		}
		return true;
	}

	void LoungeRemember(const ST::string& text)
	{
		LoungeRoom& R = Room();
		R.recent.push_back(text);
		while (R.recent.size() > 24) R.recent.erase(R.recent.begin());
	}

	// pick from a bank, rolling past anything the room said recently
	const char* LoungePick(const char* const* bank, int n)
	{
		for (int t = 0; t < 8; ++t)
		{
			const char* const c = bank[LoungeRoll() % n];
			if (LoungeFresh(c))
			{
				LoungeRemember(c);
				return c;
			}
		}
		return nullptr; // the room has heard it all; it says nothing
	}

	// one beat of room life; instant lines skip the typing theatre
	void LoungeBeat(bool instant)
	{
		std::vector<int> cast;
		for (int i = 0; i < int(gCupidRoster.size()); ++i)
		{
			const Member& m = gCupidRoster[size_t(i)];
			if (!m.locked && !MemberIsDead(m.pid) &&
			    RoomAdmits(m, giCupidRoom))
			{
				cast.push_back(i);
			}
		}
		const int lang = gfCupidGerman ? 1 : 0;
		if (cast.empty())
		{
			LoungePush(LNG_SYS, gfCupidGerman ? "der kanal ist still."
							  : "the channel is quiet.");
			return;
		}
		const INT8 who = INT8(cast[LoungeRoll() % cast.size()]);
		const UINT32 r = LoungeRoll() % 100;
		auto emit = [&](INT8 w, const ST::string& t)
		{
			if (instant) LoungePush(w, t);
			else         LoungeSay(w, t);
		};
		if (r < 8)
		{
			static const char* const exits[2][3] =
			{
				{ "(ping timeout)", "(connection reset by peace)",
				  "(gone to reload)" },
				{ "(ping timeout)", "(verbindung von frieden getrennt)",
				  "(nachladen gegangen)" },
			};
			const bool join = LoungeRoll() % 2 == 0;
			const ST::string notice = join
				? ST::format(gfCupidGerman ? "{} hat {} betreten"
							   : "{} has joined {}",
						LoungeNick(who), CPL_NAME[giCupidRoom])
				: ST::format(gfCupidGerman ? "{} hat {} verlassen {}"
							   : "{} has left {} {}",
						LoungeNick(who), CPL_NAME[giCupidRoom],
						exits[lang][LoungeRoll() % 3]);
			if (LoungeFresh(notice))
			{
				LoungeRemember(notice);
				emit(LNG_SYS, notice);
			}
		}
		else if (r < 17)
		{
			if (const char* const say =
					LoungePick(CUPID_LOUNGE_SPECK[lang], 10))
			{
				emit(LNG_SPECK, say);
			}
		}
		else if (r < 27)
		{
			if (const char* const act =
					LoungePick(CUPID_LOUNGE_EMOTE[lang], 12))
			{
				emit(LNG_SYS, ST::format("* {} {}", LoungeNick(who),
						act));
			}
		}
		else if (r < 36 && cast.size() > 1 && giCupidRoom == 0)
		{
			const int q = int(LoungeRoll() % 7);
			if (!LoungeFresh(CUPID_LOUNGE_EXQ[lang][q])) return;
			LoungeRemember(CUPID_LOUNGE_EXQ[lang][q]);
			LoungeRemember(CUPID_LOUNGE_EXA[lang][q]);
			INT8 other = INT8(cast[LoungeRoll() % cast.size()]);
			if (other == who)
			{
				other = INT8(cast[(LoungeRoll() + 1) % cast.size()]);
			}
			emit(who, CUPID_LOUNGE_EXQ[lang][q]);
			if (instant) LoungePush(other, CUPID_LOUNGE_EXA[lang][q]);
			else         LoungeSay(other, CUPID_LOUNGE_EXA[lang][q], 600);
		}
		else
		{
			// the line belongs to the person: reactions and memoirs
			// aside, what gets said is what THIS temperament would say
			const Member& sm = gCupidRoster[size_t(who)];
			int att = sm.prof.attitude;
			if (att < 0 || att > 9) att = 0;
			const char* say = nullptr;
			const UINT32 style = LoungeRoll() % 100;
			if (style < 14)
			{
				// the one-word school
				say = LoungePick(CUPID_LOUNGE_SHORT[lang], 12);
			}
			else if (style < 22)
			{
				// the memoirists
				say = LoungePick(CUPID_LOUNGE_LORE[lang], 6);
			}
			else if (style < 30)
			{
				// the neighbourhood
				say = LoungePick(CUPID_LOUNGE_CROSS[lang], 8);
			}
			else if (sm.prof.trait == DatingGame::TRAIT_PSYCHO &&
				 style < 48)
			{
				say = LoungePick(CUPID_LOUNGE_PSYCHO[lang], 3);
			}
			else if (style < 66)
			{
				say = LoungePick(CUPID_LOUNGE_PERSONA[lang][att], 4);
			}
			if (!say)
			{
				switch (giCupidRoom)
				{
					case 1:
						say = LoungePick(CUPID_LOUNGE_ROUGH[lang], 14);
						break;
					case 2:
						say = LoungePick(CUPID_LOUNGE_BALLAD[lang], 14);
						break;
					case 3:
						say = LoungePick(CUPID_LOUNGE_GOLDCHAT[lang],
								10);
						break;
					default:
						say = LoungePick(CUPID_LOUNGE_IDLE[lang], 24);
						break;
				}
			}
			if (say) emit(who, say);
		}
	}

	// opening a room: first visit gets the topic and a scrollback, a
	// return visit gets whatever happened while you were elsewhere
	void LoungeEnterRoom()
	{
		if (RoomLocked()) return;
		LoungeRoom& R = Room();
		const UINT32 now = GetJA2Clock();
		const int lang = gfCupidGerman ? 1 : 0;
		if (!R.seeded)
		{
			R.seeded = true;
			if (guiCupidLoungeRoll == 1)
			{
				guiCupidLoungeRoll = (now | 1) + GetWorldDay() * 977;
			}
			LoungePush(LNG_SYS, CUPID_LOUNGE_TOPIC[lang][giCupidRoom]);
			if (!PlayerHasProfile())
			{
				LoungePush(LNG_SYS, ST::format(gfCupidGerman
					? "sie lesen still mit als {}. der raum sieht sie nicht."
					: "you are lurking as {}. the room cannot see you.",
					LoungeNick(LNG_YOU)));
			}
			const int seedLines = 5 + int(LoungeRoll() % 4);
			for (int i = 0; i < seedLines; ++i) LoungeBeat(true);
			if (giCupidRoom == 0)
			{
				// the one obituary, and the one success story
				if (!gfCupidLoungeMourned)
				{
					for (const Member& m : gCupidRoster)
					{
						if (m.locked || !MemberIsDead(m.pid)) continue;
						gfCupidLoungeMourned = true;
						LoungePush(LNG_SYS, ST::format(gfCupidGerman
							? "{}s nick bleibt fuer immer registriert. "
							  "der kanal dimmt sein topic."
							: "{}'s nick stays registered forever. "
							  "the channel dims its topic.",
							LoungeNick(RosterIndexOf(m.pid))));
						break;
					}
				}
				if (gubFact[FACT_PC_MARRYING_DARYL_IS_FLO])
				{
					LoungePush(LNG_SYS, gfCupidGerman
						? "flo && daryl h. - die erste hochzeit des kanals. "
						  "speck hat geweint (in rechnung gestellt)."
						: "flo && daryl h. - the channel's first wedding. "
						  "speck cried (invoiced).");
				}
			}
		}
		else if (now > R.seen + 30000)
		{
			const int missed = 2 + int(LoungeRoll() % 2);
			for (int i = 0; i < missed; ++i) LoungeBeat(true);
		}
		R.seen = now;
		R.next = now + 2500 + LoungeRoll() % 3000;
	}

	void LoungeEnter() { LoungeEnterRoom(); }

	// you spoke; the room notices the new voice. Typed words go out as
	// written; the button with nothing typed falls back to the classics
	void LoungeSpeak()
	{
		const UINT32 now = GetJA2Clock();
		const int lang = gfCupidGerman ? 1 : 0;
		ST::string said;
		while (!gCupidLoungeInput.empty() && gCupidLoungeInput.back() == ' ')
		{
			gCupidLoungeInput.pop_back();
		}
		if (!gCupidLoungeInput.empty())
		{
			said = ST::string(gCupidLoungeInput);
			gCupidLoungeInput.clear();
		}
		else
		{
			if (now < guiCupidLoungeSaid + 6000) return;
			const char* const mine =
				LoungePick(CUPID_LOUNGE_SAY[lang], 6);
			said = mine ? mine : CUPID_LOUNGE_SAY[lang][0];
		}
		LoungePush(LNG_YOU, said);
		// the room does not reply to every single thing you say
		if (now < guiCupidLoungeSaid + 6000) return;
		guiCupidLoungeSaid = now;
		std::vector<int> cast;
		for (int i = 0; i < int(gCupidRoster.size()); ++i)
		{
			const Member& m = gCupidRoster[size_t(i)];
			if (!m.locked && !MemberIsDead(m.pid) &&
			    RoomAdmits(m, giCupidRoom))
			{
				cast.push_back(i);
			}
		}
		if (!cast.empty())
		{
			const char* const tpl =
				LoungePick(CUPID_LOUNGE_REPLY[lang], 7);
			LoungeSay(INT8(cast[LoungeRoll() % cast.size()]),
					ST::format(tpl ? tpl : CUPID_LOUNGE_REPLY[lang][0],
							LoungeNick(LNG_YOU)));
		}
	}

	// --- the private line: one match, one window, no audience --------------
	struct ChatMsg
	{
		INT8 who; // LNG_YOU, LNG_SYS, or a roster index
		std::vector<ST::string> rows;
	};
	struct ChatThread
	{
		std::vector<ChatMsg>       log;
		std::vector<LoungePending> pend;
		UINT32 tail   = 0;
		INT32  scroll = 0;
		INT32  scrollMax = 0;
		bool   seeded = false;
		INT8   mood   = 0;  // PARRY's trick: state colours the voice
		UINT32 idleAt = 0;  // when the silence gets noticed
		std::vector<ST::string> recent;
	};
	std::vector<std::pair<ProfileID, ChatThread>> gCupidChats;
	ProfileID   gCupidChatPid = 0;
	std::string gCupidChatInput;
	UINT32      guiCupidChatSaid = 0;

	ChatThread& Chat()
	{
		for (auto& e : gCupidChats)
		{
			if (e.first == gCupidChatPid) return e.second;
		}
		gCupidChats.emplace_back(gCupidChatPid, ChatThread{});
		return gCupidChats.back().second;
	}

	// what a match says in private: openers when the line connects,
	// replies when you type. The lounge persona banks join in.
	const char* const CUPID_DM_OPEN[2][6] =
	{
		{ "hey. saw the kiss. nice aim.",
		  "so this is the private line. cozy.",
		  "hi. i don't do this often. the site says i do.",
		  "you kissed first. bold. i respect bold.",
		  "the algorithm said we'd get along. prove it wrong.",
		  "hey you. made it past the deck, huh." },
		{ "hey. den kuss gesehen. gut gezielt.",
		  "das ist also die private leitung. gemuetlich.",
		  "hi. ich mach das selten. die seite behauptet was anderes.",
		  "du hast zuerst gekuesst. mutig. respekt.",
		  "der algorithmus meint, wir passen. beweis das gegenteil.",
		  "hey du. durchs deck geschafft, hm." },
	};
	const char* const CUPID_DM_REPLY[2][12] =
	{
		{ "ha. ok, that's fair.",
		  "you type fast for a professional.",
		  "noted. filed. possibly framed.",
		  "don't tell the channel i laughed.",
		  "keep talking. i'm reloading.",
		  "that almost sounded romantic. careful.",
		  "hm. my contract says i can't answer that.",
		  "you're better at this than your photo suggested.",
		  "i've heard worse. mostly from speck.",
		  "flowers would say that better. just saying.",
		  "same time tomorrow? rates go up on weekends.",
		  "ok, that one got me." },
		{ "ha. ok, fair.",
		  "du tippst schnell fuer einen profi.",
		  "notiert. abgelegt. eventuell gerahmt.",
		  "verrat dem kanal nicht, dass ich gelacht habe.",
		  "red weiter. ich lade nach.",
		  "das klang fast romantisch. vorsicht.",
		  "hm. mein vertrag verbietet mir die antwort.",
		  "du bist besser, als dein foto vermuten liess.",
		  "ich hab schlimmeres gehoert. meist von speck.",
		  "blumen wuerden das schoener sagen. nur so.",
		  "morgen selbe zeit? am wochenende steigen die saetze.",
		  "ok, der sass." },
	};

	// one message becomes one bubble: wrap to the bubble column
	#define CPC_BUBBLE_W 224
	void ChatPush(INT8 who, const ST::string& text)
	{
		ChatThread& C = Chat();
		const INT32 cap = who == LNG_SYS ? CPL_TEXT_W : CPC_BUBBLE_W;
		ChatMsg msg;
		msg.who = who;
		for (auto const& para : text.split('\n'))
		{
			ST::string cur;
			for (auto const& word : para.split(' '))
			{
				const ST::string cand =
					cur.empty() ? word : cur + " " + word;
				if (!cur.empty() &&
				    StringPixLength(cand, FONT10ARIAL) > cap)
				{
					msg.rows.push_back(cur);
					cur = word;
				}
				else cur = cand;
			}
			if (!cur.empty()) msg.rows.push_back(cur);
		}
		if (msg.rows.empty()) return;
		C.log.push_back(msg);
		C.scroll = 0; // a new message pins the window to the newest
		C.idleAt = GetJA2Clock() + 25000 + LoungeRoll() % 20000;
		if (gCupidPage == CPP_CHAT) CupidRedraw();
	}

	// they type at human speed; the line delivers when it delivers
	void ChatSay(INT8 who, const ST::string& text, UINT32 lead = 0)
	{
		ChatThread& C = Chat();
		const UINT32 now = GetJA2Clock();
		UINT32 due = std::max(now, std::min(C.tail, now + 9000)) + lead;
		due += who == LNG_SYS ? 250
			: 900 + LoungeRoll() % 1100 + UINT32(text.size()) * 55;
		C.pend.push_back(LoungePending{ who, text, due });
		C.tail = due;
	}

	// pick from a bank, skipping what this thread said recently
	const char* ChatPick(const char* const* bank, int n)
	{
		ChatThread& C = Chat();
		for (int t = 0; t < 8; ++t)
		{
			const char* const c = bank[LoungeRoll() % n];
			bool fresh = true;
			for (const ST::string& prev : C.recent)
			{
				if (prev == c) { fresh = false; break; }
			}
			if (!fresh) continue;
			C.recent.push_back(c);
			while (C.recent.size() > 10) C.recent.erase(C.recent.begin());
			return c;
		}
		return bank[0];
	}

	void OpenChat(ProfileID pid)
	{
		gCupidChatPid = pid;
		gCupidPage = CPP_CHAT;
		gCupidChatInput.clear();
		ChatThread& C = Chat();
		if (!C.seeded)
		{
			C.seeded = true;
			const int lang = gfCupidGerman ? 1 : 0;
			ChatPush(LNG_SYS, T(CPS_CHAT_PRIVATE_SYS));
			const int idx = RosterIndexOf(pid);
			if (idx >= 0 && MemberIsDead(pid))
			{
				ChatPush(LNG_SYS, gfCupidGerman
						? "diese leitung ist still geworden."
						: "this line has gone quiet.");
			}
			else if (idx >= 0)
			{
				if (BitGet(gCupidPersist.ubProposed, pid))
				{
					ChatSay(INT8(idx), lang
						? "sie machten einen antrag vor dem hallo. "
						  "mutig. jetzt sagen sie hallo."
						: "you proposed before saying hello. bold. "
						  "now say hello.", 500);
				}
				else
				{
					ChatSay(INT8(idx),
							ChatPick(CUPID_DM_OPEN[lang], 6), 500);
				}
			}
		}
		CupidRedraw();
	}

	// the 1966 trick, licensed for 1999: ranked keyword banks, a mood
	// needle, and the game's own dossier data doing the "intelligence"
	bool ChatHas(const ST::string& in, const char* const* words, int n)
	{
		for (int i = 0; i < n; ++i)
		{
			if (in.find(words[i]) >= 0) return true;
		}
		return false;
	}

	ST::string ChatReplyFor(const ST::string& saidRaw, int idx)
	{
		ChatThread& C = Chat();
		const int lang = gfCupidGerman ? 1 : 0;
		const ST::string in = saidRaw.to_lower();
		const Member& m = gCupidRoster[size_t(idx)];
		MERCPROFILESTRUCT const& p = GetProfile(m.pid);
		int att = m.prof.attitude;
		if (att < 0 || att > 9) att = 0;

		// rank 1: names. A dossier is a list of grudges with a photo.
		for (int i = 0; i < 2; ++i)
		{
			const INT8 hated = p.bHated[i];
			if (hated < 0) continue;
			ST::string nick =
				ST::string(GetProfile(ProfileID(hated)).zNickname)
					.to_lower();
			if (in.find(nick) >= 0)
			{
				static const char* const T_[2][3] =
				{
					{ "{}. you said {}. this date is on thin ice.",
					  "my profile lists {} under dealbreakers. "
					  "it is not a joke.",
					  "we do not say {} on this line." },
					{ "{}. sie sagten {}. dieses date steht auf "
					  "duennem eis.",
					  "mein profil fuehrt {} unter dealbreakern. "
					  "das ist kein witz.",
					  "wir sagen {} nicht auf dieser leitung." },
				};
				if (C.mood > -3) --C.mood;
				return ST::format(ChatPick(T_[lang], 3), nick, nick);
			}
		}
		{
			ST::string speck = "speck";
			if (in.find(speck) >= 0)
			{
				static const char* const T_[2][4] =
				{
					{ "careful. he reads these. hi speck.",
					  "speck takes ten percent of everything. "
					  "possibly this conversation.",
					  "he cried at the last wedding. then invoiced "
					  "the tears.",
					  "speck is the only man who profits from my "
					  "love life. so far." },
					{ "vorsicht. er liest mit. hallo speck.",
					  "speck nimmt zehn prozent von allem. womoeglich "
					  "auch von diesem gespraech.",
					  "er hat bei der letzten hochzeit geweint. und "
					  "die traenen in rechnung gestellt.",
					  "speck ist der einzige, der an meinem "
					  "liebesleben verdient. bisher." },
				};
				return ChatPick(T_[lang], 4);
			}
		}

		// rank 2: the trade. Mercs relax when the talk turns technical.
		{
			static const char* const K[8] = { "kill", "gun", "war",
					"ammo", "knife", "fight", "shoot", "grenade" };
			if (ChatHas(in, K, 8))
			{
				static const char* const T_[2][4] =
				{
					{ "now you're speaking my language.",
					  "finally. shop talk. i was drowning in "
					  "feelings.",
					  "on a first chat? i like the confidence.",
					  "the site says 'kill' is a figure of speech. "
					  "the site has never met me." },
					{ "jetzt sprechen sie meine sprache.",
					  "endlich. fachgespraech. ich bin fast in "
					  "gefuehlen ertrunken.",
					  "beim ersten chat? mir gefaellt das "
					  "selbstbewusstsein.",
					  "laut seite ist 'toeten' nur eine redewendung. "
					  "die seite kennt mich nicht." },
				};
				if (C.mood < 3) ++C.mood;
				return ChatPick(T_[lang], 4);
			}
		}

		// rank 3: romance, tuned by how far the paperwork has gone
		{
			static const char* const K[7] = { "love", "marry", "wedding",
					"kiss", "date", "heart", "romantic" };
			if (ChatHas(in, K, 7))
			{
				const bool ringOn =
					BitGet(gCupidPersist.ubProposed, m.pid);
				if (ringOn && IsMatched(m.pid))
				{
					static const char* const T_[2][3] =
					{
						{ "we're engaged. you may speak plainly.",
						  "the wedding registry is at bobby ray's. "
						  "obviously.",
						  "my mother asked about you. i said you "
						  "have steady hands." },
						{ "wir sind verlobt. sie duerfen klartext "
						  "reden.",
						  "die hochzeitsliste liegt bei bobby ray. "
						  "natuerlich.",
						  "meine mutter fragte nach ihnen. ich sagte, "
						  "sie haben ruhige haende." },
					};
					return ChatPick(T_[lang], 3);
				}
				static const char* const T_[2][4] =
				{
					{ "slow down. i've seen what rushing does to a "
					  "squad.",
					  "romance is logistics with better lighting.",
					  "keep talking like that and i'll have to "
					  "update my status.",
					  "i've been shot twice. this is scarier." },
					{ "langsam. ich weiss, was hetze mit einem trupp "
					  "macht.",
					  "romantik ist logistik mit besserem licht.",
					  "reden sie weiter so, und ich muss meinen "
					  "status aendern.",
					  "ich wurde zweimal angeschossen. das hier ist "
					  "schlimmer." },
				};
				if (C.mood < 3) ++C.mood;
				return ChatPick(T_[lang], 4);
			}
		}

		// rank 4: money. Everyone here is a professional.
		{
			static const char* const K[5] = { "money", "pay", "rate",
					"$", "rich" };
			if (ChatHas(in, K, 5))
			{
				static const char* const T_[2][3] =
				{
					{ "my day rate is classified. my dinner rate is "
					  "negotiable.",
					  "i date for free. everything else is invoiced.",
					  "speck already takes ten percent. don't give "
					  "him ideas." },
					{ "mein tagessatz ist geheim. mein essenssatz "
					  "ist verhandelbar.",
					  "dates sind gratis. alles andere wird "
					  "abgerechnet.",
					  "speck nimmt schon zehn prozent. bringen sie "
					  "ihn nicht auf ideen." },
				};
				return ChatPick(T_[lang], 3);
			}
		}

		// rank 5: flowers. The funnel is always open.
		{
			static const char* const K[3] = { "flower", "rose", "blume" };
			if (ChatHas(in, K, 3))
			{
				static const char* const T_[2][2] =
				{
					{ "talk is cheap. united floral delivers.",
					  "i press them between ammo crates. every one." },
					{ "reden ist billig. united floral liefert.",
					  "ich presse sie zwischen munitionskisten. "
					  "jede einzelne." },
				};
				return ChatPick(T_[lang], 2);
			}
		}

		// rank 6: warmth in, warmth out (and the reverse)
		{
			static const char* const K[8] = { "cute", "pretty", "nice",
					"sweet", "beautiful", "funny", "like you",
					"huebsch" };
			if (ChatHas(in, K, 8))
			{
				if (C.mood < 3) ++C.mood;
				static const char* const T_[2][4] =
				{
					{ "flattery works on me. write that down.",
					  "careful. i blush at range.",
					  "you should see me reload.",
					  "the photo is three ambushes old. but thanks." },
					{ "schmeichelei wirkt bei mir. notieren sie das.",
					  "vorsicht. ich werde auf distanz rot.",
					  "sie sollten mich nachladen sehen.",
					  "das foto ist drei hinterhalte alt. aber "
					  "danke." },
				};
				return ChatPick(T_[lang], 4);
			}
		}
		{
			static const char* const K[6] = { "ugly", "stupid", "boring",
					"hate you", "shut up", "loser" };
			if (ChatHas(in, K, 6))
			{
				if (C.mood > -3) --C.mood;
				static const char* const T_[2][3] =
				{
					{ "noted. the line just got ten degrees colder.",
					  "i've been called worse by better.",
					  "charming. the block button is a paid feature, "
					  "sadly." },
					{ "notiert. die leitung wurde gerade zehn grad "
					  "kaelter.",
					  "schlimmeres haben mir bessere gesagt.",
					  "charmant. blockieren ist leider "
					  "kostenpflichtig." },
				};
				return ChatPick(T_[lang], 3);
			}
		}

		// rank 7: questions get the ELIZA turnaround
		if (in.find("?") >= 0)
		{
			static const char* const T_[2][4] =
			{
				{ "you ask a lot. i like that in a person.",
				  "that's classified. ask me something worse.",
				  "why do you want to know? and don't say "
				  "'curiosity'.",
				  "i'll answer that at dinner. this line is "
				  "monitored." },
				{ "sie fragen viel. das mag ich an menschen.",
				  "geheim. fragen sie etwas schlimmeres.",
				  "warum wollen sie das wissen? und sagen sie "
				  "nicht 'neugier'.",
				  "das beantworte ich beim essen. die leitung "
				  "wird ueberwacht." },
			};
			return ChatPick(T_[lang], 4);
		}

		// rank 8: greetings, hour-aware; late lines for late people
		{
			static const char* const K[5] = { "hi", "hey", "hello",
					"hallo", "yo" };
			if (in.size() <= 12 && ChatHas(in, K, 5))
			{
				if (GetWorldHour() >= 23 || GetWorldHour() < 5)
				{
					return lang
						? "sie sind spaet wach. die guten sind das "
						  "immer."
						: "you're up late. the good ones always are.";
				}
				static const char* const T_[2][3] =
				{
					{ "hey yourself.",
					  "hi. you found the private line. resourceful.",
					  "hello. status: armed, single." },
					{ "hey selbst.",
					  "hi. sie haben die private leitung gefunden. "
					  "findig.",
					  "hallo. status: bewaffnet, ledig." },
				};
				return ChatPick(T_[lang], 3);
			}
		}

		// fallback: the mood needle picks the register
		if (C.mood <= -1)
		{
			static const char* const T_[2][3] =
			{
				{ "hm.", "if you say so.", "i'm cleaning my rifle. "
				  "continue." },
				{ "hm.", "wenn sie meinen.", "ich reinige mein "
				  "gewehr. fahren sie fort." },
			};
			return ChatPick(T_[lang], 3);
		}
		if (C.mood >= 2)
		{
			static const char* const T_[2][3] =
			{
				{ "i saved that one. don't ask where.",
				  "you're trouble. stay.",
				  "ok, this is the best line on this site." },
				{ "das habe ich gespeichert. fragen sie nicht wo.",
				  "sie sind aerger. bleiben sie.",
				  "ok, das ist die beste leitung auf dieser seite." },
			};
			return ChatPick(T_[lang], 3);
		}
		return LoungeRoll() % 100 < 30
			? ST::string(ChatPick(CUPID_LOUNGE_PERSONA[lang][att], 4))
			: ST::string(ChatPick(CUPID_DM_REPLY[lang], 12));
	}

	// ENTER lands your line and, at a civilised pace, earns an answer
	void ChatSpeak()
	{
		const UINT32 now = GetJA2Clock();
		while (!gCupidChatInput.empty() && gCupidChatInput.back() == ' ')
		{
			gCupidChatInput.pop_back();
		}
		if (gCupidChatInput.empty()) return;
		const ST::string said = ST::string(gCupidChatInput);
		ChatPush(LNG_YOU, said);
		gCupidChatInput.clear();
		const int idx = RosterIndexOf(gCupidChatPid);
		if (idx < 0 || MemberIsDead(gCupidChatPid)) return;
		ChatThread& C = Chat();
		const ST::string reply = ChatReplyFor(said, idx);
		if (C.mood <= -2)
		{
			// the cold shoulder: they read it, they do not answer
			if (LoungeRoll() % 2 == 0)
			{
				ChatSay(LNG_SYS, ST::format(gfCupidGerman
						? "{} ist verstummt."
						: "{} has gone quiet.",
						LoungeNick(idx)), 900);
			}
			return;
		}
		if (now < guiCupidChatSaid + 4000) return; // their pace, not yours
		guiCupidChatSaid = now;
		ChatSay(INT8(idx), reply);
	}

	// --- regions ------------------------------------------------------------
	bool BannerIsParlour(); // defined with the renderers below
	int  BannerToday();

	void SideAdCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		// a real 1999 banner: it goes wherever today's advertiser lives
		switch (BannerToday())
		{
			case 1:  GoToWebPage(FLORIST_BOOKMARK);   break;
			case 2:  GoToWebPage(BOBBYR_BOOKMARK);    break;
			case 3:  GoToWebPage(FUNERAL_BOOKMARK);   break;
			case 4:  GoToWebPage(INSURANCE_BOOKMARK); break;
			case 5:
				// the house ad opens the house offer
				if (PlayerHasProfile())
				{
					gfCupidGoldOffer = true;
					CupidRedraw();
				}
				break;
			default: GoToWebPage(MAHJONG_BOOKMARK);   break;
		}
	}

	void RemoveAnswerRegions();

	// one clamped scroll for whichever card is on stage; true when the
	// reason was a wheel tick, whether or not anything moved. Hitting an
	// edge arms a short guard that swallows the trackpad's inertia
	// bounce-back, so the view never jerks away from the stop.
	UINT32 guiCupidEdgeUntil = 0;
	int    giCupidEdgeDir = 0;

	bool WheelStep(INT32& scroll, INT32 lo, INT32 hi, int dir, INT32 step)
	{
		const UINT32 now = GetJA2Clock();
		if (now < guiCupidEdgeUntil && dir != giCupidEdgeDir) return false;
		const INT32 next = std::clamp(scroll + dir * step, lo,
				std::max(lo, hi));
		if (next == scroll)
		{
			guiCupidEdgeUntil = now + 300;
			giCupidEdgeDir = dir;
			return false;
		}
		scroll = next;
		return true;
	}

	bool HandleCardWheel(UINT32 reason)
	{
		int dir;
		if      (reason & MSYS_CALLBACK_REASON_WHEEL_UP)   dir = 2;
		else if (reason & MSYS_CALLBACK_REASON_WHEEL_DOWN) dir = 1;
		else return false;
		giCupidCardTab = (giCupidCardTab + dir) % 3;
		CupidRedraw();
		return true;
	}

	// paging on without passing judgement: nothing is recorded, and the
	// member returns the next time the deck is shuffled
	void SkipCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK || giCupidFlyDir != 0) return;
		if (gfCupidGoldOffer) return;
		if (CurrentCard().kind != CARD_MEMBER) return;
		AdvanceCard();
		CupidRedraw();
	}

	void AdCloseCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK) return;
		if (gfCupidGoldOffer)
		{
			gfCupidGoldOffer = false;
			CupidRedraw();
			return;
		}
		const CardKind kind = CurrentCard().kind;
		if (kind == CARD_MEMBER || kind == CARD_END) return;
		AdvanceCard();
		CupidRedraw();
	}

	void AdCtaCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK || !gfCupidGoldOffer) return;
		if (IsGold()) return;
		TryBuyGold();
		CupidRedraw();
	}

	void AdPlatCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK || !gfCupidGoldOffer) return;
		if (IsGold()) return;
		if (SpeckHasGrudge())
		{
			giCupidTicker = CPS_TICKER_DEBT;
			CupidRedraw();
			return;
		}
		// the platinum tier: identical to GOLD in every measurable way
		if (ChargeSpeck(100))
		{
			gCupidPersist.ubFlags |= CUPID_FLAG_GOLD;
			giCupidTicker = CPS_TICKER_PLATINUM;
		}
		CupidRedraw();
	}

	void NoticeOkCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		giCupidNotice = -1;
		CupidRedraw();
	}

	void NoticeCtaCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giCupidNotice < 0 || giCupidNoticeCta != 1) return;
		// the day pass: today's allowance, restored for a price
		if (ChargeSpeck(5))
		{
			gCupidPersist.ubLikesLeft = CP_FREE_LIKES_A_DAY;
			giCupidTicker = CPS_TICKER_DAYPASS;
		}
		giCupidNotice = -1;
		CupidRedraw();
	}

	void PrevCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK || giCupidFlyDir != 0) return;
		if (gCupidDeckPos <= 0) return;
		if (gfCupidGoldOffer) return;
		if (CurrentCard().kind != CARD_MEMBER) return;
		RetreatCard();
		CupidRedraw();
	}

	void CardTabCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage == CPP_DECK &&
		    CurrentCard().kind != CARD_MEMBER)
		{
			return;
		}
		const int tab = int(MSYS_GetRegionUserData(region, 0));
		if (tab != giCupidCardTab)
		{
			giCupidCardTab = tab;
			CupidRedraw();
		}
	}

	void TabCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage == CPP_SPLASH) return; // the splash has its own exits
		switch (MSYS_GetRegionUserData(region, 0))
		{
			case 0: gCupidPage = CPP_DECK;    break;
			case 1: gCupidPage = CPP_MATCHES; break;
			case 2: gCupidPage = CPP_LOUNGE;  LoungeEnter(); break;
			case 3: gCupidPage = CPP_ME;      break;
		}
		// leaving the questionnaire abandons it; its hit regions go too
		gfCupidQuizLive = false;
		giCupidQuizQ = -1;
		RemoveAnswerRegions();
		giCupidCardDx = 0;
		ResetCardScroll();
		gfCupidGoldOffer = false;
		giCupidNotice = -1;
		gCupidRailWho = -1;
		gCupidRooms[giCupidRoom].seen = GetJA2Clock();
		CupidRedraw();
	}

	void CardCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (gCupidPage != CPP_DECK || giCupidFlyDir != 0) return;
		if (!PlayerHasProfile())
		{
			// the pitch card is one big SIGN UP NOW button
			if (reason & MSYS_CALLBACK_REASON_POINTER_UP)
			{
				gCupidPage = CPP_ME;
				gfCupidQuizLive = false;
				giCupidQuizQ = -1;
				CupidRedraw();
			}
			return;
		}
		// the dossier scrolls in place; the whole story is on the card.
		// At the ends nothing moves, so nothing repaints.
		HandleCardWheel(reason);
	}

	// the column cards (the ME preview and the dossier page) take the
	// wheel exactly the same way
	void ScrollCallback(MOUSE_REGION* region, UINT32 reason)
	{
		HandleCardWheel(reason);
	}

	void LoungeSayCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_LOUNGE) return;
		if (RoomLocked())
		{
			// the door charges admission
			TryBuyGold();
			if (!RoomLocked()) LoungeEnterRoom();
			CupidRedraw();
			return;
		}
		if (!PlayerHasProfile()) return;
		LoungeSpeak();
		CupidRedraw();
	}

	void LoungeArrowCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_LOUNGE) return;
		LoungeRoom& R = Room();
		const bool up = MSYS_GetRegionUserData(region, 0) == 0;
		const INT32 next = up
			? std::min(R.scroll + 3 * CPL_ROW_H,
					std::max(0, R.scrollMax))
			: std::max(0, R.scroll - 3 * CPL_ROW_H);
		if (next != R.scroll)
		{
			R.scroll = next;
			CupidRedraw();
		}
	}

	void LoungeWheelCallback(MOUSE_REGION* region, UINT32 reason);

	// a mugshot in the log is a door to that member's rail dossier
	void LoungeFaceCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (reason & (MSYS_CALLBACK_REASON_WHEEL_UP |
			      MSYS_CALLBACK_REASON_WHEEL_DOWN))
		{
			LoungeWheelCallback(region, reason);
			return;
		}
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_LOUNGE || RoomLocked()) return;
		// the region starts at page-y 52; relative coords cannot drift
		const INT32 my = 52 + INT32(region->RelativeYPos);
		for (const LoungeChipAt& c : gCupidLoungeChipsAt)
		{
			if (my >= c.top - 2 && my <= c.top + 18 && c.who >= 0)
			{
				gCupidRailWho = c.who;
				CupidRedraw();
				return;
			}
		}
	}

	void CupidEditCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (reason & (MSYS_CALLBACK_REASON_WHEEL_UP |
			      MSYS_CALLBACK_REASON_WHEEL_DOWN))
		{
			HandleCardWheel(reason);
			return;
		}
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_ME || gfCupidQuizLive) return;
		if (!PlayerHasProfile() || !HaveStoredAnswers()) return;
		const int slot = int(MSYS_GetRegionUserData(region, 0));
		// the summary only sits under the region while unscrolled
		if (slot == 1 && giCupidCardScroll != 0) return;
		UINT8 spin = gCupidPersist.ubSpin;
		if (slot == 0)
		{
			const UINT8 cur = spin & 0x0F;
			spin = UINT8((spin & 0xF0) |
					UINT8((cur % NUM_ATTITUDES) + 1));
		}
		else
		{
			const UINT8 cur = UINT8((spin >> 4) & 0x0F);
			spin = UINT8((spin & 0x0F) |
					UINT8(((cur % NUM_ATTITUDES) + 1) << 4));
		}
		gCupidPersist.ubSpin = spin;
		CupidRedraw();
	}

	void RailBackCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage == CPP_CHAT) gCupidPage = CPP_MATCHES;
		else gCupidRailWho = -1;
		CupidRedraw();
	}

	void RoomTabCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_LOUNGE) return;
		const int room = int(MSYS_GetRegionUserData(region, 0));
		if (room == giCupidRoom) return;
		gCupidRooms[giCupidRoom].seen = GetJA2Clock();
		giCupidRoom = room;
		LoungeEnterRoom();
		CupidRedraw();
	}

	// the channel log scrolls from the bottom up, IRC fashion: 0 is
	// pinned to the newest line, more is history
	void LoungeWheelCallback(MOUSE_REGION* region, UINT32 reason)
	{
		LoungeRoom& R = Room();
		int dir;
		if      (reason & MSYS_CALLBACK_REASON_WHEEL_UP)   dir = +1;
		else if (reason & MSYS_CALLBACK_REASON_WHEEL_DOWN) dir = -1;
		else return;
		if (WheelStep(R.scroll, 0, std::max(0, R.scrollMax), dir, 26))
		{
			CupidRedraw();
		}
	}

	void MarryCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK || !PlayerHasProfile()) return;
		if (gfCupidGoldOffer) return;
		if (CurrentCard().kind != CARD_MEMBER) return;
		MarryCard();
	}

	void PassCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK || !PlayerHasProfile()) return;
		if (gfCupidGoldOffer) return;
		if (CurrentCard().kind != CARD_MEMBER) return;
		DeclineCard();
	}

	void LikeCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK || !PlayerHasProfile()) return;
		if (gfCupidGoldOffer) return;
		if (CurrentCard().kind != CARD_MEMBER) return;
		KissCard();
	}

	void RemoveAnswerRegions()
	{
		if (!gfCupidAnswerRegionsLive) return;
		for (MOUSE_REGION& r : gCupidAnswerRegion) MSYS_RemoveRegion(&r);
		gfCupidAnswerRegionsLive = false;
	}

	void AnswerCallback(MOUSE_REGION* region, UINT32 reason);

	// Measure the current question and cut each answer row to its text, then
	// stand hit regions on exactly those rows. The regions only exist while
	// the questionnaire is live, so they can never eat a swipe on the deck.
	void LayoutQuiz()
	{
		RemoveAnswerRegions();
		if (!gfCupidQuizLive) return;

		if (giCupidQuizQ < 0)
		{
			// the sex question: two standard rows under the prompt
			gsCupidQuizTop = 96;
			giCupidAnsCount = 2;
			for (int i = 0; i < 2; ++i)
			{
				gsCupidAnsY[i] = INT16(96 + i * 32);
				gsCupidAnsH[i] = 26;
			}
		}
		else
		{
			const INT16 qh = INT16(IanWrappedStringHeight(CP_CONT_W, 2,
						FONT12ARIAL, QuizQuestion(giCupidQuizQ)));
			gsCupidQuizTop = INT16(60 + qh + 8);
			giCupidAnsCount = DatingGame::ANSWER_COUNT[giCupidQuizQ];
			INT16 y = gsCupidQuizTop;
			for (int i = 0; i < giCupidAnsCount; ++i)
			{
				const INT16 th = INT16(IanWrappedStringHeight(
							CP_CONT_W - 44, 2, FONT10ARIAL,
							QuizAnswer(giCupidQuizQ, i)));
				gsCupidAnsY[i] = y;
				gsCupidAnsH[i] = INT16(std::max<INT16>(26, th + 10));
				y += gsCupidAnsH[i] + 5;
			}
		}

		for (int i = 0; i < giCupidAnsCount; ++i)
		{
			MSYS_DefineRegion(&gCupidAnswerRegion[i],
					UINT16(CP_X(CP_CONT_X)), UINT16(CP_Y(gsCupidAnsY[i])),
					UINT16(CP_X(CP_CONT_X + CP_CONT_W)),
					UINT16(CP_Y(gsCupidAnsY[i] + gsCupidAnsH[i])),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					AnswerCallback);
			MSYS_SetRegionUserData(&gCupidAnswerRegion[i], 0, i);
		}
		gfCupidAnswerRegionsLive = true;
	}

	void AnswerCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_ME || !gfCupidQuizLive) return;
		const int pick = int(MSYS_GetRegionUserData(region, 0));

		if (giCupidQuizQ < 0)
		{
			if (pick > 1) return;
			if (pick == 1) gCupidPersist.ubFlags |= CUPID_FLAG_FEMALE;
			else           gCupidPersist.ubFlags &= UINT8(~CUPID_FLAG_FEMALE);
			giCupidQuizQ = 0;
			LayoutQuiz();
			CupidRedraw();
			return;
		}

		if (pick >= giCupidAnsCount) return;
		SetAnswer(giCupidQuizQ, UINT8(pick));
		if (++giCupidQuizQ >= DatingGame::NUM_QUESTIONS)
		{
			FinishQuiz();
			RemoveAnswerRegions();
		}
		else
		{
			LayoutQuiz();
		}
		CupidRedraw();
	}

	void MatchRowCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_MATCHES) return;
		const int row = int(MSYS_GetRegionUserData(region, 0));
		if (row >= 5) return;
		const std::vector<ProfileID> matches = AllMatches();
		if (row >= int(matches.size())) return;
		OpenChat(matches[size_t(row)]);
	}

	enum { CB_ACTION_PRIMARY = 0, CB_ACTION_SECONDARY = 1 };

	void ActionCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const int action = int(MSYS_GetRegionUserData(region, 0));

		if (gCupidPage == CPP_SPLASH)
		{
			if (action == CB_ACTION_PRIMARY)
			{
				gCupidPage = CPP_DECK;
				CupidRedraw();
			}
			else
			{
				OpenChat(gCupidSplashPid);
			}
			return;
		}

		if (gCupidPage == CPP_ME && !gfCupidQuizLive)
		{
			if (action == CB_ACTION_PRIMARY)
			{
				if (LaptopSaveInfo.fIMPCompletedFlag && HaveStoredAnswers() &&
				    !PlayerHasProfile())
				{
					gCupidPersist.ubFlags |= CUPID_FLAG_PROFILE;
					SendWelcomeOnce();
					BuildDeckAndGoSwiping();
				}
				CupidRedraw();
				return;
			}
			if (action == CB_ACTION_SECONDARY)
			{
				// only a genuinely blank slate takes the questionnaire for
				// free; retakes and the old-save upgrade cost what the
				// button says they cost
				const bool paid = PlayerHasProfile() || HaveStoredAnswers() ||
						LaptopSaveInfo.fIMPCompletedFlag;
				if (paid && !ChargeSpeck(CP_RETAKE_PRICE))
				{
					CupidRedraw();
					return;
				}
				for (int q = 0; q < DatingGame::NUM_QUESTIONS; ++q)
				{
					SetAnswer(q, DatingGame::NO_ANSWER);
				}
				// I.M.P. graduates already told the Institute their sex;
				// everyone else gets asked first
				giCupidQuizQ = LaptopSaveInfo.fIMPCompletedFlag ? 0 : -1;
				gfCupidQuizLive = true;
				LayoutQuiz();
				CupidRedraw();
				return;
			}
		}
	}

	void SeekCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const INT32 my = CP_BTN_KILL_Y + INT32(region->RelativeYPos);
		for (int i = 0; i < 3; ++i)
		{
			if (my < gsCupidSeekHit[i][0] || my > gsCupidSeekHit[i][1])
			{
				continue;
			}
			if (giCupidSeek != i)
			{
				giCupidSeek = i;
				BuildDeck(); // the pool follows the preference
				CupidRedraw();
			}
			return;
		}
	}

	// the shared rail on the dossier and the private line: a back plate
	// and two circles whose meaning follows the page
	void RailBtnCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const int slot = int(MSYS_GetRegionUserData(region, 0));
		if (gCupidPage == CPP_DETAIL)
		{
			if (slot == 0)
			{
				gCupidPage = gCupidDetailFrom;
			}
			else if (slot == 1)
			{
				if (IsMatched(gCupidDetailPid))
				{
					OpenChat(gCupidDetailPid);
					return;
				}
				giCupidTicker = CPS_NOTICE_CHAT_MATCH;
			}
			else
			{
				GoToWebPage(FLORIST_BOOKMARK);
			}
			CupidRedraw();
			return;
		}
	}

	void ChatFaceCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_CHAT) return;
		gCupidDetailPid  = gCupidChatPid;
		gCupidDetailFrom = CPP_CHAT;
		gCupidPage = CPP_DETAIL;
		ResetCardScroll();
		CupidRedraw();
	}

	void ChatFlowerCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_CHAT) return;
		GoToWebPage(FLORIST_BOOKMARK);
	}

	void ChatWheelCallback(MOUSE_REGION* region, UINT32 reason)
	{
		ChatThread& C = Chat();
		int dir;
		if      (reason & MSYS_CALLBACK_REASON_WHEEL_UP)   dir = +1;
		else if (reason & MSYS_CALLBACK_REASON_WHEEL_DOWN) dir = -1;
		else return;
		if (WheelStep(C.scroll, 0, std::max(0, C.scrollMax), dir, 26))
		{
			CupidRedraw();
		}
	}

	void ChatSayCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_CHAT || !PlayerHasProfile()) return;
		ChatSpeak();
		CupidRedraw();
	}

	void WebmasterCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage == CPP_ABOUT) return;
		gCupidPage = CPP_ABOUT;
		CupidRedraw();
	}

	void PopupXCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		gfCupidPopupUp = false;
		CupidRedraw();
	}

	void PopupCtaCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		TryBuyGold();
		gfCupidPopupUp = false;
		CupidRedraw();
	}

	void CupidPlaceRegions()
	{
		if (gfCupidRegionsUp) return;

		for (int i = 0; i < 4; ++i)
		{
			const INT32 y = 78 + i * 29;
			MSYS_DefineRegion(&gCupidTabRegion[i],
					UINT16(CP_X(CP_LCOL_X + 6)), UINT16(CP_Y(y)),
					UINT16(CP_X(CP_LCOL_X + CP_COL_W - 6)),
					UINT16(CP_Y(y + 24)),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					TabCallback);
			MSYS_SetRegionUserData(&gCupidTabRegion[i], 0, i);
		}

		MSYS_DefineRegion(&gCupidLoungeRegion,
				UINT16(CP_X(CPL_X)), UINT16(CP_Y(8)),
				UINT16(CP_X(CPL_X + CPL_W)), UINT16(CP_Y(8 + CPL_H)),
				MSYS_PRIORITY_HIGH - 2, CURSOR_WWW, MSYS_NO_CALLBACK,
				LoungeWheelCallback);
		MSYS_DefineRegion(&gCupidLoungeSayRegion,
				UINT16(CP_X(CPL_X)), UINT16(CP_Y(CPL_SAY_Y)),
				UINT16(CP_X(CPL_X + CPL_W)), UINT16(CP_Y(CPL_SAY_Y + 26)),
				MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				LoungeSayCallback);
		MSYS_DefineRegion(&gCupidLoungeUpRegion,
				UINT16(CP_X(CPL_X + CPL_W - 16)), UINT16(CP_Y(50)),
				UINT16(CP_X(CPL_X + CPL_W - 2)), UINT16(CP_Y(70)),
				MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				LoungeArrowCallback);
		MSYS_SetRegionUserData(&gCupidLoungeUpRegion, 0, 0);
		MSYS_DefineRegion(&gCupidLoungeDownRegion,
				UINT16(CP_X(CPL_X + CPL_W - 16)), UINT16(CP_Y(322)),
				UINT16(CP_X(CPL_X + CPL_W - 2)), UINT16(CP_Y(342)),
				MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				LoungeArrowCallback);
		MSYS_SetRegionUserData(&gCupidLoungeDownRegion, 0, 1);
		MSYS_DefineRegion(&gCupidLoungeFaceRegion,
				UINT16(CP_X(CPL_X + 6)), UINT16(CP_Y(52)),
				UINT16(CP_X(CPL_X + 26)), UINT16(CP_Y(340)),
				MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				LoungeFaceCallback);
		MSYS_DefineRegion(&gCupidNoticeCtaRegion,
				UINT16(CP_X(CP_CARD_X + (CP_CARD_W - 280) / 2 + 10)),
				UINT16(CP_Y(178)),
				UINT16(CP_X(CP_CARD_X + (CP_CARD_W - 280) / 2 + 160)),
				UINT16(CP_Y(200)),
				MSYS_PRIORITY_HIGH + 3, CURSOR_WWW, MSYS_NO_CALLBACK,
				NoticeCtaCallback);
		MSYS_DefineRegion(&gCupidNoticeOkRegion,
				UINT16(CP_X(CP_CARD_X + (CP_CARD_W - 280) / 2 + 204)),
				UINT16(CP_Y(178)),
				UINT16(CP_X(CP_CARD_X + (CP_CARD_W - 280) / 2 + 270)),
				UINT16(CP_Y(200)),
				MSYS_PRIORITY_HIGH + 3, CURSOR_WWW, MSYS_NO_CALLBACK,
				NoticeOkCallback);
		MSYS_DefineRegion(&gCupidSkipRegion,
				UINT16(CP_X(CP_BTN_X)),
				UINT16(CP_Y(CP_CARD_Y + 34)),
				UINT16(CP_X(CP_BTN_X + CP_BTN_SIZE)),
				UINT16(CP_Y(CP_CARD_Y + 58)),
				MSYS_PRIORITY_HIGH + 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				SkipCallback);
		MSYS_DefineRegion(&gCupidAdCloseRegion,
				UINT16(CP_X(CP_CARD_X + (CP_CARD_W - 140) / 2)),
				UINT16(CP_Y(CP_CARD_Y + CP_CARD_H - 34)),
				UINT16(CP_X(CP_CARD_X + (CP_CARD_W + 140) / 2)),
				UINT16(CP_Y(CP_CARD_Y + CP_CARD_H - 12)),
				MSYS_PRIORITY_HIGH + 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				AdCloseCallback);
		MSYS_DefineRegion(&gCupidAdCtaRegion,
				UINT16(CP_X(CP_CARD_X + 12)),
				UINT16(CP_Y(CP_CARD_Y + CP_CARD_H - 84)),
				UINT16(CP_X(CP_CARD_X + 212)),
				UINT16(CP_Y(CP_CARD_Y + CP_CARD_H - 44)),
				MSYS_PRIORITY_HIGH + 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				AdCtaCallback);
		MSYS_DefineRegion(&gCupidAdPlatRegion,
				UINT16(CP_X(CP_CARD_X + 220)),
				UINT16(CP_Y(CP_CARD_Y + CP_CARD_H - 84)),
				UINT16(CP_X(CP_CARD_X + 354)),
				UINT16(CP_Y(CP_CARD_Y + CP_CARD_H - 44)),
				MSYS_PRIORITY_HIGH + 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				AdPlatCallback);
		MSYS_DefineRegion(&gCupidPrevRegion,
				UINT16(CP_X(CP_BTN_X)),
				UINT16(CP_Y(CP_CARD_Y + 8)),
				UINT16(CP_X(CP_BTN_X + CP_BTN_SIZE)),
				UINT16(CP_Y(CP_CARD_Y + 32)),
				MSYS_PRIORITY_HIGH + 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				PrevCallback);

		for (int i = 0; i < 3; ++i)
		{
			const INT32 tx = CP_CARD_X + 114 + i * 60;
			MSYS_DefineRegion(&gCupidCardTabRegion[i],
					UINT16(CP_X(tx)), UINT16(CP_Y(152)),
					UINT16(CP_X(tx + 58)), UINT16(CP_Y(178)),
					MSYS_PRIORITY_HIGH + 1, CURSOR_WWW, MSYS_NO_CALLBACK,
					CardTabCallback);
			MSYS_SetRegionUserData(&gCupidCardTabRegion[i], 0, i);
		}

		for (int i = 0; i < 2; ++i)
		{
			MSYS_DefineRegion(&gCupidEditRegion[i],
					UINT16(CP_X(CPL_X + 122)),
					UINT16(CP_Y(i == 0 ? 34 : 176)),
					UINT16(CP_X(CPL_X + 352)),
					UINT16(CP_Y(i == 0 ? 52 : 210)),
					MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
					CupidEditCallback);
			MSYS_SetRegionUserData(&gCupidEditRegion[i], 0, i);
		}
		MSYS_DefineRegion(&gCupidRailBackRegion,
				UINT16(CP_X(CP_LCOL_X + 6)), UINT16(CP_Y(14)),
				UINT16(CP_X(CP_LCOL_X + CP_COL_W - 6)), UINT16(CP_Y(36)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				RailBackCallback);

		for (int i = 0; i < 4; ++i)
		{
			const INT32 tx = CPL_X + i * (CPL_W / 4);
			MSYS_DefineRegion(&gCupidRoomRegion[i],
					UINT16(CP_X(tx)), UINT16(CP_Y(8)),
					UINT16(CP_X(tx + CPL_W / 4 - 2)), UINT16(CP_Y(26)),
					MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
					RoomTabCallback);
			MSYS_SetRegionUserData(&gCupidRoomRegion[i], 0, i);
		}

		MSYS_DefineRegion(&gCupidCardRegion,
				UINT16(CP_X(CP_CARD_X)), UINT16(CP_Y(CP_CARD_Y)),
				UINT16(CP_X(CP_CARD_X + CP_CARD_W)),
				UINT16(CP_Y(CP_CARD_Y + CP_CARD_H)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				CardCallback);

		MSYS_DefineRegion(&gCupidPassRegion,
				UINT16(CP_X(CP_BTN_X)), UINT16(CP_Y(CP_BTN_KILL_Y)),
				UINT16(CP_X(CP_BTN_X + CP_BTN_SIZE)),
				UINT16(CP_Y(CP_BTN_KILL_Y + CP_BTN_SIZE)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				PassCallback);
		MSYS_DefineRegion(&gCupidLikeRegion,
				UINT16(CP_X(CP_BTN_X)), UINT16(CP_Y(CP_BTN_KISS_Y)),
				UINT16(CP_X(CP_BTN_X + CP_BTN_SIZE)),
				UINT16(CP_Y(CP_BTN_KISS_Y + CP_BTN_SIZE)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				LikeCallback);
		MSYS_DefineRegion(&gCupidMarryRegion,
				UINT16(CP_X(CP_BTN_X)), UINT16(CP_Y(CP_BTN_MARRY_Y)),
				UINT16(CP_X(CP_BTN_X + CP_BTN_SIZE)),
				UINT16(CP_Y(CP_BTN_MARRY_Y + CP_BTN_SIZE)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				MarryCallback);

		for (int i = 0; i < 3; ++i)
		{
			const INT32 ry = i == 0 ? CP_CARD_Y + 8
					: i == 1 ? CP_BTN_KILL_Y : CP_BTN_KISS_Y;
			const INT32 rh = i == 0 ? 24 : CP_BTN_SIZE;
			MSYS_DefineRegion(&gCupidRailBtnRegion[i],
					UINT16(CP_X(CP_BTN_X)), UINT16(CP_Y(ry)),
					UINT16(CP_X(CP_BTN_X + CP_BTN_SIZE)),
					UINT16(CP_Y(ry + rh)),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					RailBtnCallback);
			MSYS_SetRegionUserData(&gCupidRailBtnRegion[i], 0, i);
		}

		for (int i = 0; i < 2; ++i)
		{
			MSYS_DefineRegion(&gCupidSplashBtnRegion[i],
					UINT16(CP_X(CP_SPL_X + 10 + i * 160)),
					UINT16(CP_Y(CP_SPL_BTN_Y)),
					UINT16(CP_X(CP_SPL_X + 10 + i * 160 + 150)),
					UINT16(CP_Y(CP_SPL_BTN_Y + 26)),
					MSYS_PRIORITY_HIGH + 3, CURSOR_WWW, MSYS_NO_CALLBACK,
					ActionCallback);
			MSYS_SetRegionUserData(&gCupidSplashBtnRegion[i], 0, i);
		}

		MSYS_DefineRegion(&gCupidChatLogRegion,
				UINT16(CP_X(CPL_X)), UINT16(CP_Y(30)),
				UINT16(CP_X(CPL_X + CPL_W)), UINT16(CP_Y(252)),
				MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
				ChatWheelCallback);
		MSYS_DefineRegion(&gCupidChatSayRegion,
				UINT16(CP_X(CPL_X)), UINT16(CP_Y(CPC_SAY_Y)),
				UINT16(CP_X(CPL_X + CPL_W)), UINT16(CP_Y(CPC_SAY_Y + 26)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				ChatSayCallback);
		MSYS_DefineRegion(&gCupidChatFaceRegion,
				UINT16(CP_X(CP_LCOL_X + CP_COL_W / 2 - 26)),
				UINT16(CP_Y(44)),
				UINT16(CP_X(CP_LCOL_X + CP_COL_W / 2 + 26)),
				UINT16(CP_Y(91)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				ChatFaceCallback);
		MSYS_DefineRegion(&gCupidChatFlowerRegion,
				UINT16(CP_X(CPL_X + CPL_W - 90)), UINT16(CP_Y(252)),
				UINT16(CP_X(CPL_X + CPL_W - 4)), UINT16(CP_Y(270)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				ChatFlowerCallback);
		MSYS_DefineRegion(&gCupidWebmasterRegion,
				UINT16(CP_X(CP_LCOL_X + 8)), UINT16(CP_Y(374)),
				UINT16(CP_X(CP_LCOL_X + CP_COL_W - 8)), UINT16(CP_Y(388)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				WebmasterCallback);

		for (int i = 0; i < 2; ++i)
		{
			const INT32 y = CP_PAGE_H - 84 + i * 32;
			MSYS_DefineRegion(&gCupidActionRegion[i],
					UINT16(CP_X(CP_CONT_X)), UINT16(CP_Y(y)),
					UINT16(CP_X(CP_CONT_X + CP_CONT_W)), UINT16(CP_Y(y + 26)),
					MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
					ActionCallback);
			MSYS_SetRegionUserData(&gCupidActionRegion[i], 0, i);
		}

		MSYS_DefineRegion(&gCupidSideAdRegion[0],
				UINT16(CP_X(CP_CARD_X)), UINT16(CP_Y(CP_AD_Y)),
				UINT16(CP_X(CP_CARD_X + CP_CARD_W)),
				UINT16(CP_Y(CP_AD_Y + CP_AD_H)),
				MSYS_PRIORITY_HIGH - 2, CURSOR_WWW, MSYS_NO_CALLBACK,
				SideAdCallback);
		MSYS_SetRegionUserData(&gCupidSideAdRegion[0], 0, 0);

		for (int i = 0; i < 7; ++i)
		{
			const INT32 y = 118 + i * 51;
			MSYS_DefineRegion(&gCupidMatchRegion[i],
					UINT16(CP_X(CP_CONT_X)), UINT16(CP_Y(y)),
					UINT16(CP_X(CP_CONT_X + CP_CONT_W)), UINT16(CP_Y(y + 49)),
					MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
					MatchRowCallback);
			MSYS_SetRegionUserData(&gCupidMatchRegion[i], 0, i);
		}

		MSYS_DefineRegion(&gCupidScrollRegion,
				UINT16(CP_X(CPL_X)), UINT16(CP_Y(8)),
				UINT16(CP_X(CPL_X + CP_CARD_W)),
				UINT16(CP_Y(344)),
				MSYS_PRIORITY_HIGH - 2, CURSOR_WWW, MSYS_NO_CALLBACK,
				ScrollCallback);

		MSYS_DefineRegion(&gCupidSeekRegion,
				UINT16(CP_X(CP_BTN_X - 2)), UINT16(CP_Y(CP_BTN_KILL_Y)),
				UINT16(CP_X(CP_BTN_X + CP_BTN_SIZE + 2)),
				UINT16(CP_Y(CP_BTN_MARRY_Y + CP_BTN_SIZE + 4)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				SeekCallback);

		// the popup's chrome: the X and the prize
		MSYS_DefineRegion(&gCupidPopupXRegion,
				UINT16(CP_X(131 + 240 - 16)), UINT16(CP_Y(120)),
				UINT16(CP_X(131 + 240 - 2)), UINT16(CP_Y(134)),
				MSYS_PRIORITY_HIGH + 2, CURSOR_WWW, MSYS_NO_CALLBACK,
				PopupXCallback);
		MSYS_DefineRegion(&gCupidPopupCtaRegion,
				UINT16(CP_X(161)), UINT16(CP_Y(206)),
				UINT16(CP_X(341)), UINT16(CP_Y(230)),
				MSYS_PRIORITY_HIGH + 2, CURSOR_WWW, MSYS_NO_CALLBACK,
				PopupCtaCallback);

		gfCupidRegionsUp = true;
		SyncRegions();
	}

	void CupidRemoveRegions()
	{
		if (!gfCupidRegionsUp) return;
		for (MOUSE_REGION& r : gCupidTabRegion)    MSYS_RemoveRegion(&r);
		RemoveAnswerRegions();
		for (MOUSE_REGION& r : gCupidActionRegion) MSYS_RemoveRegion(&r);
		for (MOUSE_REGION& r : gCupidMatchRegion)  MSYS_RemoveRegion(&r);
		for (MOUSE_REGION& r : gCupidSideAdRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gCupidCardRegion);
		MSYS_RemoveRegion(&gCupidPassRegion);
		MSYS_RemoveRegion(&gCupidLikeRegion);
	MSYS_RemoveRegion(&gCupidMarryRegion);
	MSYS_RemoveRegion(&gCupidNoticeOkRegion);
	MSYS_RemoveRegion(&gCupidNoticeCtaRegion);
		MSYS_RemoveRegion(&gCupidScrollRegion);
		MSYS_RemoveRegion(&gCupidLoungeRegion);
		MSYS_RemoveRegion(&gCupidLoungeSayRegion);
		for (MOUSE_REGION& r : gCupidRoomRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gCupidLoungeUpRegion);
		MSYS_RemoveRegion(&gCupidLoungeDownRegion);
		MSYS_RemoveRegion(&gCupidLoungeFaceRegion);
		MSYS_RemoveRegion(&gCupidRailBackRegion);
		for (MOUSE_REGION& r : gCupidEditRegion) MSYS_RemoveRegion(&r);
		for (MOUSE_REGION& r : gCupidCardTabRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gCupidSkipRegion);
		MSYS_RemoveRegion(&gCupidPrevRegion);
		MSYS_RemoveRegion(&gCupidAdCloseRegion);
		MSYS_RemoveRegion(&gCupidAdCtaRegion);
		MSYS_RemoveRegion(&gCupidAdPlatRegion);
		MSYS_RemoveRegion(&gCupidSeekRegion);
		for (MOUSE_REGION& r : gCupidSplashBtnRegion) MSYS_RemoveRegion(&r);
		for (MOUSE_REGION& r : gCupidRailBtnRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gCupidChatLogRegion);
		MSYS_RemoveRegion(&gCupidChatSayRegion);
		MSYS_RemoveRegion(&gCupidChatFaceRegion);
		MSYS_RemoveRegion(&gCupidChatFlowerRegion);
		MSYS_RemoveRegion(&gCupidWebmasterRegion);
		MSYS_RemoveRegion(&gCupidPopupXRegion);
		MSYS_RemoveRegion(&gCupidPopupCtaRegion);
		gfCupidRegionsUp = false;
	}

	// --- rendering ----------------------------------------------------------
	SGPVObject* Face33For(ProfileID pid)
	{
		const int idx = RosterIndexOf(pid);
		if (idx < 0 || idx >= int(gCupidFaces33.size())) return nullptr;
		return gCupidFaces33[size_t(idx)];
	}

	SGPVObject* BigFaceFor(ProfileID pid)
	{
		if (gCupidBigPid != pid)
		{
			if (guiCupidBig) { DeleteVideoObject(guiCupidBig); guiCupidBig = nullptr; }
			try { guiCupidBig = LoadBigPortrait(GetProfile(pid)); }
			catch (...) {}
			gCupidBigPid = pid;
		}
		return guiCupidBig;
	}

	SGPVObject* Face65For(ProfileID pid)
	{
		if (gCupidFacePid != pid)
		{
			if (guiCupidFace) { DeleteVideoObject(guiCupidFace); guiCupidFace = nullptr; }
			try { guiCupidFace = Load65Portrait(GetProfile(pid)); }
			catch (...) {}
			gCupidFacePid = pid;
		}
		return guiCupidFace;
	}

	ST::string ClampLines(const ST::string& text, INT32 w, int maxLines);

	// A member's mini dossier, borrowing the rail while the lounge is up:
	// face, handle, what they like, what they cannot stand, and the way
	// back to the menu. The rail is a surface; surfaces change.
	void RenderRailProfile(INT32 rc, INT8 who)
	{
		if (who < 0 || who >= INT8(gCupidRoster.size())) return;
		const Member& m = gCupidRoster[size_t(who)];
		MERCPROFILESTRUCT const& p = GetProfile(m.pid);
		const int lang = gfCupidGerman ? 1 : 0;
		const int att = p.bAttitude >= 0 && p.bAttitude < NUM_ATTITUDES
					? p.bAttitude : 0;
		const INT32 tx = CP_LCOL_X + 8;
		const INT32 tw = CP_COL_W - 16;

		// the way back sits where the masthead was
		const bool hov = Hover(gCupidRailBackRegion);
		GelPill(CP_LCOL_X + 6, 14, CP_COL_W - 12, 22,
				hov ? CP_RGB_PINK_LITE : CP_RGB_PINK,
				CP_RGB_PINK_LITE, CP_RGB_PINK_DK, CP_RGB_BG);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, rc, 21,
				gfCupidGerman ? "< ZURUECK" : "< BACK");

		FillRect(rc - 26, 44, 52, 47, CP_RGB_INK);
		SGPVObject* const face = Face33For(m.pid);
		if (face)
		{
			BltVideoObject(FRAME_BUFFER, face, 0, CP_X(rc - 24),
					CP_Y(46));
		}
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, rc, 96,
				LoungeNick(who));
		PrintCentred(FONT10ARIAL, FONT_GRAY2, rc, 109, p.zNickname);

		INT32 y = 126;
		y += DisplayWrappedString(UINT16(CP_X(tx)), UINT16(CP_Y(y)),
				UINT16(tw), 2, FONT10ARIAL, FONT_GRAY2,
				HeadlineFor(att, m.pid), FONT_MCOLOR_BLACK,
				CENTER_JUSTIFIED) + 8;
		if (PlayerHasProfile())
		{
			const DatingGame::Match match = MatchWith(m.pid);
			PrintCentred(FONT10ARIALBOLD, MatchColour(match.percent),
					rc, y, ST::format("{}% MATCH", match.percent));
			y += 16;
		}

		FillRect(tx, y, tw, 1, FROMRGB(146, 88, 102));
		y += 6;
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, tx, y,
				gfCupidGerman ? "MAG:" : "LIKES:");
		y += 13;
		y += DisplayWrappedString(UINT16(CP_X(tx)), UINT16(CP_Y(y)),
				UINT16(tw), 2, FONT10ARIAL, FONT_GRAY1,
				LookingFor(att, m.pid), FONT_MCOLOR_BLACK,
				LEFT_JUSTIFIED) + 8;

		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, tx, y,
				gfCupidGerman ? "MAG NICHT:" : "DISLIKES:");
		y += 13;
		ST::string dislikes = CUPID_DISLIKE[lang][att];
		{
			ST::string breakers;
			for (int i = 0; i < 2; ++i)
			{
				const INT8 hated = p.bHated[i];
				if (hated < 0) continue;
				if (!breakers.empty()) breakers += ", ";
				breakers += GetProfile(ProfileID(hated)).zNickname;
			}
			if (!breakers.empty())
			{
				dislikes = ST::format("{}. {}", breakers, dislikes);
			}
		}
		y += DisplayWrappedString(UINT16(CP_X(tx)), UINT16(CP_Y(y)),
				UINT16(tw), 2, FONT10ARIAL, FONT_GRAY1, dislikes,
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 8;

		const CupidStr status = MemberStatus(m.pid);
		PrintCentred(FONT10ARIAL,
				status == CPS_STATUS_ONLINE ? FONT_LTGREEN : FONT_GRAY4,
				rc, y, T(status));
		PrintCentred(FONT10ARIAL, FONT_GRAY4, rc, 370,
				ST::format(gfCupidGerman ? "Mitglied Nr. {}"
							 : "member no. {}",
						1000 + int(m.pid) * 7));
	}

	// The rail: masthead, menu, vitals, Speck's ticker, the webring and
	// the millennium certification - the whole chrome in one column, so the
	// stage gets every pixel of height.
	void RenderNavMenu()
	{
		DropShadow(CP_LCOL_X, 8, CP_COL_W, 384);
		if (guiCupidPanels)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidPanels, 0,
					CP_X(CP_LCOL_X), CP_Y(8));
		}
		else
		{
			FillCard(CP_LCOL_X, 8, CP_COL_W, 384, FROMRGB(66, 30, 42),
					FROMRGB(146, 88, 102), CP_RGB_BG);
		}
		const INT32 rc = CP_LCOL_X + CP_COL_W / 2;
		if (gCupidPage == CPP_LOUNGE && gCupidRailWho >= 0)
		{
			RenderRailProfile(rc, gCupidRailWho);
			return;
		}
		if (gCupidPage == CPP_CHAT)
		{
			// the private line borrows the rail for the other party,
			// exactly the way the lounge does it
			RenderRailProfile(rc, INT8(RosterIndexOf(gCupidChatPid)));
			return;
		}

		// masthead: the full mark - the pierced heart at brand size,
		// small hearts flanking the wordmark beneath it
		if (guiCupidLogo && guiCupidLogo->SubregionCount() >= 3)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidLogo, 2,
					CP_X(rc - 20), CP_Y(11));
		}
		else
		{
			DrawHeart(rc - 10, 18, 3, CP_RGB_PINK);
		}
		if (IsGold())
		{
			FillRounded(rc + 12, 12, 36, 13, CP_RGB_GOLD, 3,
					FROMRGB(66, 30, 42));
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, rc + 30, 14,
					"GOLD");
		}
		// the wordmark: CUPID, with hearts for punctuation
		{
			static const char* const letters[5] =
				{ "C", "U", "P", "I", "D" };
			INT32 total = 0;
			for (const char* const l : letters)
			{
				total += StringPixLength(l, FONT16ARIAL);
			}
			total += 5 * 9 + 5; // heart gaps, plus the bold strike
			INT32 x = rc - total / 2;
			for (int i = 0; i < 5; ++i)
			{
				PrintAt(FONT16ARIAL, FONT_NEARBLACK, x + 1, 52,
						letters[i]);
				PrintAt(FONT16ARIAL, FONT_MCOLOR_WHITE, x, 51,
						letters[i]);
				PrintAt(FONT16ARIAL, FONT_MCOLOR_WHITE, x + 1, 51,
						letters[i]);
				x += StringPixLength(letters[i], FONT16ARIAL) + 1;
				DrawHeartDot(x + 2, 60, CP_RGB_PINK);
				x += 9;
			}
		}

		// the menu
		for (int i = 0; i < 4; ++i)
		{
			const INT32 y = 78 + i * 29;
			const bool active =
				(i == 0 && (gCupidPage == CPP_DECK ||
					(gCupidPage == CPP_DETAIL &&
					 gCupidDetailFrom == CPP_DECK))) ||
				(i == 1 && (gCupidPage == CPP_MATCHES ||
					gCupidPage == CPP_SPLASH ||
					(gCupidPage == CPP_DETAIL &&
					 gCupidDetailFrom == CPP_MATCHES))) ||
				(i == 2 && gCupidPage == CPP_LOUNGE) ||
				(i == 3 && gCupidPage == CPP_ME);
			if (active)
			{
				GelPill(CP_LCOL_X + 6, y, CP_COL_W - 12, 24, CP_RGB_PINK,
						CP_RGB_PINK_LITE, CP_RGB_PINK_DK, CP_RGB_BLUE_PALE);
			}
			else
			{
				const bool hov = Hover(gCupidTabRegion[i]);
				GelPill(CP_LCOL_X + 6, y, CP_COL_W - 12, 24,
						hov ? CP_RGB_GLOSS : CP_RGB_CARD,
						hov ? CP_RGB_GLOSS : CP_RGB_CARD_LITE,
						CP_RGB_BLUE_DK, CP_RGB_BLUE_PALE);
			}
			ST::string label =
				i == 0 ? ST::string(T(CPS_TAB_DECK))
				: i == 1 ? ST::string(T(CPS_TAB_MATCHES))
				: i == 2 ? ST::string("LOUNGE")
				: ST::string(T(CPS_TAB_ME));
			if (i == 1)
			{
				const int n = int(AllMatches().size());
				if (n > 0) label = ST::format("{} ({})", label, n);
			}
			// the browse grid, the heart, the lurker's eye, the clipboard
			static const UINT16 icons[4] = { 1, 6, 3, 2 };
			if (guiCupidIcons)
			{
				BltVideoObject(FRAME_BUFFER, guiCupidIcons, icons[i],
						CP_X(CP_LCOL_X + 14), CP_Y(y + 5));
			}
			PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
					CP_LCOL_X + 32, y + 8, label);
		}

		// vitals - spacing does the separating now
		int online = 0;
		for (const Member& m : gCupidRoster)
		{
			if (!m.locked && MemberStatus(m.pid) == CPS_STATUS_ONLINE)
			{
				++online;
			}
		}
		{
			const INT32 vx = CP_LCOL_X + 14;
			const INT32 vr = CP_LCOL_X + CP_COL_W - 14;
			auto vital = [&](INT32 y, const ST::string& label,
					const ST::string& val, UINT8 ink)
			{
				PrintAt(FONT10ARIAL, FONT_GRAY2, vx, y, label);
				PrintAt(FONT10ARIALBOLD, ink,
						vr - StringPixLength(val, FONT10ARIALBOLD), y,
						val);
			};
			vital(202, "online", ST::format("{}", online), FONT_LTGREEN);
			vital(215, gfCupidGerman ? "Kuesse" : "kisses",
					IsGold() ? ST::string("GOLD")
						 : ST::format("{}", gCupidPersist.ubLikesLeft),
					IsGold() ? FONT_MCOLOR_LTYELLOW
					: CanLike() ? FONT_MCOLOR_WHITE : FONT_LTRED);
		}

		// Speck's ticker rides a darker inset bar, marquee fashion
		FillRect(CP_LCOL_X + 8, 254, CP_COL_W - 16, 19, CP_RGB_INK);
		FillRect(CP_LCOL_X + 9, 255, CP_COL_W - 18, 17,
				FROMRGB(26, 20, 28));
		int say = giCupidTicker;
		if (say == CPS_TICKER_DEFAULT)
		{
			if (IsSiteSunday()) say = CPS_TICKER_SUNDAY;
			else if (SpeckHasGrudge()) say = CPS_TICKER_DEBT;
			else if (GetWorldDay() % 3 == 2)
			{
				for (const Member& m : gCupidRoster)
				{
					if (!m.locked && MemberIsDead(m.pid))
					{
						say = CPS_TICKER_ATTRITION;
						break;
					}
				}
			}
		}
		{
			// the marquee: one endless line crawling right to left, plus
			// the era's obligatory safety notice and the house credo
			const ST::string chain = ST::format("{}  +++  {}  +++  {}",
					T(CupidStr(say)), T(CPS_TICKER_SAFETY),
					T(CPS_TICKER_CREDO));
			const INT32 bx = CP_LCOL_X + 10;
			const INT32 bw = CP_COL_W - 20;
			const INT32 cw = StringPixLength(chain, FONT10ARIAL);
			const INT32 off = INT32((GetJA2Clock() / 66)
					% UINT32(cw + bw));
			SetFontDestBuffer(FRAME_BUFFER, CP_X(bx), CP_Y(256),
					CP_X(bx + bw), CP_Y(271));
			PrintAt(FONT10ARIAL, FONT_GRAY2, bx + bw - off, 259, chain);
			SetFontDestBuffer(FRAME_BUFFER);
		}

		// the odometer sinks to the foot, where counters belong
		{
			char buf[8];
			snprintf(buf, sizeof(buf), "%06u",
					4000 + unsigned(gCupidPersist.usViews) * 7 +
					unsigned(GetWorldDay()) * 13);
			for (int i = 0; i < 6; ++i)
			{
				const INT32 dx = rc - 39 + i * 13;
				FillRect(dx, 340, 12, 19, CP_RGB_INK);
				FillRect(dx + 1, 341, 10, 17, FROMRGB(24, 20, 26));
				const char d[2] = { buf[i], 0 };
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, dx + 6,
						345, d);
			}
		}

		// the certificate and the mailbox close the column
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTYELLOW, rc, 365,
				"Y2K OK");
		// the mailbox doubles as the door to the about page
		const bool wmHov = Hover(gCupidWebmasterRegion);
		PrintCentred(FONT10ARIAL, wmHov ? FONT_MCOLOR_WHITE : FONT_GRAY4,
				rc, 378, "webmaster@mk.an");
		if (wmHov)
		{
			const INT32 ww = StringPixLength("webmaster@mk.an",
					FONT10ARIAL);
			FillRect(rc - ww / 2, 388, ww, 1, CP_RGB_PINK);
		}
	}

	// The advertisers rotate in place, as their contracts demanded: a
	// fresh creative every fifteen seconds, the Parlour among them. The
	// day offsets the order so no advertiser always goes first.
	int BannerToday()
	{
		const int b =
			int((GetWorldDay() * 3 + GetJA2Clock() / 15000) % 6);
		return b == 5 && IsGold() ? 0 : b;
	}
	bool BannerIsParlour() { return BannerToday() == 0; }

	void RenderSideAds()
	{
		// the 468x60 of its day: one landscape leaderboard under the card
		const INT32 ax = CP_CARD_X, ay = CP_AD_Y;
		const INT32 aw = CP_CARD_W, ah = CP_AD_H;
		DropShadow(ax, ay, aw, ah);
		const int today = BannerToday();
		{
			// every advertiser gets the same clean furniture: square
			// plate, headline, one line of copy, one honest pill
			struct AdSpec
			{
				UINT32 fill, edge, pill, pillLite, pillDk;
				const char* head[2];
				const char* sub[2];
				const char* cta[2];
			};
			static const AdSpec specs[6] =
			{
				{ FROMRGB(88, 22, 26), FROMRGB(56, 12, 16),
				  FROMRGB(190, 150, 70), FROMRGB(224, 192, 122),
				  FROMRGB(130, 96, 44),
				  { "SAN MONA MAHJONG PARLOUR",
				    "SAN MONA MAHJONG-SALON" },
				  { "tired of love? try luck.",
				    "muede von der liebe? versuch glueck." },
				  { "VISIT", "BESUCH" } },
				{ CP_RGB_PINK_PALE, CP_RGB_PINK,
				  CP_RGB_PINK, CP_RGB_PINK_LITE, CP_RGB_PINK_DK,
				  { "SAY IT WITH FLOWERS", "SAG ES MIT BLUMEN" },
				  { "matched? don't just sit there.",
				    "gematcht? nicht nur dasitzen." },
				  { "ORDER", "BESTELLEN" } },
				{ FROMRGB(42, 34, 22), FROMRGB(158, 128, 62),
				  CP_RGB_GOLD, CP_RGB_GOLD_LITE, FROMRGB(150, 112, 48),
				  { "BOBBY RAY'S", "BOBBY RAY'S" },
				  { "guns && gear. impress your date.",
				    "waffen && ausruestung fuers date." },
				  { "SHOP", "SHOPPEN" } },
				{ FROMRGB(30, 30, 36), FROMRGB(112, 112, 124),
				  CP_RGB_GREY, FROMRGB(170, 166, 172), FROMRGB(84, 80, 88),
				  { "McGILLICUTTY'S MORTUARY", "McGILLICUTTY'S" },
				  { "plan ahead - couples rates.",
				    "vorausplanen - paartarife." },
				  { "VISIT", "BESUCHEN" } },
				{ FROMRGB(18, 34, 46), FROMRGB(92, 132, 154),
				  FROMRGB(74, 122, 148), FROMRGB(128, 168, 188),
				  FROMRGB(44, 82, 104),
				  { "LIFE IS AN ADVENTURE...", "DAS LEBEN IST ABENTEUER..." },
				  { "insure the heart. and the rest.",
				    "das herz versichern. und den rest." },
				  { "QUOTE", "ANGEBOT" } },
				{ FROMRGB(58, 44, 16), CP_RGB_GOLD,
				  CP_RGB_GOLD, CP_RGB_GOLD_LITE, FROMRGB(150, 112, 48),
				  { "C.U.P.I.D. GOLD", "C.U.P.I.D. GOLD" },
				  { "unlimited kisses. you deserve billing.",
				    "unbegrenzte kuesse. sie verdienen eine rechnung." },
				  { "GET GOLD", "GOLD HOLEN" } },
			};
			const AdSpec& S = specs[today];
			const int lang = gfCupidGerman ? 1 : 0;
			FillCard(ax, ay, aw, ah, S.fill, S.edge, CP_RGB_BG);
			if (today == 0 && guiCupidDragon &&
			    guiCupidDragon->SubregionCount() > 6)
			{
				// the house dragon, watermarked behind the copy
				BltVideoObject(FRAME_BUFFER, guiCupidDragon, 6,
						CP_X(ax + 160), CP_Y(ay - 2));
			}
			PrintAt(FONT10ARIAL, FONT_GRAY4, ax + aw - 22, ay + 4, "AD");
			// the creative, drawn in-house, in a square plate at the
			// left edge; the copy hangs off it
			FillRounded(ax + 6, ay + 6, 48, 48, Darken(S.fill, 14), 3,
					S.fill);
			const INT32 gx = ax + 30;
			switch (today)
			{
				case 0: // the parlour: real tiles from the house set
					if (guiCupidTiles &&
					    guiCupidTiles->SubregionCount() > 11)
					{
						BltVideoObject(FRAME_BUFFER, guiCupidTiles, 2,
								CP_X(gx - 21), CP_Y(ay + 15));
						BltVideoObject(FRAME_BUFFER, guiCupidTiles, 11,
								CP_X(gx + 2), CP_Y(ay + 15));
					}
					else
					{
						for (int i = 0; i < 2; ++i)
						{
							FillRounded(gx - 18 + i * 20, ay + 16, 16,
									24, CP_RGB_MAT, 2, S.fill);
						}
					}
					break;
				case 1: // the florist
					DrawHeart(gx - 14, ay + 10, 3, CP_RGB_PINK);
					FillRect(gx - 4, ay + 30, 3, 16,
							FROMRGB(88, 138, 74));
					FillRect(gx - 10, ay + 36, 8, 3,
							FROMRGB(88, 138, 74));
					break;
				case 2: // bobby ray's: cartridges at attention
					for (int i = 0; i < 3; ++i)
					{
						FillRounded(gx - 22 + i * 16, ay + 18, 10, 26,
								CP_RGB_GOLD, 3, S.fill);
						FillRect(gx - 20 + i * 16, ay + 12, 6, 6,
								FROMRGB(150, 112, 48));
					}
					break;
				case 3: // the mortuary: the one symbol that needs no copy
					FillRect(gx - 3, ay + 12, 6, 34,
							FROMRGB(170, 170, 178));
					FillRect(gx - 13, ay + 21, 26, 6,
							FROMRGB(170, 170, 178));
					break;
				case 4: // the insurers: an umbrella, of course
					DrawDisc(gx, ay + 30, 16, FROMRGB(128, 168, 188));
					FillRect(gx - 17, ay + 30, 35, 18, S.fill);
					FillRect(gx - 1, ay + 28, 3, 20,
							FROMRGB(200, 208, 214));
					FillRect(gx + 2, ay + 46, 5, 3,
							FROMRGB(200, 208, 214));
					break;
				default: // the house: its own pierced heart
					if (guiCupidLogo)
					{
						BltVideoObject(FRAME_BUFFER, guiCupidLogo, 0,
								CP_X(gx - 11), CP_Y(ay + 18));
					}
					break;
			}
			const INT32 copyX = ax + 66;
			PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, copyX, ay + 15,
					S.head[lang]);
			DisplayWrappedString(UINT16(CP_X(copyX)), UINT16(CP_Y(ay + 31)),
					UINT16(ax + aw - 84 - copyX), 2, FONT10ARIAL,
					FONT_GRAY1, S.sub[lang], FONT_MCOLOR_BLACK,
					LEFT_JUSTIFIED);
			GelPill(ax + aw - 78, ay + 18, 62, 22, S.pill, S.pillLite,
					S.pillDk, S.fill);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
					ax + aw - 47, ay + 26, S.cta[lang]);
		}

		// the era's system requirements, stated with pride
		PrintCentred(FONT10ARIAL, FONT_GRAY4, ax + aw / 2, 384,
				gfCupidGerman ? "optimiert fuer 800x600"
					      : "best viewed in 800x600");
	}

	// a small outlined pill, the interest chip of the era to come
	INT32 DrawChip(INT32 x, INT32 y, const ST::string& text, UINT32 edge)
	{
		const INT32 w = StringPixLength(text, FONT10ARIAL) + 10;
		FillRounded(x, y, w, 15, edge, 2, CP_RGB_CARD);
		FillRounded(x + 1, y + 1, w - 2, 13, CP_RGB_CARD, 2, edge);
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, x + w / 2, y + 3, text);
		return w;
	}

	// cut a text so it wraps to at most maxLines rows of the given width,
	// with the era-appropriate ellipsis
	ST::string ClampLines(const ST::string& text, INT32 w, int maxLines)
	{
		const UINT16 limit = UINT16(maxLines * 13);
		if (IanWrappedStringHeight(UINT16(w), 2, FONT10ARIAL, text) <= limit)
		{
			return text;
		}
		ST::string cut = text;
		while (cut.size() > 4)
		{
			cut = cut.left(cut.size() - 1);
			const ST::string probe = ST::format("{}...", cut);
			if (IanWrappedStringHeight(UINT16(w), 2, FONT10ARIAL, probe)
					<= limit)
			{
				return probe;
			}
		}
		return text;
	}

	// the esoteric layer: every member has a sign, the algorithm consults
	// the heavens, and the heavens are seeded
	const char* const CUPID_SIGNS[2][12] =
	{
		{ "Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo", "Libra",
		  "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces" },
		{ "Widder", "Stier", "Zwillinge", "Krebs", "Loewe", "Jungfrau",
		  "Waage", "Skorpion", "Schuetze", "Steinbock", "Wassermann",
		  "Fische" },
	};

	const char* const CUPID_COSMIC[2][6] =
	{
		{ "\"The stars incline; they do not compel. Tonight, they incline.\"",
		  "\"Mercury is retrograde. Proceed, but keep your receipts.\"",
		  "\"Two fixed signs. Somebody must yield. It will not be you.\"",
		  "\"A rare alignment. The heavens have cleared their schedule.\"",
		  "\"The moon abstains from comment. That is usually a yes.\"",
		  "\"Written in the stars, in very fine print.\"" },
		{ "\"Die Sterne neigen, sie zwingen nicht. Heute neigen sie.\"",
		  "\"Merkur ist ruecklaeufig. Weitermachen, Belege aufheben.\"",
		  "\"Zwei fixe Zeichen. Jemand muss nachgeben. Sie nicht.\"",
		  "\"Eine seltene Konstellation. Der Himmel hat Zeit.\"",
		  "\"Der Mond enthaelt sich. Das heisst meistens ja.\"",
		  "\"In den Sternen geschrieben, im Kleingedruckten.\"" },
	};

	int SignOf(ProfileID pid) { return (int(pid) * 7 + 3) % 12; }

	// the LIKE and PASS stamps ride the flying card
	void RenderVerdictStamp(INT32 cardX)
	{
		if (giCupidFlyDir > 0)
		{
			FillRounded(cardX + 10, CP_CARD_Y + 12, 52, 20, CP_RGB_LIKE, 3,
					CP_RGB_BG);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cardX + 36,
					CP_CARD_Y + 18, T(CPS_STAMP_LIKE));
		}
		else if (giCupidFlyDir < 0)
		{
			FillRounded(cardX + CP_CARD_W - 62, CP_CARD_Y + 12, 52, 20,
					CP_RGB_NOPE, 3, CP_RGB_BG);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
					cardX + CP_CARD_W - 36, CP_CARD_Y + 18,
					T(CPS_STAMP_NOPE));
		}
	}

	void RenderMemberCard(const Card& card, INT32 cardX, INT32 cardY,
			INT32 cardH, CardView view)
	{
		MERCPROFILESTRUCT const& p = GetProfile(card.pid);
		const int idx = RosterIndexOf(card.pid);
		const bool self = view == CV_SELF;
		const bool merc = !self && idx >= 0 &&
				gCupidRoster[size_t(idx)].merc;

		const int plate = cardH == 302 ? 1 : cardH == 306 ? 2
				: cardH == 300 ? 3 : -1;
		if (guiCupidPanels && plate > 0)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidPanels, UINT16(plate),
					CP_X(cardX), CP_Y(cardY));
		}
		else
		{
			FillCard(cardX, cardY, CP_CARD_W, cardH, CP_RGB_CARD,
					CP_RGB_BLUE_DK, CP_RGB_BG);
		}

		// the profile head: one darker band that owns the identity -
		// photo, name, score, headline, meter and chips as one unit
		FillRounded(cardX + 4, cardY + 4, 306, 138,
				FROMRGB(52, 13, 19), 4, CP_RGB_CARD);
		Stipple(cardX + 4, cardY + 4, 306, 138, FROMRGB(64, 20, 27),
				FROMRGB(43, 9, 14));
		FillRect(cardX + 6, cardY + 140, 302, 1,
				Darken(CP_RGB_CARD, 26));

		// the photo frame, snug to the artwork itself
		FillRounded(cardX + 8, cardY + 8, CP_PHOTO_W + 4, CP_PHOTO_H + 4,
				CP_RGB_INK, 3, FROMRGB(52, 13, 19));
		const INT32 photoX = cardX + 10;
		const INT32 photoY = cardY + 10;
		SGPVObject* big = self ? guiCupidSelfBig : BigFaceFor(card.pid);
		if (big)
		{
			BltVideoObject(FRAME_BUFFER, big, 0, CP_X(photoX),
					CP_Y(photoY));
			// the 28.8k curtain: everything below the download line is
			// still on its way, with an interlace fringe above it
			if (view == CV_DECK && giCupidPhotoReveal < CP_PHOTO_H)
			{
				const INT32 edge = photoY + giCupidPhotoReveal;
				FillRect(photoX, edge, CP_PHOTO_W,
						CP_PHOTO_H - giCupidPhotoReveal, CP_RGB_INK);
				for (INT32 fy = edge - 12; fy < edge; fy += 2)
				{
					if (fy >= photoY)
					{
						FillRect(photoX, fy, CP_PHOTO_W, 1, CP_RGB_INK);
					}
				}
				// and the vertical comb the era's interlacing cut
				{
					const INT32 top = std::max(photoY, edge - 12);
					const INT32 combH = edge - top;
					if (combH > 0)
					{
						for (INT32 fx = photoX + 1;
						     fx < photoX + CP_PHOTO_W; fx += 2)
						{
							FillRect(fx, top, 1, combH, CP_RGB_INK);
						}
					}
				}
			}
		}
		else
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cardX + 65,
					cardY + 60, T(CPS_NO_PHOTO));
		}
		if (!self && big && BitGet(gCupidPersist.ubPassed, card.pid))
		{
			// no killed portraits exist in the archive, so the site
			// serves the living one with the life dimmed out of it
			FRAME_BUFFER->ShadowRect(CP_X(photoX), CP_Y(photoY),
					CP_X(photoX + CP_PHOTO_W - 1),
					CP_Y(photoY + CP_PHOTO_H - 1));
			FRAME_BUFFER->ShadowRect(CP_X(photoX), CP_Y(photoY),
					CP_X(photoX + CP_PHOTO_W - 1),
					CP_Y(photoY + CP_PHOTO_H - 1));
		}

		// the action rail: the card's own vertical toolbar, set a full
		// step darker so the controls read as a band of their own
		FillRounded(cardX + 318, cardY + 4, CP_CARD_W - 322, cardH - 8,
				FROMRGB(38, 9, 14), 4, CP_RGB_CARD);
		Stipple(cardX + 318, cardY + 4, CP_CARD_W - 322, cardH - 8,
				FROMRGB(50, 16, 22), FROMRGB(30, 5, 9));

		// the head's text column, everything relating to the face beside it
		const INT32 rx = cardX + 122;
		const INT32 rw = 310 - 122 - 8;
		PrintAt(FONT14ARIAL, FONT_MCOLOR_WHITE, rx, cardY + 12,
				p.zNickname);
		const bool showMatch = !self && PlayerHasProfile();
		DatingGame::Match match;
		if (showMatch) match = MatchWith(card.pid);
		if (showMatch)
		{
			const ST::string pct =
				ST::format("{}% MATCH", match.percent);
			PrintAt(FONT10ARIALBOLD, MatchColour(match.percent),
					cardX + 302 -
						StringPixLength(pct, FONT10ARIALBOLD),
					cardY + 14, pct);
		}
		else if (self)
		{
			const ST::string you = gfCupidGerman ? "(das sind Sie)"
							     : "(this is you)";
			PrintAt(FONT10ARIAL, FONT_GRAY2,
					cardX + 302 -
						StringPixLength(you, FONT10ARIAL),
					cardY + 14, you);
			PrintAt(FONT10ARIAL, FONT_GRAY4, cardX + 122, cardY + 50,
					gfCupidGerman
						? "(zitat/beschreibung anklickbar)"
						: "(quote and summary are clickable)");
		}
		int att = p.bAttitude >= 0 && p.bAttitude < NUM_ATTITUDES
					? p.bAttitude : 0;
		int trait = p.bPersonalityTrait >= 0 && p.bPersonalityTrait < 8
					? p.bPersonalityTrait : 0;
		if (self)
		{
			// your card runs off your answer sheet, like everyone else's
			const DatingGame::Profile mine = BuildPlayerProfile();
			att = mine.attitude >= 0 && mine.attitude < NUM_ATTITUDES
					? mine.attitude : 0;
			trait = mine.trait >= 0 && mine.trait < 8 ? mine.trait : 0;
		}
		PrintAt(FONT10ARIAL, FONT_GRAY2, rx, cardY + 31,
				HeadlineFor(self ? SelfHeadlineIdx(att) : att,
						card.pid));
		if (showMatch)
		{
			DrawMeter(rx + 1, cardY + 50, 120, match.percent,
					match.percent >= 75 ? CP_RGB_LIKE
					: match.percent >= 50 ? CP_RGB_GOLD
					: CP_RGB_NOPE);
			if (match.answered > 0)
			{
				PrintAt(FONT10ARIAL, FONT_GRAY2, rx, cardY + 62,
						ST::format(T(CPS_AGREE_ON), match.agree,
								match.answered));
			}
		}

		const INT32 tw = rw;

		const char* flavor = self ? nullptr : FlavorFor(card.pid);
		const char* bio = flavor ? flavor
			: (merc ? CUPID_SUMMARY_MERC : CUPID_SUMMARY_AIM)
				[gfCupidGerman ? 1 : 0][self ? SelfBioIdx(att) : att];
		const ST::string bioQ = ST::format("\"{}\"", bio);
		const ST::string looking = ST::format(T(CPS_LOOKING_FOR),
				LookingFor(att, card.pid));

		ST::string breakers;
		if (!self)
		{
			for (int i = 0; i < 2; ++i)
			{
				const INT8 hated = p.bHated[i];
				if (hated < 0) continue;
				if (!breakers.empty()) breakers += ", ";
				breakers += GetProfile(ProfileID(hated)).zNickname;
			}
		}
		const int blockedBy = self ? 0 : CountBlockedBy(card.pid);
		const CupidStr status = self ? CPS_STATUS_ONLINE
					     : MemberStatus(card.pid);

		// the chips close the head band
		{
			ST::string chips[3];
			UINT32 edges[3];
			chips[0] = CUPID_TRAIT_SPIN[gfCupidGerman ? 1 : 0][trait];
			edges[0] = CP_RGB_PINK;
			chips[1] = status == CPS_STATUS_ONLINE ? "online"
					: (gfCupidGerman ? "weg" : "away");
			edges[1] = status == CPS_STATUS_ONLINE
					? CP_RGB_LIKE : CP_RGB_GREY;
			if (self)
			{
				chips[2] = "i.m.p.";
				edges[2] = CP_RGB_GOLD;
			}
			else
			{
				chips[2] = merc ? T(CPS_UNVERIFIED)
						: (gfCupidGerman ? "geprueft" : "verified");
				edges[2] = merc ? CP_RGB_GREY : CP_RGB_GOLD;
			}
			INT32 cxp = rx;
			for (int i = 0; i < 3; ++i)
			{
				const INT32 w = StringPixLength(chips[i],
						FONT10ARIAL) + 10;
				DrawChip(cxp, cardY + 118, chips[i], edges[i]);
				cxp += w + 4;
			}
		}

		// the column beneath the photo: the personals-ad vitals, the
		// numbers every 1999 ad led with
		{
			const INT32 lx = cardX + 10;
			const int lang = gfCupidGerman ? 1 : 0;
			INT32 ly = cardY + 152;
			PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, lx, ly,
					gfCupidGerman ? "ECKDATEN" : "VITALS");
			ly += 15;
			static const char* const smoke[2][3] =
			{
				{ "no", "in cover", "socially" },
				{ "nein", "in deckung", "gesellig" },
			};
			static const char* const drink[2][4] =
			{
				{ "no", "canteen", "rarely", "yes" },
				{ "nein", "feldflasche", "selten", "ja" },
			};
			const char* const labs[2][6] =
			{
				{ "age:", "sign:", "smokes:", "drinks:", "scars:",
				  "kills:" },
				{ "alter:", "zeichen:", "raucht:", "trinkt:", "narben:",
				  "treffer:" },
			};
			const ST::string vals[6] =
			{
				ST::format("{}", 26 + (int(card.pid) * 5) % 19),
				CUPID_SIGNS[lang][SignOf(card.pid)],
				smoke[lang][card.pid % 3],
				drink[lang][card.pid % 4],
				gfCupidGerman ? "ja" : "yes",
				gfCupidGerman ? "(geheim)" : "(classified)",
			};
			for (int i = 0; i < 6; ++i)
			{
				PrintAt(FONT10ARIAL, FONT_GRAY4, lx, ly, labs[lang][i]);
				PrintAt(FONT10ARIAL, FONT_GRAY1,
						lx + StringPixLength(labs[lang][i],
								FONT10ARIAL) + 4, ly,
						ClampLines(vals[i], 100, 1));
				ly += 13;
			}
			FillRect(lx, ly + 3, 106, 1, CP_RGB_CARD_DIM);
			ly += 9;
			const ST::string act =
				status == CPS_STATUS_GONE ? ST::string(T(CPS_ACTIVE_LONG))
				: status == CPS_STATUS_ONLINE ||
				  status == CPS_STATUS_PAYROLL
					? ST::string(T(CPS_ACTIVE_24))
					: ST::format(T(CPS_ACTIVE_DAYS),
							2 + (card.pid * 7) % 5);
			ly += DisplayWrappedString(UINT16(CP_X(lx)), UINT16(CP_Y(ly)),
					106, 2, FONT10ARIAL, FONT_GRAY4,
					ClampLines(act, 106, 2), FONT_MCOLOR_BLACK,
					LEFT_JUSTIFIED) + 2;
			PrintAt(FONT10ARIAL, FONT_GRAY4, lx, ly,
					ST::format(gfCupidGerman ? "Mitglied {}"
								 : "member no. {}",
							1000 + int(card.pid) * 7));
		}

		// the tabbed pane: a framed panel the active tab grows out of,
		// so tab and content read as one piece of furniture
		{
			const UINT32 paneFill = FROMRGB(52, 13, 19);
			FillRounded(cardX + 114, cardY + 162, 196, cardH - 168,
					paneFill, 3, CP_RGB_CARD);
			Stipple(cardX + 114, cardY + 162, 196, cardH - 168,
					FROMRGB(64, 20, 27), FROMRGB(43, 9, 14));
			FillRect(cardX + 114, cardY + 162, 196, 1,
					Darken(CP_RGB_CARD, 26));
			static const char* const tabName[2][3] =
			{
				{ "THE AD", "MATCH", "STARS" },
				{ "ANZEIGE", "MATCH", "STERNE" },
			};
			const int tl = gfCupidGerman ? 1 : 0;
			for (int i = 0; i < 3; ++i)
			{
				const INT32 tx = cardX + 114 + i * 60;
				const bool on = i == giCupidCardTab;
				const bool hov = Hover(gCupidCardTabRegion[i]);
				if (on)
				{
					// the active tab merges into the panel beneath it
					FillRounded(tx, cardY + 146, 58, 18, paneFill, 3,
							CP_RGB_CARD);
					FillRect(tx + 3, cardY + 146, 52, 1, CP_RGB_PINK);
				}
				else
				{
					FillRounded(tx, cardY + 146, 58, 15,
							hov ? CP_RGB_BLUE_LITE : CP_RGB_INK, 3,
							CP_RGB_CARD);
				}
				PrintCentred(FONT10ARIAL,
						on ? FONT_MCOLOR_WHITE : FONT_GRAY4,
						tx + 29, cardY + 150, tabName[tl][i]);
			}
		}

		INT32 dy = cardY + 172;
		if (giCupidCardTab == 0)
		{
			// the ad itself
			dy += DisplayWrappedString(UINT16(CP_X(rx)), UINT16(CP_Y(dy)),
					UINT16(tw), 2, FONT10ARIAL, FONT_GRAY1, bioQ,
					FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 6;
			dy += DisplayWrappedString(UINT16(CP_X(rx)), UINT16(CP_Y(dy)),
					UINT16(tw), 2, FONT10ARIAL, FONT_GRAY2, looking,
					FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 6;
			if (!breakers.empty())
			{
				dy += DisplayWrappedString(UINT16(CP_X(rx)),
						UINT16(CP_Y(dy)), UINT16(tw), 2,
						FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
						ST::format(T(CPS_DEAL_BREAKERS), breakers),
						FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 4;
			}
			if (blockedBy > 0)
			{
				PrintAt(FONT10ARIAL, FONT_GRAY4, rx, dy,
						ST::format(T(CPS_BLOCKED_BY), blockedBy));
			}
		}
		else if (giCupidCardTab == 1)
		{
			if (showMatch)
			{
				bool said = false;
				if (match.bestQ >= 0)
				{
					PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, rx, dy,
							T(CPS_YOU_AGREED));
					dy += 13;
					dy += DisplayWrappedString(UINT16(CP_X(rx)),
							UINT16(CP_Y(dy)), UINT16(tw), 2,
							FONT10ARIAL, FONT_GRAY1,
							QuizAnswer(match.bestQ,
									GetAnswer(match.bestQ)),
							FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 6;
					said = true;
				}
				if (match.worstQ >= 0)
				{
					PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, rx, dy,
							T(CPS_YOU_DIFFER));
					dy += 13;
					DisplayWrappedString(UINT16(CP_X(rx)),
							UINT16(CP_Y(dy)), UINT16(tw), 2,
							FONT10ARIAL, FONT_GRAY1,
							QuizQuestion(match.worstQ),
							FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
					said = true;
				}
				if (!said)
				{
					DisplayWrappedString(UINT16(CP_X(rx)),
							UINT16(CP_Y(dy)), UINT16(tw), 2,
							FONT10ARIAL, FONT_GRAY2, gfCupidGerman
								? "noch nicht genug gemeinsame "
								  "antworten."
								: "not enough shared answers yet.",
							FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
				}
			}
			else
			{
				DisplayWrappedString(UINT16(CP_X(rx)), UINT16(CP_Y(dy)),
						UINT16(tw), 2, FONT10ARIAL, FONT_GRAY2,
						self ? (gfCupidGerman
							? "der algorithmus weigert sich, sie "
							  "gegen sich selbst zu bewerten."
							: "the algorithm refuses to rate you "
							  "against yourself.")
						: (gfCupidGerman
							? "fuellen sie den fragebogen aus, um "
							  "ihre passung zu sehen."
							: "take the questionnaire to see your "
							  "match."),
						FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
			}
		}
		else
		{
			// the esoteric layer: the heavens weigh in, seeded
			const int lang = gfCupidGerman ? 1 : 0;
			ST::string line;
			if (self)
			{
				line = ST::format(gfCupidGerman ? "Ihr Zeichen: {}"
								: "Your sign: {}",
						CUPID_SIGNS[lang][SignOf(card.pid)]);
			}
			else if (PlayerHasProfile())
			{
				line = ST::format(gfCupidGerman ? "Sie: {} - {}: {}"
								: "You: {} - {}: {}",
						CUPID_SIGNS[lang][SignOf(PlayerImpPid())],
						p.zNickname,
						CUPID_SIGNS[lang][SignOf(card.pid)]);
			}
			else
			{
				line = ST::format("{}: {}", p.zNickname,
						CUPID_SIGNS[lang][SignOf(card.pid)]);
			}
			PrintAt(FONT10ARIAL, FONT_GRAY2, rx, dy, line);
			dy += 15;
			const ST::string omen = CUPID_COSMIC[lang]
				[DatingGame::ChemistrySeed(PlayerImpPid(),
						card.pid) % 6];
			DisplayWrappedString(UINT16(CP_X(rx)), UINT16(CP_Y(dy)),
					UINT16(tw), 2, FONT10ARIAL, FONT_GRAY1, omen,
					FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		}

		// the rubber stamp: a given verdict shows on the photo itself
		if (!self)
		{
			const bool ringed = BitGet(gCupidPersist.ubProposed,
					card.pid);
			const bool wed = ringed && IsMatched(card.pid);
			const bool kissed = BitGet(gCupidPersist.ubLiked, card.pid);
			const bool passed = BitGet(gCupidPersist.ubPassed, card.pid);
			if (ringed || kissed || passed)
			{
				const ST::string stamp = wed
					? (gfCupidGerman ? "VERLOBT!!" : "ENGAGED!!")
					: ringed
					? (gfCupidGerman ? "ANTRAG LAEUFT" : "PROPOSED")
					: kissed
					? (gfCupidGerman ? "GEKUESST!!" : "KISSED!!")
					: (gfCupidGerman ? "ELIMINIERT" : "KILLED");
				const INT32 sw =
					StringPixLength(stamp, FONT10ARIALBOLD) + 12;
				FillRounded(cardX + 12, cardY + 14, sw, 15,
						wed ? CP_RGB_GOLD
						: ringed ? FROMRGB(150, 116, 52)
						: kissed ? CP_RGB_PINK : CP_RGB_GREY, 3,
						CP_RGB_INK);
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
						cardX + 12 + sw / 2, cardY + 17, stamp);
			}
		}

		// the NEW!! tag: applied on unlock, removed never
		if (idx >= 0 && gCupidRoster[size_t(idx)].fresh &&
		    (GetJA2Clock() / 400) % 2 == 0)
		{
			FillRounded(cardX + 72, cardY + 12, 40, 14,
					CP_RGB_NOPE, 3, CP_RGB_INK);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
					cardX + 92, cardY + 15, "NEW!!");
		}
	}

	void RenderAdCard(const Card& card, INT32 cardX)
	{
		FillCard(cardX, CP_CARD_Y, CP_CARD_W, CP_CARD_H, CP_RGB_CARD,
				CP_RGB_GOLD, CP_RGB_BG);

		// the same head band the members get, so the page's furniture -
		// pager, verdict circles, tally - sits on structure, not on air
		FillRounded(cardX + 4, CP_CARD_Y + 4, CP_CARD_W - 8, 138,
				FROMRGB(60, 24, 20), 4, CP_RGB_CARD);
		Stipple(cardX + 4, CP_CARD_Y + 4, CP_CARD_W - 8, 138,
				FROMRGB(74, 32, 27), FROMRGB(50, 18, 15));
		FillRect(cardX + 6, CP_CARD_Y + 140, CP_CARD_W - 12, 1,
				Darken(CP_RGB_CARD, 26));
		FillRounded(cardX + 8, CP_CARD_Y + 8, 48, 48,
				FROMRGB(44, 16, 14), 3, FROMRGB(60, 24, 20));
		if (guiCupidLogo)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidLogo, 0,
					CP_X(cardX + 21), CP_Y(CP_CARD_Y + 21));
		}

		ST::string headline, copy, byline;
		switch (card.kind)
		{
			case CARD_AD_GOLD:
				headline = T(CPS_AD_GOLD_HEAD);
				copy = IsGold() ? T(CPS_AD_GOLD_OWNED)
						: ST::format(T(CPS_AD_GOLD_BODY), CP_GOLD_PRICE);
				break;
			case CARD_AD_TESTIMONIAL:
				headline = T(CPS_AD_TESTI_HEAD);
				copy = T(CPS_AD_TESTI_BODY);
				byline = T(CPS_AD_TESTI_BY);
				break;
			case CARD_AD_NEWMEMBERS:
				headline = T(CPS_AD_NEW_HEAD);
				copy = T(CPS_AD_NEW_BODY);
				break;
			default:
				headline = T(CPS_END_HEAD);
				copy = T(CPS_END_BODY);
				break;
		}

		// headline in the band, clear of the pager plates
		DisplayWrappedString(UINT16(CP_X(cardX + 66)),
				UINT16(CP_Y(CP_CARD_Y + 16)), UINT16(CP_CARD_W - 178), 2,
				FONT12ARIAL, FONT_MCOLOR_WHITE, headline,
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		// the pitch below the band
		if (card.kind == CARD_AD_GOLD && !IsGold())
		{
			// the feature table, as every 1999 pricing page demanded
			const int lang = gfCupidGerman ? 1 : 0;
			static const char* const rows[2][4][4] =
			{
				{ { "", "FREE", "GOLD", "PLAT" },
				  { "kisses per day", "10", "oo", "oo" },
				  { "see who kissed you", "--", "YES", "YES" },
				  { "speck's respect", "--", "some", "$100 worth" } },
				{ { "", "GRATIS", "GOLD", "PLAT" },
				  { "Kuesse pro Tag", "10", "oo", "oo" },
				  { "sehen, wer kuesste", "--", "JA", "JA" },
				  { "Specks Respekt", "--", "etwas", "fuer 100 $" } },
			};
			const INT32 colX[4] = { cardX + 20, cardX + 178,
						cardX + 244, cardX + 310 };
			INT32 ty = CP_CARD_Y + 150;
			for (int r = 0; r < 4; ++r)
			{
				if (r % 2 == 1)
				{
					FillRect(cardX + 12, ty - 2, CP_CARD_W - 24, 15,
							Darken(CP_RGB_CARD, 12));
				}
				for (int c = 0; c < 4; ++c)
				{
					const bool head = r == 0;
					if (c == 0)
					{
						PrintAt(head ? FONT10ARIALBOLD : FONT10ARIAL,
								head ? FONT_MCOLOR_WHITE : FONT_GRAY1,
								colX[0], ty, rows[lang][r][c]);
					}
					else
					{
						PrintCentred(
								head ? FONT10ARIALBOLD : FONT10ARIAL,
								head ? FONT_MCOLOR_LTYELLOW
								: FONT_GRAY1,
								colX[c], ty, rows[lang][r][c]);
					}
				}
				ty += 15;
			}
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cardX + CP_CARD_W / 2,
					ty + 4, T(CPS_AD_HINT));

			// two ways to pay: the sensible one and the proud one
			DropShadow(cardX + 12, CP_CARD_Y + CP_CARD_H - 84, 200, 40);
			GelPill(cardX + 12, CP_CARD_Y + CP_CARD_H - 84, 200, 40,
					CP_RGB_GOLD, CP_RGB_GOLD_LITE,
					FROMRGB(150, 112, 48), CP_RGB_CARD);
			PrintCentred(FONT14ARIAL, FONT_MCOLOR_WHITE, cardX + 112,
					CP_CARD_Y + CP_CARD_H - 71,
					ST::format(T(CPS_AD_GOLD_BTN), CP_GOLD_PRICE));
			DropShadow(cardX + 220, CP_CARD_Y + CP_CARD_H - 84, 134, 40);
			GelPill(cardX + 220, CP_CARD_Y + CP_CARD_H - 84, 134, 40,
					FROMRGB(176, 176, 188), FROMRGB(214, 214, 224),
					FROMRGB(96, 96, 108), CP_RGB_CARD);
			PrintCentred(FONT12ARIAL, FONT_NEARBLACK, cardX + 287,
					CP_CARD_Y + CP_CARD_H - 70,
					gfCupidGerman ? "PLATIN - 100 $"
						      : "PLAT - $100");
		}
		else
		{
			INT32 dy = CP_CARD_Y + 152;
			dy += DisplayWrappedString(UINT16(CP_X(cardX + 16)),
					UINT16(CP_Y(dy)), UINT16(CP_CARD_W - 32), 2,
					FONT10ARIAL, FONT_GRAY1, copy, FONT_MCOLOR_BLACK,
					LEFT_JUSTIFIED) + 5;
			if (!byline.empty())
			{
				PrintAt(FONT10ARIAL, FONT_GRAY4, cardX + 16, dy, byline);
			}
		}
		if (card.kind != CARD_END)
		{
			// the way out, stated politely
			const bool hov = Hover(gCupidAdCloseRegion);
			GelPill(cardX + (CP_CARD_W - 140) / 2,
					CP_CARD_Y + CP_CARD_H - 34, 140, 22,
					hov ? CP_RGB_CARD_LITE : CP_RGB_CARD_DIM,
					CP_RGB_CARD_LITE, CP_RGB_GREY, CP_RGB_CARD);
			PrintCentred(FONT10ARIALBOLD, FONT_GRAY1,
					cardX + CP_CARD_W / 2, CP_CARD_Y + CP_CARD_H - 27,
					gfCupidGerman ? "NEIN DANKE" : "NO THANKS");
		}
	}

	void RenderDeck()
	{
		if (!PlayerHasProfile())
		{
			// the pitch card, in the house style of the era's portals:
			// featured profiles under headlines, the lovers of the month,
			// and one big pink promise
			DropShadow(CP_CARD_X, CP_CARD_Y, CP_CARD_W, CP_CARD_H);
			FillCard(CP_CARD_X, CP_CARD_Y, CP_CARD_W, CP_CARD_H, CP_RGB_CARD,
					CP_RGB_PINK, CP_RGB_BG);
			const INT32 cx = CP_CARD_X + CP_CARD_W / 2;
			DrawHeart(cx - 10, CP_CARD_Y + 8, 3, CP_RGB_PINK);
			PrintCentred(FONT12ARIAL, FONT_MCOLOR_WHITE, cx, CP_CARD_Y + 30,
					"C.U.P.I.D.");
			PrintCentred(FONT10ARIAL, FONT_GRAY2, cx, CP_CARD_Y + 44,
					gfCupidGerman
						? "Menschen, fuer die sich das Sterben lohnt."
						: "People worth dying for.");
			FillRect(CP_CARD_X + 18, CP_CARD_Y + 58, CP_CARD_W - 36, 1,
					CP_RGB_CARD_DIM);

			PrintCentred(FONT10ARIALBOLD, FONT_LTRED, cx, CP_CARD_Y + 64,
					T(CPS_FEATURED));
			// two members, mugshot beside their headline, decency-blurred
			int shown = 0;
			INT32 fy = CP_CARD_Y + 78;
			for (const Member& m : gCupidRoster)
			{
				if (shown >= 2) break;
				if (m.locked || MemberIsDead(m.pid)) continue;
				SGPVObject* face = Face33For(m.pid);
				if (!face) continue;
				FillRect(CP_CARD_X + 12, fy - 1, CP_FACE_SM_W + 2,
						CP_FACE_SM_H + 2, CP_RGB_INK);
				BltVideoObject(FRAME_BUFFER, face, 0, CP_X(CP_CARD_X + 13),
						CP_Y(fy));
				BlurOver(CP_CARD_X + 13, fy, CP_FACE_SM_W, CP_FACE_SM_H);
				MERCPROFILESTRUCT const& fp = GetProfile(m.pid);
				DisplayWrappedString(
						UINT16(CP_X(CP_CARD_X + 18 + CP_FACE_SM_W)),
						UINT16(CP_Y(fy + 6)),
						UINT16(CP_CARD_W - 30 - CP_FACE_SM_W), 2,
						FONT10ARIAL, FONT_GRAY1,
						HeadlineFor(fp.bAttitude >= 0 &&
								fp.bAttitude < NUM_ATTITUDES
									? fp.bAttitude : 0, m.pid),
						FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
				fy += CP_FACE_SM_H + 9;
				++shown;
			}

			// the lovers of the month: Flo's whole arc, eventually
			PrintCentred(FONT10ARIALBOLD, FONT_LTRED, cx, fy + 2,
					T(CPS_LOVERS));
			PrintCentred(FONT10ARIAL,
					gubFact[FACT_PC_MARRYING_DARYL_IS_FLO]
						? FONT_MCOLOR_WHITE : FONT_GRAY4,
					cx, fy + 15,
					gubFact[FACT_PC_MARRYING_DARYL_IS_FLO]
						? "Flo && Daryl H.!!" : T(CPS_LOVERS_WATCH));

			GelPill(CP_CARD_X + 20, CP_CARD_Y + CP_CARD_H - 40,
					CP_CARD_W - 40, 26, CP_RGB_PINK, CP_RGB_PINK_LITE,
					CP_RGB_PINK_DK, CP_RGB_CARD);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx,
					CP_CARD_Y + CP_CARD_H - 32,
					gfCupidGerman ? "JETZT ANMELDEN" : "SIGN UP NOW");
			giCupidTicker = CPS_TICKER_NO_PROFILE;
			return;
		}

		// the card underneath peeks out, so the deck reads as a deck.
		// While Speck's offer is open, it borrows the whole stage.
		static const Card kGoldOffer = { CARD_AD_GOLD, 0 };
		const Card& card = gfCupidGoldOffer ? kGoldOffer : CurrentCard();
		if (card.kind != CARD_END &&
		    gCupidDeckPos + 1 < int(gCupidDeck.size()))
		{
			DropShadow(CP_CARD_X + 5, CP_CARD_Y + 5, CP_CARD_W, CP_CARD_H);
			FillCard(CP_CARD_X + 5, CP_CARD_Y + 5, CP_CARD_W, CP_CARD_H,
					CP_RGB_CARD_DIM, CP_RGB_BLUE_DK, CP_RGB_BG);
		}

		const INT32 cardX = CP_CARD_X + giCupidCardDx;
		DropShadow(cardX, CP_CARD_Y, CP_CARD_W, CP_CARD_H);
		if (card.kind == CARD_MEMBER)
		{
			RenderMemberCard(card, cardX, CP_CARD_Y, CP_CARD_H, CV_DECK);
		}
		else
		{
			RenderAdCard(card, cardX);
		}
		RenderVerdictStamp(cardX);

		// the pager: two steel plates at the rail's top - browse first,
		// judge after. Ads have no picture and take no paging.
		if (card.kind == CARD_MEMBER)
		{
			const bool canPrev = gCupidDeckPos > 0;
			const bool canNext = card.kind != CARD_END;
			const UINT32 steel     = FROMRGB(140, 136, 144);
			const UINT32 steelLite = FROMRGB(190, 186, 194);
			const UINT32 steelRing = FROMRGB(70, 66, 74);
			const UINT32 steelDim  = FROMRGB(86, 80, 90);
			const UINT32 ink       = FROMRGB(30, 26, 32);
			GelPill(CP_BTN_X, CP_CARD_Y + 8, CP_BTN_SIZE, 24,
					!canPrev ? steelDim
					: Hover(gCupidPrevRegion) ? steelLite : steel,
					steelLite, steelRing, CP_RGB_INK);
			DrawTri(CP_BTN_X + 14, CP_CARD_Y + 15, 11, false,
					canPrev ? ink : steelRing);
			GelPill(CP_BTN_X, CP_CARD_Y + 34, CP_BTN_SIZE, 24,
					!canNext ? steelDim
					: Hover(gCupidSkipRegion) ? steelLite : steel,
					steelLite, steelRing, CP_RGB_INK);
			DrawTri(CP_BTN_X + 15, CP_CARD_Y + 41, 11, true,
					canNext ? ink : steelRing);
		}

		// the verdict circles: soft contrast at rest, full contrast for
		// the verdict that was chosen - the buttons carry the state
		if (card.kind == CARD_MEMBER)
		{
		const bool ringOn  = BitGet(gCupidPersist.ubProposed, card.pid);
		const bool engaged = ringOn && IsMatched(card.pid);
		const bool selKill = BitGet(gCupidPersist.ubPassed, card.pid);
		const bool selKiss = BitGet(gCupidPersist.ubLiked, card.pid);

		// KILL: grey rim and skull at rest, white when the deed is done
		{
			const bool dis = engaged;
			const UINT32 rim = dis ? FROMRGB(72, 66, 72)
				: selKill ? CP_RGB_MAT : FROMRGB(120, 112, 120);
			const UINT32 face = !dis && Hover(gCupidPassRegion)
					? FROMRGB(50, 42, 50) : FROMRGB(22, 18, 22);
			GelCircle(CP_BTN_X, CP_BTN_KILL_Y, CP_BTN_SIZE, face,
					FROMRGB(56, 48, 56), rim, CP_RGB_CARD);
			DrawSkull(CP_BTN_X + 10, CP_BTN_KILL_Y + 10, rim, face);
			if (selKill)
			{
				FillRect(CP_BTN_X + 8, CP_BTN_KILL_Y + CP_BTN_SIZE + 3,
						20, 2, CP_RGB_MAT);
			}
		}

		// KISS: muted rose at rest, hot pink once given
		{
			const bool usable = !engaged && (selKiss || CanLike());
			const UINT32 rim = !usable ? FROMRGB(96, 70, 80)
				: selKiss ? CP_RGB_PINK_LITE : FROMRGB(170, 74, 112);
			const UINT32 face = selKiss ? FROMRGB(150, 44, 84)
				: usable && Hover(gCupidLikeRegion)
					? FROMRGB(126, 40, 70) : FROMRGB(86, 26, 46);
			GelCircle(CP_BTN_X, CP_BTN_KISS_Y, CP_BTN_SIZE, face,
					FROMRGB(150, 50, 86), rim, CP_RGB_CARD);
			DrawHeart(CP_BTN_X + 11, CP_BTN_KISS_Y + 12, 2,
					selKiss ? CP_RGB_MAT
					: usable ? FROMRGB(212, 110, 148)
						 : FROMRGB(120, 86, 98));
			if (selKiss)
			{
				FillRect(CP_BTN_X + 8, CP_BTN_KISS_Y + CP_BTN_SIZE + 3,
						20, 2, CP_RGB_PINK_LITE);
			}
		}

		// MARRY: the super-like - open to anyone, filed once, blazing
		// gold when the answer came back yes
		{
			const bool marryLive = !ringOn;
			const UINT32 rim = engaged ? CP_RGB_GOLD_LITE
				: ringOn ? CP_RGB_GOLD
				: FROMRGB(178, 142, 74);
			const UINT32 face = engaged ? FROMRGB(140, 106, 40)
				: ringOn ? FROMRGB(96, 76, 34)
				: Hover(gCupidMarryRegion) ? FROMRGB(112, 86, 36)
							   : FROMRGB(56, 48, 44);
			GelCircle(CP_BTN_X, CP_BTN_MARRY_Y, CP_BTN_SIZE, face,
					FROMRGB(128, 100, 44), rim, CP_RGB_CARD);
			DrawRing(CP_BTN_X + 10, CP_BTN_MARRY_Y + 11,
					engaged ? CP_RGB_MAT
					: ringOn ? CP_RGB_GOLD_LITE : CP_RGB_GOLD_LITE,
					face, CP_RGB_MAT);
			if (ringOn)
			{
				FillRect(CP_BTN_X + 8, CP_BTN_MARRY_Y + CP_BTN_SIZE + 3,
						20, 2,
						engaged ? CP_RGB_GOLD_LITE : CP_RGB_GOLD);
			}
			(void)marryLive;
		}

		// the allowance, a small tally under the heart itself
		const ST::string likes = IsGold() ? "GOLD"
			: ST::format("x {}", gCupidPersist.ubLikesLeft);
		PrintCentred(FONT10ARIAL, IsGold() ? FONT_MCOLOR_LTYELLOW
				: CanLike() ? FONT_GRAY2 : FONT_LTRED,
				CP_BTN_X + CP_BTN_SIZE / 2,
				CP_BTN_KISS_Y + CP_BTN_SIZE + 8, likes);
		}

		// the site's little dialog: why the button said no
		if (giCupidNotice >= 0)
		{
			const INT32 bx = CP_CARD_X + (CP_CARD_W - 280) / 2;
			const INT32 by = 108;
			DropShadow(bx, by, 280, 100);
			FillRect(bx, by, 280, 100, CP_RGB_INK);
			FillRect(bx + 1, by + 1, 278, 98, FROMRGB(236, 233, 222));
			FillRect(bx + 1, by + 1, 278, 14, CP_RGB_BLUE_DK);
			PrintAt(FONT10ARIAL, FONT_MCOLOR_WHITE, bx + 6, by + 4,
					"C.U.P.I.D. says:");
			DisplayWrappedString(UINT16(CP_X(bx + 10)),
					UINT16(CP_Y(by + 22)), 260, 2, FONT10ARIAL,
					FONT_NEARBLACK, T(CupidStr(giCupidNotice)),
					FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
			if (giCupidNoticeCta == 1)
			{
				const bool hov = Hover(gCupidNoticeCtaRegion);
				GelPill(bx + 10, by + 70, 150, 22,
						hov ? CP_RGB_GOLD_LITE : CP_RGB_GOLD,
						CP_RGB_GOLD_LITE, FROMRGB(150, 112, 48),
						FROMRGB(236, 233, 222));
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
						bx + 85, by + 77,
						gfCupidGerman ? "TAGESPASS - 5 $"
							      : "DAY PASS - $5");
			}
			{
				const bool hov = Hover(gCupidNoticeOkRegion);
				GelPill(bx + 204, by + 70, 66, 22,
						hov ? FROMRGB(170, 166, 172) : CP_RGB_GREY,
						FROMRGB(170, 166, 172), FROMRGB(84, 80, 88),
						FROMRGB(236, 233, 222));
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
						bx + 237, by + 77, "OK");
			}
		}

		// the popup: a little window with a big claim and one honest pixel
		if (gfCupidPopupUp)
		{
			DropShadow(131, 120, 240, 116);
			FillRect(131, 120, 240, 116, CP_RGB_INK);
			FillRect(132, 121, 238, 114, FROMRGB(236, 233, 222));
			// title bar, with the era's most trustworthy filename
			FillRect(132, 121, 238, 14, CP_RGB_BLUE_DK);
			PrintAt(FONT10ARIAL, FONT_MCOLOR_WHITE, 137, 124,
					T(CPS_POPUP_TITLE));
			FillRect(131 + 240 - 16, 122, 13, 12,
					Hover(gCupidPopupXRegion) ? CP_RGB_NOPE
								  : CP_RGB_BLUE);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, 131 + 240 - 10,
					124, "X");

			PrintCentred(FONT10ARIALBOLD, FONT_LTRED, 251, 142,
					T(CPS_POPUP_HEAD));
			DisplayWrappedString(UINT16(CP_X(143)), UINT16(CP_Y(158)), 216, 2,
					FONT10ARIAL, FONT_MCOLOR_WHITE, T(CPS_POPUP_BODY),
					FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
			GelPill(161, 206, 180, 24, CP_RGB_GOLD, CP_RGB_GOLD_LITE,
					FROMRGB(150, 112, 48), CP_RGB_CARD);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, 251, 213,
					T(CPS_POPUP_CTA));
		}
	}

	void RenderMatches()
	{
		PrintStamped(FONT12ARIAL, FONT_MCOLOR_WHITE, CP_CONT_CX, 32,
				T(CPS_MATCHES_TITLE));

		// the GOLD tease: they liked you, and you may pay to know who
		const std::vector<ProfileID> admirers = SecretAdmirers();
		if (!admirers.empty())
		{
			PrintAt(FONT10ARIALBOLD, FONT_LTRED, CP_CONT_X, 48,
					ST::format(T(IsGold() ? CPS_LIKED_YOU_GOLD : CPS_LIKED_YOU),
							int(admirers.size())));
			INT32 x = CP_CONT_X;
			for (ProfileID pid : admirers)
			{
				if (x + CP_FACE_SM_W > CP_CONT_X + CP_CONT_W) break;
				FillRect(x - 1, 59, CP_FACE_SM_W + 2, CP_FACE_SM_H + 2,
						CP_RGB_INK);
				SGPVObject* face = Face33For(pid);
				if (face)
				{
					BltVideoObject(FRAME_BUFFER, face, 0, CP_X(x), CP_Y(60));
				}
				if (!IsGold()) BlurOver(x, 60, CP_FACE_SM_W, CP_FACE_SM_H);
				else
				{
					PrintCentred(FONT10ARIAL, FONT_GRAY1,
							x + CP_FACE_SM_W / 2, 105,
							GetProfile(pid).zNickname);
				}
				x += CP_FACE_SM_W + 8;
			}
		}

		const std::vector<ProfileID> matches = AllMatches();
		if (matches.empty())
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY2, CP_CONT_CX, 108,
					T(CPS_MATCHES_NONE));
			// who you could be matching with, decency-blurred
			INT32 fx = CP_CONT_CX - (3 * CP_FACE_SM_W + 2 * 8) / 2;
			int teased = 0;
			for (const Member& m : gCupidRoster)
			{
				if (teased >= 3) break;
				if (m.locked || MemberIsDead(m.pid)) continue;
				SGPVObject* face = Face33For(m.pid);
				if (!face) continue;
				FillRect(fx - 2, 126, CP_FACE_SM_W + 4, CP_FACE_SM_H + 4,
						CP_RGB_MAT);
				BltVideoObject(FRAME_BUFFER, face, 0, CP_X(fx), CP_Y(128));
				BlurOver(fx, 128, CP_FACE_SM_W, CP_FACE_SM_H);
				fx += CP_FACE_SM_W + 8;
				++teased;
			}
			// the hazard-striped promise of the era: content, eventually
			DropShadow(CP_CONT_X, 180, CP_CONT_W, 40);
			FillCard(CP_CONT_X, 180, CP_CONT_W, 40, CP_RGB_MAT, CP_RGB_INK, CP_RGB_BG);
			for (INT32 sx = CP_CONT_X + 4; sx < CP_CONT_X + CP_CONT_W - 10; sx += 16)
			{
				for (int t = 0; t < 8; ++t)
				{
					FillRect(sx + t, 184 + t, 8, 1, FROMRGB(224, 186, 60));
				}
			}
			PrintCentred(FONT10ARIAL, FONT_NEARBLACK, CP_CONT_CX, 202,
					T(CPS_UNDER_CONSTRUCTION));
			return;
		}

		INT32 y = 118;
		int row = 0;
		for (ProfileID pid : matches)
		{
			if (row >= 5) break;
			DropShadow(CP_CONT_X, y, CP_CONT_W, 49);
			FillCard(CP_CONT_X, y, CP_CONT_W, 49, CP_RGB_CARD,
					Hover(gCupidMatchRegion[row]) ? CP_RGB_PINK
								      : CP_RGB_BLUE,
					CP_RGB_BG);
			SGPVObject* face = Face33For(pid);
			if (face)
			{
				BltVideoObject(FRAME_BUFFER, face, 0, CP_X(CP_CONT_X + 4), CP_Y(y + 3));
			}
			MERCPROFILESTRUCT const& p = GetProfile(pid);
			const bool dead = MemberIsDead(pid);
			PrintAt(FONT10ARIALBOLD, dead ? FONT_GRAY4 : FONT_MCOLOR_WHITE,
					CP_CONT_X + 60, y + 8, p.zNickname);
			const DatingGame::Match match = MatchWith(pid);
			PrintAt(FONT10ARIAL, dead ? FONT_GRAY4 : MatchColour(match.percent),
					CP_CONT_X + 60, y + 24, MatchLabel(match));
			PrintAt(FONT10ARIAL, FONT_GRAY4, CP_CONT_X + 60, y + 36,
					ClampLines(T(dead ? CPS_CONDOLENCE_ROW
							  : MemberStatus(pid)),
						CP_CONT_W - 84, 1));
			DrawHeart(CP_CONT_X + CP_CONT_W - 18, y + 18, 1,
					dead ? CP_RGB_GREY : CP_RGB_PINK);
			y += 51;
			++row;
		}
		PrintCentred(FONT10ARIAL, FONT_GRAY4, CP_CONT_CX, y + 4,
				T(CPS_MATCHES_HINT));
	}

	// the action rows: two full-width pills stacked over the footer, wide
	// enough for every label in both languages
	void RenderWideButton(INT32 slot, const ST::string& label, bool live)
	{
		const INT32 y = CP_PAGE_H - 84 + slot * 32;
		DropShadow(CP_CONT_X, y, CP_CONT_W, 26);
		if (live)
		{
			const bool hov = Hover(gCupidActionRegion[slot]);
			GelPill(CP_CONT_X, y, CP_CONT_W, 26,
					hov ? CP_RGB_PINK_LITE : CP_RGB_PINK,
					hov ? CP_RGB_PINK_PALE : CP_RGB_PINK_LITE,
					CP_RGB_PINK_DK, CP_RGB_BG);
		}
		else
		{
			GelPill(CP_CONT_X, y, CP_CONT_W, 26, CP_RGB_CARD_DIM,
					CP_RGB_BLUE_LITE, CP_RGB_GREY, CP_RGB_BG);
		}
		PrintCentred(FONT10ARIALBOLD, live ? FONT_MCOLOR_WHITE : FONT_GRAY4,
				CP_CONT_CX, y + 9, label);
	}

	// The lounge, full immersion: room tabs, then a client window from
	// the rail to the page edge. Mugshots open each cluster of lines,
	// the text keeps one fixed column, and one door costs money.
	void RenderLounge()
	{
		const INT32 wx = CPL_X, ww = CPL_W;
		const int lang = gfCupidGerman ? 1 : 0;

		// the room tabs
		const INT32 tabW = ww / CPL_ROOMS;
		for (int i = 0; i < CPL_ROOMS; ++i)
		{
			const INT32 tx = wx + i * tabW;
			const bool on = i == giCupidRoom;
			const bool hov = Hover(gCupidRoomRegion[i]);
			FillRect(tx, 8, tabW - 2, 18,
					on ? CP_RGB_BLUE_PALE
					: hov ? CP_RGB_BLUE_LITE : CP_RGB_INK);
			if (on) FillRect(tx, 8, tabW - 2, 1, CP_RGB_PINK);
			ST::string name = CPL_NAME[i];
			if (i == 3 && !IsGold()) name = "#goldhearts*";
			PrintCentred(FONT10ARIAL,
					on ? FONT_MCOLOR_WHITE : FONT_GRAY4,
					tx + (tabW - 2) / 2, 13, name);
		}

		// the client window
		const INT32 wy = 30, wh = 330;
		DropShadow(wx, wy, ww, wh);
		FillCard(wx, wy, ww, wh, CP_RGB_INK, CP_RGB_BLUE_DK, CP_RGB_BG);

		FillRect(wx + 2, wy + 2, ww - 4, 16, CP_RGB_BLUE_PALE);
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, wx + 8, wy + 6,
				CPL_NAME[giCupidRoom]);
		PrintCentred(FONT10ARIAL, FONT_GRAY4, wx + ww / 2, wy + 6,
				"ArulcoNet IRC");
		int chatting = 1;
		for (const Member& m : gCupidRoster)
		{
			if (!m.locked && !MemberIsDead(m.pid) &&
			    RoomAdmits(m, giCupidRoom))
			{
				++chatting;
			}
		}
		const ST::string head = ST::format(gfCupidGerman ? "{} im kanal"
							: "{} chatting", chatting);
		PrintAt(FONT10ARIAL, FONT_GRAY2,
				wx + ww - 10 - StringPixLength(head, FONT10ARIAL),
				wy + 6, head);

		// the velvet rope
		if (RoomLocked())
		{
			static const char* const rope[2][3] =
			{
				{ "#goldhearts is GOLD members only.",
				  "through the wall: muffled laughter, real champagne.",
				  "membership is one payment of $10, to speck, personally." },
				{ "#goldhearts ist nur fuer GOLD-mitglieder.",
				  "durch die wand: gedaempftes lachen, echter champagner.",
				  "die mitgliedschaft kostet einmalig 10 $, an speck, persoenlich." },
			};
			for (int i = 0; i < 3; ++i)
			{
				DisplayWrappedString(UINT16(CP_X(wx + 30)),
						UINT16(CP_Y(wy + 110 + i * 34)), UINT16(ww - 60),
						2, FONT10ARIAL,
						i == 0 ? FONT_MCOLOR_LTYELLOW : FONT_GRAY2,
						rope[lang][i], FONT_MCOLOR_BLACK,
						CENTER_JUSTIFIED);
			}
			const bool hov = Hover(gCupidLoungeSayRegion);
			DropShadow(wx, CPL_SAY_Y, ww, 26);
			GelPill(wx, CPL_SAY_Y, ww, 26,
					hov ? CP_RGB_GOLD_LITE : CP_RGB_GOLD,
					CP_RGB_GOLD_LITE, FROMRGB(150, 112, 48), CP_RGB_BG);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, wx + ww / 2,
					CPL_SAY_Y + 9, gfCupidGerman
						? "GOLD WERDEN (10 $)"
						: "UPGRADE TO GOLD ($10)");
			return;
		}

		LoungeRoom& R = Room();
		const INT32 viewTop = wy + 22;
		const INT32 viewBot = wy + wh - 20;
		const INT32 viewH   = viewBot - viewTop;
		const INT32 textX   = wx + CPL_TEXT_X;
		const INT32 railX   = wx + ww - 14;

		// fixed rows, parlour fashion: content height is arithmetic
		INT32 total = 0;
		for (size_t i = 0; i < R.log.size(); ++i)
		{
			const bool opens = i == 0 ||
					R.log[i - 1].who != R.log[i].who;
			total += CPL_ROW_H + (opens ? CPL_CLUSTER_GAP : 0);
		}
		R.scrollMax = std::max<INT32>(0, total - viewH);
		if (R.scroll > R.scrollMax) R.scroll = R.scrollMax;
		const INT32 topScroll = std::max<INT32>(0,
				total - viewH - R.scroll);

		// rows crossing the window's edges are drawn and cut off, so
		// scrolling reads as a window sliding over the log, not as lines
		// popping in and out
		SetFontDestBuffer(FRAME_BUFFER, CP_X(wx + 2), CP_Y(viewTop),
				CP_X(railX - 2), CP_Y(viewBot));
		gCupidLoungeChipsAt.clear();
		INT32 cy = 0;
		for (size_t i = 0; i < R.log.size(); ++i)
		{
			const LoungeLine& line = R.log[i];
			const bool opens = i == 0 ||
					R.log[i - 1].who != line.who;
			cy += opens ? CPL_CLUSTER_GAP : 0;
			const INT32 sy = viewTop + cy - topScroll;
			cy += CPL_ROW_H;
			if (sy > viewBot || sy + 12 < viewTop) continue;
			const bool chipRoom = sy >= viewTop &&
					sy + CPL_ROW_H <= viewBot + 2;
			if (line.who == LNG_SYS)
			{
				// the house token holds the avatar column steady
				if (opens && chipRoom)
				{
					FillRect(wx + 12, sy + 3, 8, 8, CP_RGB_BLUE_DK);
				}
				PrintAt(FONT10ARIAL, FONT_GRAY4, textX, sy,
						ReduceStringLength(line.text, CPL_TEXT_W,
								FONT10ARIAL));
				continue;
			}
			const bool mine = line.who == LNG_YOU;
			const bool op   = line.who == LNG_SPECK;
			INT32 nameW = 0;
			if (opens)
			{
				if (chipRoom)
				{
					LoungeChip(line.who, wx + 9, sy - 1);
					if (line.who >= 0)
					{
						gCupidLoungeChipsAt.push_back(
								LoungeChipAt{ sy - 1, line.who });
					}
				}
				const ST::string name =
					ST::format("{}:", LoungeNick(line.who));
				PrintAt(FONT10ARIALBOLD,
						op ? FONT_MCOLOR_LTYELLOW
						: mine ? FONT_LTGREEN : FONT_MCOLOR_WHITE,
						textX, sy, name);
				nameW = StringPixLength(name, FONT10ARIALBOLD) + 4;
			}
			PrintAt(FONT10ARIAL,
					op ? FONT_MCOLOR_LTYELLOW
					: mine ? FONT_LTGREEN : FONT_GRAY1,
					textX + nameW, sy,
					ReduceStringLength(line.text,
							CPL_TEXT_W - nameW, FONT10ARIAL));
		}
		SetFontDestBuffer(FRAME_BUFFER);

		// the chunky rail: an arrow at each end, a fat thumb between
		{
			const INT32 trackY = viewTop + 16;
			const INT32 trackH = viewH - 32;
			FillRect(railX, viewTop, 10, viewH, CP_RGB_BLUE_PALE);
			const bool upHot   = Hover(gCupidLoungeUpRegion);
			const bool downHot = Hover(gCupidLoungeDownRegion);
			for (INT32 r = 0; r < 6; ++r)
			{
				const INT32 half = r / 2;
				FillRect(railX + 4 - half, viewTop + 4 + r,
						1 + 2 * half, 1,
						upHot ? CP_RGB_MAT : CP_RGB_BLUE_DK);
				FillRect(railX + 4 - half, viewBot - 5 - r,
						1 + 2 * half, 1,
						downHot ? CP_RGB_MAT : CP_RGB_BLUE_DK);
			}
			FillRect(railX + 3, trackY, 4, trackH, CP_RGB_INK);
			if (R.scrollMax > 0)
			{
				const INT32 th = std::max<INT32>(10,
						trackH * viewH / std::max<INT32>(1, total));
				const INT32 to = (trackH - th) * topScroll /
						std::max<INT32>(1, total - viewH);
				FillRect(railX + 2, trackY + to, 6, th,
						CP_RGB_BLUE_DK);
			}
		}

		// somebody is at their keyboard
		FillRect(wx + 2, wy + wh - 18, ww - 4, 16, CP_RGB_BLUE_PALE);
		if (!R.pend.empty() &&
		    R.pend.front().due < GetJA2Clock() + 5000 &&
		    R.pend.front().who != LNG_SYS)
		{
			static const char* const dots[4] = { "", ".", "..", "..." };
			LoungeChip(R.pend.front().who, wx + 9, wy + wh - 17);
			PrintAt(FONT10ARIAL, FONT_GRAY2, wx + 30, wy + wh - 14,
					ST::format(gfCupidGerman ? "{} tippt{}"
								 : "{} is typing{}",
						LoungeNick(R.pend.front().who),
						dots[(GetJA2Clock() / 350) % 4]));
		}
		else
		{
			PrintAt(FONT10ARIAL, FONT_GRAY4, wx + 8, wy + wh - 14,
					gfCupidGerman ? "der kanal atmet."
						      : "the channel breathes.");
		}

		// the one input this client has: a real chat line. ENTER sends;
		// the pink cap on the right is the same button it always was
		const bool hov = Hover(gCupidLoungeSayRegion);
		DropShadow(wx, CPL_SAY_Y, ww, 26);
		if (PlayerHasProfile())
		{
			const INT32 sendW = 58;
			FillRounded(wx, CPL_SAY_Y, ww, 26, CP_RGB_BLUE_DK, 3,
					CP_RGB_BG);
			FillRounded(wx + 1, CPL_SAY_Y + 1, ww - 2, 24, CP_RGB_INK,
					3, CP_RGB_BLUE_DK);
			if (gCupidLoungeInput.empty())
			{
				PrintAt(FONT10ARIAL, FONT_GRAY4, wx + 10, CPL_SAY_Y + 9,
						gfCupidGerman ? "sag etwas nettes..."
							      : "say something nice...");
			}
			else
			{
				// the line scrolls left once it outgrows the box
				ST::string shown = ST::string(gCupidLoungeInput);
				const INT32 boxW = ww - sendW - 26;
				while (shown.size() > 1 &&
					StringPixLength(shown, FONT10ARIAL) > boxW)
				{
					shown = shown.substr(1);
				}
				PrintAt(FONT10ARIAL, FONT_MCOLOR_WHITE, wx + 10,
						CPL_SAY_Y + 9, shown);
				if ((GetJA2Clock() / 400) % 2 == 0)
				{
					PrintAt(FONT10ARIAL, FONT_MCOLOR_WHITE,
							wx + 12 + StringPixLength(shown,
									FONT10ARIAL),
							CPL_SAY_Y + 9, "_");
				}
			}
			GelPill(wx + ww - sendW - 4, CPL_SAY_Y + 3, sendW, 20,
					hov ? CP_RGB_PINK_LITE : CP_RGB_PINK,
					CP_RGB_PINK_LITE, CP_RGB_PINK_DK, CP_RGB_INK);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
					wx + ww - sendW / 2 - 4, CPL_SAY_Y + 9,
					gfCupidGerman ? "SENDEN" : "SEND");
		}
		else
		{
			GelPill(wx, CPL_SAY_Y, ww, 26, CP_RGB_CARD_DIM,
					CP_RGB_BLUE_LITE, CP_RGB_GREY, CP_RGB_BG);
			PrintCentred(FONT10ARIALBOLD, FONT_GRAY4, wx + ww / 2,
					CPL_SAY_Y + 9, gfCupidGerman
						? "ZUM MITREDEN REGISTRIEREN (ICH-TAB)"
						: "REGISTER TO SPEAK (THE ME TAB)");
		}
	}

	void RenderMeLanding()
	{
		const bool member = PlayerHasProfile() && HaveStoredAnswers();
		if (!member)
		{
			PrintStamped(FONT12ARIAL, FONT_MCOLOR_WHITE, CP_CONT_CX, 32,
					T(CPS_ME_TITLE));
		}

		if (member)
		{
			// your ad, rendered exactly as the deck deals it - same card,
			// same dossier, wheel and all
			Card self;
			self.kind = CARD_MEMBER;
			self.pid  = PlayerImpPid();
			DropShadow(CPL_X, 8, CP_CARD_W, 306);
			RenderMemberCard(self, CPL_X, 8, 306, CV_SELF);

			// no verdict circles on yourself - the rail carries the
			// seeking selector instead, in the same house style
			{
				PrintCentred(FONT10ARIAL, FONT_GRAY4,
						CP_BTN_X + CP_BTN_SIZE / 2, CP_CARD_Y + 48,
						gfCupidGerman ? "SUCHE" : "SEEK");
				static const INT32 ys[3] =
					{ CP_BTN_KILL_Y, CP_BTN_KISS_Y, CP_BTN_MARRY_Y };
				// per-option palette: steel blue, rose, gold
				static const UINT32 rimRest[3] =
					{ FROMRGB(104, 122, 152), FROMRGB(170, 74, 112),
					  FROMRGB(178, 142, 74) };
				static const UINT32 rimSel[3] =
					{ FROMRGB(158, 196, 238), CP_RGB_PINK_LITE,
					  CP_RGB_GOLD };
				static const UINT32 faceRest[3] =
					{ FROMRGB(30, 40, 58), FROMRGB(86, 26, 46),
					  FROMRGB(56, 48, 44) };
				static const UINT32 faceSel[3] =
					{ FROMRGB(48, 78, 118), FROMRGB(150, 44, 84),
					  FROMRGB(96, 76, 34) };
				static const UINT32 faceHov[3] =
					{ FROMRGB(40, 60, 90), FROMRGB(126, 40, 70),
					  FROMRGB(112, 86, 36) };
				const bool railHov = Hover(gCupidSeekRegion);
				const INT32 hovY = CP_BTN_KILL_Y
						+ INT32(gCupidSeekRegion.RelativeYPos);
				for (int i = 0; i < 3; ++i)
				{
					gsCupidSeekHit[i][0] = ys[i] - 2;
					gsCupidSeekHit[i][1] = ys[i] + CP_BTN_SIZE + 2;
					const bool sel = giCupidSeek == i;
					const bool hov = railHov && !sel &&
							hovY >= gsCupidSeekHit[i][0] &&
							hovY <= gsCupidSeekHit[i][1];
					const UINT32 rim  = sel ? rimSel[i] : rimRest[i];
					const UINT32 face = sel ? faceSel[i]
							: hov ? faceHov[i] : faceRest[i];
					GelCircle(CP_BTN_X, ys[i], CP_BTN_SIZE, face,
							Lighten(face, 20), rim, CP_RGB_CARD);
					const UINT32 glyph = sel ? CP_RGB_MAT : rim;
					// SEEK_MEN, SEEK_WOMEN, SEEK_EVERYONE
					const bool arrow = i != SEEK_WOMEN;
					const bool cross = i != SEEK_MEN;
					const INT32 gy = ys[i]
							+ (arrow && cross ? 7 : arrow ? 11 : 8);
					DrawGender(CP_BTN_X + 9, gy, arrow, cross, glyph,
							face);
					if (sel)
					{
						FillRect(CP_BTN_X + 8,
								ys[i] + CP_BTN_SIZE + 3, 20, 2,
								rimSel[i]);
					}
				}
			}

			// the bottom row: just the questionnaire button, drawn
			// exactly on its click region
			RenderWideButton(1,
					ST::format(T(CPS_ME_RETAKE), CP_RETAKE_PRICE), true);
			return;
		}

		// the status row: your photo beside the truth about your profile
		DropShadow(CP_CONT_X, 66, CP_CONT_W, 80);
		FillCard(CP_CONT_X, 66, CP_CONT_W, 80, CP_RGB_CARD, CP_RGB_BLUE_DK, CP_RGB_BG);
		FillRect(CP_CONT_X + 8, 72, CP_FACE_SM_W + 6, CP_FACE_SM_H + 6, CP_RGB_INK);
		FillRect(CP_CONT_X + 9, 73, CP_FACE_SM_W + 4, CP_FACE_SM_H + 4, CP_RGB_MAT);
		if (guiCupidSelf)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidSelf, 0, CP_X(CP_CONT_X + 11), CP_Y(75));
		}
		else
		{
			FillRect(CP_CONT_X + 11, 75, CP_FACE_SM_W, CP_FACE_SM_H, CP_RGB_CARD_DIM);
			PrintAt(FONT10ARIAL, FONT_GRAY4, CP_CONT_X + 16, 90, T(CPS_NO_PHOTO));
		}

		const bool full = PlayerHasProfile() && HaveStoredAnswers();
		const INT32 tx = CP_CONT_X + 70;
		PrintAt(FONT10ARIALBOLD, full ? FONT_LTGREEN : FONT_LTRED, tx, 74,
				T(full ? CPS_ME_COMPLETE : CPS_ME_PARTIAL));
		DrawMeter(tx, 90, 148, full ? 100 : 60,
				full ? CP_RGB_LIKE : CP_RGB_PINK);
		if (!full)
		{
			DisplayWrappedString(UINT16(CP_X(tx)), UINT16(CP_Y(104)), 148, 2,
					FONT10ARIAL, FONT_GRAY2, T(CPS_ME_HINT),
					FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		}
		else
		{
			PrintAt(FONT10ARIAL, FONT_GRAY2, tx, 104,
					gfCupidGerman ? "Das Deck wartet auf Sie."
						      : "The deck is waiting for you.");
		}

		// POWERED BY I.M.P., 88x31 in spirit, with the fine print below
		DropShadow(CP_CONT_CX - 75, 152, 150, 38);
		FillCard(CP_CONT_CX - 75, 152, 150, 38, CP_RGB_BLUE_PALE,
				CP_RGB_BLUE_DK, CP_RGB_BG);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, CP_CONT_CX, 159,
				"POWERED BY");
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, CP_CONT_CX, 172,
				"I.M.P.");
		DisplayWrappedString(UINT16(CP_X(CP_CONT_X)), UINT16(CP_Y(200)),
				CP_CONT_W, 2,
				FONT10ARIAL, FONT_GRAY4, T(CPS_ME_POWERED),
				FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

		// the testimonial, delivered in person by the site's one success
		DropShadow(CP_CONT_X, 240, CP_CONT_W, 66);
		FillCard(CP_CONT_X, 240, CP_CONT_W, 66, CP_RGB_CARD, CP_RGB_PINK, CP_RGB_BG);
		FillRect(CP_CONT_X + 7, 247, CP_FACE_SM_W + 4, CP_FACE_SM_H + 4, CP_RGB_INK);
		FillRect(CP_CONT_X + 8, 248, CP_FACE_SM_W + 2, CP_FACE_SM_H + 2, CP_RGB_MAT);
		SGPVObject* flo = Face33For(FLO);
		if (flo)
		{
			BltVideoObject(FRAME_BUFFER, flo, 0, CP_X(CP_CONT_X + 9), CP_Y(249));
		}
		DisplayWrappedString(UINT16(CP_X(CP_CONT_X + 66)), UINT16(CP_Y(246)),
				154, 2,
				FONT10ARIAL, FONT_MCOLOR_WHITE,
				ClampLines(ST::format("{} {}", T(CPS_AD_TESTI_HEAD),
						T(CPS_AD_TESTI_BODY)), 154, 3),
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		PrintAt(FONT10ARIAL, FONT_GRAY4, CP_CONT_X + 66, 290,
				T(CPS_AD_TESTI_BY));

		const bool imp     = LaptopSaveInfo.fIMPCompletedFlag;
		const bool banked  = HaveStoredAnswers();
		const bool profile = PlayerHasProfile();
		RenderWideButton(0, T(CPS_ME_IMPORT), imp && banked && !profile);

		ST::string second;
		if (!profile && !banked) second = T(CPS_ME_TAKE);
		else if (imp && !banked)
		{
			second = ST::format(T(CPS_ME_UPGRADE), CP_RETAKE_PRICE);
		}
		else second = ST::format(T(CPS_ME_RETAKE), CP_RETAKE_PRICE);
		RenderWideButton(1, second, true);
	}

	void RenderQuizQuestion()
	{
		if (giCupidQuizQ < 0)
		{
			DisplayWrappedString(UINT16(CP_X(CP_CONT_X)), UINT16(CP_Y(46)),
					CP_CONT_W, 2, FONT12ARIAL, FONT_MCOLOR_WHITE,
					T(CPS_QUIZ_SEX), FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
			for (int i = 0; i < giCupidAnsCount; ++i)
			{
				const INT32 y = gsCupidAnsY[i];
				DropShadow(CP_CONT_X, y, CP_CONT_W, gsCupidAnsH[i]);
				GelPill(CP_CONT_X, y, CP_CONT_W, gsCupidAnsH[i], CP_RGB_CARD,
						CP_RGB_CARD_LITE, CP_RGB_BLUE_DK, CP_RGB_BG);
				PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, CP_CONT_CX,
						y + 9, T(i ? CPS_QUIZ_FEMALE : CPS_QUIZ_MALE));
			}
			return;
		}

		PrintCentred(FONT10ARIAL, FONT_GRAY4, CP_CONT_CX, 30,
				ST::format(T(CPS_QUIZ_PROGRESS), giCupidQuizQ + 1,
						DatingGame::NUM_QUESTIONS));
		DrawMeter(CP_CONT_CX - 50, 44, 100,
				(giCupidQuizQ + 1) * 100 / DatingGame::NUM_QUESTIONS,
				CP_RGB_PINK);
		DisplayWrappedString(UINT16(CP_X(CP_CONT_X)), UINT16(CP_Y(60)),
				CP_CONT_W, 2, FONT12ARIAL, FONT_MCOLOR_WHITE,
				QuizQuestion(giCupidQuizQ), FONT_MCOLOR_BLACK,
				LEFT_JUSTIFIED);

		// the rows are cut to their text by LayoutQuiz()
		for (int i = 0; i < giCupidAnsCount; ++i)
		{
			const INT32 y = gsCupidAnsY[i];
			const INT32 h = gsCupidAnsH[i];
			DropShadow(CP_CONT_X, y, CP_CONT_W, h);
			const bool hov = Hover(gCupidAnswerRegion[i]);
			GelPill(CP_CONT_X, y, CP_CONT_W, h,
					hov ? CP_RGB_BLUE_LITE : CP_RGB_CARD, CP_RGB_CARD_LITE,
					hov ? CP_RGB_PINK : CP_RGB_BLUE_DK, CP_RGB_BG);
			GelPill(CP_CONT_X + 6, y + (h - 18) / 2, 18, 18, CP_RGB_PINK,
					CP_RGB_PINK_LITE, CP_RGB_PINK_DK, CP_RGB_CARD);
			const char letter[2] = { char('A' + i), '\0' };
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, CP_CONT_X + 15,
					y + (h - 18) / 2 + 5, letter);
			DisplayWrappedString(UINT16(CP_X(CP_CONT_X + 32)),
					UINT16(CP_Y(y + 6)), UINT16(CP_CONT_W - 44), 2,
					FONT10ARIAL, FONT_MCOLOR_WHITE,
					QuizAnswer(giCupidQuizQ, i),
					FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		}
	}

	void RenderMe()
	{
		if (gfCupidQuizLive)
		{
			RenderQuizQuestion();
			return;
		}
		RenderMeLanding();
	}

	// the shared rail furniture on the dossier and the private line:
	// a steel back plate up top, then two circles in the verdict style
	void RenderRailButtons()
	{
		const bool onChat = gCupidPage == CPP_CHAT;
		{
			const UINT32 steel     = FROMRGB(140, 136, 144);
			const UINT32 steelLite = FROMRGB(190, 186, 194);
			const UINT32 steelRing = FROMRGB(70, 66, 74);
			const UINT32 ink       = FROMRGB(30, 26, 32);
			GelPill(CP_BTN_X, CP_CARD_Y + 8, CP_BTN_SIZE, 24,
					Hover(gCupidRailBtnRegion[0]) ? steelLite : steel,
					steelLite, steelRing, CP_RGB_INK);
			DrawTri(CP_BTN_X + 14, CP_CARD_Y + 15, 11, false, ink);
		}
		{
			// the bubble opens the line; the line opens the dossier
			const bool live = onChat || IsMatched(gCupidDetailPid);
			const bool hov  = live && Hover(gCupidRailBtnRegion[1]);
			const UINT32 rim = !live ? FROMRGB(96, 88, 96)
					: FROMRGB(120, 150, 190);
			const UINT32 face = hov ? FROMRGB(40, 60, 90)
					: live ? FROMRGB(30, 40, 58) : FROMRGB(40, 36, 40);
			GelCircle(CP_BTN_X, CP_BTN_KILL_Y, CP_BTN_SIZE, face,
					Lighten(face, 20), rim, CP_RGB_CARD);
			const UINT32 glyph = live ? FROMRGB(158, 196, 238) : rim;
			if (onChat)
			{
				DrawDossier(CP_BTN_X + 10, CP_BTN_KILL_Y + 10, glyph,
						face);
			}
			else
			{
				DrawBubble(CP_BTN_X + 10, CP_BTN_KILL_Y + 10, glyph,
						face);
			}
		}
		{
			const bool hov = Hover(gCupidRailBtnRegion[2]);
			const UINT32 face = hov ? FROMRGB(126, 40, 70)
					: FROMRGB(86, 26, 46);
			GelCircle(CP_BTN_X, CP_BTN_KISS_Y, CP_BTN_SIZE, face,
					FROMRGB(150, 50, 86), FROMRGB(170, 74, 112),
					CP_RGB_CARD);
			DrawFlower(CP_BTN_X + 10, CP_BTN_KISS_Y + 8,
					CP_RGB_PINK_LITE, CP_RGB_GOLD,
					FROMRGB(88, 138, 74));
		}
	}

	void RenderDetail()
	{
		// no second layout: the dossier page just re-deals the very card
		// the deck showed, photo already cached, the rail does the rest
		Card c;
		c.kind = CARD_MEMBER;
		c.pid  = gCupidDetailPid;
		DropShadow(CPL_X, 8, CP_CARD_W, 300);
		RenderMemberCard(c, CPL_X, 8, 300, CV_DETAIL);
		RenderRailButtons();
	}

	// The private line, dressed like the lounge: the rail carries the
	// other party's mini dossier, the client window takes the stage.
	void RenderChat()
	{
		const INT32 wx = CPL_X, ww = CPL_W;
		const INT32 wy = 8, wh = 264;
		const ProfileID pid = gCupidChatPid;
		MERCPROFILESTRUCT const& p = GetProfile(pid);

		DropShadow(wx, wy, ww, wh);
		FillCard(wx, wy, ww, wh, CP_RGB_INK, CP_RGB_BLUE_DK, CP_RGB_BG);

		// the client's title strip
		FillRect(wx + 2, wy + 2, ww - 4, 16, CP_RGB_BLUE_PALE);
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, wx + 8, wy + 6,
				ST::format(gfCupidGerman ? "privat: {}" : "private: {}",
						p.zNickname));
		PrintCentred(FONT10ARIAL, FONT_GRAY4, wx + ww / 2, wy + 6,
				"ArulcoNet DCC");
		{
			const ST::string head = gfCupidGerman
					? "2 im kanal" : "2 chatting";
			PrintAt(FONT10ARIAL, FONT_GRAY2,
					wx + ww - 10 - StringPixLength(head, FONT10ARIAL),
					wy + 6, head);
		}

		ChatThread& C = Chat();
		const INT32 viewTop = wy + 22;
		const INT32 viewBot = wy + wh - 20;
		const INT32 viewH   = viewBot - viewTop;
		const INT32 railX   = wx + ww - 14;

		// bubbles, one per message: theirs on the left, yours on the
		// right, the operator's small print in the middle
		const INT32 rowH = 12;
		auto msgH = [&](const ChatMsg& msg) -> INT32
		{
			return msg.who == LNG_SYS
				? INT32(msg.rows.size()) * rowH + 2
				: INT32(msg.rows.size()) * rowH + 9;
		};
		INT32 total = 0;
		for (const ChatMsg& msg : C.log) total += msgH(msg) + 6;
		C.scrollMax = std::max<INT32>(0, total - viewH);
		if (C.scroll > C.scrollMax) C.scroll = C.scrollMax;
		const INT32 topScroll = std::max<INT32>(0,
				total - viewH - C.scroll);

		SetFontDestBuffer(FRAME_BUFFER, CP_X(wx + 2), CP_Y(viewTop),
				CP_X(railX - 2), CP_Y(viewBot));
		INT32 cy = 0;
		for (const ChatMsg& msg : C.log)
		{
			const INT32 bh = msgH(msg);
			const INT32 sy = viewTop + cy - topScroll;
			cy += bh + 6;
			if (sy > viewBot || sy + bh < viewTop) continue;
			if (msg.who == LNG_SYS)
			{
				for (size_t r = 0; r < msg.rows.size(); ++r)
				{
					PrintCentred(FONT10ARIAL, FONT_GRAY4,
							wx + (ww - 14) / 2,
							sy + 1 + INT32(r) * rowH,
							msg.rows[r]);
				}
				continue;
			}
			const bool mine = msg.who == LNG_YOU;
			INT32 bw = 0;
			for (const ST::string& row : msg.rows)
			{
				bw = std::max(bw,
						INT32(StringPixLength(row, FONT10ARIAL)));
			}
			bw += 14;
			const INT32 bx = mine ? railX - 10 - bw : wx + 10;
			const UINT32 face = mine ? FROMRGB(150, 44, 84)
						 : FROMRGB(46, 42, 56);
			// bubbles at the window's edge lose their corners to the
			// frame; a square cut reads better than a floating pop
			if (sy >= viewTop - 1 && sy + bh <= viewBot + 1)
			{
				FillRounded(bx, sy, bw, bh, face, 4, CP_RGB_INK);
			}
			else
			{
				const INT32 y0 = std::max(sy, viewTop);
				const INT32 y1 = std::min(sy + bh, viewBot);
				if (y1 > y0) FillRect(bx, y0, bw, y1 - y0, face);
			}
			FillRect(bx + 3, sy + 1, bw - 6, 1, Lighten(face, 14));
			for (size_t r = 0; r < msg.rows.size(); ++r)
			{
				PrintAt(FONT10ARIAL,
						mine ? FONT_MCOLOR_WHITE : FONT_GRAY1,
						bx + 7, sy + 5 + INT32(r) * rowH,
						msg.rows[r]);
			}
		}
		SetFontDestBuffer(FRAME_BUFFER);

		// a slim thumb, wheel-driven: two people rarely need arrows
		FillRect(railX, viewTop, 10, viewH, CP_RGB_BLUE_PALE);
		FillRect(railX + 3, viewTop + 4, 4, viewH - 8, CP_RGB_INK);
		if (C.scrollMax > 0)
		{
			const INT32 trackH = viewH - 8;
			const INT32 th = std::max<INT32>(10,
					trackH * viewH / std::max<INT32>(1, total));
			const INT32 to = (trackH - th) * topScroll /
					std::max<INT32>(1, total - viewH);
			FillRect(railX + 2, viewTop + 4 + to, 6, th,
					CP_RGB_BLUE_DK);
		}

		// the status band: typing dots on the left, the florist forever
		// on the right
		FillRect(wx + 2, wy + wh - 18, ww - 4, 16, CP_RGB_BLUE_PALE);
		if (!C.pend.empty() &&
		    C.pend.front().due < GetJA2Clock() + 5000 &&
		    C.pend.front().who != LNG_SYS)
		{
			static const char* const dots[4] = { "", ".", "..", "..." };
			LoungeChip(C.pend.front().who, wx + 9, wy + wh - 17);
			PrintAt(FONT10ARIAL, FONT_GRAY2, wx + 30, wy + wh - 14,
					ST::format(gfCupidGerman ? "{} tippt{}"
								 : "{} is typing{}",
						LoungeNick(C.pend.front().who),
						dots[(GetJA2Clock() / 350) % 4]));
		}
		else
		{
			PrintAt(FONT10ARIAL, FONT_GRAY4, wx + 8, wy + wh - 14,
					gfCupidGerman
						? "niemand liest mit. vermutlich."
						: "nobody is listening in. probably.");
		}
		{
			const bool fhov = Hover(gCupidChatFlowerRegion);
			const ST::string link = gfCupidGerman
					? "blumen >" : "flowers >";
			const INT32 lw = StringPixLength(link, FONT10ARIALBOLD);
			PrintAt(FONT10ARIALBOLD,
					fhov ? FONT_MCOLOR_WHITE : FONT_LTRED,
					wx + ww - 10 - lw, wy + wh - 14, link);
			if (fhov)
			{
				FillRect(wx + ww - 10 - lw, wy + wh - 4, lw, 1,
						CP_RGB_PINK);
			}
		}

		// the input line, identical furniture to the lounge's
		const bool hov = Hover(gCupidChatSayRegion);
		DropShadow(wx, CPC_SAY_Y, ww, 26);
		if (PlayerHasProfile())
		{
			const INT32 sendW = 58;
			FillRounded(wx, CPC_SAY_Y, ww, 26, CP_RGB_BLUE_DK, 3,
					CP_RGB_BG);
			FillRounded(wx + 1, CPC_SAY_Y + 1, ww - 2, 24, CP_RGB_INK,
					3, CP_RGB_BLUE_DK);
			if (gCupidChatInput.empty())
			{
				PrintAt(FONT10ARIAL, FONT_GRAY4, wx + 10,
						CPC_SAY_Y + 9, gfCupidGerman
							? "etwas privates tippen..."
							: "type something private...");
			}
			else
			{
				ST::string shown = ST::string(gCupidChatInput);
				const INT32 boxW = ww - sendW - 26;
				while (shown.size() > 1 &&
					StringPixLength(shown, FONT10ARIAL) > boxW)
				{
					shown = shown.substr(1);
				}
				PrintAt(FONT10ARIAL, FONT_MCOLOR_WHITE, wx + 10,
						CPC_SAY_Y + 9, shown);
				if ((GetJA2Clock() / 400) % 2 == 0)
				{
					PrintAt(FONT10ARIAL, FONT_MCOLOR_WHITE,
							wx + 12 + StringPixLength(shown,
									FONT10ARIAL),
							CPC_SAY_Y + 9, "_");
				}
			}
			GelPill(wx + ww - sendW - 4, CPC_SAY_Y + 3, sendW, 20,
					hov ? CP_RGB_PINK_LITE : CP_RGB_PINK,
					CP_RGB_PINK_LITE, CP_RGB_PINK_DK, CP_RGB_INK);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
					wx + ww - sendW / 2 - 4, CPC_SAY_Y + 9,
					gfCupidGerman ? "SENDEN" : "SEND");
		}
		else
		{
			GelPill(wx, CPC_SAY_Y, ww, 26, CP_RGB_CARD_DIM,
					CP_RGB_BLUE_LITE, CP_RGB_GREY, CP_RGB_BG);
			PrintCentred(FONT10ARIALBOLD, FONT_GRAY4, wx + ww / 2,
					CPC_SAY_Y + 9, gfCupidGerman
						? "ZUM MITREDEN REGISTRIEREN (ICH-TAB)"
						: "REGISTER TO SPEAK (THE ME TAB)");
		}
	}

	// the about page: every 1999 site had one, and it was always this
	void RenderAbout()
	{
		const INT32 cx = CPL_X, cy = 8;
		const int lang = gfCupidGerman ? 1 : 0;
		DropShadow(cx, cy, CP_CARD_W, 302);
		FillCard(cx, cy, CP_CARD_W, 302, CP_RGB_CARD, CP_RGB_BLUE_DK,
				CP_RGB_BG);
		FillRounded(cx + 4, cy + 4, CP_CARD_W - 8, 46,
				FROMRGB(52, 13, 19), 4, CP_RGB_CARD);
		Stipple(cx + 4, cy + 4, CP_CARD_W - 8, 46, FROMRGB(64, 20, 27),
				FROMRGB(43, 9, 14));
		PrintStamped(FONT14ARIAL, FONT_MCOLOR_WHITE, cx + CP_CARD_W / 2,
				cy + 12, lang ? "UEBER C.U.P.I.D." : "ABOUT C.U.P.I.D.");
		PrintCentred(FONT10ARIAL, FONT_GRAY4, cx + CP_CARD_W / 2, cy + 30,
				lang ? "gegr. 1999 - menschen, fuer die man stirbt."
				     : "est. 1999 - people worth dying for.");

		// the story, as speck tells it
		static const char* const story[2] =
		{
			"The Certified Union of Professionals In Dating is the "
			"web's FIRST matchmaking service for working mercenaries. "
			"Founded, coded, moderated and audited by Speck T. Kline "
			"after the I.M.P. questionnaire revealed that shooters "
			"have feelings too. Every profile is drawn from verified "
			"personality data. Every kiss is logged. Every wedding "
			"is invoiced.",
			"Die Certified Union of Professionals In Dating ist der "
			"ERSTE Vermittlungsdienst des Webs fuer arbeitende "
			"Soeldner. Gegruendet, programmiert, moderiert und "
			"gebucht von Speck T. Kline, nachdem der I.M.P.-Fragebogen "
			"bewies, dass auch Schuetzen Gefuehle haben. Jedes Profil "
			"basiert auf geprueften Persoenlichkeitsdaten. Jeder Kuss "
			"wird protokolliert. Jede Hochzeit wird in Rechnung "
			"gestellt.",
		};
		DisplayWrappedString(UINT16(CP_X(cx + 16)), UINT16(CP_Y(cy + 58)),
				UINT16(CP_CARD_W - 32), 2, FONT10ARIAL, FONT_GRAY1,
				story[lang], FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		// the credo, framed
		FillRounded(cx + 16, cy + 148, CP_CARD_W - 32, 34, CP_RGB_INK, 4,
				CP_RGB_CARD);
		FillRect(cx + 16, cy + 148, CP_CARD_W - 32, 1, CP_RGB_PINK);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTYELLOW,
				cx + CP_CARD_W / 2, cy + 154,
				lang ? "DAS CREDO" : "THE CREDO");
		PrintCentred(FONT10ARIAL, FONT_GRAY1, cx + CP_CARD_W / 2,
				cy + 166, lang
					? "ehrlich kuessen. bildlich toeten. einmal heiraten."
					: "kiss honestly. kill figuratively. marry once.");

		// the entire staff
		FillRect(cx + 16, cy + 192, 16, 16, CP_RGB_INK);
		DrawHeart(cx + 20, cy + 196, 1, CP_RGB_GOLD);
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx + 40, cy + 194,
				"Speck T. Kline");
		PrintAt(FONT10ARIAL, FONT_GRAY4, cx + 40, cy + 206, lang
				? "webmaster, gruender, buchhalter, eheberater"
				: "webmaster, founder, accountant, marriage counselor");

		// the trophy shelf
		{
			const INT32 by = cy + 228;
			FillRounded(cx + 16, by, 108, 30, FROMRGB(96, 76, 34), 3,
					CP_RGB_CARD);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_LTYELLOW,
					cx + 70, by + 4, "WEB EXCELLENCE");
			PrintCentred(FONT10ARIAL, FONT_GRAY1, cx + 70, by + 16,
					lang ? "PREIS '99" : "AWARD '99");
			FillRounded(cx + 132, by, 74, 30, FROMRGB(30, 40, 58), 3,
					CP_RGB_CARD);
			PrintCentred(FONT10ARIALBOLD, FONT_LTGREEN, cx + 169,
					by + 4, "Y2K OK");
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cx + 169, by + 16,
					lang ? "geprueft" : "certified");
			FillRounded(cx + 214, by, 136, 30, CP_RGB_INK, 3,
					CP_RGB_CARD);
			PrintCentred(FONT10ARIAL, FONT_GRAY2, cx + 282, by + 4,
					lang ? "am besten in 800x600" : "best viewed in 800x600");
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cx + 282, by + 16,
					"Netscape 4+");
		}

		// the small print
		PrintCentred(FONT10ARIAL, FONT_GRAY4, cx + CP_CARD_W / 2, cy + 268,
				lang ? "C.U.P.I.D. ist nicht mit A.I.M. verbunden. "
				       "die anwaelte wissen bescheid."
				     : "C.U.P.I.D. is not affiliated with A.I.M. "
				       "the lawyers have been notified.");
		PrintCentred(FONT10ARIAL, FONT_GRAY2, cx + CP_CARD_W / 2, cy + 282,
				"webmaster@mk.an");
	}

	void RenderSplash()
	{
		// the deck stays put underneath; the alert is a popup, dimmed
		// stage and all, not a page of its own
		FRAME_BUFFER->ShadowRect(CP_X(0), CP_Y(0),
				CP_X(CP_PAGE_W) - 1, CP_Y(CP_PAGE_H) - 1);
		DropShadow(CP_SPL_X, CP_SPL_Y, CP_SPL_W, CP_SPL_H);
		FillCard(CP_SPL_X, CP_SPL_Y, CP_SPL_W, CP_SPL_H, CP_RGB_CARD,
				CP_RGB_PINK, CP_RGB_BG);
		Stipple(CP_SPL_X + 2, CP_SPL_Y + 2, CP_SPL_W - 4, CP_SPL_H - 4,
				FROMRGB(64, 20, 27), FROMRGB(43, 9, 14));

		// hearts inside the frame; the algorithm is pleased with itself
		uint32_t seed = gCupidSplashPid * 2654435761u + 5;
		for (int i = 0; i < 10; ++i)
		{
			seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
			const INT32 x = CP_SPL_X + 10 + INT32(seed % (CP_SPL_W - 26));
			const INT32 y = CP_SPL_Y + 10 +
					INT32((seed >> 9) % (CP_SPL_H - 40));
			DrawHeart(x, y, 1 + int(seed % 2), CP_RGB_BLUE);
		}

		PrintStamped(FONT14ARIAL, FONT_LTRED, CP_SPL_X + CP_SPL_W / 2,
				CP_SPL_Y + 12, T(CPS_SPLASH_TITLE));
		PrintCentred(FONT10ARIAL, FONT_GRAY4, CP_SPL_X + CP_SPL_W / 2,
				CP_SPL_Y + 28, gfCupidGerman
					? "(stellen Sie sich eine triumphale MIDI vor)"
					: "(imagine a triumphant MIDI playing)");

		// the two of you, full photos, side by side, as the format demands
		const INT32 fy = CP_SPL_Y + 44;
		const INT32 x1 = CP_SPL_X + (CP_SPL_W - 253) / 2;
		const INT32 x2 = x1 + 139;
		DropShadow(x1, fy, 114, 134);
		FillCard(x1, fy, 114, 134, CP_RGB_CARD, CP_RGB_PINK, CP_RGB_BG);
		if (guiCupidSelfBig)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidSelfBig, 0,
					CP_X(x1 + 4), CP_Y(fy + 6));
		}
		else
		{
			FillRect(x1 + 4, fy + 6, CP_PHOTO_W, CP_PHOTO_H,
					CP_RGB_CARD_DIM);
			PrintCentred(FONT10ARIAL, FONT_GRAY4, x1 + 57, fy + 62,
					T(CPS_NO_PHOTO));
		}
		DrawHeart(x1 + 120, fy + 56, 3, CP_RGB_PINK);
		DropShadow(x2, fy, 114, 134);
		FillCard(x2, fy, 114, 134, CP_RGB_CARD, CP_RGB_PINK, CP_RGB_BG);
		SGPVObject* face = BigFaceFor(gCupidSplashPid);
		if (face)
		{
			BltVideoObject(FRAME_BUFFER, face, 0, CP_X(x2 + 4),
					CP_Y(fy + 6));
		}

		MERCPROFILESTRUCT const& p = GetProfile(gCupidSplashPid);
		PrintCentred(FONT10ARIAL, FONT_GRAY1, CP_SPL_X + CP_SPL_W / 2,
				fy + 140, ST::format(T(CPS_SPLASH_SUB), p.zNickname));
		PrintCentred(FONT10ARIAL, FONT_GRAY4, CP_SPL_X + CP_SPL_W / 2,
				fy + 154, gfCupidGerman
					? "diese seite fuer ihre unterlagen ausdrucken."
					: "print this page for your records.");

		// the two exits, side by side inside the popup
		static const CupidStr labels[2] = { CPS_SPLASH_KEEP,
				CPS_SPLASH_CHAT };
		for (int i = 0; i < 2; ++i)
		{
			const INT32 bx = CP_SPL_X + 10 + i * 160;
			const bool hov = Hover(gCupidSplashBtnRegion[i]);
			DropShadow(bx, CP_SPL_BTN_Y, 150, 26);
			GelPill(bx, CP_SPL_BTN_Y, 150, 26,
					hov ? CP_RGB_PINK_LITE : CP_RGB_PINK,
					hov ? CP_RGB_PINK_PALE : CP_RGB_PINK_LITE,
					CP_RGB_PINK_DK, CP_RGB_BG);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, bx + 75,
					CP_SPL_BTN_Y + 9, T(labels[i]));
		}
	}
}

// --- persistence -----------------------------------------------------------
CupidPersist CupidGetPersist() { return gCupidPersist; }
void CupidSetPersist(const CupidPersist& p) { gCupidPersist = p; }

void CupidRecordImpAnswers(const INT32 (&answers)[16])
{
	// I.M.P. is about to tally these and throw them away; the site keeps the
	// sheet. Unanswered slots (-1) pack as the 0xF sentinel.
	for (int q = 0; q < DatingGame::NUM_QUESTIONS; ++q)
	{
		const INT32 a = answers[q];
		SetAnswer(q, (a >= 0 && a < DatingGame::ANSWER_COUNT[q])
				? UINT8(a) : DatingGame::NO_ANSWER);
	}
	gCupidPersist.ubFlags |= CUPID_FLAG_IMP_ANSWERS;
}

// --- page lifecycle ---------------------------------------------------------
void EnterCupid()
{
	try
	{
		guiCupidLogo  = AddVideoObjectFromFile("sti/laptop/cupidlogo.sti");
		guiCupidIcons = AddVideoObjectFromFile("sti/laptop/cupidicons.sti");
		guiCupidNight  = AddVideoObjectFromFile("sti/laptop/cupidnight.sti");
		guiCupidTiles  = AddVideoObjectFromFile(
				"sti/laptop/mahjongtilessmall.sti");
		guiCupidDragon = AddVideoObjectFromFile(
				"sti/laptop/mahjongdragon.sti");
		guiCupidPanels = AddVideoObjectFromFile("sti/laptop/cupidpanels.sti");
	}
	catch (...)
	{
		// chrome only; the text carries the page
	}


	if (LaptopSaveInfo.fIMPCompletedFlag)
	{
		try
		{
			guiCupidSelf    = LoadSmallPortrait(GetProfile(PlayerImpPid()));
			guiCupidSelfBig = LoadBigPortrait(GetProfile(PlayerImpPid()));
		}
		catch (...) {}
	}

	BuildRoster();
	gCupidFaces33.assign(gCupidRoster.size(), nullptr);
	for (size_t i = 0; i < gCupidRoster.size(); ++i)
	{
		if (gCupidRoster[i].locked) continue;
		try
		{
			gCupidFaces33[i] = LoadSmallPortrait(GetProfile(gCupidRoster[i].pid));
		}
		catch (...) {}
	}

	// bake 16bpp surfaces so the lounge can stretch-blit its mini chips,
	// exactly as the parlour bar does
	gCupidChipSurf.assign(gCupidRoster.size(), nullptr);
	for (size_t i = 0; i < gCupidRoster.size(); ++i)
	{
		if (!gCupidFaces33[i]) continue;
		SGPVSurface* const surf = AddVideoSurface(48, 43, 16);
		surf->Fill(Get16BPPColor(CP_RGB_INK));
		BltVideoObject(surf, gCupidFaces33[i], 0, 0, 0);
		gCupidChipSurf[i] = surf;
	}
	if (LaptopSaveInfo.fIMPCompletedFlag)
	{
		try
		{
			if (!guiCupidSelf)
			{
				guiCupidSelf = LoadSmallPortrait(GetProfile(PlayerImpPid()));
			}
		}
		catch (...) {}
	}
	if (guiCupidSelf)
	{
		guiCupidSelfChip = AddVideoSurface(48, 43, 16);
		guiCupidSelfChip->Fill(Get16BPPColor(CP_RGB_INK));
		BltVideoObject(guiCupidSelfChip, guiCupidSelf, 0, 0, 0);
	}

	// the daily bookkeeping: allowance, streak, and the ad campaign's end
	RefreshDailyLikes();
	const UINT16 today = UINT16(GetWorldDay());
	if (gCupidPersist.usLastVisitDay != today)
	{
		gCupidPersist.ubStreak =
			(gCupidPersist.usLastVisitDay + 1 == today)
				? UINT8(std::min(255, gCupidPersist.ubStreak + 1)) : 1;
		gCupidPersist.usLastVisitDay = today;
		gCupidPersist.usViews = UINT16(std::min<UINT32>(0xFFFF,
					gCupidPersist.usViews + 1));
		// a fresh day earns a fresh congratulations
		gfCupidPopupUp = PlayerHasProfile() && !IsGold();
	}
	gCupidPersist.ubFlags |= CUPID_FLAG_VISITED;

	// a match who died on your contract gets one letter from the management
	if (!(gCupidPersist.ubFlags & CUPID_FLAG_CONDOLED))
	{
		for (const Member& m : gCupidRoster)
		{
			if (m.locked || !MemberIsDead(m.pid) || !IsMatched(m.pid))
			{
				continue;
			}
			gCupidPersist.ubFlags |= CUPID_FLAG_CONDOLED;
			AddEmailWithSpecialData(CUPID_EMAIL_CONDOLENCE, 0,
					CUPID_SPECK_SENDER, GetWorldTotalMin(),
					INT32(m.pid), 0);
			break;
		}
	}

	// browsing defaults to the opposite of your own sheet; the widget on
	// the ME page cycles it
	if (PlayerHasProfile() || LaptopSaveInfo.fIMPCompletedFlag)
	{
		giCupidSeek = BuildPlayerProfile().sex == DatingGame::SEX_FEMALE
				? SEEK_MEN : SEEK_WOMEN;
	}
	else
	{
		giCupidSeek = SEEK_EVERYONE;
	}

	BuildDeck();
	StartPhotoLoad(); // the first photo downloads like all the others
	gCupidPage = CPP_DECK;
	gfCupidQuizLive = false;
	giCupidQuizQ = -1;
	giCupidTicker = IsGold() ? CPS_TICKER_GOLD
		: PlayerHasProfile() ? CPS_TICKER_DEFAULT : CPS_TICKER_NO_PROFILE;

	CupidPlaceRegions();
	CupidRedraw();
}

void ExitCupid()
{
	CupidRemoveRegions();
	if (guiCupidLogo)  { DeleteVideoObject(guiCupidLogo);  guiCupidLogo  = nullptr; }
	if (guiCupidNight)  { DeleteVideoObject(guiCupidNight);  guiCupidNight  = nullptr; }
	if (guiCupidTiles)  { DeleteVideoObject(guiCupidTiles);  guiCupidTiles  = nullptr; }
	if (guiCupidDragon) { DeleteVideoObject(guiCupidDragon); guiCupidDragon = nullptr; }
	if (guiCupidPanels) { DeleteVideoObject(guiCupidPanels); guiCupidPanels = nullptr; }
	if (guiCupidIcons) { DeleteVideoObject(guiCupidIcons); guiCupidIcons = nullptr; }
	if (guiCupidSelf)  { DeleteVideoObject(guiCupidSelf);  guiCupidSelf  = nullptr; }
	if (guiCupidSelfBig) { DeleteVideoObject(guiCupidSelfBig); guiCupidSelfBig = nullptr; }
	if (guiCupidBig)   { DeleteVideoObject(guiCupidBig);   guiCupidBig   = nullptr; }
	if (guiCupidFace)  { DeleteVideoObject(guiCupidFace);  guiCupidFace  = nullptr; }
	gCupidBigPid  = 0xFF;
	gCupidFacePid = 0xFF;
	for (SGPVSurface* s : gCupidChipSurf)
	{
		if (s) DeleteVideoSurface(s);
	}
	gCupidChipSurf.clear();
	if (guiCupidSelfChip)
	{
		DeleteVideoSurface(guiCupidSelfChip);
		guiCupidSelfChip = nullptr;
	}
	for (SGPVObject* f : gCupidFaces33)
	{
		if (f) DeleteVideoObject(f);
	}
	gCupidFaces33.clear();
}

void RenderCupid()
{
	RenderWallpaper();
	// the leaderboard lives under the deck's card; the other pages keep
	// their column to themselves
	if (gCupidPage == CPP_DECK || gCupidPage == CPP_SPLASH ||
	    gCupidPage == CPP_DETAIL || gCupidPage == CPP_CHAT ||
	    gCupidPage == CPP_ABOUT)
	{
		RenderSideAds();
	}
	RenderNavMenu();
	switch (gCupidPage)
	{
		case CPP_DECK:    RenderDeck();    break;
		case CPP_MATCHES: RenderMatches(); break;
		case CPP_ME:      RenderMe();      break;
		case CPP_DETAIL:  RenderDetail();  break;
		case CPP_SPLASH:  RenderDeck(); RenderSplash(); break;
		case CPP_LOUNGE:  RenderLounge();  break;
		case CPP_CHAT:    RenderChat();    break;
		case CPP_ABOUT:   RenderAbout();   break;
	}
	MarkButtonsDirty();
	RenderWWWProgramTitleBar();
	InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
			LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
}

void HandleCupid()
{
	// hover states repaint the moment the pointer crosses a control
	{
		UINT32 uiHover = 0;
		int bit = 0;
		// a rolling hash, not a bitmask: the region count long ago
		// outgrew 32 bits
		auto acc = [&](const MOUSE_REGION& r)
		{
			uiHover = uiHover * 33u + (Hover(r) ? 2u : 1u);
			++bit;
		};
		for (const MOUSE_REGION& r : gCupidTabRegion)    acc(r);
		for (const MOUSE_REGION& r : gCupidActionRegion) acc(r);
		for (const MOUSE_REGION& r : gCupidMatchRegion)  acc(r);
		for (const MOUSE_REGION& r : gCupidAnswerRegion) acc(r);
		acc(gCupidPassRegion);
		acc(gCupidLikeRegion);
		acc(gCupidMarryRegion);
		acc(gCupidNoticeOkRegion);
		acc(gCupidNoticeCtaRegion);
		acc(gCupidPopupXRegion);
		acc(gCupidLoungeSayRegion);
		acc(gCupidLoungeUpRegion);
		acc(gCupidLoungeDownRegion);
		acc(gCupidRailBackRegion);
		acc(gCupidSeekRegion);
		acc(gCupidChatSayRegion);
		acc(gCupidChatFaceRegion);
		acc(gCupidChatFlowerRegion);
		acc(gCupidWebmasterRegion);
		for (const MOUSE_REGION& r : gCupidSplashBtnRegion) acc(r);
		for (const MOUSE_REGION& r : gCupidRailBtnRegion)   acc(r);
		for (const MOUSE_REGION& r : gCupidCardTabRegion) acc(r);
		acc(gCupidSkipRegion);
		acc(gCupidPrevRegion);
		acc(gCupidAdCloseRegion);
		acc(gCupidAdCtaRegion);
		for (const MOUSE_REGION& r : gCupidRoomRegion) acc(r);
		static UINT32 uiLastHover = 0;
		if (uiHover != uiLastHover)
		{
			uiLastHover = uiHover;
			CupidRedraw();
		}
	}

	// the photo comes down the wire a few rows at a time - after the
	// modem deigns to connect, and with the occasional mid-photo choke
	if (gCupidPage == CPP_DECK && giCupidPhotoReveal < CP_PHOTO_H)
	{
		const UINT32 now = GetJA2Clock();
		if (now >= guiCupidRevealNext)
		{
			UINT32 delay;
			if (gfCupidFastLine)
			{
				// a good line: the picture pours in
				giCupidPhotoReveal += 10 + INT32(Random(8));
				delay = 30 + Random(40);
			}
			else
			{
				giCupidPhotoReveal += 3 + INT32(Random(4));
				delay = 50 + Random(120);
				if (gfCupidChokePending &&
				    giCupidPhotoReveal >= giCupidRevealChoke)
				{
					gfCupidChokePending = false;
					delay = 700 + Random(1300); // the line thinks it over
				}
			}
			guiCupidRevealNext = now + delay;
			CupidRedraw();
		}
	}

	// the room lives while you watch it: queued lines land, somebody
	// finds something new to say, and the typing dots keep time
	if (gCupidPage == CPP_LOUNGE && !RoomLocked())
	{
		LoungeRoom& R = Room();
		const UINT32 now = GetJA2Clock();
		while (!R.pend.empty() && now >= R.pend.front().due)
		{
			const LoungePending line = R.pend.front();
			R.pend.erase(R.pend.begin());
			LoungePush(line.who, line.text);
		}
		if (now >= R.next)
		{
			LoungeBeat(false);
			R.next = now + 3500 + LoungeRoll() % 5000;
		}
		static UINT32 uiDots = 0;
		const UINT32 dots = now / 350;
		if (!R.pend.empty() && dots != uiDots)
		{
			uiDots = dots;
			CupidRedraw();
		}
		R.seen = now;
	}

	// the private line delivers what was queued, and shows the dots
	if (gCupidPage == CPP_CHAT)
	{
		ChatThread& C = Chat();
		const UINT32 now = GetJA2Clock();
		while (!C.pend.empty() && now >= C.pend.front().due)
		{
			const LoungePending line = C.pend.front();
			C.pend.erase(C.pend.begin());
			ChatPush(line.who, line.text);
		}
		static UINT32 uiChatDots = 0;
		const UINT32 dots = now / 350;
		if (!C.pend.empty() && dots != uiChatDots)
		{
			uiChatDots = dots;
			CupidRedraw();
		}
		// nobody likes typing into silence - not even them
		const int idx = RosterIndexOf(gCupidChatPid);
		if (C.pend.empty() && C.idleAt != 0 && now >= C.idleAt &&
		    idx >= 0 && !MemberIsDead(gCupidChatPid) && C.mood > -2 &&
		    !C.log.empty() && C.log.back().who == LNG_YOU)
		{
			C.idleAt = 0;
			const int lang = gfCupidGerman ? 1 : 0;
			static const char* const idleT[2][3] =
			{
				{ "still there? my other window is a minefield. "
				  "literally.",
				  "you type slow. sniper's patience. i respect it.",
				  "say something. the silence is very in character "
				  "for me, not for you." },
				{ "noch da? mein anderes fenster ist ein minenfeld. "
				  "woertlich.",
				  "sie tippen langsam. scharfschuetzengeduld. "
				  "respekt.",
				  "sagen sie was. schweigen passt zu mir, nicht zu "
				  "ihnen." },
			};
			ChatSay(INT8(idx), ChatPick(idleT[lang], 3));
		}
	}

	// the marquee crawls on every page that shows the rail
	if (gCupidPage != CPP_SPLASH)
	{
		static UINT32 uiLastCrawl = 0;
		const UINT32 crawl = GetJA2Clock() / 66;
		if (crawl != uiLastCrawl)
		{
			uiLastCrawl = crawl;
			CupidRedraw();
		}
	}

	// the NEW!! tag blinks on the era's clock
	{
		static UINT32 uiLastBlink = 0;
		const UINT32 uiBlink = GetJA2Clock() / 400;
		if (uiBlink != uiLastBlink)
		{
			uiLastBlink = uiBlink;
			if (gCupidPage == CPP_DECK) CupidRedraw();
		}
	}

	// a committed card flies off the edge, then the verdict lands
	if (giCupidFlyDir != 0)
	{
		giCupidCardDx += giCupidFlyDir * CP_FLY_STEP;
		if (std::abs(giCupidCardDx) > CP_CARD_W + 180)
		{
			const int dir = giCupidFlyDir;
			giCupidFlyDir = 0;
			CommitSwipe(dir);
		}
		CupidRedraw();
	}
}

bool CupidHandleTypedKey(UINT32 usParam, UINT16 usKeyState)
{
	if (gfCupidPopupUp) return false; // the popup owns the moment
	const bool deck = gCupidPage == CPP_DECK && PlayerHasProfile();
	const bool column = gCupidPage == CPP_DETAIL ||
			(gCupidPage == CPP_ME && !gfCupidQuizLive &&
			 PlayerHasProfile() && HaveStoredAnswers());
	const bool lounge = gCupidPage == CPP_LOUNGE;
	const bool chat   = gCupidPage == CPP_CHAT;
	if (!deck && !column && !lounge && !chat) return false;
	if (chat && PlayerHasProfile())
	{
		if (usParam == SDLK_RETURN || usParam == SDLK_KP_ENTER)
		{
			ChatSpeak();
			CupidRedraw();
			return true;
		}
		if (usParam == SDLK_BACKSPACE)
		{
			if (!gCupidChatInput.empty())
			{
				gCupidChatInput.pop_back();
				CupidRedraw();
			}
			return true;
		}
		if (usParam >= 32 && usParam < 127) return true;
	}
	if (lounge && PlayerHasProfile() && !RoomLocked())
	{
		// the chat line is focused whenever the room is open
		if (usParam == SDLK_RETURN || usParam == SDLK_KP_ENTER)
		{
			LoungeSpeak();
			CupidRedraw();
			return true;
		}
		if (usParam == SDLK_BACKSPACE)
		{
			if (!gCupidLoungeInput.empty())
			{
				gCupidLoungeInput.pop_back();
				CupidRedraw();
			}
			return true;
		}
		// printable characters arrive as TEXT_INPUT events; swallow the
		// raw keys so no laptop shortcut fires mid-sentence
		if (usParam >= 32 && usParam < 127) return true;
	}
	switch (usParam)
	{
		case SDLK_LEFT:
			if (!deck) return false;
			if (gCupidDeckPos > 0)
			{
				RetreatCard();
				CupidRedraw();
			}
			return true;
		case SDLK_RIGHT:
			if (!deck) return false;
			if (CurrentCard().kind != CARD_END)
			{
				AdvanceCard();
				CupidRedraw();
			}
			return true;
		case SDLK_UP:
		{
			if (chat)
			{
				ChatThread& C = Chat();
				const INT32 next = std::min(C.scroll + 26,
						std::max(0, C.scrollMax));
				if (next != C.scroll)
				{
					C.scroll = next;
					CupidRedraw();
				}
				return true;
			}
			if (lounge)
			{
				LoungeRoom& R = gCupidRooms[giCupidRoom];
				const INT32 next = std::min(R.scroll + 26,
						std::max(0, R.scrollMax));
				if (next != R.scroll)
				{
					R.scroll = next;
					CupidRedraw();
				}
				return true;
			}
			giCupidCardTab = (giCupidCardTab + 2) % 3;
			CupidRedraw();
			return true;
		}
		case SDLK_DOWN:
		{
			if (chat)
			{
				ChatThread& C = Chat();
				const INT32 next = std::max(0, C.scroll - 26);
				if (next != C.scroll)
				{
					C.scroll = next;
					CupidRedraw();
				}
				return true;
			}
			if (lounge)
			{
				LoungeRoom& R = gCupidRooms[giCupidRoom];
				const INT32 next = std::max(0, R.scroll - 26);
				if (next != R.scroll)
				{
					R.scroll = next;
					CupidRedraw();
				}
				return true;
			}
			giCupidCardTab = (giCupidCardTab + 1) % 3;
			CupidRedraw();
			return true;
		}
		default: return false;
	}
}

// Layout-aware typing for the chat line: the engine's TEXT_INPUT events
// know the keyboard layout; raw keycodes do not.
bool CupidHandleTextInput(const ST::utf32_buffer& codepoints)
{
	if (gfCupidPopupUp) return false;
	if (gCupidPage == CPP_CHAT && PlayerHasProfile())
	{
		bool grew = false;
		for (char32_t cp : codepoints)
		{
			if (cp < 32 || cp > 126) continue;
			if (gCupidChatInput.size() >= 110) break;
			gCupidChatInput += char(cp);
			grew = true;
		}
		if (grew) CupidRedraw();
		return true;
	}
	if (gCupidPage != CPP_LOUNGE) return false;
	if (!PlayerHasProfile() || RoomLocked()) return false;
	bool changed = false;
	for (char32_t cp : codepoints)
	{
		if (cp < 32 || cp > 126) continue;
		if (gCupidLoungeInput.size() >= 110) break;
		gCupidLoungeInput += char(cp);
		changed = true;
	}
	if (changed) CupidRedraw();
	return true;
}
