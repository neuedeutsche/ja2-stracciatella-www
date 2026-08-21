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
#define CP_CARD_H       288
#define CP_CARD_X       ((502 - CP_CARD_W) / 2)
#define CP_CARD_Y       34
#define CP_PHOTO_W      106
#define CP_PHOTO_H      122

// swipe thresholds and animation speed, in pixels
#define CP_SWIPE_COMMIT 55
#define CP_FLY_STEP     34

// the two verdict buttons under the card
#define CP_BTN_SIZE     36
#define CP_BTN_Y        (CP_CARD_Y + CP_CARD_H + 8)
#define CP_BTN_PASS_X   (CP_CARD_X + 18)
#define CP_BTN_LIKE_X   (CP_CARD_X + CP_CARD_W - 18 - CP_BTN_SIZE)

#define CP_FACE33_W     29
#define CP_FACE33_H     33
#define CP_FACE65_W     58
#define CP_FACE65_H     65

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

	// the card in hand: dragging follows the pointer, a commit flies it out
	bool  gfCupidDragging = false;
	INT32 giCupidDragAnchor = 0;
	INT32 giCupidCardDx = 0;
	int   giCupidFlyDir = 0; // -1 flying left, +1 flying right, 0 still

	struct Member
	{
		ProfileID pid;
		bool      merc;    // M.E.R.C., unverified, oversharing
		bool      locked;  // not yet unlocked on the M.E.R.C. site
		DatingGame::Profile prof;
	};
	std::vector<Member> gCupidRoster;

	SGPVObject* guiCupidLogo  = nullptr;
	SGPVObject* guiCupidIcons = nullptr;
	SGPVObject* guiCupidSelf  = nullptr;   // the member's own photo, 29x33
	SGPVObject* guiCupidBig   = nullptr;   // the dealt card's 106x122 photo
	ProfileID   gCupidBigPid  = 0xFF;
	SGPVObject* guiCupidFace  = nullptr;   // 65-face for the splash
	ProfileID   gCupidFacePid = 0xFF;
	std::vector<SGPVObject*> gCupidFaces33; // parallel to gCupidRoster

	MOUSE_REGION gCupidTabRegion[3];
	MOUSE_REGION gCupidCardRegion;
	MOUSE_REGION gCupidDropRegion;   // catches a card released off the stage
	MOUSE_REGION gCupidPassRegion;
	MOUSE_REGION gCupidLikeRegion;
	MOUSE_REGION gCupidLangRegion;
	MOUSE_REGION gCupidAnswerRegion[8];
	MOUSE_REGION gCupidActionRegion[2]; // ME landing / detail / splash slots
	MOUSE_REGION gCupidMatchRegion[7];  // clickable rows on the matches page
	MOUSE_REGION gCupidSideAdRegion[2]; // the skyscraper banners, deck page

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
		CPS_ME_TITLE, CPS_ME_IMPORT, CPS_ME_TAKE, CPS_ME_RETAKE, CPS_ME_UPGRADE,
		CPS_ME_COMPLETE, CPS_ME_PARTIAL, CPS_ME_HINT, CPS_ME_POWERED,
		CPS_QUIZ_SEX, CPS_QUIZ_MALE, CPS_QUIZ_FEMALE, CPS_QUIZ_PROGRESS,
		CPS_NO_PHOTO, CPS_NO_PROFILE_CARD,
		CPS_NOTICE_FIRST, CPS_LOOKING_FOR, CPS_SUMMARY,
		CPS_DEAL_BREAKERS, CPS_BLOCKED_BY,
		CPS_YOU_AGREED, CPS_YOU_DIFFER, CPS_AGREE_ON,
		CPS_SEND_FLOWERS, CPS_BACK,
		CPS_TICKER_DEFAULT, CPS_TICKER_NO_PROFILE, CPS_TICKER_NO_LIKES,
		CPS_TICKER_GOLD, CPS_TICKER_BROKE,
		CPS_COUNT
	};

	const char* const CUPID_TEXT[2][CPS_COUNT] =
	{
		{
			"DECK", "MATCHES", "ME",
			"{} likes left today", "GOLD - likes never run out",
			"OUT OF LIKES",
			"LIKE", "NOPE",
			"ONLINE NOW", "AWAY - ON CONTRACT", "ON YOUR PAYROLL",
			"LAST LOGIN: a long time ago", "MARRIED (a satisfied customer)",
			"A.I.M. VERIFIED", "unverified",
			"{}% MATCH", "{}-{}% MATCH",
			"IT'S A MATCH!!", "{} likes you back. The algorithm saw it "
			"coming.", "KEEP SWIPING", "VIEW PROFILE",
			"YOUR MATCHES", "No matches yet. The deck is waiting.",
			"Click a match to read the full dossier.",
			"{} members already LIKE you.", "They liked you. Now you know.",
			"MERCS & KISSES GOLD", "Unlimited likes. See who liked you "
			"FIRST. Prestige beyond measure. One payment of ${}, to me, "
			"Speck T. Kline.", "GET GOLD - ${}", "You are a GOLD member. "
			"Everything I promised is now true.",
			"\"He said he'd never settle down.\"", "\"We are MARRIED now. "
			"Thank you Mercs && Kisses!!!\"", "- Flo, satisfied member",
			"NEW MEMBERS COMING", "The roster grows as M.E.R.C. grows. "
			"Spend generously and love will follow. That is just science.",
			"THAT'S EVERYONE", "You have seen every professional in Arulco. "
			"I am in talks with several other war zones. - S.T.K.",
			"(the heart accepts. the X declines. no refunds.)",
			"THE QUESTIONNAIRE", "IMPORT MY I.M.P. PROFILE (free)",
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
			"MERCS & KISSES - where the tough get tender - a Speck T. "
			"Kline company",
			"No profile, no romance. The ME tab is right there. - Speck",
			"Out of likes. GOLD members never run out. Just saying. - Speck",
			"Thank you for going GOLD. You complete me. - Speck",
			"Your card was declined. It happens to everyone. Not to me. "
			"- Speck",
		},
		{
			"DECK", "MATCHES", "ICH",
			"noch {} Likes heute", "GOLD - Likes gehen nie aus",
			"KEINE LIKES MEHR",
			"LIKE", "NEIN",
			"JETZT ONLINE", "ABWESEND - IM EINSATZ", "AUF IHRER GEHALTSLISTE",
			"LETZTER LOGIN: vor langer Zeit", "VERHEIRATET (zufriedene "
			"Kundin)",
			"A.I.M.-GEPRUEFT", "ungeprueft",
			"{}% PASSUNG", "{}-{}% PASSUNG",
			"EIN MATCH!!", "{} mag Sie auch. Der Algorithmus wusste es "
			"vorher.", "WEITER WISCHEN", "PROFIL ANSEHEN",
			"IHRE MATCHES", "Noch keine Matches. Das Deck wartet.",
			"Klicken Sie ein Match fuer das volle Dossier.",
			"{} Mitglieder LIKEN Sie bereits.", "Sie mochten Sie. Jetzt "
			"wissen Sie es.",
			"MERCS & KISSES GOLD", "Unbegrenzte Likes. Sehen Sie ZUERST, "
			"wer Sie mag. Unermessliches Prestige. Eine Zahlung von {} $, "
			"an mich, Speck T. Kline.", "GOLD HOLEN - {} $",
			"Sie sind GOLD-Mitglied. Alles, was ich versprach, ist jetzt "
			"wahr.",
			"\"Er wollte sich nie binden.\"", "\"Wir sind jetzt VERHEIRATET. "
			"Danke Mercs && Kisses!!!\"", "- Flo, zufriedenes Mitglied",
			"NEUE MITGLIEDER KOMMEN", "Die Liste waechst mit M.E.R.C. Geben "
			"Sie grosszuegig aus, die Liebe folgt. Das ist Wissenschaft.",
			"DAS WAREN ALLE", "Sie haben jeden Profi in Arulco gesehen. Ich "
			"verhandle mit weiteren Kriegsgebieten. - S.T.K.",
			"(das Herz nimmt an. das X lehnt ab. keine Rueckerstattung.)",
			"DER FRAGEBOGEN", "MEIN I.M.P.-PROFIL IMPORTIEREN (gratis)",
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
			"MERCS & KISSES - wo die Harten zaertlich werden - eine Speck "
			"T. Kline Firma",
			"Kein Profil, keine Romantik. Der ICH-Tab ist gleich da. - Speck",
			"Keine Likes mehr. GOLD-Mitgliedern passiert das nie. Nur so. "
			"- Speck",
			"Danke, dass Sie GOLD sind. Sie vervollstaendigen mich. - Speck",
			"Ihre Karte wurde abgelehnt. Passiert jedem. Mir nicht. - Speck",
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

	void CupidRedraw() { fPausedReDrawScreenFlag = TRUE; }

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
		gfCupidDragging = false;

		std::vector<ProfileID> pool;
		for (const Member& m : gCupidRoster)
		{
			if (m.locked || MemberIsDead(m.pid) || MemberIsMarried(m.pid))
			{
				continue;
			}
			if (BitGet(gCupidPersist.ubLiked, m.pid)) continue;
			if (BitGet(gCupidPersist.ubPassed, m.pid)) continue;
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

	void RefreshDailyLikes()
	{
		const UINT16 today = UINT16(GetWorldDay());
		if (gCupidPersist.usDeckDay != today)
		{
			gCupidPersist.usDeckDay   = today;
			gCupidPersist.ubLikesLeft = CP_FREE_LIKES_A_DAY;
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
		if (gCupidDeckPos < int(gCupidDeck.size()) - 1) ++gCupidDeckPos;
	}

	// what happens once the card has flown off the edge
	void CommitSwipe(int dir)
	{
		const Card card = CurrentCard();

		if (card.kind != CARD_MEMBER)
		{
			// the heart accepts an offer; the X declines it
			if (dir > 0 && card.kind == CARD_AD_GOLD && !IsGold())
			{
				if (ChargeSpeck(CP_GOLD_PRICE))
				{
					gCupidPersist.ubFlags |= CUPID_FLAG_GOLD;
					giCupidTicker = CPS_TICKER_GOLD;
				}
			}
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
		gfCupidDragging = false;
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
		gfCupidDragging = false;
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

		if (reason & MSYS_CALLBACK_REASON_POINTER_DWN)
		{
			if (card.kind == CARD_END) return;
			gfCupidDragging  = true;
			giCupidDragAnchor = INT32(gusMouseXPos);
			giCupidCardDx     = 0;
			CupidRedraw();
			return;
		}

		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

		if (gfCupidDragging)
		{
			gfCupidDragging = false;
			if (giCupidCardDx <= -CP_SWIPE_COMMIT) StartFly(-1);
			else if (giCupidCardDx >= CP_SWIPE_COMMIT) StartFly(1);
			else if (std::abs(giCupidCardDx) < 4 && card.kind == CARD_MEMBER)
			{
				// a click, not a drag: open the dossier
				gCupidDetailPid  = card.pid;
				gCupidDetailFrom = CPP_DECK;
				gCupidPage = CPP_DETAIL;
				giCupidCardDx = 0;
			}
			else
			{
				giCupidCardDx = 0; // snap back
			}
			CupidRedraw();
		}
	}

	void DropCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (!gfCupidDragging) return;
		gfCupidDragging = false;
		if (giCupidCardDx <= -CP_SWIPE_COMMIT) StartFly(-1);
		else if (giCupidCardDx >= CP_SWIPE_COMMIT) StartFly(1);
		else giCupidCardDx = 0;
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
				const bool paid = PlayerHasProfile() || HaveStoredAnswers();
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

	void CupidPlaceRegions()
	{
		if (gfCupidRegionsUp) return;

		// everything released outside the card still resolves the drag; sits
		// above the laptop's own screen region, below the live controls
		MSYS_DefineRegion(&gCupidDropRegion,
				UINT16(CP_X(0)), UINT16(CP_Y(0)),
				UINT16(CP_X(CP_PAGE_W)), UINT16(CP_Y(CP_PAGE_H)),
				MSYS_PRIORITY_NORMAL + 2, CURSOR_WWW, MSYS_NO_CALLBACK,
				DropCallback);

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
			const INT32 x = 116 + i * 146;
			MSYS_DefineRegion(&gCupidActionRegion[i],
					UINT16(CP_X(x)), UINT16(CP_Y(CP_PAGE_H - 52)),
					UINT16(CP_X(x + 124)), UINT16(CP_Y(CP_PAGE_H - 28)),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					ActionCallback);
			MSYS_SetRegionUserData(&gCupidActionRegion[i], 0, i);
		}

		for (int i = 0; i < 2; ++i)
		{
			const INT32 x = i == 0 ? 16 : CP_PAGE_W - 16 - 88;
			MSYS_DefineRegion(&gCupidSideAdRegion[i],
					UINT16(CP_X(x)), UINT16(CP_Y(44)),
					UINT16(CP_X(x + 88)), UINT16(CP_Y(44 + 286)),
					MSYS_PRIORITY_HIGH - 2, CURSOR_WWW, MSYS_NO_CALLBACK,
					SideAdCallback);
			MSYS_SetRegionUserData(&gCupidSideAdRegion[i], 0, i);
		}

		for (int i = 0; i < 7; ++i)
		{
			const INT32 y = 96 + i * 40;
			MSYS_DefineRegion(&gCupidMatchRegion[i],
					UINT16(CP_X(96)), UINT16(CP_Y(y)),
					UINT16(CP_X(96 + 310)), UINT16(CP_Y(y + 38)),
					MSYS_PRIORITY_HIGH - 1, CURSOR_WWW, MSYS_NO_CALLBACK,
					MatchRowCallback);
			MSYS_SetRegionUserData(&gCupidMatchRegion[i], 0, i);
		}

		gfCupidRegionsUp = true;
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
		MSYS_RemoveRegion(&gCupidDropRegion);
		MSYS_RemoveRegion(&gCupidPassRegion);
		MSYS_RemoveRegion(&gCupidLikeRegion);
		MSYS_RemoveRegion(&gCupidLangRegion);
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
				GelPill(x, 3, 74, CP_TOPBAR_H - 6, CP_RGB_BLUE_PALE,
						CP_RGB_BLUE_LITE, CP_RGB_BLUE_DK, CP_RGB_BLUE);
			}
			PrintCentred(FONT10ARIALBOLD,
					active ? FONT_MCOLOR_WHITE : FONT_NEARBLACK,
					x + 37, 8, T(tabs[i]));
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
		PrintCentred(FONT10ARIAL, FONT_NEARBLACK, CP_PAGE_W / 2, CP_PAGE_H - 12,
				T(CupidStr(giCupidTicker)));
	}

	void RenderVerdictStamp(INT32 cardX)
	{
		if (giCupidCardDx >= CP_SWIPE_COMMIT || giCupidFlyDir > 0)
		{
			FillRounded(cardX + 10, CP_CARD_Y + 12, 52, 20, CP_RGB_LIKE, 3,
					CP_RGB_BG);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cardX + 36,
					CP_CARD_Y + 18, T(CPS_STAMP_LIKE));
		}
		else if (giCupidCardDx <= -CP_SWIPE_COMMIT || giCupidFlyDir < 0)
		{
			FillRounded(cardX + CP_CARD_W - 62, CP_CARD_Y + 12, 52, 20,
					CP_RGB_NOPE, 3, CP_RGB_BG);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
					cardX + CP_CARD_W - 36, CP_CARD_Y + 18,
					T(CPS_STAMP_NOPE));
		}
	}

	void RenderMemberCard(const Card& card, INT32 cardX)
	{
		MERCPROFILESTRUCT const& p = GetProfile(card.pid);
		const int idx = RosterIndexOf(card.pid);
		const bool merc = idx >= 0 && gCupidRoster[size_t(idx)].merc;

		FillCard(cardX, CP_CARD_Y, CP_CARD_W, CP_CARD_H, CP_RGB_CARD,
				CP_RGB_BLUE_DK, CP_RGB_BG);

		// the photo, big, the entire point of the format - matted like a
		// polaroid, with an ink keyline so it sits in the card
		const INT32 photoX = cardX + (CP_CARD_W - CP_PHOTO_W) / 2;
		FillRect(photoX - 4, CP_CARD_Y + 6, CP_PHOTO_W + 8, CP_PHOTO_H + 8,
				CP_RGB_INK);
		FillRect(photoX - 3, CP_CARD_Y + 7, CP_PHOTO_W + 6, CP_PHOTO_H + 6,
				CP_RGB_MAT);
		SGPVObject* big = BigFaceFor(card.pid);
		if (big)
		{
			BltVideoObject(FRAME_BUFFER, big, 0, CP_X(photoX),
					CP_Y(CP_CARD_Y + 10));
		}
		else
		{
			FillRect(photoX, CP_CARD_Y + 10, CP_PHOTO_W, CP_PHOTO_H,
					CP_RGB_CARD_DIM);
			PrintCentred(FONT10ARIAL, FONT_GRAY5, cardX + CP_CARD_W / 2,
					CP_CARD_Y + 64, T(CPS_NO_PHOTO));
		}

		INT32 y = CP_CARD_Y + 10 + CP_PHOTO_H + 10;
		PrintCentred(FONT14ARIAL, FONT_NEARBLACK, cardX + CP_CARD_W / 2, y,
				p.zNickname);
		y += 18;

		if (PlayerHasProfile())
		{
			const DatingGame::Match match = MatchWith(card.pid);
			PrintCentred(FONT12ARIAL, MatchColour(match.percent),
					cardX + CP_CARD_W / 2, y, MatchLabel(match));
			y += 15;
			DrawMeter(cardX + (CP_CARD_W - 120) / 2, y, 120, match.percent,
					match.percent >= 75 ? CP_RGB_LIKE
					: match.percent >= 50 ? CP_RGB_GOLD : CP_RGB_NOPE);
			y += 13;
		}
		else
		{
			PrintCentred(FONT12ARIAL, FONT_GRAY5, cardX + CP_CARD_W / 2, y,
					"??%");
			y += 28;
		}

		const CupidStr status = MemberStatus(card.pid);
		PrintCentred(FONT10ARIAL,
				status == CPS_STATUS_ONLINE ? FONT_DKGREEN : FONT_GRAY5,
				cardX + CP_CARD_W / 2, y, T(status));
		y += 14;

		// verification is the class system here
		if (merc)
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY5, cardX + CP_CARD_W / 2, y,
					T(CPS_UNVERIFIED));
		}
		else
		{
			const ST::string v = T(CPS_VERIFIED);
			const INT32 w = StringPixLength(v, FONT10ARIAL);
			if (guiCupidIcons)
			{
				BltVideoObject(FRAME_BUFFER, guiCupidIcons, CP_ICON_VERIFIED,
						CP_X(cardX + (CP_CARD_W - w) / 2 - 17), CP_Y(y - 2));
			}
			PrintCentred(FONT10ARIAL, FONT_DKYELLOW, cardX + CP_CARD_W / 2, y,
					v);
		}
		y += 16;

		const int trait = p.bPersonalityTrait >= 0 && p.bPersonalityTrait < 8
					? p.bPersonalityTrait : 0;
		DisplayWrappedString(UINT16(CP_X(cardX + 10)), UINT16(CP_Y(y)),
				CP_CARD_W - 20, 2, FONT10ARIAL, FONT_GRAY4,
				ST::format(T(CPS_NOTICE_FIRST),
					CUPID_TRAIT_SPIN[gfCupidGerman ? 1 : 0][trait]),
				FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
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

	// the empty flanks of the stage carry skyscraper banners, as was the law
	void RenderSideAds()
	{
		// left: the Parlour, house green, a rival Kingpin property
		const INT32 lx = 16;
		DropShadow(lx, 44, 88, 286);
		FillCard(lx, 44, 88, 286, FROMRGB(24, 82, 50), FROMRGB(16, 56, 34),
				CP_RGB_BG);
		PrintCentred(FONT10ARIAL, FONT_GRAY6, lx + 78, 48, "AD");
		PrintCentred(FONT12ARIAL, FONT_MCOLOR_WHITE, lx + 44, 72, "TIRED");
		PrintCentred(FONT12ARIAL, FONT_MCOLOR_WHITE, lx + 44, 90, "OF");
		PrintCentred(FONT12ARIAL, FONT_MCOLOR_WHITE, lx + 44, 108, "LOVE?");
		FillRect(lx + 16, 130, 56, 1, FROMRGB(120, 180, 140));
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_LTGREEN, lx + 44, 142, "TRY");
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_LTGREEN, lx + 44, 156, "LUCK");
		// a fan of tiles, suggested in three blank dominoes
		for (int i = 0; i < 3; ++i)
		{
			FillRounded(lx + 20 + i * 18, 178, 14, 22, CP_RGB_CARD, 2,
					FROMRGB(24, 82, 50));
			FillRect(lx + 23 + i * 18, 183 + (i % 2) * 6, 8, 6,
					i == 1 ? CP_RGB_NOPE : FROMRGB(24, 82, 50));
		}
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, lx + 44, 226, "SAN MONA");
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, lx + 44, 238, "MAHJONG");
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, lx + 44, 250, "PARLOUR");
		FillRounded(lx + 12, 292, 64, 18, FROMRGB(120, 180, 140), 3,
				FROMRGB(24, 82, 50));
		PrintCentred(FONT10ARIALBOLD, FONT_NEARBLACK, lx + 44, 297, "VISIT");

		// right: the florist, because the funnel has a next step
		const INT32 rx = CP_PAGE_W - 16 - 88;
		DropShadow(rx, 44, 88, 286);
		FillCard(rx, 44, 88, 286, CP_RGB_PINK_PALE, CP_RGB_PINK, CP_RGB_BG);
		PrintCentred(FONT10ARIAL, FONT_GRAY5, rx + 78, 48, "AD");
		PrintCentred(FONT12ARIAL, FONT_DKRED, rx + 44, 72, "SAY IT");
		PrintCentred(FONT12ARIAL, FONT_DKRED, rx + 44, 90, "WITH");
		PrintCentred(FONT12ARIAL, FONT_DKRED, rx + 44, 108, "FLOWERS");
		// a rose: bloom over a stem
		DrawHeart(rx + 33, 140, 3, CP_RGB_PINK);
		FillRect(rx + 42, 160, 3, 34, FROMRGB(88, 138, 74));
		FillRect(rx + 36, 170, 8, 3, FROMRGB(88, 138, 74));
		PrintCentred(FONT10ARIAL, FONT_GRAY3, rx + 44, 210, "matched?");
		PrintCentred(FONT10ARIAL, FONT_GRAY3, rx + 44, 222, "don't just");
		PrintCentred(FONT10ARIAL, FONT_GRAY3, rx + 44, 234, "sit there.");
		PrintCentred(FONT10ARIAL, FONT_NEARBLACK, rx + 44, 254, "UNITED");
		PrintCentred(FONT10ARIAL, FONT_NEARBLACK, rx + 44, 266, "FLORAL");
		PrintCentred(FONT10ARIAL, FONT_NEARBLACK, rx + 44, 278, "SERVICE");
		FillRounded(rx + 12, 292, 64, 18, CP_RGB_PINK, 3, CP_RGB_PINK_PALE);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, rx + 44, 297,
				"ORDER");
	}

	void RenderDeck()
	{
		RenderSideAds();

		if (!PlayerHasProfile())
		{
			// the pitch card: this is a landing page and it should sell
			DropShadow(CP_CARD_X, CP_CARD_Y, CP_CARD_W, CP_CARD_H);
			FillCard(CP_CARD_X, CP_CARD_Y, CP_CARD_W, CP_CARD_H, CP_RGB_CARD,
					CP_RGB_PINK, CP_RGB_BG);
			const INT32 cx = CP_CARD_X + CP_CARD_W / 2;
			DrawHeart(cx - 17, CP_CARD_Y + 20, 5, CP_RGB_PINK);
			PrintCentred(FONT14ARIAL, FONT_NEARBLACK, cx, CP_CARD_Y + 62,
					"MERCS & KISSES");
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, CP_CARD_Y + 80,
					gfCupidGerman ? "Wo die Harten zaertlich werden"
						      : "Where the tough get tender");
			FillRect(CP_CARD_X + 24, CP_CARD_Y + 96, CP_CARD_W - 48, 1,
					CP_RGB_CARD_DIM);

			// three members you could be meeting, decency-blurred
			INT32 fx = cx - (3 * CP_FACE33_W + 2 * 6) / 2;
			int shown = 0;
			for (const Member& m : gCupidRoster)
			{
				if (shown >= 3) break;
				if (m.locked || MemberIsDead(m.pid)) continue;
				SGPVObject* face = Face33For(m.pid);
				if (!face) continue;
				FillRect(fx - 2, CP_CARD_Y + 108, CP_FACE33_W + 4,
						CP_FACE33_H + 4, CP_RGB_MAT);
				BltVideoObject(FRAME_BUFFER, face, 0, CP_X(fx),
						CP_Y(CP_CARD_Y + 110));
				BlurOver(fx, CP_CARD_Y + 110, CP_FACE33_W, CP_FACE33_H);
				fx += CP_FACE33_W + 6;
				++shown;
			}
			PrintCentred(FONT10ARIAL, FONT_GRAY4, cx, CP_CARD_Y + 152,
					gfCupidGerman ? "51 Profis warten schon"
						      : "51 professionals are waiting");

			DisplayWrappedString(UINT16(CP_X(CP_CARD_X + 16)),
					UINT16(CP_Y(CP_CARD_Y + 176)), CP_CARD_W - 32, 2,
					FONT10ARIAL, FONT_GRAY3, T(CPS_NO_PROFILE_CARD),
					FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

			GelPill(CP_CARD_X + 20, CP_CARD_Y + CP_CARD_H - 52,
					CP_CARD_W - 40, 26, CP_RGB_PINK, CP_RGB_PINK_LITE,
					CP_RGB_PINK_DK, CP_RGB_CARD);
			PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE, cx,
					CP_CARD_Y + CP_CARD_H - 44,
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

		// the verdict buttons: white gel with a coloured ring
		DropShadow(CP_BTN_PASS_X, CP_BTN_Y, CP_BTN_SIZE, CP_BTN_SIZE);
		GelPill(CP_BTN_PASS_X, CP_BTN_Y, CP_BTN_SIZE, CP_BTN_SIZE,
				CP_RGB_CARD, CP_RGB_CARD_LITE, CP_RGB_NOPE, CP_RGB_BG);
		DrawCross(CP_BTN_PASS_X + 11, CP_BTN_Y + 11, 14, 3, CP_RGB_NOPE);

		const bool likeLive = CanLike() && card.kind != CARD_END;
		DropShadow(CP_BTN_LIKE_X, CP_BTN_Y, CP_BTN_SIZE, CP_BTN_SIZE);
		GelPill(CP_BTN_LIKE_X, CP_BTN_Y, CP_BTN_SIZE, CP_BTN_SIZE,
				CP_RGB_CARD, CP_RGB_CARD_LITE,
				likeLive ? CP_RGB_PINK : CP_RGB_GREY, CP_RGB_BG);
		DrawHeart(CP_BTN_LIKE_X + 8, CP_BTN_Y + 10, 3,
				likeLive ? CP_RGB_PINK : CP_RGB_GREY);

		// the allowance between the buttons
		const ST::string likes = IsGold() ? T(CPS_LIKES_GOLD)
			: CanLike()
				? ST::format(T(CPS_LIKES_LEFT), gCupidPersist.ubLikesLeft)
				: T(CPS_OUT_OF_LIKES);
		PrintCentred(FONT10ARIAL, IsGold() ? FONT_DKYELLOW
				: CanLike() ? FONT_GRAY4 : FONT_DKRED,
				251, CP_BTN_Y + 14, likes);
	}

	void RenderMatches()
	{
		PrintCentred(FONT12ARIAL, FONT_NEARBLACK, CP_PAGE_W / 2, 32,
				T(CPS_MATCHES_TITLE));

		// the GOLD tease: they liked you, and you may pay to know who
		const std::vector<ProfileID> admirers = SecretAdmirers();
		if (!admirers.empty())
		{
			PrintAt(FONT10ARIALBOLD, FONT_DKRED, 96, 50,
					ST::format(T(IsGold() ? CPS_LIKED_YOU_GOLD : CPS_LIKED_YOU),
							int(admirers.size())));
			INT32 x = 96;
			for (ProfileID pid : admirers)
			{
				SGPVObject* face = Face33For(pid);
				if (face)
				{
					BltVideoObject(FRAME_BUFFER, face, 0, CP_X(x), CP_Y(62));
				}
				if (!IsGold()) BlurOver(x, 62, CP_FACE33_W, CP_FACE33_H);
				x += CP_FACE33_W + 4;
			}
			if (IsGold())
			{
				x += 6;
				INT32 nx = x;
				for (ProfileID pid : admirers)
				{
					PrintAt(FONT10ARIAL, FONT_GRAY3, nx, 72,
							GetProfile(pid).zNickname);
					nx += StringPixLength(GetProfile(pid).zNickname,
							FONT10ARIAL) + 8;
				}
			}
		}

		const std::vector<ProfileID> matches = AllMatches();
		if (matches.empty())
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY4, CP_PAGE_W / 2, 160,
					T(CPS_MATCHES_NONE));
			return;
		}

		INT32 y = 96;
		int row = 0;
		for (ProfileID pid : matches)
		{
			if (row >= 7) break;
			DropShadow(96, y, 310, 38);
			FillCard(96, y, 310, 38, CP_RGB_CARD, CP_RGB_BLUE, CP_RGB_BG);
			SGPVObject* face = Face33For(pid);
			if (face)
			{
				BltVideoObject(FRAME_BUFFER, face, 0, CP_X(100), CP_Y(y + 2));
			}
			MERCPROFILESTRUCT const& p = GetProfile(pid);
			const bool dead = MemberIsDead(pid);
			PrintAt(FONT10ARIALBOLD, dead ? FONT_GRAY5 : FONT_NEARBLACK,
					138, y + 6, p.zNickname);
			const DatingGame::Match match = MatchWith(pid);
			PrintAt(FONT10ARIAL, dead ? FONT_GRAY5 : MatchColour(match.percent),
					138, y + 20, MatchLabel(match));
			PrintAt(FONT10ARIAL, FONT_GRAY5, 240, y + 20, T(MemberStatus(pid)));
			DrawHeart(388, y + 14, 1, dead ? CP_RGB_GREY : CP_RGB_PINK);
			y += 40;
			++row;
		}
		PrintCentred(FONT10ARIAL, FONT_GRAY5, CP_PAGE_W / 2, y + 4,
				T(CPS_MATCHES_HINT));
	}

	void RenderWideButton(INT32 slot, const ST::string& label, bool live)
	{
		const INT32 x = 116 + slot * 146;
		DropShadow(x, CP_PAGE_H - 52, 124, 24);
		if (live)
		{
			GelPill(x, CP_PAGE_H - 52, 124, 24, CP_RGB_PINK,
					CP_RGB_PINK_LITE, CP_RGB_PINK_DK, CP_RGB_BG);
		}
		else
		{
			GelPill(x, CP_PAGE_H - 52, 124, 24, CP_RGB_CARD_DIM,
					CP_RGB_BLUE_LITE, CP_RGB_GREY, CP_RGB_BG);
		}
		PrintCentred(FONT10ARIALBOLD, live ? FONT_MCOLOR_WHITE : FONT_GRAY5,
				x + 62, CP_PAGE_H - 45, label);
	}

	void RenderMeLanding()
	{
		PrintCentred(FONT12ARIAL, FONT_NEARBLACK, CP_PAGE_W / 2, 32,
				T(CPS_ME_TITLE));

		// the member's own card: photo, or the penalty for not having one
		DropShadow(186, 50, 130, 128);
		FillCard(186, 50, 130, 128, CP_RGB_CARD, CP_RGB_BLUE_DK, CP_RGB_BG);
		if (guiCupidSelf)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidSelf, 0, CP_X(202), CP_Y(58));
		}
		else
		{
			FillRect(202, 58, CP_FACE33_W, CP_FACE33_H, CP_RGB_CARD_DIM);
			PrintAt(FONT10ARIAL, FONT_GRAY5, 236, 66, T(CPS_NO_PHOTO));
		}

		const bool full = PlayerHasProfile() && HaveStoredAnswers();
		PrintCentred(FONT10ARIALBOLD, full ? FONT_DKGREEN : FONT_DKRED,
				251, 100, T(full ? CPS_ME_COMPLETE : CPS_ME_PARTIAL));
		const int fillW = full ? 106 : 106 * 60 / 100;
		FillRect(198, 116, 106, 8, CP_RGB_CARD_DIM);
		FillRect(198, 116, fillW, 8, full ? CP_RGB_LIKE : CP_RGB_PINK);
		if (!full)
		{
			DisplayWrappedString(UINT16(CP_X(192)), UINT16(CP_Y(130)), 118, 2,
					FONT10ARIAL, FONT_GRAY4, T(CPS_ME_HINT),
					FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
		}

		// POWERED BY I.M.P., 88x31 in spirit
		FillCard(186, 192, 130, 36, CP_RGB_BLUE_PALE, CP_RGB_BLUE_DK,
				CP_RGB_BG);
		PrintCentred(FONT10ARIALBOLD, FONT_NEARBLACK, 251, 198, "POWERED BY");
		PrintCentred(FONT10ARIALBOLD, FONT_NEARBLACK, 251, 210, "I.M.P.");
		DisplayWrappedString(UINT16(CP_X(106)), UINT16(CP_Y(238)), 290, 2,
				FONT10ARIAL, FONT_GRAY5, T(CPS_ME_POWERED),
				FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

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
			GelPill(96, y, 310, h, CP_RGB_CARD, CP_RGB_CARD_LITE,
					CP_RGB_BLUE_DK, CP_RGB_BG);
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

		DropShadow(76, 30, 350, 306);
		FillCard(76, 30, 350, 306, CP_RGB_CARD, CP_RGB_BLUE_DK, CP_RGB_BG);

		SGPVObject* big = BigFaceFor(pid);
		if (big)
		{
			BltVideoObject(FRAME_BUFFER, big, 0, CP_X(90), CP_Y(42));
		}

		const INT32 tx = 90 + CP_PHOTO_W + 12;
		INT32 y = 44;
		PrintAt(FONT14ARIAL, FONT_NEARBLACK, tx, y, p.zNickname);
		y += 17;
		PrintAt(FONT10ARIAL, FONT_GRAY4, tx, y, p.zName);
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
			if (match.bestQ >= 0 && y < 286)
			{
				PrintAt(FONT10ARIAL, FONT_DKGREEN, 90, y, T(CPS_YOU_AGREED));
				y += 11;
				y += DisplayWrappedString(UINT16(CP_X(98)), UINT16(CP_Y(y)),
						314, 2, FONT10ARIAL, FONT_GRAY3,
						QuizAnswer(match.bestQ, GetAnswer(match.bestQ)),
						FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 3;
			}
			if (match.worstQ >= 0 && y < 300)
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

		PrintCentred(FONT14ARIAL, FONT_DKRED, CP_PAGE_W / 2, 62,
				T(CPS_SPLASH_TITLE));

		// the two of you, side by side, as the format demands
		const INT32 cy = 116;
		DropShadow(160, cy, 74, 84);
		FillCard(160, cy, 74, 84, CP_RGB_CARD, CP_RGB_PINK, CP_RGB_BG);
		if (guiCupidSelf)
		{
			BltVideoObject(FRAME_BUFFER, guiCupidSelf, 0, CP_X(182),
					CP_Y(cy + 10));
		}
		else
		{
			PrintCentred(FONT10ARIAL, FONT_GRAY5, 197, cy + 24,
					T(CPS_NO_PHOTO));
		}
		DrawHeart(240, cy + 32, 3, CP_RGB_PINK);
		DropShadow(268, cy, 74, 84);
		FillCard(268, cy, 74, 84, CP_RGB_CARD, CP_RGB_PINK, CP_RGB_BG);
		SGPVObject* face = Face65For(gCupidSplashPid);
		if (face)
		{
			BltVideoObject(FRAME_BUFFER, face, 0, CP_X(276), CP_Y(cy + 8));
		}

		MERCPROFILESTRUCT const& p = GetProfile(gCupidSplashPid);
		PrintCentred(FONT10ARIAL, FONT_GRAY3, CP_PAGE_W / 2, cy + 96,
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
			guiCupidSelf = Load33Portrait(GetProfile(PlayerImpPid()));
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
			gCupidFaces33[i] = Load33Portrait(GetProfile(gCupidRoster[i].pid));
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
	}
	gCupidPersist.ubFlags |= CUPID_FLAG_VISITED;

	BuildDeck();
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
	// the card in hand follows the pointer
	if (gfCupidDragging)
	{
		giCupidCardDx = INT32(gusMouseXPos) - giCupidDragAnchor;
		CupidRedraw();
		return;
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
	switch (usParam)
	{
		case SDLK_LEFT:  StartFly(-1); return true;
		case SDLK_RIGHT: StartFly(1);  return true;
		default: return false;
	}
}
