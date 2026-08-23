#include "Catzon.h"

#include "Feline.h"

#include "Cursors.h"
#include "Finances.h"
#include "Font.h"
#include "Font_Control.h"
#include "Game_Clock.h"
#include "HImage.h"
#include "Laptop.h"
#include "MercPortrait.h"
#include "Soldier_Profile.h"
#include "LaptopSave.h"
#include "MouseSystem.h"
#include "Timer_Control.h"
#include "VObject.h"
#include "VSurface.h"
#include "Video.h"
#include "WordWrap.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <string_theory/format>
#include <string_theory/string>

// The reference is Amazon circa 1999: white page, a navy masthead, a
// search box nobody needs on six products, tabs promising departments
// that will never open, and the orange CTA that launched a thousand
// warehouses. Ours ships through Pablo, which the reviews reflect.

namespace
{
#define CZ_X(x) ((INT32)(LAPTOP_SCREEN_UL_X + (x)))
#define CZ_Y(y) ((INT32)(LAPTOP_SCREEN_WEB_UL_Y + (y)))
#define CZ_PAGE_W 500
#define CZ_PAGE_H 390

	constexpr UINT32 CZ_RGB_PAPER  = FROMRGB(250, 250, 246);
	constexpr UINT32 CZ_RGB_NAVY   = FROMRGB(20, 34, 64);
	constexpr UINT32 CZ_RGB_NAVY_D = FROMRGB(12, 22, 44);
	constexpr UINT32 CZ_RGB_ORANGE = FROMRGB(240, 153, 30);
	constexpr UINT32 CZ_RGB_GOLD   = FROMRGB(255, 204, 51);
	constexpr UINT32 CZ_RGB_INK    = FROMRGB(30, 30, 34);
	constexpr UINT32 CZ_RGB_RULE   = FROMRGB(200, 198, 190);
	constexpr UINT32 CZ_RGB_LINK   = FROMRGB(0, 51, 153);
	constexpr UINT32 CZ_RGB_STAR   = FROMRGB(230, 140, 20);
	constexpr UINT32 CZ_RGB_TRAY   = FROMRGB(238, 240, 244);

	struct CzReview
	{
		const char* who;
		INT16       pid;   // a real profile, or -1 for the anonymous
		int         stars; // out of 5
		const char* title;
		const char* text;
	};

	struct CzProduct
	{
		const char* name;
		const char* blurb;
		int         price;
		int         stars;    // the average the page admits to
		int         reviews;  // the count it claims
		bool        tins;     // ships to Brenda's shelf as supplies
		CzReview    review[2];
	};

	const CzProduct CZ_PRODUCTS[6] =
	{
		{ "TUNA-ISH TIN, 6-PACK",
		  "ze good kind, allegedly. label art by somebody who has heard "
		  "of fish.", 9, 2, 41, true,
		  { { "manny", MANNY, 1, "ze fish does not exist",
		      "i looked it up. no ocean has zis fish. my cat noticed "
		      "before i did and now we do not speak." },
		    { "a mother of six (cats)", -1, 3, "they eat it",
		      "they eat it the way soldiers eat: quickly, without eye "
		      "contact. two stars extra because the tins stack." } } },
		{ "SCRATCHING POST (AMMO CRATE, USED)",
		  "genuine army surplus pine. some assembly. some splinters. "
		  "some markings we cannot explain.", 14, 3, 17, false,
		  { { "@shady_lady", -1, 4, "smells of cordite",
		      "the cat approves. the husband approves. i am watching "
		      "both of them re-evaluate their lives around a crate." },
		    { "disappointed in balime", -1, 1, "STILL HAD AMMO IN IT",
		      "customer service said 'bonus'. i said 'children'. we "
		      "have reached an arrangement involving a shovel." } } },
		{ "LASER POINTER - MILITARY SURPLUS",
		  "red dot. cat chases dot. simple. do not point at aircraft, "
		  "hillsides, or anything you love.", 25, 1, 33, false,
		  { { "hamous", HAMOUS, 1, "zat is a designator",
		      "my friend, this is not a pointer. i pointed it at the "
		      "curtains and something answered from very far away." },
		    { "@e11iot", ELLIOT, 1, "look at me returning things!!",
		      "i pressed the button before reading. the queen has "
		      "questions about her east wall. one star." } } },
		{ "BLOODCAT-PROOF GLOVES",
		  "triple-stitched leather, tested against a housecat of "
		  "unusual temper. results extrapolated.", 40, 1, 58, false,
		  { { "auntie", AUNTIE, 1, "zey are not",
		      "i will keep this review professional: they are not. the "
		      "extrapolation died with my hedge. bessie is avenged of "
		      "nothing." },
		    { "one-glove Tomas", -1, 2, "sold as pair",
		      "arrived as glove. singular. customer service asked "
		      "which hand i liked more. a fair question. two stars." } } },
		{ "FLEA POWDER 'GUARANTEED'",
		  "eliminates fleas, ticks, and ambiguity. veterinarian "
		  "consulted (once, informally).", 6, 2, 24, false,
		  { { "brenda", BRENDA, 2, "the guarantee is on the powder",
		      "read the tin: the powder itself is guaranteed to be "
		      "powder. it is. the fleas have opened a second location "
		      "behind the counter." },
		    { "flo^", FLO, 3, "fur situation",
		      "the fleas are gone. so is some fur. the cat looks "
		      "aerodynamic now, which it enjoys. mixed feelings." } } },
		{ "CAT CARRIER (ARMORED)",
		  "twelve kilos of peace of mind. viewing slit, sandbag "
		  "mounts, tie-downs rated for road and rotor.", 120, 4, 9, false,
		  { { "a courier who knows", -1, 5, "survived ze ambush",
		      "the taxi did not. the driver did not (he is fine, he "
		      "walked). the cat emerged judging everyone. five stars." },
		    { "small arms enthusiast", -1, 4, "minor complaint",
		      "stops what the label promises. minus one star: my cat "
		      "now refuses the normal carrier, and the armored one "
		      "does not fit a bicycle." } } },
	};

	enum CzPage { CZP_LIST, CZP_DETAIL };
	int  giCzPage    = CZP_LIST;
	int  giCzProduct = 0;
	int  giCzCartQty[6] = {};
	ST::string gCzTicker;
	bool gfCzRegionsUp = false;

	struct CzFace { INT16 pid; SGPVObject* face; };
	CzFace gCzFaces[6] =
	{
		{ MANNY, nullptr }, { HAMOUS, nullptr }, { ELLIOT, nullptr },
		{ AUNTIE, nullptr }, { BRENDA, nullptr }, { FLO, nullptr },
	};

	MOUSE_REGION gCzRowRegion[6];   // product rows / list links
	MOUSE_REGION gCzBuyRegion;      // the orange button on the detail page
	MOUSE_REGION gCzCheckoutRegion; // the cart line in the masthead
	MOUSE_REGION gCzBackRegion;     // "back to ze society" escape hatch
	MOUSE_REGION gCzListRegion;     // detail page: back to all products

	// --- kit ---------------------------------------------------------------
	void FillRect(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb)
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, CZ_X(x), CZ_Y(y),
					CZ_X(x + w), CZ_Y(y + h), Get16BPPColor(rgb));
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
		MPrint(CZ_X(x), CZ_Y(y), text);
	}

	void PrintCentred(SGPFont font, UINT8 colour, INT32 cx, INT32 y,
				const ST::string& text)
	{
		PrintAt(font, colour, cx - StringPixLength(text, font) / 2, y, text);
	}

	INT32 Wrapped(INT32 x, INT32 y, INT32 w, UINT8 colour,
				const ST::string& text)
	{
		return DisplayWrappedString(UINT16(CZ_X(x)), UINT16(CZ_Y(y)),
				UINT16(w), 2, FONT10ARIAL, colour, text,
				FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
	}

	void CzRedraw() { fPausedReDrawScreenFlag = TRUE; }

	bool Hover(const MOUSE_REGION& r)
	{
		return (r.uiFlags & MSYS_MOUSE_IN_AREA) != 0;
	}

	// five stars, filled to n: the currency of the whole idea
	void Stars(INT32 x, INT32 y, int n)
	{
		for (int i = 0; i < 5; ++i)
		{
			const UINT32 c = i < n ? CZ_RGB_STAR : CZ_RGB_RULE;
			const INT32 sx = x + i * 9;
			FillRect(sx + 2, y, 2, 2, c);
			FillRect(sx, y + 2, 6, 2, c);
			FillRect(sx + 1, y + 4, 4, 1, c);
			FillRect(sx, y + 5, 2, 1, c);
			FillRect(sx + 4, y + 5, 2, 1, c);
		}
	}

	// JA2 paints in grain: deterministic single-pixel flecks over a
	// surface, seeded so the noise never swims between frames
	void Grain(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 dark,
				UINT32 lite, int seed)
	{
		std::uint32_t st = std::uint32_t(seed) * 2654435761u + 13u;
		for (INT32 yy = 0; yy < h; ++yy)
		{
			for (INT32 xx = 0; xx < w; ++xx)
			{
				st = st * 1103515245u + 12345u;
				const std::uint32_t roll = (st >> 16) % 100;
				if (roll < 9)       FillRect(x + xx, y + yy, 1, 1, dark);
				else if (roll > 95) FillRect(x + xx, y + yy, 1, 1, lite);
			}
		}
	}

	// the transition between lit and shadow sides, checkered
	void Dither(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb)
	{
		for (INT32 yy = 0; yy < h; ++yy)
		{
			for (INT32 xx = (yy & 1); xx < w; xx += 2)
			{
				FillRect(x + xx, y + yy, 1, 1, rgb);
			}
		}
	}

	// a product thumbnail: gradient studio backdrop, bevelled rim, the
	// goods shaded like somebody owned a lamp - and grained like the
	// game the page lives inside
	void Thumb(int idx, INT32 x, INT32 y)
	{
		// the studio: light falls from the top, a soft floor shadow
		FillRect(x, y, 40, 14, FROMRGB(255, 255, 255));
		FillRect(x, y + 14, 40, 12, FROMRGB(248, 248, 244));
		FillRect(x, y + 26, 40, 14, FROMRGB(238, 238, 232));
		FillRect(x + 6, y + 32, 28, 3, FROMRGB(222, 222, 214));
		Grain(x + 2, y + 28, 36, 10, FROMRGB(228, 228, 220),
				FROMRGB(252, 252, 248), idx * 7 + 3);
		// the bevel rim: lit above, shaded below
		Frame(x, y, 40, 40, CZ_RGB_RULE);
		FillRect(x + 1, y + 1, 38, 1, FROMRGB(255, 255, 255));
		FillRect(x + 1, y + 38, 38, 1, FROMRGB(180, 178, 170));
		switch (idx)
		{
			case 0: // the tin: a cylinder with a lid catching the light
				FillRect(x + 10, y + 12, 20, 18, FROMRGB(172, 178, 184));
				FillRect(x + 10, y + 12, 4, 18, FROMRGB(198, 204, 210));
				FillRect(x + 26, y + 12, 4, 18, FROMRGB(140, 148, 156));
				FillRect(x + 9, y + 11, 22, 3, FROMRGB(210, 216, 222));
				FillRect(x + 9, y + 13, 22, 1, FROMRGB(120, 128, 136));
				FillRect(x + 13, y + 18, 14, 8, FROMRGB(70, 120, 165));
				FillRect(x + 13, y + 18, 14, 2, FROMRGB(100, 150, 195));
				FillRect(x + 15, y + 21, 8, 2, FROMRGB(230, 234, 238));
				Grain(x + 10, y + 14, 20, 15, FROMRGB(140, 148, 156),
						FROMRGB(214, 220, 226), 11);
				Dither(x + 24, y + 13, 3, 16, FROMRGB(152, 160, 168));
				break;
			case 1: // the crate: planks, grain, a lit top edge
				FillRect(x + 8, y + 13, 24, 18, FROMRGB(168, 132, 84));
				FillRect(x + 8, y + 13, 24, 2, FROMRGB(206, 172, 118));
				FillRect(x + 8, y + 28, 24, 3, FROMRGB(128, 96, 58));
				FillRect(x + 8, y + 20, 24, 1, FROMRGB(120, 90, 54));
				FillRect(x + 19, y + 13, 2, 18, FROMRGB(120, 90, 54));
				FillRect(x + 10, y + 16, 6, 1, FROMRGB(146, 112, 70));
				FillRect(x + 24, y + 24, 5, 1, FROMRGB(146, 112, 70));
				FillRect(x + 12, y + 7, 3, 6, FROMRGB(200, 170, 120));
				FillRect(x + 12, y + 7, 3, 1, FROMRGB(226, 200, 150));
				Grain(x + 8, y + 14, 24, 16, FROMRGB(140, 106, 64),
						FROMRGB(192, 156, 106), 23);
				Dither(x + 27, y + 14, 5, 16, FROMRGB(146, 112, 70));
				break;
			case 2: // the designator: machined body, hot lens, dots
				FillRect(x + 9, y + 18, 17, 7, FROMRGB(56, 58, 64));
				FillRect(x + 9, y + 18, 17, 2, FROMRGB(96, 100, 110));
				FillRect(x + 9, y + 23, 17, 2, FROMRGB(34, 36, 40));
				FillRect(x + 26, y + 19, 2, 5, FROMRGB(20, 20, 22));
				FillRect(x + 26, y + 20, 2, 2, FROMRGB(255, 90, 70));
				FillRect(x + 30, y + 15, 2, 2, FROMRGB(230, 40, 40));
				FillRect(x + 34, y + 21, 2, 2, FROMRGB(230, 40, 40));
				FillRect(x + 31, y + 27, 2, 2, FROMRGB(200, 30, 30));
				Grain(x + 9, y + 19, 17, 6, FROMRGB(38, 40, 44),
						FROMRGB(110, 114, 124), 37);
				break;
			case 3: // the glove: leather with a lit knuckle and stitches
				FillRect(x + 14, y + 9, 12, 19, FROMRGB(146, 104, 62));
				FillRect(x + 14, y + 9, 12, 3, FROMRGB(178, 134, 86));
				FillRect(x + 10, y + 13, 6, 9, FROMRGB(146, 104, 62));
				FillRect(x + 10, y + 13, 6, 2, FROMRGB(178, 134, 86));
				FillRect(x + 24, y + 12, 2, 14, FROMRGB(112, 78, 46));
				FillRect(x + 14, y + 26, 12, 6, FROMRGB(104, 72, 42));
				FillRect(x + 14, y + 26, 12, 1, FROMRGB(80, 54, 30));
				FillRect(x + 16, y + 15, 1, 8, FROMRGB(112, 78, 46));
				FillRect(x + 19, y + 15, 1, 8, FROMRGB(112, 78, 46));
				Grain(x + 10, y + 11, 16, 20, FROMRGB(118, 82, 48),
						FROMRGB(172, 130, 84), 41);
				Dither(x + 22, y + 12, 4, 14, FROMRGB(126, 90, 52));
				break;
			case 4: // the powder: a bottle with a cap and a proud label
				FillRect(x + 14, y + 9, 12, 23, FROMRGB(236, 236, 230));
				FillRect(x + 14, y + 9, 3, 23, FROMRGB(250, 250, 246));
				FillRect(x + 23, y + 9, 3, 23, FROMRGB(212, 212, 204));
				FillRect(x + 13, y + 8, 14, 5, FROMRGB(186, 58, 58));
				FillRect(x + 13, y + 8, 14, 1, FROMRGB(220, 100, 100));
				FillRect(x + 16, y + 17, 8, 10, FROMRGB(186, 58, 58));
				FillRect(x + 17, y + 19, 6, 2, FROMRGB(236, 236, 230));
				FillRect(x + 17, y + 23, 6, 1, FROMRGB(236, 236, 230));
				Grain(x + 14, y + 10, 12, 21, FROMRGB(214, 214, 206),
						FROMRGB(252, 252, 248), 53);
				Dither(x + 23, y + 10, 3, 21, FROMRGB(222, 222, 214));
				break;
			default: // the carrier: armour plate, rivets, the slit
				FillRect(x + 7, y + 11, 26, 20, FROMRGB(98, 106, 98));
				FillRect(x + 7, y + 11, 26, 3, FROMRGB(138, 148, 138));
				FillRect(x + 7, y + 28, 26, 3, FROMRGB(66, 74, 66));
				FillRect(x + 11, y + 17, 18, 4, FROMRGB(26, 28, 26));
				FillRect(x + 11, y + 17, 18, 1, FROMRGB(12, 14, 12));
				FillRect(x + 9, y + 13, 2, 2, FROMRGB(160, 170, 160));
				FillRect(x + 29, y + 13, 2, 2, FROMRGB(160, 170, 160));
				FillRect(x + 9, y + 26, 2, 2, FROMRGB(160, 170, 160));
				FillRect(x + 29, y + 26, 2, 2, FROMRGB(160, 170, 160));
				Grain(x + 7, y + 14, 26, 15, FROMRGB(76, 84, 76),
						FROMRGB(126, 136, 126), 67);
				Dither(x + 28, y + 14, 5, 14, FROMRGB(82, 90, 82));
				break;
		}
	}

	SGPVObject* CzFaceOf(INT16 pid)
	{
		for (const CzFace& f : gCzFaces)
		{
			if (f.pid == pid) return f.face;
		}
		return nullptr;
	}

	// a reviewer chip: the profile portrait scaled to fill 24x22, or the
	// low-contrast directory bust for the anonymous
	void CzAvatar(INT16 pid, INT32 x, INT32 y)
	{
		Frame(x - 1, y - 1, 26, 24, CZ_RGB_RULE);
		SGPVObject* const face = pid >= 0 ? CzFaceOf(pid) : nullptr;
		if (face && face->SubregionCount() > 0)
		{
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
				for (INT32 dy = 0; dy < 22; ++dy)
				{
					for (INT32 dx = 0; dx < 24; ++dx)
					{
						const UINT8 v = pix[size_t(dy * h / 22) * w +
								size_t(dx * w / 24)];
						if (v == 0) continue;
						buf[UINT32(CZ_Y(y + dy)) * pitch +
								UINT32(CZ_X(x + dx))] = pal[v];
					}
				}
				return;
			}
		}
		FillRect(x, y, 24, 22, FROMRGB(240, 238, 232));
		const UINT32 bust = FROMRGB(206, 202, 192);
		FillRect(x + 9, y + 4, 6, 6, bust);
		for (int r = 0; r < 7; ++r)
		{
			const double t = (6.0 - r) / 7.0;
			const INT32 half = INT32(9.0 *
					std::sqrt(std::max(0.0, 1.0 - t * t)) + 0.5);
			FillRect(x + 12 - half, y + 11 + r, 2 * half, 1, bust);
		}
	}

	int CzCartCount()
	{
		int n = 0;
		for (int q : giCzCartQty) n += q;
		return n;
	}

	int CzCartTotal()
	{
		int t = 0;
		for (int i = 0; i < 6; ++i) t += giCzCartQty[i] * CZ_PRODUCTS[i].price;
		return t;
	}

	// --- chrome -------------------------------------------------------------
	void RenderMasthead()
	{
		FillRect(0, 0, CZ_PAGE_W + 2, CZ_PAGE_H + 10, CZ_RGB_PAPER);
		FillRect(0, 0, CZ_PAGE_W + 2, 44, CZ_RGB_NAVY);
		// the logo: lowercase, with the smile drawn under the wrong letters
		PrintAt(FONT16ARIAL, FONT_MCOLOR_WHITE, 12, 8, "catzon.an");
		{
			// the swoosh, from c to z, missing the point on purpose
			for (int i = 0; i < 26; ++i)
			{
				const INT32 dy = (i * (26 - i)) / 46;
				FillRect(14 + i * 2, 30 - dy + 4, 2, 2, CZ_RGB_ORANGE);
			}
			FillRect(64, 30, 3, 2, CZ_RGB_ORANGE);
			FillRect(66, 28, 2, 2, CZ_RGB_ORANGE);
		}
		PrintAt(FONT10ARIAL, FONT_GRAY6, 110, 6,
				"Arulco's Biggest Selection");
		PrintAt(FONT10ARIAL, FONT_GRAY6, 110, 17, "(of cat items)");

		// the search box, over six products
		FillRect(240, 8, 150, 16, FROMRGB(255, 255, 255));
		Frame(240, 8, 150, 16, CZ_RGB_RULE);
		PrintAt(FONT10ARIAL, FONT_GRAY5, 245, 12, "search all 6 products");
		FillRect(390, 8, 30, 16, CZ_RGB_GOLD);
		Frame(390, 8, 30, 16, FROMRGB(180, 140, 30));
		PrintCentred(FONT10ARIAL, FONT_NEARBLACK, 405, 12, "GO");

		// the cart, which is the point
		const bool ch = Hover(gCzCheckoutRegion);
		PrintAt(FONT10ARIALBOLD, ch ? FONT_MCOLOR_LTYELLOW : FONT_MCOLOR_WHITE,
				428, 8, ST::format("CART ({})", CzCartCount()));
		PrintAt(FONT10ARIAL, ch ? FONT_MCOLOR_LTYELLOW : FONT_GRAY6, 428, 19,
				ST::format("${}", CzCartTotal()));

		// the tab strip of a company with plans
		FillRect(0, 44, CZ_PAGE_W + 2, 16, CZ_RGB_NAVY_D);
		PrintAt(FONT10ARIALBOLD, FONT_MCOLOR_LTYELLOW, 12, 48, "CATS");
		PrintAt(FONT10ARIAL, FONT_GRAY5, 52, 48, "| MORE CATS");
		PrintAt(FONT10ARIAL, FONT_GRAY5, 130, 48, "| DOGS (never)");
		PrintAt(FONT10ARIAL, FONT_GRAY5, 222, 48, "| BOOKS (ask ze other site)");
		const bool bh = Hover(gCzBackRegion);
		PrintAt(FONT10ARIAL, bh ? FONT_MCOLOR_WHITE : FONT_GRAY5,
				CZ_PAGE_W - 116, 48, "< back to ze society");

		PrintAt(FONT10ARIAL, FONT_GRAY5, 12, 66,
				"Hello, valued customer. (sign-in is coming. why?)");
		if (!gCzTicker.empty())
		{
			PrintAt(FONT10ARIAL, FONT_MCOLOR_DKRED, 12, CZ_PAGE_H - 14,
					gCzTicker);
		}
		PrintCentred(FONT10ARIAL, FONT_GRAY5, CZ_PAGE_W / 2, CZ_PAGE_H - 26,
				"fulfilment partner: Pablo Logistics S.A. - tracking "
				"numbers are commemorative");
	}

	// --- pages --------------------------------------------------------------
	void RenderList()
	{
		INT32 y = 84;
		PrintAt(FONT12ARIAL, FONT_NEARBLACK, 12, y - 3,
				"ALL DEPARTMENTS (there is one)");
		FillRect(12, y + 12, CZ_PAGE_W - 24, 1, CZ_RGB_RULE);
		y += 18;
		for (int i = 0; i < 6; ++i)
		{
			const CzProduct& p = CZ_PRODUCTS[i];
			const bool hov = Hover(gCzRowRegion[i]);
			if (hov) FillRect(10, y - 2, CZ_PAGE_W - 20, 44, CZ_RGB_TRAY);
			Thumb(i, 14, y);
			PrintAt(FONT10ARIALBOLD, hov ? FONT_DKBLUE : FONT_NEARBLACK,
					62, y + 5, p.name);
			Stars(62, y + 15, p.stars);
			PrintAt(FONT10ARIAL, FONT_GRAY5, 112, y + 15,
					ST::format("({})", p.reviews));
			PrintAt(FONT10ARIAL, FONT_GRAY5, 62, y + 25,
					"in stock: probably");
			PrintAt(FONT12ARIAL, FONT_MCOLOR_DKRED,
					CZ_PAGE_W - 24 - StringPixLength(
							ST::format("${}", p.price), FONT12ARIAL),
					y + 6, ST::format("${}", p.price));
			if (giCzCartQty[i] > 0)
			{
				PrintAt(FONT10ARIAL, FONT_DKGREEN,
						CZ_PAGE_W - 90, y + 25,
						ST::format("in cart: {}", giCzCartQty[i]));
			}
			y += 44;
		}
	}

	void RenderDetail()
	{
		const CzProduct& p = CZ_PRODUCTS[giCzProduct];
		INT32 y = 78;
		const bool lh = Hover(gCzListRegion);
		PrintAt(FONT10ARIAL, lh ? FONT_NEARBLACK : FONT_DKBLUE, 12, y,
				"< all 6 products");
		FillRect(12, y + 10,
				StringPixLength("< all 6 products", FONT10ARIAL), 1,
				CZ_RGB_LINK);
		y += 18;

		Thumb(giCzProduct, 14, y);
		PrintAt(FONT12ARIAL, FONT_NEARBLACK, 64, y, p.name);
		Stars(64, y + 16, p.stars);
		PrintAt(FONT10ARIAL, FONT_GRAY5, 114, y + 15,
				ST::format("{} customer reviews", p.reviews));
		PrintAt(FONT12ARIAL, FONT_MCOLOR_DKRED, 64, y + 28,
				ST::format("${}   ", p.price));
		PrintAt(FONT10ARIAL, FONT_GRAY5, 110, y + 30,
				p.tins ? "ships to: Brenda's shelf, Cambria"
				       : "ships to: wherever Pablo decides");
		y += 46;
		y += Wrapped(14, y, CZ_PAGE_W - 160, FONT_GRAY2, p.blurb) + 6;

		// the orange button that started it all
		{
			const bool hov = Hover(gCzBuyRegion);
			FillRect(CZ_PAGE_W - 132, 96, 118, 24,
					hov ? FROMRGB(255, 176, 60) : CZ_RGB_ORANGE);
			Frame(CZ_PAGE_W - 132, 96, 118, 24, FROMRGB(170, 100, 20));
			PrintCentred(FONT10ARIALBOLD, FONT_NEARBLACK,
					CZ_PAGE_W - 73, 103, "ADD TO CART");
			PrintCentred(FONT10ARIAL, FONT_GRAY5, CZ_PAGE_W - 73, 124,
					"1-click? one click IS the order");
		}

		FillRect(12, y, CZ_PAGE_W - 24, 1, CZ_RGB_RULE);
		y += 6;
		PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, 14, y,
				"CUSTOMER REVIEWS");
		y += 16;
		for (int r = 0; r < 2; ++r)
		{
			const CzReview& rv = p.review[r];
			CzAvatar(rv.pid, 14, y);
			Stars(46, y, rv.stars);
			PrintAt(FONT10ARIALBOLD, FONT_NEARBLACK, 96, y - 1, rv.title);
			PrintAt(FONT10ARIAL, FONT_GRAY5, 46, y + 11,
					ST::format("by {} - verified purchase (sadly)",
							rv.who));
			y += 27;
			y += Wrapped(14, y, CZ_PAGE_W - 40, FONT_GRAY2, rv.text) + 6;
			PrintAt(FONT10ARIAL, FONT_GRAY5, 14, y,
					"was this review helpful?  [ no ]");
			y += 16;
			if (r == 0)
			{
				// a light rule between reviews so they read as two
				// entries, not one column of grey text
				FillRect(14, y + 2, CZ_PAGE_W - 28, 1,
						FROMRGB(224, 222, 214));
				y += 12;
			}
		}
	}

	// --- clicks -------------------------------------------------------------
	void SyncRegions();

	void RowCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giCzPage != CZP_LIST) return;
		giCzProduct = int(MSYS_GetRegionUserData(region, 0));
		giCzPage = CZP_DETAIL;
		gCzTicker = ST::string();
		SyncRegions();
		CzRedraw();
	}

	void BuyCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giCzPage != CZP_DETAIL) return;
		++giCzCartQty[giCzProduct];
		gCzTicker = ST::format("added. ze cart holds {} item{} (${}). "
				"click CART to checkout.", CzCartCount(),
				CzCartCount() == 1 ? "" : "s", CzCartTotal());
		CzRedraw();
	}

	void CheckoutCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		const int total = CzCartTotal();
		if (total == 0)
		{
			gCzTicker = "ze cart is empty. ze cart is always listening.";
			CzRedraw();
			return;
		}
		if (LaptopSaveInfo.iCurrentBalance < total)
		{
			gCzTicker = "card declined. Pablo Logistics does not extend "
					"credit twice.";
			CzRedraw();
			return;
		}
		AddTransactionToPlayersBook(BOBBYR_PURCHASE, 0, GetWorldTotalMin(),
					-total);
		// the tins are the one order with an honest destination
		if (giCzCartQty[0] > 0)
		{
			FelinePersist fe = FelineGetPersist();
			fe.ubSupplies = UINT8(std::min(250,
					int(fe.ubSupplies) + giCzCartQty[0] * 6));
			fe.iSpent += giCzCartQty[0] * CZ_PRODUCTS[0].price;
			FelineSetPersist(fe);
		}
		const int items = CzCartCount();
		for (int& q : giCzCartQty) q = 0;
		gCzTicker = ST::format("order placed: {} item{}, ${}. tins reach "
				"Brenda's shelf; everything else enters ze Pablo "
				"system.", items, items == 1 ? "" : "s", total);
		CzRedraw();
	}

	void BackCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		GoToHiddenSite(LAPTOP_MODE_FELINE);
	}

	void ListCallback(MOUSE_REGION* region, UINT32 reason)
	{
		if (!(reason & MSYS_CALLBACK_REASON_POINTER_UP)) return;
		if (giCzPage != CZP_DETAIL) return;
		giCzPage = CZP_LIST;
		SyncRegions();
		CzRedraw();
	}

	void SyncRegions()
	{
		if (!gfCzRegionsUp) return;
		auto set = [](MOUSE_REGION& r, bool on)
		{
			if (on) r.Enable(); else r.Disable();
		};
		for (MOUSE_REGION& r : gCzRowRegion) set(r, giCzPage == CZP_LIST);
		set(gCzBuyRegion, giCzPage == CZP_DETAIL);
		set(gCzListRegion, giCzPage == CZP_DETAIL);
		set(gCzCheckoutRegion, true);
		set(gCzBackRegion, true);
	}

	void DefineRegions()
	{
		if (gfCzRegionsUp) return;
		gfCzRegionsUp = true;
		for (int i = 0; i < 6; ++i)
		{
			const INT32 y = 102 + i * 44;
			MSYS_DefineRegion(&gCzRowRegion[i],
					UINT16(CZ_X(10)), UINT16(CZ_Y(y - 2)),
					UINT16(CZ_X(CZ_PAGE_W - 10)), UINT16(CZ_Y(y + 42)),
					MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
					RowCallback);
			MSYS_SetRegionUserData(&gCzRowRegion[i], 0, i);
		}
		MSYS_DefineRegion(&gCzBuyRegion,
				UINT16(CZ_X(CZ_PAGE_W - 132)), UINT16(CZ_Y(96)),
				UINT16(CZ_X(CZ_PAGE_W - 14)), UINT16(CZ_Y(120)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				BuyCallback);
		MSYS_DefineRegion(&gCzCheckoutRegion,
				UINT16(CZ_X(424)), UINT16(CZ_Y(4)),
				UINT16(CZ_X(CZ_PAGE_W)), UINT16(CZ_Y(30)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				CheckoutCallback);
		MSYS_DefineRegion(&gCzBackRegion,
				UINT16(CZ_X(CZ_PAGE_W - 120)), UINT16(CZ_Y(44)),
				UINT16(CZ_X(CZ_PAGE_W)), UINT16(CZ_Y(60)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				BackCallback);
		MSYS_DefineRegion(&gCzListRegion,
				UINT16(CZ_X(10)), UINT16(CZ_Y(74)),
				UINT16(CZ_X(120)), UINT16(CZ_Y(90)),
				MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
				ListCallback);
		SyncRegions();
	}

	void RemoveRegions()
	{
		if (!gfCzRegionsUp) return;
		gfCzRegionsUp = false;
		for (MOUSE_REGION& r : gCzRowRegion) MSYS_RemoveRegion(&r);
		MSYS_RemoveRegion(&gCzBuyRegion);
		MSYS_RemoveRegion(&gCzCheckoutRegion);
		MSYS_RemoveRegion(&gCzBackRegion);
		MSYS_RemoveRegion(&gCzListRegion);
	}
}

// --- page lifecycle ---------------------------------------------------------
void EnterCatzon()
{
	for (auto& f : gCzFaces)
	{
		if (f.face) continue;
		try { f.face = Load33Portrait(GetProfile(ProfileID(f.pid))); }
		catch (...) { f.face = nullptr; }
		if (!f.face)
		{
			try { f.face = LoadSmallPortrait(GetProfile(ProfileID(f.pid))); }
			catch (...) { f.face = nullptr; }
		}
		if (!f.face)
		{
			try
			{
				f.face = AddVideoObjectFromFile(ST::format(
						"faces/b{02d}.sti", int(f.pid)));
			}
			catch (...) { f.face = nullptr; }
		}
	}
	giCzPage = CZP_LIST;
	gCzTicker = ST::string();
	DefineRegions();
	CzRedraw();
}

void ExitCatzon()
{
	for (auto& f : gCzFaces)
	{
		if (f.face)
		{
			DeleteVideoObject(f.face);
			f.face = nullptr;
		}
	}
	RemoveRegions();
}

void RenderCatzon()
{
	RenderMasthead();
	if (giCzPage == CZP_LIST) RenderList();
	else                      RenderDetail();
	MarkButtonsDirty();
	RenderWWWProgramTitleBar();
	InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y,
			LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y);
}

void HandleCatzon()
{
	UINT32 uiHover = 1;
	auto acc = [&](const MOUSE_REGION& r)
	{
		uiHover = uiHover * 33u + (Hover(r) ? 2u : 1u);
	};
	for (const MOUSE_REGION& r : gCzRowRegion) acc(r);
	acc(gCzBuyRegion);
	acc(gCzCheckoutRegion);
	acc(gCzBackRegion);
	acc(gCzListRegion);
	static UINT32 uiLast = 0;
	if (uiHover != uiLast)
	{
		uiLast = uiHover;
		CzRedraw();
	}
}
