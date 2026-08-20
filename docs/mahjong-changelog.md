# San Mona Mahjong Parlour — changelog

All notable changes to the mahjong minigame, newest first. The parlour is a
laptop website added on top of JA2 Stracciatella; see the project README for
what it is and how to build it.

The page carries its own build number (`MJ_PAGE_VERSION` in
`src/game/Laptop/Mahjong.cc`), shown on the home page, the ladder and the
table footer. Bump it when the site changes.

## Parked ideas

Not built, kept on the record:

- **Skill-tiered rooms.** The lobby currently offers one table. A beginner /
  intermediate / high-stakes split would let the buy-in, the rake and the
  opponents' error rates scale with the room — and give the permanently
  "under construction" Beginner Room joke somewhere real to land.
- **Choosing your table.** Different rooms seat different people: A.I.M.
  mercs at the low table (loose, chatty, beatable), the San Mona regulars in
  the middle, the Queen and Kingpin only at the top. Opponent skill becomes a
  choice the player makes rather than a fixed property of the site.

## 0.4.2 — Ordering, light and profiles

- Chat delivery unified into **one ordered lane**. Speech was queued while
  house notices posted instantly, so a notice could overtake the line said
  before it ("@wolf has left the room" printing above @wolf's own message).
  Everything now enqueues in the order it was said and a single consumer
  pops it; humans keep their typing pace, the house prints fast but never
  jumps the line, and your own messages stay instant as a local echo.
- The typing indicator is derived from the head of that queue rather than
  its own timer, and is suppressed for anyone who has left — the classic
  stuck-indicator race. Visitors cannot leave until their words have landed.
- A lamp over your seat: an elliptical pool of lighter felt, dithered into
  the speckle so it reads as light rather than a shape.
- A wooden lip with grain between the felt and the client.
- Profile bios for whoever holds your chair, bottom-anchored under the
  portrait — twenty of them, one per merc, plus your own.
- Your own messages run warm: cream handle, cream body, warm avatar frame,
  so your voice reads as a set against everyone else's neutral grey.
- Player of the Month keeps their handle and portrait after leaving, so
  their lines stay attributed in the log.
- Elliot mutes chat flooders on an escalating scale, with a live countdown
  in the input line.
- The input field scrolls with the caret instead of truncating, and wrap
  budgets match what the renderer will actually draw.

## 0.4 — The room comes alive

**Chat client rebuilt around how people actually talk**

- Messages arrive as *bursts*: sentences (and long comma clauses) land as
  separate messages, mirroring real messaging habits — Baron & Ling found the
  send key does the work of a full stop, so trailing periods are dropped.
  Deidranna keeps hers, because a sentence-final period reads as cold and
  abrupt (Gunraj et al.), which is exactly her register.
- Humans and the terminal have different voices: NPC lines queue behind an
  `@handle is typing` indicator with an animated braille-style wave and then
  appear whole; system lines print themselves out character by character.
- Roughly one message in nine gets typed and never sent.
- Saying "brb" now means it: that character goes genuinely away for 26–48s,
  misses whatever is said in the meantime, and returns with a fresh line.
- Pronounced gaps between messages — a lead-in before anyone starts typing,
  a pause between a speaker's own bursts, and a floor of quiet after each
  post so two characters never talk over each other.
- Three visually distinct line types: speech (white handle, grey body),
  house notices (green), and table events — claims, kongs, wins, settlements
  — in yellow with their own marker.
- Chat scrolling rewritten on the canonical stick-to-bottom model: one
  continuous pixel position with an eased target, per-frame reconciliation of
  content growth and history trimming, stickiness decided by input rather
  than by the animation's own writes, and rows clipped at the pane edge so a
  departing line visibly slides out. Wheel scrolling eases too, and works
  anywhere along the bottom bar.
- Scrolling up holds your place; new messages no longer yank you down.
- The input line is a real field: it scrolls with the caret instead of
  truncating, and accepts 220 characters.
- Chat greets you by your own handle and portrait even while the House is
  warming your seat.

**Moderation, visitors and outages**

- Elliot is the room's `mod`. Post four times in four seconds and he mutes
  you — one minute for the first offence, two for the second, escalating —
  apologising the whole time. The input line shows a live countdown.
- Role badges next to handles, used sparingly: `mod` (Elliot), `owner`
  (Kingpin), `staff` (Darren), `POTM` for the visiting champion.
- A Player of the Month drops in at random: a real A.I.M. merc with their own
  portrait, rating and fourteen lines of trash talk, then leaves.
- Connection dropouts: a feed occasionally dies for several seconds, showing
  a dial-up spinner and "reconnecting", announced in the log.

**Content**

Authored lines roughly doubled to ~1,000:

| pool | before | after |
|---|---|---|
| greetings | 15 | 36 |
| idle chatter | 45 | 90 |
| reactions to your win | 15 | 36 |
| wall running low | 9 | 24 |
| tenpai leaks | 12 | 30 |
| bad hands | 12 | 30 |
| void-suit draws | 12 | 30 |
| war commentary | 18 | 36 |
| claims | 9 | 24 |
| win taunts | 12 | 32 |
| ron-victim taunts | 12 | 32 |
| spouse barbs | 6 | 12 |
| bickering exchanges | 28 | 60 |
| lurker cameos | 24 | 54 |
| house PA notices | 17 | 42 |

New material leans on JA1/JA2 canon: Andreas Chivaldori teaching his son the
game, Miguel Cordona's 1988 election run, Deidranna's Romanian winters and 98%
result, Elliot's dungeon paperwork and bank career, Metavira sap trees, the
Shady Lady, Frank's bar, the Hummer, and Spike as a dispute resolution
mechanism.

**Look**

- One shared accent colour for every green element in the site, backed by a
  new `SetFontForegroundRGB()` in the font engine so UI code can pick exact
  colours instead of palette indices.
- Hand-drawn terminal icons (house, open book, info disc, chat bubble) on a
  left rail, replacing the metal buttons.
- Chat avatars with hairline frames; system and event lines get marker chips
  so every line shares one column.
- Score ledger redesigned as mini profile rows: avatar, name, handle, rating
  column, chip splay, score and delta, with final standings ranked.
- Wind and void-suit badges moved onto the portraits.
- Tile faces redrawn: monolinear sans-serif numerals, softer ink, even
  spacing in the dot and bamboo grids.
- Vertical space reclaimed twice — the chat bar grew from 92px to 136px.
- Immersive mode: the bubble icon expands the bar to the full page with a
  career-record panel.

## 0.3 — The site

- AIM-style home page with a warning block, live-table tile showing all four
  seats, and four link tiles.
- House Ladder on red felt: ratings for every regular, your derived rating,
  recent results, a permanently banned entry, and a Report a Cheater button
  whose responses escalate.
- House rules as a four-page paged modal, the last page a signed letter from
  the proprietor with a procedurally drawn handwritten signature.
- Guestbook, hit counter, and dragon medallions generated procedurally.
- Discovery via an escalating spam campaign: four emails that grow less
  "spam" and more "Kingpin", backfilled to the day they would have arrived.

## 0.2 — The world

- Real stakes wired into Finances: $250 buy-in at 20 points to the dollar,
  10% house rake, Kingpin loans at 20% vig, triple-stakes invitational every
  seventh day.
- Kingpin emails your winnings and collects your debts via delayed strategic
  events; Elliot writes a letter he regrets.
- Rotating cast: Elliot, Kingpin and Tony share the left seat by game day;
  Darren and Madame Layla cover for the Queen when she is busy.
- Character voice samples at the table, staggered so nobody talks over
  anybody.
- War-aware chatter tracking campaign progress.
- Webcam feeds with blinking, facial ticks and dropouts.

## 0.1 — The game

- Engine-free Sichuan mahjong core: 108 tiles, three-tile exchange, void
  suit, pong and kong, robbing the kong, fan scoring with roots, kong
  bonuses, tenpai and flower-pig settlements, bloody-battle flow.
- Deterministic given a seed; 22 gtest cases covering win detection,
  zero-sum payments and meld bookkeeping.
- AI with per-character skill, error rates and a grudge model.
- All art generated programmatically into indexed STI sheets.
