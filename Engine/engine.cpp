#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstring>

enum Piece { EMPTY = 0, P = 1, N = 2, B = 3, R = 4, Q = 5, K = 6,
                        p = -1, n = -2, b = -3, r = -4, q = -5, k = -6 };

struct Move {
    int fromRow, fromCol, toRow, toCol;

    bool operator==(const Move& o) const {
        return fromRow == o.fromRow && fromCol == o.fromCol &&
               toRow == o.toRow && toCol == o.toCol;
    }

    std::string toUCI(int pType = 0) const {
        if (fromRow == -1) return "0000";
        char fC = 'a' + fromCol, fR = '8' - fromRow;
        char tC = 'a' + toCol, tR = '8' - toRow;
        std::string uci = {fC, fR, tC, tR};
        if (pType == 1 && (toRow == 0 || toRow == 7)) uci += "q";
        return uci;
    }
};

// Piece-Square Tables (White perspective; mirrored for Black)
const int mg_pawnTable[8][8] = {
    {  0,   0,   0,   0,   0,   0,   0,   0},
    { 50,  50,  50,  50,  50,  50,  50,  50},
    { 10,  10,  20,  30,  30,  20,  10,  10},
    {  5,   5,  10,  25,  25,  10,   5,   5},
    {  0,   0,   0,  20,  20,   0,   0,   0},
    {  5,  -5, -10,   0,   0, -10,  -5,   5},
    {  5,  10,  10, -20, -20,  10,  10,   5},
    {  0,   0,   0,   0,   0,   0,   0,   0}
};

const int eg_pawnTable[8][8] = {
    {  0,   0,   0,   0,   0,   0,   0,   0},
    { 80,  80,  80,  80,  80,  80,  80,  80},
    { 50,  50,  50,  50,  50,  50,  50,  50},
    { 30,  30,  30,  30,  30,  30,  30,  30},
    { 15,  15,  15,  15,  15,  15,  15,  15},
    {  5,   5,   5,   5,   5,   5,   5,   5},
    {  0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0}
};

const int mg_knightTable[8][8] = {
    {-50, -40, -30, -30, -30, -30, -40, -50},
    {-40, -20,   0,   5,   5,   0, -20, -40},
    {-30,   5,  15,  20,  20,  15,   5, -30},
    {-30,   0,  15,  25,  25,  15,   0, -30},
    {-30,   5,  15,  25,  25,  15,   5, -30},
    {-30,   0,  10,  15,  15,  10,   0, -30},
    {-40, -20,   0,   0,   0,   0, -20, -40},
    {-50, -40, -30, -30, -30, -30, -40, -50}
};

const int mg_bishopTable[8][8] = {
    {-20, -10, -10, -10, -10, -10, -10, -20},
    {-10,   5,   0,   0,   0,   0,   5, -10},
    {-10,  10,  10,  10,  10,  10,  10, -10},
    {-10,   0,  10,  15,  15,  10,   0, -10},
    {-10,   5,   5,  15,  15,   5,   5, -10},
    {-10,   0,   5,  10,  10,   5,   0, -10},
    {-10,   0,   0,   0,   0,   0,   0, -10},
    {-20, -10, -10, -10, -10, -10, -10, -20}
};

const int mg_rookTable[8][8] = {
    {  0,   0,   0,   5,   5,   0,   0,   0},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    {  5,  10,  10,  10,  10,  10,  10,   5},
    {  0,   0,   0,   0,   0,   0,   0,   0}
};

const int mg_queenTable[8][8] = {
    {-20, -10, -10,  -5,  -5, -10, -10, -20},
    {-10,   0,   5,   0,   0,   0,   0, -10},
    {-10,   5,   5,   5,   5,   5,   0, -10},
    {  0,   0,   5,   5,   5,   5,   0,  -5},
    { -5,   0,   5,   5,   5,   5,   0,  -5},
    {-10,   0,   5,   5,   5,   5,   0, -10},
    {-10,   0,   0,   0,   0,   0,   0, -10},
    {-20, -10, -10,  -5,  -5, -10, -10, -20}
};

const int mg_kingTable[8][8] = {
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-20, -30, -30, -40, -40, -30, -30, -20},
    {-10, -20, -20, -20, -20, -20, -20, -10},
    { 20,  20,   0,   0,   0,   0,  20,  20},
    { 20,  30,  10,   0,   0,  10,  30,  20}
};

const int eg_kingTable[8][8] = {
    {-50, -40, -30, -20, -20, -30, -40, -50},
    {-30, -20, -10,   0,   0, -10, -20, -30},
    {-30, -10,  20,  30,  30,  20, -10, -30},
    {-30, -10,  30,  40,  40,  30, -10, -30},
    {-30, -10,  30,  40,  40,  30, -10, -30},
    {-30, -10,  20,  30,  30,  20, -10, -30},
    {-30, -30,   0,   0,   0,   0, -30, -30},
    {-50, -30, -30, -30, -30, -30, -30, -50}
};

static const int pieceValues[7] = {0, 100, 320, 335, 500, 900, 20000};

// Search control & heuristics
const int TIME_LIMIT_MS = 2000; // 2.0 seconds max per move
std::chrono::steady_clock::time_point searchStartTime;
bool searchAborted = false;
long long nodesSearched = 0;

Move killerMoves[64][2];
int historyTable[2][64][64];

inline void checkTime() {
    if ((++nodesSearched & 1023) == 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStartTime).count();
        if (elapsed >= TIME_LIMIT_MS) {
            searchAborted = true;
        }
    }
}

class Board {
public:
    int squares[8][8];
    bool whiteTurn = true;

    Board() { reset(); }

    void reset() {
        for (int r = 0; r < 8; ++r)
            for (int c = 0; c < 8; ++c) squares[r][c] = EMPTY;
    }

    void loadFEN(const std::string& fen) {
        reset();
        std::istringstream ss(fen);
        std::string placement, turn;
        ss >> placement >> turn;
        whiteTurn = (turn == "w");

        int r = 0, c = 0;
        for (char ch : placement) {
            if (ch == '/') { r++; c = 0; }
            else if (isdigit(ch)) { c += (ch - '0'); }
            else {
                int piece = EMPTY;
                switch (ch) {
                    case 'P': piece = P; break; case 'N': piece = N; break;
                    case 'B': piece = B; break; case 'R': piece = R; break;
                    case 'Q': piece = Q; break; case 'K': piece = K; break;
                    case 'p': piece = p; break; case 'n': piece = n; break;
                    case 'b': piece = b; break; case 'r': piece = r; break;
                    case 'q': piece = q; break; case 'k': piece = k; break;
                }
                squares[r][c++] = piece;
            }
        }
    }

    inline bool inBounds(int r, int c) const { return (unsigned)r < 8 && (unsigned)c < 8; }

    // Zero-heap-overhead move generation
    std::vector<Move> generatePseudoMoves(bool capturesOnly = false) const {
        std::vector<Move> moves;
        moves.reserve(capturesOnly ? 16 : 48);

        static const int drN[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
        static const int dcN[8] = {-1, 1, -2, 2, -2, 2, -1, 1};
        static const int drDiag[4] = {-1, -1, 1, 1};
        static const int dcDiag[4] = {-1, 1, -1, 1};
        static const int drOrtho[4] = {-1, 1, 0, 0};
        static const int dcOrtho[4] = {0, 0, -1, 1};

        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int piece = squares[r][c];
                if (piece == EMPTY || (whiteTurn && piece < 0) || (!whiteTurn && piece > 0))
                    continue;

                int pType = std::abs(piece);
                if (pType == 1) { // Pawn
                    int dir = (piece > 0) ? -1 : 1;
                    int startRow = (piece > 0) ? 6 : 1;
                    int nextR = r + dir;
                    if (inBounds(nextR, c) && squares[nextR][c] == EMPTY) {
                        bool isPromo = (nextR == 0 || nextR == 7);
                        if (!capturesOnly || isPromo) {
                            moves.push_back({r, c, nextR, c});
                        }
                        if (!capturesOnly && r == startRow && squares[r + 2 * dir][c] == EMPTY) {
                            moves.push_back({r, c, r + 2 * dir, c});
                        }
                    }
                    for (int dc : {-1, 1}) {
                        int nc = c + dc;
                        if (inBounds(nextR, nc)) {
                            int target = squares[nextR][nc];
                            if ((piece > 0 && target < 0) || (piece < 0 && target > 0))
                                moves.push_back({r, c, nextR, nc});
                        }
                    }
                } else if (pType == 2) { // Knight
                    for (int i = 0; i < 8; ++i) {
                        int nr = r + drN[i], nc = c + dcN[i];
                        if (inBounds(nr, nc)) {
                            int target = squares[nr][nc];
                            if (target == EMPTY) {
                                if (!capturesOnly) moves.push_back({r, c, nr, nc});
                            } else if ((piece > 0 && target < 0) || (piece < 0 && target > 0)) {
                                moves.push_back({r, c, nr, nc});
                            }
                        }
                    }
                } else { // Bishops, Rooks, Queens, Kings
                    int maxSteps = (pType == 6) ? 1 : 7;
                    if (pType == 3 || pType == 5 || pType == 6) {
                        for (int d = 0; d < 4; ++d) {
                            for (int step = 1; step <= maxSteps; ++step) {
                                int nr = r + drDiag[d] * step, nc = c + dcDiag[d] * step;
                                if (!inBounds(nr, nc)) break;
                                int target = squares[nr][nc];
                                if (target == EMPTY) {
                                    if (!capturesOnly) moves.push_back({r, c, nr, nc});
                                } else {
                                    if ((piece > 0 && target < 0) || (piece < 0 && target > 0))
                                        moves.push_back({r, c, nr, nc});
                                    break;
                                }
                            }
                        }
                    }
                    if (pType == 4 || pType == 5 || pType == 6) {
                        for (int d = 0; d < 4; ++d) {
                            for (int step = 1; step <= maxSteps; ++step) {
                                int nr = r + drOrtho[d] * step, nc = c + dcOrtho[d] * step;
                                if (!inBounds(nr, nc)) break;
                                int target = squares[nr][nc];
                                if (target == EMPTY) {
                                    if (!capturesOnly) moves.push_back({r, c, nr, nc});
                                } else {
                                    if ((piece > 0 && target < 0) || (piece < 0 && target > 0))
                                        moves.push_back({r, c, nr, nc});
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        return moves;
    }

    void makeMove(const Move& m, int& captured, int& movedPiece) {
        captured = squares[m.toRow][m.toCol];
        movedPiece = squares[m.fromRow][m.fromCol];
        int placedPiece = movedPiece;

        if (std::abs(movedPiece) == 1 && (m.toRow == 0 || m.toRow == 7)) {
            placedPiece = (movedPiece > 0) ? Q : q;
        }

        squares[m.toRow][m.toCol] = placedPiece;
        squares[m.fromRow][m.fromCol] = EMPTY;
        whiteTurn = !whiteTurn;
    }

    void unmakeMove(const Move& m, int captured, int movedPiece) {
        squares[m.fromRow][m.fromCol] = movedPiece;
        squares[m.toRow][m.toCol] = captured;
        whiteTurn = !whiteTurn;
    }

    bool isSquareAttacked(int sqR, int sqC, bool byWhite) const {
        int enemyP = byWhite ? P : p;
        int enemyN = byWhite ? N : n;
        int enemyB = byWhite ? B : b;
        int enemyR = byWhite ? R : r;
        int enemyQ = byWhite ? Q : q;
        int enemyK = byWhite ? K : k;

        int pDir = byWhite ? 1 : -1;
        if (inBounds(sqR + pDir, sqC - 1) && squares[sqR + pDir][sqC - 1] == enemyP) return true;
        if (inBounds(sqR + pDir, sqC + 1) && squares[sqR + pDir][sqC + 1] == enemyP) return true;

        static const int drN[] = {-2, -2, -1, -1, 1, 1, 2, 2};
        static const int dcN[] = {-1, 1, -2, 2, -2, 2, -1, 1};
        for (int i = 0; i < 8; i++) {
            int r = sqR + drN[i], c = sqC + dcN[i];
            if (inBounds(r, c) && squares[r][c] == enemyN) return true;
        }

        static const int drK[] = {-1, -1, -1, 0, 0, 1, 1, 1};
        static const int dcK[] = {-1, 0, 1, -1, 1, -1, 0, 1};
        for (int i = 0; i < 8; i++) {
            int r = sqR + drK[i], c = sqC + dcK[i];
            if (inBounds(r, c) && squares[r][c] == enemyK) return true;
        }

        static const int drS[] = {-1, 1, 0, 0};
        static const int dcS[] = {0, 0, -1, 1};
        for (int i = 0; i < 4; i++) {
            for (int step = 1; step < 8; step++) {
                int r = sqR + drS[i] * step, c = sqC + dcS[i] * step;
                if (!inBounds(r, c)) break;
                int p = squares[r][c];
                if (p != EMPTY) {
                    if (p == enemyR || p == enemyQ) return true;
                    break;
                }
            }
        }

        static const int drD[] = {-1, -1, 1, 1};
        static const int dcD[] = {-1, 1, -1, 1};
        for (int i = 0; i < 4; i++) {
            for (int step = 1; step < 8; step++) {
                int r = sqR + drD[i] * step, c = sqC + dcD[i] * step;
                if (!inBounds(r, c)) break;
                int p = squares[r][c];
                if (p != EMPTY) {
                    if (p == enemyB || p == enemyQ) return true;
                    break;
                }
            }
        }
        return false;
    }

    bool inCheck(bool white) const {
        int target = white ? K : k;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (squares[r][c] == target) return isSquareAttacked(r, c, !white);
            }
        }
        return false;
    }

    int evaluate() const {
        int mgScore = 0, egScore = 0;
        int gamePhase = 0;
        int wBishops = 0, bBishops = 0;
        int wPawnsPerCol[8] = {0}, bPawnsPerCol[8] = {0};

        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int p = squares[r][c];
                if (p == EMPTY) continue;
                if (p == P) wPawnsPerCol[c]++;
                else if (p == p) bPawnsPerCol[c]++;
            }
        }

        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int p = squares[r][c];
                if (p == EMPTY) continue;
                int pType = std::abs(p);
                bool isWhite = (p > 0);
                int mirrorR = isWhite ? r : (7 - r);

                int mat = pieceValues[pType];
                int mgPos = 0, egPos = 0;

                switch (pType) {
                    case 1:
                        mgPos = mg_pawnTable[mirrorR][c];
                        egPos = eg_pawnTable[mirrorR][c];
                        break;
                    case 2:
                        mgPos = egPos = mg_knightTable[mirrorR][c];
                        gamePhase += 1;
                        break;
                    case 3:
                        mgPos = egPos = mg_bishopTable[mirrorR][c];
                        gamePhase += 1;
                        if (isWhite) wBishops++; else bBishops++;
                        break;
                    case 4:
                        mgPos = egPos = mg_rookTable[mirrorR][c];
                        if (isWhite && wPawnsPerCol[c] == 0) { mgPos += 15; egPos += 15; }
                        if (!isWhite && bPawnsPerCol[c] == 0) { mgPos += 15; egPos += 15; }
                        gamePhase += 2;
                        break;
                    case 5:
                        mgPos = egPos = mg_queenTable[mirrorR][c];
                        gamePhase += 4;
                        break;
                    case 6:
                        mgPos = mg_kingTable[mirrorR][c];
                        egPos = eg_kingTable[mirrorR][c];
                        break;
                }

                if (isWhite) {
                    mgScore += mat + mgPos;
                    egScore += mat + egPos;
                } else {
                    mgScore -= (mat + mgPos);
                    egScore -= (mat + egPos);
                }
            }
        }

        if (gamePhase > 24) gamePhase = 24;
        int score = (mgScore * gamePhase + egScore * (24 - gamePhase)) / 24;

        if (wBishops >= 2) score += 30;
        if (bBishops >= 2) score -= 30;

        return whiteTurn ? score : -score;
    }
};

int scoreMove(const Board& b, const Move& m, int ply, const Move& pvMove) {
    if (m == pvMove) return 2000000;

    int target = std::abs(b.squares[m.toRow][m.toCol]);
    int attacker = std::abs(b.squares[m.fromRow][m.fromCol]);

    // Pawn promotion priority
    if (attacker == 1 && (m.toRow == 0 || m.toRow == 7)) {
        return 1500000 + pieceValues[target];
    }

    // Captures: MVV-LVA using actual centipawn weights
    if (target != EMPTY) {
        return 1000000 + 10 * pieceValues[target] - pieceValues[attacker];
    }

    // Quiet moves: Killer moves & History heuristic
    if (ply < 64) {
        if (m == killerMoves[ply][0]) return 900000;
        if (m == killerMoves[ply][1]) return 800000;
    }
    int side = b.whiteTurn ? 0 : 1;
    int fromSq = m.fromRow * 8 + m.fromCol;
    int toSq = m.toRow * 8 + m.toCol;
    return historyTable[side][fromSq][toSq];
}

int quiescence(Board& b, int ply, int alpha, int beta) {
    checkTime();
    if (searchAborted) return 0;

    bool in_check = b.inCheck(b.whiteTurn);

    // Do NOT stand pat if in check; must verify legal check evasions exist
    if (!in_check) {
        int standPat = b.evaluate();
        if (standPat >= beta) return beta;
        if (alpha < standPat) alpha = standPat;
    }

    // If in check, generate ALL moves to find evasions; otherwise only captures/promotions
    std::vector<Move> moves = b.generatePseudoMoves(!in_check);
    Move dummyPV{-1, -1, -1, -1};
    std::sort(moves.begin(), moves.end(), [&b, ply, &dummyPV](const Move& m1, const Move& m2) {
        return scoreMove(b, m1, ply, dummyPV) > scoreMove(b, m2, ply, dummyPV);
    });

    bool hasLegalMove = false;
    for (const auto& m : moves) {
        int captured, movedPiece;
        bool movingSide = b.whiteTurn;
        b.makeMove(m, captured, movedPiece);

        if (b.inCheck(movingSide)) {
            b.unmakeMove(m, captured, movedPiece);
            continue;
        }
        hasLegalMove = true;

        int score = -quiescence(b, ply + 1, -beta, -alpha);
        b.unmakeMove(m, captured, movedPiece);

        if (searchAborted) return 0;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    if (in_check && !hasLegalMove) {
        return -100000 + ply; // Checkmate detected at horizon
    }

    return alpha;
}

int negamax(Board& b, int depth, int ply, int alpha, int beta) {
    checkTime();
    if (searchAborted) return 0;

    int mate_val = 100000 - ply;
    if (alpha < -mate_val) alpha = -mate_val;
    if (beta > mate_val - 1) beta = mate_val - 1;
    if (alpha >= beta) return alpha;

    bool in_check = b.inCheck(b.whiteTurn);
    // Check extension: never stop searching while in check
    if (in_check && ply < 40) depth++;

    if (depth <= 0) return quiescence(b, ply, alpha, beta);

    std::vector<Move> moves = b.generatePseudoMoves(false);
    Move dummyPV{-1, -1, -1, -1};
    std::sort(moves.begin(), moves.end(), [&b, ply, &dummyPV](const Move& m1, const Move& m2) {
        return scoreMove(b, m1, ply, dummyPV) > scoreMove(b, m2, ply, dummyPV);
    });

    int bestScore = -1000000;
    bool hasLegalMove = false;

    for (const auto& m : moves) {
        int captured, movedPiece;
        bool movingSide = b.whiteTurn;
        b.makeMove(m, captured, movedPiece);

        if (b.inCheck(movingSide)) {
            b.unmakeMove(m, captured, movedPiece);
            continue;
        }
        hasLegalMove = true;

        int score = -negamax(b, depth - 1, ply + 1, -beta, -alpha);
        b.unmakeMove(m, captured, movedPiece);

        if (searchAborted) return 0;

        if (score > bestScore) bestScore = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
            // Record killer & history heuristics for quiet moves that cause cutoffs
            if (captured == EMPTY && ply < 64) {
                if (!(m == killerMoves[ply][0])) {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = m;
                }
                int side = b.whiteTurn ? 0 : 1;
                int fromSq = m.fromRow * 8 + m.fromCol;
                int toSq = m.toRow * 8 + m.toCol;
                historyTable[side][fromSq][toSq] += depth * depth;
            }
            break;
        }
    }

    if (!hasLegalMove) {
        if (in_check) return -100000 + ply;
        return 0;
    }
    return bestScore;
}

Move findBestMove(Board& b, int maxDepth) {
    searchStartTime = std::chrono::steady_clock::now();
    searchAborted = false;
    nodesSearched = 0;
    std::memset(killerMoves, 0, sizeof(killerMoves));
    std::memset(historyTable, 0, sizeof(historyTable));

    std::vector<Move> moves = b.generatePseudoMoves(false);
    Move globalBestMove{-1, -1, -1, -1};

    // Iterative Deepening Loop (Depth 1 -> maxDepth)
    for (int currentDepth = 1; currentDepth <= maxDepth; ++currentDepth) {
        std::sort(moves.begin(), moves.end(), [&b, &globalBestMove](const Move& m1, const Move& m2) {
            return scoreMove(b, m1, 0, globalBestMove) > scoreMove(b, m2, 0, globalBestMove);
        });

        Move depthBestMove{-1, -1, -1, -1};
        int bestVal = -1000000;
        int alpha = -1000000;
        int beta = 1000000;

        for (const auto& m : moves) {
            int captured, movedPiece;
            bool movingSide = b.whiteTurn;
            b.makeMove(m, captured, movedPiece);

            if (b.inCheck(movingSide)) {
                b.unmakeMove(m, captured, movedPiece);
                continue;
            }

            int eval = -negamax(b, currentDepth - 1, 1, -beta, -alpha);
            b.unmakeMove(m, captured, movedPiece);

            // Always allow Depth 1 to finish; abort deeper depths if time expires
            if (searchAborted && currentDepth > 1) break;

            if (eval > bestVal) {
                bestVal = eval;
                depthBestMove = m;
                if (currentDepth == 1 || !searchAborted) {
                    globalBestMove = m;
                }
            }
            if (eval > alpha) {
                alpha = eval;
            }
        }

        if (searchAborted && currentDepth > 1) break;
        if (depthBestMove.fromRow != -1) {
            globalBestMove = depthBestMove;
        }
        // Stop early if forced checkmate is already found
        if (bestVal > 90000 || bestVal < -90000) break;
    }

    return globalBestMove;
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    std::string line;
    Board board;

    while (std::getline(std::cin, line)) {
        if (line.empty() || line == "quit") break;

        if (line.rfind("fen ", 0) == 0) {
            std::string fen = line.substr(4);
            board.loadFEN(fen);
            Move best = findBestMove(board, 64); // Iterative deepening up to 2.0 seconds

            int pType = 0;
            if (best.fromRow != -1) pType = std::abs(board.squares[best.fromRow][best.fromCol]);

            std::cout << "bestmove " << best.toUCI(pType) << std::endl;
        }
    }
    return 0;
}