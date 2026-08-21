# Chach.com

> **Spoilers.** This page describes the site in full — how you find it, who
> runs it, what happens when you hire him. If you would rather discover it in
> game, stop here and just play; the invitation finds you on its own.

A chess site run by one man off one machine: Helmut "Grunty" Grunther, of
A.I.M. It is a faithful port of the modern chess.com — dark theme, green
board, Neo pieces, a daily puzzle with a streak — built by someone in 1999
who cannot spell the name he wanted and will not discuss it.

> best viewed at 800x600 — solution tomorrow

<p align="center">
  <img src="../screenshots/chach.png" width="720"
       alt="The Chach.com daily puzzle, open in the laptop browser">
</p>

## Finding it

You do nothing. On the fourth evening of the campaign Grunty mails you,
unprompted — he has noticed you reading his A.I.M. page and never once
clicking the small link at the foot of it. That mail puts the bookmark in
your browser. Saves already past day four receive the letter backdated to
the evening it was sent.

## The daily puzzle

One puzzle per campaign day, the same in every save — day one is puzzle one.
The corpus is 440 real positions, rating-sorted from 400 to 2872, so the
rotation runs about fourteen months and gets harder as your war does. Every
position is replayed move-by-move by the test suite; a broken record is a
build failure, not an unsolvable board.

- **Five tries**, shown as hearts. A legal move that is not the answer costs
  one; an illegal move just snaps back, free and silent. Spent hearts crack.
- **The hint** marks the piece that has to move, and costs a try.
- **The streak** counts consecutive days solved. Miss a day and it dies;
  hold it seven days and it enters the campaign history log.
- **The archive**: earlier days open read-only with their answers played out.
- Each day carries a title, escalating with the difficulty ramp — *Morning
  Patrol* at the bottom, *Checkmate at Meduna* near the top.

Pieces move by click or by drag, with chess.com's target dots, capture
rings, last-move highlights and per-move sound cues (check outranks the
capture that delivered it). Solving raises a result card over a dimmed
board; failing plays the solution onto it.

## The game report

Analysis is not instant in 1999. When your day ends — either way — the
report is queued on the one machine Grunty owns and arrives as mail the
next morning: the solution line set in SAN by the engine, and a verdict
keyed to how many tries you spent. It is signed *AnalysisBot v0.9*. The bot
is him.

## The rest of the site

- **Play** — a full game against the proprietor. You take White; he plays
  at his invented 1850, thinking a moment before each reply. NEW GAME is
  also how you resign.
- **Learn** — his book: three lessons, each a diagram and three lines. The
  test suite proves every diagram shows what its caption claims.
- **Watch** — the house plays itself, live, arriving mid-game the way a
  real table would.
- **Groups** — the guestbook. The regulars, the accidental traffic a typo
  domain earns — the official tourism page of Chach, Slovakia would like
  the e-mails to stop — and a sign-once button that adds your I.M.P. handle
  permanently.
- **The ad slot** rotates a Bobby Ray's creative, a Parlour cross-promotion,
  and the GOLD CROWN membership, which is coming soon forever. Click the
  crown and he mails you, once, asking you never to ask again.
- **EN | DE** in the footer. The proprietor is German; the whole site is
  translated in his own voice.

## He writes

Grunty mails rarely, and only when something happened: the invitation, a
streak reaching three days and each week it survives, a lapsed run worth
mourning, the morning game reports, the crown. Every letter arrives on the
strategic clock, hours after the event — the only honest latency for a man
with one modem.

## Hire him and

The site stays up — the daily puzzle is automated and he is not. A notice
appears over the board (*ze proprietor is on contract. yours.*), and the
coach bubble carries his away message for the length of the contract. He
explains the situation by mail once per contract.

## The pipeline

Everything on screen is generated: the Neo piece set (34px and 20px, plus
dimmed twins for the result card), logo, nav icons, banners — one Python
file, composed from geometric primitives with mechanical symmetry for the
bilateral pieces. The corpus is imported from a Lichess-format database and
compiled in. The engine is 0x88 with full legal movegen, proven by perft
against the standard positions.
