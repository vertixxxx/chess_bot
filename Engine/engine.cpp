#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

enum Piece { EMPTY = 0, P = 1, N = 2, B = 3, R = 4, Q = 5, K = 6,
                        p = -1, n = -2, b = -3, r = -4, q = -5, k = -6 };

struct Move {
    int fromRow, fromCol, toRow, toCol;
    std::string toUCI() const {
        char fC = 'a' + fromCol, fR = '8' - fromRow;
        char tC = 'a' + toCol, tR = '8' - toRow;
        return std::string{fC, fR, tC, tR};
    }
};

class Board {
public:
    int squares[8][8];
    bool whiteTurn = true;

    Board() {
        reset();
    }

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

    bool inBounds(int r, int c) const { return r >= 0 && r < 8 && c >= 0 && c < 8; }

    std::vector<Move> generateLegalMoves() const {
        std::vector<Move> moves;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int piece = squares[r][c];
                if (piece == EMPTY || (whiteTurn && piece < 0) || (!whiteTurn && piece > 0))
                    continue;

                int pType = std::abs(piece);
                if (pType == 1) { // Pawn
                    int dir = (piece > 0) ? -1 : 1;
                    int startRow = (piece > 0) ? 6 : 1;
                    if (inBounds(r + dir, c) && squares[r + dir][c] == EMPTY) {
                        moves.push_back({r, c, r + dir, c});
                        if (r == startRow && squares[r + 2 * dir][c] == EMPTY)
                            moves.push_back({r, c, r + 2 * dir, c});
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
                            if (squares[nr][nc] == EMPTY || (piece > 0 && squares[nr][nc] < 0) || (piece < 0 && squares[nr][nc] > 0))
                                moves.push_back({r, c, nr, nc});
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
                                moves.push_back({r, c, nr, nc});
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

    void makeMove(const Move& m, int& captured) {
        captured = squares[m.toRow][m.toCol];
        squares[m.toRow][m.toCol] = squares[m.fromRow][m.fromCol];
        squares[m.fromRow][m.fromCol] = EMPTY;
        whiteTurn = !whiteTurn;
    }

    void unmakeMove(const Move& m, int captured) {
        squares[m.fromRow][m.fromCol] = squares[m.toRow][m.toCol];
        squares[m.toRow][m.toCol] = captured;
        whiteTurn = !whiteTurn;
    }

    int evaluate() const {
        static const int pieceValues[7] = {0, 100, 320, 330, 500, 900, 20000};
        int score = 0;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int p = squares[r][c];
                if (p > 0) score += pieceValues[p];
                else if (p < 0) score -= pieceValues[-p];
            }
        }
        return score;
    }
};



// Add this helper function above findBestMove
bool isLegal(Board b, const Move& m) { // Passed by value intentionally to simulate the move
    int captured;
    b.makeMove(m, captured);
    
    // Find our King (turn flipped, so check the opponent's color)
    int ourKing = b.whiteTurn ? -6 : 6; 
    int kr = -1, kc = -1;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (b.squares[r][c] == ourKing) {
                kr = r; kc = c; break;
            }
        }
        if (kr != -1) break;
    }
    
    // Generate responses to see if the king can be captured
    std::vector<Move> oppMoves = b.generateLegalMoves();
    for (const auto& oppM : oppMoves) {
        if (oppM.toRow == kr && oppM.toCol == kc) return false;
    }
    return true;
}

int minimax(Board& b, int depth, int alpha, int beta, bool maximizing) {
    if (depth == 0) return b.evaluate();

    std::vector<Move> moves = b.generateLegalMoves();
    if (moves.empty()) return maximizing ? -100000 : 100000;

    // Instant checkmate/illegal move detection (bypasses Alpha-Beta blindness)
    for (const auto& m : moves) {
        int target = b.squares[m.toRow][m.toCol];
        if (target == 6 || target == -6) {
            return maximizing ? (300000 + depth) : (-300000 - depth);
        }
    }

    if (maximizing) {
        int maxEval = -1000000;
        for (const auto& m : moves) {
            int captured;
            b.makeMove(m, captured);
            int eval = minimax(b, depth - 1, alpha, beta, false);
            b.unmakeMove(m, captured);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break;
        }
        return maxEval;
    } else {
        int minEval = 1000000;
        for (const auto& m : moves) {
            int captured;
            b.makeMove(m, captured);
            int eval = minimax(b, depth - 1, alpha, beta, true);
            b.unmakeMove(m, captured);
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break;
        }
        return minEval;
    }
}

Move findBestMove(Board& b, int depth) {
    std::vector<Move> pseudo_moves = b.generateLegalMoves();
    std::vector<Move> legal_moves;
    
    // Strictly filter out illegal moves at the root
    for (const auto& m : pseudo_moves) {
        if (isLegal(b, m)) legal_moves.push_back(m);
    }

    // Failsafe if in checkmate
    if (legal_moves.empty()) return Move{-1, -1, -1, -1};

    Move bestMove = legal_moves[0];
    int bestVal = b.whiteTurn ? -1000000 : 1000000;

    for (const auto& m : legal_moves) {
        int captured;
        b.makeMove(m, captured);
        int eval = minimax(b, depth - 1, -1000000, 1000000, !b.whiteTurn);
        b.unmakeMove(m, captured);

        if (b.whiteTurn && eval > bestVal) {
            bestVal = eval;
            bestMove = m;
        } else if (!b.whiteTurn && eval < bestVal) {
            bestVal = eval;
            bestMove = m;
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
            Move best = findBestMove(board, 4);
            std::cout << "bestmove " << best.toUCI() << std::endl;
        }
    }
    return 0;
}