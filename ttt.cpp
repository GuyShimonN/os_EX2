#include <iostream>
#include <vector>
#include <limits>
#include <unordered_map>

using namespace std;
// Function to check the board for a win, lose, or draw condition
void check_board(const vector<vector<int>>& board) {
    // Check rows and columns for win/lose condition
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            if (board[i][0] != 0) {
                cout << (board[i][0] == 1 ? "WIN" : "LOSE") << endl;
                cout << "good game" << endl;
                exit(0);
            }
        }
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i]) {
            if (board[0][i] != 0) {
                cout << (board[0][i] == 1 ? "WIN" : "LOSE") << endl;
                cout << "gg" << endl;
                exit(0);
            }
        }
    }

    // Check diagonals for win/lose condition
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
        if (board[0][0] != 0) {
            cout << (board[0][0] == 1 ? "WIN" : "LOSE") << endl;
            cout << "gg" << endl;
            exit(0);
        }
    }
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
        if (board[0][2] != 0) {
            cout << (board[0][2] == 1 ? "WIN" : "LOSE") << endl;
            cout << "gg" << endl;
            exit(0);
        }
    }

    // Check for draw condition
    bool isDraw = true;
    for (const auto& row : board) {
        for (int cell : row) {
            if (cell == 0) {
                isDraw = false;
                break;
            }
        }
        if (!isDraw) break;
    }
    if (isDraw) {
        cout << "DRAW" << endl;
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "not valid input" << endl;
        exit(1);
    }

    string select = argv[1];

    // Validation: check that the input length is 9 and contains each digit 1-9 exactly once
    if (select.length() != 9) {
        cout << "Input must be exactly 9 characters long." << endl;
        exit(1);
    }

    unordered_map<char, int> digit_count;
    for (char c : select) {
        if (c < '1' || c > '9') {
            cout << "Input must contain only digits from 1 to 9." << endl;
            exit(1);
        }
        digit_count[c]++;
    }

    for (char c = '1'; c <= '9'; c++) {
        if (digit_count[c] != 1) {
            cout << "Each digit from 1 to 9 must appear exactly once." << endl;
            exit(1);
        }
    }

    vector<int> matrix(9);
    vector<vector<int>> board(3, vector<int>(3, 0));

    for (int i = 0; i < 9; i++) {
        matrix[i] = select[i] - '0';
    }

    for (size_t i = 0; i < matrix.size(); i++) {
        cout << matrix.at(i) << " ";
    }
    cout << endl;

    bool turn = true; // true for AI, false for human

    for (int j = 0; j < 9; j++) {
        if (turn) {
//            bool moveMade = false;
            for (int i = 0; i < 9; i++) {
                int num = matrix.at(i) - 1; // Adjusting 1-9 to 0-8 for indexing
//                if (num < 0 || num > 8) continue; // Ensure num is within valid range

                int row = num / 3;
                int col = num % 3;

                // Debugging: Print AI move information
                cout << "AI move: i = " << i << ", num = " << num << ", row = " << row << ", col = " << col << endl;

                if (board[row][col] == 0) {
                    board[row][col] = 1;
                    turn = false;
//                    moveMade = true;
                    break;
                }
            }
//            if (!moveMade) {
//                cout << "No valid moves for AI. Ending game." << endl;
//                exit(1);
//            }
        } else {
            int num = 0;
            while (true) {
                cout << "Enter your move (1-9): ";
                if (!(cin >> num) || num < 1 || num > 9) {
                    cout << "Invalid input. Please enter a number between 1 and 9." << endl;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                } else {
                    int row = (num - 1) / 3;
                    int col = (num - 1) % 3;

                    // Debugging: Print human move information
                    cout << "Human move: num = " << num << ", row = " << row << ", col = " << col << endl;

                    if (board[row][col] != 0) {
                        cout << "Invalid move. The cell is already occupied. Try again." << endl;
                    } else {
                        board[row][col] = -1;
                        turn = true;
                        break;
                    }
                }
            }
        }

        // Debugging: Print the board state after each move
        cout << "Board state after move:" << endl;
        for (const auto& row : board) {
            for (int cell : row) {
                cout << cell << " ";
            }
            cout << endl;
        }
        cout << endl;

        check_board(board);
    }

    return 0;
}