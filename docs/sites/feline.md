# The Arulco Feline Society

"Founded 1994. Member, the FELINE WEBRING." A hobbyist cat-fanciers' club
page for the laptop browser — the softest thing the 1999 web produced,
in a country whose only native feline eats people.

## Premise

Arulco has no domestic animals except cows. The one canonical feline is
the bloodcat: no profile, `CREATURE_TEAM`, farmed for blood sport at the
N5 arena and breeding at the I16 lair. The Society is nine enthusiasts
who maintain a breed standard for the "Arulcan Highland" in the loving,
delusional register of a breed registry, post show results while the war
takes their members, and run a foster programme because their audience
cannot keep a cat in a pocket.

## Pages

| Page | Content |
| --- | --- |
| HOME | welcome, notices (1999 show POSTPONED / the memorial), member count |
| THE BREED | the Arulcan Highland standard, points table, "photograph" |
| MY CAT | the foster programme: name it, feed it, watch it (see below) |
| CATTERY | supplies: tins by the pack or the piece, one framed whisker |
| SIGHTINGS | confirmed reports read live from `SectorInfo[].bBloodCats`; wrong rumours |
| SHOWS | 1997/1998 results in full; the 1997 names no longer update |
| GUESTBOOK | Manny's devotion, Hamous's route notes, Auntie's arguments |
| STAMPS | the ten poses as free clip-art; the download link is broken |

## The cat

Engine-free rules in `FelineCat.{h,cc}` (unit tested, `FelineCat_unittest.cc`):

- Hunger rises 12 points per unfed campaign day; rollover is lazy on page
  entry, `ChessRollOverDay()` fashion.
- While tins last, Brenda feeds from the shelf every second day unasked.
- Mood ladder: AWAY > HIDING (war nearby) > THIN (80+) > HUNGRY (45+) >
  LONELY (5 days unvisited) > PLAYFUL (fed, seen) > CONTENT.
- Ten poses drawn as monoline ink sprites, chosen per mood.
- It does not die. Twelve days past empty it "has gone to stay with a
  member in the country", recoverable for $30 and a letter from Brenda
  that does not quite explain.

## What the world does to the site

- `FACT_I16_BLOODCATS_KILLED` (259): black-bordered notices, the breed
  standard moves to the past tense.
- `FACT_PLAYER_KNOWS_ABOUT_BLOODCAT_LAIR` (335): the breed page gets
  defensive about "remarks made about the I16 colony".
- `FACT_BRENDA_DEAD` (79): succession — Manny takes the keys, the ledger
  and the cats; payments reroute to him.
- Sightings list any sector whose `bBloodCats > 0` — i.e. the ambush
  roll has actually happened there. No free intel.

## Wiring

- `LAPTOP_MODE_FELINE`, `FELINE_BOOKMARK` (Laptop.h), full dispatch in
  Laptop.cc, dev entry `JA2_DEV_FELINE=1`.
- Discovery: `EVENT_FELINE_SOCIETY_EMAIL` (82) delivers Brenda's one
  letter (`FELINE_EMAIL_INVITE` 5300, sender 54) on day 5, with the same
  backfill-on-first-laptop-entry pattern as the other sites.
- Persistence: `FelinePersist` takes the laptop blob's last 13 reserve
  bytes (11 fixed + marker `0xFE` + name length); the cat's name rides
  the save tail behind the chess guestbook line. The reserve is now 0 —
  this was the last site that fit.
- Money: `PAYMENT_TO_NPC` to Brenda (Manny after the succession),
  balance-guarded like `ChargeSpeck()`.

## Deliberately not done (yet)

- Physical Bobby Ray shipments (and Pablo's theft) for the tins — the
  supplies currently land on the shelf ledger directly.
- Baked STI art: the poses, rosettes and the photograph render
  procedurally, which turned out to be enough.
- Guestbook condolences on merc deaths, Cambria-falls staleness, and the
  war-nearby mood hook (the `warNearby` parameter exists and is tested,
  but the page passes `false`).
