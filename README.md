# ja2-stracciatella-www

*New websites for Jagged Alliance 2's in-game laptop browser, built on
[JA2 Stracciatella](https://github.com/ja2-stracciatella/ja2-stracciatella).
Arulco's internet, circa 1999, expanded.*

![The Parlour lobby](docs/screenshots/lobby.png)

## What is this?

An unofficial fan fork that adds new sites to the laptop's web browser —
playable, written, and dressed in the worst web design 1999 had to offer.
Each one is discovered the way you discovered anything back then: it emails
you first.

**Now live: the San Mona Mahjong Parlour** — a fully playable online mahjong
room run by mob boss Peter "Kingpin" Klaus, where you face Enrico, Deidranna
and Elliot for real campaign money.

> est. 1999 — a Kingpin establishment. Games are fair because Mr. Klaus says so.

Face **Enrico**, **Deidranna** and **Elliot** (plus a rotating cast of San Mona regulars) over dial-up, for real campaign money.

## Features

**The game**
- Full Sichuan rules (血战到底, "bloody to the end"): 108 suit tiles, the three-tile exchange (huan san zhang), a declared void suit (que yi men), pong and kong with replacement draws, robbing the kong, fan scoring with roots, kong bonuses, tenpai and flower-pig settlements — winners retire and the hand fights on until three stand or the wall dies
- Engine-free deterministic core with a 154-test suite; AI opponents with distinct skill levels, error rates and a grudge model (the Queen keeps count)

**The world**
- Real stakes: $250 buy-in settled through the Finances screen at 20:1, a 10% house rake, Kingpin loans at 20% vig, and the triple-stakes BLOODY INVITATIONAL every 7th day
- Kingpin emails your winnings and collects your debts via delayed strategic events; Elliot writes you a letter he immediately regrets
- Seat rotation tied to the campaign calendar: Elliot most nights, Kingpin and Tony dropping in, and Darren or Madame Layla covering when the Queen is busy running your war
- The table knows the war: chatter, voice samples ("Elliot, you idiot!") and moods track your campaign progress — and what happens in Meduna does not stay out of the chat

**The 1999 of it all**
- AIM-style home page with a WARNING block, a live-table lobby, a House Ladder with Yahoo-style ratings (provisional asterisk included, one entry permanently BANNED), paged house rules signed by the proprietor, a guestbook, and a hit counter
- chess.com-style chat with mini avatars, an ELIZA-flavoured chatbot that answers what you type, away messages, lurker cameos, house PA announcements, and superstition rituals when the losing streaks bite
- Webcam portrait feeds with blinking, facial ticks, dropouts, and a dead-account glitch nobody at the table wants to talk about
- When nobody is seated, the House plays an exhibition four-hander you can watch live — mid-game, chat already scrolling

**The pipeline**
- Every asset — tiles, felt, neon sign, chips, dragons, TV static, even Kingpin's handwritten signature — is generated programmatically into indexed STI sheets by one Python script

## Screenshots

| | |
|---|---|
| ![Lobby](docs/screenshots/lobby.png) | ![Table](docs/screenshots/table.png) |
| ![Ladder](docs/screenshots/ladder.png) | ![House rules](docs/screenshots/rules.png) |
| ![Hand over](docs/screenshots/handover.png) | ![Spam](docs/screenshots/spam.png) |

## Building

You need the original Jagged Alliance 2 game data (any retail/GOG copy). On macOS:

```sh
brew install cmake googletest fltk
mkdir _bin && cd _bin
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-macos.cmake -DBUILD_LAUNCHER=OFF ..
make -j$(sysctl -n hw.ncpu)
./ja2 -unittests   # 154 tests
./ja2
```

Point `~/.ja2/ja2.json`'s `game_dir` at your JA2 installation. For other platforms, follow upstream's [COMPILATION.md](COMPILATION.md).

In game: open the laptop, read your mail, and wait for the Parlour to find you. It will.

## Code map

| Where | What |
|---|---|
| `src/game/Laptop/MahjongGame.{h,cc}` | engine-free Sichuan rules core (no JA2 headers, deterministic by seed) |
| `src/game/Laptop/MahjongGame_unittest.cc` | gtest coverage: win detection, zero-sum payments, meld bookkeeping |
| `src/game/Laptop/Mahjong.{h,cc}` | the entire website: lobby, table, ladder, rules, chat, exhibition |
| `docs/custom_artworks_source/laptop/generate_mahjong_tiles.py` | generates all STI art assets |
| `src/game/Laptop/{Laptop,EMail}.*`, `src/game/Strategic/Game_Event_Hook.*` | integration: page registration, spam campaign, win/debt emails, save persistence |

## Changelog

See [docs/mahjong-changelog.md](docs/mahjong-changelog.md) for what changed and when.

## Roadmap

More sites for the browser. San Mona has room for another establishment or
two, and Arulco's other institutions have not been online yet at all.

## Credits & license

Built on [JA2 Stracciatella](https://github.com/ja2-stracciatella/ja2-stracciatella) — all credit for the port, engine and tooling belongs to that project and its contributors. This fork's additions follow the same terms as upstream: changes are released to the public domain; the original Jagged Alliance 2 source was released by Strategy First Inc. in 2004 under the SFI Source Code License Agreement (see *SFI Source Code license agreement.txt*). Jagged Alliance 2 and its characters are the property of their respective rights holders; this is an unofficial fan project and requires an original copy of the game.
