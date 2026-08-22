#ifndef FELINE_H
#define FELINE_H

#include "Types.h"

#include <string_theory/string>

// The Arulco Feline Society - "Founded 1994. Member, the FELINE WEBRING."
// Nine cat fanciers in a country whose only native feline eats people. The
// site is a hobbyist club page: breed standard, show results, sightings,
// a guestbook the war keeps leaking into, and a foster cat that lives in
// the back room of Brenda's store and ages by campaign day. The pet rules
// are engine-free in FelineCat.{h,cc}; this is the page around them.

void EnterFeline(void);
void ExitFeline(void);
void RenderFeline(void);
void HandleFeline(void);
bool FelineHandleTypedKey(UINT32 usParam, UINT16 usKeyState);
bool FelineHandleTextInput(const ST::utf32_buffer& codepoints);

// save persistence: the laptop blob's last 13 reserve bytes. The name is
// text and rides behind the fixed block, chess-guestbook fashion.
struct FelinePersist
{
	UINT8  ubFlags;        // FELINE_FLAG_* below
	UINT8  ubHunger;       // 0 fed .. 100 empty
	UINT8  ubSupplies;     // tins on Brenda's shelf
	UINT16 usLastFedDay;
	UINT16 usLastVisitDay;
	INT32  iSpent;         // lifetime dollars into the Society's tin
	char   szName[17];     // the foster cat; empty = not yet fostered
};

#define FELINE_FLAG_INVITED   0x01 // Brenda's letter has been scheduled
#define FELINE_FLAG_VISITED   0x02 // the member has seen the site
#define FELINE_FLAG_FOSTERED  0x04 // a cat has a name and a page
#define FELINE_FLAG_AWAY      0x08 // "staying with a member in the country"
#define FELINE_FLAG_MOURNED   0x10 // the I16 memorial page has been seen

FelinePersist FelineGetPersist();
void FelineSetPersist(const FelinePersist& p);

#endif
