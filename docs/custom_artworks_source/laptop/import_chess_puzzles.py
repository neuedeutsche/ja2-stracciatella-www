#!/usr/bin/env python3
"""Import Lichess-format puzzles into the generated chach.com corpus.

Reads a puzzle database and emits src/game/Laptop/ChessPuzzles.{h,cc}. The
corpus is compiled in rather than loaded at runtime: it is small, it never
changes during a campaign, and baking it lets the unit test validate every
position with no file I/O and no game data present.

Accepted inputs:
    *.json  a JSON array of Lichess puzzle objects
    *.csv   the official lichess_db_puzzle.csv export
    *.html  a page with a `var somethingPuzzles = [ ... ]` array inline

Lichess record shape, which the loader depends on:
    FEN     the position BEFORE a setup move
    Moves   space-separated UCI; Moves[0] is the opponent's setup move, so the
            solver plays the side opposite the FEN's side to move, and the
            solver's own moves are at odd indices from there

Puzzles are emitted sorted by rating so the daily rotation gets harder as the
campaign goes on.

Usage:
    python3 import_chess_puzzles.py <database> [--limit N]
"""

import argparse
import csv
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
OUT_H = REPO / "src" / "game" / "Laptop" / "ChessPuzzles.h"
OUT_CC = REPO / "src" / "game" / "Laptop" / "ChessPuzzles.cc"

REQUIRED = ("PuzzleId", "FEN", "Moves", "Rating")


def load_json(path):
    return json.loads(path.read_text())


def load_html(path):
    """Pull the first `var <name>Puzzles = [ ... ]` array out of a page."""
    text = path.read_text(errors="replace")
    match = re.search(r"var\s+\w*[Pp]uzzles\s*=\s*\[", text)
    if not match:
        raise SystemExit(f"no puzzle array found in {path}")
    start = text.index("[", match.start())
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "[":
            depth += 1
        elif text[i] == "]":
            depth -= 1
            if depth == 0:
                return json.loads(text[start:i + 1])
    raise SystemExit(f"unterminated puzzle array in {path}")


def load_csv(path):
    with path.open(newline="") as fh:
        return [
            {
                "PuzzleId": row.get("PuzzleId", ""),
                "FEN": row.get("FEN", ""),
                "Moves": row.get("Moves", ""),
                "Rating": row.get("Rating", "0"),
                "Themes": row.get("Themes", ""),
            }
            for row in csv.DictReader(fh)
        ]


def load(path):
    suffix = path.suffix.lower()
    if suffix == ".json":
        return load_json(path)
    if suffix == ".csv":
        return load_csv(path)
    if suffix in (".html", ".htm"):
        return load_html(path)
    raise SystemExit(f"unsupported database format: {suffix}")


def sanity_check(entry):
    """Cheap structural checks. Real legality is proven by the unit test."""
    for key in REQUIRED:
        if not entry.get(key):
            return f"missing {key}"
    fen_fields = entry["FEN"].split()
    if len(fen_fields) < 4:
        return "FEN has too few fields"
    if fen_fields[1] not in ("w", "b"):
        return "FEN side to move is not w or b"
    moves = entry["Moves"].split()
    if len(moves) < 2:
        return "needs a setup move plus at least one solution move"
    for move in moves:
        if not re.fullmatch(r"[a-h][1-8][a-h][1-8][qrbn]?", move):
            return f"malformed UCI move {move!r}"
    return None


# One title per slot, applied after the rating sort, so they escalate with the
# difficulty ramp: routine contract work at the start, the war at the end.
TITLES = [
    "Morning Patrol", "Sector Sweep", "Loose Ends", "Standard Contract",
    "Petty Cash", "Basic Training", "Warm Up", "Roll Call", "Light Duty",
    "First Blood",
    "Cheap Insurance", "Hazard Pay", "Bar Fight", "The Shakedown",
    "Small Arms", "Night Watch", "Trigger Discipline", "Cover Me",
    "Two Rounds Left", "Short Contract",
    "Interrupt", "Overwatch", "Suppressing Fire", "Flanking Manoeuvre",
    "The Ambush", "Blind Corner", "Point Man", "Danger Close",
    "Fire in the Hole", "No Cover",
    "Contract Dispute", "Late Payment", "Severance Package",
    "Hostile Takeover", "The Merger", "The Retainer", "Bonus Clause",
    "Kill Fee", "Termination Notice", "Non-Negotiable",
    "Drassen Airfield", "The Mine at Grumm", "Cambria Hospital",
    "San Mona Nights", "Chitzena Crossing", "Balime Heights",
    "Estoni Junction", "Alma Barracks", "Orta Approach", "Tixa Yard",
    "Bloodcat Country", "Jungle Rot", "Radio Silence", "Dead Drop",
    "Compromised", "Burned", "The Informant", "Counterintelligence",
    "Sleeper", "Double Agent",
    "Enemy Reinforcements", "Elite Patrol", "Armoured Column",
    "Rocket Rifle", "Mortar Position", "Minefield", "Tank Buster",
    "Air Support", "Heavy Weapons", "Breach and Clear",
    "Queen's Gambit Declined", "Royal Guard", "The Palace Steps",
    "Meduna Express", "Her Excellency Regrets", "State Funeral",
    "The Throne Room", "Loyalists", "Crown Prosecution",
    "Long Live the Queen",
    "Last Man Standing", "Out of Ammunition", "Bleeding Out",
    "Field Surgery", "No Extraction", "Behind Enemy Lines", "Pinned Down",
    "The Long Night", "Casualty Report", "Hold This Position",
    "Endgame in Arulco", "Total War", "The Final Contract",
    "Scorched Earth", "No Survivors", "Checkmate at Meduna", "Liberation",
    "The Last Sector", "Full Retirement", "Mission Accomplished",
]


def c_string(value):
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def emit(entries):
    OUT_H.write_text('''#ifndef CHESSPUZZLES_H
#define CHESSPUZZLES_H

// Generated by docs/custom_artworks_source/laptop/import_chess_puzzles.py
// Do not edit by hand - re-run the importer against the puzzle database.

// One daily puzzle, in Lichess's layout.
//
// fen is the position BEFORE the setup move, and the first move in `moves` is
// the opponent's. Play that move to reach the position the player is asked to
// solve, which also means the solver is the side opposite the FEN's side to
// move. From there the solver's moves are at even indices of the remainder and
// the scripted replies at odd ones.
struct ChessPuzzle
{
	const char* id;
	const char* fen;
	const char* moves;
	const char* themes;
	const char* title;
	short       rating;
};

extern const ChessPuzzle CHESS_PUZZLES[];
extern const int NUM_CHESS_PUZZLES;

#endif
''')

    lines = [
        '#include "ChessPuzzles.h"',
        "",
        "// Generated by docs/custom_artworks_source/laptop/import_chess_puzzles.py",
        "// Sorted by rating: the daily rotation gets harder as the campaign runs.",
        "",
        "const ChessPuzzle CHESS_PUZZLES[] =",
        "{",
    ]
    for i, e in enumerate(entries):
        lines.append(
            "\t{{ {}, {}, {}, {}, {}, {} }},".format(
                c_string(e["PuzzleId"]),
                c_string(e["FEN"]),
                c_string(e["Moves"]),
                c_string(e.get("Themes", "")),
                c_string(TITLES[i % len(TITLES)]),
                int(e["Rating"]),
            )
        )
    lines += [
        "};",
        "",
        "const int NUM_CHESS_PUZZLES = int(sizeof(CHESS_PUZZLES) / sizeof(CHESS_PUZZLES[0]));",
        "",
    ]
    OUT_CC.write_text("\n".join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path)
    parser.add_argument("--limit", type=int, default=0,
                        help="keep only the first N puzzles after sorting")
    args = parser.parse_args()

    raw = load(args.database)
    kept, dropped = [], []
    seen = set()
    for entry in raw:
        problem = sanity_check(entry)
        if problem:
            dropped.append((entry.get("PuzzleId", "?"), problem))
            continue
        if entry["PuzzleId"] in seen:
            dropped.append((entry["PuzzleId"], "duplicate id"))
            continue
        seen.add(entry["PuzzleId"])
        kept.append(entry)

    kept.sort(key=lambda e: (int(e["Rating"]), e["PuzzleId"]))
    if args.limit:
        kept = kept[:args.limit]

    if not kept:
        raise SystemExit("no usable puzzles found")

    emit(kept)
    print(f"wrote {OUT_CC} and {OUT_H}: {len(kept)} puzzles, "
          f"ratings {kept[0]['Rating']}-{kept[-1]['Rating']}")
    for puzzle_id, problem in dropped:
        print(f"  dropped {puzzle_id}: {problem}", file=sys.stderr)


if __name__ == "__main__":
    main()
