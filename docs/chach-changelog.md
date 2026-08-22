# Chach.com — changelog

All notable changes to Chach.com, newest first.

**Spoilers throughout.** See [docs/sites/chach.md](sites/chach.md) for what
the site is, or the project README for what this repo is.

## 2026-08-22 — the seats learn to count material

- **The bots stop hanging pieces.** Every seat rolled a "greed" percentage
  and, on a hit, played a *uniformly random legal capture* — the engine was
  never asked. That is where a 1994-rated account taking a defended pawn with
  its queen came from. The engine now has static exchange evaluation
  (`ChessGame::See`), so a greedy grab is chosen from the captures that
  survive the recapture, best first, and a blunder is drawn from moves that do
  not simply give material away. How much a seat can fail to notice scales
  with its rating: a pawn to the titled seats, a knight to the bottom of the
  ladder. Both pickers — Play and Watch — now share one function.
- **Won endgames now finish.** Material and piece-square tables score every
  rook shuffle the same, so K+R against a bare king wandered until the
  fifty-move rule rescued the defender. Added the standard mop-up term —
  drive the bare king off-centre, walk your own king in, and shrink the box
  he has left, which is what a rook cutting off a rank actually does — plus
  repetition detection inside the search, so a winning side can no longer
  think a repeat costs nothing. Tested by playing K+2R, K+R and K+Q out to
  mate at depth 3.
- The move on the board is marked in the list: a chip in the title badge's
  dress, brighter text and piece, and it follows the scrubber — walk the game
  back and forth and the list scrolls to keep it in view.
- Legal-move indicators are circles: a disc on an empty square, a ring around
  a piece that can be taken, in place of the square blob and four-edge box.
- Pieces separate into foot, body and head. The generator finds each waist in
  the silhouette and lays a shaded row under it, so the joins read the way the
  reference set's do. The knight is exempt — a line across the horse's chest
  reads as a scar.
- **Country flags** beside handles, after the rating, as on the reference.
  They follow A.I.M.'s bios rather than guesswork: the Dolviches fly Russian,
  Buns is Danish (Monica Sonderguard shot for Denmark at the Atlanta games),
  Scope's correction notice puts her in the S.A.S., and Enrico signs the
  guestbook under Arulco's own flag.
- The move list shows pieces instead of letters, reusing the capture-tally
  glyphs, and its zebra rows run the full width of the sidebar in a band a
  shade *lighter* than the panel rather than sunk into it.
- Discovery mail is one-shot. The invitation and every ad stage go through
  `AddEmailOnce`, and a scheduled campaign stage that outlives the visit which
  answered it no longer posts — the first notice included, which is the hole
  that let Kingpin's "PRE-APPROVED" arrive twice.

## 2026-08-21 — the whole site

- Corpus grown to 440 validated puzzles (ratings 400–2872), imported from
  the canonical database at
  `docs/custom_artworks_source/laptop/chach-puzzle-db.json` — about fourteen
  months of dailies before a repeat.
- **Play**: a full game against Grunty at his invented 1850.
- **Learn**: his three-lesson book, diagrams proven correct by the tests.
- **Watch**: the house plays itself, arriving mid-game.
- **Groups**: the guestbook, with a sign-once button.
- **The game report**: overnight analysis arriving as morning mail, signed
  AnalysisBot v0.9.
- A seven-day streak enters the campaign history log. The Gold Crown banner
  answers a click exactly once. The language switch returns as a footer
  link.
- Result card dims the whole board — squares, coordinates and pieces (via
  baked dim frames 12–23) — in place of the earlier scanlines.

## Earlier

- Daily puzzle with hearts, hint, streak, archive and titles; drag and
  click input; chess.com sound cues; the Neo piece set, composed from
  geometric primitives with mechanical symmetry; Grunty's discovery mail,
  streak and lapse letters; unattended mode while he is under contract;
  ad slot and hit counter.

## Parked ideas

- A rating for the player in Play mode, and a bot ladder beneath Grunty.
- Move list / PGN export on the Play panel.
- Puzzle Rush ("3 minutes, 3 strikes, ze timer drifts, zis is known").
