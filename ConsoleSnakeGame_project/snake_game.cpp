// main.cpp
// Console Snake game with a start-screen and "typewriter" code-writing animation.
// Controls: WASD or Arrow keys. Press 'q' to quit.

#include <iostream>
#include <vector>
#include <deque>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <string>

#ifdef _WIN32
  #include <conio.h>
  #include <windows.h>
#else
  #include <termios.h>
  #include <unistd.h>
  #include <sys/select.h>
#endif

using namespace std;

constexpr int WIDTH = 30;
constexpr int HEIGHT = 20;
constexpr char SNAKE_CHAR = 'O';
constexpr char FOOD_CHAR = '*';
constexpr char EMPTY_CHAR = ' ';

enum class Direction { UP, DOWN, LEFT, RIGHT };

struct Point {
    int x;
    int y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

#ifdef _WIN32
void enableANSI() {
    // Enable ANSI escape codes on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#endif

// ---------- Non-blocking keyboard input helpers ----------
#ifdef _WIN32
bool kbhit_nonblock() {
    return _kbhit();
}
int getch_nonblock() {
    if (_kbhit()) {
        return _getch();
    }
    return -1;
}
#else
// POSIX: implement kbhit and getch-like behavior
static struct termios orig_termios;
void reset_terminal_mode() {
    tcsetattr(0, TCSANOW, &orig_termios);
}
void set_conio_terminal_mode() {
    struct termios new_termios;
    tcgetattr(0, &orig_termios);
    new_termios = orig_termios;
    // disable canonical mode, and set buffer size to 1 byte
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_termios);
    atexit(reset_terminal_mode);
}
bool kbhit_nonblock() {
    fd_set set;
    struct timeval tv;
    FD_ZERO(&set);
    FD_SET(0, &set);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    return select(1, &set, nullptr, nullptr, &tv) > 0;
}
int getch_nonblock() {
    unsigned char ch;
    if (read(0, &ch, 1) == 1) return ch;
    return -1;
}
#endif

// ---------- Utility functions ----------
void clear_screen() {
    // Use ANSI escape to clear screen and move cursor to home.
    std::cout << "\x1B[2J\x1B[H";
}

void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Print text with typewriter effect (character-by-character)
void typeEffect(const string &s, int ms_per_char = 30) {
    for (char c : s) {
        cout << c << flush;
        sleep_ms(ms_per_char);
    }
}

// Print a centered line within board width (for intro)
void printCentered(const string &s, int totalWidth = WIDTH + 2) {
    int pad = max(0, (totalWidth - (int)s.size()) / 2);
    for (int i = 0; i < pad; ++i) cout << ' ';
    cout << s << '\n';
}

// ---------- Game class ----------
class SnakeGame {
public:
    SnakeGame()
    : dir(Direction::RIGHT), score(0), gameOver(false), rng(rd()), playerName("Player") {
        reset();
    }

    void reset() {
        board.assign(HEIGHT, std::string(WIDTH, EMPTY_CHAR));
        snake.clear();
        // start snake in middle
        Point mid{WIDTH / 2, HEIGHT / 2};
        snake.push_back(mid);
        // initial length 3
        snake.push_back({mid.x - 1, mid.y});
        snake.push_back({mid.x - 2, mid.y});
        dir = Direction::RIGHT;
        placeFood();
        score = 0;
        gameOver = false;
    }

    // Show start-screen with ASCII title, ask for name, and show "typing code" animation
    void showIntro() {
#ifdef _WIN32
        enableANSI();
#endif
        clear_screen();
        cout << "\n";
        printCentered("+-------------------------------------------+");
        printCentered("|                                           |");
        printCentered("|               S N A K E   G A M E         |");
        printCentered("|                                           |");
        printCentered("+-------------------------------------------+");
        cout << "\n";
        printCentered("A pure C++ console game. Controls: WASD or Arrow keys.");
        cout << "\n";
        cout << "Enter your name (press Enter to accept): ";
        string name;
        getline(cin, name);
        if (!name.empty()) playerName = name;
        cout << "\n";
        printCentered("Preparing game...");
        sleep_ms(400);

        // Typewriter "code writing" effect - small fake code snippet to simulate typing
        vector<string> fakeCode = {
            "int main() {",
            "    // initializing game engine",
            "    SnakeGame game;",
            "    game.run();",
            "    return 0;",
            "}"
        };
        cout << "\n";
        for (const auto &line : fakeCode) {
            printCentered(""); // blank line spacing
            // indent a bit
            cout << "    ";
            typeEffect(line, 25);
            cout << "\n";
            sleep_ms(220);
        }
        cout << "\n";
        printCentered("Press any key to start...");
        // wait for any key press (blocking until a key)
#ifdef _WIN32
        while (!_kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
        // consume key
        _getch();
#else
        // On POSIX, just wait until a key is pressed
        set_conio_terminal_mode();
        while (!kbhit_nonblock()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
        // consume the key
        getch_nonblock();
        reset_terminal_mode(); // restore so input line is clean
#endif
        clear_screen();
    }

    void run() {
#ifdef _WIN32
        enableANSI();
#else
        set_conio_terminal_mode();
#endif
        using clock = std::chrono::steady_clock;
        auto last_update = clock::now();
        int speed_ms = 120; // lower = faster

        while (!gameOver) {
            handleInput();
            auto now = clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();
            if (elapsed >= speed_ms) {
                update();
                draw();
                last_update = now;
            }
            sleep_ms(5);
        }
        draw();
        cout << "\nGame Over! " << playerName << "'s Score: " << score << "\n";
    }

    void setPlayerName(const string &n) { if (!n.empty()) playerName = n; }

private:
    std::vector<std::string> board;
    std::deque<Point> snake;
    Point food;
    Direction dir;
    int score;
    bool gameOver;
    std::random_device rd;
    std::mt19937 rng;
    string playerName;

    void placeFood() {
        std::uniform_int_distribution<int> dx(0, WIDTH - 1);
        std::uniform_int_distribution<int> dy(0, HEIGHT - 1);
        while (true) {
            Point p{dx(rng), dy(rng)};
            bool onSnake = false;
            for (auto &s : snake) if (s == p) { onSnake = true; break; }
            if (!onSnake) { food = p; break; }
        }
    }

    void handleInput() {
        while (kbhit_nonblock()) {
            int ch = getch_nonblock();
            if (ch == -1) break;
#ifdef _WIN32
            // Windows: arrow keys return 0 or 224 first
            if (ch == 0 || ch == 224) {
                int ch2 = getch_nonblock();
                if (ch2 == -1) break;
                switch (ch2) {
                    case 72: tryChangeDir(Direction::UP); break;    // up arrow
                    case 80: tryChangeDir(Direction::DOWN); break;  // down arrow
                    case 75: tryChangeDir(Direction::LEFT); break;  // left arrow
                    case 77: tryChangeDir(Direction::RIGHT); break; // right arrow
                }
            } else {
                handleCharInput(static_cast<char>(ch));
            }
#else
            // POSIX: handle arrow keys via escape sequences or WASD
            if (ch == 27) { // possible arrow key: ESC [
                // attempt to read two more bytes
                int c2 = getch_nonblock();
                int c3 = getch_nonblock();
                if (c2 == '[' && c3 != -1) {
                    switch (c3) {
                        case 'A': tryChangeDir(Direction::UP); break;
                        case 'B': tryChangeDir(Direction::DOWN); break;
                        case 'C': tryChangeDir(Direction::RIGHT); break;
                        case 'D': tryChangeDir(Direction::LEFT); break;
                    }
                }
            } else {
                handleCharInput(static_cast<char>(ch));
            }
#endif
        }
    }

    void handleCharInput(char c) {
        c = std::tolower(c);
        if (c == 'w') tryChangeDir(Direction::UP);
        else if (c == 's') tryChangeDir(Direction::DOWN);
        else if (c == 'a') tryChangeDir(Direction::LEFT);
        else if (c == 'd') tryChangeDir(Direction::RIGHT);
        else if (c == 'q') gameOver = true;
    }

    void tryChangeDir(Direction newDir) {
        // prevent reversing directly
        if ((dir == Direction::UP && newDir == Direction::DOWN) ||
            (dir == Direction::DOWN && newDir == Direction::UP) ||
            (dir == Direction::LEFT && newDir == Direction::RIGHT) ||
            (dir == Direction::RIGHT && newDir == Direction::LEFT)) {
            return;
        }
        dir = newDir;
    }

    void update() {
        Point head = snake.front();
        Point next = head;
        switch (dir) {
            case Direction::UP: next.y -= 1; break;
            case Direction::DOWN: next.y += 1; break;
            case Direction::LEFT: next.x -= 1; break;
            case Direction::RIGHT: next.x += 1; break;
        }
        // wrap around (or comment this out to make walls deadly)
        if (next.x < 0) next.x = WIDTH - 1;
        if (next.x >= WIDTH) next.x = 0;
        if (next.y < 0) next.y = HEIGHT - 1;
        if (next.y >= HEIGHT) next.y = 0;

        // check collision with self
        for (const auto &part : snake) {
            if (next == part) { gameOver = true; return; }
        }

        // move snake
        snake.push_front(next);

        // check food
        if (next == food) {
            score += 10;
            placeFood();
        } else {
            // normal move: pop tail
            snake.pop_back();
        }
    }

    void draw() {
        // build board
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) board[y][x] = EMPTY_CHAR;
        }
        for (const auto &p : snake) board[p.y][p.x] = SNAKE_CHAR;
        board[food.y][food.x] = FOOD_CHAR;

        clear_screen();

        // top border
        cout << '+';
        for (int i = 0; i < WIDTH; ++i) cout << '-';
        cout << "+\n";

        for (int y = 0; y < HEIGHT; ++y) {
            cout << '|';
            for (int x = 0; x < WIDTH; ++x) cout << board[y][x];
            cout << "|\n";
        }

        // bottom border
        cout << '+';
        for (int i = 0; i < WIDTH; ++i) cout << '-';
        cout << "+\n";

        // Player name + score on same line
        cout << playerName << "   Score: " << score << "   Controls: WASD or Arrow keys. Press 'q' to quit.\n";
        cout << flush;
    }
};

int main() {
    SnakeGame game;
    // show intro and allow name entry + typing animation
    game.showIntro();
    game.run();
    return 0;
}
