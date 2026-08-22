#include "ChessGame.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <random>
#include <sstream>

namespace
{
	// 0x88 ray offsets
	const int KNIGHT_OFFSETS[8] = { 33, 31, 18, 14, -33, -31, -18, -14 };
	const int BISHOP_OFFSETS[4] = { 15, 17, -15, -17 };
	const int ROOK_OFFSETS[4]   = { 1, 16, -1, -16 };
	const int KING_OFFSETS[8]   = { 1, 15, 16, 17, -1, -15, -16, -17 };

	const int PIECE_VALUE[7] = { 0, 100, 320, 330, 500, 900, 0 };

	// Michniewski's simplified piece-square tables, written rank 8 first so the
	// literal reads like a board. Indexed from White's point of view; Black
	// mirrors vertically.
	const int PST_PAWN[64] = {
		 0,  0,  0,  0,  0,  0,  0,  0,
		50, 50, 50, 50, 50, 50, 50, 50,
		10, 10, 20, 30, 30, 20, 10, 10,
		 5,  5, 10, 25, 25, 10,  5,  5,
		 0,  0,  0, 20, 20,  0,  0,  0,
		 5, -5,-10,  0,  0,-10, -5,  5,
		 5, 10, 10,-20,-20, 10, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0,
	};
	const int PST_KNIGHT[64] = {
		-50,-40,-30,-30,-30,-30,-40,-50,
		-40,-20,  0,  0,  0,  0,-20,-40,
		-30,  0, 10, 15, 15, 10,  0,-30,
		-30,  5, 15, 20, 20, 15,  5,-30,
		-30,  0, 15, 20, 20, 15,  0,-30,
		-30,  5, 10, 15, 15, 10,  5,-30,
		-40,-20,  0,  5,  5,  0,-20,-40,
		-50,-40,-30,-30,-30,-30,-40,-50,
	};
	const int PST_BISHOP[64] = {
		-20,-10,-10,-10,-10,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-10,  5,  5, 10, 10,  5,  5,-10,
		-10,  0, 10, 10, 10, 10,  0,-10,
		-10, 10, 10, 10, 10, 10, 10,-10,
		-10,  5,  0,  0,  0,  0,  5,-10,
		-20,-10,-10,-10,-10,-10,-10,-20,
	};
	const int PST_ROOK[64] = {
		  0,  0,  0,  0,  0,  0,  0,  0,
		  5, 10, 10, 10, 10, 10, 10,  5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		  0,  0,  0,  5,  5,  0,  0,  0,
	};
	const int PST_QUEEN[64] = {
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		 -5,  0,  5,  5,  5,  5,  0, -5,
		  0,  0,  5,  5,  5,  5,  0, -5,
		-10,  5,  5,  5,  5,  5,  0,-10,
		-10,  0,  5,  0,  0,  0,  0,-10,
		-20,-10,-10, -5, -5,-10,-10,-20,
	};
	const int PST_KING[64] = {
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-20,-30,-30,-40,-40,-30,-30,-20,
		-10,-20,-20,-20,-20,-20,-20,-10,
		 20, 20,  0,  0,  0,  0, 20, 20,
		 20, 30, 10,  0,  0, 10, 30, 20,
	};

	const int* PST_FOR[7] = {
		nullptr, PST_PAWN, PST_KNIGHT, PST_BISHOP, PST_ROOK, PST_QUEEN, PST_KING
	};

	// Zobrist keys, built once from a fixed seed so hashes are stable across
	// runs and platforms - repetition detection must not drift between saves.
	struct Zobrist
	{
		std::uint64_t piece[2][7][128];
		std::uint64_t side;
		std::uint64_t castling[16];
		std::uint64_t epFile[8];

		Zobrist()
		{
			std::mt19937_64 rng(0x9E3779B97F4A7C15ULL);
			for (int c = 0; c < 2; ++c)
				for (int p = 0; p < 7; ++p)
					for (int s = 0; s < 128; ++s)
						piece[c][p][s] = rng();
			side = rng();
			for (int i = 0; i < 16; ++i) castling[i] = rng();
			for (int i = 0; i < 8; ++i) epFile[i] = rng();
		}
	};

	const Zobrist& Keys()
	{
		static const Zobrist z;
		return z;
	}

	constexpr int MATE_SCORE = 30000;

	char PieceLetter(std::uint8_t type)
	{
		switch (type)
		{
			case ChessGame::Pawn:   return 'P';
			case ChessGame::Knight: return 'N';
			case ChessGame::Bishop: return 'B';
			case ChessGame::Rook:   return 'R';
			case ChessGame::Queen:  return 'Q';
			case ChessGame::King:   return 'K';
			default:                return '?';
		}
	}

	std::uint8_t LetterPiece(char c)
	{
		switch (std::toupper(static_cast<unsigned char>(c)))
		{
			case 'P': return ChessGame::Pawn;
			case 'N': return ChessGame::Knight;
			case 'B': return ChessGame::Bishop;
			case 'R': return ChessGame::Rook;
			case 'Q': return ChessGame::Queen;
			case 'K': return ChessGame::King;
			default:  return ChessGame::NoPiece;
		}
	}

	std::string SquareName(std::uint8_t sq)
	{
		std::string s;
		s += char('a' + ChessGame::FileOf(sq));
		s += char('1' + ChessGame::RankOf(sq));
		return s;
	}
}

void ChessGame::Clear()
{
	std::memset(mBoard, 0, sizeof(mBoard));
	mSide     = White;
	mCastling = 0;
	mEp       = NO_SQUARE;
	mHalfmove = 0;
	mFullmove = 1;
	mKingSq[0] = mKingSq[1] = NO_SQUARE;
	mHistory.clear();
	mKeyHistory.clear();
}

void ChessGame::PlacePiece(std::uint8_t sq, std::uint8_t encoded)
{
	mBoard[sq] = encoded;
	const Color c = Color((encoded >> 3) & 1);
	const std::uint8_t type = encoded & 0x07;
	mKey ^= Keys().piece[c][type][sq];
	if (type == King) mKingSq[c] = sq;
}

void ChessGame::RemovePiece(std::uint8_t sq)
{
	const std::uint8_t encoded = mBoard[sq];
	if (!encoded) return;
	mKey ^= Keys().piece[(encoded >> 3) & 1][encoded & 0x07][sq];
	mBoard[sq] = 0;
}

void ChessGame::RecomputeKey()
{
	mKey = 0;
	for (int sq = 0; sq < 128; ++sq)
	{
		if (sq & 0x88) continue;
		const std::uint8_t encoded = mBoard[sq];
		if (!encoded) continue;
		mKey ^= Keys().piece[(encoded >> 3) & 1][encoded & 0x07][sq];
	}
	if (mSide == Black) mKey ^= Keys().side;
	mKey ^= Keys().castling[mCastling];
	if (mEp != NO_SQUARE) mKey ^= Keys().epFile[FileOf(mEp)];
}

void ChessGame::SetStartPosition()
{
	SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

bool ChessGame::SetFen(const std::string& fen)
{
	// parse into locals first so a malformed FEN leaves the position alone
	std::uint8_t board[128] = {};
	std::uint8_t kingSq[2]  = { NO_SQUARE, NO_SQUARE };

	std::istringstream in(fen);
	std::string placement, side, castling, ep;
	int halfmove = 0, fullmove = 1;
	if (!(in >> placement >> side >> castling >> ep)) return false;
	if (!(in >> halfmove)) halfmove = 0;
	if (!(in >> fullmove)) fullmove = 1;

	int rank = 7, file = 0;
	for (const char c : placement)
	{
		if (c == '/')
		{
			if (file != 8) return false;
			--rank;
			file = 0;
			if (rank < 0) return false;
			continue;
		}
		if (std::isdigit(static_cast<unsigned char>(c)))
		{
			file += c - '0';
			if (file > 8) return false;
			continue;
		}
		const std::uint8_t type = LetterPiece(c);
		if (type == NoPiece || file > 7 || rank < 0) return false;
		const Color col = std::isupper(static_cast<unsigned char>(c)) ? White : Black;
		const std::uint8_t sq = MakeSquare(file, rank);
		board[sq] = Encode(col, type);
		if (type == King)
		{
			if (kingSq[col] != NO_SQUARE) return false;  // two kings of a colour
			kingSq[col] = sq;
		}
		++file;
	}
	if (rank != 0 || file != 8) return false;
	if (kingSq[White] == NO_SQUARE || kingSq[Black] == NO_SQUARE) return false;

	std::uint8_t rights = 0;
	if (castling != "-")
	{
		for (const char c : castling)
		{
			switch (c)
			{
				case 'K': rights |= CASTLE_WK; break;
				case 'Q': rights |= CASTLE_WQ; break;
				case 'k': rights |= CASTLE_BK; break;
				case 'q': rights |= CASTLE_BQ; break;
				default:  return false;
			}
		}
	}

	std::uint8_t epSquare = NO_SQUARE;
	if (ep != "-")
	{
		if (ep.size() != 2) return false;
		const int f = ep[0] - 'a';
		const int r = ep[1] - '1';
		if (f < 0 || f > 7 || r < 0 || r > 7) return false;
		epSquare = MakeSquare(f, r);
	}

	if (side != "w" && side != "b") return false;

	Clear();
	std::memcpy(mBoard, board, sizeof(mBoard));
	mKingSq[0] = kingSq[0];
	mKingSq[1] = kingSq[1];
	mSide      = side == "w" ? White : Black;
	mCastling  = rights;
	mEp        = epSquare;
	mHalfmove  = std::uint16_t(std::max(0, halfmove));
	mFullmove  = std::uint16_t(std::max(1, fullmove));
	RecomputeKey();
	mKeyHistory.push_back(mKey);
	return true;
}

std::string ChessGame::Fen() const
{
	std::string out;
	for (int rank = 7; rank >= 0; --rank)
	{
		int empty = 0;
		for (int file = 0; file < 8; ++file)
		{
			const std::uint8_t encoded = mBoard[MakeSquare(file, rank)];
			if (!encoded)
			{
				++empty;
				continue;
			}
			if (empty)
			{
				out += char('0' + empty);
				empty = 0;
			}
			const char letter = PieceLetter(encoded & 0x07);
			out += ((encoded >> 3) & 1) == White ? letter
			                                     : char(std::tolower(static_cast<unsigned char>(letter)));
		}
		if (empty) out += char('0' + empty);
		if (rank) out += '/';
	}

	out += mSide == White ? " w " : " b ";
	if (!mCastling)
	{
		out += '-';
	}
	else
	{
		if (mCastling & CASTLE_WK) out += 'K';
		if (mCastling & CASTLE_WQ) out += 'Q';
		if (mCastling & CASTLE_BK) out += 'k';
		if (mCastling & CASTLE_BQ) out += 'q';
	}
	out += ' ';
	out += mEp == NO_SQUARE ? "-" : SquareName(mEp);
	out += ' ' + std::to_string(mHalfmove) + ' ' + std::to_string(mFullmove);
	return out;
}

bool ChessGame::IsSquareAttacked(std::uint8_t sq, Color by) const
{
	// pawns: step back along the attacker's capture directions
	const int pawnDir = by == White ? 16 : -16;
	for (const int side : { -1, 1 })
	{
		const int from = sq - pawnDir + side;
		if (!OnBoard(from)) continue;
		const std::uint8_t p = mBoard[from];
		if (p && ((p >> 3) & 1) == by && (p & 0x07) == Pawn) return true;
	}

	for (const int off : KNIGHT_OFFSETS)
	{
		const int from = sq + off;
		if (!OnBoard(from)) continue;
		const std::uint8_t p = mBoard[from];
		if (p && ((p >> 3) & 1) == by && (p & 0x07) == Knight) return true;
	}

	for (const int off : KING_OFFSETS)
	{
		const int from = sq + off;
		if (!OnBoard(from)) continue;
		const std::uint8_t p = mBoard[from];
		if (p && ((p >> 3) & 1) == by && (p & 0x07) == King) return true;
	}

	for (const int off : BISHOP_OFFSETS)
	{
		for (int from = sq + off; OnBoard(from); from += off)
		{
			const std::uint8_t p = mBoard[from];
			if (!p) continue;
			if (((p >> 3) & 1) == by)
			{
				const std::uint8_t type = p & 0x07;
				if (type == Bishop || type == Queen) return true;
			}
			break;
		}
	}

	for (const int off : ROOK_OFFSETS)
	{
		for (int from = sq + off; OnBoard(from); from += off)
		{
			const std::uint8_t p = mBoard[from];
			if (!p) continue;
			if (((p >> 3) & 1) == by)
			{
				const std::uint8_t type = p & 0x07;
				if (type == Rook || type == Queen) return true;
			}
			break;
		}
	}

	return false;
}

bool ChessGame::IsInCheck(Color c) const
{
	return mKingSq[c] != NO_SQUARE && IsSquareAttacked(mKingSq[c], Color(c ^ 1));
}

namespace
{
	// The cheapest piece of `by` that attacks `sq` on this board, or -1.
	// Works off a scratch board so the exchange can strip attackers one at a
	// time; a slider behind a removed slider is found on the next pass, which
	// is what gives the swap x-ray attackers for free.
	int LeastValuableAttacker(const std::uint8_t* board, int sq, int by)
	{
		const int pawnDir = by == ChessGame::White ? 16 : -16;
		for (const int side : { -1, 1 })
		{
			const int from = sq - pawnDir + side;
			if (!ChessGame::OnBoard(from)) continue;
			const std::uint8_t p = board[from];
			if (p && ((p >> 3) & 1) == by && (p & 0x07) == ChessGame::Pawn) return from;
		}

		for (const int off : KNIGHT_OFFSETS)
		{
			const int from = sq + off;
			if (!ChessGame::OnBoard(from)) continue;
			const std::uint8_t p = board[from];
			if (p && ((p >> 3) & 1) == by && (p & 0x07) == ChessGame::Knight) return from;
		}

		// bishops before rooks before queens: the swap wants them in value order
		for (const std::uint8_t want : { ChessGame::Bishop, ChessGame::Rook, ChessGame::Queen })
		{
			const bool diagonal = want != ChessGame::Rook;
			const bool straight  = want != ChessGame::Bishop;
			for (int dir = 0; dir < 8; ++dir)
			{
				const bool isDiagonal = dir < 4;
				if (isDiagonal ? !diagonal : !straight) continue;
				const int off = isDiagonal ? BISHOP_OFFSETS[dir] : ROOK_OFFSETS[dir - 4];
				for (int from = sq + off; ChessGame::OnBoard(from); from += off)
				{
					const std::uint8_t p = board[from];
					if (!p) continue;
					if (((p >> 3) & 1) == by && (p & 0x07) == want) return from;
					break;
				}
			}
		}

		for (const int off : KING_OFFSETS)
		{
			const int from = sq + off;
			if (!ChessGame::OnBoard(from)) continue;
			const std::uint8_t p = board[from];
			if (p && ((p >> 3) & 1) == by && (p & 0x07) == ChessGame::King) return from;
		}

		return -1;
	}

	bool AnyAttacker(const std::uint8_t* board, int sq, int by)
	{
		return LeastValuableAttacker(board, sq, by) >= 0;
	}
}

int ChessGame::See(const Move& m) const
{
	if (m.IsNull()) return 0;

	std::uint8_t board[128];
	std::memcpy(board, mBoard, sizeof(board));

	const int target = m.to;
	int gain[34];

	// what the first capture wins. En passant takes the pawn beside the
	// square, not the one on it.
	if (m.flags & MF_EN_PASSANT)
	{
		const int victim = target + (mSide == White ? -16 : 16);
		board[victim] = 0;
		gain[0] = PIECE_VALUE[Pawn];
	}
	else
	{
		gain[0] = PIECE_VALUE[board[target] & 0x07];
	}

	// the piece that will be standing on the square once the dust settles
	int onSquare = PIECE_VALUE[board[m.from] & 0x07];
	if (m.promo != NoPiece)
	{
		gain[0] += PIECE_VALUE[m.promo] - PIECE_VALUE[Pawn];
		onSquare = PIECE_VALUE[m.promo];
	}
	board[m.from] = 0;

	// then the two sides take turns on the square, cheapest piece first,
	// until one of them has nothing left to gain by continuing
	int side = mSide ^ 1;
	int d = 0;
	while (d + 1 < int(sizeof(gain) / sizeof(gain[0])))
	{
		const int from = LeastValuableAttacker(board, target, side);
		if (from < 0) break;
		const int attacker = board[from] & 0x07;
		board[from] = 0;
		if (attacker == King && AnyAttacker(board, target, side ^ 1)) break;

		++d;
		gain[d] = onSquare - gain[d - 1];
		// a pawn reaching the last rank recaptures as a queen
		const bool promotes = attacker == Pawn &&
			(RankOf(std::uint8_t(target)) == 7 || RankOf(std::uint8_t(target)) == 0);
		onSquare = promotes ? PIECE_VALUE[Queen] : PIECE_VALUE[attacker];
		if (promotes) gain[d] += PIECE_VALUE[Queen] - PIECE_VALUE[Pawn];
		// neither side is forced to continue a losing exchange
		if (std::max(-gain[d - 1], gain[d]) < 0) break;
		side ^= 1;
	}

	while (d > 0)
	{
		gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
		--d;
	}
	return gain[0];
}

bool ChessGame::LosesMaterial(const Move& m, int threshold)
{
	if (m.IsNull()) return false;
	if ((m.flags & MF_CAPTURE) && See(m) < -threshold) return true;

	if (!MakeMove(m)) return false;   // illegal moves are not our problem here
	Move replies[MAX_MOVES];
	const int n = GenerateLegalCaptures(replies);
	int worst = 0;
	for (int i = 0; i < n; ++i)
	{
		worst = std::max(worst, See(replies[i]));
	}
	Unmake();
	return worst > threshold;
}

void ChessGame::AddPawnMoves(Move* out, int& n, std::uint8_t from, std::uint8_t to,
                             std::uint8_t flags) const
{
	const int promoRank = mSide == White ? 7 : 0;
	if (RankOf(to) == promoRank)
	{
		for (const std::uint8_t promo : { Queen, Rook, Bishop, Knight })
		{
			out[n].from  = from;
			out[n].to    = to;
			out[n].promo = promo;
			out[n].flags = flags;
			++n;
		}
	}
	else
	{
		out[n].from  = from;
		out[n].to    = to;
		out[n].promo = NoPiece;
		out[n].flags = flags;
		++n;
	}
}

int ChessGame::GeneratePseudo(Move* out, bool capturesOnly) const
{
	int n = 0;
	const Color us   = mSide;
	const Color them = Color(us ^ 1);

	for (int from = 0; from < 128; ++from)
	{
		if (from & 0x88) continue;
		const std::uint8_t encoded = mBoard[from];
		if (!encoded || ((encoded >> 3) & 1) != us) continue;
		const std::uint8_t type = encoded & 0x07;

		switch (type)
		{
			case Pawn:
			{
				const int dir       = us == White ? 16 : -16;
				const int startRank = us == White ? 1 : 6;

				if (!capturesOnly)
				{
					const int one = from + dir;
					if (OnBoard(one) && !mBoard[one])
					{
						AddPawnMoves(out, n, std::uint8_t(from), std::uint8_t(one), 0);
						const int two = from + 2 * dir;
						if (RankOf(std::uint8_t(from)) == startRank && OnBoard(two) && !mBoard[two])
						{
							out[n].from  = std::uint8_t(from);
							out[n].to    = std::uint8_t(two);
							out[n].promo = NoPiece;
							out[n].flags = MF_DOUBLE_PUSH;
							++n;
						}
					}
				}

				for (const int side : { -1, 1 })
				{
					const int to = from + dir + side;
					if (!OnBoard(to)) continue;
					const std::uint8_t target = mBoard[to];
					if (target && ((target >> 3) & 1) == them)
					{
						AddPawnMoves(out, n, std::uint8_t(from), std::uint8_t(to), MF_CAPTURE);
					}
					else if (!target && mEp != NO_SQUARE && std::uint8_t(to) == mEp)
					{
						out[n].from  = std::uint8_t(from);
						out[n].to    = std::uint8_t(to);
						out[n].promo = NoPiece;
						out[n].flags = MF_CAPTURE | MF_EN_PASSANT;
						++n;
					}
				}
				break;
			}

			case Knight:
			case King:
			{
				const int* offsets = type == Knight ? KNIGHT_OFFSETS : KING_OFFSETS;
				for (int i = 0; i < 8; ++i)
				{
					const int to = from + offsets[i];
					if (!OnBoard(to)) continue;
					const std::uint8_t target = mBoard[to];
					if (target && ((target >> 3) & 1) == us) continue;
					if (capturesOnly && !target) continue;
					out[n].from  = std::uint8_t(from);
					out[n].to    = std::uint8_t(to);
					out[n].promo = NoPiece;
					out[n].flags = target ? MF_CAPTURE : 0;
					++n;
				}
				break;
			}

			default:
			{
				const int* offsets = type == Bishop ? BISHOP_OFFSETS
				                   : type == Rook   ? ROOK_OFFSETS
				                                    : KING_OFFSETS;
				const int count = type == Queen ? 8 : 4;
				for (int i = 0; i < count; ++i)
				{
					for (int to = from + offsets[i]; OnBoard(to); to += offsets[i])
					{
						const std::uint8_t target = mBoard[to];
						if (target && ((target >> 3) & 1) == us) break;
						if (!capturesOnly || target)
						{
							out[n].from  = std::uint8_t(from);
							out[n].to    = std::uint8_t(to);
							out[n].promo = NoPiece;
							out[n].flags = target ? MF_CAPTURE : 0;
							++n;
						}
						if (target) break;
					}
				}
				break;
			}
		}
	}

	if (!capturesOnly)
	{
		// castling: rights, empty path, and the king may not start, cross or
		// land on an attacked square
		const std::uint8_t kingSq = mKingSq[us];
		const std::uint8_t kingSide  = us == White ? CASTLE_WK : CASTLE_BK;
		const std::uint8_t queenSide = us == White ? CASTLE_WQ : CASTLE_BQ;
		const int backRank = us == White ? 0 : 7;

		if (kingSq == MakeSquare(4, backRank) && !IsSquareAttacked(kingSq, them))
		{
			if ((mCastling & kingSide) &&
			    !mBoard[MakeSquare(5, backRank)] && !mBoard[MakeSquare(6, backRank)] &&
			    !IsSquareAttacked(MakeSquare(5, backRank), them) &&
			    !IsSquareAttacked(MakeSquare(6, backRank), them))
			{
				out[n].from  = kingSq;
				out[n].to    = MakeSquare(6, backRank);
				out[n].promo = NoPiece;
				out[n].flags = MF_CASTLE;
				++n;
			}
			if ((mCastling & queenSide) &&
			    !mBoard[MakeSquare(3, backRank)] && !mBoard[MakeSquare(2, backRank)] &&
			    !mBoard[MakeSquare(1, backRank)] &&
			    !IsSquareAttacked(MakeSquare(3, backRank), them) &&
			    !IsSquareAttacked(MakeSquare(2, backRank), them))
			{
				out[n].from  = kingSq;
				out[n].to    = MakeSquare(2, backRank);
				out[n].promo = NoPiece;
				out[n].flags = MF_CASTLE;
				++n;
			}
		}
	}

	return n;
}

int ChessGame::GenerateLegal(Move* out)
{
	Move pseudo[MAX_MOVES];
	const int count = GeneratePseudo(pseudo, false);

	int n = 0;
	for (int i = 0; i < count; ++i)
	{
		if (!MakeMove(pseudo[i])) continue;  // MakeMove rejects self-check
		Unmake();
		out[n++] = pseudo[i];
	}
	return n;
}

int ChessGame::GenerateLegalCaptures(Move* out)
{
	Move pseudo[MAX_MOVES];
	const int count = GeneratePseudo(pseudo, true);

	int n = 0;
	for (int i = 0; i < count; ++i)
	{
		if (!MakeMove(pseudo[i])) continue;
		Unmake();
		out[n++] = pseudo[i];
	}
	return n;
}

bool ChessGame::MakeMove(const Move& m)
{
	const Color us   = mSide;
	const Color them = Color(us ^ 1);

	Undo undo;
	undo.move     = m;
	undo.castling = mCastling;
	undo.ep       = mEp;
	undo.halfmove = mHalfmove;
	undo.key      = mKey;
	undo.captured = 0;
	undo.pushedKey = false;

	const std::uint8_t moved = mBoard[m.from];
	const std::uint8_t type  = moved & 0x07;

	// lift the captured piece first - en passant takes from a different square
	std::uint8_t captureSq = m.to;
	if (m.flags & MF_EN_PASSANT)
	{
		captureSq = std::uint8_t(m.to + (us == White ? -16 : 16));
	}
	if (mBoard[captureSq])
	{
		undo.captured = mBoard[captureSq];
		RemovePiece(captureSq);
	}

	RemovePiece(m.from);
	PlacePiece(m.to, m.promo != NoPiece ? Encode(us, m.promo) : moved);

	if (m.flags & MF_CASTLE)
	{
		const int backRank = us == White ? 0 : 7;
		const bool kingSide = FileOf(m.to) == 6;
		const std::uint8_t rookFrom = MakeSquare(kingSide ? 7 : 0, backRank);
		const std::uint8_t rookTo   = MakeSquare(kingSide ? 5 : 3, backRank);
		const std::uint8_t rook     = mBoard[rookFrom];
		RemovePiece(rookFrom);
		PlacePiece(rookTo, rook);
	}

	// castling rights: king moves clear both, rook moves or captures clear one
	mKey ^= Keys().castling[mCastling];
	if (type == King) mCastling &= us == White ? ~(CASTLE_WK | CASTLE_WQ)
	                                           : ~(CASTLE_BK | CASTLE_BQ);
	const std::uint8_t A1 = MakeSquare(0, 0), H1 = MakeSquare(7, 0);
	const std::uint8_t A8 = MakeSquare(0, 7), H8 = MakeSquare(7, 7);
	for (const std::uint8_t sq : { m.from, m.to })
	{
		if (sq == H1) mCastling &= ~CASTLE_WK;
		if (sq == A1) mCastling &= ~CASTLE_WQ;
		if (sq == H8) mCastling &= ~CASTLE_BK;
		if (sq == A8) mCastling &= ~CASTLE_BQ;
	}
	mKey ^= Keys().castling[mCastling];

	if (mEp != NO_SQUARE) mKey ^= Keys().epFile[FileOf(mEp)];
	mEp = NO_SQUARE;
	if (m.flags & MF_DOUBLE_PUSH)
	{
		mEp = std::uint8_t(m.from + (us == White ? 16 : -16));
		mKey ^= Keys().epFile[FileOf(mEp)];
	}

	mHalfmove = (type == Pawn || undo.captured != 0) ? 0 : std::uint16_t(mHalfmove + 1);
	if (us == Black) ++mFullmove;

	mSide = them;
	mKey ^= Keys().side;

	mHistory.push_back(undo);

	if (IsInCheck(us))
	{
		Unmake();
		return false;
	}

	mKeyHistory.push_back(mKey);
	mHistory.back().pushedKey = true;
	return true;
}

void ChessGame::Unmake()
{
	if (mHistory.empty()) return;
	const Undo undo = mHistory.back();
	mHistory.pop_back();
	if (undo.pushedKey && !mKeyHistory.empty()) mKeyHistory.pop_back();

	const Move& m    = undo.move;
	const Color us   = Color(mSide ^ 1);   // the side that made the move

	const std::uint8_t landed = mBoard[m.to];
	mBoard[m.to] = 0;
	// promotions go back as pawns
	mBoard[m.from] = m.promo != NoPiece ? Encode(us, Pawn) : landed;
	if ((mBoard[m.from] & 0x07) == King) mKingSq[us] = m.from;

	if (undo.captured)
	{
		const std::uint8_t captureSq =
			(m.flags & MF_EN_PASSANT) ? std::uint8_t(m.to + (us == White ? -16 : 16)) : m.to;
		mBoard[captureSq] = undo.captured;
	}

	if (m.flags & MF_CASTLE)
	{
		const int backRank  = us == White ? 0 : 7;
		const bool kingSide = FileOf(m.to) == 6;
		const std::uint8_t rookFrom = MakeSquare(kingSide ? 7 : 0, backRank);
		const std::uint8_t rookTo   = MakeSquare(kingSide ? 5 : 3, backRank);
		mBoard[rookFrom] = mBoard[rookTo];
		mBoard[rookTo]   = 0;
	}

	mCastling = undo.castling;
	mEp       = undo.ep;
	mHalfmove = undo.halfmove;
	mKey      = undo.key;
	mSide     = us;
	if (us == Black) --mFullmove;
}

ChessGame::Result ChessGame::GetResult()
{
	Move moves[MAX_MOVES];
	const int count = GenerateLegal(moves);
	if (count == 0)
	{
		if (IsInCheck(mSide)) return mSide == White ? Result::BlackMates : Result::WhiteMates;
		return Result::Stalemate;
	}
	if (mHalfmove >= 100) return Result::DrawFiftyMove;

	// Only positions since the last irreversible move can repeat, and only
	// every second ply has the same side to move.
	const int n = int(mKeyHistory.size());
	int repeats = 1;
	const int limit = std::min(int(mHalfmove), n - 1);
	for (int back = 2; back <= limit; back += 2)
	{
		if (mKeyHistory[n - 1 - back] == mKey) ++repeats;
	}
	if (repeats >= 3) return Result::DrawRepetition;

	// insufficient material: lone kings, or a king plus a single minor
	int minors[2] = { 0, 0 };
	bool heavyOrPawn = false;
	for (int sq = 0; sq < 128 && !heavyOrPawn; ++sq)
	{
		if (sq & 0x88) continue;
		const std::uint8_t p = mBoard[sq];
		if (!p) continue;
		const std::uint8_t type = p & 0x07;
		if (type == Pawn || type == Rook || type == Queen) heavyOrPawn = true;
		else if (type == Knight || type == Bishop) ++minors[(p >> 3) & 1];
	}
	if (!heavyOrPawn && minors[White] <= 1 && minors[Black] <= 1) return Result::DrawInsufficient;

	return Result::Ongoing;
}

int ChessGame::Evaluate() const
{
	int score = 0;
	for (int sq = 0; sq < 128; ++sq)
	{
		if (sq & 0x88) continue;
		const std::uint8_t p = mBoard[sq];
		if (!p) continue;
		const std::uint8_t type = p & 0x07;
		const Color c = Color((p >> 3) & 1);

		const int file = FileOf(std::uint8_t(sq));
		const int rank = RankOf(std::uint8_t(sq));
		// tables are written rank 8 first, so White reads them flipped
		const int index = c == White ? (7 - rank) * 8 + file : rank * 8 + file;
		const int value = PIECE_VALUE[type] + PST_FOR[type][index];

		score += c == White ? value : -value;
	}

	score += MopUp();
	return score;
}

// Material and piece-square tables say nothing about how to finish a won
// endgame: with a rook up and the board otherwise bare, every legal move
// scores the same and the rook shuffles until the fifty-move rule saves the
// defender. This is the standard mop-up term - drive the bare king to the
// edge, and walk your own king in to take the squares away from him - which
// is what turns "winning" into a ladder mate.
int ChessGame::MopUp() const
{
	int material[2] = { 0, 0 };
	int pieces[2]   = { 0, 0 };   // anything that is not a king
	int pawns[2]    = { 0, 0 };
	for (int sq = 0; sq < 128; ++sq)
	{
		if (sq & 0x88) continue;
		const std::uint8_t p = mBoard[sq];
		if (!p) continue;
		const std::uint8_t type = p & 0x07;
		if (type == King) continue;
		const int c = (p >> 3) & 1;
		material[c] += PIECE_VALUE[type];
		++pieces[c];
		if (type == Pawn) ++pawns[c];
	}

	// the winner needs enough to mate with, the loser has to be down to a
	// bare king - with a pawn on the board this is an endgame, not a mate net
	const int strong = material[White] > material[Black] ? White : Black;
	const int weak   = strong ^ 1;
	if (pieces[weak] != 0) return 0;
	if (pawns[strong] != 0) return 0;
	if (material[strong] - material[weak] < PIECE_VALUE[Rook]) return 0;

	const std::uint8_t wk = mKingSq[weak];
	const std::uint8_t sk = mKingSq[strong];
	if (wk == NO_SQUARE || sk == NO_SQUARE) return 0;

	// how far the bare king is from the middle, and how close the other king
	// has walked: the two halves of every mating technique there is. The
	// weights are the usual ones, and they are deliberately large - the
	// king's middlegame table wants him home, and in this position it is
	// wrong, so the mop-up has to outvote it.
	const int wf = FileOf(wk), wr = RankOf(wk);
	const int centre = (wf < 3 ? 3 - wf : wf > 4 ? wf - 4 : 0) +
	                   (wr < 3 ? 3 - wr : wr > 4 ? wr - 4 : 0);
	const int between = std::abs(wf - FileOf(sk)) + std::abs(wr - RankOf(sk));

	// and the part a three-ply search cannot see for itself: how much room
	// the bare king has left. Rewarding the shrinking box is what a rook
	// cutting off a rank amounts to, and without it the mate does not arrive
	// inside fifty moves.
	int room = 0;
	for (const int off : KING_OFFSETS)
	{
		const int to = wk + off;
		if (!OnBoard(to)) continue;
		const std::uint8_t occupant = mBoard[to];
		if (occupant && ((occupant >> 3) & 1) == weak) continue;
		if (IsSquareAttacked(std::uint8_t(to), Color(strong))) continue;
		++room;
	}

	const int bonus = 47 * centre + 16 * (14 - between) - 30 * room;
	return strong == White ? bonus : -bonus;
}

int ChessGame::Quiesce(int alpha, int beta)
{
	const int standPat = mSide == White ? Evaluate() : -Evaluate();
	if (standPat >= beta) return beta;
	if (standPat > alpha) alpha = standPat;

	Move moves[MAX_MOVES];
	const int count = GenerateLegalCaptures(moves);
	// most-valuable-victim first: cheap ordering, big cutoff win
	std::sort(moves, moves + count, [this](const Move& a, const Move& b) {
		return PIECE_VALUE[PieceAt(a.to)] > PIECE_VALUE[PieceAt(b.to)];
	});

	for (int i = 0; i < count; ++i)
	{
		if (!MakeMove(moves[i])) continue;
		const int score = -Quiesce(-beta, -alpha);
		Unmake();
		if (score >= beta) return beta;
		if (score > alpha) alpha = score;
	}
	return alpha;
}

// Has this position stood on the board before, within the current
// irreversible span? One earlier occurrence is enough for the search: a side
// that is winning must not be allowed to think a repetition costs nothing,
// which is how a rook ending turns into a shuffle.
bool ChessGame::IsRepetition() const
{
	if (mKeyHistory.empty()) return false;
	const int span = std::min<int>(mHalfmove, int(mKeyHistory.size()) - 1);
	for (int back = 2; back <= span; back += 2)
	{
		if (mKeyHistory[mKeyHistory.size() - 1 - back] == mKey) return true;
	}
	return false;
}

int ChessGame::Negamax(int depth, int alpha, int beta)
{
	if (IsRepetition()) return 0;
	if (depth <= 0) return Quiesce(alpha, beta);

	Move moves[MAX_MOVES];
	const int count = GenerateLegal(moves);
	if (count == 0)
	{
		// Mate scores carry their distance: more remaining depth means the mate
		// lands sooner, so it must score worse for the side being mated and
		// better for the attacker once negated up the tree.
		return IsInCheck(mSide) ? -MATE_SCORE - depth : 0;
	}
	if (mHalfmove >= 100) return 0;

	std::sort(moves, moves + count, [this](const Move& a, const Move& b) {
		return PIECE_VALUE[PieceAt(a.to)] > PIECE_VALUE[PieceAt(b.to)];
	});

	for (int i = 0; i < count; ++i)
	{
		if (!MakeMove(moves[i])) continue;
		const int score = -Negamax(depth - 1, -beta, -alpha);
		Unmake();
		if (score >= beta) return beta;
		if (score > alpha) alpha = score;
	}
	return alpha;
}

ChessGame::Move ChessGame::Search(int depth, int errorPercent, std::uint32_t& seed)
{
	Move moves[MAX_MOVES];
	const int count = GenerateLegal(moves);
	if (count == 0) return Move();

	std::mt19937 rng(seed);
	seed = rng();

	// a weaker opponent throws away its turn some of the time - this is the
	// whole rating spread across the bot ladder
	if (errorPercent > 0 && int(rng() % 100) < errorPercent)
	{
		return moves[rng() % unsigned(count)];
	}

	int bestScore = -MATE_SCORE * 2;
	std::vector<Move> best;
	for (int i = 0; i < count; ++i)
	{
		if (!MakeMove(moves[i])) continue;
		const int score = -Negamax(depth - 1, -MATE_SCORE * 2, MATE_SCORE * 2);
		Unmake();
		if (score > bestScore)
		{
			bestScore = score;
			best.clear();
			best.push_back(moves[i]);
		}
		else if (score == bestScore)
		{
			best.push_back(moves[i]);
		}
	}
	if (best.empty()) return moves[0];
	// break ties at random so the bots do not replay the same game every night
	return best[rng() % best.size()];
}

std::string ChessGame::Uci(const Move& m) const
{
	if (m.IsNull()) return "0000";
	std::string s = SquareName(m.from) + SquareName(m.to);
	if (m.promo != NoPiece)
	{
		s += char(std::tolower(static_cast<unsigned char>(PieceLetter(m.promo))));
	}
	return s;
}

ChessGame::Move ChessGame::ParseUci(const std::string& uci)
{
	if (uci.size() < 4) return Move();
	const int ff = uci[0] - 'a', fr = uci[1] - '1';
	const int tf = uci[2] - 'a', tr = uci[3] - '1';
	if (ff < 0 || ff > 7 || fr < 0 || fr > 7 || tf < 0 || tf > 7 || tr < 0 || tr > 7) return Move();

	const std::uint8_t from = MakeSquare(ff, fr);
	const std::uint8_t to   = MakeSquare(tf, tr);
	const std::uint8_t promo = uci.size() > 4 ? LetterPiece(uci[4]) : NoPiece;

	Move moves[MAX_MOVES];
	const int count = GenerateLegal(moves);
	for (int i = 0; i < count; ++i)
	{
		if (moves[i].from == from && moves[i].to == to && moves[i].promo == promo) return moves[i];
	}
	return Move();
}

std::string ChessGame::San(const Move& m)
{
	if (m.IsNull()) return "--";

	const std::uint8_t type = PieceAt(m.from);
	std::string san;

	if (m.flags & MF_CASTLE)
	{
		san = FileOf(m.to) == 6 ? "O-O" : "O-O-O";
	}
	else if (type == Pawn)
	{
		if (m.flags & MF_CAPTURE)
		{
			san += char('a' + FileOf(m.from));
			san += 'x';
		}
		san += SquareName(m.to);
		if (m.promo != NoPiece)
		{
			san += '=';
			san += PieceLetter(m.promo);
		}
	}
	else
	{
		san += PieceLetter(type);

		// disambiguate against other same-type pieces that can also reach m.to
		Move moves[MAX_MOVES];
		const int count = GenerateLegal(moves);
		bool sameFile = false, sameRank = false, ambiguous = false;
		for (int i = 0; i < count; ++i)
		{
			if (moves[i].to != m.to || moves[i].from == m.from) continue;
			if (PieceAt(moves[i].from) != type) continue;
			ambiguous = true;
			if (FileOf(moves[i].from) == FileOf(m.from)) sameFile = true;
			if (RankOf(moves[i].from) == RankOf(m.from)) sameRank = true;
		}
		if (ambiguous)
		{
			if (!sameFile)      san += char('a' + FileOf(m.from));
			else if (!sameRank) san += char('1' + RankOf(m.from));
			else                san += SquareName(m.from);
		}

		if (m.flags & MF_CAPTURE) san += 'x';
		san += SquareName(m.to);
	}

	// check and mate suffixes need the resulting position
	if (MakeMove(m))
	{
		Move replies[MAX_MOVES];
		const int count = GenerateLegal(replies);
		if (IsInCheck(mSide)) san += count == 0 ? '#' : '+';
		Unmake();
	}
	return san;
}

std::uint64_t ChessGame::Perft(int depth)
{
	if (depth == 0) return 1;

	Move moves[MAX_MOVES];
	const int count = GeneratePseudo(moves, false);
	std::uint64_t nodes = 0;
	for (int i = 0; i < count; ++i)
	{
		if (!MakeMove(moves[i])) continue;
		nodes += depth == 1 ? 1 : Perft(depth - 1);
		Unmake();
	}
	return nodes;
}
