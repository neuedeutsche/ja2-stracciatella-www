#ifndef CHESSLESSONS_H
#define CHESSLESSONS_H

// Grunty's book, such as it is: a diagram and three lines per lesson. The
// positions are validated in ChessGame_unittest - each lesson must actually
// show what its text claims.
struct ChessLesson
{
	const char* title;
	const char* fen;
	const char* lines[3];
	const char* answer;  // the move the lesson is about, UCI
	const char* answer2; // an equally correct second door, or nullptr
	const char* deny;    // what ze coach says to anything else
};

constexpr int CHESS_LESSON_COUNT = 8;
extern const ChessLesson CHESS_LESSONS[CHESS_LESSON_COUNT];

#endif
