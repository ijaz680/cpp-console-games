// tictactoe.cpp
// Simple Tic-Tac-Toe console game in C++ (Styled version)
// Supports: 2-player or Human vs Computer (AI using Minimax)

#include <iostream>
#include <vector>
#include <limits>
using namespace std;

const char HUMAN = 'X';
const char COMPUTER = 'O';
const char EMPTY = ' ';

void lineStyle() {
    cout << "############################################\n";
}

void printBoard(const vector<char>& board) {
    cout << "\n";
    lineStyle();
    for (int r = 0; r < 3; ++r) {
        cout << " " << board[r*3 + 0] << " | " << board[r*3 + 1] << " | " << board[r*3 + 2] << " \n";
        if (r < 2) cout << "---+---+---\n";
    }
    lineStyle();
    cout << "\n";
}

bool isMovesLeft(const vector<char>& board) {
    for (char c : board) if (c == EMPTY) return true;
    return false;
}

int evaluate(const vector<char>& b) {
    // Rows
    for (int row = 0; row < 3; ++row) {
        if (b[row*3] == b[row*3 + 1] && b[row*3 + 1] == b[row*3 + 2]) {
            if (b[row*3] == COMPUTER) return +10;
            else if (b[row*3] == HUMAN) return -10;
        }
    }
    // Columns
    for (int col = 0; col < 3; ++col) {
        if (b[col] == b[col + 3] && b[col + 3] == b[col + 6]) {
            if (b[col] == COMPUTER) return +10;
            else if (b[col] == HUMAN) return -10;
        }
    }
    // Diagonals
    if (b[0] == b[4] && b[4] == b[8]) {
        if (b[0] == COMPUTER) return +10;
        else if (b[0] == HUMAN) return -10;
    }
    if (b[2] == b[4] && b[4] == b[6]) {
        if (b[2] == COMPUTER) return +10;
        else if (b[2] == HUMAN) return -10;
    }
    return 0;
}

// Minimax algorithm
int minimax(vector<char>& board, int depth, bool isMax) {
    int score = evaluate(board);
    if (score == 10) return score - depth;   // prefer faster wins
    if (score == -10) return score + depth;  // prefer slower losses
    if (!isMovesLeft(board)) return 0; // draw

    if (isMax) {
        int best = numeric_limits<int>::min();
        for (int i = 0; i < 9; ++i) {
            if (board[i] == EMPTY) {
                board[i] = COMPUTER;
                best = max(best, minimax(board, depth + 1, !isMax));
                board[i] = EMPTY;
            }
        }
        return best;
    } else {
        int best = numeric_limits<int>::max();
        for (int i = 0; i < 9; ++i) {
            if (board[i] == EMPTY) {
                board[i] = HUMAN;
                best = min(best, minimax(board, depth + 1, !isMax));
                board[i] = EMPTY;
            }
        }
        return best;
    }
}

int findBestMove(vector<char>& board) {
    int bestVal = numeric_limits<int>::min();
    int bestMove = -1;
    for (int i = 0; i < 9; ++i) {
        if (board[i] == EMPTY) {
            board[i] = COMPUTER;
            int moveVal = minimax(board, 0, false);
            board[i] = EMPTY;
            if (moveVal > bestVal) {
                bestMove = i;
                bestVal = moveVal;
            }
        }
    }
    return bestMove;
}

int checkWin(const vector<char>& b) {
    int val = evaluate(b);
    if (val == 10) return 1;   // computer
    if (val == -10) return -1; // human
    if (!isMovesLeft(b)) return 0; // draw
    return 2; // game ongoing
}

int promptMove(const vector<char>& board) {
    while (true) {
        cout << "Enter your move (1-9): ";
        int pos;
        if (!(cin >> pos)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number 1-9.\n";
            continue;
        }
        if (pos < 1 || pos > 9) {
            cout << "Position must be 1..9.\n";
            continue;
        }
        if (board[pos - 1] != EMPTY) {
            cout << "Cell already taken. Choose another.\n";
            continue;
        }
        return pos - 1;
    }
}

void twoPlayerGame() {
    vector<char> board(9, EMPTY);
    char turn = HUMAN; // X starts
    lineStyle();
    cout << " Two-player mode. X = Player1, O = Player2\n";
    lineStyle();
    printBoard(board);

    while (true) {
        lineStyle();
        if (turn == HUMAN) cout << " Player X's turn.\n";
        else cout << " Player O's turn.\n";
        lineStyle();

        int move = promptMove(board);
        board[move] = turn;
        printBoard(board);
        int state = checkWin(board);

        if (state == 1) { lineStyle(); cout << " O (Player 2) wins!\n"; lineStyle(); break; }
        else if (state == -1) { lineStyle(); cout << " X (Player 1) wins!\n"; lineStyle(); break; }
        else if (state == 0) { lineStyle(); cout << " It's a draw!\n"; lineStyle(); break; }

        turn = (turn == HUMAN) ? COMPUTER : HUMAN;
    }
}

void humanVsComputer() {
    vector<char> board(9, EMPTY);
    lineStyle();
    cout << " Human vs Computer\n You are X. Computer is O.\n";
    lineStyle();
    printBoard(board);

    char choice;
    cout << "Do you want to go first? (y/n): ";
    cin >> choice;
    bool humanTurn = (choice == 'y' || choice == 'Y');

    while (true) {
        if (humanTurn) {
            lineStyle();
            cout << " Your move (X):\n";
            lineStyle();
            int move = promptMove(board);
            board[move] = HUMAN;
        } else {
            lineStyle();
            cout << " Computer is thinking...\n";
            lineStyle();
            int best = findBestMove(board);
            if (best == -1) {
                for (int i=0;i<9;++i) if (board[i]==EMPTY) { best = i; break; }
            }
            board[best] = COMPUTER;
            cout << " Computer chose position " << (best + 1) << ".\n";
        }

        printBoard(board);
        int state = checkWin(board);
        if (state == 1) { lineStyle(); cout << " Computer (O) wins!\n"; lineStyle(); break; }
        else if (state == -1) { lineStyle(); cout << " You (X) win! Congrats!\n"; lineStyle(); break; }
        else if (state == 0) { lineStyle(); cout << " It's a draw!\n"; lineStyle(); break; }

        humanTurn = !humanTurn;
    }
}

int main() {
    lineStyle();
    cout << "          === Tic-Tac-Toe Game ===\n";
    lineStyle();
    cout << "1) Two players\n2) Play vs Computer (AI)\nChoose mode (1 or 2): ";
    int mode;
    if (!(cin >> mode)) {
        cout << "Invalid input. Exiting.\n";
        return 0;
    }
    if (mode == 1) twoPlayerGame();
    else if (mode == 2) humanVsComputer();
    else cout << "Unknown mode. Exiting.\n";
    return 0;
}
