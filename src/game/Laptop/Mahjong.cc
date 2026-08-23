// Arulco Mahjong Club: an in-game "online" Sichuan-rules mahjong parlour
// where the player faces Enrico (right), Deidranna (across) and Elliot
// (left) in first-person table view. All rules/AI live in the engine-free
// MahjongGame class; this file is the laptop page wrapper.

#include "Mahjong.h"

#include "Button_Sound_Control.h"
#include "Button_System.h"
#include "Cursors.h"
#include "Directories.h"
#include "EMail.h"
#include "Faces.h"
#include "Campaign.h"
#include "Finances.h"
#include "Game_Clock.h"
#include "Game_Event_Hook.h"
#include "History.h"
#include "IMP_Compile_Character.h"
#include "GameInstance.h"
#include "ContentManager.h"
#include "RPCSmallFaceModel.h"
#include "LaptopSave.h"
#include "MercPortrait.h"
#include "Soldier_Add.h"
#include "Font.h"
#include "Font_Control.h"
#include "HImage.h"
#include "Input.h"
#include "Laptop.h"
#include "Local.h"
#include <cmath>
#include "MahjongGame.h"
#include "MouseSystem.h"
#include "Soldier_Profile.h"
#include "Sound_Control.h"
#include "Timer_Control.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"
#include "WordWrap.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <string_theory/format>
#include <string_theory/string>

#include <SDL_keycode.h>

#define MJ_X(x) ((INT32)(LAPTOP_SCREEN_UL_X + (x)))
#define MJ_Y(y) ((INT32)(LAPTOP_SCREEN_WEB_UL_Y + (y)))

// tiles
#define MJ_TILE_W		30
#define MJ_TILE_H		40
#define MJ_TILE_PITCH		32
#define MJ_MINI_W		20
#define MJ_MINI_H		27
#define MJ_MINI_PITCH		22
#define MJ_POND_ROW_PITCH	29

// opponents: vertical edge panels left/right, horizontal card top-centre,
// all top-aligned and all with the normalised 58x65 face
#define MJ_TOP_Y		5   // a sliver of felt above the panels
#define MJ_TOP_H		71   // Deidranna's centre card
#define MJ_PANEL_W		164
#define MJ_PANEL_X1		169
#define MJ_SIDE_W		66   // Elliot / Enrico vertical panels
#define MJ_SIDE_H		143  // panel foot aligns with the side pond
#define MJ_LEFT_X		5
#define MJ_RIGHT_X		431

// side panels (left = Elliot, right = Enrico); NPC 65-faces are 58x65
#define MJ_FACE65_W		58
#define MJ_FACE65_H		65
#define MJ_FACE33_W		29
#define MJ_FACE33_H		33


// discard ponds (first person: yours at the bottom, seat order CCW)
#define MJ_POND_COLS		8
#define MJ_POND_TOP_X		163
#define MJ_POND_TOP_Y		77
#define MJ_POND_BOTTOM_Y	181
#define MJ_POND_SIDE_ROWS	5
#define MJ_SIDE_Y		5  // side seats sit flush with the top edge
#define MJ_POND_SIDE_Y		5  // side ponds align with the panel tops
#define MJ_POND_LEFT_X		72
#define MJ_POND_RIGHT_X		408

// hand + buttons + chat bar
#define MJ_HAND_Y		218
#define MJ_HAND_RAISE		6
#define MJ_HAND_X		22
#define MJ_DRAWN_X		450
#define MJ_SETUP_BTN_Y		156  // void/pass live mid-table, clear of the hand
#define MJ_CLAIM_BTN_X		22
#define MJ_CLAIM_BTN_Y		186
#define MJ_CHAT_Y		264
#define MJ_CHAT_H		136  // runs to the true bottom of the web page area
#define MJ_CHAT_VISIBLE		7
#define MJ_CHAT_HISTORY		44
#define MJ_CLUSTER_GAP		5   // air between one speaker's block and the next
#define MJ_CHAT_W		338  // the right side of the bar is the info block
#define MJ_CHAT_INPUT_MAX	220
// the singular UI accent: green with a slight tint toward blue
// the parlour's own build number - bump it when the site changes
#define MJ_PAGE_VERSION		"v0.4.2"

#define MJ_TOKEN_RGB		FROMRGB(74, 182, 100)
// your own voice in the room: a warm cream, distinct from everyone else's
// neutral grey and from the yellow the table uses for money
#define MJ_SELF_NAME_RGB	FROMRGB(246, 228, 176)
#define MJ_SELF_TEXT_RGB	FROMRGB(214, 196, 150)

#define MJ_NUM_HAND_SLOTS	14
#define MJ_DRAWN_SLOT		13

// timing (ms)
#define MJ_DEAL_STEPS		14   // rapid ticks, accelerating, closed by a slam
#define MJ_AI_THINK_MIN		600
#define MJ_AI_THINK_SPREAD	900
#define MJ_RON_WINDOW_TIME	4000
#define MJ_ANNOUNCE_TIME	2200

enum MahjongUiState
{
	MJUI_IDLE,
	MJUI_LOBBY,   // the site's home page
	MJUI_LADDER,  // ratings and results
	MJUI_DEALING,
	MJUI_EXCHANGE,
	MJUI_CHOOSE_VOID,
	MJUI_PLAYER_TURN,
	MJUI_AI_THINK,
	MJUI_RON_WINDOW,
	MJUI_CLAIM_WINDOW, // you may pong/kong the discard on the table
	MJUI_ROB_WINDOW,   // someone is promoting a pong: rob the 4th tile to win
	MJUI_ANNOUNCE,   // brief pause showing a mid-hand win before play continues
	MJUI_HAND_END,
	MJUI_MATCH_END,
};

// Session-lifetime: survives leaving the site; never reaches save games.
static std::unique_ptr<MahjongGame> gGame;

// Page-lifetime
static MahjongUiState guiMJState = MJUI_IDLE;
static UINT32 guiMJNextEventTime = 0;
static UINT32 guiMJDealStep = 0;
static INT8 gbMJSelectedSlot = -1;
static ST::string gMJMessage;
static SGPVObject* guiMJFace65[3];    // 58x65: Enrico, Deidranna, Elliot
static SGPVObject* guiMJFace33[3];    // 29x33 variants for the cramped top band
static SGPVObject* guiMJBigFace[3];   // 106x122 talking heads
static SGPVObject* guiMJTiles;        // 30x40 sheet: 27 faces + back + tinted
static SGPVObject* guiMJTilesSmall;   // 20x27 sheet
static SGPVObject* guiMJFelt;         // 502x381 table background
static SGPVObject* guiMJLogo;         // 380x100 parlour sign
static SGPVObject* guiMJChips;        // 18x7 casino chips: 0 house, 1 gold
static SGPVObject* guiMJSelfFace;     // your I.M.P. character's 65face (odd size)
static SGPVSurface* guiMJSelfFaceSurf; // big portrait, stretch-blitted to fill
static ST::string gMJSelfNick;         // your table handle
static SGPVSurface* guiMJShillSurf;    // the House shill warming your seat
static UINT8 gubMJShillPid = 0;        // which merc is warming it (for voice)
static ST::string gMJShillNick;
static SGPVObject* guiMJStatic;       // 65x56 TV static, 3 frames
static SGPVObject* guiMJFeltRed;      // 502x381 red felt for the ladder page
static SGPVObject* guiMJDragon;       // dragon medallions: 120 red/gold, 56 red/gold
static SGPVObject* guiMJGlow;         // pool of light over your own tiles
static SGPVObject* guiMJRail;         // wooden lip between felt and client
static SGPVObject* guiMJKingpinFace;  // the proprietor, for the rules page signature
static SGPVObject* guiMJSign;         // his handwritten scrawl
static SGPVObject* guiMJVoidIcon;     // red suit glyphs for the void marker
static SGPVSurface* guiMJChipSurf[3];  // 16bpp bakes of the 29x33 faces for chat mini-avatars
// webcam life: blink schedules and talk windows per opponent feed
static UINT32 guiMJBlinkAt[3];
// superstition machinery: per-match loss streaks and the Queen's decree
static UINT8 gMJLossStreak[4];
static INT8 gbMJDecreeRank = -1;
static BOOLEAN gfMJDecreeChecked = FALSE;

// "poor connection": an opponent's video feed occasionally drops to static
static int giMJGlitchWho = -1;
static UINT32 guiMJGlitchEnd = 0;
static UINT32 guiMJNextGlitch = 0;
static FACETYPE* gpMJOverlayFace;     // animated "video chat" face on the win overlay
static SGPVSurface* guiMJFaceSurface;
static MOUSE_REGION gMJHandRegion[MJ_NUM_HAND_SLOTS];
static GUIButtonRef guiMJNewGameBtn;
static GUIButtonRef guiMJMahjongBtn;
static MOUSE_REGION gMJVoidRegion[3]; // the drawn void-pick buttons
static GUIButtonRef guiMJPassBtn;
static GUIButtonRef guiMJPongBtn;
static GUIButtonRef guiMJKongBtn;
static GUIButtonRef guiMJLeaveBtn;
static BOOLEAN gfMJClaimKongPossible = FALSE;
static MahjongGame::TileId gMJRobTile = MahjongGame::NO_TILE; // pending added kong
static int giMJGrudge = 0;             // times you ronned the Queen this session
static int giMJElliotGoodNights = 0;   // Elliot's secret positive matches
static BOOLEAN gfMJElliotSecretSent = FALSE;
static BOOLEAN gfMJBackRoom = FALSE;   // Kingpin's 5x table (unlock: 3 match wins)
static MOUSE_REGION gMJRulesDismissRegion;
static BOOLEAN gfMJShowRules = FALSE;
static int giMJOverlayKind = 0; // 1 = house rules, 2 = guestbook
static MOUSE_REGION gMJChatUpRegion;
static MOUSE_REGION gMJChatDownRegion;
static MOUSE_REGION gMJSponsorRegion;
static MOUSE_REGION gMJModalCTARegion; // the drawn gold button on the verdict card
static BOOLEAN gfMJSaidWallLow = FALSE;
// per-hand cooldowns for contextual table talk
static BOOLEAN gfMJSaidTenpai[4];
static BOOLEAN gfMJSaidBadHand[4];
static BOOLEAN gfMJSaidVoidDraw[4];
static BOOLEAN gfMJExchangeSel[MJ_NUM_HAND_SLOTS] = {};
static MOUSE_REGION gMJChatRegion;
static MOUSE_REGION gMJLobbyRegion[4];
static MOUSE_REGION gMJLadderBackRegion;
static GUIButtonRef guiMJReportBtn;
static MOUSE_REGION gMJIconRegion[4]; // 0 home, 1 guestbook, 2 rules, 3 chat size
static BOOLEAN gfMJChatBig = FALSE;   // immersive mode: the bar takes the page
static UINT8 gubMJReportCount = 0; // how often a cheater has been reported
// session match history for the ladder page
struct MahjongMatchRecord { UINT16 day; INT32 net; UINT8 place; };
static std::vector<MahjongMatchRecord> gMJHistory;
static INT32 giMJChatScroll = 0; // 0 = pinned to the newest line
static std::string gMJInput;     // what the player is typing
static BOOLEAN gfMJCoach = FALSE; // /coach chat command: discard-hint marker

// a lurking spectator eventually leaves the room
static ST::string gMJPendingCameoLeave;
static UINT32 guiMJCameoLeaveTime = 0;

// a queued voice sample (e.g. Elliot's scripted reply)
static ST::string gMJPendingSound;
static UINT32 guiMJPendingSoundTime = 0;
static UINT32 guiMJVoiceBusyUntil = 0; // nobody talks over anybody else
static BOOLEAN gfMJSettled = TRUE; // stakes paid out for the current match?
static BOOLEAN gfMJLoan = FALSE;   // Kingpin fronted the buy-in this match
static UINT32 guiMJDeltaAnimStart = 0; // score count-up on the hand-over panel
static INT32 giMJLastNetGain = 0;      // dollars, shown on final standings
static UINT32 guiMJVisitorNo = 0;  // the proud hit counter

// session statistics, printed by the /stats chat command
static INT32 giMJStatMatches = 0;
static INT32 giMJStatMatchesWon = 0;
static INT32 giMJStatHandsWon = 0;
static INT32 giMJStatBiggestHand = 0;
static INT32 giMJStatDollars = 0;

// one pending chatbot reply, delivered after a "typing" delay
static int giMJBotWho = -1;
static ST::string gMJBotLine;
static UINT32 guiMJBotDueTime = 0;

// players: 0 you, 1 right (Enrico), 2 across (Deidranna), 3 left (rotating)
// the left seat rotates between Elliot and San Mona guests by game day
enum MahjongPersona { MJP_ELLIOT = 0, MJP_KINGPIN, MJP_TONY };
static int giMJSeat3Persona = MJP_ELLIOT;      // session-lifetime
static BOOLEAN gfMJExhibition = FALSE;         // The House plays seat 0 while idle
static const ProfileID gMJPersonaProfile[3] = { ELLIOT, KINGPIN, TONY };
static const char* const gMJPersonaName[3] = { "Elliot", "Kingpin", "Tony" };
// their 1999 handles, shown under the clear names
static const char* const gMJPersonaHandle[3] = { "@e11iot", "@the_house", "@no_refunds" };

// seat 2 (across): the Queen most nights - but she runs a country, so San
// Mona regulars sit in on her off days
enum MahjongSeat2Persona { MJP2_QUEEN = 0, MJP2_DARREN, MJP2_LAYLA };
static int giMJSeat2Persona = MJP2_QUEEN;
static const ProfileID gMJSeat2Profile[3] = { QUEEN, DARREN, MADAME };
static const char* const gMJSeat2Name[3] = { "Deidranna", "Darren", "Layla" };
static const char* const gMJSeat2Handle[3] = { "@dejdranna666", "@ringside_d", "@shady_lady" };
static const char* const gMJSeat2Short[3] = { "@666", "@darren", "@layla" };
// seat 4 is nobody's seat: the parlour's player of the month, dropping in
static ST::string gMJVisitorHandle;   // kept after they leave: the log needs it
static SGPVSurface* guiMJVisitorSurf = nullptr;
static UINT32 guiMJVisitorLeaves = 0;
static BOOLEAN gfMJVisitorHere = FALSE;

static const char* MahjongSeatHandle(int seat)
{
	switch (seat)
	{
		case 1:  return "@heir2throne";    // Enrico
		case 2:  return gMJSeat2Handle[giMJSeat2Persona];
		case 3:  return gMJPersonaHandle[giMJSeat3Persona];
		default: return "";
	}
}

// what a chat line is signed with: short handles keep the lines readable,
// the panels carry the full ones
static ST::string MahjongChatHandle(int who)
{
	if (who <= 0)
	{
		// you speak as yourself even when the House is warming your chair
		return gMJSelfNick.empty() ? ST::string("@you") : ST::format("@{}", gMJSelfNick.to_lower());
	}
	switch (who)
	{
		case 1:  return "@heir";
		case 2:  return gMJSeat2Short[giMJSeat2Persona];
		default: break;
	}
	if (who == 4) return gMJVisitorHandle.empty() ? ST::string("@guest") : gMJVisitorHandle;
	static const char* const shortPersona[3] = { "@e11iot", "@house", "@tony" };
	return shortPersona[giMJSeat3Persona];
}
static ProfileID gMJOpponentProfile[3] = { ENRICO, QUEEN, ELLIOT };
static const char* const gMJSeatNameFixed[4] = { "You", "Enrico", "Deidranna", "Elliot" };

static const char* MahjongSeatName(int seat)
{
	if (seat == 3) return gMJPersonaName[giMJSeat3Persona];
	if (seat == 2) return gMJSeat2Name[giMJSeat2Persona];
	if (seat == 0 && gfMJExhibition) return "House";
	return gMJSeatNameFixed[seat];
}

// seat 0 speaks in the second person, so third-person verbs shed their s
// ("You win", not "You wins") - unless the House is playing itself
static const char* MahjongVerb(int seat, const char* second,
		const char* third)
{
	return (seat == 0 && !gfMJExhibition) ? second : third;
}

// today's guest, deterministic by game day: Elliot most nights, Kingpin and
// Tony drop in on a rota
static int MahjongPersonaForToday()
{
	UINT32 const day = GetWorldDay();
	if (day % 4 != 3) return MJP_ELLIOT;
	return (day / 4) % 2 ? MJP_TONY : MJP_KINGPIN;
}

// the Queen's diary: Darren covers every fifth day plus two, Layla plus four
static int MahjongSeat2ForToday()
{
	UINT32 const day = GetWorldDay();
	if (day % 5 == 2) return MJP2_DARREN;
	if (day % 5 == 4) return MJP2_LAYLA;
	return MJP2_QUEEN;
}

// forward declarations (definitions live further down with their friends)
static void MahjongBotQueueReply(int who, const char* line, UINT32 hash);
static bool MahjongInvitationalToday();
static int MahjongStakeMultiplier();
static SGPVObject* MahjongLoadBigFace(ProfileID id);
// suit 0 = characters (wan), 1 = dots (tong), 2 = bamboo (tiao)
static const char* const gMJSuitName[3] = { "Chars", "Dots", "Bams" };

// --- table chat ---------------------------------------------------------
struct MahjongChatLine
{
	INT8 who; // 1..3 = opponent, 0 = you (unused for now)
	ST::string text;
};
static std::vector<MahjongChatLine> gMJChat;

static const char* const gMJChatGreet[3][12] =
{
	{ "gentlemen. and... whoever else is dialed in.", "another evening, another table. my father loved this game.",
	  "ah, my favourite opponents. mostly.", "we are civil at the tiles. nowhere else.", "from exile, good evening.",
	  "the modem held. that is already a victory.", "I have set aside the whole evening. I have little else set aside.",
	  "good evening from a rented room with excellent posture.", "let us pretend, for four hands, that nothing happened.",
	  "I dressed for this. nobody can see it, but I dressed for this.",
	  "my accountant advised against tonight. my accountant is not invited.",
	  "if I win I buy back one hectare. symbolically." },
	{ "I expect tribute, not resistance.", "sit. lose. thank me after.", "the Queen has arrived. bow or pay.",
	  "I play to remind you all of your place.", "Meduna's finest table. MY table.",
	  "I have cleared my evening. clearing things is what I do.",
	  "the palace is quiet tonight. I had it made quiet.",
	  "four hands. then I return to ruining something larger.",
	  "you may address me as Your Majesty. Elliot will demonstrate.",
	  "I am in a generous mood. it will pass before the deal.",
	  "someone has been talking about my rating. someone will stop.",
	  "let us begin. the mines do not extort themselves." },
	{ "hi everyone!! gl hf :)", "hii!! I practiced this time!!", "please be nice to me today :)",
	  "the palace modem is being weird again, sorry if I lag!!", "oh good, nobody brought guns this time :)",
	  "I finished the reports EARLY so I could be here!!", "hello!! I brought my lucky pencil!!",
	  "sorry sorry am I late?? I got lost in my own dungeon again",
	  "everyone please remember I am a person with feelings!! ok gl!!",
	  "hi!! I'm allowed one hobby and this is it!!",
	  "I read a strategy article!! it was mostly about breathing!!",
	  "good evening everyone!! (I have to say it that way now, orders)" },
};
static const char* const gMJChatIdle[3][30] =
{
	{ "hmm. a delicate position.", "in my day we played for estates.", "patience is a weapon too.",
	  "this table is the only place she and I still speak.", "somewhere out there my mercenaries are working. I hope.",
	  "I still read the Arulco Times. cover to cover.", "a king in exile is still a king at the table.",
	  "this felt is nicer than my apartment. do not tell anyone.",
	  "Miguel was my father's aide once. good man. terrible at tiles.",
	  "I fund an entire liberation and still lose pocket money here.",
	  "Speck writes me weekly about premiums. I drink before opening them.",
	  "the Chivaldori signet ring is the last thing I have not pawned.",
	  "my father Andreas taught me this game. patience, he said. patience.",
	  "Miguel Cordona ran against me in '88. now neither of us has a job.",
	  "in Milan I had a red smoking jacket for card nights. the pawnbroker has it now.",
	  "I dream of the Drassen airstrip. of all things. the airstrip.",
	  "one learns a great deal about people from how they discard.",
	  "there is a portrait of me in a palace hallway. I hope they dust it.",
	  "the hotel here charges by the week. I pay by the week.",
	  "I was told exile builds character. I had character. I wanted a country.",
	  "half my cabinet fled to Estoni. the useless half, thankfully.",
	  "when I am restored, this table gets a plaque. a small one.",
	  "my tailor still calls me Highness. he also still bills me.",
	  "the mines paid for my education. now they pay for her army.",
	  "I have three suits and four opinions left.",
	  "sometimes I forget I am the one who hired mercenaries. odd sensation.",
	  "they say the palace kitchens still make my mother's bread. probably a lie.",
	  "I keep a map of Arulco above the bed. it is not restful.",
	  "a gentleman does not sulk. a gentleman plays another hand.",
	  "the wall is like politics. you draw what is left." },
	{ "hurry UP, peasants.", "read the rules, cretins.", "I could have you all shot, you know.",
	  "faster. wars do not run themselves.", "I miss the days when losing to me was fatal.",
	  "somewhere a village is being renamed after me. it soothes.", "Elliot. POSTURE.",
	  "do NOT make me wait. it never ends well for anyone.",
	  "Elliot spilled coffee on my war maps this morning. MAPS. plural.",
	  "those SAM sites were EXPENSIVE. someone keeps breaking them.",
	  "Kingpin pays his San Mona taxes in bruises. but he pays.",
	  "I should double the head price on that rebel girl. Ira, was it.",
	  "we had winters in Romania that killed softer players than you.",
	  "I won my election with 98 percent. the other 2 percent are no longer with us.",
	  "red is MY colour. that is why the maps keep changing.",
	  "my generals asked for reinforcements. I sent them a photograph of me.",
	  "the peasants have started singing again. find out what about.",
	  "I do not sleep. I merely stop supervising for a few hours.",
	  "the throne is uncomfortable. that is the point of a throne.",
	  "someone in Cambria is printing pamphlets. someone in Cambria will stop.",
	  "I had a husband once. I have a country now. the trade favours me.",
	  "Elliot, remind me why I keep you. no. do not. it will be worse.",
	  "the loyalty of an army is purchased monthly. like a bad magazine.",
	  "when this is over I shall have the whole coast repainted.",
	  "my portraitist made me look kind. he is no longer a portraitist.",
	  "every mine I hold is a vein I have opened. poetic, no?",
	  "there is a rumour I inherited nothing. correct. I took it.",
	  "power is simply the shortest distance between wanting and having.",
	  "I have outlived two prophecies and one husband.",
	  "Deidranna Reitman. say it properly or do not say it." },
	{ "sorry sorry, still sorting!", "is it my turn? oh no.", "please nobody need my tiles...",
	  "the Queen is staring at me through the screen. I can FEEL it.", "I should be filing reports right now...",
	  "brb someone is yelling for me!!", "I get one nice evening a week and this is it!!",
	  "did everyone see me not lose last hand?? progress!!",
	  "I alphabetized the dungeon paperwork today! don't ask by what.",
	  "Joe the guard says hi! he's the only one here who's nice to me.",
	  "the Queen threw an ashtray at me today. a SMALL one though :)",
	  "I used to work at a bank. a BANK. with NORMAL people.",
	  "the Queen keeps a photo of Romania on her desk. she yells at it. in Romanian.",
	  "Darren invited me to watch the boxing once. I hid in the supply closet.",
	  "wearing my lucky red socks today!! both of the lucky ones!!",
	  "I have a desk now!! it's in a hallway but it's MINE!!",
	  "does anyone else practice saying 'yes, Your Highness' in the mirror?? just me??",
	  "I typed 'Dear Deidranna' once. ONCE.",
	  "my performance review said 'present'. that's it. just present.",
	  "the palace has 400 rooms and I am not allowed in 397 of them",
	  "I saved a moth from the war room today. big day for us both!!",
	  "sometimes I hide in the archive and just... breathe. it's nice down there!!",
	  "I know where all the bodies are filed!! FILED. alphabetically!!",
	  "if anything happens to me my cactus is on the third windowsill",
	  "I asked for a raise once. I now have a smaller desk!!",
	  "everyone at the palace has a nickname for me. none of them are Elliot.",
	  "I've been practicing my chess. it's like this but sadder!!",
	  "one day I'll retire somewhere flat and quiet with no queens in it",
	  "the good news is nobody shoots at the person holding the clipboard!!",
	  "I'm not nervous!! this is just how my hands are!!" },
};
static const char* const gMJChatOnHumanWin[3][12] =
{
	{ "well played. I mean that sincerely.", "a clean win. my compliments.", "the student surpasses the table.",
	  "good. spend it on ammunition.", "my money is well invested in you, it seems.",
	  "that is how my father used to close a hand.", "beautiful. cruel, but beautiful.",
	  "I would applaud but the modem would drop.", "you play like someone with a country to take.",
	  "take it. take all of it. just keep going north.",
	  "I have lost to worse people for smaller reasons.",
	  "note the hand. I intend to steal it later." },
	{ "CHEATER. I demand a recount.", "lag. that was LAG.", "enjoy it. it will not happen twice.",
	  "first my country, now my tiles. INSUFFERABLE.", "Elliot, note their name. the list, Elliot.",
	  "the wall was tampered with. I can feel it.", "beginners' luck. sustained, statistically improbable luck.",
	  "you will regret being memorable.", "I have had generals executed for less impressive work.",
	  "fine. FINE. deal again.", "somewhere a mine just became more expensive for you.",
	  "savour it. I am extremely patient and extremely well armed." },
	{ "wow!! can you teach me??", "that was so cool!!", "I knew you could do it!! (I lost tho)",
	  "at least SOMEBODY here is allowed to win :)", "quick, win again before she says anything!!",
	  "I'm writing that hand down!! for science!!", "see, THIS is why I watch instead of think!!",
	  "amazing!! don't look at her face right now!! don't!!",
	  "if I clap does that count as treason??", "you're my favourite player and I will deny saying that",
	  "that was better than anything that happens at my job!!",
	  "please win the war too!! sorry!! ignore that!!" },
};
static const char* const gMJChatWallLow[3][8] =
{
	{ "the wall runs thin, friends.", "few tiles remain. choose with care.", "the endgame. always my favourite.",
	  "we are down to the last honest tiles.", "whatever is left is what we deserve.",
	  "this is where fortunes and countries are decided. usually badly.",
	  "hold something back. always hold something back.",
	  "the wall empties like a treasury. I would know." },
	{ "end this FARCE already.", "the wall dies. like everything else around me.", "someone win before I have the wall executed.",
	  "if this ends in a draw I am blaming Elliot.", "so few tiles. so many disappointments.",
	  "the wall is nearly gone and NOBODY has suffered enough.",
	  "finish. I have a country to mismanage.",
	  "we are scraping the bottom. how familiar." },
	{ "maybe nobody wins? that'd be ok...", "almost over!! nobody panic!!", "wall's almost gone. phew.",
	  "counting the tiles makes me anxious!! so I stopped!!",
	  "is it bad that I'm relieved?? it's bad isn't it",
	  "everyone stay calm!! I said everyone!! including me!!",
	  "so close to a draw!! my best result!!",
	  "the wall's nearly empty and so is my hand!! matching!!" },
};
// contextual table talk: what the AIs blurt out about their own hands.
// Elliot always leaks his tenpai - that is the table's one honest tell.
static const char* const gMJChatTenpai[3][10] =
{
	{ "the wind changes, friends.", "hm. suddenly this evening improves.", "careful with your discards now.",
	  "I would hold your tiles a little tighter, all of you.",
	  "a door has opened. I shall be polite about it.",
	  "one tile. one small, obedient tile.",
	  "do not be alarmed. be careful, but not alarmed.",
	  "I feel the way I felt the night before the election.",
	  "somebody is about to fund my dinner.",
	  "gentlemen. and lady. brace yourselves." },
	{ "I can taste it. Someone is about to PAY.", "one little tile and one of you SUFFERS.", "come now. feed me.",
	  "the noose is tied. who walks into it?",
	  "I am one tile from a very good mood.",
	  "somebody in this room is about to fund a battalion.",
	  "hold your breath. I intend to enjoy this.",
	  "the wall and I have reached an understanding.",
	  "discard carefully. or do not. I am flexible about your suffering.",
	  "this is the part where I stop being patient." },
	{ "oh!! oh!! I just need ONE more tile!!", "guys I'm SO close!! oh no I said that out loud", "one tile!! just one!! (oops)",
	  "I'm waiting on something!! I mean- forget I said that!!",
	  "nothing is happening!! nothing at all!! definitely not tenpai!!",
	  "if I win I'm going to faint. fair warning!!",
	  "my hands are shaking!! that's normal right??",
	  "one tile!! ONE!! I said it again!!",
	  "I have never been this close to anything in my life",
	  "please don't look at my discards. please." },
};
static const char* const gMJChatBadHand[3][10] =
{
	{ "a garden of weeds, this hand.", "some evenings the tiles simply refuse.",
	  "I have commanded armies with less discipline than this hand.", "even Speck would not insure this hand.",
	  "this hand has the coherence of my cabinet.",
	  "I have been dealt worse. in 1992. by my wife.",
	  "there is no plan here. only architecture I did not approve.",
	  "the tiles have voted. against me. as usual.",
	  "one plays the hand one is given. loudly, in my case.",
	  "if this were a country I would flee it. again." },
	{ "who DEALT this garbage?", "these tiles are an INSULT.", "I should have the dealer flogged.",
	  "my army gets better equipment than my tiles. and my army gets NOTHING.",
	  "this hand is treason in ceramic form.",
	  "somebody has arranged this. somebody will be arranged in turn.",
	  "I did not seize a nation to be handed THIS.",
	  "the wall has made an enemy tonight.",
	  "Elliot. did you touch the shuffler.",
	  "this is what disloyalty looks like as a hand." },
	{ "my tiles are awful... as usual :(", "does anyone else just... never draw anything good?",
	  "I think my modem is cursed", "these tiles are worse than my performance reviews",
	  "I'd complain but that's how I got the dungeon job",
	  "this hand is a metaphor and I hate it",
	  "13 tiles and not one of them likes me",
	  "I'm going to pretend this is a strategy!!",
	  "it's fine!! I'm used to it!! that's the sad part!!",
	  "somewhere out there is a good hand. I hope it's happy." },
};
static const char* const gMJChatVoidDraw[3][10] =
{
	{ "fate hands me stones.", "and again the dead suit finds me.", "the wall has a sense of humour.",
	  "exiled from my country, and now from this suit.",
	  "the one suit I renounced. it follows me like creditors.",
	  "I declared it void. the wall declared otherwise.",
	  "useless. beautiful, but useless. like my title.",
	  "another dead tile for the collection.",
	  "I renounce things and they keep arriving. story of the decade.",
	  "the wall is testing my manners." },
	{ "ANOTHER dead tile. Insufferable.", "the wall mocks me. the WALL. mocks ME.", "dead tiles for a Queen. outrageous.",
	  "whoever shuffled this will answer for it.",
	  "I VOIDED that suit. the wall is being insubordinate.",
	  "this is the third one. I am counting. I always count.",
	  "worthless. I have executed men for being less worthless.",
	  "the wall will be re-educated after this hand.",
	  "give me anything else. ANYTHING.",
	  "I declared that suit dead and it keeps breathing." },
	{ "aww not again...", "why do I keep drawing these :(", "the wall hates me personally",
	  "I swear the wall knows which suit I dropped",
	  "of course. of COURSE.",
	  "that's the fourth one!! I'm not even mad, I'm impressed!!",
	  "should I keep it as a souvenir?? no?? ok",
	  "I void a suit and the wall takes it personally",
	  "this is exactly like my career!!",
	  "dead tile. right on schedule." },
};
// war-aware table talk: lines keyed to actual campaign progress
// (0 = early war, 1 = mid, 2 = late)
// two lines per war tier: [tier*2] and [tier*2 + 1]
// four lines per war tier: [tier*4 .. tier*4+3]
static const char* const gMJChatWar[3][12] =
{
	{ "my investment shows promise. keep it up.", "Drassen, Omerta... names from my youth. take them back.",
	  "the first towns are the hardest. so I am told, from here.",
	  "I sent money and hope. mostly money.",
	  "Arulco stirs, friend. I feel it even from here.", "half a country reclaimed. my father would have wept.",
	  "the mines are changing hands. she will feel that in her ledgers.",
	  "the newspapers have started printing my name again. cautiously.",
	  "we are close now. so close I can smell the palace dust.", "when Meduna falls, drinks at this table are on me.",
	  "I have begun writing a speech. I have rewritten it nine times.",
	  "one more push. I have a suit pressed and nowhere to wear it yet." },
	{ "your little uprising amuses me. for now.", "a few dusty villages. keep them. they smell.",
	  "I did not notice. I was told to notice, so I noticed.",
	  "a rebellion is just an unpaid invoice with guns.",
	  "you are becoming an IRRITATION.", "take my mines and I raise my ARMY. see how that ends.",
	  "my generals now knock before entering. that is your doing.",
	  "I have started reading the casualty lists myself. how quaint.",
	  "enjoy the table. it is the LAST thing you will take from me.", "come to Meduna, then. I will be waiting. with everything.",
	  "the palace windows are bricked. for the draft. only the draft.",
	  "you will find I am considerably worse when cornered." },
	{ "things are pretty calm at the palace! mostly!", "some rebels landed somewhere! probably nothing!!",
	  "I filed a report about it! she filed it back at me!",
	  "everything is FINE. that's the official position!!",
	  "the Queen has been... tense lately. very tense.", "we lost ANOTHER town today. I had to read the report out loud.",
	  "I've started keeping my coat by the door!! no reason!!",
	  "morale is high!! mine isn't but morale is!!",
	  "please win quickly, the palace is NOT a fun place right now.", "the palace staff are all updating their resumes. all of them.",
	  "I've memorized every exit. as a hobby!!",
	  "if anyone needs a diligent clerk after all this, I'm just saying" },
};

// guest personality packs: category lines for the rotating left seat.
// index [persona-1]: 0 = Kingpin, 1 = Tony (Elliot uses the main tables)
struct MahjongGuestPack
{
	const char* greet[3];
	const char* idle[3];
	const char* war[3];
	const char* tenpai[3];
	const char* badhand[3];
	const char* onHumanWin[3];
	const char* claim[3];
	const char* winTaunt[3];
	const char* ronVictim[3];
};
static const MahjongGuestPack gMJGuestPack[2] =
{
	{ // Kingpin: the owner, playing at his own establishment
		{ "evening. I own this table. and the felt. and the modem.", "sit down, play clean, nobody gets hurt.", "the Queen, the exile, and my accountant's nightmare. perfect." },
		{ "take your time. the vig doesn't.", "I count tiles the way I count debts. always.", "Darren says hi. he doesn't. sit still." },
		{ "war's good for business. bad for everything else.", "half of Arulco owes me money. the winning half.", "whoever wins this war better honour my markers." },
		{ "one tile out. start counting your cash.", "the house edge just got personal.", "I can smell a finished hand from Cambria." },
		{ "even I get dealt garbage. difference is I don't whine.", "somebody stacked this wall wrong. find him.", "bad tiles. good thing I own the till." },
		{ "not bad. I've broken thumbs for less skill.", "you win at MY table. savour that.", "take the pot. remember where it came from." },
		{ "that's mine. everything here is.", "claimed. house rules.", "hand it over." },
		{ "Kingpin: \"the house always wins. tonight I AM the house.\"", "Kingpin: \"pot's mine. drinks are not on me.\"", "Kingpin: \"like taking candy. profitable candy.\"" },
		{ "Kingpin: \"appreciate the donation. Darren, log it.\"", "Kingpin: \"you handed me that. bad instinct. costly too.\"", "Kingpin: \"tsk. never show me your money.\"" },
	},
	{ // Tony: the arms dealer, strictly business
		{ "tony here. cash games only, same as guns.", "let's make this quick, I got inventory coming in.", "no refunds at this table either, friends." },
		{ "tick tock. time is ammunition.", "I've seen faster reloads from rusted AKs.", "somebody discard, I got a shipment at dawn." },
		{ "war's been GREAT for my margins.", "both sides buy from me. beautiful arrangement.", "whoever wins, they'll still need ammo. I'm fine." },
		{ "locked and loaded over here.", "one more tile and this deal closes.", "safety's off, people." },
		{ "this hand's worth less than surplus 9mm.", "defective merchandise, this hand.", "I've sold better hands for scrap." },
		{ "clean shot. respect.", "you'd make a decent negotiator.", "fine. discount stays theoretical though." },
		{ "sold! to me.", "I'll take that off your hands. no refunds.", "consider it repossessed." },
		{ "Tony: \"deal closed. pleasure doing business.\"", "Tony: \"that's how you liquidate assets, people.\"", "Tony: \"cash on the table. always cash.\"" },
		{ "Tony: \"you just paid my restocking fee.\"", "Tony: \"careless. in my trade that costs fingers.\"", "Tony: \"thanks for the margin, friend.\"" },
	},
};

// seat-3 line lookup: Elliot uses the main character tables, guests their packs
static const char* MahjongSeat3Line(const char* const elliotLine, const char* const* guestLines, UINT32 roll)
{
	if (giMJSeat3Persona == MJP_ELLIOT) return elliotLine;
	return guestLines[roll % 3];
}

static const MahjongGuestPack* MahjongGuest()
{
	return giMJSeat3Persona == MJP_ELLIOT ? nullptr : &gMJGuestPack[giMJSeat3Persona - 1];
}

// stand-ins for the Queen's chair: Darren the ring manager, Madame Layla
static const MahjongGuestPack gMJSeat2Pack[2] =
{
	{ // Darren van Haussen: polite muscle, house rules
		{ "evening. the Queen is busy. you get me instead.", "Darren, filling in. house rules apply. all of them.", "I run the ring on D5. tiles are quieter. usually." },
		{ "in boxing you can at least see the punches coming.", "Kingpin watches every match. assume he watches this too.", "no wagering against the house. so wager WITH it." },
		{ "your war is good for the fight business. keep it up.", "the Queen cancels on us more since you landed.", "whoever wins out there, the ring stays open." },
		{ "one more and I collect.", "the round is nearly mine. no hard feelings.", "watch your discards now. friendly advice." },
		{ "this hand belongs on the third undercard.", "I have seen better tiles in the lost and found.", "even Spike draws better. and Spike cannot read." },
		{ "clean win. the house respects clean.", "not bad. try the ring sometime. two-to-one payout.", "noted. Kingpin will hear about you." },
		{ "that one works for me.", "I will take that. house privilege.", "mine. nothing personal." },
		{ "the house always finishes standing.", "that is the round. pay the man.", "boxing money and tile money spend the same." },
		{ "you clipped me. it happens.", "that one got through my guard.", "point taken. literally." },
	},
	{ // Madame Layla: the Shady Lady plays for rent money
		{ "evening, sugar. Her Majesty cancelled. you're stuck with me.", "Layla. yes, THAT Layla. deal me in.", "the Lady runs itself tonight. I'm here to win rent money." },
		{ "Billy watches the door, so I can stay all night.", "half of San Mona owes me money. the other half owes me favours.", "I hear everything in this town, sugar. EVERYTHING." },
		{ "soldiers on leave tip better when they're losing a war.", "business is up since you landed, sugar. keep shooting.", "the Queen's boys don't come around anymore. shame. they tipped." },
		{ "almost there, sugar. don't make it easy for me.", "one tile, and mama makes rent.", "I can smell a win coming. it smells like perfume." },
		{ "these tiles are uglier than Billy's mug. don't tell him.", "I've thrown men out for less than this hand.", "somebody shuffled this wall with their eyes closed." },
		{ "well played, sugar. first drink's on MY house.", "you win like someone who practices. suspicious.", "Maria says nice things about you. now I see why." },
		{ "that's mine now, sugar.", "hand it over, I've got plans for it.", "the Lady takes what the Lady needs." },
		{ "rent money, sugar. nothing personal.", "mama wins. mama always wins eventually.", "that's the round. kisses." },
		{ "ouch, sugar. right in the purse.", "you got me. it happens once a decade.", "noted. no discount for you." },
	},
};

// Darren plays tight, Layla plays loose; the Queen stays sharp
static int MahjongSeat2ErrorRate()
{
	static int const rate[3] = { 15, 20, 28 };
	return rate[giMJSeat2Persona];
}

static const MahjongGuestPack* MahjongSeat2Guest()
{
	return giMJSeat2Persona == MJP2_QUEEN ? nullptr : &gMJSeat2Pack[giMJSeat2Persona - 1];
}

// central routing: any seat with a stand-in speaks from that pack
static const MahjongGuestPack* MahjongPackFor(int seat)
{
	if (seat == 3) return MahjongGuest();
	if (seat == 2) return MahjongSeat2Guest();
	return nullptr;
}

static int MahjongWarTier()
{
	UINT8 const progress = CurrentPlayerProgressPercentage();
	return progress >= 60 ? 2 : progress >= 20 ? 1 : 0;
}

// the Chivaldoris: she framed him, took his country, and now they share a
// table. lines that only fire between the estranged spouses.
static const char* const gMJSpouseEnricoRonsQueen[6] =
{
	"Enrico: \"you always did discard too soon, my dear.\"",
	"Enrico: \"thank you, darling. for once.\"",
	"Enrico: \"twenty years and you still lead with your temper.\"",
	"Enrico: \"I take this one for the palace. and the dog you rehomed.\"",
	"Enrico: \"a small country's worth of satisfaction, that.\"",
	"Enrico: \"you fed me. how very like our marriage.\"",
};
static const char* const gMJSpouseQueenRonsEnrico[6] =
{
	"Deidranna: \"I took your country. the tile is a formality.\"",
	"Deidranna: \"you were always careless with what was yours, darling.\"",
	"Deidranna: \"and THAT is why I manage the family assets now.\"",
	"Deidranna: \"you hand things to me. it is your one talent.\"",
	"Deidranna: \"another loss, my love. do keep a ledger.\"",
	"Deidranna: \"I would say it was close. it was not.\"",
};
// idle bicker exchanges: opener + comeback, delivered via the reply queue
struct MahjongBicker { const char* open; int replyWho; const char* reply; };
static const MahjongBicker gMJBicker[30] =
{
	{ "you never played like this when we were married.", 2, "when we were married you never NOTICED how I played." },
	{ "does the palace still leak when it rains, my dear?", 2, "does exile still suit you, my dear?" },
	{ "I could have forgiven almost everything, you know.", 2, "how fortunate that I never asked." },
	{ "my father taught me this game at that very palace.", 2, "and now the palace teaches ME. poetic." },
	{ "Elliot, blink twice if she is making you play.", 3, "haha! what! no!! (help)" },
	{ "you married me for the election, admit it.", 2, "I married you for Arulco, darling. the election was gravy." },
	{ "Miguel sends his regards, by the way.", 2, "tell Cordona his file is still on my desk." },
	{ "Elliot. how is the palace plumbing lately?", 3, "sir we agreed never to talk about the plumbing." },
	{ "your father promised me a mining empire, my dear.", 2, "and MY father promised me a man with a spine." },
	{ "the Drassen miners sang for us at the wedding. remember?", 2, "they sing for whoever holds the payroll. I checked." },
	{ "one day Arulco will remember the Chivaldori name.", 2, "it does. the old currency still has your nose on it." },
	{ "half a fortune says you cannot win the next hand.", 2, "keep the fortune. I will take the other half of Arulco." },
	{ "I gave you a crown, my dear. you gave me a suitcase.", 2, "packed by MY staff. you are welcome." },
	{ "you have my mother's ring, I notice.", 2, "I have your mother's country. the ring came with it." },
	{ "did you ever love the man or only the address?", 2, "the address had better lighting." },
	{ "the roses at the west wing were mine, you know.", 2, "they were dug up. they were sulking. like their owner." },
	{ "you cannot govern a country by fear alone.", 2, "correct. one also needs a budget. I have both." },
	{ "Elliot, does she still hum when she signs the warrants?", 3, "I really shouldn't answer that!! ...yes." },
	{ "I still have the wedding photographs.", 2, "so do I. I use them for target practice." },
	{ "we were happy once. briefly. on a Tuesday.", 2, "Wednesday. and you were asleep for most of it." },
	{ "you always did overplay a strong position.", 2, "and you always did fold one before the reveal." },
	{ "your generals despise you, my dear.", 2, "my generals are paid to despise me quietly." },
	{ "Elliot, do you get holidays?", 3, "I get an hour!! on my birthday!! if there's no coup!!" },
	{ "I dreamt of the palace kitchens last night.", 2, "they are a barracks now. the ovens still work." },
	{ "you were softer before the crown.", 2, "the crown did not change me. it merely fit." },
	{ "there is still a chair for you at my table, Deidranna.", 2, "there is still a cell for you at mine, Enrico." },
	{ "the people will choose, in the end.", 2, "the people will be TOLD, in the end." },
	{ "Elliot, has she read my letters?", 3, "she reads them aloud!! in a voice!! sorry!!" },
	{ "one of us will be remembered kindly.", 2, "yes. and history is written in Meduna." },
	{ "we could have been something, you and I.", 2, "we were. briefly. it is called a transaction." },
};

// every profile needs a bio. this is 1999 and everyone is a brand
static const char* const gMJBio[20] =
{
	"A.I.M. certified. ask about my hourly",
	"available for work. not for feelings",
	"i shoot things. i also know things",
	"currently deployed. do not ask where",
	"here for the felt. staying for the felt",
	"between contracts. between everything",
	"i do not lose. i redeploy",
	"references available. survivors fewer",
	"tiles by night. jungle by day",
	"my medical is current. is yours",
	"turning down work to be here",
	"ask me about my kill/deal ratio",
	"semi-retired. mostly semi",
	"i bill by the hour and the hand",
	"three languages. one of them polite",
	"i have been shot in four countries",
	"will work for tiles",
	"discretion guaranteed. results negotiable",
	"do not add me. i will find you",
	"licensed, bonded, and slightly insured",
};

static const char* MahjongSeatBio()
{
	if (gfMJExhibition) return gMJBio[gubMJShillPid % 20];
	if (LaptopSaveInfo.fIMPCompletedFlag) return "I.M.P. graduate. licensed to lose money";
	return "guest account. no rating, no shame";
}

// Yahoo-style ladder ratings: yours derives from your persisted record,
// the regulars' are house numbers with a small daily wobble
static INT32 MahjongPlayerRating()
{
	MahjongPersist const mj = MahjongGetPersist();
	INT32 r = 1200 + mj.usMatchesWon * 35 + mj.usHandsWon * 6
			- (mj.usMatches - mj.usMatchesWon) * 15;
	r += std::max(-100, std::min(200, mj.iDollarsNet / 500));
	return std::max(800, r);
}

static bool MahjongRatingProvisional()
{
	return MahjongGetPersist().usMatches < 3;
}

static INT32 MahjongSeatRating(int seat)
{
	static INT32 const regular[3] = { 1687, 2141, 1104 }; // Enrico, the Queen (disputed), Elliot
	static INT32 const guest[3]   = { 1104, 1899, 1521 }; // Elliot, Kingpin, Tony
	static INT32 const seat2[3]   = { 2141, 1774, 1490 }; // the Queen, Darren, Layla
	INT32 const base = seat == 3 ? guest[giMJSeat3Persona]
			: seat == 2 ? seat2[giMJSeat2Persona] : regular[seat - 1];
	return base + static_cast<INT32>(GetWorldDay() * (seat * 7 + 3) % 23) - 11;
}

// house PA system: service notices, patch notes, legal disclaimers
static const char* const gMJService[] =
{
	"The House reminds you: all results are final. NO REFUNDS",
	"the technician who broke table 2 has been dealt with. table 2 is fine",
	"the rake is 10%. counting the chips twice costs extra",
	"we apologize for the lack of umlauts. the font budget went to the neon sign",
	"lost connection? the modem answers to 'Bessie'. be gentle",
	"Spike is not a valid dispute resolution mechanism",
	"the parlour accepts: cash, gold teeth, SAM site access codes",
	"tonight's special at Frank's: the usual",
	"do not tap on the webcams. she can see you",
	"Parlour v0.9.7: fixed a bug where Elliot could win",
	"UPDATE: tiles are now 3% shinier. you're welcome",
	"feature requests may be submitted at the manor, in writing. bring a will",
	"known issue: the Queen. no fix planned",
	"jukebox out of order since the '89 incident. do not ask about the '89 incident",
	"@dejdranna666 - abandoned games: 0 (disputed)",
	"the Beginner Room remains under construction. Mr. Klaus sees no profit in mercy",
	"Y2K compliance check: the parlour is ready. Bessie is not",
	"best viewed in sir-FER 4.0 at 640x480. no exceptions",
	"reminder: the felt is leased. treat it like it is someone else's. it is",
	"our server lives above the Shady Lady. do not ask about the humidity",
	"a guest requested a quieter table. a quieter table has been arranged for him",
	"the house cat is called Ante. Ante does not take sides",
	"management thanks the Drassen airport for its continued unwitting hospitality",
	"new this week: losing streaks now come with a complimentary shrug",
	"we do not host tournaments. tournaments imply somebody else could win",
	"players are reminded that the ceiling fan is load-bearing",
	"membership dues are voluntary the way breathing is voluntary",
	"tipping the dealer is encouraged. the dealer is a modem",
	"the parlour operates under Arulcan gaming law. we wrote it",
	"complaints department hours: never. location: unmarked",
	"tonight's dealer has been playing since before some of you were solvent",
	"we have never had a fire. we have had three incidents involving fire",
	"the coat check is Darren. Darren is also the coat",
	"our lawyer says we cannot promise fair play. our owner says we can",
	"the vending machine takes chips and gives nothing back. it is thematic",
	"any resemblance to persons living or recently deceased is coincidental",
	"winning too often is not against the rules. it is against the spirit",
	"proudly sponsored by nobody. we asked. they declined",
	"if the felt flickers that is the felt, not your screen",
	"we are hiring. the position is 'sitting very still'. see Spike",
	"lost items are held 24 hours, then become house property. as is everything",
	"thank you for choosing San Mona. there were no other choices",
};
#define MJ_SERVICE_COUNT (sizeof(gMJService) / sizeof(gMJService[0]))

// rare spectators: the room has lurkers
struct MahjongCameo { const char* join; int reactWho; const char* react; const char* leave; };
static const MahjongCameo gMJCameo[18] =
{
	{ "@miguel_c has joined the room", 2, "WHO let the rebel in here? BAN him!", "@miguel_c has left the room" },
	{ "@m1ke has joined the room", 3, "...nobody type anything.", "@m1ke has left the room" },
	{ "@skyrider has joined the room", 1, "not the helicopter rates again, please.", "@skyrider has left the room" },
	{ "@icecreamtruck has joined the room", 3, "oh! I love that truck!!", "@icecreamtruck has left the room" },
	{ "@speck1185 has joined the room", 1, "no, Speck. I will NOT bundle the premiums.", "@speck1185 has left the room" },
	{ "@madamelayla has joined the room", 3, "I'm not allowed on that street!! officially!!", "@madamelayla has left the room" },
	{ "@gabby has joined the room", 1, "ah, the chemist. my condolences to your nose.", "@gabby has left the room" },
	{ "@fatherwalker has joined the room", 3, "oh good!! someone pray for my tiles!!", "@fatherwalker has left the room" },
	{ "@ira_r has joined the room", 2, "the little rebel girl. how BRAVE of her.", "@ira_r has left the room" },
	{ "@dr_q has joined the room", 1, "the doctor charges by the syllable. do not greet him.", "@dr_q has left the room" },
	{ "@bobbyray has joined the room", 1, "no, I do not need a crate of grenades tonight.", "@bobbyray has left the room" },
	{ "@hamous has joined the room", 3, "IS THAT THE TRUCK MAN!! hi!!", "@hamous has left the room" },
	{ "@brenda_d has joined the room", 1, "the shopkeeper. she has seen things. all of them.", "@brenda_d has left the room" },
	{ "@carmen_bh has joined the room", 2, "a bounty hunter. how many of my subjects has he priced?", "@carmen_bh has left the room" },
	{ "@spike has joined the room", 3, "...I am going to sit very still now.", "@spike has left the room" },
	{ "@angel_leather has joined the room", 1, "good man. bad luck. worse brother-in-law.", "@angel_leather has left the room" },
	{ "@thebiggun has joined the room", 2, "if that is who I think it is, someone is being measured.", "@thebiggun has left the room" },
	{ "@krott has joined the room", 3, "he sells things that BITE!! not tonight please!!", "@krott has left the room" },
};


// what a claimer blurts out when they pong/kong (used sparingly)
static const char* const gMJChatClaim[3][8] =
{
	{ "excuse me - I require that.", "I will be taking that, thank you.", "a gentleman never wastes a tile.",
	  "with apologies. and interest.", "that belongs to the restoration effort now.",
	  "you were finished with it. I checked.", "one takes what one is offered. eventually.",
	  "my compliments on the timing. mine, not yours." },
	{ "MINE.", "yes. bring it to me.", "everything on this table is mine eventually.",
	  "confiscated.", "thank you for the tribute.",
	  "I annex that.", "the crown claims it. the crown always claims it.",
	  "careless. and now expensive." },
	{ "oh! I can use that!!", "mine mine mine!! sorry!!", "grabby hands, sorry!!",
	  "was that ok?? that was allowed right??", "I did a thing!! a GOOD thing!!",
	  "sorry sorry but I needed it!!", "look at me making decisions!!",
	  "please don't be mad!! it was RIGHT THERE!!" },
};

static const char* const gMJWinTaunt[4][8] =
{
	{ "MAHJONG! The table pays up.", "Mahjong. Collect your winnings.", "That's the hand. Pay the man.",
	  "Mahjong! Nobody move.", "Mahjong. Just like that.", "Mahjong - and the chips come home.",
	  "That'll do. Mahjong.", "Mahjong. The House says nothing. The House never does." },
	{ "Mahjong. A small restoration.", "Mahjong! One province at a time.",
	  "Mahjong - my compliments to the wall.", "Mahjong. My father would have approved.",
	  "Mahjong! The exile still has teeth.", "Mahjong. Consider it a down payment.",
	  "Mahjong - dignity, briefly, restored.", "Mahjong! Tonight I dine properly." },
	{ "MAHJONG. Kneel.", "Mahjong. As arranged.", "Mahjong! Pay me and be grateful.",
	  "Mahjong. Predictable. Necessary.", "MAHJONG. Do not make that face.",
	  "Mahjong - the crown collects.", "Mahjong. I did warn you.",
	  "MAHJONG. Elliot, write it down." },
	{ "MAHJONG!! oh my GOD!!", "mahjong!! wait really?? MAHJONG!!",
	  "I won!! I actually- MAHJONG!!", "MAHJONG!! nobody take this from me!!",
	  "mahjong!!! I'm going to be sick!!", "MAHJONG! ...sorry for shouting!!",
	  "I did it!! MAHJONG!! me!!", "MAHJONG!! best day of my entire employment!!" },
};
static const char* const gMJRonVictimTaunt[4][8] =
{
	{ "The House thanks you for that tile.", "Fed by your own hand. Classic.",
	  "You handed it over. The House kept it.", "Careless. The House is grateful.",
	  "That discard cost you the hand.", "Right into it. Happens to everyone here.",
	  "The House would like to buy you a drink. With your money.",
	  "One tile. That is all it ever takes." },
	{ "Forgive me - you offered it.", "You gave me that. I merely accepted.",
	  "A gift. I shall not forget the giver.", "That tile had my name on it.",
	  "You threw it. I caught it. That is the game.",
	  "My thanks. Sincerely, if uselessly.",
	  "You will want that one back. You cannot have it.",
	  "The exile appreciates the donation." },
	{ "You FED me. How obedient.", "Thank you for the tile. And the humiliation.",
	  "You handed a Queen her win. Naturally.", "That was YOURS. Now it is MINE.",
	  "Careless. And I am never careless.",
	  "I did not even have to work for it.",
	  "Your discard. My hand. My table.",
	  "Do that again. I enjoy it." },
	{ "you gave me that!! sorry!! thank you!!", "oh no I won off YOUR tile!! sorry!!",
	  "that was so nice of you!! accidentally!!", "I feel terrible!! I'm keeping it though!!",
	  "sorry sorry sorry!! but MAHJONG!!", "your tile did that!! not me!! mostly!!",
	  "I'll never forget this!! neither will you!!",
	  "please still like me!! I won!! but please!!" },
};

static ST::string MahjongTileLabel(MahjongGame::TileId t)
{
	static const char* const suit[3] = { "m", "p", "s" };
	return ST::format("{}{}", t % 9 + 1, suit[t / 9]);
}


static UINT8 MahjongSuitColor(int suit)
{
	switch (suit)
	{
		case 0:  return FONT_MCOLOR_RED;
		case 1:  return FONT_DKBLUE;
		default: return FONT_MCOLOR_DKGRAY;
	}
}


// Tile rendering: blit from the generated STI sheets; fall back to a drawn
// tile with a text glyph if the sheets are missing. `voided` picks the
// red-tinted twin (sheet indices 28..54) for tiles of the player's void suit.
#define MJ_SHEET_VOID_OFFSET 28

static void MahjongDrawTile(INT32 x, INT32 y, INT32 w, INT32 h, MahjongGame::TileId t, bool outlined, bool voided = false)
{
	SGPVObject const* const sheet = w >= MJ_TILE_W ? guiMJTiles : guiMJTilesSmall;
	if (sheet)
	{
		BltVideoObject(FRAME_BUFFER, sheet, voided ? t + MJ_SHEET_VOID_OFFSET : t, x, y);
	}
	else
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + w, y + h, Get16BPPColor(FROMRGB(60, 60, 60)));
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 1, y + 1, x + w - 1, y + h - 1,
					Get16BPPColor(voided ? FROMRGB(242, 202, 196) : FROMRGB(238, 232, 213)));

		SGPFont const font = w >= MJ_TILE_W ? FONT12ARIAL : FONT10ARIAL;
		ST::string const label = MahjongTileLabel(t);
		INT16 sX, sY;
		FindFontCenterCoordinates(static_cast<INT16>(x), static_cast<INT16>(y), static_cast<INT16>(w), static_cast<INT16>(h), label, font, &sX, &sY);
		SetFontAttributes(font, MahjongSuitColor(MahjongGame::SuitOf(t)), NO_SHADOW, 0);
		MPrint(sX, sY, label);
		SetFontShadow(DEFAULT_SHADOW);
	}

	if (outlined)
	{
		UINT16 const red = Get16BPPColor(FROMRGB(220, 50, 50));
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + w, y + 1, red);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y + h - 1, x + w, y + h, red);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + 1, y + h, red);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x + w - 1, y, x + w, y + h, red);
	}
}


static void MahjongRedraw()
{
	fPausedReDrawScreenFlag = TRUE;
}


static UINT32 MahjongNow()
{
	return GetJA2Clock();
}


// subtle UI sounds, reusing vanilla switch/beep samples; discards are panned
// to the seat they come from
static void MahjongPlayVoice(const char* file, UINT32 vol, UINT32 pan, UINT32 estMs);

static BOOLEAN gfMJWarmup = FALSE; // fast-forwarding: no sounds

static void MahjongPlay(const char* file, UINT32 vol, UINT32 pan = MIDDLEPAN)
{
	if (gfMJWarmup) return;
	try
	{
		PlayJA2SampleFromFile(file, vol, 1, pan);
	}
	catch (...)
	{
		// missing sample: stay silent
	}
}


static UINT32 MahjongSeatPan(int player)
{
	switch (player)
	{
		case 1:  return RIGHTSIDE; // Enrico
		case 3:  return LEFTSIDE;  // Elliot
		default: return MIDDLEPAN;
	}
}

#define MJ_SND_SELECT   SOUNDSDIR "/very small switch 01 in.wav"
#define MJ_SND_DISCARD  SOUNDSDIR "/small switch 02 in.wav"
#define MJ_SND_AIDROP   SOUNDSDIR "/small switch 02 out.wav"
#define MJ_SND_DRAW     SOUNDSDIR "/very small switch 02 in.wav"
#define MJ_SND_DEAL     SOUNDSDIR "/very small switch 02 out.wav"
#define MJ_SND_ALERT    SOUNDSDIR "/computer beep 01 in.wav"
#define MJ_SND_WIN      SOUNDSDIR "/computer switch 01 in.wav"
#define MJ_SND_HANDEND  SOUNDSDIR "/computer switch 01 out.wav"
#define MJ_SND_MELD     SOUNDSDIR "/big switch 03 in.wav"


static UINT32 MahjongChatRoll();
static INT32 MahjongChatVisibleLines();
static INT32 MahjongRowHeight(std::size_t i);
static const char* MahjongRoleBadge(int who);
static void MahjongReconcileScroll();

// how fast the regulars type: slow, uneven, dial-up era hands
#define MJ_TYPE_MS	64

// a line somebody is currently typing; the room sees the indicator first
struct MahjongPendingLine { INT8 who; ST::string text; UINT32 dueTime; bool ghost; };
static std::vector<MahjongPendingLine> gMJPending;
static UINT32 guiMJQueueTail = 0; // when the last queued line will land
static UINT32 guiMJAwayUntil[5] = {}; // stepped away from the keyboard
// what each speaker has actually said lately, whole lines. the chat log
// holds burst fragments, so it cannot answer "have you said this before"
static std::vector<ST::string> gMJRecentSaid[5];
// moderation: Elliot has one duty at this table and he takes it seriously
static UINT32 guiMJMutedUntil = 0;
static UINT8 gubMJOffences = 0;
static UINT32 guiMJLastPost = 0;
static UINT8 gubMJRecentPosts = 0;
// scrolling: one continuous position in content pixels, plus the target it
// eases toward. content grows at the bottom (messages, typing row) and is
// trimmed at the top (history cap), so each mutation is reconciled once.
static INT32 giMJScrollY = 0;       // current, pixels from the top of content
static INT32 giMJScrollTarget = 0;  // where it is heading
static INT32 giMJContentH = 0;      // measured last reconcile
static INT32 giMJTrimmedPx = 0;     // height erased off the top since then
static BOOLEAN gfMJSticky = TRUE;   // pinned to the newest line
static UINT32 guiMJTypingFloor = 0; // earliest moment the next typist starts

// the terminal prints character by character; people do not
#define MJ_SYS_TYPE_MS	14
static UINT32 guiMJSysTypeStart = 0;
static std::size_t guiMJSysTypeLen = 0;

static void MahjongPushWrapped(int who, const ST::string& say)
{
	// long messages wrap into continuation lines instead of truncating
	INT32 budget = MJ_CHAT_W - 74;
	if (who >= 0) budget -= StringPixLength(ST::format("{}:", MahjongChatHandle(who)), FONT10ARIAL);
	if (const char* const badge = MahjongRoleBadge(who)) budget -= StringPixLength(badge, FONT10ARIAL) + 12;
	ST::string cur;
	INT8 pushWho = static_cast<INT8>(who);
	for (auto const& word : say.split(' '))
	{
		ST::string const cand = cur.empty() ? word : cur + " " + word;
		if (!cur.empty() && StringPixLength(cand, FONT10ARIAL) > budget)
		{
			gMJChat.push_back(MahjongChatLine{ pushWho, cur });
			cur = word;
			budget = MJ_CHAT_W - 74; // continuation lines get the full width
			// system and event continuations keep their own colour
			if (who == -1) pushWho = -3;
			else if (who == -4) pushWho = -5;
		}
		else cur = cand;
	}
	gMJChat.push_back(MahjongChatLine{ pushWho, cur });
	while (gMJChat.size() > MJ_CHAT_HISTORY)
	{
		// remember what left the top so the view can stay anchored
		giMJTrimmedPx += MahjongRowHeight(0);
		gMJChat.erase(gMJChat.begin());
	}
	// system lines print themselves out; human lines simply arrive
	guiMJSysTypeLen = who < 0 ? cur.size() : 0;
	guiMJSysTypeStart = MahjongNow();

	MahjongRedraw();
}

// a line lands once its author has finished "typing" it - unless they
// think better of it and never hit send
static void MahjongFlushPending()
{
	if (gMJPending.empty()) return;
	if (MahjongNow() < gMJPending.front().dueTime) return;
	MahjongPendingLine const line = gMJPending.front();
	gMJPending.erase(gMJPending.begin());
	if (line.ghost) MahjongRedraw();          // typed, deleted, never sent
	else            MahjongPushWrapped(line.who, line.text);
	// a beat of quiet before the next person starts typing
	guiMJTypingFloor = MahjongNow() + 700 + MahjongChatRoll() % 1100;
}

static void MahjongSay(int who, const ST::string& text)
{
	// a speaker repeating themselves reads terribly. checked against whole
	// lines they have said recently, not against the fragments in the log
	ST::string say = text;
	if (who > 0 && who < 5)
	{
		for (ST::string const& prev : gMJRecentSaid[who])
		{
			if (prev != text) continue;
			return; // they have said this. they say nothing instead
		}
		gMJRecentSaid[who].push_back(text);
		if (gMJRecentSaid[who].size() > 16) gMJRecentSaid[who].erase(gMJRecentSaid[who].begin());
	}
	// somebody who just said "brb" is not at their keyboard: they miss the
	// next stretch of conversation entirely, then come back to it
	if (who > 0)
	{
		if (MahjongNow() < guiMJAwayUntil[who]) return;
		std::string const low = say.to_std_string();
		if (low.compare(0, 3, "brb") == 0 || low.find("afk") != std::string::npos)
		{
			guiMJAwayUntil[who] = MahjongNow() + 26000 + MahjongChatRoll() % 22000;
		}
	}

	// people type in bursts. real chat habits (Baron & Ling): hitting send
	// does the work of a full stop, so trailing periods mostly vanish and
	// clauses arrive as separate messages. the Queen keeps her periods -
	// a sentence-final period reads as cold and abrupt, which is the point.
	std::vector<ST::string> bursts;
	{
		std::string const raw = say.to_std_string();
		std::size_t start = 0;
		for (std::size_t i = 0; i < raw.size(); ++i)
		{
			char const c = raw[i];
			bool const isStop = (c == '.' || c == '!' || c == '?');
			bool const isComma = (c == ',');
			if (!isStop && !isComma) continue;
			// run past repeated punctuation ("!!", "...")
			std::size_t end = i;
			if (isStop)
			{
				while (end + 1 < raw.size() &&
					(raw[end + 1] == '.' || raw[end + 1] == '!' || raw[end + 1] == '?')) ++end;
			}
			if (end + 1 >= raw.size()) break;             // final sentence: keep as is
			if (raw[end + 1] != ' ') { i = end; continue; }
			// a comma only splits when both halves stand on their own
			std::size_t const cut = isComma ? end : end + 1;
			if (isComma && (cut - start < 14 || raw.size() - (end + 2) < 14)) { i = end; continue; }
			std::string const piece = raw.substr(start, cut - start);
			if (piece.size() > 3) { bursts.push_back(ST::string(piece)); start = end + 2; }
			i = end;
		}
		std::string const tail = raw.substr(start);
		if (!tail.empty()) bursts.push_back(ST::string(tail));
	}
	if (bursts.empty()) bursts.push_back(say);
	// drop the terminal period: the send key already said that
	bool const keepsPeriods = who == 2 && giMJSeat2Persona == MJP2_QUEEN;
	if (!keepsPeriods)
	{
		for (ST::string& burst : bursts)
		{
			std::string b = burst.to_std_string();
			if (b.size() > 1 && b.back() == '.' && b[b.size() - 2] != '.')
			{
				b.pop_back();
				burst = ST::string(b);
			}
		}
	}

	// Everything the room says goes through ONE ordered lane, in the order
	// it was said. Two delivery paths (queued speech, instant notices) is
	// what let "@wolf has left the room" print before @wolf's own line.
	// Your own typing is the exception: a local echo, as every chat client
	// does, because latency on your own input reads as a bug.
	if (who == 0)
	{
		MahjongPushWrapped(who, bursts.front());
		for (std::size_t i = 1; i < bursts.size(); ++i)
		{
			gMJPending.push_back(MahjongPendingLine{ 0, bursts[i],
					MahjongNow() + static_cast<UINT32>(i) * 400, false });
		}
		return;
	}

	// the queue never runs more than a few seconds ahead of the room
	UINT32 due = std::max(MahjongNow(), std::min(guiMJQueueTail, MahjongNow() + 8000));
	if (who > 0)
	{
		// somebody has to sit down and type it, and the room breathes first
		due += 900 + MahjongChatRoll() % 1400;
		bool const ghost = bursts.size() == 1 && MahjongChatRoll() % 9 == 4;
		for (std::size_t i = 0; i < bursts.size(); ++i)
		{
			if (i > 0) due += 500 + MahjongChatRoll() % 700; // between their bursts
			due += 350 + static_cast<UINT32>(bursts[i].size()) * (MJ_TYPE_MS + 40);
			gMJPending.push_back(MahjongPendingLine{ static_cast<INT8>(who), bursts[i], due, ghost });
		}
	}
	else
	{
		// the house prints fast, but never ahead of what was said before it
		for (ST::string const& burst : bursts)
		{
			due += 220;
			gMJPending.push_back(MahjongPendingLine{ static_cast<INT8>(who), burst, due, false });
		}
	}
	guiMJQueueTail = due;
	MahjongRedraw();
}


// tactical-style exclamations: COOL / GOTIT / LAUGH from the merc voice
// sets. Only the human seat has one (your I.M.P. voice, or the shill's).
static void MahjongPlayBattleSnd(const char* suffix)
{
	int pid = -1;
	if (gfMJExhibition) pid = gubMJShillPid;
	else if (LaptopSaveInfo.fIMPCompletedFlag) pid = PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId;
	if (pid < 0) return;
	char path[64];
	snprintf(path, sizeof(path), BATTLESNDSDIR "/%03d_%s.wav", pid, suffix);
	MahjongPlayVoice(path, MIDVOLUME, MIDDLEPAN, 1600);
}

// character speech is staggered: a queued line waits for the floor
static void MahjongPlayVoice(const char* file, UINT32 vol, UINT32 pan, UINT32 estMs)
{
	UINT32 const now = MahjongNow();
	if (now < guiMJVoiceBusyUntil)
	{
		if (gMJPendingSound.empty())
		{
			gMJPendingSound = file;
			guiMJPendingSoundTime = guiMJVoiceBusyUntil + 500;
			guiMJVoiceBusyUntil = guiMJPendingSoundTime + estMs;
		}
		return; // the floor is taken and the queue is full: let it go
	}
	MahjongPlay(file, vol, pan);
	guiMJVoiceBusyUntil = now + estMs;
}

static UINT32 guiMJQuipCounter = 0;

static UINT32 MahjongChatRoll()
{
	return MahjongNow() / 7 + guiMJQuipCounter * 13;
}

static ST::string MahjongFanLabel(MahjongGame::WinEvent const& e)
{
	if (e.fan == 0) return ST::format("pays {}", e.payment);
	ST::string parts;
	auto const add = [&](ST::string const& t)
	{
		if (!parts.empty()) parts += ", ";
		parts += t;
	};
	if (e.fanFlags & MahjongGame::FAN_PURE_SUIT) add("pure suit");
	if (e.fanFlags & MahjongGame::FAN_ALL_TRIPLETS) add("all triplets");
	if (e.fanFlags & MahjongGame::FAN_SEVEN_PAIRS) add("seven pairs");
	if (e.roots > 0) add(ST::format("{} root{}", e.roots, e.roots > 1 ? "s" : ""));
	return ST::format("{} fan ({}) pays {}", e.fan, parts, e.payment);
}


// system status line, chess.com style; skips exact repeats so re-entering
// a state (e.g. after leaving the laptop) does not spam the log
// table events (claims, kongs, wins, settlements) read louder than the
// room's general notices, so they carry the yellow of the felt markers
// a notice already said - or already queued - is not said again
static bool MahjongNoticePending(const ST::string& text)
{
	std::size_t const from = gMJChat.size() > 8 ? gMJChat.size() - 8 : 0;
	for (std::size_t i = from; i < gMJChat.size(); ++i)
	{
		if (gMJChat[i].who < 0 && text.find(gMJChat[i].text) == 0) return true;
	}
	for (MahjongPendingLine const& q : gMJPending)
	{
		if (q.who < 0 && text.find(q.text) == 0) return true;
	}
	return false;
}

static void MahjongEventSay(const ST::string& text)
{
	if (MahjongNoticePending(text)) return;
	MahjongSay(-4, text);
}

static void MahjongSystemSay(const ST::string& text)
{
	// the House does not repeat itself within sight of the last notice
	if (MahjongNoticePending(text)) return;
	MahjongSay(-1, text);
}


// the player of the month: a stranger with a rating and no manners
static const char* const gMJVisitorTrash[14] =
{
	"heard this table was tough. I have seen tougher paperwork",
	"nice discards. do you take requests",
	"I am only here because the good room is full",
	"whoever is voided in bams is playing like it is a suggestion",
	"the House pays for my drinks. that tells you who has been winning",
	"four hands and I will be back at a real table",
	"somebody in here has never seen a kong. it shows",
	"I have beaten better players in worse rooms for less money",
	"do not mind me. I am scouting",
	"they put my face on the wall for this. small wall",
	"you all play like the felt owes you something",
	"I would offer advice but I would rather keep winning",
	"is this the beginner room. be honest",
	"whoever is up right now: enjoy it. it is temporary",
};

static void MahjongVisitorArrives()
{
	if (gfMJVisitorHere) return;
	MERCPROFILESTRUCT const& merc = GetProfile(static_cast<ProfileID>(MahjongChatRoll() % 40));
	if (merc.zNickname.empty()) return;
	gMJVisitorHandle = ST::format("@{}", merc.zNickname.to_lower());
	if (guiMJVisitorSurf) { DeleteVideoSurface(guiMJVisitorSurf); guiMJVisitorSurf = nullptr; }
	try
	{
		SGPVObject* const big = LoadBigPortrait(merc);
		guiMJVisitorSurf = AddVideoSurface(106, 122, PIXEL_DEPTH);
		BltVideoObject(guiMJVisitorSurf, big, 0, 0, 0);
		DeleteVideoObject(big);
	}
	catch (...) {}
	gfMJVisitorHere = TRUE;
	MahjongSystemSay(ST::format("{} has joined the room - player of the month", gMJVisitorHandle));
	MahjongSay(4, gMJVisitorTrash[MahjongChatRoll() % 14]);
	MahjongSay(4, gMJVisitorTrash[(MahjongChatRoll() / 3 + 7) % 14]);
	guiMJVisitorLeaves = MahjongNow() + 26000 + MahjongChatRoll() % 20000;
}

// log + chat reactions to a win; Deidranna's line is canon
static void MahjongChatOnWin(int winner, int discarder)
{
	if (winner == 0)
	{
		++giMJStatHandsWon;
		if (gGame->handDelta(0) > giMJStatBiggestHand) giMJStatBiggestHand = gGame->handDelta(0);
	}
	MahjongGame::WinEvent const* const win =
		gGame && !gGame->wins().empty() ? &gGame->wins().back() : nullptr;
	ST::string const fanInfo = win
		? ST::format(" - {}", MahjongFanLabel(*win)) : ST::string();
	ST::string const tileName = win && win->winningTile != MahjongGame::NO_TILE
		? MahjongTileLabel(win->winningTile) : ST::string("tile");
	MahjongSystemSay(discarder < 0
		? ST::format("{} {} by self-draw on the {}{}", MahjongSeatName(winner),
				MahjongVerb(winner, "win", "wins"), tileName, fanInfo)
		: ST::format("{} {} {} {}{}", MahjongSeatName(winner),
				MahjongVerb(winner, "claim", "claims"),
				discarder == 0 ? "your" : ST::format("{}'s", MahjongSeatName(discarder)).c_str(),
				tileName, fanInfo));
	// the Queen keeps count of who robs her
	if (winner == 0 && discarder == 2 && gGame)
	{
		++giMJGrudge;
		if (giMJSeat2Persona == MJP2_QUEEN) gGame->SetAiGrudge(2, 0, giMJGrudge);
		if (giMJGrudge >= 2)
		{
			MahjongSay(2, ST::format("that is the {}{} time. I keep COUNT.", giMJGrudge,
					giMJGrudge == 2 ? "nd" : giMJGrudge == 3 ? "rd" : "th"));
		}
	}
	if (winner == 3 && giMJSeat3Persona == MJP_ELLIOT)
	{
		MahjongPlayVoice(NPC_SPEECHDIR "/135_042.wav", MIDVOLUME, LEFTSIDE, 2800); // "he-heh, heh. You idiot!"
	}
	if (discarder == 3 && winner != 3 && giMJSeat3Persona == MJP_ELLIOT && giMJSeat2Persona == MJP2_QUEEN)
	{
		MahjongSay(2, "Elliot, you idiot!");
		// the real thing, straight from the palace
		static const char* const idiot[2] =
		{
			NPC_SPEECHDIR "/075_026.wav", // "Elliot! You idiot!"
			NPC_SPEECHDIR "/075_064.wav", // "Elll-eee-ooot! You IDIOT! Idiot! Idiot!"
		};
		MahjongPlayVoice(idiot[MahjongNow() / 7 % 2], MIDVOLUME, MIDDLEPAN, 2800);
	}
	if (winner == 0)
	{
		MahjongPlayBattleSnd("LAUGH");
		int const who = 1 + static_cast<int>(MahjongNow() % 3);
		MahjongGuestPack const* const g = MahjongPackFor(who);
		MahjongSay(who, g ? g->onHumanWin[MahjongChatRoll() % 3]
						: gMJChatOnHumanWin[who - 1][MahjongChatRoll() % 12]);
	}
}


static int MahjongWinOrderOf(int player)
{
	if (!gGame) return -1;
	for (size_t i = 0; i < gGame->wins().size(); ++i)
	{
		if (gGame->wins()[i].winner == player) return static_cast<int>(i);
	}
	return -1;
}


static void BtnMahjongNewGameCallback(GUI_BUTTON* btn, UINT32 reason);

// the New Game button moves between the bottom bar and the end-of-hand
// overlay, and its label follows the context; there is no move API, so it is
// recreated in place
static void MahjongPlaceNewGameButton(const ST::string& label, INT32 x, INT32 y, INT32 w)
{
	if (guiMJNewGameBtn) RemoveButton(guiMJNewGameBtn);
	guiMJNewGameBtn = CreateTextButton(label, FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK,
					static_cast<INT16>(x), static_cast<INT16>(y), static_cast<INT16>(w), 22,
					MSYS_PRIORITY_HIGH, BtnMahjongNewGameCallback);
	guiMJNewGameBtn->SetCursor(CURSOR_WWW);
	SpecifyButtonSoundScheme(guiMJNewGameBtn, BUTTON_SOUND_SCHEME_COMPUTERBEEP2);
}


static bool MahjongExchangeSelectionValid()
{
	if (!gGame) return false;
	std::vector<MahjongGame::TileId> const hand = MahjongGame::SortedHand(gGame->player(0));
	int n = 0, suit = -1;
	for (size_t i = 0; i < hand.size(); ++i)
	{
		if (!gfMJExchangeSel[i]) continue;
		++n;
		int const s = MahjongGame::SuitOf(hand[i]);
		if (suit == -1) suit = s;
		else if (suit != s) return false;
	}
	return n == 3;
}


static INT32 MahjongModalHeight()
{
	if (!gGame) return 306;
	INT32 c = 36 + 4 * 38;
	if (guiMJState == MJUI_HAND_END)
	{
		// each winner shows a line plus their hand laid open
		if (gGame->wins().empty()) c += 13 + 4;
		else c += static_cast<INT32>(gGame->wins().size())
				* (13 + MJ_MINI_H + 4) + 4;
	}
	if (guiMJState == MJUI_MATCH_END && giMJLastNetGain != 0) c += 24;
	return c + 64;
}

// the card centres on the felt and climbs when the open hands need room
static INT32 MahjongModalTop()
{
	return std::max(8, (398 - MahjongModalHeight()) / 2);
}

static void MahjongUpdateButtons()
{
	if (gfMJChatBig && guiMJState != MJUI_LOBBY && guiMJState != MJUI_LADDER)
	{
		if (guiMJNewGameBtn) HideButton(guiMJNewGameBtn);
		if (guiMJMahjongBtn) HideButton(guiMJMahjongBtn);
		if (guiMJPassBtn)    HideButton(guiMJPassBtn);
		if (guiMJPongBtn)    HideButton(guiMJPongBtn);
		if (guiMJKongBtn)    HideButton(guiMJKongBtn);
		if (guiMJLeaveBtn)   HideButton(guiMJLeaveBtn);
		if (guiMJReportBtn)  HideButton(guiMJReportBtn);
		for (MOUSE_REGION& r : gMJVoidRegion) r.Disable();
		return;
	}
	if (guiMJState == MJUI_LOBBY || guiMJState == MJUI_LADDER)
	{
		if (guiMJNewGameBtn) HideButton(guiMJNewGameBtn);
		if (guiMJMahjongBtn) HideButton(guiMJMahjongBtn);
		if (guiMJPassBtn)    HideButton(guiMJPassBtn);
		if (guiMJPongBtn)    HideButton(guiMJPongBtn);
		if (guiMJKongBtn)    HideButton(guiMJKongBtn);
		if (guiMJLeaveBtn)   HideButton(guiMJLeaveBtn);
		for (MOUSE_REGION& r : gMJIconRegion) r.Disable();
		for (MOUSE_REGION& r : gMJVoidRegion) r.Disable();
		if (guiMJReportBtn)
		{
			if (guiMJState == MJUI_LADDER) ShowButton(guiMJReportBtn);
			else                           HideButton(guiMJReportBtn);
		}
		return;
	}
	if (gfMJShowRules)
	{
		if (guiMJNewGameBtn) HideButton(guiMJNewGameBtn);
		if (guiMJMahjongBtn) HideButton(guiMJMahjongBtn);
		if (guiMJPassBtn)    HideButton(guiMJPassBtn);
		for (MOUSE_REGION& r : gMJIconRegion) r.Disable();

		for (MOUSE_REGION& r : gMJVoidRegion) r.Disable();
		return;
	}

	bool const canClaim =
		(guiMJState == MJUI_PLAYER_TURN && gGame && gGame->CanTsumo()) ||
		guiMJState == MJUI_RON_WINDOW || guiMJState == MJUI_ROB_WINDOW;
	bool const canPong = guiMJState == MJUI_CLAIM_WINDOW;
	bool const canKong =
		(guiMJState == MJUI_CLAIM_WINDOW && gfMJClaimKongPossible) ||
		(guiMJState == MJUI_PLAYER_TURN && gGame && !gGame->SelfKongOptions().empty());
	// the claim row anchors on the left edge and packs in a fixed order,
	// so a lone Kong! cant float where Mahjong! usually sits
	{
		INT32 bx = MJ_CLAIM_BTN_X;
		auto const place = [&bx](GUIButtonRef const& b, bool show, INT32 w)
		{
			if (!b) return;
			if (!show)
			{
				HideButton(b);
				return;
			}
			INT16 const nx = static_cast<INT16>(MJ_X(bx));
			b->Area.RegionTopLeftX = nx;
			b->Area.RegionBottomRightX = static_cast<INT16>(nx + w);
			ShowButton(b);
			bx += w + 8;
		};
		place(guiMJMahjongBtn, canClaim, 120);
		place(guiMJPongBtn, canPong, 88);
		place(guiMJKongBtn, canKong, 88);
	}

	switch (guiMJState)
	{
		case MJUI_IDLE:
			// below the parlour sign
			// centred in the pool of light, where the hand will be dealt
			MahjongPlaceNewGameButton("Deal Me In", MJ_X(201), MJ_Y(227), 100);
			break;
		case MJUI_HAND_END:
		case MJUI_MATCH_END:
			// the verdict card draws its own gold CTA; the grey stock
			// button stays out of it. The card is content-sized, so the
			// click region chases the button's actual row.
			if (guiMJNewGameBtn) HideButton(guiMJNewGameBtn);
			{
				INT16 const top = static_cast<INT16>(
						MJ_Y(MahjongModalTop() + MahjongModalHeight() - 58));
				gMJModalCTARegion.RegionTopLeftY = top;
				gMJModalCTARegion.RegionBottomRightY = static_cast<INT16>(top + 30);
			}
			gMJModalCTARegion.Enable();
			break;
		default:
			if (guiMJNewGameBtn) HideButton(guiMJNewGameBtn);
			break;
	}
	if (guiMJState != MJUI_HAND_END && guiMJState != MJUI_MATCH_END)
	{
		gMJModalCTARegion.Disable();
	}

	if (guiMJPassBtn)
	{
		if (guiMJState == MJUI_EXCHANGE)
		{
			ShowButton(guiMJPassBtn);
			EnableButton(guiMJPassBtn, MahjongExchangeSelectionValid());
		}
		else
		{
			HideButton(guiMJPassBtn);
		}
	}
	for (int s = 0; s < 3; ++s)
	{
		if (guiMJState == MJUI_CHOOSE_VOID) gMJVoidRegion[s].Enable();
		else                                gMJVoidRegion[s].Disable();
	}
	if (guiMJLeaveBtn)
	{
		if (guiMJState == MJUI_MATCH_END) ShowButton(guiMJLeaveBtn);
		else                              HideButton(guiMJLeaveBtn);
	}
	for (MOUSE_REGION& r : gMJIconRegion) r.Enable();
	if (guiMJReportBtn) HideButton(guiMJReportBtn);

}


static INT8 gbMJRulesPage = 0;

static void MahjongSetOverlayPage(int kind)
{
	giMJOverlayKind = kind;
	if (kind == 1) gbMJRulesPage = 0;
	gfMJShowRules = kind != 0 ? TRUE : FALSE;
	if (kind != 0) gMJRulesDismissRegion.Enable();
	else           gMJRulesDismissRegion.Disable();
	if (kind == 2) gMJSponsorRegion.Enable();
	else           gMJSponsorRegion.Disable();
	MahjongUpdateButtons();
	MahjongRedraw();
}


static void MahjongEnableHandRegions(bool enable)
{
	FOR_EACH(MOUSE_REGION, i, gMJHandRegion)
	{
		if (enable) i->Enable();
		else        i->Disable();
	}
}


#define MJ_OVERLAY_FACE_X 73  // overlay x 61 + 12
#define MJ_OVERLAY_FACE_Y 64  // overlay y 50 + 14
#define MJ_FACE_W 106
#define MJ_FACE_H 122
#define MJ_FACE_CROP_H 96     // the talking heads are black below the chin

static void MahjongDestroyOverlayFace()
{
	if (gpMJOverlayFace)
	{
		DeleteFace(gpMJOverlayFace);
		gpMJOverlayFace = nullptr;
	}
	if (guiMJFaceSurface)
	{
		DeleteVideoSurface(guiMJFaceSurface);
		guiMJFaceSurface = nullptr;
	}
}


static void MahjongCreateOverlayFace(ProfileID id)
{
	MahjongDestroyOverlayFace();
	try
	{
		guiMJFaceSurface = AddVideoSurface(MJ_FACE_W, MJ_FACE_H, PIXEL_DEPTH);
		gpMJOverlayFace = &InitFace(id, 0, FACE_BIGFACE);
		SetAutoFaceActive(guiMJFaceSurface, FACE_AUTO_RESTORE_BUFFER, *gpMJOverlayFace, 0, 0);
		RenderAutoFace(*gpMJOverlayFace);
	}
	catch (...)
	{
		MahjongDestroyOverlayFace();
	}
}


static INT32 MahjongBarTop() { return gfMJChatBig ? 4 : MJ_CHAT_Y; }
static INT32 MahjongBarH()   { return gfMJChatBig ? 396 : MJ_CHAT_H; }
static INT32 MahjongChatVisibleLines() { return (MahjongBarH() - 36) / 14; }
static INT32 MahjongChatViewH() { return MahjongBarH() - 30; }

// intrinsic height of a chat row: a new speaker opens a cluster with air
static INT32 MahjongRowHeight(std::size_t i)
{
	bool const opensCluster = i > 0 && gMJChat[i - 1].who != gMJChat[i].who;
	return 14 + (opensCluster ? MJ_CLUSTER_GAP : 0);
}

static bool MahjongTypingRowActive()
{
	// ephemeral by construction: derived from the head of the queue, never
	// from a timer of its own, and never for somebody who has left
	if (gMJPending.empty()) return false;
	INT8 const who = gMJPending.front().who;
	if (who <= 0) return false;
	if (who == 4 && !gfMJVisitorHere) return false;
	return true;
}

static INT32 MahjongContentH()
{
	INT32 total = 0;
	for (std::size_t i = 0; i < gMJChat.size(); ++i) total += MahjongRowHeight(i);
	if (MahjongTypingRowActive()) total += 14 + MJ_CLUSTER_GAP;
	return total;
}

// called before every layout: fold content changes into the scroll position
// so nothing ever jumps. growth at the bottom only moves the target (the
// eased follow), trimming at the top shifts both to anchor the reader.
static void MahjongReconcileScroll()
{
	INT32 const contentH = MahjongContentH();
	INT32 const scrollMax = std::max(0, contentH - MahjongChatViewH());
	if (giMJTrimmedPx != 0)
	{
		giMJScrollY -= giMJTrimmedPx;
		giMJScrollTarget -= giMJTrimmedPx;
		giMJTrimmedPx = 0;
	}
	if (gfMJSticky) giMJScrollTarget = scrollMax;
	giMJScrollTarget = std::max(0, std::min(scrollMax, giMJScrollTarget));
	giMJScrollY = std::max(0, std::min(scrollMax, giMJScrollY));
	giMJContentH = contentH;
}

static void MahjongEnterState(MahjongUiState state)
{
	guiMJState = state;
	for (MOUSE_REGION& r : gMJLobbyRegion)
	{
		if (state == MJUI_LOBBY) r.Enable(); else r.Disable();
	}
	if (state == MJUI_LADDER) gMJLadderBackRegion.Enable(); else gMJLadderBackRegion.Disable();

	gbMJSelectedSlot = -1;
	if (state != MJUI_HAND_END) MahjongDestroyOverlayFace();

	switch (state)
	{
		case MJUI_DEALING:
			guiMJDealStep = 0;
			if (gGame && gGame->handNumber() == 0)
			{
				for (std::vector<ST::string>& said : gMJRecentSaid) said.clear();
			}
			gfMJDecreeChecked = FALSE;
			gfMJSaidWallLow = FALSE;
			for (int i = 0; i < 4; ++i)
			{
				gfMJSaidTenpai[i] = FALSE;
				gfMJSaidBadHand[i] = FALSE;
				gfMJSaidVoidDraw[i] = FALSE;
			}
			guiMJNextEventTime = MahjongNow() + 130;
			gMJMessage = ST::format("Hand {} of {}. Shuffling 108 tiles...", gGame->handNumber() + 1, MahjongGame::HANDS_PER_MATCH);
			MahjongSystemSay(ST::format("Hand {} of {} - {}", gGame->handNumber() + 1,
					MahjongGame::HANDS_PER_MATCH, gGame->dealer() == 0
						? ST::string("you deal") : ST::format("{} deals", MahjongSeatName(gGame->dealer()))));
			if (gGame->handNumber() + 1 == MahjongGame::HANDS_PER_MATCH)
			{
				static const char* const lastHand[3] =
				{
					"the final hand. let us make it a dignified one.",
					"LAST hand. settle your debts and your prayers.",
					"last one!! then I can finally relax!!",
				};
				int const who = 1 + static_cast<int>(MahjongNow() % 3);
				MahjongSay(who, lastHand[who - 1]);
			}
			break;

		case MJUI_EXCHANGE:
		{
			std::fill(std::begin(gfMJExchangeSel), std::end(gfMJExchangeSel), FALSE);
			int const off = gGame->exchangeOffset();
			static const char* const dir[4] = { "", "on your right", "across", "on your left" };
			gMJMessage = ST::format("Exchange: pick 3 tiles of ONE suit to pass to {} ({}).", MahjongSeatName(off), dir[off]);
			if (gGame->handNumber() == 0)
			{
				std::fill(std::begin(gMJLossStreak), std::end(gMJLossStreak), 0);
				// the ladder, posted like it's 1999
				ST::string ratings = ST::format("ratings: you {}{}",
						MahjongPlayerRating(), MahjongRatingProvisional() ? " (prov)" : "");
				for (int seat = 1; seat <= 3; ++seat)
				{
					ratings += ST::format(" | {} {}", MahjongSeatName(seat), MahjongSeatRating(seat));
					if (seat == 2 && giMJSeat2Persona == MJP2_QUEEN) ratings += " (?)";
				}
				MahjongSystemSay(ratings);
				// once in a while the Queen invents a house superstition
				if (MahjongChatRoll() % 3 == 0 && GetProfile(QUEEN).bLife > 0 && giMJSeat2Persona == MJP2_QUEEN)
				{
					gbMJDecreeRank = 1 + static_cast<INT8>(MahjongChatRoll() % 9);
					MahjongSay(2, ST::format("house superstition: nobody discards a {} in the first round. it is DECREED.", gbMJDecreeRank));
				}
				else gbMJDecreeRank = -1;
				if (giMJSeat2Persona != MJP2_QUEEN)
				{
					MahjongSystemSay(ST::format("@dejdranna666 is busy tonight (affairs of state) - {} sits in", MahjongSeatName(2)));
				}
				else if (GetProfile(QUEEN).bLife > 0 && MahjongChatRoll() % 5 < 2)
				{
					// the Queen acknowledges the challenger, out loud
					MahjongPlayVoice(MahjongChatRoll() % 2 ? NPC_SPEECHDIR "/075_105.wav"  // "Against ME?!?"
							: NPC_SPEECHDIR "/075_050.wav", MIDVOLUME, MIDDLEPAN, 2800);
				}
			}
			{
				UINT32 const r = MahjongNow();
				{
					int const gwho = 1 + static_cast<int>(r % 3);
					MahjongGuestPack const* const g = MahjongPackFor(gwho);
					MahjongSay(gwho, g ? g->greet[MahjongChatRoll() % 3]
									: gMJChatGreet[gwho - 1][MahjongChatRoll() % 12]);
				}
			}
			break;
		}

		case MJUI_CHOOSE_VOID:
			gMJMessage = "Sichuan rules: choose the suit you will VOID. You cannot win while holding any tile of it.";
			break;

		case MJUI_PLAYER_TURN:
			gGame->DrawForCurrent();
			if (gGame->phase() == MahjongGame::Phase::HandEnd)
			{
				MahjongEnterState(MJUI_HAND_END);
				return;
			}
			MahjongPlay(MJ_SND_DRAW, LOWVOLUME);
			if (!gGame->CanTsumo() && gGame->ShantenFor(0) == 0 && !gfMJSaidTenpai[0])
			{
				gfMJSaidTenpai[0] = TRUE;
				MahjongPlayBattleSnd("COOL");
			}
			if (gGame->CanTsumo())
			{
				gMJMessage = "MAHJONG! Click the button to claim your win!";
				MahjongPlay(MJ_SND_ALERT, BTNVOLUME);
			}
			else
			{
				gMJMessage = "Your turn. Click a tile twice to discard it.";
			}
			break;

		case MJUI_AI_THINK:
			guiMJNextEventTime = MahjongNow() + MJ_AI_THINK_MIN + MahjongNow() % MJ_AI_THINK_SPREAD;
			// once in a while somebody wanders off, 1999 style
			if (guiMJQuipCounter % 89 == 53 && gGame->currentPlayer() > 0 && !gGame->player(gGame->currentPlayer()).finished)
			{
				int const away = gGame->currentPlayer();
				guiMJNextEventTime += 3500;
				MahjongSystemSay(ST::format("{} is away", MahjongChatHandle(away)));
				static const char* const awayMsg[3] =
				{
					"auto-reply: polishing the signet ring. back shortly.",
					"auto-reply: EXECUTING. back in five.",
					"auto-reply: afk!! Her Highness requires me!!",
				};
				const char* awayLine = awayMsg[away - 1];
				if (away == 3 && giMJSeat3Persona != 0) awayLine = "auto-reply: counting the take. touch nothing.";
				if (away == 2 && giMJSeat2Persona == MJP2_DARREN) awayLine = "auto-reply: ringside. back after round three.";
				if (away == 2 && giMJSeat2Persona == MJP2_LAYLA) awayLine = "auto-reply: attending to a guest, sugar. patience.";
				MahjongBotQueueReply(away, awayLine, MahjongChatRoll());
			}
			gMJMessage = gGame->player(0).finished
				? ST::format("You already won. {} plays on...", MahjongSeatName(gGame->currentPlayer()))
				: ST::format("{} is thinking...", MahjongSeatName(gGame->currentPlayer()));
			++guiMJQuipCounter;
			if (guiMJQuipCounter % 21 == 5)
			{
				int const who = gGame->currentPlayer();
				if (who > 0)
				{
					MahjongGuestPack const* const g = MahjongPackFor(who);
					if (g)
					{
						MahjongSay(who, MahjongChatRoll() % 2 == 0
							? g->war[MahjongWarTier()]
							: g->idle[MahjongChatRoll() % 3]);
					}
					else
					{
						MahjongSay(who, MahjongChatRoll() % 2 == 0
							? gMJChatWar[who - 1][MahjongWarTier() * 4 + MahjongChatRoll() % 4]
							: gMJChatIdle[who - 1][MahjongChatRoll() % 30]);
					}
				}
			}
			// your own mercs kibitz from your side of the connection
			if (guiMJQuipCounter % 47 == 17 && !gfMJExhibition)
			{
				struct Kibitz { UINT8 pid; const char* nick; const char* line[2]; };
				static Kibitz const kib[] =
				{
					{ IVAN,    "Ivan",    { "horosho. bird tile best tile.", "on Metavira we played for sap trees. this pays better." } },
					{ GRUNTY,  "Grunty",  { "in Germany ve play with more discipline. und beer.", "ze discard vas... suboptimal." } },
					{ STEROID, "Steroid", { "I could crush these tiles with one hand!!", "is arm wrestling option after?" } },
					{ FIDEL,   "Fidel",   { "just blow up the wall, si?", "Fidel likes the little bird tile. BOOM. sorry." } },
				};
				for (Kibitz const& k : kib)
				{
					if (!IsMercOnTeam(k.pid)) continue;
					MahjongSay(-2, ST::format("{}: {}", k.nick, k.line[MahjongChatRoll() % 2]));
					break;
				}
			}
			// the house PA crackles
			if (guiMJQuipCounter % 61 == 41)
			{
				MahjongSystemSay(gMJService[MahjongChatRoll() % MJ_SERVICE_COUNT]);
			}
			// the exes bicker (opener from Enrico, comeback from the Queen)
			if (guiMJQuipCounter % 37 == 11 && giMJBotWho < 0 && GetProfile(QUEEN).bLife > 0 && giMJSeat2Persona == MJP2_QUEEN)
			{
				MahjongBicker const& b = gMJBicker[MahjongChatRoll() % 30];
				MahjongSay(1, b.open);
				MahjongBotQueueReply(b.replyWho, b.reply, MahjongChatRoll());
			}
			// a lurker slips into the room
			else if (guiMJQuipCounter % 89 == 41)
			{
				MahjongVisitorArrives();
			}
			else if (guiMJQuipCounter % 59 == 23 && giMJBotWho < 0)
			{
				MahjongCameo const& c = gMJCameo[MahjongChatRoll() % 18];
				MahjongSystemSay(c.join);
				MahjongBotQueueReply(c.reactWho, c.react, MahjongChatRoll());
				gMJPendingCameoLeave = c.leave;
				guiMJCameoLeaveTime = MahjongNow() + 9000 + MahjongChatRoll() % 6000;
			}
			if (GetProfile(QUEEN).bLife == 0 && giMJSeat3Persona == MJP_ELLIOT && giMJSeat2Persona == MJP2_QUEEN && MahjongChatRoll() % 17 == 0)
			{
				MahjongSay(3, "...who is playing on her account??");
			}
			if (gGame->wallRemaining() <= 10 && !gfMJSaidWallLow)
			{
				gfMJSaidWallLow = TRUE;
				UINT32 const r = MahjongNow();
				{
					int const wwho = 1 + static_cast<int>(r % 3);
					MahjongGuestPack const* const g = MahjongPackFor(wwho);
					MahjongSay(wwho, g ? g->idle[MahjongChatRoll() % 3]
									: gMJChatWallLow[wwho - 1][MahjongChatRoll() % 8]);
				}
			}
			break;

		case MJUI_RON_WINDOW:
			guiMJNextEventTime = MahjongNow() + MJ_RON_WINDOW_TIME;
			MahjongPlay(MJ_SND_ALERT, BTNVOLUME);
			gMJMessage = ST::format("{} discarded your winning tile! Click Mahjong! to claim it!", MahjongSeatName(gGame->lastDiscarder()));
			break;

		case MJUI_ROB_WINDOW:
			guiMJNextEventTime = MahjongNow() + MJ_RON_WINDOW_TIME;
			MahjongPlay(MJ_SND_ALERT, BTNVOLUME);
			gMJMessage = ST::format("{} is promoting a kong of {} - ROB it to win!",
					MahjongSeatName(gGame->currentPlayer()), MahjongTileLabel(gMJRobTile));
			break;

		case MJUI_CLAIM_WINDOW:
			guiMJNextEventTime = MahjongNow() + MJ_RON_WINDOW_TIME;
			MahjongPlay(MJ_SND_ALERT, LOWVOLUME);
			gMJMessage = gfMJClaimKongPossible
				? ST::format("You can Pong or Kong {}'s discard!", MahjongSeatName(gGame->lastDiscarder()))
				: ST::format("You can Pong {}'s discard!", MahjongSeatName(gGame->lastDiscarder()));
			break;

		case MJUI_ANNOUNCE:
			guiMJNextEventTime = MahjongNow() + MJ_ANNOUNCE_TIME;
			// message set by the caller
			break;

		case MJUI_HAND_END:
			MahjongPlay(MJ_SND_HANDEND, BTNVOLUME);
			guiMJDeltaAnimStart = MahjongNow();
			// (the ledger's mini profiles replaced the big talking head)
			if (gGame->endedByWallExhaustion() && !gGame->aborted())
			{
				// cha jiao: name the pigs and the sleepers
				for (int i = 0; i < MahjongGame::NUM_PLAYERS; ++i)
				{
					if (gGame->player(i).finished) continue;
					if (gGame->IsFlowerPig(i))
					{
						MahjongEventSay(ST::format("{} is the FLOWER PIG - pays every player {}",
								MahjongSeatName(i), MahjongGame::PIG_PENALTY_EACH));
					}
					else if (!gGame->IsTenpai(i))
					{
						MahjongEventSay(ST::format("{} {} not ready - {} the waiting players",
								MahjongSeatName(i),
								MahjongVerb(i, "were", "was"),
								MahjongVerb(i, "pay", "pays")));
					}
				}
			}
			if (gGame->endedByWallExhaustion() && gGame->wins().empty())
			{
				gMJMessage = "The wall is empty. Nobody wins this hand.";
				MahjongEventSay("Hand over - exhaustive draw, no winners");
			}
			else if (gGame->endedByWallExhaustion())
			{
				gMJMessage = "The wall is empty. The bloody battle is over.";
				MahjongEventSay("Hand over - the wall is empty");
			}
			else if (gGame->aborted())
			{
				// the card's header already says HAND VOID; the footnote
				// only adds the sentiment
				gMJMessage = "Nobody pays, nobody forgets.";
				MahjongEventSay("Hand over - VOID after an irregularity");
			}
			else
			{
				gMJMessage = "Three winners stand. The last one left pays for the tea.";
				MahjongEventSay("Hand over - three winners stand");
			}
			for (int i = 0; i < MahjongGame::NUM_PLAYERS; ++i)
			{
				if (gGame->handDelta(i) < 0) ++gMJLossStreak[i];
				else if (gGame->handDelta(i) > 0) gMJLossStreak[i] = 0;
			}
			// real mahjong superstition: losing streaks demand rituals
			if (gMJLossStreak[1] == 2)
				MahjongSay(1, "excuse me. I must walk around my chair three times. do not ask.");
			if (gMJLossStreak[2] == 2)
				MahjongSay(2, "reshuffle the wall. the feng shui of this table is COMPROMISED.");
			if (gMJLossStreak[3] == 2)
			{
				if (giMJSeat3Persona == 0)
				{
					MahjongSay(3, "brb!! washing my hands!! for LUCK reasons!!");
					giMJGlitchWho = 3;
					guiMJGlitchEnd = MahjongNow() + 2600;
				}
				else MahjongSay(3, "hold my seat. I need to find a nun to bump into.");
			}
			if (gMJLossStreak[0] == 3)
				MahjongSystemSay("something brushed past table one. probably nothing");
			break;

		case MJUI_MATCH_END:
		{
			int best = 0;
			for (int i = 1; i < MahjongGame::NUM_PLAYERS; ++i)
			{
				if (gGame->player(i).score > gGame->player(best).score) best = i;
			}
			gMJMessage = best == 0
				? "Match over: you top the table! Enrico wires his congratulations."
				: ST::format("Match over: {} takes the table. Care for a rematch?", MahjongSeatName(best));
			MahjongEventSay(ST::format("Match over - {} {} the table with {} points",
					MahjongSeatName(best), best == 0 ? "top" : "takes", gGame->player(best).score));
			// settle the stakes: table points convert to dollars at 20:1,
			// posted to the Financial screen; Kingpin rakes 10% of winnings
			if (!gfMJSettled)
			{
				gfMJSettled = TRUE;
				{
					UINT8 place = 1;
					for (int i = 1; i < MahjongGame::NUM_PLAYERS; ++i)
					{
						if (gGame->player(i).score > gGame->player(0).score) ++place;
					}
					gMJHistory.push_back(MahjongMatchRecord{ static_cast<UINT16>(GetWorldDay()),
							gGame->player(0).score - MahjongGame::START_SCORE, place });
					if (gMJHistory.size() > 6) gMJHistory.erase(gMJHistory.begin());
				}
				int const mult = MahjongStakeMultiplier();
				INT32 const buyIn = 250 * mult;
				INT32 const net = gGame->player(0).score - MahjongGame::START_SCORE;
				INT32 const dollars = net * mult / 20;
				INT32 const raked = dollars > 0 ? dollars * 9 / 10 : dollars;
				// cash out: chips back plus table result, minus loan + 20% vig
				INT32 const payout = buyIn + raked - (gfMJLoan ? buyIn * 6 / 5 : 0);
				INT32 const netGain = payout - (gfMJLoan ? 0 : buyIn);
				giMJLastNetGain = netGain;
				giMJStatDollars += netGain;
				if (best == 0)
				{
					++giMJStatMatchesWon;
					if (MahjongInvitationalToday())
					{
						AddHistoryToPlayersLog(HISTORY_WON_MAHJONG_INVITATIONAL, 0,
								GetWorldTotalMin(), SGPSector());
					}
				}
				// Elliot's quiet winning streak
				if (giMJSeat3Persona == MJP_ELLIOT && gGame->player(3).score > MahjongGame::START_SCORE)
				{
					++giMJElliotGoodNights;
					if (giMJElliotGoodNights >= 3 && !gfMJElliotSecretSent)
					{
						gfMJElliotSecretSent = TRUE;
						AddEmailWithSpecialData(MAHJONG_EMAIL_ELLIOT_SECRET, 0, MAHJONG_ELLIOT_SENDER,
								GetWorldTotalMin(), giMJElliotGoodNights, 0);
					}
				}
				if (payout > 0)
				{
					AddTransactionToPlayersBook(ANONYMOUS_DEPOSIT, 0, GetWorldTotalMin(), payout);
					MahjongSystemSay(gfMJLoan
						? ST::format("Cash-out ${} after Kingpin's loan and vig", payout)
						: ST::format("Cash-out: ${} in chips converted - house kept its 10% rake", payout));
				}
				else if (payout < 0)
				{
					AddTransactionToPlayersBook(PAYMENT_TO_NPC, KINGPIN, GetWorldTotalMin(), payout);
					MahjongSystemSay(ST::format("The house collects a further ${} for Kingpin", -payout));
				}
				if (netGain >= 300)
				{
					AddStrategicEvent(EVENT_MAHJONG_KINGPIN_EMAIL,
							GetWorldTotalMin() + 90 + MahjongNow() % 120, static_cast<UINT32>(netGain));
				}
				else if (netGain <= -300)
				{
					AddStrategicEvent(EVENT_MAHJONG_KINGPIN_EMAIL,
							GetWorldTotalMin() + 90 + MahjongNow() % 120,
							static_cast<UINT32>(-netGain) | 0x80000000u);
				}
			}
			{
				// if Elliot finishes last, the Queen makes him say it
				int worst = 0;
				for (int i = 1; i < MahjongGame::NUM_PLAYERS; ++i)
				{
					if (gGame->player(i).score < gGame->player(worst).score) worst = i;
				}
				if (worst == 3 && giMJSeat3Persona == MJP_ELLIOT && giMJSeat2Persona == MJP2_QUEEN)
				{
					MahjongPlayVoice(NPC_SPEECHDIR "/075_066.wav", MIDVOLUME, MIDDLEPAN, 2800);
					gMJPendingSound = NPC_SPEECHDIR "/135_037.wav";       // "Ahem... I am an idiot, Your Highness."
					guiMJPendingSoundTime = std::max(MahjongNow() + 6000, guiMJVoiceBusyUntil + 500);
					guiMJVoiceBusyUntil = guiMJPendingSoundTime + 2800;
					MahjongSay(3, "...ahem. I am an idiot, Your Highness. :(");
				}
			}
			break;
		}

		case MJUI_IDLE:
		default:
			gMJMessage = MahjongInvitationalToday()
				? "BLOODY INVITATIONAL TONIGHT - triple stakes. hit Deal Me In and take the chair."
				: "The House is warming your seat. hit Deal Me In and take the chair.";
			MahjongSystemSay(gMJMessage);
			break;
	}

	// the prompts that ask something of you belong in the log, where the
	// input line used to shout them
	switch (state)
	{
		case MJUI_EXCHANGE:
		case MJUI_CHOOSE_VOID:
		case MJUI_CLAIM_WINDOW:
		case MJUI_RON_WINDOW:
		case MJUI_ROB_WINDOW:
			MahjongSystemSay(gMJMessage);
			break;
		default:
			break;
	}

	MahjongEnableHandRegions(guiMJState == MJUI_PLAYER_TURN || guiMJState == MJUI_EXCHANGE);
	MahjongUpdateButtons();
	MahjongRedraw();
}


static void MahjongContinueAfterEvent()
{
	if (gGame->phase() == MahjongGame::Phase::HandEnd)
	{
		MahjongEnterState(MJUI_HAND_END);
		return;
	}
	MahjongEnterState(gGame->currentPlayer() == 0 ? MJUI_PLAYER_TURN : MJUI_AI_THINK);
}


// A win happened but the bloody battle continues: show the taunt briefly.
static void MahjongAnnounce(const ST::string& msg)
{
	if (gGame->phase() == MahjongGame::Phase::HandEnd)
	{
		MahjongEnterState(MJUI_HAND_END);
		return;
	}
	MahjongEnterState(MJUI_ANNOUNCE);
	MahjongPlay(MJ_SND_WIN, MIDVOLUME);
	gMJMessage = msg;
	MahjongRedraw();
}


// After a discard: resolve ron claims, else pass the turn on.
static void MahjongAfterDiscard()
{
	int const claimant = gGame->RonClaimant();
	if (claimant == 0)
	{
		MahjongEnterState(MJUI_RON_WINDOW);
		return;
	}
	if (claimant > 0)
	{
		// Elliot occasionally fails to notice his own winning tile
		if (claimant == 3 && giMJSeat3Persona == MJP_ELLIOT && MahjongNow() % 5 == 0)
		{
			MahjongSay(3, "wait. WAIT. was that my tile?? ...aww.");
			gGame->PassRon();
			MahjongContinueAfterEvent();
			return;
		}
		int const discarder = gGame->lastDiscarder();
		gGame->ResolveRon(claimant);
		MahjongChatOnWin(claimant, discarder);
		{
			MahjongGuestPack const* const g = MahjongPackFor(claimant);
			if (claimant == 1 && discarder == 2 && giMJSeat2Persona == MJP2_QUEEN)
			{
				MahjongAnnounce(gMJSpouseEnricoRonsQueen[MahjongChatRoll() % 6]);
			}
			else if (claimant == 2 && discarder == 1 && giMJSeat2Persona == MJP2_QUEEN)
			{
				MahjongAnnounce(gMJSpouseQueenRonsEnrico[MahjongChatRoll() % 6]);
			}
			else if (g)
			{
				MahjongAnnounce((discarder == 0 ? g->ronVictim : g->winTaunt)[MahjongChatRoll() % 3]);
			}
			else
			{
				MahjongAnnounce((discarder == 0 ? gMJRonVictimTaunt : gMJWinTaunt)[claimant][MahjongChatRoll() % 3]);
			}
		}
		return;
	}
	// pong / kong arbitration (ron above already failed)
	bool kongPossible = false;
	int const mc = gGame->MeldClaimant(kongPossible);
	if (mc == 0)
	{
		gfMJClaimKongPossible = kongPossible ? TRUE : FALSE;
		MahjongEnterState(MJUI_CLAIM_WINDOW);
		return;
	}
	if (mc > 0)
	{
		if (kongPossible && gGame->AiWantsKong(mc))
		{
			MahjongGame::TileId const t = gGame->lastDiscard();
			gGame->ClaimKong(mc);
			MahjongPlay(MJ_SND_MELD, BTNVOLUME, MahjongSeatPan(mc));
			MahjongEventSay(ST::format("{} {} the {}", MahjongSeatName(mc),
					MahjongVerb(mc, "kong", "kongs"), MahjongTileLabel(t)));
			{
				MahjongGuestPack const* const g = MahjongPackFor(mc);
				MahjongSay(mc, g ? g->claim[MahjongChatRoll() % 3] : gMJChatClaim[mc - 1][MahjongChatRoll() % 8]);
			}
			MahjongContinueAfterEvent();
			return;
		}
		if (gGame->AiWantsPong(mc))
		{
			MahjongGame::TileId const t = gGame->lastDiscard();
			gGame->ClaimPong(mc);
			MahjongPlay(MJ_SND_MELD, BTNVOLUME, MahjongSeatPan(mc));
			MahjongEventSay(ST::format("{} {} the {}", MahjongSeatName(mc),
					MahjongVerb(mc, "pong", "pongs"), MahjongTileLabel(t)));
			if (mc == 3 && giMJSeat3Persona == 0 && MahjongChatRoll() % 7 == 3)
			{
				MahjongSay(3, "wait. WAIT. I didn't need that one. um. no takebacks? ok.");
			}
			else if (MahjongChatRoll() % 3 == 0)
			{
				MahjongGuestPack const* const g = MahjongPackFor(mc);
				MahjongSay(mc, g ? g->claim[MahjongChatRoll() % 3] : gMJChatClaim[mc - 1][MahjongChatRoll() % 8]);
			}
			MahjongContinueAfterEvent();
			return;
		}
	}

	gGame->PassRon();
	MahjongContinueAfterEvent();
}


// the Bloody Invitational: every 7th day the parlour runs triple stakes
static bool MahjongInvitationalToday()
{
	return GetWorldDay() > 0 && GetWorldDay() % 7 == 0;
}

static int MahjongStakeMultiplier()
{
	if (gfMJBackRoom) return 5;
	return MahjongInvitationalToday() ? 3 : 1;
}

static void MahjongReloadSeat3Faces();
static void MahjongReloadSeat2Faces();

static void MahjongSyncSeat3(int persona)
{
	if (giMJSeat3Persona == persona) return;
	giMJSeat3Persona = persona;
	gMJOpponentProfile[2] = gMJPersonaProfile[persona];
	MahjongReloadSeat3Faces();
}

static void MahjongSyncSeat2(int persona)
{
	if (giMJSeat2Persona == persona) return;
	giMJSeat2Persona = persona;
	gMJOpponentProfile[1] = gMJSeat2Profile[persona];
	MahjongReloadSeat2Faces();
}

// The House keeps the table warm: a full 4-player exhibition game runs while
// nobody is sitting in seat 0
static void MahjongExhibitionStep();

static void MahjongStartExhibition()
{
	MahjongSyncSeat3(MahjongPersonaForToday());
	MahjongSyncSeat2(MahjongSeat2ForToday());
	if (!gGame) gGame = std::make_unique<MahjongGame>();
	gGame->NewMatch(MahjongNow());
	MahjongGame::TileId give[3];
	MahjongGame::AiChooseExchange(gGame->player(0).counts, give);
	gGame->SetHumanExchange(give);
	gGame->SetHumanVoidSuit(MahjongGame::AiChooseVoidSuit(gGame->player(0).counts));
	gGame->SetAiErrorRate(0, 20);
	gGame->SetAiErrorRate(1, 25);
	gGame->SetAiErrorRate(2, MahjongSeat2ErrorRate());
	gGame->SetAiErrorRate(3, giMJSeat3Persona == MJP_ELLIOT ? 45 : giMJSeat3Persona == MJP_KINGPIN ? 10 : 30);
	gfMJExhibition = TRUE;
	gfMJSettled = TRUE; // exhibition plays for pride, not dollars

	// the table has been running "for a while": fast-forward so the room
	// already has discards on the felt and chat history on the wire
	if (!gfMJWarmup)
	{
		gfMJWarmup = TRUE;
		int iSteps = 40 + static_cast<int>(MahjongNow() % 50);
		while (iSteps-- > 0 && gGame->phase() != MahjongGame::Phase::MatchEnd) MahjongExhibitionStep();
		gfMJWarmup = FALSE;
	}

	// a random off-duty A.I.M. merc warms your seat for the House
	if (guiMJShillSurf) { DeleteVideoSurface(guiMJShillSurf); guiMJShillSurf = nullptr; }
	gMJShillNick.clear();
	try
	{
		gubMJShillPid = static_cast<UINT8>(MahjongNow() % 40);
		MERCPROFILESTRUCT const& shill = GetProfile(static_cast<ProfileID>(gubMJShillPid));
		SGPVObject* const big = LoadBigPortrait(shill);
		guiMJShillSurf = AddVideoSurface(106, 122, PIXEL_DEPTH);
		BltVideoObject(guiMJShillSurf, big, 0, 0, 0);
		DeleteVideoObject(big);
		gMJShillNick = shill.zNickname;
	}
	catch (...)
	{
		// no face for that one: the empty-seat silhouette covers it
	}
}

static INT32 giMJRatingBefore = 0; // ladder rating when the match began

static void MahjongStartMatch()
{
	giMJRatingBefore = MahjongPlayerRating();
	MahjongSyncSeat3(MahjongPersonaForToday());
	MahjongSyncSeat2(MahjongSeat2ForToday());
	gfMJExhibition = FALSE;
	if (!gGame) gGame = std::make_unique<MahjongGame>();
	gGame->NewMatch(MahjongNow());
	// human-like fallibility per seat: Elliot fumbles, the Queen barely does
	gGame->SetAiErrorRate(0, 0);
	int const div = gfMJBackRoom ? 2 : 1; // the back room plays sharp
	gGame->SetAiErrorRate(1, 25 / div); // Enrico
	gGame->SetAiErrorRate(2, MahjongSeat2ErrorRate() / div);
	gGame->SetAiErrorRate(3, (giMJSeat3Persona == MJP_ELLIOT ? 45 : giMJSeat3Persona == MJP_KINGPIN ? 10 : 30) / div);
	if (giMJSeat2Persona == MJP2_QUEEN) gGame->SetAiGrudge(2, 0, giMJGrudge);
	if (gfMJBackRoom) MahjongSystemSay("BACK ROOM - 5x stakes, sharp players, no tourists");
	gfMJSettled = FALSE;
	++giMJStatMatches;
	if (giMJSeat3Persona != MJP_ELLIOT)
	{
		MahjongSystemSay(ST::format("Tonight's guest at the table: {}", gMJPersonaName[giMJSeat3Persona]));
	}
	// the ritual: chips first; Kingpin fronts you if the account is dry
	{
		INT32 const buyIn = 250 * MahjongStakeMultiplier();
		if (MahjongInvitationalToday())
		{
			MahjongSystemSay("BLOODY INVITATIONAL tonight - triple stakes, no mercy");
		}
		if (GetCurrentBalance() < buyIn)
		{
			gfMJLoan = TRUE;
			MahjongSystemSay(ST::format("Kingpin fronts your ${} buy-in. 20% vig due at close, friend.", buyIn));
		}
		else
		{
			gfMJLoan = FALSE;
			AddTransactionToPlayersBook(PAYMENT_TO_NPC, KINGPIN, GetWorldTotalMin(), -buyIn);
			MahjongSystemSay(ST::format("Buy-in: ${} in chips", buyIn));
		}
	}
	MahjongEnterState(MJUI_DEALING);
}


static void BtnMahjongLeaveCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (guiMJState != MJUI_MATCH_END) return;

	MahjongSystemSay("You leave the table. The House takes your seat before it cools.");
	MahjongStartExhibition();
	MahjongEnterState(MJUI_IDLE);
}


static void MahjongNewGameAction()
{
	if (guiMJState == MJUI_HAND_END)
	{
		gGame->NewHand();
		MahjongEnterState(gGame->phase() == MahjongGame::Phase::MatchEnd ? MJUI_MATCH_END : MJUI_DEALING);
	}
	else if (guiMJState == MJUI_IDLE || guiMJState == MJUI_MATCH_END)
	{
		MahjongStartMatch();
	}
}

static void BtnMahjongNewGameCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	MahjongNewGameAction();
}

static void MahjongModalCTACallback(MOUSE_REGION* region, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (guiMJState != MJUI_HAND_END && guiMJState != MJUI_MATCH_END) return;
	MahjongPlay(MJ_SND_SELECT, BTNVOLUME);
	MahjongNewGameAction();
}


static void BtnMahjongMahjongCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	if (guiMJState == MJUI_PLAYER_TURN && gGame->CanTsumo())
	{
		gGame->ResolveTsumo();
		MahjongChatOnWin(0, -1);
		MahjongAnnounce(gMJWinTaunt[0][MahjongChatRoll() % 8]);
	}
	else if (guiMJState == MJUI_RON_WINDOW)
	{
		int const discarder = gGame->lastDiscarder();
		gGame->ResolveRon(0);
		MahjongChatOnWin(0, discarder);
		MahjongAnnounce(gMJWinTaunt[0][MahjongChatRoll() % 8]);
	}
	else if (guiMJState == MJUI_ROB_WINDOW)
	{
		int const declarer = gGame->currentPlayer();
		gGame->ResolveRobKong(0, gMJRobTile);
		MahjongSystemSay("KONG ROBBED - the 4th tile never lands");
		MahjongChatOnWin(0, declarer);
		MahjongAnnounce(gMJWinTaunt[0][MahjongChatRoll() % 8]);
	}
}


// hover repaints the page so the drawn buttons can actually react
static void MahjongHoverRedrawCallback(MOUSE_REGION* region, UINT32 reason)
{
	if (reason & (MSYS_CALLBACK_REASON_GAIN_MOUSE | MSYS_CALLBACK_REASON_LOST_MOUSE))
	{
		MahjongRedraw();
	}
}

static void MahjongVoidPickCallback(MOUSE_REGION* region, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (guiMJState != MJUI_CHOOSE_VOID) return;

	MahjongPlay(MJ_SND_SELECT, BTNVOLUME);
	gGame->SetHumanVoidSuit(static_cast<int>(MSYS_GetRegionUserData(region, 0)));
	MahjongSystemSay(ST::format("Voids declared - You: {} | Enrico: {} | Deidranna: {} | Elliot: {}",
			gMJSuitName[gGame->player(0).voidSuit], gMJSuitName[gGame->player(1).voidSuit],
			gMJSuitName[gGame->player(2).voidSuit], gMJSuitName[gGame->player(3).voidSuit]));
	MahjongContinueAfterEvent();
}


static void BtnMahjongPongCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (guiMJState != MJUI_CLAIM_WINDOW) return;

	MahjongGame::TileId const t = gGame->lastDiscard();
	gGame->ClaimPong(0);
	MahjongPlayBattleSnd("GOTIT");
	MahjongPlay(MJ_SND_MELD, BTNVOLUME);
	MahjongEventSay(ST::format("You pong the {}", MahjongTileLabel(t)));
	MahjongEnterState(MJUI_PLAYER_TURN);
}


static void BtnMahjongKongCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	if (guiMJState == MJUI_CLAIM_WINDOW)
	{
		MahjongGame::TileId const t = gGame->lastDiscard();
		gGame->ClaimKong(0);
		MahjongPlayBattleSnd("GOTIT");
		MahjongPlay(MJ_SND_MELD, BTNVOLUME);
		MahjongEventSay(ST::format("You kong the {} - the discarder pays the bonus", MahjongTileLabel(t)));
		MahjongEnterState(MJUI_PLAYER_TURN);
	}
	else if (guiMJState == MJUI_PLAYER_TURN)
	{
		std::vector<MahjongGame::TileId> const options = gGame->SelfKongOptions();
		if (options.empty()) return;
		MahjongGame::TileId const t = options.front();
		if (gGame->IsAddedKong(t))
		{
			int const robber = gGame->RobKongClaimant(t);
			if (robber > 0)
			{
				gGame->ResolveRobKong(robber, t);
				MahjongEventSay(ST::format("{} ROBS your kong of the {}!",
						MahjongSeatName(robber), MahjongTileLabel(t)));
				MahjongAnnounce(gMJWinTaunt[robber][MahjongChatRoll() % 8]);
				return;
			}
		}
		if (gGame->DeclareSelfKong(t))
		{
			MahjongPlay(MJ_SND_MELD, BTNVOLUME);
			MahjongEventSay(ST::format("You kong the {} - the table pays the bonus", MahjongTileLabel(t)));
			MahjongEnterState(MJUI_PLAYER_TURN); // recompute tsumo/waits with the replacement
		}
	}
}


static void BtnMahjongPassCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (guiMJState != MJUI_EXCHANGE) return;

	std::vector<MahjongGame::TileId> const hand = MahjongGame::SortedHand(gGame->player(0));
	MahjongGame::TileId give[3];
	int n = 0;
	for (size_t i = 0; i < hand.size() && n < 3; ++i)
	{
		if (gfMJExchangeSel[i]) give[n++] = hand[i];
	}
	if (n != 3 || !gGame->SetHumanExchange(give))
	{
		gMJMessage = "Pick exactly 3 tiles of ONE suit to pass.";
		MahjongRedraw();
		return;
	}
	std::fill(std::begin(gfMJExchangeSel), std::end(gfMJExchangeSel), FALSE);
	MahjongPlay(MJ_SND_DISCARD, BTNVOLUME);
	MahjongSystemSay(ST::format("Tiles passed - you gave 3 {} to {}",
			gMJSuitName[MahjongGame::SuitOf(give[0])], MahjongSeatName(gGame->exchangeOffset())));
	MahjongEnterState(MJUI_CHOOSE_VOID);
}




// --- minimal chatbot ----------------------------------------------------
static bool MahjongTextContains(const std::string& haystack, const char* needle)
{
	return haystack.find(needle) != std::string::npos;
}


static void MahjongBotQueueReply(int who, const char* line, UINT32 hash)
{
	// ask the same question twice and you get told so, rather than hearing
	// the same sentence again
	if (who > 0 && who < 5)
	{
		for (ST::string const& prev : gMJRecentSaid[who])
		{
			if (prev != ST::string(line)) continue;
			static const char* const brushOff[8] =
			{
				"you asked me that already",
				"asked and answered",
				"my answer has not improved",
				"same as last time",
				"I already told you that",
				"we did this bit",
				"still true, still boring",
				"ask me something else",
			};
			line = brushOff[hash % 8];
			break;
		}
	}
	giMJBotWho = who;
	gMJBotLine = line;
	guiMJBotDueTime = MahjongNow() + 700 + hash % 900;
}


static ST::string MahjongReflect(const std::string& frag)
{
	// ELIZA-style pronoun reflection, token by token
	static const std::pair<const char*, const char*> refl[] =
	{
		{ "i", "you" }, { "me", "you" }, { "my", "your" }, { "mine", "yours" },
		{ "am", "are" }, { "im", "you're" }, { "i'm", "you're" }, { "myself", "yourself" },
		{ "you", "I" }, { "your", "my" }, { "yours", "mine" }, { "yourself", "myself" },
		{ "we", "you" }, { "us", "you" }, { "our", "your" },
	};
	ST::string out;
	std::string tok;
	auto flush = [&]()
	{
		if (tok.empty()) return;
		const char* mapped = nullptr;
		for (auto const& r : refl)
		{
			if (tok == r.first) { mapped = r.second; break; }
		}
		if (!out.empty()) out += " ";
		out += mapped ? mapped : tok.c_str();
		tok.clear();
	};
	for (char c : frag)
	{
		if (c == ' ') flush();
		else tok += c;
	}
	flush();
	return out;
}


// single-wildcard decomposition: "i am *", "* hate *" etc.
static bool MahjongMatchRule(const std::string& text, const char* pat, std::string& cap)
{
	std::string const p(pat);
	size_t const star = p.find('*');
	if (star == std::string::npos) return text.find(p) != std::string::npos;
	std::string const pre = p.substr(0, star);
	std::string const post = p.substr(star + 1);
	size_t const at = pre.empty() ? 0 : text.find(pre);
	if (at == std::string::npos) return false;
	size_t const capStart = at + pre.size();
	size_t capEnd = text.size();
	if (!post.empty())
	{
		size_t const pAt = text.find(post, capStart);
		if (pAt == std::string::npos) return false;
		capEnd = pAt;
	}
	cap = text.substr(capStart, capEnd - capStart);
	while (!cap.empty() && cap.front() == ' ') cap.erase(cap.begin());
	while (!cap.empty() && cap.back() == ' ') cap.pop_back();
	if (cap.size() > 40) cap.resize(40);
	return !cap.empty() || post.empty();
}


static std::string gMJChatMemory; // ELIZA memory queue: the last "my ..." topic

struct MahjongElizaRule
{
	const char* pat;
	int who;         // responder; -9 = pick 1..3 by roll
	const char* r[3]; // {} = reflected capture; cycled to avoid repeats
};

static const MahjongElizaRule gMJElizaRules[] =
{
	// direct address beats everything
	{ "elliot*idiot", 3, { "s-sorry!! I'm trying my best...", "even YOU now?? aww.", "noted. adding it to the pile." } },
	{ "elliot*", 3, { "y-yes? did I do something?", "I'm listening!! (quietly, in case she reads this)", "present! sadly." } },
	{ "deidranna*", 2, { "you DARE address me directly? bold. stupid, but bold.", "speak quickly. tiles wait for no peasant.", "I am listening. that is already more than you deserve." } },
	{ "queen*", 2, { "yes. THE Queen. mind your discards.", "flattery will cost you extra at MY table.", "address me as Your Highness or not at all." } },
	{ "enrico*", 1, { "at your service, my friend.", "you have my attention.", "speak freely. this table keeps secrets poorly, but I keep them well." } },
	{ "kingpin*", -9, { "shhh he reads the chat logs!! probably!!", "the house always collects, friend. I speak from experience.", "we do not say his name at the table. he PREFERS it." } },

	// ELIZA classics, reflected
	{ "i am *", -9, { "why are you {}?", "how long have you been {}?", "and being {} helps your mahjong how?" } },
	{ "i feel *", 1, { "the tiles feel {} too, some nights.", "why do you feel {}, friend?", "a table is a poor place to feel {}." } },
	{ "i think *", 2, { "you THINK {}. adorable.", "and why would {} matter to me?", "thinking is free. losing is not." } },
	{ "i want *", -9, { "everyone at this table wants {}.", "why do you want {}?", "want {} less. discard better." } },
	{ "i hate *", 2, { "good. hate {} MORE. it builds character.", "why do you hate {}?", "I have executed people over {}. we are not so different." } },
	{ "i have *", 3, { "you have {}?? all I have is anxiety!!", "keep {} away from the Queen.", "trade you {} for a winning hand!!" } },
	{ "do you *", -9, { "do I {}? do any of us, truly?", "I {} only on invitational nights.", "ask the wall. the wall knows." } },
	{ "are you *", -9, { "am I {}? after this war, who can say.", "MORE {} than you will ever be.", "only on the modem. in person, worse." } },
	{ "can you *", 1, { "at these stakes? I can {} twice.", "once, before the exile, I could {}.", "the question is whether you can afford it." } },
	{ "why *", -9, { "why {}? because Arulco, friend. always because Arulco.", "some questions the palace does not answer.", "why not? that is how we all ended up here." } },
	{ "you are *", -9, { "I am {} AND winning. note the difference.", "what makes you think I am {}?", "{}. yes. it is on my card." } },
	{ "my *", -9, { "your {} concerns the table how?", "we all have troubles with our {}.", "tell the guestbook about your {}." } },

	// game-adjacent smalltalk
	{ "sichuan*", 1, { "the purest rules ever carved into tiles.", "no chi, no mercy. as it should be.", "learned it from a Chengdu smuggler. lovely man. terrible debts." } },
	{ "arulco*", 1, { "my father's country. one day again, perhaps.", "she runs it. I fund the correction.", "somewhere out there, my investment is shooting its way to Meduna." } },
	{ "war*", 2, { "we do NOT discuss the war at MY table.", "the war is going PERFECTLY. next topic.", "ask your mercenaries. they seem chatty." } },
	{ "money*", -9, { "the house always collects, friend.", "money is just points with consequences.", "keep it. you will need it for the vig." } },
	{ "cheat*", 2, { "I do not cheat. I REIGN. there is a difference.", "prove it. my lawyers are also my executioners.", "the extra tile was PLANTED." } },
	{ "bobby*", 3, { "great shipping rates!! don't tell the Queen I said that!!", "half my paycheck goes there. the other half goes to her.", "their catalog is my only joy." } },
	{ "sorry*", 3, { "it's ok!! I apologize professionally, I know how it is!!", "apology accepted. teach me your confidence.", "no no, *I'M* sorry. force of habit." } },
	{ "thank*", 1, { "think nothing of it.", "courtesy costs nothing. unlike this table.", "you are welcome at this table. your money more so." } },
	{ "hello*", -9, { "hi!! gl hf :)", "good evening. may the tiles be kind.", "greetings. tribute is accepted in points." } },
	{ "hi*", -9, { "hii!!", "good evening.", "sit. lose. socialize later." } },
	{ "bye*", 1, { "until the tiles call again.", "go well, friend. the seat stays warm.", "leaving while ahead? wise. suspicious, but wise." } },
	{ "lol*", 3, { "hehe :)", "glad SOMEONE is having fun!!", "I laughed too and then she looked at me." } },
	{ "help*", 1, { "click the little ? in the corner, friend.", "watch the voids in the log. safe tiles hide there.", "keep pairs. dump terminals. pray." } },
	{ "rule*", 1, { "the ? button explains everything the house admits to.", "108 tiles, one void suit, no mercy.", "rule one: the vig is not negotiable." } },
};

static void MahjongBotConsider(const std::string& raw)
{
	// normalise: lowercase, letters/digits/spaces only
	std::string text;
	text.reserve(raw.size());
	for (char c : raw)
	{
		unsigned char const u = static_cast<unsigned char>(c);
		if (std::isalnum(u) || c == ' ' || c == '\'') text += static_cast<char>(std::tolower(u));
	}
	UINT32 hash = 0;
	for (char c : text) hash = hash * 31 + static_cast<unsigned char>(c);

	// --- keyboard mash gets called out before anything else ---
	if (text.size() >= 6)
	{
		int vowels = 0, letters = 0, run = 0, worstRun = 0;
		for (char c : text)
		{
			if (c == ' ') { run = 0; continue; }
			++letters;
			if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y') { ++vowels; run = 0; }
			else if (std::isalpha(static_cast<unsigned char>(c))) { ++run; worstRun = std::max(worstRun, run); }
		}
		if (letters >= 6 && (worstRun >= 5 || vowels * 4 < letters))
		{
			static const int mashWho[4] = { 3, 2, 1, 3 };
			static const char* const mashLine[4] =
			{
				"did your cat walk across the keyboard?? mine does that!!",
				"was that a code? I will have it decrypted. and then you.",
				"I have seen ransom notes with better spelling.",
				"blink twice if your keyboard is holding you hostage.",
			};
			int const pick = static_cast<int>(hash % 4);
			MahjongBotQueueReply(mashWho[pick], mashLine[pick], hash);
			return;
		}
	}

	// --- summoning someone by name gets their attention ---
	if (text.find("elliot") != std::string::npos || text.find("e11iot") != std::string::npos)
	{
		MahjongBotQueueReply(3, giMJSeat3Persona == 0
				? "you rang?? sorry!! here!!" : "the palace kid? not at this table tonight.", hash);
		return;
	}
	if (text.find("deidranna") != std::string::npos || text.find("dejdranna") != std::string::npos ||
		text.find("queen") != std::string::npos)
	{
		if (giMJSeat2Persona == MJP2_DARREN)
			MahjongBotQueueReply(2, "the Queen is busy running your war. you get me tonight.", hash);
		else if (giMJSeat2Persona == MJP2_LAYLA)
			MahjongBotQueueReply(2, "Her Majesty cancelled again, sugar. affairs of state.", hash);
		else
			MahjongBotQueueReply(2, "speak my name with more respect. or at least less volume.", hash);
		return;
	}
	if (text.find("enrico") != std::string::npos || text.find("chivaldori") != std::string::npos)
	{
		MahjongBotQueueReply(1, "present. deposed, but present.", hash);
		return;
	}
	if (text.find("kingpin") != std::string::npos || text.find("klaus") != std::string::npos)
	{
		if (giMJSeat3Persona == 1) MahjongBotQueueReply(3, "I hear everything in this room anyway.", hash);
		else MahjongBotQueueReply(1, "careful. in San Mona the felt has ears.", hash);
		return;
	}

	// --- live game-state intents come first: the table answers questions ---
	if (gGame && gGame->phase() != MahjongGame::Phase::NotStarted)
	{
		if (text.find("what should i discard") != std::string::npos ||
			text.find("which tile") != std::string::npos)
		{
			if (guiMJState == MJUI_PLAYER_TURN && gGame->drawnTile() != MahjongGame::NO_TILE)
			{
				MahjongBotQueueReply(1, ST::format("between us? I would let the {} go.",
						MahjongTileLabel(gGame->AiChooseDiscard(0))).c_str(), hash);
			}
			else
			{
				MahjongBotQueueReply(2, "it is not even your turn. FOCUS.", hash);
			}
			return;
		}
		if (text.find("who is winning") != std::string::npos || text.find("whos winning") != std::string::npos)
		{
			int lead = 0;
			for (int i = 1; i < MahjongGame::NUM_PLAYERS; ++i)
			{
				if (gGame->player(i).score > gGame->player(lead).score) lead = i;
			}
			MahjongBotQueueReply(lead == 0 ? 2 : lead, lead == 0
					? "YOU are. enjoy it while it lasts."
					: ST::format("I am, with {}. obviously.", gGame->player(lead).score).c_str(), hash);
			return;
		}
		if (text.find("wall") != std::string::npos && text.find('?') == std::string::npos)
		{
			MahjongBotQueueReply(1, ST::format("{} tiles left in the wall, by my count.",
					gGame->wallRemaining()).c_str(), hash);
			return;
		}
		if (text.find("whose turn") != std::string::npos || text.find("who's turn") != std::string::npos)
		{
			MahjongBotQueueReply(3, gGame->currentPlayer() == 0
					? "yours!! it's yours!!" : ST::format("{}'s! I'm keeping track!!",
						MahjongSeatName(gGame->currentPlayer())).c_str(), hash);
			return;
		}
		if (text.find("tournament") != std::string::npos || text.find("invitational") != std::string::npos)
		{
			int const days = (7 - static_cast<int>(GetWorldDay() % 7)) % 7;
			MahjongBotQueueReply(1, days == 0
					? "tonight, friend. triple stakes. sharpen up."
					: ST::format("{} day{} until the invitational. save your money.",
						days, days == 1 ? "" : "s").c_str(), hash);
			return;
		}
	}

	// --- ELIZA rules: first match wins, replies cycle ---
	static UINT8 sCycle[sizeof(gMJElizaRules) / sizeof(gMJElizaRules[0])] = {};
	for (size_t i = 0; i < sizeof(gMJElizaRules) / sizeof(gMJElizaRules[0]); ++i)
	{
		MahjongElizaRule const& rule = gMJElizaRules[i];
		std::string cap;
		if (!MahjongMatchRule(text, rule.pat, cap)) continue;
		int const who = rule.who == -9 ? 1 + static_cast<int>(hash % 3) : rule.who;
		ST::string reply(rule.r[sCycle[i] % 3]);
		sCycle[i]++;
		if (reply.contains("{}"))
		{
			reply = reply.replace("{}", MahjongReflect(cap));
		}
		MahjongBotQueueReply(who, reply.c_str(), hash);
		// remember "my ..." topics for later
		std::string mem;
		if (MahjongMatchRule(text, "my *", mem) && !mem.empty()) gMJChatMemory = mem;
		return;
	}

	// --- memory fallback: earlier you mentioned... ---
	if (!gMJChatMemory.empty() && hash % 3 == 0)
	{
		MahjongBotQueueReply(1 + static_cast<int>(hash % 3),
				ST::format("earlier you mentioned your {}. how is that going?",
						MahjongReflect(gMJChatMemory)).c_str(), hash);
		gMJChatMemory.clear();
		return;
	}

	// --- xnone ---
	switch (hash % 4)
	{
		case 0:  MahjongBotQueueReply(2, "less chatter. more losing.", hash); break;
		case 1:  MahjongBotQueueReply(3, "haha yeah! totally!", hash); break;
		case 2:  MahjongBotQueueReply(1, "hm. the table hears you.", hash); break;
		default: MahjongBotQueueReply(2, "was that supposed to distract me?", hash); break;
	}
}


MahjongPersist MahjongGetPersist()
{
	MahjongPersist p;
	p.usMatches = static_cast<UINT16>(giMJStatMatches);
	p.usMatchesWon = static_cast<UINT16>(giMJStatMatchesWon);
	p.usHandsWon = static_cast<UINT16>(giMJStatHandsWon);
	p.iBiggestHand = giMJStatBiggestHand;
	p.iDollarsNet = giMJStatDollars;
	p.ubFlags = static_cast<UINT8>((gfMJElliotSecretSent ? 1 : 0) |
			((std::min(giMJElliotGoodNights, 7) & 7) << 1));
	p.ubGrudge = static_cast<UINT8>(std::min(giMJGrudge, 255));
	return p;
}


void MahjongSetPersist(const MahjongPersist& p)
{
	giMJStatMatches = p.usMatches;
	giMJStatMatchesWon = p.usMatchesWon;
	giMJStatHandsWon = p.usHandsWon;
	giMJStatBiggestHand = p.iBiggestHand;
	giMJStatDollars = p.iDollarsNet;
	gfMJElliotSecretSent = (p.ubFlags & 1) ? TRUE : FALSE;
	giMJElliotGoodNights = (p.ubFlags >> 1) & 7;
	giMJGrudge = p.ubGrudge;
}


bool MahjongHandleTypedKey(UINT32 usParam, UINT16 usKeyState)
{
	if (usKeyState & (CTRL_DOWN | ALT_DOWN)) return false;

	if (usParam == SDLK_RETURN || usParam == SDLK_KP_ENTER)
	{
		while (!gMJInput.empty() && gMJInput.back() == ' ') gMJInput.pop_back();
		if (gMJInput.empty()) return true;
		if (gMJInput == "/coach")
		{
			gfMJCoach = !gfMJCoach;
			MahjongSystemSay(gfMJCoach ? "coach marker on" : "coach marker off");
		}
		else if (gMJInput == "/table")
		{
			if (giMJStatMatchesWon < 3)
			{
				MahjongSystemSay("K.: the back room is for proven players. three match wins buys the key.");
			}
			else
			{
				gfMJBackRoom = !gfMJBackRoom;
				MahjongSystemSay(gfMJBackRoom
					? "K.: welcome to the back room. 5x stakes. mind the carpet."
					: "back to the floor tables");
			}
		}
		else if (gMJInput == "/stats")
		{
			MahjongSystemSay(ST::format("Session: {} matches, {} won | {} hands won",
					giMJStatMatches, giMJStatMatchesWon, giMJStatHandsWon));
			MahjongSystemSay(ST::format("Biggest hand: {} pts | Net: {}{}$",
					giMJStatBiggestHand, giMJStatDollars < 0 ? "-" : "",
					giMJStatDollars < 0 ? -giMJStatDollars : giMJStatDollars));
		}
		else if (MahjongNow() < guiMJMutedUntil)
		{
			// still muted: the client swallows it
			UINT32 const left = (guiMJMutedUntil - MahjongNow() + 999) / 1000;
			MahjongSystemSay(ST::format("you are muted for another {} seconds", left));
		}
		else
		{
			// the mod counts how fast you are talking
			UINT32 const now = MahjongNow();
			gubMJRecentPosts = now - guiMJLastPost < 4000 ? gubMJRecentPosts + 1 : 0;
			guiMJLastPost = now;
			if (gubMJRecentPosts >= 4)
			{
				gubMJRecentPosts = 0;
				if (gubMJOffences < 250) ++gubMJOffences;
				UINT32 const minutes = gubMJOffences;
				guiMJMutedUntil = now + minutes * 60000;
				static const char* const modLine[4] =
				{
					"please slow down!! I have to log these!!",
					"sorry!! that's the rules!! I don't make them!! (she does)",
					"I really really don't enjoy this part of the job",
					"I warned you!! well- I meant to warn you!!",
				};
				MahjongSay(3, modLine[MahjongChatRoll() % 4]);
				MahjongSystemSay(ST::format("@e11iot muted {} for {} minute{} (flooding)",
						MahjongChatHandle(0), minutes, minutes == 1 ? "" : "s"));
			}
			else
			{
				MahjongSay(0, ST::string(gMJInput.c_str()));
				MahjongBotConsider(gMJInput);
			}
		}
		gMJInput.clear();
		MahjongPlay(MJ_SND_SELECT, LOWVOLUME);
		return true;
	}
	if (usParam == SDLK_BACKSPACE)
	{
		if (!gMJInput.empty())
		{
			gMJInput.pop_back();
			MahjongRedraw();
		}
		return true;
	}
	// printable characters arrive as TEXT_INPUT events, which know the
	// keyboard layout; here they are only swallowed so no shortcut fires
	if (usParam >= 32 && usParam < 127) return true;
	return false;
}

// Layout-aware typing for the chat line, via the engine's TEXT_INPUT events.
bool MahjongHandleTextInput(const ST::utf32_buffer& codepoints)
{
	bool changed = false;
	for (char32_t cp : codepoints)
	{
		if (cp < 32 || cp > 126) continue;
		if (gMJInput.size() >= MJ_CHAT_INPUT_MAX) break;
		gMJInput += static_cast<char>(cp);
		changed = true;
	}
	if (changed) MahjongRedraw();
	return true;
}


static void MahjongChatArrowCallback(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (!(iReason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	INT32 const maxScroll = std::max(0, giMJContentH - MahjongChatViewH());
	INT32 const was = giMJScrollTarget;
	giMJScrollTarget = MSYS_GetRegionUserData(pRegion, 0) > 0
		? std::max(0, giMJScrollTarget - 14)
		: std::min(maxScroll, giMJScrollTarget + 14);
	gfMJSticky = giMJScrollTarget >= maxScroll - 6;
	if (giMJScrollTarget != was) MahjongPlay(MJ_SND_SELECT, LOWVOLUME);
	MahjongRedraw();
}


static void MahjongChatScrollCallback(MOUSE_REGION* pRegion, UINT32 iReason)
{
	INT32 const maxScroll = std::max(0, giMJContentH - MahjongChatViewH());
	INT32 const was = giMJScrollTarget;
	INT32 const step = 3 * 14; // a trackpad notch should actually travel
	if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		giMJScrollTarget = std::max(0, giMJScrollTarget - step);
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		giMJScrollTarget = std::min(maxScroll, giMJScrollTarget + step);
	}
	// stickiness is decided by where the user asked to be, never by the
	// animation's own writes
	gfMJSticky = giMJScrollTarget >= maxScroll - 6;
	if (giMJScrollTarget != was)
	{
		MahjongPlay(MJ_SND_SELECT, LOWVOLUME);
		MahjongRedraw();
	}
}


static void MahjongRulesDismissCallback(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (!(iReason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	// the rules modal pages instead of scrolling: catch the pager hotspots
	if (gfMJShowRules && giMJOverlayKind == 1)
	{
		INT32 const x = MJ_X(111), y = MJ_Y(40), w = 280, h = 280;
		if (gusMouseYPos >= y + h - 24 && gusMouseYPos <= y + h)
		{
			if (gusMouseXPos >= x + 6 && gusMouseXPos <= x + 60)
			{
				// no wrap-around, email style: the edge answers with a beep
				if (gbMJRulesPage > 0)
				{
					--gbMJRulesPage;
					MahjongPlay(MJ_SND_SELECT, BTNVOLUME);
					MahjongRedraw();
				}
				else MahjongPlay(SOUNDSDIR "/computer beep 01 out.wav", BTNVOLUME);
				return;
			}
			if (gusMouseXPos >= x + w - 60 && gusMouseXPos <= x + w - 6)
			{
				if (gbMJRulesPage < 3)
				{
					++gbMJRulesPage;
					MahjongPlay(MJ_SND_SELECT, BTNVOLUME);
					MahjongRedraw();
				}
				else MahjongPlay(SOUNDSDIR "/computer beep 01 out.wav", BTNVOLUME);
				return;
			}
		}
	}
	if (gfMJShowRules && giMJOverlayKind == 1 && gbMJRulesPage == 3)
	{
		INT32 const x = MJ_X(111), y = MJ_Y(40), w = 280, h = 280;
		INT32 const okX = x + (w - 96) / 2, okY = y + h - 56;
		if (gusMouseXPos >= okX && gusMouseXPos <= okX + 96 &&
			gusMouseYPos >= okY && gusMouseYPos <= okY + 20)
		{
			MahjongPlay(MJ_SND_SELECT, BTNVOLUME);
		}
	}
	MahjongSetOverlayPage(0);
}




static void MahjongSponsorCallback(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (!(iReason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	MahjongSetOverlayPage(0);
	GoToWebPage(BOBBYR_BOOKMARK); // our proud sponsor
}


static MahjongGame::TileId MahjongTileAtSlot(INT8 slot)
{
	if (slot == MJ_DRAWN_SLOT) return gGame->drawnTile();
	std::vector<MahjongGame::TileId> const hand = MahjongGame::SortedHand(gGame->player(0));
	if (slot < 0 || static_cast<size_t>(slot) >= hand.size()) return MahjongGame::NO_TILE;
	return hand[slot];
}


static void MahjongHandRegionCallback(MOUSE_REGION* pRegion, UINT32 iReason)
{
	if (!(iReason & MSYS_CALLBACK_REASON_POINTER_UP)) return;

	INT8 const slot = static_cast<INT8>(MSYS_GetRegionUserData(pRegion, 0));
	if (MahjongTileAtSlot(slot) == MahjongGame::NO_TILE) return;

	if (guiMJState == MJUI_EXCHANGE)
	{
		if (slot >= MahjongGame::HAND_TILES) return;
		gfMJExchangeSel[slot] = !gfMJExchangeSel[slot];
		MahjongPlay(MJ_SND_SELECT, LOWVOLUME);
		MahjongUpdateButtons();
		MahjongRedraw();
		return;
	}
	if (guiMJState != MJUI_PLAYER_TURN) return;

	if (gbMJSelectedSlot != slot)
	{
		gbMJSelectedSlot = slot;
		MahjongPlay(MJ_SND_SELECT, LOWVOLUME);
		MahjongRedraw();
		return;
	}

	// second click on the same slot: discard it
	MahjongGame::TileId const dumpedTile = MahjongTileAtSlot(slot);
	gGame->Discard(dumpedTile);
	gbMJSelectedSlot = -1;
	MahjongPlay(MJ_SND_DISCARD, BTNVOLUME);
	if (gbMJDecreeRank > 0 && !gfMJDecreeChecked && gGame->handNumber() == 0)
	{
		gfMJDecreeChecked = TRUE;
		if (dumpedTile % 9 + 1 == gbMJDecreeRank)
		{
			MahjongSay(2, "you DARE. the decree was CLEAR. luck will remember this.");
			MahjongBotQueueReply(3, "she's writing your name in the book. there's a BOOK.", MahjongChatRoll());
		}
	}
	MahjongAfterDiscard();
}


// NPC portraits use a 'b' prefix (b75.sti etc.), unlike merc portraits.
static SGPVObject* MahjongLoadFace(ProfileID id, const char* subdir)
{
	return AddVideoObjectFromFile(ST::format(FACESDIR "/{}/b{02d}.sti", subdir, GetProfile(id).ubFaceIndex));
}


static SGPVObject* MahjongLoadFaceForSlot(int i, const char* subdir)
{
	return MahjongLoadFace(gMJOpponentProfile[i], subdir);
}


static SGPVObject* MahjongLoadBigFaceForSlot(int i)
{
	return MahjongLoadBigFace(gMJOpponentProfile[i]);
}


static SGPVObject* MahjongLoadFaceForSlot(int i, const char* subdir);
static SGPVObject* MahjongLoadBigFaceForSlot(int i);

// per-face patch anchors, found by matching the patch art against the base
// portrait (prof.dat carries no usable coords for these NPCs)
static INT16 gMJFaceAuxXY[3][4]; // exEyes, eyEyes, mxMouth, myMouth (-1 = none)

// unpack one ETRLE subregion into a flat row-major buffer
// (SGPVObject::GetETRLEPixelValue mis-walks multi-row images, so decode here)
static std::vector<UINT8> MahjongDecodeSub(SGPVObject const* f, UINT16 sub, INT32& w, INT32& h)
{
	ETRLEObject const& e = f->SubregionProperties(sub);
	w = e.usWidth; h = e.usHeight;
	std::vector<UINT8> out(static_cast<size_t>(w) * h, 0);
	UINT8 const* p = f->PixData(e);
	for (INT32 y = 0; y < h; ++y)
	{
		INT32 x = 0;
		while (*p != 0)
		{
			UINT8 const c = *p++;
			UINT8 const len = c & 0x7F;
			if (c & 0x80) x += len;
			else for (UINT8 k = 0; k < len && x < w; ++k) out[y * w + x++] = *p++;
		}
		++p; // row terminator
	}
	return out;
}

static void MahjongCalibrateFacePatches(int i)
{
	gMJFaceAuxXY[i][0] = gMJFaceAuxXY[i][1] = gMJFaceAuxXY[i][2] = gMJFaceAuxXY[i][3] = -1;
	SGPVObject const* const f = guiMJFace65[i];
	if (!f || f->SubregionCount() < 8) return;

	INT32 bw, bh;
	std::vector<UINT8> const base = MahjongDecodeSub(f, 0, bw, bh);

	// slide the patch over the portrait; most of a blink/talk frame is
	// unchanged skin, so the true anchor is the global best match
	auto const align = [&](UINT16 sub, INT16* outX, INT16* outY)
	{
		INT32 pw, ph;
		std::vector<UINT8> const patch = MahjongDecodeSub(f, sub, pw, ph);
		if (pw >= bw || ph >= bh) return;
		INT32 best = INT32_MAX, bx = 0, by = 0;
		for (INT32 oy = 0; oy <= bh - ph; ++oy)
		{
			for (INT32 ox = 0; ox <= bw - pw; ++ox)
			{
				INT32 cost = 0;
				for (INT32 y = 0; y < ph && cost < best; ++y)
					for (INT32 x = 0; x < pw; ++x)
						if (patch[y * pw + x] != base[(oy + y) * bw + ox + x]) ++cost;
				if (cost < best) { best = cost; bx = ox; by = oy; }
			}
		}
		*outX = static_cast<INT16>(bx);
		*outY = static_cast<INT16>(by);
	};
	align(1, &gMJFaceAuxXY[i][0], &gMJFaceAuxXY[i][1]); // eye frame
	align(5, &gMJFaceAuxXY[i][2], &gMJFaceAuxXY[i][3]); // mouth frame
	fprintf(stderr, "MJFACE cal %d: eyes(%d,%d) mouth(%d,%d)\n", i,
		gMJFaceAuxXY[i][0], gMJFaceAuxXY[i][1], gMJFaceAuxXY[i][2], gMJFaceAuxXY[i][3]);
}

// stretch-blit needs a 16bpp source, so each 29x33 face gets baked once
static void MahjongBakeChip(int i)
{
	if (guiMJChipSurf[i]) { DeleteVideoSurface(guiMJChipSurf[i]); guiMJChipSurf[i] = nullptr; }
	if (!guiMJFace33[i]) return;
	guiMJChipSurf[i] = AddVideoSurface(MJ_FACE33_W, MJ_FACE33_H, PIXEL_DEPTH);
	guiMJChipSurf[i]->Fill(Get16BPPColor(FROMRGB(9, 34, 21)));
	BltVideoObject(guiMJChipSurf[i], guiMJFace33[i], 0, 0, 0);
}

static void MahjongReloadSeat3Faces()
{
	if (guiMJFace65[2]) { DeleteVideoObject(guiMJFace65[2]); guiMJFace65[2] = nullptr; }
	if (guiMJFace33[2]) { DeleteVideoObject(guiMJFace33[2]); guiMJFace33[2] = nullptr; }
	if (guiMJBigFace[2]) { DeleteVideoObject(guiMJBigFace[2]); guiMJBigFace[2] = nullptr; }
	try
	{
		guiMJFace65[2] = MahjongLoadFaceForSlot(2, "65face");
		guiMJFace33[2] = MahjongLoadFaceForSlot(2, "33face");
		guiMJBigFace[2] = MahjongLoadBigFaceForSlot(2);
	}
	catch (...)
	{
		// face art missing: the feed falls back to a drawn box
	}
	MahjongBakeChip(2);
	MahjongCalibrateFacePatches(2);
}

static void MahjongReloadSeat2Faces()
{
	if (guiMJFace65[1]) { DeleteVideoObject(guiMJFace65[1]); guiMJFace65[1] = nullptr; }
	if (guiMJFace33[1]) { DeleteVideoObject(guiMJFace33[1]); guiMJFace33[1] = nullptr; }
	if (guiMJBigFace[1]) { DeleteVideoObject(guiMJBigFace[1]); guiMJBigFace[1] = nullptr; }
	try
	{
		guiMJFace65[1] = MahjongLoadFaceForSlot(1, "65face");
		guiMJFace33[1] = MahjongLoadFaceForSlot(1, "33face");
		guiMJBigFace[1] = MahjongLoadBigFaceForSlot(1);
	}
	catch (...)
	{
		// face art missing: the feed falls back to a drawn box
	}
	MahjongBakeChip(1);
	MahjongCalibrateFacePatches(1);
}


static SGPVObject* MahjongLoadBigFace(ProfileID id)
{
	MERCPROFILESTRUCT const& p = GetProfile(id);
	// Elliot's talking head bruises with his canon slap count (see Faces.cc)
	if (id == ELLIOT && p.bNPCData > 3)
	{
		char const suffix = p.bNPCData < 7 ? 'a' : p.bNPCData < 10 ? 'b' :
					p.bNPCData < 13 ? 'c' : p.bNPCData < 16 ? 'd' : 'e';
		return AddVideoObjectFromFile(ST::format(FACESDIR "/b{02d}{}.sti", p.ubFaceIndex, suffix));
	}
	return AddVideoObjectFromFile(ST::format(FACESDIR "/b{02d}.sti", p.ubFaceIndex));
}


// map the core phase back onto a UI state (page re-entry, lobby Play tile)
static void MahjongResumeTableState()
{
	switch (gGame->phase())
	{
		case MahjongGame::Phase::ExchangeSelect:
			MahjongEnterState(MJUI_EXCHANGE);
			break;
		case MahjongGame::Phase::ChooseVoid:
			MahjongEnterState(MJUI_CHOOSE_VOID);
			break;
		case MahjongGame::Phase::AwaitDraw:
		case MahjongGame::Phase::AwaitDiscard:
			MahjongEnterState(gGame->currentPlayer() == 0 ? MJUI_PLAYER_TURN : MJUI_AI_THINK);
			break;
		case MahjongGame::Phase::RonWindow:
			MahjongAfterDiscard(); // re-arbitrate ron/pong/kong claims
			break;
		case MahjongGame::Phase::HandEnd:
			MahjongEnterState(MJUI_HAND_END);
			break;
		case MahjongGame::Phase::MatchEnd:
			MahjongEnterState(MJUI_MATCH_END);
			break;
		default:
			MahjongEnterState(MJUI_IDLE);
			break;
	}
}

static void MahjongIconCallback(MOUSE_REGION* pRegion, UINT32 reason);

// chat-bar regions live in one place so immersive mode can re-seat them
static void MahjongPlaceChatRegions(bool fFirst)
{
	if (!fFirst)
	{
		MSYS_RemoveRegion(&gMJChatRegion);
		MSYS_RemoveRegion(&gMJChatUpRegion);
		MSYS_RemoveRegion(&gMJChatDownRegion);
		for (MOUSE_REGION& r : gMJIconRegion) MSYS_RemoveRegion(&r);
	}
	INT32 const top = MahjongBarTop(), hh = MahjongBarH();
	MSYS_DefineRegion(&gMJChatRegion, static_cast<UINT16>(MJ_X(2)), static_cast<UINT16>(MJ_Y(top)),
				static_cast<UINT16>(MJ_X(498)), static_cast<UINT16>(MJ_Y(top + hh)),
				MSYS_PRIORITY_NORMAL, CURSOR_WWW, MSYS_NO_CALLBACK, MahjongChatScrollCallback);
	MSYS_DefineRegion(&gMJChatUpRegion, static_cast<UINT16>(MJ_X(MJ_CHAT_W - 14)), static_cast<UINT16>(MJ_Y(top + 2)),
				static_cast<UINT16>(MJ_X(MJ_CHAT_W - 1)), static_cast<UINT16>(MJ_Y(top + 14)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, MahjongChatArrowCallback);
	MSYS_SetRegionUserData(&gMJChatUpRegion, 0, 1);
	// the down arrow moved up above the footer strip: its hit box follows
	MSYS_DefineRegion(&gMJChatDownRegion, static_cast<UINT16>(MJ_X(MJ_CHAT_W - 14)), static_cast<UINT16>(MJ_Y(top + hh - 36)),
				static_cast<UINT16>(MJ_X(MJ_CHAT_W - 1)), static_cast<UINT16>(MJ_Y(top + hh - 24)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, MahjongChatArrowCallback);
	MSYS_SetRegionUserData(&gMJChatDownRegion, 0, -1);
	static const char* const iconHelp[4] = { "Parlour home page", "Guestbook", "House rules", "Expand / collapse chat" };
	for (int i = 0; i < 4; ++i)
	{
		UINT16 const ix = static_cast<UINT16>(MJ_X(4));
		// the toggle bubble never moves: same spot in either mode
		UINT16 const iy = static_cast<UINT16>(i == 3
				? MJ_Y(MJ_CHAT_Y + 6 + 3 * 22) : MJ_Y(top + 6 + i * 22));
		MSYS_DefineRegion(&gMJIconRegion[i], ix, iy, ix + 16, iy + 16,
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, MahjongIconCallback);
		MSYS_SetRegionUserData(&gMJIconRegion[i], 0, i);
		gMJIconRegion[i].SetFastHelpText(iconHelp[i]);
	}
}

// --- the home page hotspots ----------------------------------------------
static void MahjongLobbyTileCallback(MOUSE_REGION* pRegion, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (guiMJState != MJUI_LOBBY) return;
	switch (MSYS_GetRegionUserData(pRegion, 0))
	{
		case 0: // take a seat
			if (gGame && !gfMJExhibition && gGame->phase() != MahjongGame::Phase::NotStarted &&
				gGame->phase() != MahjongGame::Phase::MatchEnd)
			{
				MahjongResumeTableState();
			}
			else
			{
				MahjongStartMatch();
			}
			break;
		case 1: // watch table 1
			MahjongEnterState(MJUI_IDLE);
			break;
		case 2: // the ladder
			MahjongEnterState(MJUI_LADDER);
			break;
		case 3: // house rules
			MahjongEnterState(MJUI_IDLE);
			gfMJShowRules = TRUE;
			giMJOverlayKind = 1;
			gMJRulesDismissRegion.Enable();
			break;
	}
	MahjongUpdateButtons();
	MahjongRedraw();
}

static void MahjongLadderBackCallback(MOUSE_REGION* pRegion, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (guiMJState != MJUI_LADDER) return;
	MahjongEnterState(MJUI_LOBBY);
	MahjongUpdateButtons();
	MahjongRedraw();
}

static void BtnMahjongReportCallback(GUI_BUTTON* btn, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	if (gubMJReportCount < 250) ++gubMJReportCount;
	MahjongRedraw();
}

static void MahjongPlaceChatRegions(bool fFirst);
static void MahjongEnableHandRegions(bool enable);

static void MahjongIconCallback(MOUSE_REGION* pRegion, UINT32 reason)
{
	if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
	switch (MSYS_GetRegionUserData(pRegion, 0))
	{
		case 0:
			MahjongEnterState(MJUI_LOBBY);
			MahjongUpdateButtons();
			MahjongRedraw();
			break;
		case 1: MahjongSetOverlayPage(2); break;
		case 2: MahjongSetOverlayPage(1); break;
		case 3:
			// immersive mode: the bottom bar takes over the page
			gfMJChatBig = !gfMJChatBig;
			MahjongPlaceChatRegions(false);
			MahjongEnableHandRegions(!gfMJChatBig && guiMJState == MJUI_PLAYER_TURN);
			MahjongPlay(MJ_SND_SELECT, BTNVOLUME);
			MahjongUpdateButtons();
			MahjongRedraw();
			break;
	}
}

void EnterMahjong()
{
	{
		// the player answered the ad: the spam chain stops here
		MahjongPersist mj = MahjongGetPersist();
		if (!(mj.ubFlags & 0x20)) { mj.ubFlags |= 0x20; MahjongSetPersist(mj); }
	}
	guiMJTiles = nullptr;
	guiMJTilesSmall = nullptr;
	guiMJFelt = nullptr;
	guiMJLogo = nullptr;
	guiMJChips = nullptr;
	guiMJSelfFace = nullptr;
	guiMJSelfFaceSurf = nullptr;
	guiMJStatic = nullptr;
	guiMJFeltRed = nullptr;
	guiMJDragon = nullptr;
	guiMJGlow = nullptr;
	guiMJRail = nullptr;
	guiMJKingpinFace = nullptr;
	guiMJSign = nullptr;
	try { guiMJKingpinFace = MahjongLoadFace(KINGPIN, "65face"); } catch (...) {}
	try { guiMJSign = AddVideoObjectFromFile("sti/laptop/mahjongsign.sti"); } catch (...) {}
	try { guiMJVoidIcon = AddVideoObjectFromFile("sti/laptop/mahjongvoid.sti"); } catch (...) {}
	giMJGlitchWho = -1;
	guiMJNextGlitch = MahjongNow() + 6000;
	try
	{
		guiMJTiles      = AddVideoObjectFromFile("sti/laptop/mahjongtiles.sti");
		guiMJFeltRed    = AddVideoObjectFromFile("sti/laptop/mahjongfeltred.sti");
		guiMJGlow       = AddVideoObjectFromFile("sti/laptop/mahjongglow.sti");
		guiMJRail       = AddVideoObjectFromFile("sti/laptop/mahjongrail.sti");
		guiMJDragon     = AddVideoObjectFromFile("sti/laptop/mahjongdragon.sti");
		guiMJTilesSmall = AddVideoObjectFromFile("sti/laptop/mahjongtilessmall.sti");
		guiMJFelt       = AddVideoObjectFromFile("sti/laptop/mahjongfelt.sti");
		guiMJLogo       = AddVideoObjectFromFile("sti/laptop/mahjonglogo.sti");
		guiMJChips      = AddVideoObjectFromFile("sti/laptop/mahjongchips.sti");
		if (LaptopSaveInfo.fIMPCompletedFlag)
		{
			MERCPROFILESTRUCT const& imp = GetProfile(
					static_cast<ProfileID>(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId));
			guiMJSelfFace = Load65Portrait(imp);
			gMJSelfNick = imp.zNickname;
			// bake the big portrait into a 16bpp surface for stretch-blitting
			SGPVObject* const big = LoadBigPortrait(imp);
			guiMJSelfFaceSurf = AddVideoSurface(106, 122, PIXEL_DEPTH);
			BltVideoObject(guiMJSelfFaceSurf, big, 0, 0, 0);
			DeleteVideoObject(big);
		}
		guiMJStatic     = AddVideoObjectFromFile("sti/laptop/mahjongstatic.sti");
	}
	catch (...)
	{
		// sheets missing: tiles are drawn as text placeholders, felt stays flat
	}

	for (int i = 0; i < 3; ++i)
	{
		guiMJFace65[i] = nullptr;
		guiMJFace33[i] = nullptr;
		guiMJBigFace[i] = nullptr;
		try
		{
			guiMJFace65[i] = MahjongLoadFace(gMJOpponentProfile[i], "65face");
			guiMJFace33[i] = MahjongLoadFace(gMJOpponentProfile[i], "33face");
			guiMJBigFace[i] = MahjongLoadBigFace(gMJOpponentProfile[i]);
		}
		catch (...)
		{
			// face art missing: panels fall back to drawn boxes
		}
		MahjongBakeChip(i);
		MahjongCalibrateFacePatches(i);
	}

	for (INT32 i = 0; i < MJ_NUM_HAND_SLOTS; ++i)
	{
		UINT16 const x = static_cast<UINT16>(MJ_X(i == MJ_DRAWN_SLOT ? MJ_DRAWN_X : MJ_HAND_X + i * MJ_TILE_PITCH));
		UINT16 const y = static_cast<UINT16>(MJ_Y(MJ_HAND_Y - MJ_HAND_RAISE));
		MSYS_DefineRegion(&gMJHandRegion[i], x, y,
					static_cast<UINT16>(x + MJ_TILE_W),
					static_cast<UINT16>(y + MJ_TILE_H + MJ_HAND_RAISE),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					MahjongHandRegionCallback);
		MSYS_SetRegionUserData(&gMJHandRegion[i], 0, i);
	}

	guiMJNewGameBtn = CreateTextButton("Deal Me In", FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK,
					MJ_X(201), MJ_Y(MJ_SETUP_BTN_Y), 100, 22, MSYS_PRIORITY_HIGH, BtnMahjongNewGameCallback);
	guiMJNewGameBtn->SetCursor(CURSOR_WWW);
	SpecifyButtonSoundScheme(guiMJNewGameBtn, BUTTON_SOUND_SCHEME_COMPUTERBEEP2);

	guiMJMahjongBtn = CreateTextButton("Mahjong!", FONT12ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK,
					MJ_X(MJ_CLAIM_BTN_X), MJ_Y(MJ_CLAIM_BTN_Y), 120, 24, MSYS_PRIORITY_HIGH, BtnMahjongMahjongCallback);
	guiMJMahjongBtn->SetCursor(CURSOR_WWW);
	SpecifyButtonSoundScheme(guiMJMahjongBtn, BUTTON_SOUND_SCHEME_COMPUTERBEEP2);

	for (int s = 0; s < 3; ++s)
	{
		MSYS_DefineRegion(&gMJVoidRegion[s],
				static_cast<UINT16>(MJ_X(102 + s * 104)), static_cast<UINT16>(MJ_Y(MJ_SETUP_BTN_Y)),
				static_cast<UINT16>(MJ_X(102 + s * 104 + 98)), static_cast<UINT16>(MJ_Y(MJ_SETUP_BTN_Y + 22)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MahjongHoverRedrawCallback, MahjongVoidPickCallback);
		MSYS_SetRegionUserData(&gMJVoidRegion[s], 0, s);
		gMJVoidRegion[s].Disable();
	}

	guiMJPassBtn = CreateTextButton("Pass 3 Tiles", FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK,
					MJ_X(191), MJ_Y(MJ_SETUP_BTN_Y), 120, 22, MSYS_PRIORITY_HIGH, BtnMahjongPassCallback);
	guiMJPassBtn->SetCursor(CURSOR_WWW);
	SpecifyButtonSoundScheme(guiMJPassBtn, BUTTON_SOUND_SCHEME_COMPUTERBEEP2);

	guiMJPongBtn = CreateTextButton("Pong!", FONT12ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK,
					MJ_X(MJ_CLAIM_BTN_X), MJ_Y(MJ_CLAIM_BTN_Y), 88, 24, MSYS_PRIORITY_HIGH, BtnMahjongPongCallback);
	guiMJPongBtn->SetCursor(CURSOR_WWW);
	SpecifyButtonSoundScheme(guiMJPongBtn, BUTTON_SOUND_SCHEME_COMPUTERBEEP2);

	guiMJKongBtn = CreateTextButton("Kong!", FONT12ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK,
					MJ_X(150), MJ_Y(MJ_CLAIM_BTN_Y), 88, 24, MSYS_PRIORITY_HIGH, BtnMahjongKongCallback);
	guiMJKongBtn->SetCursor(CURSOR_WWW);
	SpecifyButtonSoundScheme(guiMJKongBtn, BUTTON_SOUND_SCHEME_COMPUTERBEEP2);

	guiMJLeaveBtn = CreateTextButton("Leave Table", FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK,
					MJ_X(75), MJ_Y(258), 100, 22, MSYS_PRIORITY_HIGH, BtnMahjongLeaveCallback);
	guiMJLeaveBtn->SetCursor(CURSOR_WWW);
	SpecifyButtonSoundScheme(guiMJLeaveBtn, BUTTON_SOUND_SCHEME_COMPUTERBEEP2);

	MahjongPlaceChatRegions(true);

	MSYS_DefineRegion(&gMJSponsorRegion, static_cast<UINT16>(MJ_X(55)), static_cast<UINT16>(MJ_Y(310)),
				static_cast<UINT16>(MJ_X(447)), static_cast<UINT16>(MJ_Y(330)),
				MSYS_PRIORITY_HIGH + 3, CURSOR_WWW, MSYS_NO_CALLBACK, MahjongSponsorCallback);
	gMJSponsorRegion.Disable();

	MSYS_DefineRegion(&gMJModalCTARegion, static_cast<UINT16>(MJ_X(149)), static_cast<UINT16>(MJ_Y(272)),
				static_cast<UINT16>(MJ_X(353)), static_cast<UINT16>(MJ_Y(302)),
				MSYS_PRIORITY_HIGH + 3, CURSOR_WWW, MahjongHoverRedrawCallback, MahjongModalCTACallback);
	gMJModalCTARegion.Disable();

	static const INT16 sLobbyBox[4][2] = { { 61, 124 }, { 255, 124 }, { 61, 210 }, { 255, 210 } };
	for (int i = 0; i < 4; ++i)
	{
		MSYS_DefineRegion(&gMJLobbyRegion[i],
				static_cast<UINT16>(MJ_X(sLobbyBox[i][0])), static_cast<UINT16>(MJ_Y(sLobbyBox[i][1])),
				static_cast<UINT16>(MJ_X(sLobbyBox[i][0] + 186)), static_cast<UINT16>(MJ_Y(sLobbyBox[i][1] + 74)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, MahjongLobbyTileCallback);
		MSYS_SetRegionUserData(&gMJLobbyRegion[i], 0, i);
		gMJLobbyRegion[i].Disable();
	}
	MSYS_DefineRegion(&gMJLadderBackRegion,
			static_cast<UINT16>(MJ_X(58)), static_cast<UINT16>(MJ_Y(46)),
			static_cast<UINT16>(MJ_X(86)), static_cast<UINT16>(MJ_Y(68)),
			MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK, MahjongLadderBackCallback);
	gMJLadderBackRegion.SetFastHelpText("Back to the parlour");
	gMJLadderBackRegion.Disable();

	guiMJReportBtn = CreateTextButton("Report a Cheater", FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK,
					MJ_X(298), MJ_Y(316), 136, 18, MSYS_PRIORITY_HIGH, BtnMahjongReportCallback);
	guiMJReportBtn->SetCursor(CURSOR_WWW);
	SpecifyButtonSoundScheme(guiMJReportBtn, BUTTON_SOUND_SCHEME_COMPUTERBEEP2);
	HideButton(guiMJReportBtn);


	MSYS_DefineRegion(&gMJRulesDismissRegion, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
				LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y,
				MSYS_PRIORITY_HIGH + 2, CURSOR_WWW, MSYS_NO_CALLBACK, MahjongRulesDismissCallback);
	gMJRulesDismissRegion.Disable();
	gfMJShowRules = FALSE;

	// the hit counter only ever goes up
	if (guiMJVisitorNo == 0) guiMJVisitorNo = 1337 + GetWorldDay() * 3;
	++guiMJVisitorNo;

	// Re-entering with a live session: restore a sensible UI state.
	if (gGame && gGame->phase() != MahjongGame::Phase::NotStarted && !gfMJExhibition)
	{
		// a live match of yours: skip the lobby, straight to your seat
		MahjongResumeTableState();
	}
	else
	{
		// nobody at the table: The House plays an exhibition four-hander
		// behind the home page - visitors land in the lobby
		if (!gfMJExhibition) MahjongStartExhibition();
		MahjongEnterState(MJUI_LOBBY);
	}

	SetBookMark(MAHJONG_BOOKMARK);
}


void ExitMahjong()
{
	if (guiMJTiles)      { DeleteVideoObject(guiMJTiles);      guiMJTiles = nullptr; }
	if (guiMJTilesSmall) { DeleteVideoObject(guiMJTilesSmall); guiMJTilesSmall = nullptr; }
	if (guiMJFelt)       { DeleteVideoObject(guiMJFelt);       guiMJFelt = nullptr; }
	if (guiMJLogo)       { DeleteVideoObject(guiMJLogo);       guiMJLogo = nullptr; }
	if (guiMJChips)      { DeleteVideoObject(guiMJChips);      guiMJChips = nullptr; }
	if (guiMJSelfFace)   { DeleteVideoObject(guiMJSelfFace);   guiMJSelfFace = nullptr; }
	if (guiMJSelfFaceSurf) { DeleteVideoSurface(guiMJSelfFaceSurf); guiMJSelfFaceSurf = nullptr; }
	if (guiMJShillSurf) { DeleteVideoSurface(guiMJShillSurf); guiMJShillSurf = nullptr; }
	if (guiMJVisitorSurf) { DeleteVideoSurface(guiMJVisitorSurf); guiMJVisitorSurf = nullptr; }
	gMJVisitorHandle.clear();
	gfMJVisitorHere = FALSE;
	if (guiMJStatic)     { DeleteVideoObject(guiMJStatic);     guiMJStatic = nullptr; }
	if (guiMJFeltRed)    { DeleteVideoObject(guiMJFeltRed);    guiMJFeltRed = nullptr; }
	if (guiMJDragon)     { DeleteVideoObject(guiMJDragon);     guiMJDragon = nullptr; }
	if (guiMJGlow)       { DeleteVideoObject(guiMJGlow);       guiMJGlow = nullptr; }
	if (guiMJRail)       { DeleteVideoObject(guiMJRail);       guiMJRail = nullptr; }
	if (guiMJKingpinFace) { DeleteVideoObject(guiMJKingpinFace); guiMJKingpinFace = nullptr; }
	if (guiMJSign) { DeleteVideoObject(guiMJSign); guiMJSign = nullptr; }
	if (guiMJVoidIcon) { DeleteVideoObject(guiMJVoidIcon); guiMJVoidIcon = nullptr; }
	for (int i = 0; i < 3; ++i)
	{
		if (guiMJFace65[i]) { DeleteVideoObject(guiMJFace65[i]); guiMJFace65[i] = nullptr; }
		if (guiMJFace33[i]) { DeleteVideoObject(guiMJFace33[i]); guiMJFace33[i] = nullptr; }
		if (guiMJBigFace[i]) { DeleteVideoObject(guiMJBigFace[i]); guiMJBigFace[i] = nullptr; }
		if (guiMJChipSurf[i]) { DeleteVideoSurface(guiMJChipSurf[i]); guiMJChipSurf[i] = nullptr; }
	}
	FOR_EACH(MOUSE_REGION, i, gMJHandRegion) MSYS_RemoveRegion(&*i);
	MSYS_RemoveRegion(&gMJRulesDismissRegion);
	MSYS_RemoveRegion(&gMJSponsorRegion);
	MSYS_RemoveRegion(&gMJModalCTARegion);
	MSYS_RemoveRegion(&gMJChatRegion);
	for (MOUSE_REGION& r : gMJLobbyRegion) MSYS_RemoveRegion(&r);
	MSYS_RemoveRegion(&gMJLadderBackRegion);
	RemoveButton(guiMJReportBtn);
	RemoveButton(guiMJNewGameBtn);
	RemoveButton(guiMJMahjongBtn);
	RemoveButton(guiMJPassBtn);
	RemoveButton(guiMJPongBtn);
	RemoveButton(guiMJKongBtn);
	RemoveButton(guiMJLeaveBtn);
	for (MOUSE_REGION& r : gMJIconRegion) MSYS_RemoveRegion(&r);
	MSYS_RemoveRegion(&gMJChatUpRegion);
	MSYS_RemoveRegion(&gMJChatDownRegion);
	for (MOUSE_REGION& r : gMJVoidRegion) MSYS_RemoveRegion(&r);
	gfMJShowRules = FALSE;
	MahjongDestroyOverlayFace();
	// gGame intentionally kept: the table waits for the player's return
}


static bool MahjongGameLive()
{
	if (!gGame || gGame->phase() == MahjongGame::Phase::NotStarted) return false;
	return guiMJState != MJUI_IDLE || gfMJExhibition;
}


// seat winds rotate with the dealer (dealer = East)
static const char* MahjongSeatWind(int player)
{
	static const char* const wind[4] = { "E", "S", "W", "N" };
	return wind[(player - gGame->dealer() + 4) % 4];
}

static void MahjongDrawPanelFrame(INT32 x, INT32 y, INT32 w, INT32 h)
{
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + w, y + h, Get16BPPColor(FROMRGB(30, 60, 40)));
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 2, y + 2, x + w - 2, y + h - 2, Get16BPPColor(FROMRGB(10, 60, 32)));
}

// active-turn marker: a tight ring around the portrait itself. the feed
// blits the full face rect afterwards, so only the 2px rim survives.
static void MahjongDrawTurnRing(INT32 fx, INT32 fy)
{
	ColorFillVideoSurfaceArea(FRAME_BUFFER, fx - 2, fy - 2, fx + MJ_FACE65_W + 2, fy + MJ_FACE65_H + 2,
				Get16BPPColor(FROMRGB(240, 220, 60)));
}

static void MahjongDrawStar(INT32 sx, INT32 sy, UINT16 col);

// void-suit badge: top-right corner of the portrait, red glyph on dark.
// Once the seat has won, the sentence is served - the glyph retires into
// a silver star, kin to the dotted outline.
static void MahjongDrawVoidBadge(int seat, INT32 fx, INT32 fy)
{
	if (!MahjongGameLive() || !guiMJVoidIcon) return;
	int const suit = gGame->player(seat).voidSuit;
	if (suit == MahjongGame::NO_SUIT) return;
	ColorFillVideoSurfaceArea(FRAME_BUFFER, fx + MJ_FACE65_W - 16, fy, fx + MJ_FACE65_W, fy + 16,
				Get16BPPColor(FROMRGB(6, 36, 20)));
	if (MahjongWinOrderOf(seat) >= 0)
	{
		MahjongDrawStar(fx + MJ_FACE65_W - 13, fy + 3,
				Get16BPPColor(FROMRGB(216, 220, 226)));
		return;
	}
	BltVideoObject(FRAME_BUFFER, guiMJVoidIcon, static_cast<UINT16>(suit), fx + MJ_FACE65_W - 14, fy - 1);
}

static void MahjongDrawWonOutline(int seat, INT32 fx, INT32 fy)
{
	if (!MahjongGameLive() || MahjongWinOrderOf(seat) < 0) return;
	UINT16 const silver = Get16BPPColor(FROMRGB(216, 220, 226));
	for (INT32 dx = -2; dx < MJ_FACE65_W + 2; dx += 4)
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, fx + dx, fy - 2,
				fx + dx + 2, fy, silver);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, fx + dx, fy + MJ_FACE65_H + 1,
				fx + dx + 2, fy + MJ_FACE65_H + 3, silver);
	}
	for (INT32 dy = 2; dy < MJ_FACE65_H - 2; dy += 4)
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, fx - 2, fy + dy,
				fx, fy + dy + 2, silver);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, fx + MJ_FACE65_W, fy + dy,
				fx + MJ_FACE65_W + 2, fy + dy + 2, silver);
	}
}

// seat wind badge: bottom-right corner of the portrait, system green
static void MahjongDrawWindBadge(int seat, INT32 fx, INT32 fy)
{
	if (!MahjongGameLive()) return;
	// box hugs the glyph: W is wider than E and the badge should match
	ST::string const wind = MahjongSeatWind(seat);
	INT32 const gw = StringPixLength(wind, FONT10ARIAL);
	INT32 const bx = fx + MJ_FACE65_W - gw - 5;
	ColorFillVideoSurfaceArea(FRAME_BUFFER, bx, fy + MJ_FACE65_H - 12,
				fx + MJ_FACE65_W, fy + MJ_FACE65_H, Get16BPPColor(FROMRGB(6, 36, 20)));
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
	SetFontForegroundRGB(MJ_TOKEN_RGB);
	MPrint(bx + 3, fy + MJ_FACE65_H - 9, wind);
}


static void MahjongPrintPlayerLines(int player, INT32 x, INT32 y, INT32 w, INT32 scoreY)
{
	int const opponent = player - 1;
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
	MPrint(x, y, MahjongSeatName(player));
	if (MahjongGameLive())
	{
		// the ladder rating rides the name line, right-aligned
		ST::string const rating = ST::format("{}", MahjongSeatRating(player));
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKGRAY, FONT_MCOLOR_BLACK, 0);
		MPrint(x + w - StringPixLength(rating, FONT10ARIAL), y, rating);
	}
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKGRAY, FONT_MCOLOR_BLACK, 0);
	MPrint(x, y + 11, MahjongSeatHandle(player));

	if (!MahjongGameLive()) return;
	MahjongGame::Player const& p = gGame->player(player);

	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
	SetFontForegroundRGB(MJ_TOKEN_RGB);
	if (guiMJChips) BltVideoObject(FRAME_BUFFER, guiMJChips, 1, x, scoreY - 3);
	MPrint(x + (guiMJChips ? 20 : 0), scoreY, ST::format("{}", p.score));

	int const order = MahjongWinOrderOf(player);
	if (order >= 0)
	{
		MahjongGame::WinEvent const& e = gGame->wins()[static_cast<size_t>(order)];
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
		MPrint(x, y + 39, e.winningTile != MahjongGame::NO_TILE
				? ST::format("WON #{} - {}", order + 1, MahjongTileLabel(e.winningTile))
				: ST::format("WON #{}", order + 1));
	}
	(void)opponent;
}


static bool MahjongIsPlayersTurn(int player)
{
	return MahjongGameLive() &&
		guiMJState != MJUI_HAND_END && guiMJState != MJUI_MATCH_END &&
		!gGame->player(player).finished &&
		gGame->currentPlayer() == player;
}


// which aux subimage a feed shows right now: 0 = none, 1-2 = eye frames,
// 5-7 = mouth frames (same sheet layout the tactical face system uses)
static UINT16 MahjongFaceAuxFrame(int opponent)
{
	UINT32 const now = MahjongNow();
	if (guiMJBlinkAt[opponent] && now >= guiMJBlinkAt[opponent] && now < guiMJBlinkAt[opponent] + 160)
		return 1 + static_cast<UINT16>((now - guiMJBlinkAt[opponent]) / 80);
	// occasional facial tick: a quick squint or twitch, never sustained
	UINT32 const beat = now / 140 + static_cast<UINT32>(opponent) * 37;
	if (beat % 67 == 13) return 2;
	if (beat % 101 == 51) return 5;
	return 0;
}

// keep the eyes moving: schedule blinks, repaint when a frame flips
static void MahjongHandleFaceLife()
{
	if (!MahjongGameLive()) return;
	UINT32 const now = MahjongNow();
	UINT32 key = 0;
	for (int i = 0; i < 3; ++i)
	{
		if (guiMJBlinkAt[i] == 0) guiMJBlinkAt[i] = now + 1200 + (now * (i * 5 + 3)) % 3600;
		else if (now >= guiMJBlinkAt[i] + 160) guiMJBlinkAt[i] = now + 1800 + ((now / 3) * (i * 7 + 5)) % 4200;
		key = key * 16 + MahjongFaceAuxFrame(i);
	}
	static UINT32 lastKey = 0;
	if (key != lastKey)
	{
		lastKey = key;
		MahjongRedraw();
	}
}

// a dial-up spinner: dots chasing each other round the feed
static void MahjongDrawSpinner(INT32 cx, INT32 cy, INT32 r)
{
	UINT32 const phase = MahjongNow() / 90;
	for (INT32 i = 0; i < 8; ++i)
	{
		double const a = i * 3.14159265 / 4.0;
		INT32 const px = cx + static_cast<INT32>(r * std::cos(a));
		INT32 const py = cy + static_cast<INT32>(r * std::sin(a));
		INT32 const age = (i - static_cast<INT32>(phase % 8) + 8) % 8;
		UINT16 const col = age < 2 ? Get16BPPColor(MJ_TOKEN_RGB)
				: age < 4 ? Get16BPPColor(FROMRGB(40, 96, 58))
				: Get16BPPColor(FROMRGB(18, 60, 38));
		ColorFillVideoSurfaceArea(FRAME_BUFFER, px - 1, py - 1, px + 2, py + 2, col);
	}
}

// the portrait boxes are "video feeds": static until a game connects, and
// brief dropouts while playing
static void MahjongDrawFeed(int opponent, INT32 x, INT32 y, bool large)
{
	bool const disconnected = !MahjongGameLive();
	// if Deidranna is dead, her feed never resolves - yet the account plays on
	bool const glitching = giMJGlitchWho == opponent + 1 ||
				(opponent == 1 && giMJSeat2Persona == MJP2_QUEEN && GetProfile(QUEEN).bLife == 0 && MahjongGameLive());
	UINT16 const base = large ? 0 : 4; // sheet: 0-3 = 58x65 frames, 4-7 = 29x33
	SGPVObject const* const face = large ? guiMJFace65[opponent] : guiMJFace33[opponent];
	if (disconnected && guiMJStatic)
	{
		// nobody dialed in yet: dark empty-seat silhouette
		BltVideoObject(FRAME_BUFFER, guiMJStatic, base + 3, x, y);
	}
	else if (glitching)
	{
		// their line dropped: the feed reconnects in front of everyone
		INT32 const w2 = large ? MJ_FACE65_W : MJ_FACE33_W;
		INT32 const h2 = large ? MJ_FACE65_H : MJ_FACE33_H;
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + w2, y + h2, Get16BPPColor(FROMRGB(9, 34, 21)));
		// the spinner says it all; a caption would only crowd the feed
		MahjongDrawSpinner(x + w2 / 2, y + h2 / 2, large ? 9 : 7);
		(void)base;
	}
	else if (!disconnected && face)
	{
		BltVideoObject(FRAME_BUFFER, face, 0, x, y);
		// occasional life in the feed: blink and talk frames from the same
		// sheet, composited at the profile's eye/mouth offsets (58x65 only)
		if (large && face->SubregionCount() >= 8)
		{
			UINT16 const aux = MahjongFaceAuxFrame(opponent);
			if (aux > 0)
			{
				// anchors were calibrated against the artwork at load time
				INT16 const px = gMJFaceAuxXY[opponent][aux >= 5 ? 2 : 0];
				INT16 const py = gMJFaceAuxXY[opponent][aux >= 5 ? 3 : 1];
				if (px >= 0) BltVideoObject(FRAME_BUFFER, face, aux, x + px, y + py);
			}
		}
	}
	else
	{
		INT32 const w = large ? MJ_FACE65_W : MJ_FACE33_W;
		INT32 const h = large ? MJ_FACE65_H : MJ_FACE33_H;
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + w, y + h, Get16BPPColor(FROMRGB(90, 90, 90)));
	}
}


static void MahjongRenderTopPanels()
{
	// Deidranna: horizontal card, top centre
	{
		INT32 const x = MJ_X(MJ_PANEL_X1), y = MJ_Y(MJ_TOP_Y);
		MahjongDrawPanelFrame(x, y, MJ_PANEL_W, MJ_TOP_H);
		if (MahjongIsPlayersTurn(2)) MahjongDrawTurnRing(x + 3, y + 3);
		MahjongDrawFeed(1, x + 3, y + 3, true);
		MahjongDrawWindBadge(2, x + 3, y + 3);
		MahjongDrawVoidBadge(2, x + 3, y + 3);
		MahjongDrawWonOutline(2, x + 3, y + 3);
		// centre card: score line bottom-aligns with the avatar
		MahjongPrintPlayerLines(2, x + 3 + MJ_FACE65_W + 6, y + 4, MJ_PANEL_W - MJ_FACE65_W - 16, y + 3 + MJ_FACE65_H - 11);
	}
	// Elliot (left) and Enrico (right): vertical edge panels, top-aligned
	for (int seat : { 3, 1 })
	{
		INT32 const x = MJ_X(seat == 3 ? MJ_LEFT_X : MJ_RIGHT_X);
		INT32 const y = MJ_Y(MJ_SIDE_Y);
		MahjongDrawPanelFrame(x, y, MJ_SIDE_W, MJ_SIDE_H);
		INT32 const fx = x + (MJ_SIDE_W - MJ_FACE65_W) / 2;
		if (MahjongIsPlayersTurn(seat)) MahjongDrawTurnRing(fx, y + 3);
		MahjongDrawFeed(seat - 1, fx, y + 3, true);
		MahjongDrawWindBadge(seat, fx, y + 3);
		MahjongDrawVoidBadge(seat, fx, y + 3);
		MahjongDrawWonOutline(seat, fx, y + 3);
		// side panels: score line sits at the panel foot
		MahjongPrintPlayerLines(seat, x + 5, y + 3 + MJ_FACE65_H + 4, MJ_SIDE_W - 10, y + MJ_SIDE_H - 15);
	}
}





static void MahjongRenderInfoBlock()
{
	// the right split of the chat bar: hand state, links live below
	// your own video feed anchors the block's left edge; the data sits right.
	// merc portraits come in odd sizes, so centre them in a fixed 58x65 frame
	{
		INT32 const fx = MJ_X(MJ_CHAT_W + 10), fy = MJ_Y(MahjongBarTop() + 5);
		// your seat gets the same yellow turn marker as the opponents
		if (MahjongIsPlayersTurn(0)) MahjongDrawTurnRing(fx, fy);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, fx, fy, fx + MJ_FACE65_W, fy + MJ_FACE65_H,
					Get16BPPColor(FROMRGB(9, 34, 21)));
		SGPVSurface* const feed = gfMJExhibition ? guiMJShillSurf : guiMJSelfFaceSurf;
		if (feed)
		{
			// stretch the big portrait to fill the feed, cropped to match aspect
			SGPBox const src = { 4, 0, 98, 110 }; // near-full head, aspect-true
			SGPBox const dst = { static_cast<UINT16>(fx), static_cast<UINT16>(fy), MJ_FACE65_W, MJ_FACE65_H };
			BltStretchVideoSurface(FRAME_BUFFER, feed, &src, &dst);
		}
		else if (!gfMJExhibition && guiMJSelfFace)
		{
			ETRLEObject const& e = guiMJSelfFace->SubregionProperties(0);
			BltVideoObject(FRAME_BUFFER, guiMJSelfFace, 0,
					fx + (MJ_FACE65_W - e.usWidth) / 2, fy + (MJ_FACE65_H - e.usHeight) / 2);
		}
		else if (guiMJStatic)
		{
			BltVideoObject(FRAME_BUFFER, guiMJStatic, 3, fx, fy); // silhouette
		}
		MahjongDrawWindBadge(0, fx, fy);
		MahjongDrawVoidBadge(0, fx, fy);
		MahjongDrawWonOutline(0, fx, fy);

		// the profile bio sits at the foot of the block, like a caption
		if (MahjongGameLive())
		{
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKGRAY, FONT_MCOLOR_BLACK, 0);
			std::vector<ST::string> bioLines;
			ST::string bioLine;
			for (auto const& word : ST::string(MahjongSeatBio()).split(' '))
			{
				ST::string const cand = bioLine.empty() ? word : bioLine + " " + word;
				// the bio runs the full width of the profile block, not the
				// width of the portrait above it
				if (!bioLine.empty() && StringPixLength(cand, FONT10ARIAL) > MJ_X(492) - fx)
				{
					bioLines.push_back(bioLine);
					bioLine = word;
				}
				else bioLine = cand;
			}
			bioLines.push_back(bioLine);
			INT32 bioY = MJ_Y(MahjongBarTop() + MahjongBarH()) - 30
					- static_cast<INT32>(bioLines.size()) * 11;
			for (ST::string const& line : bioLines)
			{
				MPrint(fx, bioY, line);
				bioY += 11;
			}
		}
	}
	INT32 const x = MJ_X(MJ_CHAT_W + 10 + MJ_FACE65_W + 8), y = MJ_Y(MahjongBarTop() + 5);

	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
	if (!gGame || gGame->phase() == MahjongGame::Phase::NotStarted)
	{
		MPrint(x, y + 2, "No table open.");
		return;
	}
	// one layout whether you or the shill holds the seat: identity grouped
	// on top (nick + rating), the game stats spaced apart below it
	MahjongGame::Player const& you = gGame->player(0);
	// same pattern as the opponent panels: clear name + rating, handle below
	ST::string const nick = gfMJExhibition ? gMJShillNick : gMJSelfNick;
	ST::string const clearName = nick.empty() ? ST::string(gfMJExhibition ? "House" : "You") : nick;
	ST::string const handle = nick.empty() ? ST::string("@guest") : ST::format("@{}", nick.to_lower());
	INT32 const wCol = 78;
	MPrint(x, y + 2, clearName);
	{
		INT32 const shillRating = 1250 + (static_cast<INT32>(gMJShillNick.size()) * 73) % 450;
		ST::string const rating = gfMJExhibition ? ST::format("{}", shillRating)
				: ST::format("{}{}", MahjongPlayerRating(), MahjongRatingProvisional() ? "*" : "");
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKGRAY, FONT_MCOLOR_BLACK, 0);
		MPrint(std::max(x + wCol - StringPixLength(rating, FONT10ARIAL),
				x + StringPixLength(clearName, FONT10ARIAL) + 6), y + 2, rating);
		MPrint(x, y + 13, handle);
	}
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
	SetFontForegroundRGB(MJ_TOKEN_RGB);
	if (guiMJChips) BltVideoObject(FRAME_BUFFER, guiMJChips, 1, x, y + 55);
	MPrint(x + (guiMJChips ? 20 : 0), y + 58, ST::format("{}", you.score));
	int const order = MahjongWinOrderOf(0);
	if (order >= 0)
	{
		MahjongGame::WinEvent const& e = gGame->wins()[static_cast<size_t>(order)];
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
		MPrint(x, y + 41, e.winningTile != MahjongGame::NO_TILE
				? ST::format("WON #{} - {}", order + 1, MahjongTileLabel(e.winningTile))
				: ST::format("WON #{}", order + 1));
	}
	// table facts live in the footer bar: hand left, wall right
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
	SetFontForegroundRGB(MJ_TOKEN_RGB);
	{
		INT32 const footY = MJ_Y(MahjongBarTop() + MahjongBarH()) - 16;
		MPrint(MJ_X(MJ_CHAT_W + 8), footY, ST::format("Hand {}/{}",
				std::min(gGame->handNumber() + 1, static_cast<int>(MahjongGame::HANDS_PER_MATCH)),
				MahjongGame::HANDS_PER_MATCH));
		ST::string const wall = ST::format("Wall: {}", gGame->wallRemaining());
		MPrint(MJ_X(492) - StringPixLength(wall, FONT10ARIAL), footY, wall);
		SetFontAttributes(TINYFONT1, FONT_MCOLOR_DKGRAY, FONT_MCOLOR_BLACK, 0);
		MPrint(MJ_X(MJ_CHAT_W + 8), footY + 10, MJ_PAGE_VERSION);
	}
	if (!gfMJExhibition && MahjongInvitationalToday())
	{
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTRED, FONT_MCOLOR_BLACK, 0);
		MPrint(x, y + 93, "INVITATIONAL 3x");
	}
	if (gfMJChatBig)
	{
		// immersive mode: room for the whole ledger of your career
		MahjongPersist const mj = MahjongGetPersist();
		INT32 const sx = MJ_X(MJ_CHAT_W + 10);
		INT32 sy = y + 80;
		ColorFillVideoSurfaceArea(FRAME_BUFFER, sx, sy - 10, MJ_X(492), sy - 9,
					Get16BPPColor(FROMRGB(18, 66, 38)));
		struct StatLine { ST::string label; ST::string value; };
		StatLine const stats[] =
		{
			{ "Matches",      ST::format("{}", mj.usMatches) },
			{ "Matches won",  ST::format("{}", mj.usMatchesWon) },
			{ "Hands won",    ST::format("{}", mj.usHandsWon) },
			{ "Biggest hand", ST::format("{}", mj.iBiggestHand) },
			{ "Net dollars",  ST::format("{}{}", mj.iDollarsNet >= 0 ? "$" : "-$",
					mj.iDollarsNet >= 0 ? mj.iDollarsNet : -mj.iDollarsNet) },
			{ "Rating",       ST::format("{}{}", MahjongPlayerRating(), MahjongRatingProvisional() ? "*" : "") },
			{ "Grudge level", ST::format("{}", mj.ubGrudge) },
		};
		for (StatLine const& st : stats)
		{
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
			MPrint(sx, sy, st.label);
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
			SetFontForegroundRGB(MJ_TOKEN_RGB);
			MPrint(MJ_X(492) - StringPixLength(st.value, FONT10ARIAL), sy, st.value);
			sy += 16;
			// hairline rule between entries, like a printed ledger
			ColorFillVideoSurfaceArea(FRAME_BUFFER, sx, sy - 3, MJ_X(492), sy - 2,
						Get16BPPColor(FROMRGB(14, 54, 32)));
		}
	}

	// icon strip, terminal-green: bare glyphs, no button chrome
	for (int i = 0; i < 4; ++i)
	{
		INT32 const ix = MJ_X(4);
		INT32 const iy = i == 3 ? MJ_Y(MJ_CHAT_Y + 6 + 3 * 22)
				: MJ_Y(MahjongBarTop() + 6 + i * 22);
		bool const hot = gMJIconRegion[i].uiFlags & MSYS_MOUSE_IN_AREA;
		UINT16 const line = Get16BPPColor(hot ? FROMRGB(255, 255, 255) : MJ_TOKEN_RGB);
		UINT16 const bg = Get16BPPColor(FROMRGB(8, 44, 25));
		switch (i)
		{
			case 0: // little house
				for (INT32 r = 0; r < 5; ++r)
					ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 7 - r, iy + 2 + r, ix + 9 + r, iy + 3 + r, line);
				ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 3, iy + 7, ix + 13, iy + 14, line);
				ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 6, iy + 9, ix + 10, iy + 14, bg);
				break;
			case 1: // open book: two sheared rectangles meeting at the spine
				for (INT32 c = 0; c < 6; ++c)
				{
					INT32 const top = iy + 3 + c / 2; // page dips toward the middle
					ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 2 + c, top, ix + 3 + c, top + 9, line);
					ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 14 - c, top, ix + 15 - c, top + 9, line);
				}
				(void)bg;
				break;
			case 3: // speech bubble: the immersive chat toggle
				ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 2, iy + 3, ix + 14, iy + 11, line);
				ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 4, iy + 11, ix + 7, iy + 14, line);
				if (!gfMJChatBig)
				{
					// dots while collapsed; empty bubble while expanded
					ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 4, iy + 6, ix + 6, iy + 8, bg);
					ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 7, iy + 6, ix + 9, iy + 8, bg);
					ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 10, iy + 6, ix + 12, iy + 8, bg);
				}
				break;
			case 2: // solid disc with a knocked-out question mark
				for (INT32 dy = -6; dy <= 6; ++dy)
				{
					INT32 const dx = static_cast<INT32>(std::sqrt(36.0 - static_cast<double>(dy * dy)) + 0.5);
					ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 8 - dx, iy + 8 + dy, ix + 8 + dx, iy + 9 + dy, line);
				}
				// knocked-out "i": simpler and cleaner than a question mark
				ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 7, iy + 4, ix + 9, iy + 6, bg);    // dot
				ColorFillVideoSurfaceArea(FRAME_BUFFER, ix + 7, iy + 8, ix + 9, iy + 13, bg);   // stem
				break;
		}
	}
}




static void MahjongDrawFaceChip(INT8 who, INT32 cx, INT32 cy, UINT16 cw = 15, UINT16 ch = 14, bool seatView = false);
static void MahjongDrawTypingWave(INT32 dx, INT32 dy);
static const char* MahjongRoleBadge(int who);
static void MahjongDrawRoleBadge(INT32 bx, INT32 by, const char* label);

static void MahjongRenderChatBar()
{
	INT32 const x = MJ_X(2), y = MJ_Y(MahjongBarTop()), w = MJ_CHAT_W - 2, h = MahjongBarH();
	INT32 const visible = MahjongChatVisibleLines();
	// the whole bottom bar (chat + info) is one seamless surface
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(2), y, MJ_X(498), y + h, Get16BPPColor(FROMRGB(8, 44, 25)));
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(2), y, MJ_X(498), y + 1, Get16BPPColor(FROMRGB(30, 60, 40)));
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(2), y + h - 1, MJ_X(498), y + h, Get16BPPColor(FROMRGB(30, 60, 40)));
	// footer strip: a proper status bar hosting input and table facts
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(2), y + h - 22, MJ_X(498), y + h - 1,
				Get16BPPColor(FROMRGB(6, 36, 20)));
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(2), y + h - 23, MJ_X(498), y + h - 22,
				Get16BPPColor(FROMRGB(30, 60, 40)));
	// quiet divider between the icon rail and the chat text
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(23), y + 4, MJ_X(24), y + h - 26,
				Get16BPPColor(FROMRGB(18, 66, 38)));
	// quiet divider before the info split
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(MJ_CHAT_W + 1), y + 4, MJ_X(MJ_CHAT_W + 2), y + h - 26,
				Get16BPPColor(FROMRGB(18, 66, 38)));

	// mini avatar chip on the left edge of a spoken line
	auto const drawChip = [&](INT8 who, INT32 cy) { MahjongDrawFaceChip(who, x + 27, cy - 3); };
	MahjongReconcileScroll();
	INT32 const total = static_cast<INT32>(gMJChat.size());
	INT32 const last = total;
	// the newest system line is still printing itself out
	auto const sysTyped = [&](INT32 idx, ST::string const& t) -> ST::string
	{
		if (idx != total - 1 || guiMJSysTypeLen == 0) return t;
		std::size_t const shown = (MahjongNow() - guiMJSysTypeStart) / MJ_SYS_TYPE_MS;
		if (shown >= guiMJSysTypeLen) return t;
		return ST::string(t.to_std_string().substr(0, shown));
	};

	bool const typingActive = MahjongTypingRowActive();
	// every row has a fixed place in the content column; the view is a
	// window onto it. rows outside the window are skipped, rows crossing
	// the edge are drawn and clipped, so lines visibly slide out of frame
	INT32 const paneTop = y + 4, paneBot = y + h - 26;
	INT32 const originY = paneTop - giMJScrollY;
	SetFontDestBuffer(FRAME_BUFFER, MJ_X(2), paneTop, MJ_X(MJ_CHAT_W - 16), paneBot);
	std::vector<INT32> rel;
	rel.reserve(gMJChat.size());
	INT32 rely = 0;
	for (std::size_t i = 0; i < gMJChat.size(); ++i)
	{
		rely += MahjongRowHeight(i);
		rel.push_back(rely - 14); // top of the row's text
	}
	INT32 const firstDraw = 0;
	INT32 flowY = originY;
	for (INT32 i = firstDraw; i < last; ++i)
	{
		MahjongChatLine const& l = gMJChat[i];
		INT32 const lineY = originY + rel[i];
		flowY = lineY + 14;
		if (lineY > paneBot || lineY + 12 < paneTop) continue; // off-window
		bool const rowVisible = lineY >= paneTop;
		// consecutive lines from one author: chip and name only on the first
		bool const cont = i > 0 && gMJChat[i - 1].who == l.who;
		if (l.who == -2)
		{
			// squad kibitz from your side of the modem
			if (!cont && rowVisible) drawChip(0, lineY);
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGRAY, FONT_MCOLOR_BLACK, 0);
			SetFontForegroundRGB(MJ_SELF_TEXT_RGB);
			MPrint(x + 48, lineY, ReduceStringLength(l.text, w - 66, FONT10ARIAL));
		}
		else if (l.who == -5)
		{
			// wrapped continuation of a table event
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
			MPrint(x + 48, lineY, ReduceStringLength(sysTyped(i, l.text), w - 66, FONT10ARIAL));
		}
		else if (l.who == -4)
		{
			// a table event: louder than a notice, quieter than speech
			if (!cont && rowVisible)
			{
				ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 27, lineY - 3, x + 42, lineY + 11,
							Get16BPPColor(FROMRGB(6, 36, 20)));
				ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 32, lineY + 2, x + 37, lineY + 7,
							Get16BPPColor(FROMRGB(240, 220, 60)));
			}
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
			MPrint(x + 48, lineY, ReduceStringLength(sysTyped(i, l.text), w - 66, FONT10ARIAL));
		}
		else if (l.who == -3)
		{
			// wrapped continuation of a system line
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
			SetFontForegroundRGB(MJ_TOKEN_RGB);
			MPrint(x + 48, lineY, ReduceStringLength(sysTyped(i, l.text), w - 66, FONT10ARIAL));
		}
		else if (l.who < 0)
		{
			// system voice gets a placeholder avatar so the column stays true
			if (!cont && rowVisible)
			{
				ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 27, lineY - 3, x + 42, lineY + 11,
							Get16BPPColor(FROMRGB(6, 36, 20)));
				ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 32, lineY + 2, x + 37, lineY + 7,
							Get16BPPColor(MJ_TOKEN_RGB));
			}
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
			SetFontForegroundRGB(MJ_TOKEN_RGB);
			MPrint(x + 48, lineY, ReduceStringLength(sysTyped(i, l.text), w - 66, FONT10ARIAL));
		}
		else
		{
			// white handles, grey bodies; your own lines run warm
			bool const mine = l.who == 0;
			INT32 nameW = 0;
			if (!cont)
			{
				if (rowVisible) drawChip(l.who, lineY);
				ST::string const name = ST::format("{}:", MahjongChatHandle(l.who));
				SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
				if (mine) SetFontForegroundRGB(MJ_SELF_NAME_RGB);
				MPrint(x + 48, lineY, name);
				nameW = StringPixLength(name, FONT10ARIAL) + 4;
				if (const char* const badge = MahjongRoleBadge(l.who))
				{
					if (rowVisible) MahjongDrawRoleBadge(x + 48 + nameW, lineY, badge);
					nameW += StringPixLength(badge, FONT10ARIAL) + 12;
				}
			}
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGRAY, FONT_MCOLOR_BLACK, 0);
			if (mine) SetFontForegroundRGB(MJ_SELF_TEXT_RGB);
			MPrint(x + 48 + nameW, lineY, ReduceStringLength(l.text, w - 66 - nameW, FONT10ARIAL));
		}
	}
	if (typingActive)
	{
		// you watch them type it, mistakes and all
		MahjongPendingLine const& pend = gMJPending.front();
		int const twho = pend.who;
		INT32 const lineY = originY + rely + MJ_CLUSTER_GAP;
		// mid-burst the room already knows who is talking: dots alone
		bool const sameAuthor = !gMJChat.empty() && gMJChat.back().who == twho;
		if (sameAuthor)
		{
			MahjongDrawTypingWave(x + 48, lineY);
		}
		else
		{
			// the handle keeps its normal colour; only the state is green
			MahjongDrawFaceChip(static_cast<INT8>(twho), x + 27, lineY - 3);
			ST::string const handle = MahjongChatHandle(twho);
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
			MPrint(x + 48, lineY, handle);
			INT32 const sx = x + 48 + StringPixLength(handle, FONT10ARIAL) + 4;
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
			SetFontForegroundRGB(MJ_TOKEN_RGB);
			MPrint(sx, lineY, "is typing");
			MahjongDrawTypingWave(sx + StringPixLength("is typing", FONT10ARIAL) + 5, lineY);
		}
		(void)pend;
	}

	// scrollbar rail: muted like the ghost prompt, waking up under the cursor
	{
		INT32 const maxScroll = std::max(0, giMJContentH - MahjongChatViewH());
		INT32 const railX = MJ_X(MJ_CHAT_W - 12);
		INT32 const trackY = y + 16, trackH = h - 54; // between the two arrows
		bool const upHot   = gMJChatUpRegion.uiFlags & MSYS_MOUSE_IN_AREA;
		bool const downHot = gMJChatDownRegion.uiFlags & MSYS_MOUSE_IN_AREA;
		bool const railHot = (gMJChatRegion.uiFlags & MSYS_MOUSE_IN_AREA) &&
					gMJChatRegion.MouseXPos >= railX - 3;
		UINT16 const dim = Get16BPPColor(FROMRGB(40, 96, 58)); // as quiet as the typing dots
		UINT16 const hot = Get16BPPColor(FROMRGB(255, 255, 255));
		ColorFillVideoSurfaceArea(FRAME_BUFFER, railX, y + 2, railX + 9, y + h - 24,
					Get16BPPColor(FROMRGB(6, 36, 20)));
		for (INT32 r = 0; r < 6; ++r)
		{
			INT32 const half = r / 2;
			ColorFillVideoSurfaceArea(FRAME_BUFFER, railX + 4 - half, y + 4 + r, railX + 5 + half, y + 5 + r,
						upHot ? hot : dim);
			ColorFillVideoSurfaceArea(FRAME_BUFFER, railX + 4 - half, y + h - 28 - r, railX + 5 + half, y + h - 27 - r,
						downHot ? hot : dim);
		}
		ColorFillVideoSurfaceArea(FRAME_BUFFER, railX + 4, trackY, railX + 5, trackY + trackH,
					Get16BPPColor(FROMRGB(11, 46, 27)));
		if (maxScroll > 0)
		{
			INT32 const thumbH = std::max(6, trackH * MahjongChatViewH() / std::max(1, giMJContentH));
			INT32 const thumbY = trackY + (trackH - thumbH) * giMJScrollY / maxScroll;
			ColorFillVideoSurfaceArea(FRAME_BUFFER, railX + 3, thumbY, railX + 6, thumbY + thumbH,
						railHot ? hot : dim);
		}
	}

	SetFontDestBuffer(FRAME_BUFFER);

	// the sliding chat can spill into the footer: repaint it on top
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(2), y + h - 22, MJ_X(498), y + h - 1,
				Get16BPPColor(FROMRGB(6, 36, 20)));
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(2), y + h - 23, MJ_X(498), y + h - 22,
				Get16BPPColor(FROMRGB(30, 60, 40)));

	// input line: typed text, or the game prompt as a ghost placeholder
	INT32 const inputY = y + h - 16;
	bool const caret = (MahjongNow() / 500) % 2 == 0;
	if (gMJInput.empty())
	{
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
		SetFontForegroundRGB(MJ_TOKEN_RGB);
		MPrint(x + 30, inputY, caret ? "> _" : "> ");
		// the placeholder whispers, like the typing dots
		SetFontForegroundRGB(FROMRGB(40, 96, 58));
		if (MahjongNow() < guiMJMutedUntil)
		{
			UINT32 const left = (guiMJMutedUntil - MahjongNow() + 999) / 1000;
			MPrint(x + 45, inputY, ST::format("muted by @e11iot - remaining {}s", left));
		}
		else
		{
			MPrint(x + 45, inputY, "write message...");
		}
	}
	else
	{
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
		SetFontForegroundRGB(MJ_TOKEN_RGB);
		MPrint(x + 30, inputY, ">");
		// a real input field: it scrolls with the caret instead of cutting off
		std::string typed = gMJInput + (caret ? "_" : "");
		INT32 const field = w - 66;
		while (typed.size() > 1 && StringPixLength(ST::string(typed), FONT10ARIAL) > field)
		{
			typed.erase(typed.begin());
		}
		MPrint(x + 45, inputY, ST::string(typed));
	}
}


static bool MahjongIsRonTile(int player, size_t discardIndex)
{
	return gGame->phase() == MahjongGame::Phase::RonWindow &&
		player == gGame->lastDiscarder() &&
		discardIndex + 1 == gGame->player(player).discards.size();
}


// one pond cell: an exposed meld (tile + count badge + gold bar) or a discard
struct MahjongPondCell
{
	MahjongGame::TileId tile;
	bool meld;
	UINT8 count;
	bool ronOutline;
	INT8 fedRel; // meld provenance: 0 self, 1 fed from the right, 2 across, 3 left
};

static std::vector<MahjongPondCell> MahjongPondCells(int player)
{
	std::vector<MahjongPondCell> cells;
	MahjongGame::Player const& p = gGame->player(player);
	for (MahjongGame::Meld const& m : p.melds)
	{
		INT8 const rel = m.from >= 0
			? static_cast<INT8>((m.from - player + 4) % 4) : INT8(0);
		cells.push_back(MahjongPondCell{ m.tile, true, m.count, false, rel });
	}
	for (size_t i = 0; i < p.discards.size(); ++i)
	{
		cells.push_back(MahjongPondCell{ p.discards[i], false, 1,
				MahjongIsRonTile(player, i), 0 });
	}
	return cells;
}

static void MahjongDrawPondCell(MahjongPondCell const& cell, INT32 x, INT32 y)
{
	MahjongDrawTile(x, y, MJ_MINI_W, MJ_MINI_H, cell.tile, cell.ronOutline);
	if (cell.meld)
	{
		// gold bar marks an exposed set; kongs carry a red 4
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y + MJ_MINI_H, x + MJ_MINI_W, y + MJ_MINI_H + 2,
					Get16BPPColor(FROMRGB(240, 220, 60)));
		if (cell.fedRel > 0)
		{
			// the contribution: a red notch on the bar, sitting on the
			// side the claimed tile came from
			INT32 const nx = x + (cell.fedRel == 3 ? 0
			                    : cell.fedRel == 2 ? MJ_MINI_W / 2 - 2
			                                       : MJ_MINI_W - 4);
			ColorFillVideoSurfaceArea(FRAME_BUFFER, nx, y + MJ_MINI_H,
						nx + 4, y + MJ_MINI_H + 2,
						Get16BPPColor(FROMRGB(204, 56, 46)));
		}
		if (cell.count == 4)
		{
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_RED, NO_SHADOW, 0);
			MPrint(x + MJ_MINI_W - 6, y + MJ_MINI_H - 10, "4");
			SetFontShadow(DEFAULT_SHADOW);
		}
	}
}

static void MahjongRenderPonds()
{
	if (!MahjongGameLive()) return;

	// bottom (you) and top (across): rows of 8, growing toward the centre
	for (int player : { 0, 2 })
	{
		std::vector<MahjongPondCell> const cells = MahjongPondCells(player);
		for (size_t i = 0; i < cells.size() && i < 2 * MJ_POND_COLS; ++i)
		{
			INT32 const col = static_cast<INT32>(i % MJ_POND_COLS);
			INT32 const row = static_cast<INT32>(i / MJ_POND_COLS);
			INT32 const px = MJ_X(MJ_POND_TOP_X + col * MJ_MINI_PITCH);
			INT32 const py = player == 2
				? MJ_Y(MJ_POND_TOP_Y + row * MJ_POND_ROW_PITCH)
				: MJ_Y(MJ_POND_BOTTOM_Y - row * MJ_POND_ROW_PITCH);
			MahjongDrawPondCell(cells[i], px, py);
		}
	}

	// left (Elliot, 3) and right (Enrico, 1): columns of 5, growing toward the centre
	for (int player : { 1, 3 })
	{
		std::vector<MahjongPondCell> const cells = MahjongPondCells(player);
		for (size_t i = 0; i < cells.size() && i < 3 * MJ_POND_SIDE_ROWS; ++i)
		{
			INT32 const row = static_cast<INT32>(i % MJ_POND_SIDE_ROWS);
			INT32 const col = static_cast<INT32>(i / MJ_POND_SIDE_ROWS);
			INT32 const px = player == 3
				? MJ_X(MJ_POND_LEFT_X + col * MJ_MINI_PITCH)
				: MJ_X(MJ_POND_RIGHT_X - col * MJ_MINI_PITCH);
			INT32 const py = MJ_Y(MJ_POND_SIDE_Y + row * MJ_POND_ROW_PITCH);
			MahjongDrawPondCell(cells[i], px, py);
		}
	}
}


static void MahjongRenderHand()
{
	if (!MahjongGameLive() || gfMJExhibition) return;

	int voidSuit = gGame->player(0).voidSuit;
	// hovering a void pick previews the sentence: the suit lights up red
	// in the hand before the click commits to it
	if (guiMJState == MJUI_CHOOSE_VOID && voidSuit == MahjongGame::NO_SUIT)
	{
		for (int sIdx = 0; sIdx < 3; ++sIdx)
		{
			if (gMJVoidRegion[sIdx].uiFlags & MSYS_MOUSE_IN_AREA) voidSuit = sIdx;
		}
	}
	bool const voidPreview =
		guiMJState == MJUI_CHOOSE_VOID && voidSuit != MahjongGame::NO_SUIT;
	auto const isVoided = [voidSuit](MahjongGame::TileId t)
	{
		return voidSuit != MahjongGame::NO_SUIT && MahjongGame::SuitOf(t) == voidSuit;
	};

	// the house coach (opt-in via /coach): mark the heuristic discard
	MahjongGame::TileId suggested = MahjongGame::NO_TILE;
	if (gfMJCoach && guiMJState == MJUI_PLAYER_TURN && gGame->currentPlayer() == 0 &&
		gGame->drawnTile() != MahjongGame::NO_TILE && !gGame->CanTsumo())
	{
		suggested = gGame->AiChooseDiscard(0);
	}
	bool suggestionMarked = false;
	auto const markSuggestion = [&](INT32 tileX)
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, tileX + MJ_TILE_W / 2 - 3, MJ_Y(MJ_HAND_Y - 12),
					tileX + MJ_TILE_W / 2 + 3, MJ_Y(MJ_HAND_Y - 8), Get16BPPColor(FROMRGB(120, 230, 120)));
	};

	std::vector<MahjongGame::TileId> const hand = MahjongGame::SortedHand(gGame->player(0));
	UINT32 const shown = guiMJState == MJUI_DEALING
				? std::min<UINT32>(guiMJDealStep + 1, static_cast<UINT32>(hand.size()))
				: static_cast<UINT32>(hand.size());
	for (UINT32 i = 0; i < shown; ++i)
	{
		if (!suggestionMarked && suggested != MahjongGame::NO_TILE && hand[i] == suggested)
		{
			markSuggestion(MJ_X(MJ_HAND_X + i * MJ_TILE_PITCH));
			suggestionMarked = true;
		}
		bool const raised = gbMJSelectedSlot == static_cast<INT8>(i) ||
					(guiMJState == MJUI_EXCHANGE && gfMJExchangeSel[i]) ||
					(voidPreview && isVoided(hand[i]));
		MahjongDrawTile(MJ_X(MJ_HAND_X + i * MJ_TILE_PITCH), MJ_Y(MJ_HAND_Y - (raised ? MJ_HAND_RAISE : 0)),
				MJ_TILE_W, MJ_TILE_H, hand[i], false, isVoided(hand[i]));
	}

	if (gGame->drawnTile() != MahjongGame::NO_TILE && gGame->currentPlayer() == 0)
	{
		INT32 const raise = gbMJSelectedSlot == MJ_DRAWN_SLOT ? MJ_HAND_RAISE : 0;
		MahjongDrawTile(MJ_X(MJ_DRAWN_X), MJ_Y(MJ_HAND_Y - raise), MJ_TILE_W, MJ_TILE_H,
				gGame->drawnTile(), false, isVoided(gGame->drawnTile()));
		if (!suggestionMarked && suggested == gGame->drawnTile())
		{
			markSuggestion(MJ_X(MJ_DRAWN_X));
		}
	}

	// wait indicator: with a settled 13-tile hand, show what would win;
	// suppressed while the Mahjong! button occupies the same spot
	bool const canClaim = (guiMJState == MJUI_PLAYER_TURN && gGame->CanTsumo()) ||
				guiMJState == MJUI_RON_WINDOW;
	bool const inPlay = guiMJState == MJUI_PLAYER_TURN || guiMJState == MJUI_AI_THINK ||
				guiMJState == MJUI_RON_WINDOW || guiMJState == MJUI_ANNOUNCE;
	// never while it is your action: with a drawn tile - or right after a
	// claim - the hand is momentarily 14 tiles and the wait math reads it
	// as 'complete', which printed as '-1 tiles from a waiting hand'
	if (inPlay && !canClaim && !gGame->player(0).finished && voidSuit != MahjongGame::NO_SUIT &&
		!(gGame->currentPlayer() == 0 &&
			(gGame->drawnTile() != MahjongGame::NO_TILE ||
			 guiMJState == MJUI_PLAYER_TURN)))
	{
		std::vector<MahjongGame::TileId> const waits = gGame->WinningTilesFor(0);
		if (!waits.empty())
		{
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
			MPrint(MJ_X(MJ_HAND_X), MJ_Y(166), "Waiting on:");
			INT32 tx = MJ_X(MJ_HAND_X);
			for (size_t i = 0; i < waits.size() && i < 4; ++i)
			{
				MahjongDrawTile(tx, MJ_Y(MJ_POND_BOTTOM_Y), MJ_MINI_W, MJ_MINI_H, waits[i], false);
				tx += MJ_MINI_PITCH;
			}
		}
		else
		{
			int const shanten = gGame->ShantenFor(0);
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
			// a waiting shape with no legal waits means the void suit is
			// standing in the door
			MPrint(MJ_X(MJ_HAND_X), MJ_Y(198), shanten <= 0
					? ST::string("your wait is blocked by ze void suit")
					: shanten == 1
					? ST::string("1 tile from a waiting hand")
					: ST::format("{} tiles from a waiting hand", shanten));
		}
	}
}


// the room's pecking order, worn next to the handle. deliberately rare:
// Elliot moderates (the Queen volunteered him), Kingpin owns the place
static const char* MahjongRoleBadge(int who)
{
	if (who == 4) return "POTM";
	if (who == 3)
	{
		if (giMJSeat3Persona == MJP_ELLIOT)  return "mod";
		if (giMJSeat3Persona == MJP_KINGPIN) return "owner";
	}
	if (who == 2 && giMJSeat2Persona == MJP2_DARREN) return "staff";
	return nullptr;
}

static void MahjongDrawRoleBadge(INT32 bx, INT32 by, const char* label)
{
	INT32 const bw = StringPixLength(label, FONT10ARIAL) + 8;
	ColorFillVideoSurfaceArea(FRAME_BUFFER, bx, by - 1, bx + bw, by + 9,
				Get16BPPColor(FROMRGB(24, 74, 46)));
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
	SetFontForegroundRGB(MJ_TOKEN_RGB);
	MPrint(bx + 4, by - 1, label);
}

// a braille-style wave of dots: the parlour's "still typing" spinner
static void MahjongDrawTypingWave(INT32 dx, INT32 dy)
{
	static const INT32 lift[6] = { 0, -1, -2, -3, -2, -1 };
	UINT32 const phase = MahjongNow() / 110;
	// muted against the bar: present, not shouting
	UINT16 const col = Get16BPPColor(FROMRGB(40, 96, 58));
	for (INT32 i = 0; i < 3; ++i)
	{
		INT32 const y = dy + 6 + lift[(phase + 6 - i) % 6];
		ColorFillVideoSurfaceArea(FRAME_BUFFER, dx + i * 5, y, dx + i * 5 + 2, y + 2, col);
	}
}

// small face chip: used on chat lines and score tables
static void MahjongDrawFaceChip(INT8 who, INT32 cx, INT32 cy, UINT16 cw, UINT16 ch, bool seatView)
{
	SGPVSurface* surf;
	SGPBox src = { 5, 8, 19, 19 }; // square crop on the face itself
	if (who <= 0)
	{
		// in the seat it is whoever holds the chair; in chat it is always
		// you, because you are the one typing
		SGPVSurface* const seat = gfMJExhibition ? guiMJShillSurf : guiMJSelfFaceSurf;
		surf = seatView ? seat : (guiMJSelfFaceSurf ? guiMJSelfFaceSurf : seat);
		src = SGPBox{ 24, 20, 58, 58 };
	}
	else if (who == 4) { surf = guiMJVisitorSurf; src = SGPBox{ 24, 20, 58, 58 }; }
	else surf = guiMJChipSurf[who - 1];
	if (!surf) return;
	// a quiet frame separates the face from the felt; your own runs warm
	// like your text, so your lines read as a set
	bool const mine = who == 0 && !seatView;
	ColorFillVideoSurfaceArea(FRAME_BUFFER, cx - 1, cy - 1, cx + cw + 1, cy + ch + 1,
				Get16BPPColor(mine ? FROMRGB(150, 138, 100) : FROMRGB(24, 74, 46)));
	SGPBox const dst = { static_cast<UINT16>(cx), static_cast<UINT16>(cy), cw, ch };
	BltStretchVideoSurface(FRAME_BUFFER, surf, &src, &dst);
}

// a rounded panel painted span by span, so the corners never stamp over
// whatever the table has going on underneath them
static void MahjongFillRounded(INT32 x, INT32 y, INT32 w, INT32 h, INT32 r, UINT16 col)
{
	for (INT32 row = 0; row < h; ++row)
	{
		INT32 inset = 0;
		INT32 const edge = row < r ? row : h - 1 - row;
		if (edge < r)
		{
			double const dy = r - edge - 0.5;
			inset = static_cast<INT32>(r - std::sqrt(static_cast<double>(r) * r - dy * dy) + 0.5);
		}
		ColorFillVideoSurfaceArea(FRAME_BUFFER, x + inset, y + row, x + w - inset, y + row + 1, col);
	}
}

// the void glyph stamped in a single colour: the sti's own red would
// disappear into the hovered button's red field
static void MahjongStampVoidIcon(UINT16 frame, INT32 sx, INT32 sy, UINT16 col)
{
	if (!guiMJVoidIcon) return;
	ETRLEObject const& e = guiMJVoidIcon->SubregionProperties(frame);
	UINT8 const* in = guiMJVoidIcon->PixData(e);
	SGPVSurface::Lock lock(FRAME_BUFFER);
	UINT16* out = lock.Buffer<UINT16>();
	UINT32 const pitch = lock.Pitch() / 2;
	for (INT32 y = 0; y < e.usHeight; ++y)
	{
		INT32 x = 0;
		while (*in != 0)
		{
			UINT8 const code = *in++;
			UINT8 const run = code & 0x7F;
			if (code & 0x80)
			{
				x += run;
			}
			else
			{
				for (UINT8 k = 0; k < run; ++k, ++x, ++in)
				{
					INT32 const px = sx + x, py = sy + y;
					if (x >= e.usWidth || px < 0 || px >= SCREEN_WIDTH ||
						py < 0 || py >= SCREEN_HEIGHT)
					{
						continue;
					}
					out[static_cast<UINT32>(py) * pitch +
							static_cast<UINT32>(px)] = col;
				}
			}
		}
		++in;
	}
}

// The void pick, drawn by hand so it can react: each button carries its
// suit glyph, and under the mouse the whole thing goes red with glyph
// and label in white-rose.
static void MahjongRenderVoidButtons()
{
	if (guiMJState != MJUI_CHOOSE_VOID) return;
	for (int s = 0; s < 3; ++s)
	{
		INT32 const bx = MJ_X(102 + s * 104), by = MJ_Y(MJ_SETUP_BTN_Y);
		bool const hot = gMJVoidRegion[s].uiFlags & MSYS_MOUSE_IN_AREA;
		MahjongFillRounded(bx, by, 98, 22, 4,
				Get16BPPColor(hot ? FROMRGB(96, 18, 18) : FROMRGB(8, 26, 18)));
		MahjongFillRounded(bx + 1, by + 1, 96, 20, 4,
				Get16BPPColor(hot ? FROMRGB(164, 42, 38) : FROMRGB(20, 50, 34)));
		ColorFillVideoSurfaceArea(FRAME_BUFFER, bx + 3, by + 2, bx + 95, by + 3,
				Get16BPPColor(hot ? FROMRGB(192, 70, 62) : FROMRGB(32, 66, 46)));
		ST::string const label = ST::format("Void {}", gMJSuitName[s]);
		INT32 const tw = StringPixLength(label, FONT12ARIAL);
		INT32 iw = 12, ih = 12;
		if (guiMJVoidIcon)
		{
			ETRLEObject const& e = guiMJVoidIcon->SubregionProperties(static_cast<UINT16>(s));
			iw = e.usWidth;
			ih = e.usHeight;
		}
		// how many of your tiles the sentence would take, dimmer than the
		// verdict itself, parked left of the glyph
		int held = 0;
		if (gGame)
		{
			for (MahjongGame::TileId const t : MahjongGame::SortedHand(gGame->player(0)))
			{
				if (MahjongGame::SuitOf(t) == s) ++held;
			}
		}
		ST::string const count = ST::format("{}", held);
		INT32 const nw = StringPixLength(count, FONT10ARIAL);
		INT32 const cx = bx + (98 - (nw + 4 + iw + 5 + tw)) / 2 + nw + 4;
		INT32 const cy = by + (22 - ih) / 2;
		SetFontAttributes(FONT10ARIAL, hot ? FONT_MCOLOR_LTRED : FONT_MCOLOR_DKGRAY,
				FONT_MCOLOR_BLACK, 0);
		MPrint(cx - 4 - nw, by + 8, count);
		if (guiMJVoidIcon)
		{
			if (hot) MahjongStampVoidIcon(static_cast<UINT16>(s), cx, cy,
					Get16BPPColor(FROMRGB(255, 220, 216)));
			else BltVideoObject(FRAME_BUFFER, guiMJVoidIcon, static_cast<UINT16>(s), cx, cy);
		}
		SetFontAttributes(FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
		MPrint(cx + iw + 5, by + 7, label);
	}
}

static std::vector<MahjongGame::TileId> MahjongWinHandTiles(MahjongGame::WinEvent const& e)
{
	std::vector<MahjongGame::TileId> out;
	for (int k = 0; k < MahjongGame::NUM_KINDS; ++k)
	{
		for (int c = 0; c < e.winningCounts[k]; ++c)
		{
			out.push_back(static_cast<MahjongGame::TileId>(k));
		}
	}
	return out;
}

// a nine-pixel star for the winners on the verdict card
static void MahjongDrawStar(INT32 sx, INT32 sy, UINT16 col)
{
	static const char* const rows[9] = {
		"....X....",
		"....X....",
		"...XXX...",
		"XXXXXXXXX",
		".XXXXXXX.",
		"..XXXXX..",
		"..XXXXX..",
		".XX...XX.",
		".X.....X.",
	};
	for (INT32 ry = 0; ry < 9; ++ry)
	{
		for (INT32 rx = 0; rx < 9; ++rx)
		{
			if (rows[ry][rx] != 'X') continue;
			ColorFillVideoSurfaceArea(FRAME_BUFFER, sx + rx, sy + ry,
					sx + rx + 1, sy + ry + 1, col);
		}
	}
}

static void MahjongRenderOverlay()
{
	if (guiMJState != MJUI_HAND_END && guiMJState != MJUI_MATCH_END) return;

	// the table dims under the verdict, chach.com fashion
	FRAME_BUFFER->ShadowRect(MJ_X(2), MJ_Y(2), MJ_X(500), MJ_Y(398));

	INT32 const h = MahjongModalHeight();
	INT32 const x = MJ_X(135), y = MJ_Y(MahjongModalTop()), w = 232;
	MahjongFillRounded(x, y, w, h, 7, Get16BPPColor(FROMRGB(240, 220, 60)));
	MahjongFillRounded(x + 2, y + 2, w - 4, h - 4, 6, Get16BPPColor(FROMRGB(30, 26, 20)));

	INT32 const textX = x + 14;

	SetFontAttributes(FONT14ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
	{
		ST::string const title = guiMJState == MJUI_MATCH_END ? "Final standings"
				: gGame->aborted() ? "Hand VOID" : "Hand over";
		MPrint(x + w / 2 - StringPixLength(title, FONT14ARIAL) / 2, y + 14, title);
	}

	INT32 lineY = y + 36;
	if (guiMJState == MJUI_HAND_END)
	{
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
		for (MahjongGame::WinEvent const& e : gGame->wins())
		{
			ST::string line = e.discarder < 0
				? ST::format("{} won by self-draw", MahjongSeatName(e.winner))
				: ST::format("{} claimed {} discard", MahjongSeatName(e.winner),
						e.discarder == 0 ? "your" : ST::format("{}'s", MahjongSeatName(e.discarder)).c_str());
			if (e.fan > 0) line += ST::format(" ({} fan)", e.fan);
			INT32 const lw = StringPixLength(line, FONT10ARIAL);
			INT32 const lx = x + w / 2 - (lw + 13) / 2;
			MahjongDrawStar(lx, lineY, Get16BPPColor(FROMRGB(240, 214, 62)));
			MPrint(lx + 13, lineY, line);
			lineY += 13;
			// the hand itself, laid open: overlapped minis, the tile that
			// completed it ringed and drawn on top
			{
				std::vector<MahjongGame::TileId> const tiles = MahjongWinHandTiles(e);
				INT32 const pitch = 15;
				INT32 const handW = static_cast<INT32>(tiles.size() - 1) * pitch + MJ_MINI_W;
				INT32 tx = x + w / 2 - handW / 2;
				INT32 ringX = -1;
				for (MahjongGame::TileId t : tiles)
				{
					MahjongDrawTile(tx, lineY, MJ_MINI_W, MJ_MINI_H, t, false);
					if (ringX < 0 && t == e.winningTile) ringX = tx;
					tx += pitch;
				}
				if (ringX >= 0)
				{
					MahjongDrawTile(ringX, lineY, MJ_MINI_W, MJ_MINI_H,
							e.winningTile, true);
				}
				lineY += MJ_MINI_H + 4;
			}
		}
		if (gGame->wins().empty())
		{
			MPrint(x + w / 2 - StringPixLength("Exhaustive draw - no winners.", FONT10ARIAL) / 2,
					lineY, "Exhaustive draw - no winners.");
			lineY += 13;
		}
		lineY += 4;
	}
	lineY += 4;

	int leader = 0;
	for (int i = 1; i < MahjongGame::NUM_PLAYERS; ++i)
	{
		if (gGame->player(i).score > gGame->player(leader).score) leader = i;
	}
	// the deltas count up like a payout meter over the first second
	UINT32 const animMs = MahjongNow() - guiMJDeltaAnimStart;
	INT32 const animPct = animMs >= 900 ? 100 : static_cast<INT32>(animMs * 100 / 900);
	// each row: name | chip splay | right-aligned score | delta - all inside
	// the panel regardless of whether the portrait is shown
	// mini profile rows: avatar, stacked identity, results in columns
	INT32 const rowL = textX - 4, rowR = x + w - 12;
	INT32 const nameX = textX + 48;
	for (int i = 0; i < MahjongGame::NUM_PLAYERS; ++i)
	{
		int rank = 1;
		for (int j = 0; j < MahjongGame::NUM_PLAYERS; ++j)
		{
			if (gGame->player(j).score > gGame->player(i).score) ++rank;
		}
		// the podium palette: gold, silver, bronze, lead - row ground dark,
		// the rank number bright in the same metal
		static const UINT32 metalRow[4] = {
			FROMRGB(64, 52, 20), FROMRGB(48, 50, 55),
			FROMRGB(56, 40, 26), FROMRGB(40, 38, 36) };
		static const UINT32 metalNum[4] = {
			FROMRGB(240, 204, 82), FROMRGB(200, 206, 214),
			FROMRGB(198, 134, 86), FROMRGB(122, 118, 114) };
		MahjongFillRounded(rowL, lineY - 4, rowR - rowL, 34, 4,
					Get16BPPColor(metalRow[rank - 1]));
		SetFontAttributes(FONT14ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
		SetFontForegroundRGB(metalNum[rank - 1]);
		MPrint(textX + 2, lineY + 7, ST::format("{}", rank));
		MahjongDrawFaceChip(static_cast<INT8>(i), textX + 14, lineY + 1, 26, 26, true);
		// line one: who they are - name, handle, rating, in one breath
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
		MPrint(nameX, lineY + 2, MahjongSeatName(i));
		INT32 const nameW = StringPixLength(MahjongSeatName(i), FONT10ARIAL);
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKGRAY, FONT_MCOLOR_BLACK, 0);
		// just the name and the rating in brackets; at match end yours
		// shows where it moved from
		ST::string rating = ST::format("({})", i == 0 ? MahjongPlayerRating() : MahjongSeatRating(i));
		if (i == 0 && guiMJState == MJUI_MATCH_END && giMJRatingBefore != 0 &&
			giMJRatingBefore != MahjongPlayerRating())
		{
			rating = ST::format("({} > {})", giMJRatingBefore, MahjongPlayerRating());
		}
		MPrint(nameX + nameW + 8, lineY + 2, rating);
		// line two: one chip stands for the stack, then the total and the
		// movement, all left-aligned under the name
		if (guiMJChips)
		{
			BltVideoObject(FRAME_BUFFER, guiMJChips, i == leader ? 1 : 0,
					nameX, lineY + 16);
		}
		SetFontAttributes(FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
		ST::string const score = ST::format("{}", gGame->player(i).score);
		MPrint(nameX + 16, lineY + 15, score);
		if (guiMJState == MJUI_HAND_END && gGame->handDelta(i) != 0)
		{
			bool const up = gGame->handDelta(i) > 0;
			INT32 const shown = gGame->handDelta(i) * animPct / 100;
			SetFontAttributes(FONT10ARIAL, up ? FONT_MCOLOR_LTGREEN : FONT_MCOLOR_LTRED, FONT_MCOLOR_BLACK, 0);
			MPrint(nameX + 16 + StringPixLength(score, FONT12ARIAL) + 8, lineY + 17,
					ST::format("{}{}", up ? "+" : "", shown));
		}
		lineY += 38;
	}

	// final standings: the cash side of the evening, writ large
	if (guiMJState == MJUI_MATCH_END && giMJLastNetGain != 0)
	{
		bool const won = giMJLastNetGain > 0;
		SetFontAttributes(FONT14ARIAL, won ? FONT_MCOLOR_LTGREEN : FONT_MCOLOR_LTRED, FONT_MCOLOR_BLACK, 0);
		ST::string const cash = won
			? ST::format("CASH-OUT: ${}", giMJLastNetGain)
			: ST::format("DOWN: ${}", -giMJLastNetGain);
		MPrint(x + w / 2 - StringPixLength(cash, FONT14ARIAL) / 2, lineY + 6, cash);
		lineY += 24;
	}

	// the advance button: chach.com's CTA construction, recut in the
	// parlour's gold and stretched across the card's foot
	{
		INT32 const bx = x + 14, by = y + h - 58, bw = w - 28, bh = 30;
		INT32 const band = (bh - 2) / 3;
		bool const hot = gMJModalCTARegion.uiFlags & MSYS_MOUSE_IN_AREA;
		UINT16 const body = Get16BPPColor(hot ? FROMRGB(238, 208, 62) : FROMRGB(216, 184, 44));
		UINT16 const bandc = Get16BPPColor(hot ? FROMRGB(250, 228, 100) : FROMRGB(236, 208, 76));
		UINT16 const high = Get16BPPColor(hot ? FROMRGB(255, 246, 172) : FROMRGB(250, 232, 130));
		MahjongFillRounded(bx, by, bw, bh, 5, Get16BPPColor(FROMRGB(140, 112, 18)));
		MahjongFillRounded(bx, by, bw, bh - 2, 5, body);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, bx + 3, by + 2, bx + bw - 3, by + 2 + band, bandc);
		ColorFillVideoSurfaceArea(FRAME_BUFFER, bx + 5, by + 1, bx + bw - 5, by + 2, high);
		for (INT32 row = 0; row < 3; ++row)
		{
			INT32 const dy = by + 2 + band + row;
			INT32 const step = row == 2 ? 4 : 2;
			for (INT32 dx = bx + 2 + ((row + 1) & 1); dx < bx + bw - 2; dx += step)
			{
				ColorFillVideoSurfaceArea(FRAME_BUFFER, dx, dy, dx + 1, dy + 1, bandc);
			}
		}
		ST::string const cta = guiMJState == MJUI_MATCH_END ? "New Match" : "Next Hand";
		SetFontAttributes(FONT14ARIAL, FONT_MCOLOR_BLACK, NO_SHADOW, 0);
		MPrint(x + w / 2 - StringPixLength(cta, FONT14ARIAL) / 2, by + 8, cta);
	}

	// the closing line, a footnote under the button: the smallest type the
	// site owns, one sentence per line
	if (!gMJMessage.empty())
	{
		std::vector<ST::string> lines;
		ST::string cur;
		for (auto const& word : gMJMessage.split(' '))
		{
			cur = cur.empty() ? word : cur + " " + word;
			if (!word.empty() && word.c_str()[word.size() - 1] == '.')
			{
				lines.push_back(cur);
				cur = ST::string();
			}
		}
		if (!cur.empty()) lines.push_back(cur);
		SetFontAttributes(TINYFONT1, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
		SetFontForegroundRGB(FROMRGB(150, 132, 96));
		INT32 fy = y + h - 8 - static_cast<INT32>(lines.size()) * 9;
		for (ST::string const& ln : lines)
		{
			MPrint(x + w / 2 - StringPixLength(ln, TINYFONT1) / 2, fy, ln);
			fy += 9;
		}
	}
}


static void MahjongRenderGuestbook()
{
	if (!gfMJShowRules || giMJOverlayKind != 2) return;

	INT32 const x = MJ_X(41), y = MJ_Y(14), w = 420, h = 330;
	MahjongFillRounded(x, y, w, h, 7, Get16BPPColor(FROMRGB(240, 220, 60)));
	MahjongFillRounded(x + 2, y + 2, w - 4, h - 4, 6, Get16BPPColor(FROMRGB(24, 32, 24)));

	if (guiMJLogo) BltVideoObject(FRAME_BUFFER, guiMJLogo, 0, x + 20, y + 8);

	struct GuestEntry { const char* who; const char* text; };
	static GuestEntry const entries[] =
	{
		{ "Gus",      "played tiles rougher than this in 'Nam. good table, woolly." },
		{ "spdr",     "lost my medical deposit here. would lose it again." },
		{ "Iv4n",     "horosho. table honest. queen not honest. bird tile best tile." },
		{ "F1DEL",    "BOOM! ...sorry, wrong window. nice parlour." },
		{ "T-Rex",    "the 1-dot looks like a landmine. i checked. it is not." },
		{ "MadLab",   "I could automate your shuffler. nobody answers my emails." },
		{ "F.Walker",  "gambling is a ladder DOWNWARD, my children. repent." },
		{ "Kingpin",   "the father tithes his winnings every sunday. -K" },
		{ "Skyrider",  "cheap flights all over Arulco!! ask for Skyrider!! (SAM sites permitting)" },
		{ "webmaster","best viewed in sir-FER 4.0 at 640x480. no exceptions." },
	};
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
	MPrint(x + 14, y + 114, "::: GUESTBOOK :::  sign after your first bankruptcy");
	INT32 lineY = y + 130;
	for (GuestEntry const& e : entries)
	{
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
		MPrint(x + 14, lineY, ST::format("{}:", e.who));
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
		MPrint(x + 72, lineY, e.text);
		lineY += 13;
	}

	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKGRAY, FONT_MCOLOR_BLACK, 0);
	MPrint(x + 14, lineY + 6, ST::format("You are visitor No. {}", guiMJVisitorNo));

	// the sponsor banner - an actual link
	INT32 const bannerY = MJ_Y(310);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(55), bannerY, MJ_X(447), bannerY + 20, Get16BPPColor(FROMRGB(60, 20, 16)));
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
	MPrint(MJ_X(66), bannerY + 5, "SPONSOR: BOBBY RAY'S - GUNS GUNS GUNS - click here for hardware");

	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
	MPrint(x + 14, y + h - 16, "click anywhere else to close");
}


static void MahjongRenderRules()
{
	if (!gfMJShowRules || giMJOverlayKind != 1) return;

	INT32 const x = MJ_X(111), y = MJ_Y(40), w = 280, h = 280;
	MahjongFillRounded(x, y, w, h, 7, Get16BPPColor(FROMRGB(240, 220, 60)));
	MahjongFillRounded(x + 2, y + 2, w - 4, h - 4, 6, Get16BPPColor(FROMRGB(24, 32, 24)));

	SetFontAttributes(FONT14ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
	MPrint(x + 14, y + 12, "San Mona house rules");
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKWHITE, FONT_MCOLOR_BLACK, 0);
	MPrint(x + 14, y + 28, "Sichuan, bloody to the end");
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 14, y + 42, x + w - 14, y + 43, Get16BPPColor(FROMRGB(70, 84, 70)));

	static const char* const rules[] =
	{
		"108 tiles: Chars, Dots and Bams only. No winds, no dragons, no chi - but Pong (claim a discard matching your pair) and Kong (four of a kind, with a replacement draw) are allowed. Gold-barred tiles in the ponds are exposed sets.",
		"Exchange: after the deal, every player passes 3 tiles of ONE suit to a neighbour. The direction changes every hand.",
		"Void suit: every player then declares one suit. You cannot win while holding ANY tile of it. Void tiles show red in your hand - dump them.",
		"Winning: 4 sets (runs or triplets) plus a pair, or seven pairs. Win by self-draw or claim a discard with the Mahjong! button.",
		"Bloody battle: winners retire and play continues until three players have won or the wall is empty.",
		"Payments: self-draw collects 2000 from every player still in the hand; a claimed discard costs the discarder 6000. A match is 4 hands.",
		"Stakes: $250 buy-in, points settle at 20:1, the house rakes 10% of winnings. Short on cash? Kingpin fronts the buy-in at 20% vig. Every 7th day is the BLOODY INVITATIONAL: triple stakes.",
		"The left seat rotates: most nights Elliot, but Kingpin and Tony drop in. While nobody is seated, The House plays your chair.",
		"House history: founded 1999 above the Shady Lady, bankrolled by boxing debts, hosted on a server Darren 'borrowed' from the Drassen airport. The palace dial-up line was Elliot's idea; he has apologized for it daily since.",
	};
	// four pages, File Viewer style: three of rules, one from the management
	static const int pageStart[4] = { 0, 3, 6, 9 };
	int const page = std::max(0, std::min(3, static_cast<int>(gbMJRulesPage)));
	INT32 lineY = y + 52;
	if (page < 3)
	{
		for (int i = pageStart[page]; i < pageStart[page + 1]; ++i)
		{
			lineY += DisplayWrappedString(x + 14, lineY, w - 28, 2, FONT10ARIAL, FONT_MCOLOR_WHITE, rules[i], FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 9;
		}
	}
	else
	{
		// the about page: a word from the proprietor
		if (guiMJKingpinFace)
		{
			ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 13, lineY - 1, x + 13 + 60, lineY + 66,
						Get16BPPColor(FROMRGB(110, 40, 36)));
			BltVideoObject(FRAME_BUFFER, guiMJKingpinFace, 0, x + 14, lineY);
		}
		INT32 noteY = lineY;
		noteY += DisplayWrappedString(x + 84, noteY, w - 98, 2, FONT10ARIAL, FONT_MCOLOR_WHITE,
				"Friend,", FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 8;
		noteY += DisplayWrappedString(x + 84, noteY, w - 98, 2, FONT10ARIAL, FONT_MCOLOR_WHITE,
				"The rules are simple here. Play clean, pay your debts, and the Parlour will treat you better than the world outside ever has.",
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 8;
		noteY += DisplayWrappedString(x + 84, noteY, w - 98, 2, FONT10ARIAL, FONT_MCOLOR_WHITE,
				"Cross the house and... well. Read the stakes page again, slowly.",
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED) + 16;
		// the scrawl signs above the typed name, like a real letter
		if (guiMJSign) BltVideoObject(FRAME_BUFFER, guiMJSign, 0, x + 84, noteY);
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKWHITE, FONT_MCOLOR_BLACK, 0);
		MPrint(x + 84, noteY + 40, "Peter 'Kingpin' Klaus, proprietor");
		// an affirmative way out
		{
			INT32 const okX = x + (w - 96) / 2, okY = y + h - 56;
			bool const okHot = gusMouseXPos >= okX && gusMouseXPos <= okX + 96 &&
						gusMouseYPos >= okY && gusMouseYPos <= okY + 20;
			ColorFillVideoSurfaceArea(FRAME_BUFFER, okX, okY, okX + 96, okY + 20,
						Get16BPPColor(okHot ? FROMRGB(255, 255, 255) : FROMRGB(36, 96, 60)));
			ColorFillVideoSurfaceArea(FRAME_BUFFER, okX + 1, okY + 1, okX + 95, okY + 19,
						Get16BPPColor(FROMRGB(6, 36, 20)));
			SetFontAttributes(FONT10ARIAL, okHot ? FONT_MCOLOR_LTGREEN : FONT_GREEN, FONT_MCOLOR_BLACK, 0);
			MPrint(okX + 48 - StringPixLength("Understood", FONT10ARIAL) / 2, okY + 5, "Understood");
		}
	}

	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 14, y + h - 26, x + w - 14, y + h - 25, Get16BPPColor(FROMRGB(70, 84, 70)));
	// unavailable directions stay visible, just barely: super subtle mute
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
	if (page == 0) SetFontForegroundRGB(FROMRGB(40, 96, 58));
	MPrint(x + 14, y + h - 18, "< prev");
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
	if (page == 3) SetFontForegroundRGB(FROMRGB(40, 96, 58));
	MPrint(x + w - 14 - StringPixLength("next >", FONT10ARIAL), y + h - 18, "next >");
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
	ST::string const pager = ST::format("page {}/4 - click elsewhere to close", page + 1);
	MPrint(x + (w - StringPixLength(pager, FONT10ARIAL)) / 2, y + h - 18, pager);
}


// unified lobby framing: 1px border with softly rounded corners on felt
static void MahjongLobbyFrame(INT32 x, INT32 y, INT32 w, INT32 h, UINT16 frame)
{
	UINT16 const felt = Get16BPPColor(FROMRGB(92, 20, 24));
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + w, y + 1, frame);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y + h - 1, x + w, y + h, frame);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + 1, y + h, frame);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + w - 1, y, x + w, y + h, frame);
	// corners: clip back to felt, then set the diagonal pixel
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + 2, y + 2, felt);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + w - 2, y, x + w, y + 2, felt);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y + h - 2, x + 2, y + h, felt);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + w - 2, y + h - 2, x + w, y + h, felt);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 1, y + 1, x + 2, y + 2, frame);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + w - 2, y + 1, x + w - 1, y + 2, frame);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 1, y + h - 2, x + 2, y + h - 1, frame);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, x + w - 2, y + h - 2, x + w - 1, y + h - 1, frame);
}

// AIM-style home page: sign, tagline, warning, four link tiles, legalese
static void MahjongRenderLobby()
{
	if (guiMJFeltRed) BltVideoObject(FRAME_BUFFER, guiMJFeltRed, 0, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y);
	else ColorFillVideoSurfaceArea(FRAME_BUFFER, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
			LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y, Get16BPPColor(FROMRGB(92, 20, 24)));
	// the felt image stops at 381: paint the remaining rim felt too
	ColorFillVideoSurfaceArea(FRAME_BUFFER, LAPTOP_SCREEN_UL_X, MJ_Y(381),
			LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y, Get16BPPColor(FROMRGB(92, 20, 24)));
	if (guiMJLogo) BltVideoObject(FRAME_BUFFER, guiMJLogo, 0, MJ_X(61), MJ_Y(12));
	MahjongLobbyFrame(MJ_X(61), MJ_Y(12), 380, 100, Get16BPPColor(FROMRGB(110, 40, 36)));
	{
		SetFontAttributes(TINYFONT1, FONT_MCOLOR_DKWHITE, FONT_MCOLOR_BLACK, 0);
		MPrint(MJ_X(437) - StringPixLength(MJ_PAGE_VERSION, TINYFONT1), MJ_Y(16), MJ_PAGE_VERSION);
	}

	// warning block anchors the page bottom, same width as the sign
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(62), MJ_Y(297), MJ_X(440), MJ_Y(371), Get16BPPColor(FROMRGB(44, 12, 14)));
	MahjongLobbyFrame(MJ_X(61), MJ_Y(296), 380, 76, Get16BPPColor(FROMRGB(110, 40, 36)));
	if (guiMJDragon)
	{
		// maroon watermarks, contained inside the warning box
		BltVideoObject(FRAME_BUFFER, guiMJDragon, 6, MJ_X(70), MJ_Y(302));
		BltVideoObject(FRAME_BUFFER, guiMJDragon, 6, MJ_X(368), MJ_Y(302));
	}
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(78), MJ_Y(306), MJ_X(424), MJ_Y(308), Get16BPPColor(FROMRGB(200, 40, 40)));
	SetFontAttributes(FONT14ARIAL, FONT_MCOLOR_LTRED, FONT_MCOLOR_BLACK, 0);
	MPrint(MJ_X(251) - StringPixLength("WARNING!", FONT14ARIAL) / 2, MJ_Y(313), "WARNING!");
	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTRED, FONT_MCOLOR_BLACK, 0);
	ST::string const warn1 = "You must be of legal gambling age in Arulco (there is none), able to";
	ST::string const warn2 = "cover your losses, and polite to the management to enter this site.";
	MPrint(MJ_X(251) - StringPixLength(warn1, FONT10ARIAL) / 2, MJ_Y(330), warn1);
	MPrint(MJ_X(251) - StringPixLength(warn2, FONT10ARIAL) / 2, MJ_Y(342), warn2);
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(78), MJ_Y(358), MJ_X(424), MJ_Y(360), Get16BPPColor(FROMRGB(200, 40, 40)));

	struct LobbyTile { INT16 x, y; const char* caption; };
	static LobbyTile const tiles[4] =
	{
		{ 61, 124, "Play the Table" }, { 255, 124, "" }, // watch label drawn in parts
		{ 61, 210, "View Leaderboard" }, { 255, 210, "Read House Rules" },
	};
	for (int i = 0; i < 4; ++i)
	{
		INT32 const bx = MJ_X(tiles[i].x), by = MJ_Y(tiles[i].y);
		bool const hot = gMJLobbyRegion[i].uiFlags & MSYS_MOUSE_IN_AREA;
		ColorFillVideoSurfaceArea(FRAME_BUFFER, bx + 1, by + 1, bx + 185, by + 73,
					Get16BPPColor(FROMRGB(44, 12, 14)));
		MahjongLobbyFrame(bx, by, 186, 74,
					Get16BPPColor(hot ? FROMRGB(170, 80, 66) : FROMRGB(110, 40, 36)));
		switch (i)
		{
			case 0: // two tiles from the sheet
				if (guiMJTiles)
				{
					MahjongDrawTile(bx + 60, by + 8, MJ_TILE_W, MJ_TILE_H, 4, false);
					MahjongDrawTile(bx + 96, by + 8, MJ_TILE_W, MJ_TILE_H, 22, false);
				}
				break;
			case 1: // the whole table, seat by seat; the turn holder is ringed
			{
				int const turnSeat = gGame ? gGame->currentPlayer() : -1;
				for (int seatIdx = 0; seatIdx < 4; ++seatIdx)
				{
					INT32 const px = bx + 27 + seatIdx * 34, py = by + 10;
					if (seatIdx == turnSeat)
					{
						ColorFillVideoSurfaceArea(FRAME_BUFFER, px - 2, py - 2, px + 31, py + 35,
									Get16BPPColor(FROMRGB(240, 220, 60)));
					}
					ColorFillVideoSurfaceArea(FRAME_BUFFER, px, py, px + 29, py + 33,
								Get16BPPColor(FROMRGB(9, 34, 21)));
					if (seatIdx == 0)
					{
						// whoever holds your chair: the shill, or you
						SGPVSurface* const feed = gfMJExhibition ? guiMJShillSurf : guiMJSelfFaceSurf;
						if (feed)
						{
							SGPBox const src = { 4, 0, 98, 110 };
							SGPBox const dst = { static_cast<UINT16>(px), static_cast<UINT16>(py), 29, 33 };
							BltStretchVideoSurface(FRAME_BUFFER, feed, &src, &dst);
						}
						else if (guiMJStatic)
						{
							BltVideoObject(FRAME_BUFFER, guiMJStatic, 7, px, py); // empty seat
						}
					}
					else if (guiMJFace33[seatIdx - 1])
					{
						BltVideoObject(FRAME_BUFFER, guiMJFace33[seatIdx - 1], 0, px, py);
					}
				}
				break;
			}
			case 2: // chips
				if (guiMJChips)
				{
					for (INT32 c = 0; c < 6; ++c)
						BltVideoObject(FRAME_BUFFER, guiMJChips, c % 2, bx + 59 + c * 11, by + 24);
				}
				break;
			case 3:
				SetFontAttributes(FONT14ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
				MPrint(bx + 90, by + 20, "?");
				break;
		}
		SetFontAttributes(FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
		if (i == 1)
		{
			// "Watch Live Table" - the Live part blinks like it means it
			bool const blink = (MahjongNow() / 600) % 2 == 0;
			INT32 const w1 = StringPixLength("Watch ", FONT12ARIAL);
			INT32 const w2 = StringPixLength("Live", FONT12ARIAL);
			INT32 const w3 = StringPixLength(" Table", FONT12ARIAL);
			INT32 cx = bx + 93 - (w1 + w2 + w3) / 2;
			MPrint(cx, by + 54, "Watch ");
			SetFontAttributes(FONT12ARIAL, blink ? FONT_MCOLOR_LTRED : FONT_MCOLOR_RED, FONT_MCOLOR_BLACK, 0);
			MPrint(cx + w1, by + 54, "Live");
			SetFontAttributes(FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
			MPrint(cx + w1 + w2, by + 54, " Table");
		}
		else
		{
			MPrint(bx + 93 - StringPixLength(tiles[i].caption, FONT12ARIAL) / 2, by + 54, tiles[i].caption);
		}
	}

	SetFontAttributes(TINYFONT1, FONT_MCOLOR_DKWHITE, FONT_MCOLOR_BLACK, 0);
	ST::string const foot = "(c) 1998-1999 Klaus Entertainment Ltd. Complaints to Spike, in person.";
	MPrint(MJ_X(251) - StringPixLength(foot, TINYFONT1) / 2, MJ_Y(381), foot);
}

static void MahjongRenderLadder()
{
	// red felt room: the money side of the establishment
	if (guiMJFeltRed) BltVideoObject(FRAME_BUFFER, guiMJFeltRed, 0, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y);
	else ColorFillVideoSurfaceArea(FRAME_BUFFER, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
			LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y, Get16BPPColor(FROMRGB(92, 20, 24)));

	// the carrier panel hugs the content; the felt owns the rest of the view
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(52), MJ_Y(22), MJ_X(450), MJ_Y(342), Get16BPPColor(FROMRGB(110, 40, 36)));
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(54), MJ_Y(24), MJ_X(448), MJ_Y(340), Get16BPPColor(FROMRGB(40, 12, 14)));

	if (guiMJDragon)
	{
		BltVideoObject(FRAME_BUFFER, guiMJDragon, 3, MJ_X(384), MJ_Y(28)); // gold, top right
	}

	// back arrow shares the title row
	{
		bool const backHot = gMJLadderBackRegion.uiFlags & MSYS_MOUSE_IN_AREA;
		UINT16 const backCol = Get16BPPColor(backHot ? FROMRGB(255, 232, 130) : FROMRGB(216, 172, 64));
		ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(64), MJ_Y(57), MJ_X(84), MJ_Y(59), backCol);
		for (INT32 r = 0; r < 5; ++r)
		{
			ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(64 + r), MJ_Y(58 - r), MJ_X(65 + r), MJ_Y(59 + r), backCol);
		}
	}
	// header sits left-aligned with the table, padded off the top edge
	SetFontAttributes(FONT14ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
	MPrint(MJ_X(96), MJ_Y(50), "HOUSE LADDER");
	SetFontAttributes(TINYFONT1, FONT_MCOLOR_DKWHITE, FONT_MCOLOR_BLACK, 0);
	MPrint(MJ_X(436) - StringPixLength(MJ_PAGE_VERSION, TINYFONT1), MJ_Y(54), MJ_PAGE_VERSION);

	struct LadderRow { ST::string handle; const char* name; INT32 rating; ST::string note; };
	std::vector<LadderRow> rows;
	rows.push_back({ "@dejdranna666", "Deidranna", 2141, "(disputed)" });
	rows.push_back({ "@the_house",    "Kingpin",   1899, "" });
	rows.push_back({ "@ringside_d",   "Darren",    1774, "" });
	rows.push_back({ "@heir2throne",  "Enrico",    1687, "" });
	rows.push_back({ "@no_refunds",   "Tony",      1521, "" });
	rows.push_back({ "@shady_lady",   "Layla",     1490, "" });
	rows.push_back({ "@e11iot",       "Elliot",    1104, "abandoned: 0" });
	rows.push_back({ gMJSelfNick.empty() ? ST::string("@you") : ST::format("@{}", gMJSelfNick.to_lower()),
			"you", MahjongPlayerRating(), "" });
	std::sort(rows.begin(), rows.end(), [](LadderRow const& a, LadderRow const& b) { return a.rating > b.rating; });

	// inset list box, chat-style: sunken well plus a decorative rail
	INT32 const boxT = 76, boxB = boxT + 16 + (static_cast<INT32>(rows.size()) + 1) * 18 + 6;
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(66), MJ_Y(boxT), MJ_X(436), MJ_Y(boxB), Get16BPPColor(FROMRGB(140, 60, 52)));
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(67), MJ_Y(boxT + 1), MJ_X(435), MJ_Y(boxB - 1), Get16BPPColor(FROMRGB(30, 8, 10)));
	// rail
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(425), MJ_Y(boxT + 2), MJ_X(434), MJ_Y(boxB - 2), Get16BPPColor(FROMRGB(24, 6, 8)));
	ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(427), MJ_Y(boxT + 4), MJ_X(432), MJ_Y(boxB - 4), Get16BPPColor(FROMRGB(96, 34, 30)));

	SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
	MPrint(MJ_X(96), MJ_Y(boxT + 4), "player");
	MPrint(MJ_X(196), MJ_Y(boxT + 4), "handle");
	MPrint(MJ_X(316), MJ_Y(boxT + 4), "rating");
	INT32 rowY = boxT + 18;
	for (size_t i = 0; i < rows.size(); ++i, rowY += 18)
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(68), MJ_Y(rowY - 2), MJ_X(424), MJ_Y(rowY + 15),
					Get16BPPColor(i % 2 == 0 ? FROMRGB(66, 18, 18) : FROMRGB(50, 13, 15)));
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
		MPrint(MJ_X(76), MJ_Y(rowY + 2), ST::format("{}.", i + 1));
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
		MPrint(MJ_X(96), MJ_Y(rowY + 2), rows[i].name);
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKWHITE, FONT_MCOLOR_BLACK, 0);
		MPrint(MJ_X(196), MJ_Y(rowY + 2), rows[i].handle);
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTGREEN, FONT_MCOLOR_BLACK, 0);
		bool const isYou = strcmp(rows[i].name, "you") == 0;
		ST::string const rating = ST::format("{}{}", rows[i].rating,
				isYou && MahjongRatingProvisional() ? "*" : "");
		MPrint(MJ_X(344) - StringPixLength(rating, FONT10ARIAL), MJ_Y(rowY + 2), rating);
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKWHITE, FONT_MCOLOR_BLACK, 0);
		MPrint(MJ_X(352), MJ_Y(rowY + 2), rows[i].note);
	}

	// the house keeps one name on the list as a warning
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(68), MJ_Y(rowY - 2), MJ_X(424), MJ_Y(rowY + 15),
					Get16BPPColor(rows.size() % 2 == 0 ? FROMRGB(66, 18, 18) : FROMRGB(50, 13, 15)));
		// the warning row reads all red, struck through, and stays in bounds
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTRED, FONT_MCOLOR_BLACK, 0);
		MPrint(MJ_X(76), MJ_Y(rowY + 2), "-.");
		MPrint(MJ_X(96), MJ_Y(rowY + 2), "Iggy");
		ColorFillVideoSurfaceArea(FRAME_BUFFER, MJ_X(95), MJ_Y(rowY + 7), MJ_X(96) + StringPixLength("Iggy", FONT10ARIAL) + 2, MJ_Y(rowY + 8),
					Get16BPPColor(FROMRGB(200, 40, 40)));
		MPrint(MJ_X(196), MJ_Y(rowY + 2), "@glass_jaw");
		MPrint(MJ_X(344) - StringPixLength("-", FONT10ARIAL), MJ_Y(rowY + 2), "-");
		MPrint(MJ_X(352), MJ_Y(rowY + 2), "BANNED");
		rowY += 18;
	}

	INT32 secY = boxB + 14;
	SetFontAttributes(FONT12ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
	MPrint(MJ_X(76), MJ_Y(secY), "RECENT RESULTS");
	secY += 18;
	if (gMJHistory.empty())
	{
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_DKWHITE, FONT_MCOLOR_BLACK, 0);
		MPrint(MJ_X(76), MJ_Y(secY), "no completed matches yet. the felt is patient.");
	}
	else
	{
		for (size_t i = gMJHistory.size(); i-- > 0; secY += 13)
		{
			MahjongMatchRecord const& r = gMJHistory[i];
			SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, 0);
			MPrint(MJ_X(76), MJ_Y(secY), ST::format("Day {}: finished {}/4", r.day, r.place));
			SetFontAttributes(FONT10ARIAL, r.net >= 0 ? FONT_MCOLOR_LTGREEN : FONT_MCOLOR_LTRED, FONT_MCOLOR_BLACK, 0);
			MPrint(MJ_X(210), MJ_Y(secY), ST::format("{}{} pts", r.net >= 0 ? "+" : "", r.net));
		}
	}

	// the complaints department, such as it is
	if (gubMJReportCount > 0)
	{
		static const char* const reportLine[4] =
		{
			"report filed. Mr. Klaus will review it personally.",
			"your report joined the pile. the pile is load-bearing.",
			"noted. the player in question owns the server.",
			"thank you. Spike has been notified. Spike is always notified.",
		};
		SetFontAttributes(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK, 0);
		ST::string const line = reportLine[std::min<UINT8>(gubMJReportCount, 4) - 1];
		MPrint(MJ_X(434) - StringPixLength(line, FONT10ARIAL), MJ_Y(304), line);
	}
}

void RenderMahjong()
{
	if (guiMJState == MJUI_LOBBY || guiMJState == MJUI_LADDER)
	{
		if (guiMJState == MJUI_LOBBY) MahjongRenderLobby();
		else MahjongRenderLadder();
		MarkButtonsDirty();
		RenderWWWProgramTitleBar();
		InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
		return;
	}
	// felt table
	if (guiMJFelt)
	{
		BltVideoObject(FRAME_BUFFER, guiMJFelt, 0, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y);
	}
	else
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
						LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y, Get16BPPColor(FROMRGB(16, 84, 44)));
	}

	// the lamp hangs over your seat; the felt ends at a wooden lip
	if (guiMJGlow) BltVideoObject(FRAME_BUFFER, guiMJGlow, 0, MJ_X(11), MJ_Y(MJ_HAND_Y - 28));
	if (guiMJRail) BltVideoObject(FRAME_BUFFER, guiMJRail, 0, MJ_X(0), MJ_Y(MJ_CHAT_Y - 7));

	MahjongRenderTopPanels();


	MahjongRenderPonds();
	MahjongRenderHand();
	MahjongRenderChatBar();
	MahjongRenderInfoBlock();
	MahjongRenderVoidButtons();
	MahjongRenderOverlay();
	MahjongRenderRules();
	MahjongRenderGuestbook();

	MarkButtonsDirty();
	RenderWWWProgramTitleBar();
	InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
}


// one action of the House's exhibition game: driven from the idle tick and
// from a fast-forward loop so visitors arrive at a table already mid-game
static void MahjongExhibitionStep()
{
	switch (gGame->phase())
	{
		case MahjongGame::Phase::ExchangeSelect:
		{
			MahjongGame::TileId give[3];
			MahjongGame::AiChooseExchange(gGame->player(0).counts, give);
			gGame->SetHumanExchange(give);
			gGame->SetHumanVoidSuit(MahjongGame::AiChooseVoidSuit(gGame->player(0).counts));
			{
				UINT32 const r = MahjongNow();
				int const gwho = 1 + static_cast<int>(r % 3);
				MahjongGuestPack const* const g = MahjongPackFor(gwho);
				MahjongSay(gwho, g ? g->greet[MahjongChatRoll() % 3]
								: gMJChatGreet[gwho - 1][MahjongChatRoll() % 12]);
			}
			break;
		}
		case MahjongGame::Phase::AwaitDraw:
			gGame->DrawForCurrent();
			break;
		case MahjongGame::Phase::AwaitDiscard:
			if (gGame->CanTsumo())
			{
				int const w = gGame->currentPlayer();
				gGame->ResolveTsumo();
				MahjongPlay(MJ_SND_WIN, LOWVOLUME);
				MahjongEventSay(ST::format("{} {} by self-draw",
						MahjongSeatName(w), MahjongVerb(w, "win", "wins")));
			}
			else
			{
				gGame->Discard(gGame->AiChooseDiscardSkilled(gGame->currentPlayer()));
				MahjongPlay(MJ_SND_AIDROP, LOWVOLUME);
			}
			break;
		case MahjongGame::Phase::RonWindow:
		{
			int const claimant = gGame->RonClaimant();
			if (claimant >= 0)
			{
				int const d = gGame->lastDiscarder();
				gGame->ResolveRon(claimant);
				MahjongPlay(MJ_SND_WIN, LOWVOLUME);
				MahjongEventSay(ST::format("{} {} {} discard",
						MahjongSeatName(claimant),
						MahjongVerb(claimant, "claim", "claims"),
						d == 0 ? ST::string("your")
							: ST::format("{}'s", MahjongSeatName(d))));
				break;
			}
			bool kongPossible = false;
			int const mc = gGame->MeldClaimant(kongPossible);
			MahjongGame::TileId const claimed = gGame->lastDiscard();
			if (mc >= 0 && kongPossible && gGame->AiWantsKong(mc))
			{
				gGame->ClaimKong(mc);
				MahjongPlay(MJ_SND_MELD, LOWVOLUME);
				MahjongSystemSay(ST::format("{} {} the {}", MahjongSeatName(mc),
						MahjongVerb(mc, "kong", "kongs"), MahjongTileLabel(claimed)));
			}
			else if (mc >= 0 && gGame->AiWantsPong(mc))
			{
				gGame->ClaimPong(mc);
				MahjongPlay(MJ_SND_MELD, LOWVOLUME);
				if (mc > 0 && MahjongChatRoll() % 2 == 0)
				{
					MahjongGuestPack const* const g = MahjongPackFor(mc);
					MahjongSay(mc, g ? g->claim[MahjongChatRoll() % 3]
							: gMJChatClaim[mc - 1][MahjongChatRoll() % 8]);
				}
			}
			else gGame->PassRon();
			break;
		}
		case MahjongGame::Phase::HandEnd:
			gGame->NewHand();
			MahjongPlay(MJ_SND_DEAL, LOWVOLUME);
			break;
		case MahjongGame::Phase::MatchEnd:
			MahjongStartExhibition();
			break;
		default:
			break;
	}
	// spectators deserve the floor show too
	++guiMJQuipCounter;
	if (guiMJQuipCounter % 29 == 5)
	{
		int const who = 1 + static_cast<int>(MahjongChatRoll() % 3);
		MahjongGuestPack const* const g = MahjongPackFor(who);
		if (g)
		{
			MahjongSay(who, MahjongChatRoll() % 2 == 0
					? g->war[MahjongWarTier()] : g->idle[MahjongChatRoll() % 3]);
		}
		else
		{
			MahjongSay(who, MahjongChatRoll() % 2 == 0
					? gMJChatWar[who - 1][MahjongWarTier() * 4 + MahjongChatRoll() % 4]
					: gMJChatIdle[who - 1][MahjongChatRoll() % 30]);
		}
	}
	else if (guiMJQuipCounter % 67 == 31)
	{
		MahjongSystemSay(gMJService[MahjongChatRoll() % MJ_SERVICE_COUNT]);
	}
	else if (guiMJQuipCounter % 53 == 19 && giMJBotWho < 0 && GetProfile(QUEEN).bLife > 0 && giMJSeat2Persona == MJP2_QUEEN)
	{
		MahjongBicker const& b = gMJBicker[MahjongChatRoll() % 30];
		MahjongSay(1, b.open);
		MahjongBotQueueReply(b.replyWho, b.reply, MahjongChatRoll());
	}
	else if (guiMJQuipCounter % 97 == 53)
	{
		MahjongVisitorArrives();
	}
	else if (guiMJQuipCounter % 71 == 37 && giMJBotWho < 0)
	{
		MahjongCameo const& c = gMJCameo[MahjongChatRoll() % 18];
		MahjongSystemSay(c.join);
		MahjongBotQueueReply(c.reactWho, c.react, MahjongChatRoll());
		gMJPendingCameoLeave = c.leave;
		guiMJCameoLeaveTime = MahjongNow() + 9000 + MahjongChatRoll() % 6000;
	}
}

void HandleMahjong()
{
	MahjongHandleFaceLife();
	// somebody is at their keyboard: build their performance, then play it
	if (MahjongTypingRowActive())
	{
		MahjongPendingLine& pend = gMJPending.front();
		// hold the indicator until they would actually be done
		if (pend.dueTime < guiMJTypingFloor)
		{
			pend.dueTime = guiMJTypingFloor + 900 +
					static_cast<UINT32>(pend.text.size()) * MJ_TYPE_MS;
			guiMJQueueTail = std::max(guiMJQueueTail, pend.dueTime);
		}
		// keep the ellipsis breathing while they compose
		static UINT32 uiLastDotPhase = 99;
		UINT32 const phase = (MahjongNow() / 110) % 6;
		if (phase != uiLastDotPhase)
		{
			uiLastDotPhase = phase;
			MahjongRedraw();
		}
	}
	// ease toward the target: time-based, so the speed does not depend on
	// how often the page happens to repaint
	MahjongReconcileScroll();
	if (giMJScrollY != giMJScrollTarget)
	{
		static UINT32 uiLastEase = 0;
		UINT32 const now = MahjongNow();
		UINT32 const dt = std::min<UINT32>(64, now - uiLastEase);
		if (dt >= 16)
		{
			uiLastEase = now;
			INT32 const gap = giMJScrollTarget - giMJScrollY;
			INT32 step = gap * static_cast<INT32>(dt) / 90;
			if (step == 0) step = gap > 0 ? 1 : -1;
			giMJScrollY += step;
			if (std::abs(giMJScrollTarget - giMJScrollY) <= 1) giMJScrollY = giMJScrollTarget;
			MahjongRedraw();
		}
	}
	// the terminal prints its own lines out, fast
	if (guiMJSysTypeLen > 0)
	{
		std::size_t const shown = (MahjongNow() - guiMJSysTypeStart) / MJ_SYS_TYPE_MS;
		static std::size_t uiLastShown = 0;
		if (shown != uiLastShown)
		{
			uiLastShown = shown;
			if (shown > guiMJSysTypeLen) guiMJSysTypeLen = 0;
			MahjongRedraw();
		}
	}
	// the visiting champion says their piece and moves on
	// they cannot leave before their own words have landed: the queue is
	// the room's only clock
	if (gfMJVisitorHere && MahjongNow() >= guiMJVisitorLeaves &&
		MahjongNow() >= guiMJQueueTail)
	{
		// they go, but their handle and face stay so their lines keep them
		gfMJVisitorHere = FALSE;
		MahjongSystemSay(ST::format("{} has left the room", gMJVisitorHandle));
	}

	// the wanderers return once the room has moved on without them
	for (int away = 1; away <= 3; ++away)
	{
		if (guiMJAwayUntil[away] == 0 || MahjongNow() < guiMJAwayUntil[away]) continue;
		guiMJAwayUntil[away] = 0;
		static const char* const backLine[4] =
		{
			"...ok back",
			"back. what did I miss",
			"sorry! I'm back!!",
			"right. where were we",
		};
		MahjongSay(away, backLine[MahjongChatRoll() % 4]);
	}
	MahjongFlushPending();
	// hover highlights must repaint the moment the cursor moves on or off
	{
		UINT32 uiHover = 0;
		for (int i = 0; i < 4; ++i)
		{
			if (gMJLobbyRegion[i].uiFlags & MSYS_MOUSE_IN_AREA) uiHover |= 1u << i;
		}
		if (gMJLadderBackRegion.uiFlags & MSYS_MOUSE_IN_AREA) uiHover |= 1u << 4;
		for (int i = 0; i < 3; ++i)
		{
			if (gMJIconRegion[i].uiFlags & MSYS_MOUSE_IN_AREA) uiHover |= 1u << (5 + i);
		}
		if (gMJChatUpRegion.uiFlags & MSYS_MOUSE_IN_AREA) uiHover |= 1u << 8;
		if (gMJChatDownRegion.uiFlags & MSYS_MOUSE_IN_AREA) uiHover |= 1u << 9;
		if ((gMJChatRegion.uiFlags & MSYS_MOUSE_IN_AREA) &&
			gMJChatRegion.MouseXPos >= MJ_X(MJ_CHAT_W - 15)) uiHover |= 1u << 10;
		if (gfMJShowRules && giMJOverlayKind == 1 && gbMJRulesPage == 3)
		{
			INT32 const okX = MJ_X(111) + (280 - 96) / 2, okY = MJ_Y(40) + 280 - 56;
			if (gusMouseXPos >= okX && gusMouseXPos <= okX + 96 &&
				gusMouseYPos >= okY && gusMouseYPos <= okY + 20) uiHover |= 1u << 11;
		}
		static UINT32 uiLastHover = 0;
		if (uiHover != uiLastHover)
		{
			uiLastHover = uiHover;
			MahjongRedraw();
		}
	}
	// the lurker leaves as quietly as they came
	if (!gMJPendingCameoLeave.empty() && MahjongNow() >= guiMJCameoLeaveTime)
	{
		MahjongSystemSay(gMJPendingCameoLeave);
		gMJPendingCameoLeave.clear();
	}

	// deliver a queued voice sample
	if (!gMJPendingSound.empty() && MahjongNow() >= guiMJPendingSoundTime)
	{
		MahjongPlay(gMJPendingSound.c_str(), MIDVOLUME);
		guiMJVoiceBusyUntil = std::max(guiMJVoiceBusyUntil, MahjongNow() + 2800);
		gMJPendingSound.clear();
	}

	// deliver a pending chatbot reply in any state
	if (giMJBotWho >= 0 && MahjongNow() >= guiMJBotDueTime)
	{
		int const who = giMJBotWho;
		giMJBotWho = -1;
		MahjongSay(who, gMJBotLine);
	}

	// caret blink on the chat input line; also animates the static feeds
	{
		static UINT32 uiLastCaretPhase = 0;
		UINT32 const phase = MahjongNow() / 500 % 2;
		if (phase != uiLastCaretPhase)
		{
			uiLastCaretPhase = phase;
			MahjongRedraw();
		}
	}

	// "poor connection": drop a random opponent's video feed to static
	if (guiMJState == MJUI_PLAYER_TURN || guiMJState == MJUI_AI_THINK ||
		guiMJState == MJUI_RON_WINDOW || guiMJState == MJUI_ANNOUNCE)
	{
		UINT32 const now = MahjongNow();
		if (giMJGlitchWho < 0 && now >= guiMJNextGlitch)
		{
			giMJGlitchWho = 1 + static_cast<int>(now % 3);
			// most are a blink; now and then somebody genuinely drops out
			bool const outage = MahjongChatRoll() % 4 == 0;
			guiMJGlitchEnd = now + (outage ? 5000 + MahjongChatRoll() % 6000 : 450);
			if (outage) MahjongSystemSay(ST::format("{} lost connection", MahjongChatHandle(giMJGlitchWho)));
			MahjongPlay(LAPTOPDIR "/static4.wav", LOWVOLUME, MahjongSeatPan(giMJGlitchWho));
			MahjongRedraw();
		}
		else if (giMJGlitchWho >= 0 && now >= guiMJGlitchEnd)
		{
			giMJGlitchWho = -1;
			guiMJNextGlitch = now + 9000 + now % 9000;
			MahjongRedraw();
		}
	}

	if (!gGame) return;

	switch (guiMJState)
	{
		case MJUI_DEALING:
			if (MahjongNow() >= guiMJNextEventTime)
			{
				++guiMJDealStep;
				if (guiMJDealStep >= MJ_DEAL_STEPS)
				{
					MahjongPlay(MJ_SND_MELD, MIDVOLUME); // the closing TAK
					MahjongEnterState(MJUI_EXCHANGE);
				}
				else
				{
					MahjongPlay(MJ_SND_DEAL, LOWVOLUME);
					// tiles patter in faster and faster
					guiMJNextEventTime = MahjongNow() + std::max(35, 130 - static_cast<INT32>(guiMJDealStep) * 8);
					MahjongRedraw();
				}
			}
			break;

		case MJUI_AI_THINK:
			if (MahjongNow() >= guiMJNextEventTime)
			{
				// the scandal: once in a blue moon a hand simply gets voided
				if (guiMJQuipCounter % 149 == 97 && !gfMJExhibition &&
					gGame->player(2).discards.size() > 5 && GetProfile(QUEEN).bLife > 0 && giMJSeat2Persona == MJP2_QUEEN)
				{
					gGame->AbortHand();
					MahjongSystemSay("IRREGULARITY - an extra tile found at Deidranna's seat. Hand VOID.");
					MahjongSay(2, "that tile was PLANTED. Elliot, find the traitor.");
					MahjongBotQueueReply(3, "it fell out of your sleeve, Your Highn- I mean, PLANTED, yes.", MahjongChatRoll());
					MahjongEnterState(MJUI_HAND_END);
					break;
				}
				// Kingpin notices a thin wallet
				if (guiMJQuipCounter % 71 == 39 && GetCurrentBalance() < 2000 && !gfMJExhibition)
				{
					MahjongSystemSay("private message from K.: your account looks thin, friend. my door is open.");
				}
				gGame->DrawForCurrent();
				if (gGame->phase() == MahjongGame::Phase::HandEnd)
				{
					MahjongEnterState(MJUI_HAND_END);
					break;
				}
				// self-kongs: bank the meld, mind the robbers
				for (MahjongGame::TileId t : gGame->SelfKongOptions())
				{
					if (!gGame->AiWantsSelfKong(gGame->currentPlayer(), t)) continue;
					if (gGame->IsAddedKong(t))
					{
						int const robber = gGame->RobKongClaimant(t);
						if (robber == 0)
						{
							gMJRobTile = t;
							MahjongEnterState(MJUI_ROB_WINDOW);
							break;
						}
						if (robber > 0)
						{
							int const declarer = gGame->currentPlayer();
							gGame->ResolveRobKong(robber, t);
							MahjongSystemSay(ST::format("{} ROBS {}'s kong of the {}",
									MahjongSeatName(robber), MahjongSeatName(declarer), MahjongTileLabel(t)));
							MahjongAnnounce(gMJWinTaunt[robber][MahjongChatRoll() % 8]);
							break;
						}
					}
					int const declarer = gGame->currentPlayer();
					gGame->DeclareSelfKong(t);
					MahjongPlay(MJ_SND_MELD, BTNVOLUME, MahjongSeatPan(declarer));
					MahjongSystemSay(ST::format("{} {} the {} - the table pays the bonus",
							MahjongSeatName(declarer),
							MahjongVerb(declarer, "kong", "kongs"),
							MahjongTileLabel(t)));
					break;
				}
				if (guiMJState != MJUI_AI_THINK) break; // a rob or window interrupted the turn
				if (gGame->phase() == MahjongGame::Phase::HandEnd)
				{
					MahjongEnterState(MJUI_HAND_END);
					break;
				}
				if (gGame->CanTsumo())
				{
					int const winner = gGame->currentPlayer();
					gGame->ResolveTsumo();
					MahjongChatOnWin(winner, -1);
					{
						MahjongGuestPack const* const g = MahjongPackFor(winner);
						MahjongAnnounce(g ? g->winTaunt[MahjongChatRoll() % 3]
										: gMJWinTaunt[winner][MahjongChatRoll() % 8]);
					}
					break;
				}
				{
					int const who = gGame->currentPlayer();
					MahjongGame::TileId const dumped = gGame->AiChooseDiscardSkilled(who);
					MahjongPlay(MJ_SND_AIDROP, LOWVOLUME, MahjongSeatPan(who));
					gGame->Discard(dumped);

					// contextual table talk about the hand they now hold
					int const shanten = gGame->ShantenFor(who);
					if (shanten == 0 && !gfMJSaidTenpai[who])
					{
						gfMJSaidTenpai[who] = TRUE;
						{
							MahjongGuestPack const* const g = MahjongPackFor(who);
							bool const leak = (who == 3 && !g) || MahjongNow() % 5 < 2;
							if (leak) MahjongSay(who, g ? g->tenpai[MahjongChatRoll() % 3]
										: gMJChatTenpai[who - 1][MahjongChatRoll() % 4]);
						}
					}
					else if (MahjongGame::SuitOf(dumped) == gGame->player(who).voidSuit &&
						gGame->player(who).discards.size() > 4 && !gfMJSaidVoidDraw[who])
					{
						gfMJSaidVoidDraw[who] = TRUE;
						if (MahjongNow() % 3 == 0 && !MahjongPackFor(who))
						{
							MahjongSay(who, gMJChatVoidDraw[who - 1][MahjongChatRoll() % 4]);
						}
					}
					else if (shanten >= 4 && gGame->wallRemaining() < 30 && !gfMJSaidBadHand[who])
					{
						gfMJSaidBadHand[who] = TRUE;
						{
							MahjongGuestPack const* const g = MahjongPackFor(who);
							if (MahjongNow() % 3 < 2) MahjongSay(who, g ? g->badhand[MahjongChatRoll() % 3]
										: gMJChatBadHand[who - 1][MahjongChatRoll() % 4]);
						}
					}
					MahjongAfterDiscard();
				}
			}
			break;

		case MJUI_ROB_WINDOW:
			if (MahjongNow() >= guiMJNextEventTime)
			{
				int const declarer = gGame->currentPlayer();
				gGame->DeclareSelfKong(gMJRobTile);
				MahjongPlay(MJ_SND_MELD, BTNVOLUME, MahjongSeatPan(declarer));
				MahjongEventSay(ST::format("{} {} the {} - everyone pays the bonus",
						MahjongSeatName(declarer),
						MahjongVerb(declarer, "kong", "kongs"),
						MahjongTileLabel(gMJRobTile)));
				MahjongEnterState(declarer == 0 ? MJUI_PLAYER_TURN : MJUI_AI_THINK);
			}
			break;

		case MJUI_CLAIM_WINDOW:
			if (MahjongNow() >= guiMJNextEventTime)
			{
				gGame->PassRon();
				MahjongContinueAfterEvent();
			}
			break;

		case MJUI_RON_WINDOW:
			if (MahjongNow() >= guiMJNextEventTime)
			{
				gGame->PassRon();
				MahjongContinueAfterEvent();
			}
			else
			{
				// countdown in the prompt + flashing claim button
				UINT32 const msLeft = guiMJNextEventTime - MahjongNow();
				static UINT32 uiLastShownSecs = 99;
				UINT32 const secs = msLeft / 1000 + 1;
				if (secs != uiLastShownSecs)
				{
					uiLastShownSecs = secs;
					gMJMessage = ST::format("{} discarded your winning tile! Click Mahjong! NOW ({}...)",
							MahjongSeatName(gGame->lastDiscarder()), secs);
					MahjongRedraw();
				}
				if (guiMJMahjongBtn)
				{
					bool const hot = MahjongNow() / 300 % 2 == 0;
					guiMJMahjongBtn->SpecifyGeneralTextAttributes("Mahjong!", FONT12ARIAL,
							hot ? FONT_MCOLOR_RED : FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK);
					MarkAButtonDirty(guiMJMahjongBtn);
				}
			}
			break;

		case MJUI_ANNOUNCE:
			if (MahjongNow() >= guiMJNextEventTime)
			{
				MahjongContinueAfterEvent();
			}
			break;

		case MJUI_PLAYER_TURN:
			// flash the claim button while a self-draw win is on the table
			if (guiMJMahjongBtn && gGame->CanTsumo())
			{
				bool const hot = MahjongNow() / 300 % 2 == 0;
				guiMJMahjongBtn->SpecifyGeneralTextAttributes("Mahjong!", FONT12ARIAL,
						hot ? FONT_MCOLOR_RED : FONT_MCOLOR_LTYELLOW, FONT_MCOLOR_BLACK);
				MarkAButtonDirty(guiMJMahjongBtn);
			}
			break;

		case MJUI_HAND_END:
			// payout meter: repaint while the deltas count up, with ticks
			if (MahjongNow() - guiMJDeltaAnimStart < 1000)
			{
				static UINT32 uiLastTickStep = 0;
				UINT32 const step = (MahjongNow() - guiMJDeltaAnimStart) / 100;
				if (step != uiLastTickStep)
				{
					uiLastTickStep = step;
					if (step % 3 == 0) MahjongPlay(MJ_SND_SELECT, LOWVOLUME);
					MahjongRedraw();
				}
			}
			// "video chat" face: blink/expression animation on the overlay
			if (gpMJOverlayFace && guiMJFaceSurface && !gfMJShowRules)
			{
				HandleAutoFaces();
				INT32 const fx = MJ_X(61) + 12, fy = MJ_Y(50) + 14;
				SGPBox const src = { 0, 0, MJ_FACE_W, MJ_FACE_CROP_H };
				BltVideoSurface(FRAME_BUFFER, guiMJFaceSurface, fx, fy, &src);
				InvalidateRegion(fx, fy, fx + MJ_FACE_W, fy + MJ_FACE_CROP_H);
			}
			break;

		case MJUI_IDLE:
		case MJUI_LOBBY:
		case MJUI_LADDER:
			// The House plays on: advance the exhibition one action at a time
			if (gfMJExhibition && gGame)
			{
				static UINT32 uiNextExhibitionAction = 0;
				if (MahjongNow() < uiNextExhibitionAction) break;
				uiNextExhibitionAction = MahjongNow() + 700;
				MahjongExhibitionStep();
				MahjongRedraw();
			}
			break;

		case MJUI_CHOOSE_VOID:
		case MJUI_MATCH_END:
		default:
			break;
	}
}
