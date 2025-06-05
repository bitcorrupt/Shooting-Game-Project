#include <iostream>
#include <vector>
#include <conio.h>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <algorithm> // For max() function

using namespace std;

// Game constants
const int WIDTH = 60;
const int HEIGHT = 25;
const char PLAYER = 'P';
const char ENEMY = 'E';
const char BOUNDARY = '#';
const char PLAYER_BULLET = '|';
const char ENEMY_BULLET = '.';
const int MAX_HEALTH = 5;

// Double buffering setup
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
CHAR_INFO* buffer = nullptr;
COORD bufferSize = {WIDTH + 2, HEIGHT + 5};
COORD bufferCoord = {0, 0};
SMALL_RECT writeRegion = {0, 0, WIDTH + 1, HEIGHT + 4};

// Game state
int playerX = WIDTH / 2;
int playerY = HEIGHT - 2;
int health = MAX_HEALTH;
int score = 0;
int level = 1;
int enemiesKilled = 0;
int wave = 1;
bool gameOver = false;
bool gameWon = false;
int enemyDir = 1;
int enemySpeed = 8;  // Faster movement (lower number = faster)
int enemyShootDelay = 15;  // More frequent shooting (lower number = more frequent)

// Game objects
vector<pair<int, int>> enemies;
vector<pair<int, int>> eBullets;
vector<pair<int, int>> pBullets;

// Function declarations
void ClearBuffer();
void WriteToBuffer(int x, int y, char c, int color);
void Setup();
void DrawTitle();
void DrawGameOver();
void DrawWin();
void DrawHUD();
void DrawBoundary(int y);
void DrawGameField();
void DrawInstructions();
void Draw();
void Input();
void SpawnNewWave();
void Logic();

void ClearBuffer() {
    for (int i = 0; i < bufferSize.X * bufferSize.Y; i++) {
        buffer[i].Char.AsciiChar = ' ';
        buffer[i].Attributes = 7;
    }
}

void WriteToBuffer(int x, int y, char c, int color) {
    if (x >= 0 && x < bufferSize.X && y >= 0 && y < bufferSize.Y) {
        buffer[y * bufferSize.X + x].Char.AsciiChar = c;
        buffer[y * bufferSize.X + x].Attributes = color;
    }
}

void Setup() {
    // Initialize buffer
    buffer = new CHAR_INFO[bufferSize.X * bufferSize.Y];
   
    // Initialize enemies in a grid pattern
    for (int y = 3; y <= 7; y += 2) {
        for (int x = 5; x < WIDTH - 5; x += 4) {
            enemies.push_back({x, y});
        }
    }
}

void DrawTitle() {
    string title = " CONSOLE SHOOTER ";
    int start = 3;
    for (int i = 0; i < start; i++) WriteToBuffer(i, 0, BOUNDARY, 7);
    for (size_t i = 0; i < title.size(); i++) WriteToBuffer(start + i, 0, title[i], 7);
    for (int i = start + title.size(); i < bufferSize.X; i++) WriteToBuffer(i, 0, BOUNDARY, 7);
}

void DrawGameOver() {
    string title = " GAME OVER ";
    int start = 3;
    for (int i = 0; i < start; i++) WriteToBuffer(i, 0, BOUNDARY, 7);
    for (size_t i = 0; i < title.size(); i++) WriteToBuffer(start + i, 0, title[i], 7);
    for (int i = start + title.size(); i < bufferSize.X; i++) WriteToBuffer(i, 0, BOUNDARY, 7);
}

void DrawWin() {
    string title = " YOU WIN! ";
    int start = 3;
    for (int i = 0; i < start; i++) WriteToBuffer(i, 0, BOUNDARY, 7);
    for (size_t i = 0; i < title.size(); i++) WriteToBuffer(start + i, 0, title[i], 7);
    for (int i = start + title.size(); i < bufferSize.X; i++) WriteToBuffer(i, 0, BOUNDARY, 7);
}

void DrawHUD() {
    int x = 1;
    string healthStr = " HEALTH: ";
    for (size_t i = 0; i < healthStr.size(); i++) WriteToBuffer(x + i, 1, healthStr[i], 7);
    x += healthStr.size();
   
    for (int i = 0; i < MAX_HEALTH; i++) {
        if (i < health) {
            WriteToBuffer(x, 1, '<', 12);
            WriteToBuffer(x + 1, 1, '3', 12);
            x += 2;
            WriteToBuffer(x, 1, ' ', 7);
            x += 1;
        } else {
            for (int j = 0; j < 3; j++) WriteToBuffer(x + j, 1, ' ', 7);
            x += 3;
        }
    }
   
    string scoreStr = " | SCORE: " + to_string(score);
    for (size_t i = 0; i < scoreStr.size(); i++) WriteToBuffer(x + i, 1, scoreStr[i], 7);
    x += scoreStr.size();
   
    string levelStr = " | LEVEL: " + to_string(level);
    for (size_t i = 0; i < levelStr.size(); i++) WriteToBuffer(x + i, 1, levelStr[i], 7);
    x += levelStr.size();
   
    string waveStr = " | WAVE: " + to_string(wave);
    for (size_t i = 0; i < waveStr.size(); i++) WriteToBuffer(x + i, 1, waveStr[i], 7);
    x += waveStr.size();
   
    for (; x < bufferSize.X - 1; x++) WriteToBuffer(x, 1, BOUNDARY, 7);
    WriteToBuffer(bufferSize.X - 1, 1, BOUNDARY, 7);
}

void DrawBoundary(int y) {
    for (int x = 0; x < bufferSize.X; x++) {
        WriteToBuffer(x, y, BOUNDARY, 7);
    }
}

void DrawGameField() {
    DrawBoundary(2);
    DrawBoundary(HEIGHT + 3);
   
    for (int y = 0; y < HEIGHT; y++) {
        WriteToBuffer(0, y + 3, BOUNDARY, 7);
        WriteToBuffer(bufferSize.X - 1, y + 3, BOUNDARY, 7);
       
        for (int x = 1; x < bufferSize.X - 1; x++) {
            if (x - 1 == playerX && y == playerY) {
                WriteToBuffer(x, y + 3, PLAYER, 10);
                continue;
            }
           
            bool enemyDrawn = false;
            for (auto& e : enemies) {
                if (e.first == x - 1 && e.second == y) {
                    WriteToBuffer(x, y + 3, ENEMY, 12);
                    enemyDrawn = true;
                    break;
                }
            }
            if (enemyDrawn) continue;
           
            bool bulletDrawn = false;
            for (auto& b : pBullets) {
                if (b.first == x - 1 && b.second == y) {
                    WriteToBuffer(x, y + 3, PLAYER_BULLET, 11);
                    bulletDrawn = true;
                    break;
                }
            }
            if (bulletDrawn) continue;
           
            for (auto& b : eBullets) {
                if (b.first == x - 1 && b.second == y) {
                    WriteToBuffer(x, y + 3, ENEMY_BULLET, 14);
                    bulletDrawn = true;
                    break;
                }
            }
            if (bulletDrawn) continue;
           
            WriteToBuffer(x, y + 3, ' ', 7);
        }
    }
}

void DrawInstructions() {
    if (!gameOver && !gameWon) {
        string instructions = " CONTROLS: A=Left, D=Right, SPACE=Shoot, Q=Quit ";
        int x = 1;
        for (size_t i = 0; i < instructions.size(); i++) {
            WriteToBuffer(x + i, HEIGHT + 4, instructions[i], 7);
        }
        for (int i = instructions.size() + 1; i < bufferSize.X - 1; i++) {
            WriteToBuffer(i, HEIGHT + 4, ' ', 7);
        }
    } else {
        string scoreText = " FINAL SCORE: " + to_string(score) + " | ENEMIES KILLED: " + to_string(enemiesKilled);
        int x = 1;
        for (size_t i = 0; i < scoreText.size(); i++) {
            WriteToBuffer(x + i, HEIGHT + 4, scoreText[i], 7);
        }
        for (int i = scoreText.size() + 1; i < bufferSize.X - 1; i++) {
            WriteToBuffer(i, HEIGHT + 4, ' ', 7);
        }
    }
    WriteToBuffer(0, HEIGHT + 4, BOUNDARY, 7);
    WriteToBuffer(bufferSize.X - 1, HEIGHT + 4, BOUNDARY, 7);
}

void Draw() {
    ClearBuffer();
   
    if (gameOver && !gameWon) DrawGameOver();
    else if (gameWon) DrawWin();
    else DrawTitle();
   
    DrawHUD();
    DrawGameField();
    DrawInstructions();
   
    WriteConsoleOutput(hConsole, buffer, bufferSize, bufferCoord, &writeRegion);
}

void Input() {
    if (_kbhit()) {
        char ch = _getch();
        switch (ch) {
            case 'a': if (playerX > 1) playerX--; break;
            case 'd': if (playerX < WIDTH - 2) playerX++; break;
            case ' ': pBullets.push_back({playerX, playerY - 1}); break;
            case 'q': gameOver = true; break;
        }
    }
}

void SpawnNewWave() {
    wave++;
    enemySpeed = max(3, enemySpeed - 1);  // Faster enemies each wave (minimum speed of 3)
    enemyShootDelay = max(5, enemyShootDelay - 2);  // More frequent shooting (minimum delay of 5)
   
    for (int y = 3; y <= 7; y += 2) {
        for (int x = 5; x < WIDTH - 5; x += 4) {
            enemies.push_back({x, y});
        }
    }
   
    if (wave % 3 == 0) level++;
}

void Logic() {
    // More frequent enemy shooting
    if (rand() % enemyShootDelay == 0 && !enemies.empty()) {
        int idx = rand() % enemies.size();
        eBullets.push_back({enemies[idx].first, enemies[idx].second + 1});
    }

    // Move bullets
    for (auto& b : pBullets) b.second--;
    for (auto& b : eBullets) b.second++;
   
    // Faster enemy movement
    static int enemyMoveCounter = 0;
    if (++enemyMoveCounter >= enemySpeed) {
        enemyMoveCounter = 0;
        bool changeDir = false;
        for (auto& e : enemies) {
            e.first += enemyDir;
            if (e.first <= 1 || e.first >= WIDTH - 2) changeDir = true;
        }
        if (changeDir) {
            enemyDir *= -1;
            for (auto& e : enemies) e.second++;
        }
    }

    // Bullet-enemy collisions
    for (int i = 0; i < pBullets.size(); i++) {
        for (int j = 0; j < enemies.size(); j++) {
            if (pBullets[i].first == enemies[j].first && pBullets[i].second == enemies[j].second) {
                enemies.erase(enemies.begin() + j);
                pBullets.erase(pBullets.begin() + i);
                score += 10;
                enemiesKilled++;
                if (enemies.empty()) SpawnNewWave();
                return;
            }
        }
    }

    // Bullet-player collisions
    for (int i = 0; i < eBullets.size(); i++) {
        if (eBullets[i].first == playerX && eBullets[i].second == playerY) {
            eBullets.erase(eBullets.begin() + i);
            health--;
            if (health <= 0) gameOver = true;
        }
       
        if (eBullets[i].second >= HEIGHT) {
            eBullets.erase(eBullets.begin() + i);
            i--;
        }
    }

    // Clean up out-of-bound player bullets
    for (int i = 0; i < pBullets.size(); i++) {
        if (pBullets[i].second <= 0) {
            pBullets.erase(pBullets.begin() + i);
            i--;
        }
    }

    // Game over if enemies reach bottom
    for (auto& e : enemies) {
        if (e.second >= HEIGHT - 1) {
            gameOver = true;
            break;
        }
    }
   
    // Win condition
    if (score >= 100) {
        gameWon = true;
        gameOver = true;
    }
}

int main() {
    srand(time(0));
    Setup();
   
    SetConsoleTitleA("Console Shooter Game");
    SetConsoleScreenBufferSize(hConsole, {static_cast<SHORT>(bufferSize.X), static_cast<SHORT>(bufferSize.Y)});
   
    while (!gameOver) {
        Draw();
        Input();
        Logic();
        Sleep(50);
    }
   
    Draw();
   
    delete[] buffer;
    cout << "\n\n  Press any key to exit...";
    _getch();
    return 0;
}