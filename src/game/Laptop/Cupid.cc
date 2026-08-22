// Mercs & Kisses - "Where the tough get tender." A Speck T. Kline company
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
#include "EMail.h"
#include "Finances.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
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

// the card, dealt centre stage
#define CP_CARD_W       190
#define CP_CARD_H       340
#define CP_CARD_X       ((502 - CP_CARD_W) / 2)
#define CP_CARD_Y       30
#define CP_PHOTO_W      106
#define CP_PHOTO_H      122

// swipe thresholds and animation speed, in pixels
#define CP_SWIPE_COMMIT 55
#define CP_FLY_STEP     34

// the two verdict circles sit under the photo at the card's edges, the
// name held between them
#define CP_BTN_SIZE     34
#define CP_BTN_Y        (CP_CARD_Y + CP_PHOTO_H + 16)
#define CP_BTN_PASS_X   (CP_CARD_X + 10)
#define CP_BTN_LIKE_X   (CP_CARD_X + CP_CARD_W - 10 - CP_BTN_SIZE)

// the "small" face is the A.I.M. mugshot (faces/NN.sti frame 0), measured
// from the game data: 48x43. The 33face files are 14x15 tactical heads and
// the merc 65faces are 31x27 - neither survives a layout.
#define CP_FACE_SM_W    48
#define CP_FACE_SM_H    43

// the flank columns: 112 wide at the page edges, two mugshots abreast
#define CP_COL_W        112
#define CP_LCOL_X       8
#define CP_RCOL_X       (502 - 8 - CP_COL_W)

#define CP_FREE_LIKES_A_DAY 10
#define CP_RETAKE_PRICE     25
#define CP_GOLD_PRICE       10

// cupidicons.sti frame order
#define CP_ICON_VERIFIED 5
#define CP_ICON_HEART    6

// The palette: powder blue and hot pink on off-white, dark ink for the type.
// A 1999 dating site did not do subtle, and neither does Speck.
#define CP_RGB_BG        FROMRGB(228, 222, 209)
#define CP_RGB_BLUE      FROMRGB(168, 205, 232)
#define CP_RGB_BLUE_DK   FROMRGB(108, 152, 190)
#define CP_RGB_BLUE_PALE FROMRGB(210, 229, 243)
#define CP_RGB_CARD      FROMRGB(253, 252, 248)
#define CP_RGB_CARD_DIM  FROMRGB(222, 229, 235)
#define CP_RGB_INK       FROMRGB( 38,  36,  46)
#define CP_RGB_PINK      FROMRGB(230,  62, 118)
#define CP_RGB_PINK_DK   FROMRGB(178,  30,  82)
#define CP_RGB_GOLD      FROMRGB(202, 158,  74)
#define CP_RGB_LIKE      FROMRGB( 58, 152,  66)
#define CP_RGB_NOPE      FROMRGB(206,  54,  48)
#define CP_RGB_GREY      FROMRGB(148, 148, 152)
#define CP_RGB_SHADOW    FROMRGB(198, 189, 173)
#define CP_RGB_PINK_PALE FROMRGB(238, 205, 216)
#define CP_RGB_BLUE_WALL FROMRGB(212, 222, 231)
#define CP_RGB_MAT       FROMRGB(255, 255, 255)
// the aqua-gel tints: every control gets a lit top half and a shaded foot
#define CP_RGB_PINK_LITE FROMRGB(244, 136, 172)
#define CP_RGB_BLUE_LITE FROMRGB(228, 242, 252)
#define CP_RGB_GOLD_LITE FROMRGB(228, 196, 128)
#define CP_RGB_GLOSS     FROMRGB(255, 255, 255)
#define CP_RGB_CARD_LITE FROMRGB(255, 255, 255)

namespace
{
	enum CupidPage
	{
		CPP_DECK,
		CPP_MATCHES,
		CPP_ME,      // profile: import / take / retake the questionnaire
		CPP_DETAIL,  // one member's dossier, from a match or the deck photo
		CPP_SPLASH,  // IT'S A MATCH
	};

	// --- persistence -------------------------------------------------------
	CupidPersist gCupidPersist = { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF }, 0, 0, 0, 0, 0, {}, {}, 0, 0 };

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

	bool IsGold() { return (gCupidPersist.ubFlags & CUPID_FLAG_GOLD) != 0; }

	// --- session state -----------------------------------------------------
	CupidPage gCupidPage = CPP_DECK;
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
	std::vector<Card> gCupidDeck;
	int gCupidDeckPos = 0;

	// who the member is browsing for; the era called this "seeking a".
	// Defaults to the opposite of your own sheet, cycles MEN/WOMEN/EVERYONE.
	enum CupidSeek { SEEK_MEN, SEEK_WOMEN, SEEK_EVERYONE };
	int giCupidSeek = SEEK_EVERYONE;

	// the 1,000,000th-visitor popup: one per fresh day, closable only by
	// its own little X, as was the law of the era
	bool  gfCupidPopupUp = false;

	// the card photo downloads over a 28.8k line: rows revealed so far
	INT32 giCupidPhotoReveal = 0;

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
	SGPVObject* guiCupidIcons = nullptr;
	SGPVObject* guiCupidSelf  = nullptr;   // the member's own mugshot, 48x43
	SGPVObject* guiCupidSelfBig = nullptr; // and the full photo, for the splash
	SGPVObject* guiCupidBig   = nullptr;   // the dealt card's 106x122 photo
	ProfileID   gCupidBigPid  = 0xFF;
	SGPVObject* guiCupidFace  = nullptr;   // 65-face for the splash
	ProfileID   gCupidFacePid = 0xFF;
	std::vector<SGPVObject*> gCupidFaces33; // parallel to gCupidRoster

	MOUSE_REGION gCupidTabRegion[3];
	MOUSE_REGION gCupidCardRegion;
	MOUSE_REGION gCupidPassRegion;
	MOUSE_REGION gCupidLikeRegion;
	MOUSE_REGION gCupidLangRegion;
	MOUSE_REGION gCupidAnswerRegion[8];
	MOUSE_REGION gCupidActionRegion[2]; // ME landing / detail / splash slots
	MOUSE_REGION gCupidMatchRegion[7];  // clickable rows on the matches page
	MOUSE_REGION gCupidSideAdRegion[2]; // the skyscraper banners, deck page
	MOUSE_REGION gCupidRingRegion[3];   // the webring: prev, random, next
	MOUSE_REGION gCupidPopupXRegion;    // the popup's only honest exit
	MOUSE_REGION gCupidPopupCtaRegion;  // and its entire reason to exist
	MOUSE_REGION gCupidFaceRegion[8];   // the sidebar face strips, deck page
	MOUSE_REGION gCupidSeekRegion;      // "I am seeking", the ME page widget
	ProfileID    gCupidFacePids[8] = {};  // who each strip slot showed last

	bool Hover(const MOUSE_REGION& r)
	{
		return (r.uiFlags & MSYS_MOUSE_IN_AREA) != 0;
	}

	// Every control belongs to one page; everything else is switched off so
	// an invisible region can never eat a click meant for the page on show.
	void SyncRegions()
	{
		if (!gfCupidRegionsUp) return;
		const bool popup   = gfCupidPopupUp && gCupidPage == CPP_DECK;
		const bool deck    = gCupidPage == CPP_DECK && !popup;
		const bool actions = (gCupidPage == CPP_ME && !gfCupidQuizLive) ||
					gCupidPage == CPP_DETAIL ||
					gCupidPage == CPP_SPLASH;
		const bool matches = gCupidPage == CPP_MATCHES;

		auto set = [](MOUSE_REGION& r, bool on)
		{
			if (on) r.Enable(); else r.Disable();
		};
		set(gCupidCardRegion, deck);
		set(gCupidPassRegion, deck);
		set(gCupidLikeRegion, deck);
		for (MOUSE_REGION& r : gCupidSideAdRegion) set(r, deck);
		for (MOUSE_REGION& r : gCupidActionRegion) set(r, actions);
		for (MOUSE_REGION& r : gCupidMatchRegion)  set(r, matches);
		set(gCupidPopupXRegion, popup);
		set(gCupidPopupCtaRegion, popup);
		for (MOUSE_REGION& r : gCupidFaceRegion) set(r, deck);
		set(gCupidSeekRegion, gCupidPage == CPP_ME && !gfCupidQuizLive);
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
		CPS_COUNT
	};

	const char* const CUPID_TEXT[2][CPS_COUNT] =
	{
		{
			"DECK", "MATCHES", "ME",
			"{} winks left today", "GOLD - winks never run out",
			"OUT OF WINKS",
			"WINK", "PASS",
			"ONLINE NOW", "AWAY - ON CONTRACT", "ON YOUR PAYROLL",
			"LAST LOGIN: a long time ago", "MARRIED (a satisfied customer)",
			"A.I.M. VERIFIED", "unverified",
			"{}% MATCH", "{}-{}% MATCH",
			"IT'S A MATCH!!", "{} winked back. The algorithm saw it coming.", "KEEP SWIPING", "VIEW PROFILE",
			"YOUR MATCHES", "No matches yet. The deck is waiting.",
			"Click a match to read the full dossier.",
			"{} members already WINKED at you.", "They winked. Now you know who.",
			"MERCS & KISSES GOLD", "Unlimited winks. See who winked at you "
			"FIRST. Prestige beyond measure. One payment of ${}, to me, "
			"Speck T. Kline.", "GET GOLD - ${}", "You are a GOLD member. "
			"Everything I promised is now true.",
			"\"He said he'd never settle down.\"", "\"We are MARRIED now. "
			"Thank you Mercs && Kisses!!!\"", "- Flo, satisfied member",
			"NEW MEMBERS COMING", "The roster grows as M.E.R.C. grows. "
			"Spend generously and love will follow. That is just science.",
			"THAT'S EVERYONE", "You have seen every professional in Arulco. "
			"I am in talks with several other war zones. - S.T.K.",
			"(the heart winks. the X declines. no refunds.)",
			"THE QUESTIONNAIRE", "MY PROFILE",
			"HOW YOU APPEAR IN THE DECK",
			"login streak: {} days   -   winks today: {}",
			"IMPORT MY I.M.P. PROFILE (free)",
			"TAKE THE QUESTIONNAIRE (free)", "RETAKE THE QUESTIONNAIRE (${})",
			"COMPLETE MY PROFILE (${})",
			"YOUR PROFILE IS 100% COMPLETE", "YOUR PROFILE IS 60% COMPLETE",
			"Members with complete profiles receive 3x more responses!!",
			"Compatibility instrument developed independently by Speck T. "
			"Kline. Any resemblance to the Institute of Mercenary "
			"Profiling's questionnaire is coincidental and unlitigated.",
			"First: the questionnaire needs to know. You are...",
			"MALE", "FEMALE", "QUESTION {} OF {}",
			"NO PHOTO", "Take the questionnaire to start swiping - Speck",
			"First thing people notice: {}", "Looking for: {}",
			"Self-summary:",
			"DEAL BREAKERS: {}", "Blocked by {} member(s)",
			"You both answered:", "You differ on:", "You agree on {} of {}.",
			"SEND FLOWERS", "< BACK",
			"where the tough get tender - a Speck T. Kline company",
			"No profile, no romance. The ME tab is right there. - Speck",
			"Out of winks. GOLD never runs out. Just saying. - Speck",
			"Thank you for going GOLD. You complete me. - Speck",
			"Your card was declined. It happens. Not to me. - Speck",
			"Your M.E.R.C. account is overdue. Romance can wait. - Speck",
			"My greatest appreciation for your payment. Thank you. - Speck",
			"SUNDAY SPECIAL!! Double winks all day, per science.",
			"Membership attrition remains within industry norms. - mgmt",
			"advertisement - mercsandkisses.com",
			"!!! CONGRATULATIONS !!!",
			"You are the 1,000,000th visitor to this page!! You have won: "
			"eligibility to purchase MERCS & KISSES GOLD.",
			"CLAIM MY PRIZE",
			"FEATURED PROFILES",
			"LOVERS OF THE MONTH", "watch this space",
			"I am seeking: [ {} ] (click to change)",
			"MEN", "WOMEN", "EVERYONE",
			"Active during the last 24 hours",
			"Active {} days ago",
			"Active a long, long time ago",
			"has BLOCKED {}",
			"TESTIMONIALS: UNDER CONSTRUCTION (since day 1)",
			"in loving memory - profile retained",
		},
		{
			"DECK", "MATCHES", "ICH",
			"noch {} Zwinkern heute", "GOLD - Zwinkern geht nie aus",
			"KEIN ZWINKERN MEHR",
			"ZWINKER", "PASS",
			"JETZT ONLINE", "ABWESEND - IM EINSATZ", "AUF IHRER GEHALTSLISTE",
			"LETZTER LOGIN: vor langer Zeit", "VERHEIRATET (zufriedene "
			"Kundin)",
			"A.I.M.-GEPRUEFT", "ungeprueft",
			"{}% PASSUNG", "{}-{}% PASSUNG",
			"EIN MATCH!!", "{} zwinkert zurueck. Der Algorithmus wusste es "
			"vorher.", "WEITER WISCHEN", "PROFIL ANSEHEN",
			"IHRE MATCHES", "Noch keine Matches. Das Deck wartet.",
			"Klicken Sie ein Match fuer das volle Dossier.",
			"{} Mitglieder ZWINKERN Ihnen bereits zu.", "Sie zwinkerten. "
			"Jetzt wissen Sie, wer.",
			"MERCS & KISSES GOLD", "Unbegrenztes Zwinkern. Sehen Sie ZUERST, "
			"wer Ihnen zuzwinkert. Unermessliches Prestige. Eine Zahlung von "
			"{} $, an mich, Speck T. Kline.", "GOLD HOLEN - {} $",
			"Sie sind GOLD-Mitglied. Alles, was ich versprach, ist jetzt "
			"wahr.",
			"\"Er wollte sich nie binden.\"", "\"Wir sind jetzt VERHEIRATET. "
			"Danke Mercs && Kisses!!!\"", "- Flo, zufriedenes Mitglied",
			"NEUE MITGLIEDER KOMMEN", "Die Liste waechst mit M.E.R.C. Geben "
			"Sie grosszuegig aus, die Liebe folgt. Das ist Wissenschaft.",
			"DAS WAREN ALLE", "Sie haben jeden Profi in Arulco gesehen. Ich "
			"verhandle mit weiteren Kriegsgebieten. - S.T.K.",
			"(das Herz zwinkert. das X lehnt ab. keine Rueckerstattung.)",
			"DER FRAGEBOGEN", "MEIN PROFIL",
			"SO ERSCHEINEN SIE IM DECK",
			"Login-Serie: {} Tage   -   Zwinkern heute: {}",
			"MEIN I.M.P.-PROFIL IMPORTIEREN (gratis)",
			"FRAGEBOGEN AUSFUELLEN (gratis)", "FRAGEBOGEN WIEDERHOLEN ({} $)",
			"PROFIL VERVOLLSTAENDIGEN ({} $)",
			"IHR PROFIL IST ZU 100% VOLLSTAENDIG",
			"IHR PROFIL IST ZU 60% VOLLSTAENDIG",
			"Mitglieder mit vollstaendigem Profil erhalten 3x mehr "
			"Antworten!!",
			"Kompatibilitaetsinstrument in Eigenentwicklung von Speck T. "
			"Kline. Aehnlichkeiten mit dem Fragebogen des Institute of "
			"Mercenary Profiling sind zufaellig und bislang unverklagt.",
			"Zuerst: der Fragebogen muss es wissen. Sie sind...",
			"MANN", "FRAU", "FRAGE {} VON {}",
			"KEIN FOTO", "Ohne Fragebogen kein Wischen - Speck",
			"Was zuerst auffaellt: {}", "Sucht: {}",
			"Selbstbeschreibung:",
			"AUSSCHLUSSKRITERIEN: {}", "Von {} Mitglied(ern) blockiert",
			"Sie antworteten beide:", "Sie unterscheiden sich bei:",
			"Sie stimmen bei {} von {} ueberein.",
			"BLUMEN SENDEN", "< ZURUECK",
			"wo die Harten zaertlich werden - eine Speck T. Kline Firma",
			"Kein Profil, keine Romantik. Der ICH-Tab wartet. - Speck",
			"Kein Zwinkern mehr. GOLD geht nie aus. Nur so. - Speck",
			"Danke fuer GOLD. Sie vervollstaendigen mich. - Speck",
			"Karte abgelehnt. Passiert jedem. Mir nicht. - Speck",
			"Ihr M.E.R.C.-Konto ist ueberfaellig. Amor wartet. - Speck",
			"Meine groesste Wertschaetzung fuer Ihre Zahlung. - Speck",
			"SONNTAGS-SPEZIAL!! Doppeltes Zwinkern, laut Wissenschaft.",
			"Mitgliederschwund im Branchenrahmen. - Verwaltung",
			"werbung - mercsandkisses.com",
			"!!! HERZLICHEN GLUECKWUNSCH !!!",
			"Sie sind der 1.000.000ste Besucher dieser Seite!! Sie haben "
			"gewonnen: die Berechtigung, MERCS & KISSES GOLD zu kaufen.",
			"PREIS ABHOLEN",
			"AUSGEWAEHLTE PROFILE",
			"LIEBESPAAR DES MONATS", "demnaechst hier",
			"Ich suche: [ {} ] (klicken zum Aendern)",
			"MAENNER", "FRAUEN", "ALLE",
			"Aktiv in den letzten 24 Stunden",
			"Aktiv vor {} Tagen",
			"Aktiv vor sehr, sehr langer Zeit",
			"hat {} BLOCKIERT",
			"REFERENZEN: IM AUFBAU (seit Tag 1)",
			"in liebevoller Erinnerung - Profil bleibt bestehen",
		},
	};

	const char* T(CupidStr id) { return CUPID_TEXT[gfCupidGerman ? 1 : 0][id]; }

	// what Speck's footer ticker currently says
	int giCupidTicker = CPS_TICKER_DEFAULT;

	// the trait, spun charming, indexed by PersonalityTrait
	const char* const CUPID_TRAIT_SPIN[2][8] =
	{
		{ "professional", "a cool customer", "sensitive", "loves the outdoors",
		  "a landlubber at heart", "detail-oriented", "lives in the moment",
		  "SPONTANEOUS!!" },
		{ "professionell", "cool im Kopf", "einfuehlsam", "liebt das Freie",
		  "eine Landratte im Herzen", "detailverliebt", "lebt im Moment",
		  "SPONTAN!!" },
	};

	// every personals ad of the era ran under a headline; these come with
	// the temperament, indexed by Attitudes
	const char* const CUPID_HEADLINE[2][NUM_ATTITUDES] =
	{
		{ "\"Steady Hands, Steady Heart\"",
		  "\"Your New Best Friend (And Then Some??)\"",
		  "\"Not Looking. And Yet Here I Am.\"",
		  "\"The One Could Be Reading This RIGHT NOW!!\"",
		  "\"This Will Probably Not Work Out\"",
		  "\"No Games. Unless You Start One.\"",
		  "\"You Have Excellent Taste Already\"",
		  "\"You May Have Heard Of Me\"",
		  "\"Frankly, You Could Do Worse\"",
		  "\"Seeking Somewhere Quiet, Together\"" },
		{ "\"Ruhige Haende, ruhiges Herz\"",
		  "\"Ihr neuer bester Freund (und mehr??)\"",
		  "\"Suche nicht. Und doch bin ich hier.\"",
		  "\"Vielleicht liest DER RICHTIGE genau JETZT!!\"",
		  "\"Das wird vermutlich nichts\"",
		  "\"Keine Spielchen. Ausser Sie fangen an.\"",
		  "\"Sie haben bereits exzellenten Geschmack\"",
		  "\"Sie haben sicher von mir gehoert\"",
		  "\"Ehrlich: es gibt Schlimmeres\"",
		  "\"Suche einen ruhigen Ort, zu zweit\"" },
	};

	// what each attitude is "looking for", indexed by Attitudes
	const char* const CUPID_LOOKING[2][NUM_ATTITUDES] =
	{
		{ "short-term contract, long-term maybe", "new friends!!",
		  "someone who respects space", "The One :)", "not expecting much",
		  "someone who can keep up", "someone who deserves me", "an admirer",
		  "none of your business", "somewhere safe" },
		{ "kurzer Vertrag, langfristig vielleicht", "neue Freunde!!",
		  "jemanden, der Abstand respektiert", "die grosse Liebe :)",
		  "erwarte nicht viel", "jemanden, der mithalten kann",
		  "jemanden, der mich verdient", "einen Bewunderer",
		  "geht Sie nichts an", "einen sicheren Ort" },
	};

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

	// a rounded card with an outline and an embossed edge: lit along the
	// top, shaded along the foot, so the panel sits proud of the page
	void FillCard(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb, UINT32 edge,
				UINT32 bg)
	{
		FillRounded(x, y, w, h, edge, CP_RADIUS, bg);
		FillRounded(x + 1, y + 1, w - 2, h - 2, rgb, CP_RADIUS, edge);
		FillRect(x + CP_RADIUS, y + 1, w - 2 * CP_RADIUS, 1, CP_RGB_GLOSS);
		FillRect(x + CP_RADIUS, y + h - 2, w - 2 * CP_RADIUS, 1,
				CP_RGB_SHADOW);
	}

	// the round sibling: a gel disc for the verdict buttons
	void GelCircle(INT32 x, INT32 y, INT32 d, UINT32 base, UINT32 lite,
				UINT32 ring, UINT32 bg)
	{
		FillRounded(x, y, d, d, ring, d / 2, bg);
		FillRounded(x + 1, y + 1, d - 2, d - 2, base, (d - 2) / 2, ring);
		FillRounded(x + 3, y + 3, d - 6, (d - 6) / 2, lite, (d - 6) / 4,
				base);
		FillRect(x + 10, y + 3, d - 20, 1, CP_RGB_GLOSS);
	}

	// the aqua gel pill: saturated base, lighter lit top half, a one-pixel
	// gloss line, and a dark ring - 2001 called, it can have this back later
	void GelPill(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 base, UINT32 lite,
				UINT32 ring, UINT32 bg)
	{
		FillRounded(x, y, w, h, ring, h / 2 > 6 ? 6 : h / 2, bg);
		FillRounded(x + 1, y + 1, w - 2, h - 2, base,
				(h - 2) / 2 > 5 ? 5 : (h - 2) / 2, ring);
		// the lit upper half
		FillRounded(x + 2, y + 2, w - 4, (h - 4) / 2 + 1, lite, 4, base);
		// the gloss line
		FillRect(x + 6, y + 2, w - 12, 1, CP_RGB_GLOSS);
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

	// a blocky heart at 1x (7x6) or scaled up
	void DrawHeart(INT32 x, INT32 y, int s, UINT32 rgb)
	{
		FillRect(x,         y,         3 * s, 2 * s, rgb);
		FillRect(x + 4 * s, y,         3 * s, 2 * s, rgb);
		FillRect(x,         y + 2 * s, 7 * s, 2 * s, rgb);
		FillRect(x + 1 * s, y + 4 * s, 5 * s, 1 * s, rgb);
		FillRect(x + 2 * s, y + 5 * s, 3 * s, 1 * s, rgb);
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

	// wallpaper: alternating pale hearts tiled over the page, because a
	// dating site in 1999 would sooner die than show a blank background
	void RenderWallpaper()
	{
		int row = 0;
		for (INT32 y = CP_TOPBAR_H + 12; y < CP_PAGE_H - 24; y += 34, ++row)
		{
			for (INT32 x = 10 + (row % 2) * 17; x < CP_PAGE_W - 12; x += 34)
			{
				DrawHeart(x, y, 1,
					((x / 34) + row) % 2 ? CP_RGB_PINK_PALE
							     : CP_RGB_BLUE_WALL);
			}
		}
	}

	// the cheap depth every 1999 site faked with a table border
	void DropShadow(INT32 x, INT32 y, INT32 w, INT32 h)
	{
		FillRounded(x + 3, y + 3, w, h, CP_RGB_SHADOW, CP_RADIUS, CP_RGB_BG);
	}

	// the match percentage as a meter, which reads better than a number
	void DrawMeter(INT32 x, INT32 y, INT32 w, int pct, UINT32 fill)
	{
		FillRect(x - 1, y - 1, w + 2, 9, CP_RGB_INK);
		FillRect(x, y, w, 7, CP_RGB_CARD_DIM);
		FillRect(x, y, w * std::clamp(pct, 0, 100) / 100, 7, fill);
	}

	// the 1999 censor bar: chunky checkerboard over a face
	void BlurOver(INT32 x, INT32 y, INT32 w, INT32 h)
	{
		for (INT32 yy = 0; yy < h; yy += 3)
		{
			for (INT32 xx = ((yy / 3) % 2) * 3; xx < w; xx += 6)
			{
				FillRect(x + xx, y + yy, 3, 3, CP_RGB_BLUE_DK);
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
		return percent >= 75 ? FONT_DKGREEN
		     : percent >= 50 ? FONT_DKYELLOW : FONT_DKRED;
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
		return int(roll) < m.percent;
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

		// Speck deals himself in every few cards
		int adCounter = 0;
		const CardKind ads[3] = { CARD_AD_GOLD, CARD_AD_TESTIMONIAL,
					CARD_AD_NEWMEMBERS };
		for (size_t i = 0; i < pool.size(); ++i)
		{
			if (i > 0 && i % 5 == 0)
			{
				CardKind ad = ads[adCounter++ % 3];
				if (ad == CARD_AD_GOLD && IsGold()) ad = CARD_AD_TESTIMONIAL;
				if (ad == CARD_AD_NEWMEMBERS && !AnyLockedMembers())
				{
					ad = CARD_AD_TESTIMONIAL;
				}
				gCupidDeck.push_back({ ad, 0 });
			}
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
		AddEmailWithSpecialData(CUPID_EMAIL_WELCOME, 0, CUPID_SPECK_SENDER,
					GetWorldTotalMin(), 0, 0);
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
		giCupidPhotoReveal = 6; // the next photo arrives at 28.8kbps
		if (gCupidDeckPos < int(gCupidDeck.size()) - 1) ++gCupidDeckPos;
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
			if (!IsGold() && gCupidPersist.ubLikesLeft > 0)
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

	// --- regions ------------------------------------------------------------
	void SideAdCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK) return;
		// the banners are real 1999 banners: they go somewhere
		if (MSYS_GetRegionUserData(region, 0) == 0)
		{
			GoToWebPage(MAHJONG_BOOKMARK);
		}
		else
		{
			GoToWebPage(FLORIST_BOOKMARK);
		}
	}

	void RemoveAnswerRegions();

	void TabCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage == CPP_SPLASH) return; // the splash has its own exits
		switch (MSYS_GetRegionUserData(region, 0))
		{
			case 0: gCupidPage = CPP_DECK;    break;
			case 1: gCupidPage = CPP_MATCHES; break;
			case 2: gCupidPage = CPP_ME;      break;
		}
		// leaving the questionnaire abandons it; its hit regions go too
		gfCupidQuizLive = false;
		giCupidQuizQ = -1;
		RemoveAnswerRegions();
		giCupidCardDx = 0;
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
		const Card& card = CurrentCard();

		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (card.kind != CARD_MEMBER) return;

		// the card is a link: click it and the dossier opens
		gCupidDetailPid  = card.pid;
		gCupidDetailFrom = CPP_DECK;
		gCupidPage = CPP_DETAIL;
		CupidRedraw();
	}

	void PassCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK || !PlayerHasProfile()) return;
		StartFly(-1);
	}

	void LikeCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK || !PlayerHasProfile()) return;
		StartFly(1);
	}

	void LangCallback(MOUSE_REGION* region, UINT32 reason);

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
			const INT16 qh = INT16(IanWrappedStringHeight(310, 2, FONT10ARIAL,
						QuizQuestion(giCupidQuizQ)));
			gsCupidQuizTop = INT16(60 + qh + 8);
			giCupidAnsCount = DatingGame::ANSWER_COUNT[giCupidQuizQ];
			INT16 y = gsCupidQuizTop;
			for (int i = 0; i < giCupidAnsCount; ++i)
			{
				const INT16 th = INT16(IanWrappedStringHeight(266, 2,
							FONT10ARIAL, QuizAnswer(giCupidQuizQ, i)));
				gsCupidAnsY[i] = y;
				gsCupidAnsH[i] = INT16(std::max<INT16>(26, th + 10));
				y += gsCupidAnsH[i] + 5;
			}
		}

		for (int i = 0; i < giCupidAnsCount; ++i)
		{
			MSYS_DefineRegion(&gCupidAnswerRegion[i],
					UINT16(CP_X(96)), UINT16(CP_Y(gsCupidAnsY[i])),
					UINT16(CP_X(96 + 310)),
					UINT16(CP_Y(gsCupidAnsY[i] + gsCupidAnsH[i])),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					AnswerCallback);
			MSYS_SetRegionUserData(&gCupidAnswerRegion[i], 0, i);
		}
		gfCupidAnswerRegionsLive = true;
	}

	void LangCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		gfCupidGerman = !gfCupidGerman;
		// the questionnaire is bilingual and the row heights follow the text
		if (gfCupidQuizLive) LayoutQuiz();
		CupidRedraw();
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
		gCupidDetailPid  = matches[size_t(row)];
		gCupidDetailFrom = CPP_MATCHES;
		gCupidPage = CPP_DETAIL;
		CupidRedraw();
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
			}
			else
			{
				gCupidDetailPid  = gCupidSplashPid;
				gCupidDetailFrom = CPP_MATCHES;
				gCupidPage = CPP_DETAIL;
			}
			CupidRedraw();
			return;
		}

		if (gCupidPage == CPP_DETAIL)
		{
			if (action == CB_ACTION_PRIMARY)
			{
				// the whole flower economy is one click away
				GoToWebPage(FLORIST_BOOKMARK);
			}
			else
			{
				gCupidPage = gCupidDetailFrom;
			}
			CupidRedraw();
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

	void FaceStripCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (gCupidPage != CPP_DECK) return;
		const int slot = int(MSYS_GetRegionUserData(region, 0));
		const ProfileID pid = gCupidFacePids[slot];
		if (pid == 0xFF || RosterIndexOf(pid) < 0) return; // empty slot
		gCupidDetailPid  = pid;
		gCupidDetailFrom = CPP_DECK;
		gCupidPage = CPP_DETAIL;
		CupidRedraw();
	}

	void SeekCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		giCupidSeek = (giCupidSeek + 1) % 3;
		BuildDeck(); // the pool follows the preference
		CupidRedraw();
	}

	void RingCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		// the ring's three stops: prev, random, next. Random is the funeral
		// home, which is exactly the kind of luck 1999 dealt.
		switch (MSYS_GetRegionUserData(region, 0))
		{
			case 0: GoToWebPage(CHESS_BOOKMARK);   break;
			case 1: GoToWebPage(FUNERAL_BOOKMARK); break;
			default: GoToWebPage(MAHJONG_BOOKMARK); break;
		}
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

		for (int i = 0; i < 3; ++i)
		{
			const INT32 x = 168 + i * 80;
			MSYS_DefineRegion(&gCupidTabRegion[i],
					UINT16(CP_X(x)), UINT16(CP_Y(2)),
					UINT16(CP_X(x + 74)), UINT16(CP_Y(CP_TOPBAR_H - 2)),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					TabCallback);
			MSYS_SetRegionUserData(&gCupidTabRegion[i], 0, i);
		}

		MSYS_DefineRegion(&gCupidCardRegion,
				UINT16(CP_X(CP_CARD_X)), UINT16(CP_Y(CP_CARD_Y)),
				UINT16(CP_X(CP_CARD_X + CP_CARD_W)),
				UINT16(CP_Y(CP_CARD_Y + CP_CARD_H)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				CardCallback);

		MSYS_DefineRegion(&gCupidPassRegion,
				UINT16(CP_X(CP_BTN_PASS_X)), UINT16(CP_Y(CP_BTN_Y)),
				UINT16(CP_X(CP_BTN_PASS_X + CP_BTN_SIZE)),
				UINT16(CP_Y(CP_BTN_Y + CP_BTN_SIZE)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				PassCallback);
		MSYS_DefineRegion(&gCupidLikeRegion,
				UINT16(CP_X(CP_BTN_LIKE_X)), UINT16(CP_Y(CP_BTN_Y)),
				UINT16(CP_X(CP_BTN_LIKE_X + CP_BTN_SIZE)),
				UINT16(CP_Y(CP_BTN_Y + CP_BTN_SIZE)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				LikeCallback);

		MSYS_DefineRegion(&gCupidLangRegion,
				UINT16(CP_X(CP_PAGE_W - 46)), UINT16(CP_Y(4)),
				UINT16(CP_X(CP_PAGE_W - 4)), UINT16(CP_Y(CP_TOPBAR_H - 4)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				LangCallback);
		gCupidLangRegion.SetFastHelpText("English / Deutsch");

		for (int i = 0; i < 2; ++i)
		{
			const INT32 y = CP_PAGE_H - 84 + i * 32;
			MSYS_DefineRegion(&gCupidActionRegion[i],
					UINT16(CP_X(96)), UINT16(CP_Y(y)),
					UINT16(CP_X(96 + 310)), UINT16(CP_Y(y + 26)),
					MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
					ActionCallback);
			MSYS_SetRegionUserData(&gCupidActionRegion[i], 0, i);
		}

		for (int i = 0; i < 2; ++i)
		{
			const INT32 x = i == 0 ? CP_LCOL_X : CP_RCOL_X;
			MSYS_DefineRegion(&gCupidSideAdRegion[i],
					UINT16(CP_X(x)), UINT16(CP_Y(44)),
					UINT16(CP_X(x + CP_COL_W)), UINT16(CP_Y(44 + 178)),
					MSYS_PRIORITY_HIGH - 2, CURSOR_WWW, MSYS_NO_CALLBACK,
					SideAdCallback);
			MSYS_SetRegionUserData(&gCupidSideAdRegion[i], 0, i);
		}

		for (int i = 0; i < 7; ++i)
		{
			const INT32 y = 118 + i * 51;
			MSYS_DefineRegion(&gCupidMatchRegion[i],
					UINT16(CP_X(96)), UINT16(CP_Y(y)),
					UINT16(CP_X(96 + 310)), UINT16(CP_Y(y + 49)),
					MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
					MatchRowCallback);
			MSYS_SetRegionUserData(&gCupidMatchRegion[i], 0, i);
		}

		// the sidebar face strips: four online on the left, four hot on
		// the right, each mugshot a door into a dossier
		for (int i = 0; i < 8; ++i)
		{
			const bool left = i < 4;
			const int  cell = left ? i : i - 4;
			const INT32 bx = left ? CP_LCOL_X : CP_RCOL_X;
			const INT32 x  = bx + 6 + (cell % 2) * (CP_FACE_SM_W + 4);
			const INT32 y  = 246 + (cell / 2) * (CP_FACE_SM_H + 6);
			MSYS_DefineRegion(&gCupidFaceRegion[i],
					UINT16(CP_X(x)), UINT16(CP_Y(y)),
					UINT16(CP_X(x + CP_FACE_SM_W)),
					UINT16(CP_Y(y + CP_FACE_SM_H)),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					FaceStripCallback);
			MSYS_SetRegionUserData(&gCupidFaceRegion[i], 0, i);
		}

		MSYS_DefineRegion(&gCupidSeekRegion,
				UINT16(CP_X(96)), UINT16(CP_Y(132)),
				UINT16(CP_X(96 + 310)), UINT16(CP_Y(148)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				SeekCallback);

		// the webring, living in the footer's left corner
		for (int i = 0; i < 3; ++i)
		{
			const INT32 x = 96 + i * 16;
			MSYS_DefineRegion(&gCupidRingRegion[i],
					UINT16(CP_X(x)), UINT16(CP_Y(CP_PAGE_H - 14)),
					UINT16(CP_X(x + 15)), UINT16(CP_Y(CP_PAGE_H)),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					RingCallback);
			MSYS_SetRegionUserData(&gCupidRingRegion[i], 0, i);
		}

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
		MSYS_RemoveRegion(&gCupidLangRegion);
		for (MOUSE_REGION& r : gCupidRingRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gCupidSeekRegion);
		for (MOUSE_REGION& r : gCupidFaceRegion) MSYS_RemoveRegion(&r);
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

	void RenderTopBar()
	{
		// brushed aqua: pinstriped blue with a lit crown
		FillRect(0, 0, CP_PAGE_W, CP_TOPBAR_H, CP_RGB_BLUE);
		for (INT32 y = 3; y < CP_TOPBAR_H; y += 3)
		{
			FillRect(0, y, CP_PAGE_W, 1, FROMRGB(156, 194, 224));
		}
		FillRect(0, 0, CP_PAGE_W, 2, CP_RGB_BLUE_LITE);
		FillRect(0, CP_TOPBAR_H, CP_PAGE_W, 1, CP_RGB_BLUE_DK);
		FillRect(0, CP_TOPBAR_H + 1, CP_PAGE_W, 1, CP_RGB_GLOSS);

		if (guiCupidLogo)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidLogo, 1, CP_X(6), CP_Y(5));
		}
		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, 24, 7, "MERCS");
		DrawHeart(60, 8, 1, CP_RGB_PINK);
		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, 70, 7, "KISSES");
		if (IsGold())
		{
			// the prestige beyond measure, measured: one small chip
			FillRounded(118, 5, 40, 14, CP_RGB_GOLD, 3, CP_RGB_BLUE);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, 138, 8, "GOLD");
		}

		static const CupidStr tabs[3] =
			{ CPS_TAB_DECK, CPS_TAB_MATCHES, CPS_TAB_ME };
		for (int i = 0; i < 3; ++i)
		{
			const INT32 x = 168 + i * 80;
			const bool active =
				(i == 0 && (gCupidPage == CPP_DECK ||
					(gCupidPage == CPP_DETAIL && gCupidDetailFrom == CPP_DECK))) ||
				(i == 1 && (gCupidPage == CPP_MATCHES || gCupidPage == CPP_SPLASH ||
					(gCupidPage == CPP_DETAIL && gCupidDetailFrom == CPP_MATCHES))) ||
				(i == 2 && gCupidPage == CPP_ME);
			if (active)
			{
				GelPill(x, 3, 74, CP_TOPBAR_H - 6, CP_RGB_PINK,
						CP_RGB_PINK_LITE, CP_RGB_PINK_DK, CP_RGB_BLUE);
			}
			else
			{
				const bool hov = Hover(gCupidTabRegion[i]);
				GelPill(x, 3, 74, CP_TOPBAR_H - 6,
						hov ? CP_RGB_CARD : CP_RGB_BLUE_PALE,
						hov ? CP_RGB_GLOSS : CP_RGB_BLUE_LITE,
						CP_RGB_BLUE_DK, CP_RGB_BLUE);
			}
			ST::string label = T(tabs[i]);
			if (i == 1)
			{
				const int n = int(AllMatches().size());
				if (n > 0) label = ST::format("{} ({})", label, n);
			}
			PrintCentred(FONT10ARIALBOLD,
					active ? FONT_MCOLOR_WHITE : FONT_NEARBLACK,
					x + 37, 8, label);
		}

		PrintAt(FONT10ARIAL, gfCupidGerman ? FONT_GRAY5 : FONT_NEARBLACK,
				CP_PAGE_W - 42, 7, "EN");
		PrintAt(FONT10ARIAL, FONT_GRAY5, CP_PAGE_W - 28, 7, "|");
		PrintAt(FONT10ARIAL, gfCupidGerman ? FONT_NEARBLACK : FONT_GRAY5,
				CP_PAGE_W - 22, 7, "DE");
	}

	void RenderFooter()
	{
		FillRect(0, CP_PAGE_H - 14, CP_PAGE_W, 14, CP_RGB_BLUE);
		FillRect(0, CP_PAGE_H - 14, CP_PAGE_W, 1, CP_RGB_BLUE_DK);
		FillRect(0, CP_PAGE_H - 13, CP_PAGE_W, 1, CP_RGB_BLUE_LITE);

		// the webring: sideways discovery, as the era intended
		PrintAt(FONT10ARIAL, FONT_GRAY3, 8, CP_PAGE_H - 12, "ARULCO-NET");
		static const char* const glyphs[3] = { "<<", "?", ">>" };
		for (int i = 0; i < 3; ++i)
		{
			PrintAt(FONT10ARIALBOLD,
					Hover(gCupidRingRegion[i]) ? FONT_DKRED : FONT_NEARBLACK,
					98 + i * 16, CP_PAGE_H - 12, glyphs[i]);
		}

		// the site survived the millennium bug in advance
		PrintAt(FONT10ARIALBOLD, FONT_DKYELLOW, CP_PAGE_W - 58,
				CP_PAGE_H - 12, "Y2K OK");

		// the ticker: events speak first, then the daily rotation
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
		// centred in the band between the ring and the badge
		PrintCentred(FONT10ARIAL, FONT_NEARBLACK, (160 + CP_PAGE_W - 64) / 2,
				CP_PAGE_H - 12, T(CupidStr(say)));
	}

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

	// a small outlined pill, the interest chip of the era to come
	INT32 DrawChip(INT32 x, INT32 y, const ST::string& text, UINT32 edge)
	{
		const INT32 w = StringPixLength(text, FONT10ARIAL) + 12;
		FillRounded(x, y, w, 15, edge, 5, CP_RGB_CARD);
		FillRounded(x + 1, y + 1, w - 2, 13, CP_RGB_CARD, 5, edge);
		PrintCentred(FONT10ARIAL, FONT_NEARBLACK, x + w / 2, y + 3, text);
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

	void RenderMemberCard(const Card& card, INT32 cardX)
	{
		MERCPROFILESTRUCT const& p = GetProfile(card.pid);
		const int idx = RosterIndexOf(card.pid);
		const bool merc = idx >= 0 && gCupidRoster[size_t(idx)].merc;

		FillCard(cardX, CP_CARD_Y, CP_CARD_W, CP_CARD_H, CP_RGB_CARD,
				CP_RGB_BLUE_DK, CP_RGB_BG);

		// the photo frame: dark, snug around the picture
		const INT32 zoneY = CP_CARD_Y + 4;
		const INT32 zoneH = CP_PHOTO_H + 8;
		FillRounded(cardX + 3, zoneY, CP_CARD_W - 6, zoneH, CP_RGB_INK,
				CP_RADIUS, CP_RGB_CARD);
		const INT32 photoX = cardX + (CP_CARD_W - CP_PHOTO_W) / 2;
		SGPVObject* big = BigFaceFor(card.pid);
		if (big)
		{
			BltVideoObject(FRAME_BUFFER, big, 0, CP_X(photoX),
					CP_Y(zoneY + 4));
			// the 28.8k curtain: everything below the download line is
			// still on its way, with an interlace fringe above it
			if (giCupidPhotoReveal < CP_PHOTO_H)
			{
				const INT32 edge = zoneY + 4 + giCupidPhotoReveal;
				FillRect(photoX, edge, CP_PHOTO_W,
						CP_PHOTO_H - giCupidPhotoReveal, CP_RGB_INK);
				for (INT32 fy = edge - 12; fy < edge; fy += 2)
				{
					if (fy >= zoneY + 4)
					{
						FillRect(photoX, fy, CP_PHOTO_W, 1, CP_RGB_INK);
					}
				}
			}
		}
		else
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY5, cardX + CP_CARD_W / 2,
					zoneY + 56, T(CPS_NO_PHOTO));
		}

		// the name holds the centre between the two verdict circles
		PrintCentred(FONT14ARIAL, FONT_NEARBLACK, cardX + CP_CARD_W / 2,
				CP_BTN_Y + 1, p.zNickname);
		if (PlayerHasProfile())
		{
			const DatingGame::Match match = MatchWith(card.pid);
			PrintCentred(FONT10ARIALBOLD, MatchColour(match.percent),
					cardX + CP_CARD_W / 2, CP_BTN_Y + 19,
					ST::format("{}% MATCH", match.percent));
		}
		INT32 y = CP_BTN_Y + CP_BTN_SIZE + 8;

		// the ad runs under a headline, as personals always did
		PrintCentred(FONT10ARIAL, FONT_GRAY4, cardX + CP_CARD_W / 2, y,
				CUPID_HEADLINE[gfCupidGerman ? 1 : 0]
					[p.bAttitude >= 0 && p.bAttitude < NUM_ATTITUDES
						? p.bAttitude : 0]);
		y += 13;

		if (PlayerHasProfile())
		{
			const DatingGame::Match match = MatchWith(card.pid);
			DrawMeter(cardX + (CP_CARD_W - 120) / 2, y, 120, match.percent,
					match.percent >= 75 ? CP_RGB_LIKE
					: match.percent >= 50 ? CP_RGB_GOLD : CP_RGB_NOPE);
			y += 16;
		}
		else
		{
			y += 5;
		}

		// the chips: trait, standing, verification - flowed, wrapped
		{
			ST::string chips[3];
			UINT32 edges[3];
			int n = 0;
			const int trait = p.bPersonalityTrait >= 0 &&
						p.bPersonalityTrait < 8
					? p.bPersonalityTrait : 0;
			chips[n] = CUPID_TRAIT_SPIN[gfCupidGerman ? 1 : 0][trait];
			edges[n++] = CP_RGB_PINK;
			const CupidStr status = MemberStatus(card.pid);
			chips[n] = status == CPS_STATUS_ONLINE ? "ONLINE"
					: (gfCupidGerman ? "WEG" : "AWAY");
			edges[n++] = status == CPS_STATUS_ONLINE
					? CP_RGB_LIKE : CP_RGB_GREY;
			chips[n] = merc ? T(CPS_UNVERIFIED)
					: (gfCupidGerman ? "GEPRUEFT" : "VERIFIED");
			edges[n++] = merc ? CP_RGB_GREY : CP_RGB_GOLD;

			INT32 cxp = cardX + 12;
			for (int i = 0; i < n; ++i)
			{
				const INT32 w =
					StringPixLength(chips[i], FONT10ARIAL) + 12;
				if (cxp + w > cardX + CP_CARD_W - 10)
				{
					cxp = cardX + 12;
					y += 18;
				}
				DrawChip(cxp, y, chips[i], edges[i]);
				cxp += w + 5;
			}
			y += 24;
		}

		// two lines of bio, quoted and clamped; one line of intent
		const char* flavor = FlavorFor(card.pid);
		const int att = p.bAttitude >= 0 && p.bAttitude < NUM_ATTITUDES
					? p.bAttitude : 0;
		const char* bio = flavor ? flavor
			: (merc ? CUPID_SUMMARY_MERC : CUPID_SUMMARY_AIM)
				[gfCupidGerman ? 1 : 0][att];
		y += DisplayWrappedString(UINT16(CP_X(cardX + 12)), UINT16(CP_Y(y)),
				CP_CARD_W - 24, 2, FONT10ARIAL, FONT_GRAY3,
				ST::format("\"{}\"",
					ClampLines(bio, CP_CARD_W - 30, 3)),
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 6;
		PrintAt(FONT10ARIAL, FONT_GRAY4, cardX + 12, y,
				ClampLines(ST::format(T(CPS_LOOKING_FOR),
						CUPID_LOOKING[gfCupidGerman ? 1 : 0][att]),
					CP_CARD_W - 24, 1));

		// one line at the foot: the warning outranks the vanity plate
		bool warned = false;
		for (int i = 0; i < 2; ++i)
		{
			const INT8 hated = p.bHated[i];
			if (hated < 0 || !IsMercOnTeam(UINT8(hated))) continue;
			PrintCentred(FONT10ARIAL, FONT_DKRED, cardX + CP_CARD_W / 2,
					CP_CARD_Y + CP_CARD_H - 16,
					ST::format(T(CPS_BLOCKED_WARN),
						GetProfile(ProfileID(hated)).zNickname));
			warned = true;
			break;
		}
		if (!warned)
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY5, cardX + CP_CARD_W / 2,
					CP_CARD_Y + CP_CARD_H - 16,
					ST::format(gfCupidGerman ? "Mitglied Nr. {}"
								 : "member no. {}",
							1000 + int(card.pid) * 7));
		}

		// the NEW!! tag: applied on unlock, removed never
		if (idx >= 0 && gCupidRoster[size_t(idx)].fresh &&
		    (GetJA2Clock() / 400) % 2 == 0)
		{
			FillRounded(cardX + CP_CARD_W - 46, zoneY + 4, 40, 14,
					CP_RGB_NOPE, 3, CP_RGB_INK);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
					cardX + CP_CARD_W - 26, zoneY + 7, "NEW!!");
		}
	}

	void RenderAdCard(const Card& card, INT32 cardX)
	{
		FillCard(cardX, CP_CARD_Y, CP_CARD_W, CP_CARD_H, CP_RGB_CARD,
				CP_RGB_GOLD, CP_RGB_BG);

		if (guiCupidLogo)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidLogo, 0,
					CP_X(cardX + (CP_CARD_W - 22) / 2), CP_Y(CP_CARD_Y + 14));
		}

		ST::string headline, copy;
		switch (card.kind)
		{
			case CARD_AD_GOLD:
				headline = T(CPS_AD_GOLD_HEAD);
				copy = IsGold() ? T(CPS_AD_GOLD_OWNED)
						: ST::format(T(CPS_AD_GOLD_BODY), CP_GOLD_PRICE);
				break;
			case CARD_AD_TESTIMONIAL:
				headline = T(CPS_AD_TESTI_HEAD);
				copy = ST::format("{}\n\n{}", T(CPS_AD_TESTI_BODY),
						T(CPS_AD_TESTI_BY));
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

		PrintCentred(FONT12ARIAL, FONT_NEARBLACK, cardX + CP_CARD_W / 2,
				CP_CARD_Y + 46, headline);
		DisplayWrappedString(UINT16(CP_X(cardX + 12)),
				UINT16(CP_Y(CP_CARD_Y + 72)), CP_CARD_W - 24, 2, FONT10ARIAL,
				FONT_GRAY3, copy, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

		if (card.kind == CARD_AD_GOLD && !IsGold())
		{
			GelPill(cardX + 20, CP_CARD_Y + CP_CARD_H - 66, CP_CARD_W - 40,
					24, CP_RGB_GOLD, CP_RGB_GOLD_LITE, FROMRGB(150, 112, 48),
					CP_RGB_CARD);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
					cardX + CP_CARD_W / 2, CP_CARD_Y + CP_CARD_H - 58,
					ST::format(T(CPS_AD_GOLD_BTN), CP_GOLD_PRICE));
		}
		if (card.kind != CARD_END)
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY5, cardX + CP_CARD_W / 2,
					CP_CARD_Y + CP_CARD_H - 30, T(CPS_AD_HINT));
		}
	}

	// The flanks: a compact banner up top, and under it the thing every
	// 1999 page really ran on - a wall of mugshots.
	void RenderSideAds()
	{
		// left banner: the Parlour, house green, a rival Kingpin property
		const INT32 lx = CP_LCOL_X, lc = CP_LCOL_X + CP_COL_W / 2;
		DropShadow(lx, 44, CP_COL_W, 178);
		FillCard(lx, 44, CP_COL_W, 178, FROMRGB(24, 82, 50),
				FROMRGB(16, 56, 34), CP_RGB_BG);
		PrintCentred(FONT10ARIAL, FONT_GRAY6, lx + CP_COL_W - 12, 48, "AD");
		PrintCentred(FONT12ARIAL, FONT_MCOLOR_WHITE, lc, 62, "TIRED");
		PrintCentred(FONT12ARIAL, FONT_MCOLOR_WHITE, lc, 78, "OF");
		PrintCentred(FONT12ARIAL, FONT_MCOLOR_WHITE, lc, 94, "LOVE?");
		for (int i = 0; i < 3; ++i)
		{
			FillRounded(lc - 25 + i * 18, 116, 14, 22, CP_RGB_CARD, 2,
					FROMRGB(24, 82, 50));
			FillRect(lc - 22 + i * 18, 121 + (i % 2) * 6, 8, 6,
					i == 1 ? CP_RGB_NOPE : FROMRGB(24, 82, 50));
		}
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, lc, 148, "SAN MONA");
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, lc, 160, "MAHJONG");
		FillRounded(lc - 30, 178, 60, 16, FROMRGB(120, 180, 140), 3,
				FROMRGB(24, 82, 50));
		PrintCentred(FONT10ARIALBOLD, FONT_NEARBLACK, lc, 182, "VISIT");

		// right banner: the florist, because the funnel has a next step
		const INT32 rx = CP_RCOL_X, rc = CP_RCOL_X + CP_COL_W / 2;
		DropShadow(rx, 44, CP_COL_W, 178);
		FillCard(rx, 44, CP_COL_W, 178, CP_RGB_PINK_PALE, CP_RGB_PINK,
				CP_RGB_BG);
		PrintCentred(FONT10ARIAL, FONT_GRAY5, rx + CP_COL_W - 12, 48, "AD");
		PrintCentred(FONT12ARIAL, FONT_DKRED, rc, 62, "SAY IT");
		PrintCentred(FONT12ARIAL, FONT_DKRED, rc, 78, "WITH");
		PrintCentred(FONT12ARIAL, FONT_DKRED, rc, 94, "FLOWERS");
		DrawHeart(rc - 11, 114, 3, CP_RGB_PINK);
		FillRect(rc - 2, 134, 3, 24, FROMRGB(88, 138, 74));
		FillRect(rc - 8, 142, 8, 3, FROMRGB(88, 138, 74));
		PrintCentred(FONT10ARIAL, FONT_NEARBLACK, rc, 162, "UNITED FLORAL");
		FillRounded(rc - 30, 178, 60, 16, CP_RGB_PINK, 3, CP_RGB_PINK_PALE);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, rc, 182, "ORDER");

		// pick the two casts: who is reachable right now, and who the
		// algorithm is loudest about (the latter honours your search)
		std::vector<int> online, hot;
		for (int i = 0; i < int(gCupidRoster.size()); ++i)
		{
			const Member& m = gCupidRoster[size_t(i)];
			if (m.locked || MemberIsDead(m.pid)) continue;
			if (MemberStatus(m.pid) == CPS_STATUS_ONLINE) online.push_back(i);
			if (!MemberIsMarried(m.pid) && SeekAllows(m.pid)) hot.push_back(i);
		}
		const int day = int(GetWorldDay());
		if (!online.empty())
		{
			std::rotate(online.begin(),
					online.begin() + (day * 3) % int(online.size()),
					online.end());
		}
		if (PlayerHasProfile())
		{
			std::sort(hot.begin(), hot.end(), [](int a, int b)
			{
				return MatchWith(gCupidRoster[size_t(a)].pid).percent >
				       MatchWith(gCupidRoster[size_t(b)].pid).percent;
			});
		}
		else if (!hot.empty())
		{
			std::rotate(hot.begin(), hot.begin() + (day * 5) % int(hot.size()),
					hot.end());
		}

		// the two face panels: 2x2 mugshots each
		struct FacePanel { INT32 x; const char* en; const char* de;
				const std::vector<int>* cast; bool dots; };
		const FacePanel panels[2] =
		{
			{ CP_LCOL_X, "ONLINE NOW", "JETZT ONLINE", &online, true  },
			{ CP_RCOL_X, "HOT!!",      "HEISS!!",      &hot,    false },
		};
		for (int pnl = 0; pnl < 2; ++pnl)
		{
			const FacePanel& P = panels[pnl];
			DropShadow(P.x, 228, CP_COL_W, 122);
			FillCard(P.x, 228, CP_COL_W, 122, CP_RGB_CARD, CP_RGB_BLUE_DK,
					CP_RGB_BG);
			const ST::string head = pnl
				? ST::string(gfCupidGerman ? P.de : P.en)
				: ST::format("{} ({})", gfCupidGerman ? P.de : P.en,
						int(online.size()));
			PrintCentred(FONT10ARIALBOLD, pnl ? FONT_DKRED : FONT_DKGREEN,
					P.x + CP_COL_W / 2, 232, head);
			for (int cell = 0; cell < 4; ++cell)
			{
				const int slot = pnl * 4 + cell;
				const INT32 x = P.x + 6 + (cell % 2) * (CP_FACE_SM_W + 4);
				const INT32 y = 246 + (cell / 2) * (CP_FACE_SM_H + 6);
				if (cell >= int(P.cast->size()))
				{
					gCupidFacePids[slot] = 0xFF;
					FillRect(x, y, CP_FACE_SM_W, CP_FACE_SM_H,
							CP_RGB_CARD_DIM);
					continue;
				}
				const Member& m =
					gCupidRoster[size_t((*P.cast)[size_t(cell)])];
				gCupidFacePids[slot] = m.pid;
				const bool hov = Hover(gCupidFaceRegion[slot]);
				FillRect(x - 2, y - 2, CP_FACE_SM_W + 4, CP_FACE_SM_H + 4,
						hov ? CP_RGB_PINK : CP_RGB_INK);
				SGPVObject* face = Face33For(m.pid);
				if (face)
				{
					BltVideoObject(FRAME_BUFFER, face, 0, CP_X(x), CP_Y(y));
				}
				if (P.dots)
				{
					// the buddy-list dot: green means reachable
					FillRect(x + CP_FACE_SM_W - 9, y + CP_FACE_SM_H - 9, 9, 9,
							CP_RGB_INK);
					FillRect(x + CP_FACE_SM_W - 8, y + CP_FACE_SM_H - 8, 7, 7,
							FROMRGB(88, 200, 80));
				}
				else
				{
					DrawHeart(x + CP_FACE_SM_W - 10, y + CP_FACE_SM_H - 9, 1,
							CP_RGB_PINK);
				}
			}
		}
	}

	void RenderDeck()
	{
		RenderSideAds();

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
			PrintCentred(FONT12ARIAL, FONT_NEARBLACK, cx, CP_CARD_Y + 30,
					"MERCS & KISSES");
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, CP_CARD_Y + 44,
					gfCupidGerman ? "Wo die Harten zaertlich werden"
						      : "Where the tough get tender");
			FillRect(CP_CARD_X + 18, CP_CARD_Y + 58, CP_CARD_W - 36, 1,
					CP_RGB_CARD_DIM);

			PrintCentred(FONT10ARIALBOLD, FONT_DKRED, cx, CP_CARD_Y + 64,
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
						FONT10ARIAL, FONT_GRAY3,
						CUPID_HEADLINE[gfCupidGerman ? 1 : 0]
							[fp.bAttitude >= 0 &&
							 fp.bAttitude < NUM_ATTITUDES
								? fp.bAttitude : 0],
						FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
				fy += CP_FACE_SM_H + 9;
				++shown;
			}

			// the lovers of the month: Flo's whole arc, eventually
			PrintCentred(FONT10ARIALBOLD, FONT_DKRED, cx, fy + 2,
					T(CPS_LOVERS));
			PrintCentred(FONT10ARIAL,
					gubFact[FACT_PC_MARRYING_DARYL_IS_FLO]
						? FONT_NEARBLACK : FONT_GRAY5,
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

		// the card underneath peeks out, so the deck reads as a deck
		const Card& card = CurrentCard();
		if (card.kind != CARD_END &&
		    gCupidDeckPos + 1 < int(gCupidDeck.size()))
		{
			DropShadow(CP_CARD_X + 5, CP_CARD_Y + 5, CP_CARD_W, CP_CARD_H);
			FillCard(CP_CARD_X + 5, CP_CARD_Y + 5, CP_CARD_W, CP_CARD_H,
					CP_RGB_CARD_DIM, CP_RGB_BLUE_DK, CP_RGB_BG);
		}

		const INT32 cardX = CP_CARD_X + giCupidCardDx;
		DropShadow(cardX, CP_CARD_Y, CP_CARD_W, CP_CARD_H);
		if (card.kind == CARD_MEMBER) RenderMemberCard(card, cardX);
		else                          RenderAdCard(card, cardX);
		RenderVerdictStamp(cardX);

		// the verdict circles: white gel discs with coloured rings,
		// warming under the pointer
		DropShadow(CP_BTN_PASS_X, CP_BTN_Y, CP_BTN_SIZE, CP_BTN_SIZE);
		GelCircle(CP_BTN_PASS_X, CP_BTN_Y, CP_BTN_SIZE,
				Hover(gCupidPassRegion) ? FROMRGB(248, 222, 216)
							: CP_RGB_CARD,
				CP_RGB_CARD_LITE, CP_RGB_NOPE, CP_RGB_CARD);
		DrawCross(CP_BTN_PASS_X + 10, CP_BTN_Y + 10, 14, 3, CP_RGB_NOPE);

		const bool likeLive = CanLike() && card.kind != CARD_END;
		DropShadow(CP_BTN_LIKE_X, CP_BTN_Y, CP_BTN_SIZE, CP_BTN_SIZE);
		GelCircle(CP_BTN_LIKE_X, CP_BTN_Y, CP_BTN_SIZE,
				likeLive && Hover(gCupidLikeRegion) ? CP_RGB_PINK_PALE
								    : CP_RGB_CARD,
				CP_RGB_CARD_LITE,
				likeLive ? CP_RGB_PINK : CP_RGB_GREY, CP_RGB_CARD);
		DrawHeart(CP_BTN_LIKE_X + 7, CP_BTN_Y + 9, 3,
				likeLive ? CP_RGB_PINK : CP_RGB_GREY);

		// the allowance, on its own line beneath the verdict
		const ST::string likes = IsGold() ? T(CPS_LIKES_GOLD)
			: CanLike()
				? ST::format(T(CPS_LIKES_LEFT), gCupidPersist.ubLikesLeft)
				: T(CPS_OUT_OF_LIKES);
		PrintCentred(FONT10ARIAL, IsGold() ? FONT_DKYELLOW
				: CanLike() ? FONT_GRAY4 : FONT_DKRED,
				251, CP_CARD_Y + CP_CARD_H + 4, likes);

		// the popup: a little window with a big claim and one honest pixel
		if (gfCupidPopupUp)
		{
			DropShadow(131, 120, 240, 116);
			FillRect(131, 120, 240, 116, CP_RGB_INK);
			FillRect(132, 121, 238, 114, CP_RGB_CARD);
			// title bar, with the era's most trustworthy filename
			FillRect(132, 121, 238, 14, CP_RGB_BLUE_DK);
			PrintAt(FONT10ARIAL, FONT_MCOLOR_WHITE, 137, 124,
					T(CPS_POPUP_TITLE));
			FillRect(131 + 240 - 16, 122, 13, 12,
					Hover(gCupidPopupXRegion) ? CP_RGB_NOPE
								  : CP_RGB_BLUE);
			PrintCentred(FONT10ARIALBOLD, FONT_NEARBLACK, 131 + 240 - 10,
					124, "X");

			PrintCentred(FONT10ARIALBOLD, FONT_DKRED, 251, 142,
					T(CPS_POPUP_HEAD));
			DisplayWrappedString(UINT16(CP_X(143)), UINT16(CP_Y(158)), 216, 2,
					FONT10ARIAL, FONT_NEARBLACK, T(CPS_POPUP_BODY),
					FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
			GelPill(161, 206, 180, 24, CP_RGB_GOLD, CP_RGB_GOLD_LITE,
					FROMRGB(150, 112, 48), CP_RGB_CARD);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, 251, 213,
					T(CPS_POPUP_CTA));
		}
	}

	void RenderMatches()
	{
		PrintCentred(FONT12ARIAL, FONT_NEARBLACK, CP_PAGE_W / 2, 32,
				T(CPS_MATCHES_TITLE));

		// the GOLD tease: they liked you, and you may pay to know who
		const std::vector<ProfileID> admirers = SecretAdmirers();
		if (!admirers.empty())
		{
			PrintAt(FONT10ARIALBOLD, FONT_DKRED, 96, 48,
					ST::format(T(IsGold() ? CPS_LIKED_YOU_GOLD : CPS_LIKED_YOU),
							int(admirers.size())));
			INT32 x = 96;
			for (ProfileID pid : admirers)
			{
				if (x + CP_FACE_SM_W > 406) break;
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
					PrintCentred(FONT10ARIAL, FONT_GRAY3,
							x + CP_FACE_SM_W / 2, 105,
							GetProfile(pid).zNickname);
				}
				x += CP_FACE_SM_W + 8;
			}
		}

		const std::vector<ProfileID> matches = AllMatches();
		if (matches.empty())
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY4, CP_PAGE_W / 2, 108,
					T(CPS_MATCHES_NONE));
			// who you could be matching with, decency-blurred
			INT32 fx = CP_PAGE_W / 2 - (3 * CP_FACE_SM_W + 2 * 8) / 2;
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
			DropShadow(146, 180, 210, 40);
			FillCard(146, 180, 210, 40, CP_RGB_CARD, CP_RGB_INK, CP_RGB_BG);
			for (INT32 sx = 150; sx < 348; sx += 16)
			{
				for (int t = 0; t < 8; ++t)
				{
					FillRect(sx + t, 184 + t, 8, 1, FROMRGB(224, 186, 60));
				}
			}
			PrintCentred(FONT10ARIAL, FONT_NEARBLACK, 251, 202,
					T(CPS_UNDER_CONSTRUCTION));
			return;
		}

		INT32 y = 118;
		int row = 0;
		for (ProfileID pid : matches)
		{
			if (row >= 5) break;
			DropShadow(96, y, 310, 49);
			FillCard(96, y, 310, 49, CP_RGB_CARD,
					Hover(gCupidMatchRegion[row]) ? CP_RGB_PINK
								      : CP_RGB_BLUE,
					CP_RGB_BG);
			SGPVObject* face = Face33For(pid);
			if (face)
			{
				BltVideoObject(FRAME_BUFFER, face, 0, CP_X(100), CP_Y(y + 3));
			}
			MERCPROFILESTRUCT const& p = GetProfile(pid);
			const bool dead = MemberIsDead(pid);
			PrintAt(FONT10ARIALBOLD, dead ? FONT_GRAY5 : FONT_NEARBLACK,
					156, y + 8, p.zNickname);
			const DatingGame::Match match = MatchWith(pid);
			PrintAt(FONT10ARIAL, dead ? FONT_GRAY5 : MatchColour(match.percent),
					156, y + 24, MatchLabel(match));
			PrintAt(FONT10ARIAL, FONT_GRAY5, 258, y + 24,
					T(dead ? CPS_CONDOLENCE_ROW : MemberStatus(pid)));
			DrawHeart(388, y + 18, 1, dead ? CP_RGB_GREY : CP_RGB_PINK);
			y += 51;
			++row;
		}
		PrintCentred(FONT10ARIAL, FONT_GRAY5, CP_PAGE_W / 2, y + 4,
				T(CPS_MATCHES_HINT));
	}

	// the action rows: two full-width pills stacked over the footer, wide
	// enough for every label in both languages
	void RenderWideButton(INT32 slot, const ST::string& label, bool live)
	{
		const INT32 y = CP_PAGE_H - 84 + slot * 32;
		DropShadow(96, y, 310, 26);
		if (live)
		{
			const bool hov = Hover(gCupidActionRegion[slot]);
			GelPill(96, y, 310, 26,
					hov ? CP_RGB_PINK_LITE : CP_RGB_PINK,
					hov ? CP_RGB_PINK_PALE : CP_RGB_PINK_LITE,
					CP_RGB_PINK_DK, CP_RGB_BG);
		}
		else
		{
			GelPill(96, y, 310, 26, CP_RGB_CARD_DIM, CP_RGB_BLUE_LITE,
					CP_RGB_GREY, CP_RGB_BG);
		}
		PrintCentred(FONT10ARIALBOLD, live ? FONT_MCOLOR_WHITE : FONT_GRAY5,
				251, y + 9, label);
	}

	void RenderMeLanding()
	{
		const bool member = PlayerHasProfile() && HaveStoredAnswers();
		PrintCentred(FONT12ARIAL, FONT_NEARBLACK, CP_PAGE_W / 2, 32,
				T(member ? CPS_ME_TITLE_MINE : CPS_ME_TITLE));

		// the status row: your photo beside the truth about your profile
		DropShadow(96, 52, 310, 76);
		FillCard(96, 52, 310, 76, CP_RGB_CARD, CP_RGB_BLUE_DK, CP_RGB_BG);
		FillRect(104, 58, CP_FACE_SM_W + 6, CP_FACE_SM_H + 6, CP_RGB_INK);
		FillRect(105, 59, CP_FACE_SM_W + 4, CP_FACE_SM_H + 4, CP_RGB_MAT);
		if (guiCupidSelf)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidSelf, 0, CP_X(107), CP_Y(61));
		}
		else
		{
			FillRect(107, 61, CP_FACE_SM_W, CP_FACE_SM_H, CP_RGB_CARD_DIM);
			PrintAt(FONT10ARIAL, FONT_GRAY5, 112, 76, T(CPS_NO_PHOTO));
		}

		const bool full = PlayerHasProfile() && HaveStoredAnswers();
		const INT32 tx = 166;
		PrintAt(FONT10ARIALBOLD, full ? FONT_DKGREEN : FONT_DKRED, tx, 60,
				T(full ? CPS_ME_COMPLETE : CPS_ME_PARTIAL));
		DrawMeter(tx, 76, 240, full ? 100 : 60,
				full ? CP_RGB_LIKE : CP_RGB_PINK);
		if (!full)
		{
			DisplayWrappedString(UINT16(CP_X(tx)), UINT16(CP_Y(90)), 246, 2,
					FONT10ARIAL, FONT_GRAY4, T(CPS_ME_HINT),
					FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		}
		else
		{
			PrintAt(FONT10ARIAL, FONT_GRAY4, tx, 90,
					gfCupidGerman ? "Das Deck wartet auf Sie."
						      : "The deck is waiting for you.");
		}

		// the era's most important dropdown, functioning
		PrintCentred(FONT10ARIAL,
				Hover(gCupidSeekRegion) ? FONT_DKRED : FONT_GRAY3, 251, 134,
				ST::format(T(CPS_SEEK_LINE),
					T(giCupidSeek == SEEK_MEN ? CPS_SEEK_MEN
					: giCupidSeek == SEEK_WOMEN ? CPS_SEEK_WOMEN
					: CPS_SEEK_ALL)));

		if (member)
		{
			// the account view: your own ad, as the deck deals it
			PrintCentred(FONT10ARIALBOLD, FONT_DKRED, 251, 158,
					T(CPS_ME_PREVIEW));
			const DatingGame::Profile self = BuildPlayerProfile();
			MERCPROFILESTRUCT const& imp = GetProfile(PlayerImpPid());
			DropShadow(126, 172, 250, 96);
			FillCard(126, 172, 250, 96, CP_RGB_CARD, CP_RGB_PINK, CP_RGB_BG);
			FillRect(135, 181, CP_FACE_SM_W + 4, CP_FACE_SM_H + 4,
					CP_RGB_INK);
			FillRect(136, 182, CP_FACE_SM_W + 2, CP_FACE_SM_H + 2,
					CP_RGB_MAT);
			if (guiCupidSelf)
			{
				BltVideoObject(FRAME_BUFFER, guiCupidSelf, 0, CP_X(137),
						CP_Y(183));
			}
			const INT32 px = 194;
			PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, px, 182, imp.zNickname);
			PrintAt(FONT10ARIAL, FONT_GRAY4, px, 196,
					CUPID_HEADLINE[gfCupidGerman ? 1 : 0]
						[self.attitude >= 0 &&
						 self.attitude < NUM_ATTITUDES
							? self.attitude : 0]);
			INT32 cxp = px;
			cxp += DrawChip(cxp, 212,
					CUPID_TRAIT_SPIN[gfCupidGerman ? 1 : 0]
						[self.trait >= 0 && self.trait < 8
							? self.trait : 0],
					CP_RGB_PINK) + 5;
			DrawChip(cxp, 212, gfCupidGerman ? "I.M.P.-GEPRUEFT"
							 : "I.M.P. VERIFIED",
					CP_RGB_GOLD);
			PrintAt(FONT10ARIAL, FONT_GRAY4, 136, 240,
					ClampLines(ST::format(T(CPS_LOOKING_FOR),
							CUPID_LOOKING[gfCupidGerman ? 1 : 0]
								[self.attitude >= 0 &&
								 self.attitude < NUM_ATTITUDES
									? self.attitude : 0]),
						230, 1));

			// the vitals, and the one line of legal the lawyers insisted on
			PrintCentred(FONT10ARIAL, FONT_GRAY4, 251, 282,
					ST::format(T(CPS_ME_STATS), gCupidPersist.ubStreak,
						IsGold() ? 99 : gCupidPersist.ubLikesLeft));
			PrintCentred(FONT10ARIAL, FONT_GRAY5, 251, 298,
					gfCupidGerman ? "Instrument: Speck-o-metrisch(tm), "
							"unverklagt"
						      : "instrument: Speck-o-metric(tm), "
							"unlitigated");
		}
		else
		{
		// POWERED BY I.M.P., 88x31 in spirit, with the fine print below
		DropShadow(176, 148, 150, 38);
		FillCard(176, 148, 150, 38, CP_RGB_BLUE_PALE, CP_RGB_BLUE_DK,
				CP_RGB_BG);
		PrintCentred(FONT10ARIALBOLD, FONT_NEARBLACK, 251, 155, "POWERED BY");
		PrintCentred(FONT10ARIALBOLD, FONT_NEARBLACK, 251, 168, "I.M.P.");
		DisplayWrappedString(UINT16(CP_X(96)), UINT16(CP_Y(198)), 310, 2,
				FONT10ARIAL, FONT_GRAY5, T(CPS_ME_POWERED),
				FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

		// the testimonial, delivered in person by the site's one success
		DropShadow(96, 240, 310, 60);
		FillCard(96, 240, 310, 60, CP_RGB_CARD, CP_RGB_PINK, CP_RGB_BG);
		FillRect(103, 247, CP_FACE_SM_W + 4, CP_FACE_SM_H + 4, CP_RGB_INK);
		FillRect(104, 248, CP_FACE_SM_W + 2, CP_FACE_SM_H + 2, CP_RGB_MAT);
		SGPVObject* flo = Face33For(FLO);
		if (flo)
		{
			BltVideoObject(FRAME_BUFFER, flo, 0, CP_X(105), CP_Y(249));
		}
		DisplayWrappedString(UINT16(CP_X(162)), UINT16(CP_Y(248)), 236, 2,
				FONT10ARIAL, FONT_NEARBLACK,
				ST::format("{} {}", T(CPS_AD_TESTI_HEAD), T(CPS_AD_TESTI_BODY)),
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
		PrintAt(FONT10ARIAL, FONT_GRAY5, 162, 286, T(CPS_AD_TESTI_BY));
		}

		const bool imp     = LaptopSaveInfo.fIMPCompletedFlag;
		const bool banked  = HaveStoredAnswers();
		const bool profile = PlayerHasProfile();
		if (!member)
		{
			RenderWideButton(0, T(CPS_ME_IMPORT), imp && banked && !profile);
		}

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
			DisplayWrappedString(UINT16(CP_X(96)), UINT16(CP_Y(46)), 310, 2,
					FONT12ARIAL, FONT_NEARBLACK, T(CPS_QUIZ_SEX),
					FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
			for (int i = 0; i < giCupidAnsCount; ++i)
			{
				const INT32 y = gsCupidAnsY[i];
				DropShadow(96, y, 310, gsCupidAnsH[i]);
				GelPill(96, y, 310, gsCupidAnsH[i], CP_RGB_CARD,
						CP_RGB_CARD_LITE, CP_RGB_BLUE_DK, CP_RGB_BG);
				PrintCentred(FONT10ARIALBOLD, FONT_NEARBLACK, 251, y + 9,
						T(i ? CPS_QUIZ_FEMALE : CPS_QUIZ_MALE));
			}
			return;
		}

		PrintCentred(FONT10ARIAL, FONT_GRAY5, CP_PAGE_W / 2, 30,
				ST::format(T(CPS_QUIZ_PROGRESS), giCupidQuizQ + 1,
						DatingGame::NUM_QUESTIONS));
		DrawMeter(201, 44, 100,
				(giCupidQuizQ + 1) * 100 / DatingGame::NUM_QUESTIONS,
				CP_RGB_PINK);
		DisplayWrappedString(UINT16(CP_X(96)), UINT16(CP_Y(60)), 310, 2,
				FONT10ARIAL, FONT_NEARBLACK, QuizQuestion(giCupidQuizQ),
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

		// the rows are cut to their text by LayoutQuiz()
		for (int i = 0; i < giCupidAnsCount; ++i)
		{
			const INT32 y = gsCupidAnsY[i];
			const INT32 h = gsCupidAnsH[i];
			DropShadow(96, y, 310, h);
			const bool hov = Hover(gCupidAnswerRegion[i]);
			GelPill(96, y, 310, h,
					hov ? CP_RGB_BLUE_LITE : CP_RGB_CARD, CP_RGB_CARD_LITE,
					hov ? CP_RGB_PINK : CP_RGB_BLUE_DK, CP_RGB_BG);
			GelPill(102, y + (h - 18) / 2, 18, 18, CP_RGB_PINK,
					CP_RGB_PINK_LITE, CP_RGB_PINK_DK, CP_RGB_CARD);
			const char letter[2] = { char('A' + i), '\0' };
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, 111,
					y + (h - 18) / 2 + 5, letter);
			DisplayWrappedString(UINT16(CP_X(128)), UINT16(CP_Y(y + 6)), 266,
					2, FONT10ARIAL, FONT_NEARBLACK,
					QuizAnswer(giCupidQuizQ, i), FONT_MCOLOR_BLACK,
					LEFT_JUSTIFIED);
		}
	}

	void RenderMe()
	{
		if (gfCupidQuizLive) RenderQuizQuestion();
		else                 RenderMeLanding();
	}

	void RenderDetail()
	{
		const ProfileID pid = gCupidDetailPid;
		MERCPROFILESTRUCT const& p = GetProfile(pid);
		const int idx = RosterIndexOf(pid);
		const bool merc = idx >= 0 && gCupidRoster[size_t(idx)].merc;

		DropShadow(76, 30, 350, 276);
		FillCard(76, 30, 350, 276, CP_RGB_CARD, CP_RGB_BLUE_DK, CP_RGB_BG);

		SGPVObject* big = BigFaceFor(pid);
		if (big)
		{
			BltVideoObject(FRAME_BUFFER, big, 0, CP_X(90), CP_Y(42));
		}

		const INT32 tx = 90 + CP_PHOTO_W + 12;
		INT32 y = 44;
		PrintAt(FONT14ARIAL, FONT_NEARBLACK, tx, y, p.zNickname);
		y += 17;
		// the surname stays private; the headline does the talking
		PrintAt(FONT10ARIAL, FONT_GRAY4, tx, y,
				CUPID_HEADLINE[gfCupidGerman ? 1 : 0]
					[p.bAttitude >= 0 && p.bAttitude < NUM_ATTITUDES
						? p.bAttitude : 0]);
		y += 13;
		if (merc)
		{
			PrintAt(FONT10ARIAL, FONT_GRAY5, tx, y, T(CPS_UNVERIFIED));
		}
		else
		{
			if (guiCupidIcons)
			{
				BltVideoObject(FRAME_BUFFER, guiCupidIcons, CP_ICON_VERIFIED,
						CP_X(tx), CP_Y(y - 1));
			}
			PrintAt(FONT10ARIAL, FONT_DKYELLOW, tx + 17, y, T(CPS_VERIFIED));
		}
		y += 13;
		const CupidStr status = MemberStatus(pid);
		PrintAt(FONT10ARIAL,
				status == CPS_STATUS_ONLINE ? FONT_DKGREEN : FONT_GRAY5, tx, y,
				T(status));
		y += 12;
		// the Yahoo line: when they last graced the server
		if (status == CPS_STATUS_GONE)
		{
			PrintAt(FONT10ARIAL, FONT_GRAY5, tx, y, T(CPS_ACTIVE_LONG));
		}
		else if (status == CPS_STATUS_ONLINE || status == CPS_STATUS_PAYROLL)
		{
			PrintAt(FONT10ARIAL, FONT_GRAY5, tx, y, T(CPS_ACTIVE_24));
		}
		else
		{
			PrintAt(FONT10ARIAL, FONT_GRAY5, tx, y,
					ST::format(T(CPS_ACTIVE_DAYS), 2 + (pid * 7) % 5));
		}
		y += 15;

		if (PlayerHasProfile())
		{
			const DatingGame::Match match = MatchWith(pid);
			PrintAt(FONT12ARIAL, MatchColour(match.percent), tx, y,
					MatchLabel(match));
			y += 16;
			if (match.answered > 0)
			{
				PrintAt(FONT10ARIAL, FONT_GRAY4, tx, y,
						ST::format(T(CPS_AGREE_ON), match.agree,
								match.answered));
				y += 12;
			}
		}

		PrintAt(FONT10ARIAL, FONT_GRAY3, tx, y,
				ST::format(T(CPS_LOOKING_FOR),
					CUPID_LOOKING[gfCupidGerman ? 1 : 0]
						[p.bAttitude >= 0 && p.bAttitude < NUM_ATTITUDES
							? p.bAttitude : 0]));

		y = 42 + CP_PHOTO_H + 10;

		// deal breakers: the bHated list, published without euphemism
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
			PrintAt(FONT10ARIAL, FONT_DKRED, 90, y,
					ST::format(T(CPS_DEAL_BREAKERS), breakers));
			y += 12;
		}
		const int blockedBy = CountBlockedBy(pid);
		if (blockedBy > 0)
		{
			PrintAt(FONT10ARIAL, FONT_GRAY5, 90, y,
					ST::format(T(CPS_BLOCKED_BY), blockedBy));
			y += 12;
		}
		y += 2;

		PrintAt(FONT10ARIAL, FONT_GRAY5, 90, y, T(CPS_SUMMARY));
		y += 12;
		const char* flavor = FlavorFor(pid);
		const int att = p.bAttitude >= 0 && p.bAttitude < NUM_ATTITUDES
					? p.bAttitude : 0;
		const char* summary = flavor ? flavor
			: (merc ? CUPID_SUMMARY_MERC : CUPID_SUMMARY_AIM)
				[gfCupidGerman ? 1 : 0][att];
		y += DisplayWrappedString(UINT16(CP_X(90)), UINT16(CP_Y(y)), 322, 2,
				FONT10ARIAL, FONT_NEARBLACK, summary, FONT_MCOLOR_BLACK,
				LEFT_JUSTIFIED) + 4;

		// what you'd talk about, quoted from the licensed questionnaire
		if (PlayerHasProfile())
		{
			const DatingGame::Match match = MatchWith(pid);
			if (match.bestQ >= 0 && y < 252)
			{
				PrintAt(FONT10ARIAL, FONT_DKGREEN, 90, y, T(CPS_YOU_AGREED));
				y += 11;
				y += DisplayWrappedString(UINT16(CP_X(98)), UINT16(CP_Y(y)),
						314, 2, FONT10ARIAL, FONT_GRAY3,
						QuizAnswer(match.bestQ, GetAnswer(match.bestQ)),
						FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 3;
			}
			if (match.worstQ >= 0 && y < 272)
			{
				PrintAt(FONT10ARIAL, FONT_DKRED, 90, y, T(CPS_YOU_DIFFER));
				y += 11;
				DisplayWrappedString(UINT16(CP_X(98)), UINT16(CP_Y(y)), 314, 2,
						FONT10ARIAL, FONT_GRAY3, QuizQuestion(match.worstQ),
						FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
			}
		}

		RenderWideButton(0, T(CPS_SEND_FLOWERS), true);
		RenderWideButton(1, T(CPS_BACK), true);
	}

	void RenderSplash()
	{
		// hearts everywhere; the algorithm is very pleased with itself
		uint32_t seed = gCupidSplashPid * 2654435761u + 5;
		for (int i = 0; i < 14; ++i)
		{
			seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
			const INT32 x = 30 + INT32(seed % 440);
			const INT32 y = 34 + INT32((seed >> 9) % 290);
			DrawHeart(x, y, 1 + int(seed % 3), CP_RGB_BLUE);
		}

		PrintCentred(FONT14ARIAL, FONT_DKRED, CP_PAGE_W / 2, 44,
				T(CPS_SPLASH_TITLE));

		// the two of you, full photos, side by side, as the format demands
		const INT32 cy = 72;
		DropShadow(118, cy, 114, 134);
		FillCard(118, cy, 114, 134, CP_RGB_CARD, CP_RGB_PINK, CP_RGB_BG);
		if (guiCupidSelfBig)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidSelfBig, 0, CP_X(122),
					CP_Y(cy + 6));
		}
		else
		{
			FillRect(122, cy + 6, CP_PHOTO_W, CP_PHOTO_H, CP_RGB_CARD_DIM);
			PrintCentred(FONT10ARIAL, FONT_GRAY5, 175, cy + 62,
					T(CPS_NO_PHOTO));
		}
		DrawHeart(241, cy + 56, 3, CP_RGB_PINK);
		DropShadow(270, cy, 114, 134);
		FillCard(270, cy, 114, 134, CP_RGB_CARD, CP_RGB_PINK, CP_RGB_BG);
		SGPVObject* face = BigFaceFor(gCupidSplashPid);
		if (face)
		{
			BltVideoObject(FRAME_BUFFER, face, 0, CP_X(274), CP_Y(cy + 6));
		}

		MERCPROFILESTRUCT const& p = GetProfile(gCupidSplashPid);
		PrintCentred(FONT10ARIAL, FONT_GRAY3, CP_PAGE_W / 2, cy + 146,
				ST::format(T(CPS_SPLASH_SUB), p.zNickname));

		RenderWideButton(0, T(CPS_SPLASH_KEEP), true);
		RenderWideButton(1, T(CPS_SPLASH_VIEW), true);
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
	giCupidPhotoReveal = 6; // the first photo downloads like all the others
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
	if (guiCupidIcons) { DeleteVideoObject(guiCupidIcons); guiCupidIcons = nullptr; }
	if (guiCupidSelf)  { DeleteVideoObject(guiCupidSelf);  guiCupidSelf  = nullptr; }
	if (guiCupidSelfBig) { DeleteVideoObject(guiCupidSelfBig); guiCupidSelfBig = nullptr; }
	if (guiCupidBig)   { DeleteVideoObject(guiCupidBig);   guiCupidBig   = nullptr; }
	if (guiCupidFace)  { DeleteVideoObject(guiCupidFace);  guiCupidFace  = nullptr; }
	gCupidBigPid  = 0xFF;
	gCupidFacePid = 0xFF;
	for (SGPVObject* f : gCupidFaces33)
	{
		if (f) DeleteVideoObject(f);
	}
	gCupidFaces33.clear();
}

void RenderCupid()
{
	FillRect(0, 0, CP_PAGE_W, CP_PAGE_H, CP_RGB_BG);
	if (gCupidPage != CPP_SPLASH) RenderWallpaper();
	RenderTopBar();
	switch (gCupidPage)
	{
		case CPP_DECK:    RenderDeck();    break;
		case CPP_MATCHES: RenderMatches(); break;
		case CPP_ME:      RenderMe();      break;
		case CPP_DETAIL:  RenderDetail();  break;
		case CPP_SPLASH:  RenderSplash();  break;
	}
	RenderFooter();

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
		auto acc = [&](const MOUSE_REGION& r)
		{
			if (Hover(r)) uiHover |= 1u << bit;
			++bit;
		};
		for (const MOUSE_REGION& r : gCupidTabRegion)    acc(r);
		for (const MOUSE_REGION& r : gCupidActionRegion) acc(r);
		for (const MOUSE_REGION& r : gCupidMatchRegion)  acc(r);
		for (const MOUSE_REGION& r : gCupidAnswerRegion) acc(r);
		for (const MOUSE_REGION& r : gCupidFaceRegion)   acc(r);
		acc(gCupidPassRegion);
		acc(gCupidLikeRegion);
		acc(gCupidPopupXRegion);
		static UINT32 uiLastHover = 0;
		if (uiHover != uiLastHover)
		{
			uiLastHover = uiHover;
			CupidRedraw();
		}
	}

	// the photo comes down the wire a few rows at a time
	if (gCupidPage == CPP_DECK && giCupidPhotoReveal < CP_PHOTO_H)
	{
		giCupidPhotoReveal += 9;
		CupidRedraw();
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
	if (gCupidPage != CPP_DECK || !PlayerHasProfile()) return false;
	if (gfCupidPopupUp) return false; // the popup owns the moment
	switch (usParam)
	{
		case SDLK_LEFT:  StartFly(-1); return true;
		case SDLK_RIGHT: StartFly(1);  return true;
		default: return false;
	}
}
