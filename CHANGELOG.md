# Changelog

The fork's own history — the sites of Arulco's internet, from the first
tile onward. Upstream engine changes live in `changes.md`.

## 1.4 — The parlour learns to celebrate (2026-08-24)

### mahjong
- Wins ring San Mona's own fight bell — the ring from the fights downstairs — louder when the win is yours
- The winner's panel strobes lightning-white in quick ~90ms pulses: white ground, every colour collapsed to black, then back
- Last place on the final standings wears the tactical death skull (the auto-resolve `smfaces` frame) over a dimmed portrait
- Seats that already won stop drawing from their war/idle chat pools and speak from a smug spectator pool instead
- Chat handles of already-won seats wear a gold **won** badge, mod-badge style
- Squad kibitz lines (Grunty, Ivan, Steroid, Fidel) show the speaking merc's own face instead of yours
- Discardable hand tiles nudge up 1px under the pointer, live hover repaint included
- Match-end card holds two doors side by side inside the frame — **New** and **Leave** — retiring the stock button that floated behind the border
- The verdict footnote wraps to the card (breaking on `.` `!` `?`) instead of overflowing; sits tight under the button in tiny green type
- Verdict rows sort by placement — 1 on top — instead of seat order
- The WON marker names the winning tile as rank + suit glyph, stamped in gold so red stays the void's colour
- The Queen only calls Elliot an idiot when his feed actually cost her — not when she won from it or had already retired

### chess
- Guestbook signers' avatars and name lockups click through to their member pages; your closing signature opens yours
- The coach's face links to @buns' page — she signs her lessons
- The in-game footer reworked: resign becomes a quiet full-width text row that extends the backdrop upward, the four history chevrons fill the strip in the hint button's own cut
- Every footer button (puzzle HINT, learn, play) shares one width, height and label colour rule
- The challenge picker's PLAY button takes the sidebar CTA's full size and says just PLAY
- Button polish sweep: five-row dither gradients, truly rounded corners via outline-only fills, shadow feet that wrap the bottom arc, a 2px selection ring without corner holes
- Title pills (IM/FM/CM) tightened to hug their letters, lettering centred

## 1.3 — Member pages and open hands (2026-08-23)

### chess — the engine gets honest
- A real search behind the ladder: transposition table, iterative deepening, quiescence with SEE pruning, check extensions, null move, tapered evaluation
- The bot ladder re-tuned to honest ratings — @spider (1512) through @ivan_d IM (2145), each on its own depth and think budget
- Lessons demand the actual move, wrong tries snap back with the coach's correction, and the last lesson ends in a graduation card
- Game start and game end carry their own sound cues; move cues stay mutually exclusive
- The daily streak flame decodes grey at streak zero; the puzzle modal closes on a single SEE YOU TOMORROW button; the lesson target square blinks

### chess — member pages
- Clicking any portrait on the play or watch rows opens that member's profile
- Member pages: title chip, flag, rating, one-line bio, invented-but-stable site record, and the real head-to-head ledger against you
- RECENT GAMES feeds backdate through the site's own history, guestbook-fashion — the ladder existed before you dialled in
- Feed rows dress like chat lines: portrait chips, clickable handles linking onward
- CHALLENGE opens the lobby's TIME CONTROL sidebar as a modal — all four controls, daily included — with a full-size PLAY beneath
- Your own page: Elo rating with provisional asterisk (K=40 first ten games), W-L-D record, form chips, streak commentary, and a ten-game ledger persisted in save games behind a versioned marker
- The profile page becomes a stack of separate cards — header, stats, games — with move-list-style flush zebra rows and axially centred columns
- Piece dragging works on touch-driven pointers (the drag no longer dies on tap-style input)

### mahjong — the table explains itself
- The verdict card rebuilt as a podium: gold/silver/bronze/lead rows on warm charcoal, rank numbers leading
- Winners' hands laid open inside their rows, decomposed into sets and pair with the winning tile ringed and popped on top
- Meld provenance: a red notch on the gold bar shows which seat fed each claimed pong or kong
- Win announcements name the tile and the fan anatomy; the engine records the winning tile and each meld's feeder
- The void pick becomes drawn buttons carrying their suit glyph — hover floods them red, previews the doomed tiles in hand and lifts them as if already dismissed, and counts what the pick would take
- Void suit icons: a winner's void badge retires into a silver star, kin to the new dotted silver outline on won players' portraits
- The card sizes itself to content, centres on the felt, and the Next Hand button's click region follows it everywhere
- The shanten hint stays quiet mid-action (no more "-1 tiles from a waiting hand") and names a void-blocked wait for what it is
- Claim buttons (Mahjong!, Pong!, Kong!) pack from the left edge in fixed order

### the rest
- **Catzon.an** opens: a hidden proto-Amazon cat-products store behind the Feline Society webring's "next" link — six products, grained pixel-art thumbnails, disgruntled reviewer portraits, a cart billing through Bobby Ray's, and tins that stock the Society's cupboard
- **C.U.P.I.D.**: every member's last-seen line reads differently — online now / minutes / hours / days / "quiet, on contract probably" — instead of one shared clock
- Docs: step-by-step macOS install guide; README lists all live sites and retires the branch-per-site scheme

## 1.2 — Love and cats (2026-08-22)

### cupid
- **Mercs & Kisses** launches: Speck's dating venture, wired into 1999 and the war
- Profile deck with real anatomy: mugshot-scale faces, personals headlines, seek preference, vitals, deal breakers
- Verdict circles flanking the name (skull / kiss / ring), honest circle buttons, tally on the heart
- Member account view, scrolling dossier with hard scroll stops, private lines for matches, an about page
- The dark cosmic theme: single left rail, one rotating banner, everything in its column
- Upgrade charges real money, the wire resets, popups eat the keyboard properly

### elsewhere
- **The Arulco Feline Society** opens its clubhouse
- Chess play polish, hand-drawn banners and piece refinements
- Mahjong speaks to you in the second person; hand-over rows go two-line

## 1.1 — Chach.com becomes a site (2026-08-21)

- **Play**: live games against the proprietor's regulars — resign flow (armed twice), finished-game card, per-seat playing strength, clocks, capture tallies, gliding pieces
- **The lobby**: time controls chess.com-fashion, daily chess by letter (one move a day), sidebar move list and chat, quieter seeking
- **Learn and Watch stop being excuses** — eight lessons and a living exhibition board with full-size row avatars, grey ratings and a zebra move table
- A year of daily puzzles, each named, with the archive chevrons to walk them
- The Neo piece set hand-built shape by shape: the queen as a prong fan, the egg bishop, joined-ring king, collared pawn, the knight's belly — with guaranteed symmetry and an SVG override pipeline
- Guestbook rebuilt: title badges, drawn banners, layout-aware typing, the emailed game report, and the crown ad that answers exactly once
- The site earns its campaign place: Grunty's invitation mail (backfilled if he's already hired), ad slot, hit counter, unfinished pages in the right places
- `JA2_DEV_CHESS=1` boots straight to the site for development
- Crash fix: Watch wanted a 16bpp blitter and got an 8bpp one

## 1.0 — The parlour opens (2026-08-20)

- An engine-free **Sichuan mahjong core** with a full unit-test suite: bloody battle to three winners, huan san zhang exchange, void suits, kong payments, pig penalties, zero-sum invariants
- The **San Mona Mahjong Parlour** goes live in the laptop browser, wired into emails and campaign events — Kingpin's spam escalates from day 2
- The chat client rebuilt twice over: one ordered delivery lane, typing rhythms with ghosts and bursts, table lighting, profile bios, an immersive mode
- **Chach.com's daily puzzle page** with its own chess core (perft-tested) and a validated puzzle corpus
- Chess.com-style chrome: the coach and her speech bubble, sound cues wired to the board, result cards, green hearts, day chips
- Drag-and-drop resolved by pointer; only a move that lands makes a sound
- The project reframed as **ja2-stracciatella-www**: the browser as a platform, README written around it
