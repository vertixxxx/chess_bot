#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>

enum Piece { EMPTY = 0, P = 1, N = 2, B = 3, R = 4, Q = 5, K = 6,
                        p = -1, n = -2, b = -3, r = -4, q = -5, k = -6 };

struct Move {
    int fromRow, fromCol, toRow, toCol;
    std::string toUCI(int pType = 0) const {
        if (fromRow == -1) return "0000"; // Failsafe for empty moves
        char fC = 'a' + fromCol, fR = '8' - fromRow;
        char tC = 'a' + toCol, tR = '8' - toRow;
        std::string uci = {fC, fR, tC, tR};
        // Auto-append promotion flag for UCI compatibility
        if (pType == 1 && (toRow == 0 || toRow == 7)) uci += "q";
        return uci;
    }
};

// Piece-Square Tables (White perspective; flipped for Black)
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

    inline bool inBounds(int r, int c) const { return r >= 0 && r < 8 && c >= 0 && c < 8; }

    std::vector<Move> generatePseudoMoves(bool capturesOnly = false) const {
        std::vector<Move> moves;
        moves.reserve(capturesOnly ? 15 : 40); 

        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int piece = squares[r][c];
                if (piece == EMPTY || (whiteTurn && piece < 0) || (!whiteTurn && piece > 0))
                    continue;

                int pType = std::abs(piece);
                if (pType == 1) { // Pawn
                    int dir = (piece > 0) ? -1 : 1;
                    int startRow = (piece > 0) ? 6 : 1;
                    if (!capturesOnly) {
                        if (inBounds(r + dir, c) && squares[r + dir][c] == EMPTY) {
                            moves.push_back({r, c, r + dir, c});
                            if (r == startRow && squares[r + 2 * dir][c] == EMPTY)
                                moves.push_back({r, c, r + 2 * dir, c});
                        }
                    }
                    for (int dc : {-1, 1}) {
                        int nr = r + dir, nc = c + dc;
                        if (inBounds(nr, nc)) {
                            if ((piece > 0 && squares[nr][nc] < 0) || (piece < 0 && squares[nr][nc] > 0))
                                moves.push_back({r, c, nr, nc});
                        }
                    }
                } else if (pType == 2) { // Knight
                    int dr[] = {-2, -2, -1, -1, 1, 1, 2, 2};
                    int dc[] = {-1, 1, -2, 2, -2, 2, -1, 1};
                    for (int i = 0; i < 8; ++i) {
                        int nr = r + dr[i], nc = c + dc[i];
                        if (inBounds(nr, nc)) {
                            if (squares[nr][nc] == EMPTY) {
                                if (!capturesOnly) moves.push_back({r, c, nr, nc});
                            } else if ((piece > 0 && squares[nr][nc] < 0) || (piece < 0 && squares[nr][nc] > 0)) {
                                moves.push_back({r, c, nr, nc});
                            }
                        }
                    }
                } else { // Sliders & Kings
                    std::vector<std::pair<int, int>> dirs;
                    if (pType == 3 || pType == 5) dirs.insert(dirs.end(), {{-1,-1},{-1,1},{1,-1},{1,1}});
                    if (pType == 4 || pType == 5) dirs.insert(dirs.end(), {{-1,0},{1,0},{0,-1},{0,1}});
                    if (pType == 6) dirs = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};

                    int maxSteps = (pType == 6) ? 1 : 7;
                    for (auto& d : dirs) {
                        for (int step = 1; step <= maxSteps; ++step) {
                            int nr = r + d.first * step, nc = c + d.second * step;
                            if (!inBounds(nr, nc)) break;
                            if (squares[nr][nc] == EMPTY) {
                                if (!capturesOnly) moves.push_back({r, c, nr, nc});
                            } else {
                                if ((piece > 0 && squares[nr][nc] < 0) || (piece < 0 && squares[nr][nc] > 0))
                                    moves.push_back({r, c, nr, nc});
                                break;
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
        
        // Internal auto-promotion tracking
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

        int drN[] = {-2, -2, -1, -1, 1, 1, 2, 2};
        int dcN[] = {-1, 1, -2, 2, -2, 2, -1, 1};
        for(int i = 0; i < 8; i++) {
            int r = sqR + drN[i], c = sqC + dcN[i];
            if (inBounds(r, c) && squares[r][c] == enemyN) return true;
        }

        int drK[] = {-1, -1, -1, 0, 0, 1, 1, 1};
        int dcK[] = {-1, 0, 1, -1, 1, -1, 0, 1};
        for(int i = 0; i < 8; i++) {
            int r = sqR + drK[i], c = sqC + dcK[i];
            if (inBounds(r, c) && squares[r][c] == enemyK) return true;
        }

        int drS[] = {-1, 1, 0, 0};
        int dcS[] = {0, 0, -1, 1};
        for(int i = 0; i < 4; i++) {
            for(int step = 1; step < 8; step++) {
                int r = sqR + drS[i] * step, c = sqC + dcS[i] * step;
                if (!inBounds(r, c)) break;
                int p = squares[r][c];
                if (p != EMPTY) {
                    if (p == enemyR || p == enemyQ) return true;
                    break;
                }
            }
        }

        int drD[] = {-1, -1, 1, 1};
        int dcD[] = {-1, 1, -1, 1};
        for(int i = 0; i < 4; i++) {
            for(int step = 1; step < 8; step++) {
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
        static const int pieceValues[7] = {0, 100, 320, 335, 500, 900, 20000};
        int score = 0;

        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int p = squares[r][c];
                if (p == EMPTY) continue;
                int pType = std::abs(p);
                bool isWhite = (p > 0);
                int mirrorR = isWhite ? r : (7 - r);

                int val = pieceValues[pType];
                switch (pType) {
                    case 1: val += mg_pawnTable[mirrorR][c]; break;
                    case 2: val += mg_knightTable[mirrorR][c]; break;
                    case 3: val += mg_bishopTable[mirrorR][c]; break;
                    case 4: val += mg_rookTable[mirrorR][c]; break;
                    case 5: val += mg_queenTable[mirrorR][c]; break;
                    case 6: val += mg_kingTable[mirrorR][c]; break;
                }
                score += isWhite ? val : -val;
            }
        }
        return whiteTurn ? score : -score;
    }
};

int scoreMove(const Board& b, const Move& m) {
    int target = std::abs(b.squares[m.toRow][m.toCol]);
    int attacker = std::abs(b.squares[m.fromRow][m.fromCol]);
    if (target != EMPTY) return 10 * target - attacker;
    return 0;
}

int quiescence(Board& b, int alpha, int beta) {
    int standPat = b.evaluate();
    if (standPat >= beta) return beta;
    if (alpha < standPat) alpha = standPat;

    std::vector<Move> moves = b.generatePseudoMoves(true);
    std::sort(moves.begin(), moves.end(), [&b](const Move& m1, const Move& m2) {
        return scoreMove(b, m1) > scoreMove(b, m2);
    });

    for (const auto& m : moves) {
        int captured, movedPiece;
        bool movingSide = b.whiteTurn;
        b.makeMove(m, captured, movedPiece);
        
        if (b.inCheck(movingSide)) {
            b.unmakeMove(m, captured, movedPiece);
            continue;
        }

        int score = -quiescence(b, -beta, -alpha);
        b.unmakeMove(m, captured, movedPiece);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int negamax(Board& b, int depth, int ply, int alpha, int beta) {
    // 1. Mate Distance Pruning
    int mate_val = 100000 - ply;
    if (alpha < -mate_val) alpha = -mate_val;
    if (beta > mate_val - 1) beta = mate_val - 1;
    if (alpha >= beta) return alpha;

    if (depth == 0) return quiescence(b, alpha, beta);

    std::vector<Move> moves = b.generatePseudoMoves(false);
    std::sort(moves.begin(), moves.end(), [&b](const Move& m1, const Move& m2) {
        return scoreMove(b, m1) > scoreMove(b, m2);
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

        if (score > bestScore) bestScore = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }

    if (!hasLegalMove) {
        if (b.inCheck(b.whiteTurn)) return -100000 + ply; // Escalate fast checkmates
        return 0; // Stalemate
    }
    return bestScore;
}

Move findBestMove(Board& b, int depth) {
    std::vector<Move> moves = b.generatePseudoMoves(false);
    std::sort(moves.begin(), moves.end(), [&b](const Move& m1, const Move& m2) {
        return scoreMove(b, m1) > scoreMove(b, m2);
    });

    Move bestMove{-1, -1, -1, -1};
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

        int eval = -negamax(b, depth - 1, 1, -beta, -alpha);
        b.unmakeMove(m, captured, movedPiece);

        if (eval > bestVal) {
            bestVal = eval;
            bestMove = m;
        }
        if (eval > alpha) {
            alpha = eval;
        }
    }
    return bestMove;
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
            Move best = findBestMove(board, 7); 
            
            int pType = 0;
            if (best.fromRow != -1) pType = std::abs(board.squares[best.fromRow][best.fromCol]);
            
            std::cout << "bestmove " << best.toUCI(pType) << std::endl;
        }
    }
    return 0;
}