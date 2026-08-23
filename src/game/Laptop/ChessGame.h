#ifndef CHESSGAME_H
#define CHESSGAME_H

// Engine-free chess core for the chach.com laptop minigame.
//
// 0x88 board representation: squares are rank * 16 + file, so a square is off
// the board exactly when (sq & 0x88) is non-zero. That makes slider and knight
// bounds checks a single mask instead of a pair of comparisons.
//
// Move generation is pseudo-legal followed by make / king-attacked / unmake,
// which is slower than pin-aware generation but short enough to read and to
// trust. Perft is the proof: see ChessGame_unittest.cc.
//
// No JA2 headers on purpose: everything here is deterministic given the seed
// passed to the search, so the whole engine is unit-testable.

#include <cstdint>
#include <string>
#include <vector>

class ChessGame
{
public:
	enum Piece : std::uint8_t
	{
		NoPiece = 0, Pawn, Knight, Bishop, Rook, Queen, King
	};

	enum Color : std::uint8_t { White = 0, Black = 1 };

	static constexpr std::uint8_t NO_SQUARE  = 0x7F;
	static constexpr int MAX_MOVES           = 256;

	// castling rights bitmask
	static constexpr std::uint8_t CASTLE_WK = 0x01;
	static constexpr std::uint8_t CASTLE_WQ = 0x02;
	static constexpr std::uint8_t CASTLE_BK = 0x04;
	static constexpr std::uint8_t CASTLE_BQ = 0x08;

	// move flags
	static constexpr std::uint8_t MF_CAPTURE     = 0x01;
	static constexpr std::uint8_t MF_DOUBLE_PUSH = 0x02;
	static constexpr std::uint8_t MF_EN_PASSANT  = 0x04;
	static constexpr std::uint8_t MF_CASTLE      = 0x08;

	struct Move
	{
		std::uint8_t from  = NO_SQUARE;
		std::uint8_t to    = NO_SQUARE;
		std::uint8_t promo = NoPiece;   // Queen / Rook / Bishop / Knight, else NoPiece
		std::uint8_t flags = 0;

		bool IsNull() const { return from == NO_SQUARE; }
		bool operator==(const Move& o) const
		{
			return from == o.from && to == o.to && promo == o.promo;
		}
	};

	enum class Result : std::uint8_t
	{
		Ongoing,
		WhiteMates,
		BlackMates,
		Stalemate,
		DrawFiftyMove,
		DrawRepetition,
		DrawInsufficient,
	};

	ChessGame() { SetStartPosition(); }

	void SetStartPosition();
	// Accepts standard FEN. Returns false and leaves the position untouched on
	// anything malformed - puzzle corpora are loaded through here.
	bool SetFen(const std::string& fen);
	std::string Fen() const;

	// board access
	std::uint8_t PieceAt(std::uint8_t sq) const { return mBoard[sq] & 0x07; }
	Color        ColorAt(std::uint8_t sq) const { return Color((mBoard[sq] >> 3) & 1); }
	bool         IsEmpty(std::uint8_t sq) const { return mBoard[sq] == 0; }
	Color        SideToMove() const             { return mSide; }
	std::uint8_t CastlingRights() const         { return mCastling; }
	std::uint8_t EnPassantSquare() const        { return mEp; }
	int          HalfmoveClock() const          { return mHalfmove; }
	int          FullmoveNumber() const         { return mFullmove; }
	std::uint8_t KingSquare(Color c) const      { return mKingSq[c]; }

	// generation and play. Generation is not const: legality is decided by
	// making the move, testing the king, and unmaking. Copying the board per
	// candidate instead would cost more than the whole search budget.
	int  GenerateLegal(Move* out);
	int  GenerateLegalCaptures(Move* out);
	bool MakeMove(const Move& m);
	void Unmake();
	bool IsInCheck(Color c) const;
	bool IsSquareAttacked(std::uint8_t sq, Color by) const;

	// Static exchange evaluation, in centipawns: what the side to move nets by
	// playing this capture and letting both sides trade off on the square until
	// neither wants to continue. Negative means the capture loses material -
	// queen takes defended pawn is roughly -800. Quiet moves score 0.
	int See(const Move& m) const;

	// True when the move gives material away, either because the capture
	// itself loses the exchange or because it parks a piece where the reply
	// wins it. `threshold` is how many centipawns of loss to tolerate, so 90
	// permits a pawn sacrifice and refuses anything larger.
	// Not const: it makes and unmakes the move to look at the reply.
	bool LosesMaterial(const Move& m, int threshold);

	Result GetResult();
	static bool IsDraw(Result r)
	{
		return r == Result::Stalemate || r == Result::DrawFiftyMove ||
		       r == Result::DrawRepetition || r == Result::DrawInsufficient;
	}

	// search: negamax with alpha-beta and a quiescence tail.
	// errorPercent models a weaker opponent - that share of the time it plays a
	// random legal move instead of the best one, which is what gives the bot
	// ladder its rating spread.
	// Not const: the search makes and unmakes moves on this board. It always
	// restores the position before returning.
	Move Search(int depth, int errorPercent, std::uint32_t& seed);

	// The budgeted search: iterative deepening under a wall-clock or node
	// cap, with a shared transposition table, ordering heuristics and a
	// tapered evaluation. Search() above is a thin wrapper over this.
	struct SearchParams
	{
		int           maxDepth     = 64;
		int           msBudget     = 0;  // 0 = no wall-clock limit
		std::uint64_t nodeBudget   = 0;  // 0 = unlimited; deterministic cap
		int           errorPercent = 0;
		bool          useTT        = true;
	};
	struct SearchResult
	{
		Move          move;
		int           score = 0;  // centipawns, side-to-move's view
		int           depth = 0;  // deepest fully completed iteration
		std::uint64_t nodes = 0;
	};
	SearchResult SearchTimed(const SearchParams& p, std::uint32_t& seed);
	// wipes the shared transposition table; tests use it between runs
	static void ClearHash();
	// Static evaluation in centipawns, positive meaning good for White.
	int Evaluate() const;
	// The endgame half of it, exposed for the tests: zero in any position
	// that is not "one side is bare, the other can mate".
	int MopUp() const;

	// notation
	std::string San(const Move& m);              // needs the move to be legal here
	std::string Uci(const Move& m) const;
	Move        ParseUci(const std::string& uci);        // null Move if illegal

	std::uint64_t Perft(int depth);
	std::uint64_t ZobristKey() const { return mKey; }

	// squares
	static std::uint8_t MakeSquare(int file, int rank) { return std::uint8_t(rank * 16 + file); }
	static int  FileOf(std::uint8_t sq)  { return sq & 7; }
	static int  RankOf(std::uint8_t sq)  { return sq >> 4; }
	static bool OnBoard(int sq)          { return (sq & 0x88) == 0; }

private:
	struct Undo
	{
		Move         move;
		std::uint8_t captured;   // encoded piece, 0 if none
		std::uint8_t castling;
		std::uint8_t ep;
		std::uint16_t halfmove;
		std::uint64_t key;
		bool         pushedKey;  // false when the move was rejected as illegal
	};

	// encoded board: 0 empty, else (color << 3) | type
	std::uint8_t mBoard[128] = {};
	Color        mSide     = White;
	std::uint8_t mCastling = 0;
	std::uint8_t mEp       = NO_SQUARE;
	std::uint16_t mHalfmove = 0;
	std::uint16_t mFullmove = 1;
	std::uint8_t mKingSq[2] = { NO_SQUARE, NO_SQUARE };
	std::uint64_t mKey      = 0;

	std::vector<Undo>          mHistory;
	// One key per made move plus the starting position. Never cleared, so an
	// Unmake can always restore it; repetition scans stop at the halfmove
	// clock, which is what bounds them to the current irreversible span.
	std::vector<std::uint64_t> mKeyHistory;

	static std::uint8_t Encode(Color c, std::uint8_t type) { return std::uint8_t((c << 3) | type); }

	void Clear();
	void PlacePiece(std::uint8_t sq, std::uint8_t encoded);
	void RemovePiece(std::uint8_t sq);
	void RecomputeKey();

	int  GeneratePseudo(Move* out, bool capturesOnly) const;
	void AddPawnMoves(Move* out, int& n, std::uint8_t from, std::uint8_t to, std::uint8_t flags) const;

	// true when this exact position already stood on the board in the
	// current irreversible span
	bool IsRepetition() const;

	int  Quiesce(int alpha, int beta);
	int  Negamax(int depth, int ply, int alpha, int beta, bool allowNull);
	// the null move: pass the turn, keys kept honest, ep suspended
	void DoNull(std::uint8_t& epSave);
	void UndoNull(std::uint8_t epSave);
	bool HasNonPawn(Color c) const;
};

#endif
