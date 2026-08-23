#include "Feline.h"

#include "FelineCat.h"

#include "Campaign_Types.h"
#include "Cursors.h"
#include "Finances.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "HImage.h"
#include "VObject.h"
#include "Input.h"
#include "Laptop.h"
#include "LaptopSave.h"
#include "MercPortrait.h"
#include "MouseSystem.h"
#include "Quests.h"
#include "Soldier_Profile.h"
#include "Timer_Control.h"
#include "VSurface.h"
#include "Video.h"
#include "WordWrap.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdio>
#include <string_theory/format>
#include <string_theory/string>

#include <SDL_keycode.h>

// The Arulco Feline Society. Founded 1994, nine members, one constitution,
// a championship that has been "postponed" two years running, and a breed
// standard for an animal that eats people. The softest page on ArulcoNet,
// which is why the war shows through it so clearly.

static FelinePersist gFeline = {};

namespace
{
#define FE_X(x) ((INT32)(LAPTOP_SCREEN_UL_X + (x)))
#define FE_Y(y) ((INT32)(LAPTOP_SCREEN_WEB_UL_Y + (y)))
#define FE_PAGE_W 500
#define FE_PAGE_H 390

	// the 1994 palette: parchment, teal, link blue, and a lot of grey rule
	constexpr UINT32 FE_RGB_PAPER  = FROMRGB(236, 228, 204);
	constexpr UINT32 FE_RGB_CARD   = FROMRGB(248, 244, 232);
	constexpr UINT32 FE_RGB_TEAL   = FROMRGB(0, 96, 96);
	constexpr UINT32 FE_RGB_TEAL_D = FROMRGB(0, 64, 64);
	constexpr UINT32 FE_RGB_INK    = FROMRGB(40, 36, 30);
	constexpr UINT32 FE_RGB_RULE   = FROMRGB(160, 152, 132);
	constexpr UINT32 FE_RGB_LINK   = FROMRGB(20, 40, 180);
	constexpr UINT32 FE_RGB_GOLD   = FROMRGB(190, 150, 40);
	constexpr UINT32 FE_RGB_RED    = FROMRGB(150, 30, 30);
	constexpr UINT32 FE_RGB_BLACK  = FROMRGB(12, 12, 12);

	// content card boundaries shared by every page
	constexpr INT32 FE_CT_X = 12;
	constexpr INT32 FE_CT_Y = 72;
	constexpr INT32 FE_CT_W = FE_PAGE_W - 24;
	// one fixed reading height on every tab: the drawer never resizes
	constexpr INT32 FE_CT_H = 282;

	enum FelinePage
	{
		FP_HOME, FP_BREED, FP_MYCAT, FP_CATTERY,
		FP_SIGHT, FP_SHOWS, FP_BOOK, FP_STAMPS,
		FP_COUNT
	};
	int gFelinePage = FP_HOME;

	bool gfFelineRegionsUp = false;
	bool gfFelineMidi      = false; // the toggle works; the file is missing
	bool gfFelineNaming    = false; // the foster form is waiting on a name
	std::string gFelineNameInput;
	ST::string  gFelineTicker; // one line of footer commentary

	SGPVObject* guiFelineArt = nullptr; // felinecats.sti, or the ink kit
	SGPVObject* guiFelineCt  = nullptr; // the game's own bloodcat, exhibited

	// the Society, in person: 33px dialogue portraits by profile
	struct Member { ProfileID pid; SGPVObject* face; };
	Member gFelineMembers[4] =
	{
		{ BRENDA, nullptr }, { MANNY, nullptr },
		{ HAMOUS, nullptr }, { AUNTIE, nullptr },
	};

	SGPVObject* FaceOf(ProfileID pid)
	{
		for (const Member& m : gFelineMembers)
		{
			if (m.pid == pid) return m.face;
		}
		return nullptr;
	}

	// a small framed portrait, or the paw for out-of-towners
	void AvatarChip(ProfileID pid, INT32 x, INT32 y);

	enum { FE_FRAME_ROSETTE = 10, FE_FRAME_PHOTO = 11, FE_FRAME_PAW = 12,
			FE_FRAME_BURST = 13, FE_FRAME_WOOD = 14 };

	MOUSE_REGION gFelineNavRegion[FP_COUNT];
	MOUSE_REGION gFelineClickRegion; // the card; buttons hit-test inside
	MOUSE_REGION gFelineMidiRegion;
	MOUSE_REGION gFelineRingRegion[2];

	// the active folder tab's span, so the card can open into it
	INT32 gsFelineTabAct[2] = { 0, 0 };

	// where the current page drew its buttons, for the hit test
	struct BtnAt { INT32 x, y, w; bool live; };
	BtnAt gFelineBtnAt[3];
	int   gFelineBtnCount = 0;

	// --- tiny drawing kit --------------------------------------------------
	void FillRect(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb)
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, FE_X(x), FE_Y(y),
					FE_X(x + w), FE_Y(y + h), Get16BPPColor(rgb));
	}

	int CornerInset(int row, int radius)
	{
		const double dy = radius - row - 0.5;
		return int(radius - std::sqrt(double(radius) * radius - dy * dy)
				+ 0.5);
	}

	void RoundCorners(INT32 x, INT32 y, INT32 w, INT32 h, int radius,
				UINT32 bg)
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

	void FillRounded(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb,
				int radius, UINT32 bg)
	{
		FillRect(x, y, w, h, rgb);
		RoundCorners(x, y, w, h, radius, bg);
	}

	void Frame(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb)
	{
		FillRect(x, y, w, 1, rgb);
		FillRect(x, y + h - 1, w, 1, rgb);
		FillRect(x, y, 1, h, rgb);
		FillRect(x + w - 1, y, 1, h, rgb);
	}

	void PrintAt(SGPFont font, UINT8 colour, INT32 x, INT32 y,
				const ST::string& text)
	{
		SetFontAttributes(font, colour, FONT_MCOLOR_BLACK, 0);
		MPrint(FE_X(x), FE_Y(y), text);
	}

	void PrintCentred(SGPFont font, UINT8 colour, INT32 cx, INT32 y,
				const ST::string& text)
	{
		PrintAt(font, colour, cx - StringPixLength(text, font) / 2, y, text);
	}

	INT32 Wrapped(INT32 x, INT32 y, INT32 w, UINT8 colour,
				const ST::string& text)
	{
		return DisplayWrappedString(UINT16(FE_X(x)), UINT16(FE_Y(y)),
				UINT16(w), 2, FONT10ARIAL, colour, text,
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	}

	// A frame decoded by hand: the bounding box lands exactly where
	// asked (tactical offsets do not apply on a web page), pixels stay
	// inside the clip, and the engine's shadow index 254 darkens what is
	// beneath it, the way the engine means it to.
	void BltClipped(SGPVObject* ct, UINT16 frame, INT32 px, INT32 py,
				INT32 clipX, INT32 clipY, INT32 clipW, INT32 clipH)
	{
		if (!ct) return;
		const ETRLEObject& e = ct->SubregionProperties(frame);
		const UINT16* pal = ct->Palette16();
		if (!pal) return;
		const UINT8* in = ct->PixData(e);
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
					const UINT8 v = *in++;
					const INT32 gx = px + x, gy = py + y;
					if (gx < clipX || gx >= clipX + clipW ||
					    gy < clipY || gy >= clipY + clipH)
					{
						continue;
					}
					UINT16& out = buf[UINT32(FE_Y(gy)) * pitch +
							UINT32(FE_X(gx))];
					if (v == 254)
					{
						// the engine's shadow index, rendered the
						// way the engine means it: darken what is
						// already there
						out = UINT16((out >> 1) & 0x7BEF);
					}
					else
					{
						out = pal[v];
					}
				}
			}
			++in;
		}
	}

	void DrawSheet(UINT16 frame, INT32 x, INT32 y)
	{
		if (guiFelineArt && guiFelineArt->SubregionCount() > frame)
		{
			BltVideoObject(FRAME_BUFFER, guiFelineArt, frame, FE_X(x),
					FE_Y(y));
		}
	}

	void BltClipped(SGPVObject* ct, UINT16 frame, INT32 px, INT32 py,
				INT32 clipX, INT32 clipY, INT32 clipW, INT32 clipH);
	void Blob(INT32 cx, INT32 cy, INT32 rx, INT32 ry, UINT32 rgb);

	void AvatarChip(ProfileID pid, INT32 x, INT32 y)
	{
		FillRounded(x - 1, y - 1, 35, 32, FE_RGB_RULE, 4, FE_RGB_CARD);
		SGPVObject* const face = FaceOf(pid);
		if (face && face->SubregionCount() > 0)
		{
			// whatever size the profile shipped with, stretched to
			// fill the chip: decode the frame, sample it nearest
			const ETRLEObject& e = face->SubregionProperties(0);
			const INT32 w = e.usWidth, h = e.usHeight;
			const UINT16* pal = face->Palette16();
			if (w > 0 && h > 0 && pal)
			{
				std::vector<UINT8> pix(size_t(w) * h, 0);
				const UINT8* in = face->PixData(e);
				for (INT32 sy = 0; sy < h; ++sy)
				{
					INT32 sx = 0;
					while (*in != 0)
					{
						const UINT8 code = *in++;
						const UINT8 run = code & 0x7F;
						if (code & 0x80) { sx += run; continue; }
						for (UINT8 k = 0; k < run && sx < w; ++k)
						{
							pix[size_t(sy) * w + sx++] = *in++;
						}
					}
					++in;
				}
				SGPVSurface::Lock lock(FRAME_BUFFER);
				UINT16* buf = lock.Buffer<UINT16>();
				const UINT32 pitch = lock.Pitch() / 2;
				for (INT32 dy = 0; dy < 30; ++dy)
				{
					for (INT32 dx = 0; dx < 33; ++dx)
					{
						const UINT8 v = pix[size_t(dy * h / 30) * w
								+ size_t(dx * w / 33)];
						if (v == 0) continue;
						buf[UINT32(FE_Y(y + dy)) * pitch +
								UINT32(FE_X(x + dx))] = pal[v];
					}
				}
			}
			RoundCorners(x - 1, y - 1, 35, 32, 4, FE_RGB_CARD);
			return;
		}
		// no photo on file: the bust every directory printed instead,
		// in a shade barely darker than the paper
		const UINT32 bust = FROMRGB(186, 172, 146);
		FillRect(x, y, 33, 30, FROMRGB(216, 204, 178));
		Blob(x + 16, y + 11, 6, 7, bust);
		for (int r = 0; r < 9; ++r)
		{
			const double t = (8.0 - r) / 9.0;
			const INT32 half = INT32(11.0 *
					std::sqrt(std::max(0.0, 1.0 - t * t)) + 0.5);
			FillRect(x + 16 - half, y + 21 + r, 2 * half, 1, bust);
		}
		RoundCorners(x - 1, y - 1, 35, 32, 4, FE_RGB_CARD);
	}

	void Heading(INT32 x, INT32 y, const ST::string& text)
	{
		DrawSheet(FE_FRAME_PAW, x, y - 3);
		PrintAt(FONT12ARIAL, FONT_DKBLUE, x + 26, y, text);
		FillRect(x + 26, y + 13,
				StringPixLength(text, FONT12ARIAL), 1, FE_RGB_TEAL);
	}

	void Rule(INT32 x, INT32 y, INT32 w)
	{
		// the <HR> of its day: a groove, proudly
		FillRect(x, y, w, 1, FE_RGB_RULE);
		FillRect(x, y + 1, w, 1, FROMRGB(252, 250, 244));
	}

	void FelineRedraw()
	{
		fPausedReDrawScreenFlag = TRUE;
	}

	bool Hover(const MOUSE_REGION& r)
	{
		return (r.uiFlags & MSYS_MOUSE_IN_AREA) != 0;
	}

	// --- the world, read-only ---------------------------------------------
	bool LairIsGone()
	{
		return gubFact[FACT_I16_BLOODCATS_KILLED];
	}

	bool PlayerKnowsTheLair()
	{
		return gubFact[FACT_PLAYER_KNOWS_ABOUT_BLOODCAT_LAIR];
	}

	// the succession model, arriving early: when Brenda dies the Society
	// changes hands, and the cat gets a new keeper who tries very hard
	bool BrendaGone() { return gubFact[FACT_BRENDA_DEAD]; }
	const char* Keeper() { return BrendaGone() ? "manny" : "brenda"; }

	uint16_t Today() { return uint16_t(GetWorldDay()); }

	// --- money -------------------------------------------------------------
	bool ChargeSociety(INT32 amount)
	{
		if (LaptopSaveInfo.iCurrentBalance < amount)
		{
			gFelineTicker = "the treasurer notes, kindly, that the "
					"cheque would bounce.";
			return false;
		}
		AddTransactionToPlayersBook(PAYMENT_TO_NPC,
					BrendaGone() ? MANNY : BRENDA,
					GetWorldTotalMin(), -amount);
		gFeline.iSpent += amount;
		return true;
	}

	// --- the cat, mapped in and out of the persist blob --------------------
	FelineCat::State CatState()
	{
		FelineCat::State s;
		s.hunger       = gFeline.ubHunger;
		s.supplies     = gFeline.ubSupplies;
		s.lastFedDay   = gFeline.usLastFedDay;
		s.lastVisitDay = gFeline.usLastVisitDay;
		s.away         = (gFeline.ubFlags & FELINE_FLAG_AWAY) != 0;
		return s;
	}

	void StoreCat(const FelineCat::State& s)
	{
		gFeline.ubHunger      = s.hunger;
		gFeline.ubSupplies    = s.supplies;
		gFeline.usLastFedDay  = s.lastFedDay;
		gFeline.usLastVisitDay = s.lastVisitDay;
		if (s.away) gFeline.ubFlags |= FELINE_FLAG_AWAY;
		else        gFeline.ubFlags &= UINT8(~FELINE_FLAG_AWAY);
	}

	bool Fostered() { return (gFeline.ubFlags & FELINE_FLAG_FOSTERED) != 0; }

	// lazy rollover on entry, ChessRollOverDay fashion: no strategic
	// event owns this animal
	void SyncCat()
	{
		if (!Fostered()) return;
		FelineCat::State s = CatState();
		FelineCat::RollDay(s, Today());
		StoreCat(s);
	}

	FelineCat::Mood CatMood()
	{
		return FelineCat::MoodOf(CatState(), Today(), false);
	}

	// --- the cat, drawn ----------------------------------------------------
	void Blob(INT32 cx, INT32 cy, INT32 rx, INT32 ry, UINT32 rgb)
	{
		for (INT32 dy = -ry; dy <= ry; ++dy)
		{
			const double t = double(dy) / ry;
			const INT32 half = INT32(rx * std::sqrt(std::max(0.0,
					1.0 - t * t)) + 0.5);
			if (half > 0) FillRect(cx - half, cy + dy, 2 * half, 1, rgb);
		}
	}

	void CatEars(INT32 cx, INT32 cy, UINT32 rgb)
	{
		for (int i = 0; i < 5; ++i)
		{
			FillRect(cx - 9 + i / 2, cy - i, 3 - i / 2, 1, rgb);
			FillRect(cx + 6 + (4 - i) / 2, cy - i, 3 - i / 2, 1, rgb);
		}
	}

	void CatFace(INT32 cx, INT32 cy, UINT32 paper, bool eyesOpen)
	{
		if (eyesOpen)
		{
			FillRect(cx - 5, cy - 1, 2, 2, paper);
			FillRect(cx + 3, cy - 1, 2, 2, paper);
		}
		else
		{
			FillRect(cx - 6, cy, 3, 1, paper);
			FillRect(cx + 3, cy, 3, 1, paper);
		}
		// whiskers, the load-bearing feature of any cat drawing
		FillRect(cx - 14, cy + 2, 7, 1, FE_RGB_RULE);
		FillRect(cx + 7,  cy + 2, 7, 1, FE_RGB_RULE);
		FillRect(cx - 13, cy + 4, 6, 1, FE_RGB_RULE);
		FillRect(cx + 7,  cy + 4, 6, 1, FE_RGB_RULE);
	}

	// one housecat, ten attitudes: the baked clip-art sheet when it is
	// there, the in-code ink sketch when it is not
	void DrawCatInk(INT32 x, INT32 y, FelineCat::Pose pose);

	void DrawCat(INT32 x, INT32 y, FelineCat::Pose pose)
	{
		if (guiFelineArt && guiFelineArt->SubregionCount() >= 10)
		{
			BltVideoObject(FRAME_BUFFER, guiFelineArt, UINT16(pose),
					FE_X(x - 10), FE_Y(y - 4));
			return;
		}
		DrawCatInk(x, y, pose);
	}

	void DrawCatInk(INT32 x, INT32 y, FelineCat::Pose pose)
	{
		const UINT32 ink = FE_RGB_INK;
		const UINT32 paper = FE_RGB_CARD;
		switch (pose)
		{
			case FelineCat::POSE_LOAF:
				Blob(x + 34, y + 34, 26, 12, ink);
				Blob(x + 12, y + 26, 11, 10, ink);
				CatEars(x + 12, y + 17, ink);
				CatFace(x + 12, y + 25, paper, true);
				break;
			case FelineCat::POSE_SIT:
				Blob(x + 34, y + 32, 15, 14, ink);
				Blob(x + 32, y + 12, 10, 9, ink);
				CatEars(x + 32, y + 4, ink);
				CatFace(x + 32, y + 11, paper, true);
				FillRect(x + 48, y + 30, 3, 14, ink); // the prim tail
				break;
			case FelineCat::POSE_SLEEP:
				Blob(x + 32, y + 36, 24, 10, ink);
				Blob(x + 16, y + 34, 10, 8, ink);
				CatEars(x + 16, y + 27, ink);
				CatFace(x + 16, y + 33, paper, false);
				break;
			case FelineCat::POSE_STRETCH:
				Blob(x + 38, y + 30, 22, 8, ink);
				Blob(x + 12, y + 38, 10, 7, ink);
				CatEars(x + 12, y + 32, ink);
				CatFace(x + 12, y + 37, paper, true);
				FillRect(x + 58, y + 16, 3, 16, ink);
				break;
			case FelineCat::POSE_BAT:
				Blob(x + 36, y + 32, 15, 13, ink);
				Blob(x + 30, y + 13, 10, 9, ink);
				CatEars(x + 30, y + 5, ink);
				CatFace(x + 30, y + 12, paper, true);
				FillRect(x + 12, y + 18, 12, 3, ink); // the raised paw
				FillRect(x + 8, y + 12, 4, 4, FE_RGB_RED); // the ball
				break;
			case FelineCat::POSE_WALK:
				Blob(x + 32, y + 28, 20, 9, ink);
				Blob(x + 54, y + 22, 9, 8, ink);
				CatEars(x + 54, y + 15, ink);
				CatFace(x + 54, y + 21, paper, true);
				FillRect(x + 16, y + 34, 3, 10, ink);
				FillRect(x + 28, y + 35, 3, 9, ink);
				FillRect(x + 40, y + 34, 3, 10, ink);
				FillRect(x + 8, y + 14, 3, 14, ink); // tail up: content
				break;
			case FelineCat::POSE_GROOM:
				Blob(x + 34, y + 32, 16, 13, ink);
				Blob(x + 30, y + 14, 10, 9, ink);
				CatEars(x + 30, y + 6, ink);
				FillRect(x + 20, y + 26, 8, 3, ink); // the leg, aloft
				break;
			case FelineCat::POSE_CROUCH:
				Blob(x + 32, y + 38, 24, 7, ink);
				Blob(x + 12, y + 36, 9, 7, ink);
				CatEars(x + 12, y + 30, ink);
				CatFace(x + 12, y + 35, paper, true);
				break;
			case FelineCat::POSE_TAIL:
				Blob(x + 30, y + 32, 15, 13, ink);
				Blob(x + 28, y + 13, 10, 9, ink);
				CatEars(x + 28, y + 5, ink);
				CatFace(x + 28, y + 12, paper, true);
				// the question-mark tail
				FillRect(x + 46, y + 18, 3, 18, ink);
				FillRect(x + 46, y + 14, 8, 3, ink);
				FillRect(x + 52, y + 8, 3, 8, ink);
				break;
			case FelineCat::POSE_STARE:
			default:
				Blob(x + 32, y + 32, 15, 13, ink);
				Blob(x + 32, y + 12, 11, 10, ink);
				CatEars(x + 32, y + 3, ink);
				CatFace(x + 32, y + 11, paper, true);
				break;
		}
	}

	// --- chrome -------------------------------------------------------------
	const char* const FE_NAV[FP_COUNT] =
	{
		"HOME", "BREED", "MY CAT", "CATTERY",
		"SIGHTINGS", "SHOWS", "GUESTBOOK", "STAMPS",
	};

	void RenderChrome()
	{
		// the club wall: baked wood panelling, or parchment if the
		// sheet is missing
		if (guiFelineArt &&
		    guiFelineArt->SubregionCount() > FE_FRAME_WOOD)
		{
			BltVideoObject(FRAME_BUFFER, guiFelineArt, FE_FRAME_WOOD,
					FE_X(0), FE_Y(0));
		}
		else
		{
			FillRect(0, 0, FE_PAGE_W + 2, FE_PAGE_H + 10, FE_RGB_PAPER);
		}


		// the masthead: a painted sign hung shy of the frame
		const UINT32 woodBg2 = FROMRGB(98, 68, 44);
		FillRounded(12, 12, FE_PAGE_W - 24, 38, FE_RGB_TEAL, 6, woodBg2);
		FillRect(13, 46, FE_PAGE_W - 26, 2, FE_RGB_TEAL_D);
		const bool memorial = LairIsGone();
		if (memorial)
		{
			FillRounded(10, 10, FE_PAGE_W - 20, 42, FE_RGB_BLACK, 7,
					woodBg2);
			FillRounded(12, 12, FE_PAGE_W - 24, 38, FE_RGB_TEAL, 6,
					FE_RGB_BLACK);
		}
		PrintCentred(FONT14ARIAL, FONT_MCOLOR_WHITE, FE_PAGE_W / 2, 16,
				"THE ARULCO FELINE SOCIETY");
		DrawSheet(FE_FRAME_PAW, 30, 20);
		DrawSheet(FE_FRAME_PAW, FE_PAGE_W - 52, 20);
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_LTYELLOW, FE_PAGE_W / 2, 32,
				memorial
					? "Founded 1994. In remembrance."
					: "Founded 1994. Member, the FELINE WEBRING.");

		// the nav: a drawer of folder tabs rising from the reading
		// card. The open file is cream and continuous with the card;
		// the closed ones sit back in aged manila, almost wood.
		INT32 x = FE_CT_X;
		const UINT32 woodBg = FROMRGB(98, 68, 44);
		for (int i = 0; i < FP_COUNT; ++i)
		{
			const bool on = i == gFelinePage;
			const bool hov = !on && Hover(gFelineNavRegion[i]);
			const INT32 tw =
				StringPixLength(FE_NAV[i], FONT10ARIALBOLD) + 10;
			const UINT32 face = on ? FE_RGB_CARD
				: hov ? FROMRGB(176, 142, 98)
				      : FROMRGB(148, 114, 76);
			FillRounded(x, 50, tw, 24, face, 5, woodBg);
			if (!on)
			{
				// a shadowed lip keeps the closed files behind
				FillRect(x + 1, 50, tw - 2, 1,
						FROMRGB(190, 158, 114));
			}
			else
			{
				Frame(x, 50, tw, 22, FE_RGB_RULE);
				RoundCorners(x, 50, tw, 24, 5, woodBg);
				gsFelineTabAct[0] = x;
				gsFelineTabAct[1] = tw;
			}
			PrintCentred(FONT10ARIALBOLD,
					on ? FONT_NEARBLACK
					: hov ? FONT_MCOLOR_WHITE : FONT_NEARBLACK,
					x + tw / 2, 57, FE_NAV[i]);
			x += tw + 2;
		}

		// the footer: one narrow brass-plate bar, sunk toward the wood
		const INT32 fy = FE_PAGE_H - 22;
		FillRounded(36, fy - 6, FE_PAGE_W - 72, 22,
				FROMRGB(170, 140, 100), 6, FROMRGB(98, 68, 44));
		FillRect(37, fy - 6, FE_PAGE_W - 74, 1, FROMRGB(198, 168, 124));
		const int hits = 1400 + int(GetWorldDay()) * 3
				+ int(gFeline.iSpent / 2);
		PrintAt(FONT10ARIAL, FONT_NEARBLACK, 48, fy,
				ST::format("you are visitor no. {}", hits));
		{
			const ST::string ring = "< prev  [FELINE WEBRING]  next >";
			const INT32 w = StringPixLength(ring, FONT10ARIAL);
			PrintCentred(FONT10ARIAL,
					Hover(gFelineRingRegion[0]) ||
					Hover(gFelineRingRegion[1])
						? FONT_MCOLOR_WHITE : FONT_DKBLUE,
					FE_PAGE_W / 2, fy, ring);
			FillRect(FE_PAGE_W / 2 - w / 2, fy + 10, w, 1,
					FROMRGB(74, 52, 120));
		}
		PrintAt(FONT10ARIAL,
				Hover(gFelineMidiRegion) ? FONT_MCOLOR_WHITE
							 : FONT_NEARBLACK,
				FE_PAGE_W - 110, fy,
				gfFelineMidi ? "[midi: ON]" : "[midi: OFF]");
		if (!gFelineTicker.empty())
		{
			PrintCentred(FONT10ARIAL, FONT_MCOLOR_DKRED, FE_PAGE_W / 2,
					fy - 18, gFelineTicker);
		}
	}

	// a period button: grey bevel, all business. Drawing it registers
	// the hit box; the shared card region does the click arithmetic.
	void Button(int slot, INT32 x, INT32 y, INT32 w, const ST::string& label,
				bool live)
	{
		if (slot >= 0 && slot < 3)
		{
			gFelineBtnAt[slot] = BtnAt{ x, y, w, live };
			gFelineBtnCount = std::max(gFelineBtnCount, slot + 1);
		}
		const bool inCard = Hover(gFelineClickRegion);
		const INT32 mx = FE_CT_X + INT32(gFelineClickRegion.RelativeXPos);
		const INT32 my = FE_CT_Y + INT32(gFelineClickRegion.RelativeYPos);
		const bool hov = live && inCard &&
				mx >= x && mx <= x + w && my >= y && my <= y + 18;
		FillRect(x + 2, y + 2, w, 18, FROMRGB(120, 112, 96));
		FillRect(x, y, w, 18, hov ? FROMRGB(226, 220, 204)
					   : live ? FROMRGB(208, 200, 180)
						  : FROMRGB(190, 184, 170));
		Frame(x, y, w, 18, FE_RGB_INK);
		PrintCentred(FONT10ARIALBOLD,
				live ? FONT_NEARBLACK : FONT_METALGRAY,
				x + w / 2, y + 5, label);
	}

	void Card(INT32 h)
	{
		FillRect(FE_CT_X + 3, FE_CT_Y + 3, FE_CT_W, h,
				FROMRGB(212, 202, 176));
		FillRect(FE_CT_X, FE_CT_Y, FE_CT_W, h, FE_RGB_CARD);
		Frame(FE_CT_X, FE_CT_Y, FE_CT_W, h, FE_RGB_RULE);
		// open the drawer: erase the border where the active tab meets
		// the card, so folder and file read as one piece of paper
		if (gsFelineTabAct[1] > 2)
		{
			FillRect(gsFelineTabAct[0] + 1, FE_CT_Y,
					gsFelineTabAct[1] - 2, 1, FE_RGB_CARD);
			FillRect(gsFelineTabAct[0] + 1, FE_CT_Y - 2,
					gsFelineTabAct[1] - 2, 2, FE_RGB_CARD);
		}
		if (LairIsGone())
		{
			Frame(FE_CT_X - 2, FE_CT_Y - 2, FE_CT_W + 4, h + 4,
					FE_RGB_BLACK);
		}
	}

	// --- pages --------------------------------------------------------------
	void RenderHome()
	{
		Card(FE_CT_H);
		INT32 y = FE_CT_Y + 8;
		Heading(FE_CT_X + 10, y, "WELCOME");
		y += 20;
		y += Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_NEARBLACK,
				"The Arulco Feline Society is nine friends united by one "
				"belief: that this country, whatever else it lacks, "
				"deserves cats. We have a constitution (4 pages), a "
				"treasurer (Brenda), and a newsletter (quarterly, paper "
				"allowing).") + 6;
		Rule(FE_CT_X + 10, y, FE_CT_W - 20);
		y += 10;

		// the parish-hall pinboard: three boxes, side by side
		const INT32 bw = 148;
		const INT32 bh = FE_CT_Y + FE_CT_H - y - 12;
		const INT32 bx[3] =
			{ FE_CT_X + 10, FE_CT_X + 164, FE_CT_X + 318 };
		static const UINT32 tints[3] =
		{
			FROMRGB(246, 234, 226), // notice: faded rose
			FROMRGB(240, 234, 216), // foster: faded straw
			FROMRGB(232, 238, 228), // the club: faded mint
		};
		for (int i = 0; i < 3; ++i)
		{
			FillRounded(bx[i], y, bw, bh, tints[i], 6, FE_RGB_CARD);
		}

		// box one: the notice, or the memorial
		const bool memorial = LairIsGone();
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_DKRED, bx[0] + 8, y + 8,
				memorial ? "IN MEMORIAM" : "NOTICE");
		Wrapped(bx[0] + 8, y + 24, bw - 16, FONT_NEARBLACK, memorial
				? "The last managed population of the Highland is no "
				  "more. The breed standard has moved to the past "
				  "tense. Some members have written things in the "
				  "guestbook. We have left them up."
				: "The 1999 CHAMPIONSHIP SHOW is POSTPONED, again, on "
				  "account of the situation. The rosettes are safe and "
				  "dry at Brenda's. The 1998 results remain final, "
				  "whatever Manny says.");
		DrawSheet(FE_FRAME_ROSETTE, bx[0] + bw - 52, y + bh - 66);

		// box two: the foster programme, the reason to sign up
		DrawSheet(FE_FRAME_BURST, bx[1] + bw - 42, y + 4);
		PrintCentred(FONT10ARIALBOLD, FONT_MCOLOR_WHITE,
				bx[1] + bw - 24, y + 16, "NEW");
		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, bx[1] + 8, y + 8,
				"FOSTER A CAT");
		Wrapped(bx[1] + 8, y + 24, bw - 16, FONT_METALGRAY,
				"You cannot keep a cat where you are - we understand, "
				"better than most. A cat in the back room of Brenda's "
				"store can be YOURS: named by you, fed on your "
				"account, photographed monthly.");
		PrintAt(FONT10ARIALBOLD, FONT_DKBLUE, bx[1] + 8, y + bh - 22,
				"see MY CAT >");
		FillRect(bx[1] + 8, y + bh - 12,
				StringPixLength("see MY CAT >", FONT10ARIALBOLD), 1,
				FE_RGB_LINK);

		// box three: the club, in label-and-value plain talk
		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, bx[2] + 8, y + 8,
				"THE CLUB");
		{
			struct Line { const char* label; ST::string value; };
			const Line lines[4] =
			{
				{ "members",  ST::format("{}", BrendaGone() ? 8 : 9) },
				{ "cats",     ST::format("{}", Fostered() ? 4 : 3) },
				{ "climate",  "unsuitable" },
				{ "keeper",   BrendaGone() ? "manny (acting)"
							    : "brenda" },
			};
			INT32 ly = y + 26;
			for (const Line& l : lines)
			{
				PrintAt(FONT10ARIAL, FONT_METALGRAY, bx[2] + 8, ly,
						l.label);
				PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK,
						bx[2] + bw - 8 - StringPixLength(l.value,
								FONT10ARIALBOLD), ly, l.value);
				ly += 13;
			}
		}
		// the club cat holds the floor of its box
		DrawCat(bx[2] + (bw - 76) / 2, y + bh - 58,
				FelineCat::POSE_SIT);
	}

	void RenderBreed()
	{
		Card(FE_CT_H);
		const bool past = LairIsGone();
		INT32 y = FE_CT_Y + 8;
		Heading(FE_CT_X + 10, y,
				past ? "THE ARULCAN HIGHLAND (1994-1999)"
				     : "THE ARULCAN HIGHLAND - BREED STANDARD");
		y += 20;

		// the photograph: obviously a bloodcat, lovingly framed
		Frame(FE_CT_X + 8, y - 2, 114, 88, FE_RGB_GOLD);
		Frame(FE_CT_X + 9, y - 1, 112, 86, FE_RGB_GOLD);
		if (guiFelineArt && guiFelineArt->SubregionCount() > FE_FRAME_PHOTO)
		{
			DrawSheet(FE_FRAME_PHOTO, FE_CT_X + 10, y);
		}
		else
		{
			FillRect(FE_CT_X + 10, y, 110, 84, FROMRGB(60, 52, 40));
		}
		if (guiFelineCt && guiFelineCt->SubregionCount() >= 64)
		{
			// the exhibit, from the game's own animation data - one
			// still frame, because this is a photograph
			const UINT16 frame = 16;
			const ETRLEObject& e =
				guiFelineCt->SubregionProperties(frame);
			const INT32 bx = FE_CT_X + 10 + (110 - e.usWidth) / 2 - 2;
			const INT32 by = y + 72 - INT32(e.usHeight);
			BltClipped(guiFelineCt, frame, bx, by, FE_CT_X + 11, y + 1,
					108, 82);
		}
		else
		{
			Blob(FE_CT_X + 60, y + 52, 40, 18, FE_RGB_BLACK);
			Blob(FE_CT_X + 96, y + 36, 13, 11, FE_RGB_BLACK);
		}
		PrintCentred(FONT10ARIAL, FONT_MCOLOR_WHITE, FE_CT_X + 65, y + 74,
				"CH. DUCHESS KALINKA");

		const INT32 tx = FE_CT_X + 136;
		INT32 ty = y;
		ty += Wrapped(tx, ty, FE_CT_W - 146, FONT_NEARBLACK, past
				? "The Highland was a spirited breed, devoted to one "
				  "handler, and famously particular about visitors. It "
				  "required forty pounds of meat weekly and a great "
				  "deal of understanding."
				: "A spirited breed. Devoted to one handler. Particular "
				  "about visitors. Requires forty pounds of meat weekly "
				  "and a great deal of understanding. NOT suitable for "
				  "the first-time owner.") + 6;
		ty += Wrapped(tx, ty, FE_CT_W - 146, FONT_METALGRAY, past
				? "The N5 programme, the last of its kind, has closed."
				: PlayerKnowsTheLair()
				? "The Society is aware of remarks made about the I16 "
				  "colony. We do not dignify them. The N5 stock remains "
				  "the last managed breeding programme in the region, "
				  "under traditional husbandry."
				: "The N5 stock represents the last managed breeding "
				  "programme in the region. Its custodians practise "
				  "traditional husbandry.") + 4;

		y += 96;
		Rule(FE_CT_X + 10, y, FE_CT_W - 20);
		y += 8;
		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, FE_CT_X + 10, y,
				"POINTS OF THE BREED (100)");
		y += 14;
		static const char* const ptLabel[5] =
		{
			"head & expression", "coat", "temperament", "teeth",
			"condition",
		};
		static const char* const ptValue[5] =
			{ "20", "20", "10", "30", "20" };
		static const char* const ptNote[5] =
		{
			"(alert. very alert.)", "(dense, storm-grey)",
			"(see note*)", "(exceptional)", "(always)",
		};
		for (int i = 0; i < 5; ++i)
		{
			PrintAt(FONT10ARIAL, FONT_METALGRAY, FE_CT_X + 18, y,
					ptLabel[i]);
			PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK,
					FE_CT_X + 158 - StringPixLength(ptValue[i],
							FONT10ARIALBOLD), y, ptValue[i]);
			PrintAt(FONT10ARIAL, FONT_METALGRAY, FE_CT_X + 176, y,
					ptNote[i]);
			y += 12;
		}
		y += 4;
		Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_METALGRAY,
				"* the judge shall not turn their back on the exhibit. "
				"This is a formality.");
	}

	const char* MoodLine(FelineCat::Mood m)
	{
		switch (m)
		{
			case FelineCat::MOOD_PLAYFUL:
				return "in excellent spirits. Brenda says it got the "
					"string again.";
			case FelineCat::MOOD_CONTENT:
				return "content. The back room is warm and the war is "
					"somewhere else.";
			case FelineCat::MOOD_LONELY:
				return "a little withdrawn. Brenda says it sits by the "
					"modem when the page loads.";
			case FelineCat::MOOD_HUNGRY:
				return "hungry. The shelf is bare and it has opinions "
					"about that.";
			case FelineCat::MOOD_THIN:
				return "thin. Brenda's update is short this month. "
					"Please send tins.";
			case FelineCat::MOOD_HIDING:
				return "under the counter. There was noise to the "
					"north. Ears flat.";
			case FelineCat::MOOD_AWAY:
			default:
				return "gone to stay with a member in the country. The "
					"letter does not say which member.";
		}
	}

	void RenderMyCat()
	{
		Card(FE_CT_H);
		INT32 y = FE_CT_Y + 8;
		if (!Fostered())
		{
			Heading(FE_CT_X + 10, y, "THE FOSTER PROGRAMME");
			y += 20;
			y += Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_NEARBLACK,
					"For $15, a cat currently boarding in the back room "
					"of Brenda's store in Cambria becomes yours in every "
					"way that fits down a phone line. You will name it. "
					"You will feed it, via the CATTERY page. Brenda will "
					"write you about it, monthly or when something "
					"happens, whichever is worse.") + 8;
			y += Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_METALGRAY,
					"The cat stays with Brenda. This is not negotiable "
					"and you would not want it to be. You know what your "
					"life looks like.") + 10;
			if (gfFelineNaming)
			{
				PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, FE_CT_X + 10, y,
						"NAME (type it, ENTER files it):");
				y += 16;
				FillRect(FE_CT_X + 10, y, 200, 18, FROMRGB(255, 255, 255));
				Frame(FE_CT_X + 10, y, 200, 18, FE_RGB_INK);
				ST::string shown = ST::string(gFelineNameInput);
				if ((GetJA2Clock() / 400) % 2 == 0) shown += "_";
				PrintAt(FONT10ARIAL, FONT_NEARBLACK, FE_CT_X + 16, y + 5,
						shown);
			}
			else
			{
				Button(0, FE_CT_X + 10, y, 150, "FOSTER A CAT ($15)",
						true);
			}
			return;
		}

		SyncCat();
		const FelineCat::Mood mood = CatMood();
		const FelineCat::Pose pose = FelineCat::PoseFor(mood,
				uint32_t(GetWorldDay()) + uint32_t(GetJA2Clock() / 9000));
		PrintAt(FONT12ARIAL, FONT_NEARBLACK, FE_CT_X + 10, y,
				ST::format("{}, OF CAMBRIA", ST::string(gFeline.szName)
						.to_upper()));
		y += 20;

		// the pen: one cat, framed like the treasure it is
		FillRounded(FE_CT_X + 13, y + 3, 96, 66, FROMRGB(206, 196, 170),
				5, FE_RGB_CARD);
		FillRounded(FE_CT_X + 10, y, 96, 66, FE_RGB_RULE, 5, FE_RGB_CARD);
		FillRounded(FE_CT_X + 11, y + 1, 94, 64, FROMRGB(246, 234, 226),
				5, FE_RGB_RULE);
		if (mood == FelineCat::MOOD_AWAY)
		{
			PrintCentred(FONT10ARIAL, FONT_METALGRAY, FE_CT_X + 58,
					y + 26, "(no photo)");
		}
		else
		{
			DrawCat(FE_CT_X + 22, y + 10, pose);
		}

		const INT32 tx = FE_CT_X + 122;
		INT32 ty = y;
		const FelineCat::State s = CatState();
		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, tx, ty, "CONDITION:");
		// the meter reads fullness; nobody wants a hunger bar on a pet
		const INT32 mw = 120;
		FillRect(tx + 78, ty + 1, mw, 8, FROMRGB(210, 202, 184));
		FillRect(tx + 78, ty + 1, mw * (100 - s.hunger) / 100, 8,
				s.hunger >= 80 ? FE_RGB_RED
				: s.hunger >= 45 ? FE_RGB_GOLD : FROMRGB(60, 130, 60));
		Frame(tx + 78, ty + 1, mw, 8, FE_RGB_INK);
		ty += 14;
		PrintAt(FONT10ARIAL, FONT_NEARBLACK, tx, ty,
				ST::format("tins on the shelf: {}", int(s.supplies)));
		ty += 14;
		AvatarChip(BrendaGone() ? MANNY : BRENDA, tx, ty);
		ty += Wrapped(tx + 42, ty, FE_CT_W - 174, FONT_METALGRAY,
				ST::format("{} writes: {} is {}", Keeper(),
						ST::string(gFeline.szName), MoodLine(mood))) + 8;
		ty = std::max<INT32>(ty, y + 36);

		y += 76;
		if (mood == FelineCat::MOOD_AWAY)
		{
			y += Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_MCOLOR_DKRED,
					"A letter from Brenda, undated: \"the cat has gone "
					"to stay with a member in the country. it is for the "
					"best. do not ask which member. if you wish to "
					"arrange its return, the Society can make enquiries. "
					"there is a cost, and it is not only money, but also "
					"it is $30.\"") + 8;
			Button(0, FE_CT_X + 10, y, 190, "ARRANGE THE RETURN ($30)",
					true);
		}
		else
		{
			Button(0, FE_CT_X + 10, y, 130, "FEED (1 tin)",
					s.supplies > 0);
			Button(1, FE_CT_X + 150, y, 170, "ORDER SUPPLIES (CATTERY)",
					true);
			y += 26;
			PrintAt(FONT10ARIAL, FONT_METALGRAY, FE_CT_X + 10, y + 4,
					"Brenda feeds from the shelf every second day. The "
					"shelf does not refill itself.");
		}
	}

	void RenderCattery()
	{
		Card(FE_CT_H);
		INT32 y = FE_CT_Y + 8;
		Heading(FE_CT_X + 10, y, "THE CATTERY - SUPPLIES & SUNDRIES");
		y += 20;
		y += Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_METALGRAY,
				"Paid to the Society's account, kept by Brenda, "
				"delivered by Hamous when his route allows. Prices "
				"include the delivery, the danger, and a small amount "
				"of Hamous.") + 8;
		Rule(FE_CT_X + 10, y, FE_CT_W - 20);
		y += 10;

		// each ware in its own quiet pastel tray
		struct Ware { const char* name; const char* blurb; INT32 rows; };
		static const Ware wares[3] =
		{
			{ "TINNED MEAT, 6-PACK ..... $9",
			  "the good kind. the label has a fish on it that does "
			  "not occur in nature.", 1 },
			{ "TINNED MEAT, SINGLE ..... $2",
			  "for the member of modest means, or the cat of modest "
			  "appetite. neither exists.", 1 },
			{ "GENUINE HIGHLAND WHISKER, FRAMED ..... $25",
			  "provenance: Manny found it. certificate of "
			  "authenticity signed by Manny. proceeds to the "
			  "newsletter.", 2 },
		};
		static const UINT32 trays[3] =
		{
			FROMRGB(244, 232, 222), // rose, barely
			FROMRGB(232, 238, 228), // mint, barely
			FROMRGB(240, 234, 216), // straw, barely
		};
		for (int i = 0; i < 3; ++i)
		{
			const INT32 bh = 30 + wares[i].rows * 12;
			FillRounded(FE_CT_X + 10, y, FE_CT_W - 20, bh, trays[i],
					5, FE_RGB_CARD);
			PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, FE_CT_X + 20,
					y + 7, wares[i].name);
			Wrapped(FE_CT_X + 28, y + 21, FE_CT_W - 140,
					FONT_METALGRAY, wares[i].blurb);
			Button(i, FE_CT_X + FE_CT_W - 100, y + 6, 76, "ORDER",
					true);
			y += bh + 6;
		}
		y += 2;
		Rule(FE_CT_X + 10, y, FE_CT_W - 20);
		y += 8;
		Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_METALGRAY,
				ST::format("your lifetime contribution to the Society: "
						"${}. the treasurer thanks you. the treasurer "
						"is also the secretary, and the keeper.",
						int(gFeline.iSpent)));
	}

	void RenderSightings()
	{
		Card(FE_CT_H);
		INT32 y = FE_CT_Y + 8;
		Heading(FE_CT_X + 10, y, "FIELD SIGHTINGS - MEMBERS' REPORTS");
		y += 20;
		y += Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_METALGRAY,
				"Compiled from members' letters and Hamous's delivery "
				"notes. The Society reminds members that a sighting is "
				"a privilege and, increasingly, a warning.") + 8;
		Rule(FE_CT_X + 10, y, FE_CT_W - 20);
		y += 8;

		// confirmed: sectors where the war has actually rolled the dice
		int shown = 0;
		for (int sec = 0; sec < 256 && shown < 5; ++sec)
		{
			const INT8 cats = SectorInfo[sec].bBloodCats;
			if (cats <= 0) continue;
			const char row = char('A' + sec / 16);
			const int  col = sec % 16 + 1;
			PrintAt(FONT10ARIAL, FONT_NEARBLACK, FE_CT_X + 14, y,
					ST::format("sector {}{}: CONFIRMED. {} head. "
							"handler not located.",
							row, col, int(cats)));
			y += 13;
			++shown;
		}
		if (shown == 0)
		{
			PrintAt(FONT10ARIAL, FONT_METALGRAY, FE_CT_X + 14, y,
					"no confirmed reports this season. the Society "
					"chooses to find this encouraging.");
			y += 13;
		}
		y += 4;
		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, FE_CT_X + 10, y,
				"UNCONFIRMED, AND STAYING THAT WAY:");
		y += 14;
		static const char* const rumours[4] =
		{
			"\"an enormous grey cat\" at the Drassen airfield. it was "
			"a mail sack. (Hamous)",
			"purring under the Cambria mine. the mine purrs. it is a "
			"mine. (Auntie, dismissive)",
			"a member in Balime feeds \"something\" at dusk. the "
			"Society has asked her to stop.",
			"tracks by the Chitzena ruins, size of a dinner plate. the "
			"Society has NOT asked to see them.",
		};
		for (int i = 0; i < 4; ++i)
		{
			y += Wrapped(FE_CT_X + 14, y, FE_CT_W - 28, FONT_METALGRAY,
					rumours[i]) + 4;
		}
	}

	void RenderShows()
	{
		Card(FE_CT_H);
		INT32 y = FE_CT_Y + 8;
		Heading(FE_CT_X + 10, y, "CHAMPIONSHIP SHOW RESULTS");
		DrawSheet(FE_FRAME_ROSETTE, FE_CT_X + FE_CT_W - 60, y);
		y += 20;

		FillRect(FE_CT_X + 10, y, FE_CT_W - 20, 16, FROMRGB(222, 214, 192));
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_DKRED, FE_CT_X + 16, y + 4,
				"1999: POSTPONED. the hall is unavailable. the road to "
				"the hall is unavailable.");
		y += 24;

		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, FE_CT_X + 10, y,
				"1998 - BRENDA'S STOCKROOM, CAMBRIA (7 entries)");
		y += 14;
		static const char* const r98[3] =
		{
			"BEST IN SHOW: Ch. Duchess Kalinka of Cambria (Brenda). "
			"the judge wore the long gloves.",
			"RESERVE: Senor Bigotes (Manny). Manny wept. the minutes "
			"record it as \"weather\".",
			"BEST CONDITION: General Fluff (Hamous). judged from the "
			"truck, through glass, as agreed.",
		};
		for (int i = 0; i < 3; ++i)
		{
			y += Wrapped(FE_CT_X + 16, y, FE_CT_W - 32, FONT_NEARBLACK,
					r98[i]) + 4;
		}
		y += 4;
		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, FE_CT_X + 10, y,
				"1997 - THE OLD HALL, OMERTA (9 entries)");
		y += 14;
		static const char* const r97[3] =
		{
			"BEST IN SHOW: Reina (D. Garcia+). the rosette is kept at "
			"Brenda's, with his chair.",
			"RESERVE: Duchess Kalinka, then of Omerta (Brenda). "
			"relocated 1998, like everyone.",
			"NOVICE: Pepita (E. Morales+). entry fee returned to the "
			"family, 1998, with the collar.",
		};
		for (int i = 0; i < 3; ++i)
		{
			y += Wrapped(FE_CT_X + 16, y, FE_CT_W - 32, FONT_NEARBLACK,
					r97[i]) + 4;
		}
		y += 2;
		Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_METALGRAY,
				"+ the Society no longer updates these names. the "
				"results stand. results are the one thing that can.");
	}

	void RenderGuestbook()
	{
		Card(FE_CT_H);
		INT32 y = FE_CT_Y + 8;
		Heading(FE_CT_X + 10, y,
				"GUESTBOOK - sign at your own risk. Auntie reads it.");
		y += 22;
		struct Entry { const char* who; INT16 pid; const char* said; };
		static const Entry book[6] =
		{
			{ "manny", MANNY,
			  "senor bigotes ate from my hand today. from my HAND. i "
			  "have been at the bar eleven years and nobody noticed my "
			  "hand." },
			{ "auntie", AUNTIE,
			  "a cat is a wolf that learned bookkeeping. the ones at "
			  "the lair took my Bessie. you will not see me at your "
			  "\"show\"." },
			{ "hamous", HAMOUS,
			  "delivered to cambria ok. the highland at G6 watched my "
			  "truck for one kilometer. i drove with dignity and also "
			  "very fast." },
			{ "brenda", BRENDA,
			  "reminder: the stockroom is a CATTERY on show days. "
			  "customers who sneeze are customers. Auntie, we see your "
			  "entries. we keep them. it is a guestbook." },
			{ "shady_lady", -1,
			  "came for the mahjong tips (wrong site). stayed for the "
			  "duchess. she has more titles than the general." },
			{ "auntie", AUNTIE,
			  "you keep deleting nothing, brenda, so hear this: when "
			  "the last one is gone i will sign this book one final "
			  "time, and it will be POLITE." },
		};
		for (int i = 0; i < 6; ++i)
		{
			AvatarChip(book[i].pid < 0 ? ProfileID(255)
					: ProfileID(book[i].pid), FE_CT_X + 10, y);
			PrintAt(FONT10ARIALBOLD,
					book[i].who[0] == 'a' ? FONT_MCOLOR_DKRED
							      : FONT_NEARBLACK,
					FE_CT_X + 52, y + 1,
					ST::format("{} wrote:", book[i].who));
			const INT32 th = Wrapped(FE_CT_X + 52, y + 13,
					FE_CT_W - 70, FONT_METALGRAY, book[i].said);
			y += std::max<INT32>(34, th + 15) + 3;
		}
	}

	void RenderStamps()
	{
		Card(FE_CT_H);
		INT32 y = FE_CT_Y + 8;
		Heading(FE_CT_X + 10, y, "KITTY STAMPS!! - free for member homepages");
		y += 20;
		y += Wrapped(FE_CT_X + 10, y, FE_CT_W - 20, FONT_METALGRAY,
				"ten poses, drawn by Brenda's niece. right-click and "
				"SAVE AS. link back to the Society. do not hotlink, "
				"our provider counts the bytes.") + 6;
		// the reference sheet itself: all ten poses, each on its own
		// tinted stamp with the perforated look of the real thing
		static const UINT32 tints[3] =
		{
			FROMRGB(246, 234, 226), // faded rose
			FROMRGB(230, 238, 232), // faded mint
			FROMRGB(238, 234, 220), // faded straw
		};
		for (int i = 0; i < 10; ++i)
		{
			const INT32 px = FE_CT_X + 14 + (i % 5) * 92;
			const INT32 py = y + (i / 5) * 62;
			FillRounded(px + 3, py + 3, 84, 56,
					FROMRGB(206, 196, 170), 5, FE_RGB_CARD);
			FillRounded(px, py, 84, 56, FE_RGB_RULE, 5, FE_RGB_CARD);
			FillRounded(px + 1, py + 1, 82, 54, tints[i % 3], 5,
					FE_RGB_RULE);
			DrawCat(px + 10, py + 4, FelineCat::Pose(i));
		}
		y += 128;
		const ST::string link = "download all ten: kitty_stamps.gif (44kb)";
		PrintAt(FONT10ARIAL, FONT_DKBLUE, FE_CT_X + 14, y,
				link);
		FillRect(FE_CT_X + 14, y + 10, StringPixLength(link, FONT10ARIAL),
				1, FE_RGB_LINK);
		PrintAt(FONT10ARIAL, FONT_METALGRAY,
				FE_CT_X + 20 + StringPixLength(link, FONT10ARIAL), y,
				"(link broken since March)");
	}

	// --- clicks -------------------------------------------------------------
	void NavCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const int page = int(MSYS_GetRegionUserData(region, 0));
		if (page == gFelinePage) return;
		gFelinePage = page;
		gfFelineNaming = false;
		gFelineTicker = ST::string();
		FelineRedraw();
	}

	void SyncRegions();

	void ActCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const INT32 mx = FE_CT_X + INT32(region->RelativeXPos);
		const INT32 my = FE_CT_Y + INT32(region->RelativeYPos);
		int slot = -1;
		for (int i = 0; i < gFelineBtnCount; ++i)
		{
			const BtnAt& b = gFelineBtnAt[i];
			if (b.live && mx >= b.x && mx <= b.x + b.w &&
			    my >= b.y && my <= b.y + 18)
			{
				slot = i;
				break;
			}
		}
		if (slot < 0) return;
		if (gFelinePage == FP_MYCAT)
		{
			if (!Fostered())
			{
				if (slot == 0 && !gfFelineNaming && ChargeSociety(15))
				{
					gfFelineNaming = true;
					gFelineNameInput.clear();
				}
				FelineRedraw();
				return;
			}
			FelineCat::State s = CatState();
			if (s.away)
			{
				if (slot == 0 && ChargeSociety(30))
				{
					FelineCat::Recover(s, Today());
					StoreCat(s);
					gFelineTicker = "brenda writes: it is back. it "
							"is not talking about it.";
				}
			}
			else if (slot == 0)
			{
				if (FelineCat::Feed(s, Today()))
				{
					StoreCat(s);
					gFelineTicker = ST::format("{} approves of the "
							"tin.", ST::string(gFeline.szName));
				}
			}
			else if (slot == 1)
			{
				gFelinePage = FP_CATTERY;
				SyncRegions();
			}
			FelineRedraw();
			return;
		}
		if (gFelinePage == FP_CATTERY)
		{
			if (slot == 0 && ChargeSociety(9))
			{
				gFeline.ubSupplies = UINT8(std::min(250,
						int(gFeline.ubSupplies) + 6));
				gFelineTicker = "hamous will bring the pack on his "
						"next round. he says hello.";
			}
			else if (slot == 1 && ChargeSociety(2))
			{
				gFeline.ubSupplies = UINT8(std::min(250,
						int(gFeline.ubSupplies) + 1));
				gFelineTicker = "one tin, noted on the shelf ledger "
						"in brenda's hand.";
			}
			else if (slot == 2 && ChargeSociety(25))
			{
				gFelineTicker = "the whisker ships when manny can "
						"bear to part with it. so, soon-ish.";
			}
			FelineRedraw();
			return;
		}
	}

	void MidiCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		gfFelineMidi = !gfFelineMidi;
		gFelineTicker = gfFelineMidi
				? "you hear nothing. the file has been missing since "
				  "1997."
				: "the silence is now intentional.";
		FelineRedraw();
	}

	void RingCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (MSYS_GetRegionUserData(region, 0) == 1)
		{
			// next in the ring: the neighbour with a warehouse
			GoToHiddenSite(LAPTOP_MODE_CATZON);
			return;
		}
		gFelineTicker = "the previous neighbour does not answer. it has "
				"not answered since the invasion.";
		FelineRedraw();
	}

	void SyncRegions()
	{
		if (!gfFelineRegionsUp) return;
		auto set = [](MOUSE_REGION& r, bool on)
		{
			if (on) r.Enable(); else r.Disable();
		};
		for (MOUSE_REGION& r : gFelineNavRegion) set(r, true);
		set(gFelineClickRegion, gFelinePage == FP_MYCAT ||
				gFelinePage == FP_CATTERY);
		set(gFelineMidiRegion, true);
		for (MOUSE_REGION& r : gFelineRingRegion) set(r, true);
	}

	void DefineRegions()
	{
		if (gfFelineRegionsUp) return;
		gfFelineRegionsUp = true;
		// the nav spans are computed by the render; regions get fixed,
		// generous boxes over the link row instead
		INT32 x = FE_CT_X;
		for (int i = 0; i < FP_COUNT; ++i)
		{
			const INT32 tw =
				StringPixLength(FE_NAV[i], FONT10ARIALBOLD) + 10;
			MSYS_DefineRegion(&gFelineNavRegion[i],
					UINT16(FE_X(x)), UINT16(FE_Y(50)),
					UINT16(FE_X(x + tw)), UINT16(FE_Y(73)),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					NavCallback);
			MSYS_SetRegionUserData(&gFelineNavRegion[i], 0, i);
			x += tw + 2;
		}
		MSYS_DefineRegion(&gFelineClickRegion,
				UINT16(FE_X(FE_CT_X)), UINT16(FE_Y(FE_CT_Y)),
				UINT16(FE_X(FE_CT_X + FE_CT_W)),
				UINT16(FE_Y(FE_CT_Y + 290)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				ActCallback);
		MSYS_DefineRegion(&gFelineMidiRegion,
				UINT16(FE_X(FE_PAGE_W - 100)), UINT16(FE_Y(FE_PAGE_H - 26)),
				UINT16(FE_X(FE_PAGE_W - 20)), UINT16(FE_Y(FE_PAGE_H - 8)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				MidiCallback);
		for (int i = 0; i < 2; ++i)
		{
			MSYS_DefineRegion(&gFelineRingRegion[i],
					UINT16(FE_X(FE_PAGE_W / 2 - 90 + i * 130)),
					UINT16(FE_Y(FE_PAGE_H - 26)),
					UINT16(FE_X(FE_PAGE_W / 2 - 40 + i * 130)),
					UINT16(FE_Y(FE_PAGE_H - 8)),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					RingCallback);
			MSYS_SetRegionUserData(&gFelineRingRegion[i], 0, i);
		}
		SyncRegions();
	}

	void RemoveRegions()
	{
		if (!gfFelineRegionsUp) return;
		gfFelineRegionsUp = false;
		for (MOUSE_REGION& r : gFelineNavRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gFelineClickRegion);
		MSYS_RemoveRegion(&gFelineMidiRegion);
		for (MOUSE_REGION& r : gFelineRingRegion) MSYS_RemoveRegion(&r);
	}
}

// --- page lifecycle ---------------------------------------------------------
void EnterFeline()
{
	try
	{
		guiFelineArt = AddVideoObjectFromFile("sti/laptop/felinecats.sti");
	}
	catch (...)
	{
		guiFelineArt = nullptr; // the ink kit carries the page
	}
	try
	{
		// the show animal is the game's own creature, exhibited raw
		guiFelineCt = AddVideoObjectFromFile("anims/animals/ct_breath.sti");
	}
	catch (...)
	{
		guiFelineCt = nullptr;
	}
	for (auto& m : gFelineMembers)
	{
		if (m.face) continue;
		try { m.face = Load33Portrait(GetProfile(m.pid)); }
		catch (...) { m.face = nullptr; }
		if (!m.face)
		{
			try { m.face = LoadSmallPortrait(GetProfile(m.pid)); }
			catch (...) { m.face = nullptr; }
		}
		if (!m.face)
		{
			try { m.face = LoadBigPortrait(GetProfile(m.pid)); }
			catch (...) { m.face = nullptr; }
		}
		if (!m.face)
		{
			// NPCs keep their faces where the dialogue engine looks:
			// the b-prefixed talking heads
			try
			{
				m.face = AddVideoObjectFromFile(ST::format(
						"faces/b{02d}.sti", int(m.pid)));
			}
			catch (...) { m.face = nullptr; }
		}
	}
	gFeline.ubFlags |= FELINE_FLAG_VISITED;
	gFeline.usLastVisitDay = Today();
	SyncCat();
	gFelinePage = FP_HOME;
	gfFelineNaming = false;
	gFelineTicker = ST::string();
	DefineRegions();
	FelineRedraw();
}

void ExitFeline()
{
	if (guiFelineArt)
	{
		DeleteVideoObject(guiFelineArt);
		guiFelineArt = nullptr;
	}
	if (guiFelineCt)
	{
		DeleteVideoObject(guiFelineCt);
		guiFelineCt = nullptr;
	}
	for (auto& m : gFelineMembers)
	{
		if (m.face)
		{
			DeleteVideoObject(m.face);
			m.face = nullptr;
		}
	}
	RemoveRegions();
}

void RenderFeline()
{
	gFelineBtnCount = 0;
	RenderChrome();
	switch (gFelinePage)
	{
		case FP_HOME:    RenderHome();      break;
		case FP_BREED:   RenderBreed();     break;
		case FP_MYCAT:   RenderMyCat();     break;
		case FP_CATTERY: RenderCattery();   break;
		case FP_SIGHT:   RenderSightings(); break;
		case FP_SHOWS:   RenderShows();     break;
		case FP_BOOK:    RenderGuestbook(); break;
		case FP_STAMPS:  RenderStamps();    break;
	}
	MarkButtonsDirty();
	RenderWWWProgramTitleBar();
	InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
			LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
}

void HandleFeline()
{
	// hover repaint for the link row and buttons
	{
		UINT32 uiHover = 1;
		auto acc = [&](const MOUSE_REGION& r)
		{
			uiHover = uiHover * 33u + (Hover(r) ? 2u : 1u);
		};
		for (const MOUSE_REGION& r : gFelineNavRegion) acc(r);
		acc(gFelineClickRegion);
		acc(gFelineMidiRegion);
		if (Hover(gFelineClickRegion))
		{
			uiHover = uiHover * 31u
					+ UINT32(gFelineClickRegion.RelativeXPos / 8)
					* 131u
					+ UINT32(gFelineClickRegion.RelativeYPos / 8);
		}
		static UINT32 uiLast = 0;
		if (uiHover != uiLast)
		{
			uiLast = uiHover;
			FelineRedraw();
		}
	}
	// the caret blinks while the foster form waits
	if (gfFelineNaming)
	{
		static UINT32 uiBlink = 0;
		const UINT32 blink = GetJA2Clock() / 400;
		if (blink != uiBlink)
		{
			uiBlink = blink;
			FelineRedraw();
		}
	}
	SyncRegions();
}

bool FelineHandleTypedKey(UINT32 usParam, UINT16 usKeyState)
{
	if (!gfFelineNaming) return false;
	if (usParam == SDLK_RETURN || usParam == SDLK_KP_ENTER)
	{
		if (!gFelineNameInput.empty())
		{
			std::snprintf(gFeline.szName, sizeof(gFeline.szName), "%s",
					gFelineNameInput.c_str());
			gFeline.ubFlags |= FELINE_FLAG_FOSTERED;
			gFeline.usLastFedDay = Today();
			gFeline.ubHunger = 0;
			gFeline.ubSupplies = 2; // the welcome tins
			gfFelineNaming = false;
			gFelineTicker = ST::format("filed. {} of cambria is "
					"yours.", ST::string(gFeline.szName));
			SyncRegions();
			FelineRedraw();
		}
		return true;
	}
	if (usParam == SDLK_BACKSPACE)
	{
		if (!gFelineNameInput.empty())
		{
			gFelineNameInput.pop_back();
			FelineRedraw();
		}
		return true;
	}
	if (usParam >= 32 && usParam < 127) return true;
	return false;
}

bool FelineHandleTextInput(const ST::utf32_buffer& codepoints)
{
	if (!gfFelineNaming) return false;
	bool grew = false;
	for (char32_t cp : codepoints)
	{
		if (cp < 32 || cp > 126) continue;
		if (gFelineNameInput.size() >= 16) break;
		gFelineNameInput += char(cp);
		grew = true;
	}
	if (grew) FelineRedraw();
	return true;
}

// --- persistence -------------------------------------------------------------
FelinePersist FelineGetPersist() { return gFeline; }
void FelineSetPersist(const FelinePersist& p) { gFeline = p; }
