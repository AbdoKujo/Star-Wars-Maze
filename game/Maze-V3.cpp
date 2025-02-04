#include <raylib.h>
#include <vector>
#include <stack>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>
#include <queue>

// Constants
const int SCREEN_WIDTH = 1536;
const int SCREEN_HEIGHT = 864;
const float ENEMY_MOVE_INTERVAL = 1.0f;

int highestScore = 0;

// Forward declarations
class Enemy;
class Weapon;
class Maze;
class Player;
class Level;
class Game;

// Enums
enum class GameState {
    FIRST_SCREEN,
    CHARACTER_SELECTION,
    LEVEL_SELECTION,
    PLAYING,
    GAME_OVER,
    VICTORY
};

// Cell class
class Cell {
public:
    bool walls[4] = {true, true, true, true};  // top, right, bottom, left
    bool visited = false;
};

// Maze class
class Maze {
private:
    int width, height;
    int cellSize;
    std::vector<std::vector<Cell>> grid;
    Texture2D startTexture;
    Texture2D endTexture;
    int offsetX, offsetY;

public:
    Maze(int w, int h, int cSize) : width(w), height(h), cellSize(cSize) {
        grid.resize(height, std::vector<Cell>(width));
        offsetX = (SCREEN_WIDTH - width * cellSize) / 2;
        offsetY = (SCREEN_HEIGHT - height * cellSize) / 2;
    }

    void loadTextures(Texture2D start, Texture2D end) {
        startTexture = start;
        endTexture = end;
    }

    void generate() {
        std::stack<std::pair<int, int>> stack;
        stack.push({0, 0});
        grid[0][0].visited = true;

        while (!stack.empty()) {
            auto [x, y] = stack.top();
            std::vector<int> neighbors;

            if (y > 0 && !grid[y-1][x].visited) neighbors.push_back(0);
            if (x < width-1 && !grid[y][x+1].visited) neighbors.push_back(1);
            if (y < height-1 && !grid[y+1][x].visited) neighbors.push_back(2);
            if (x > 0 && !grid[y][x-1].visited) neighbors.push_back(3);

            if (!neighbors.empty()) {
                int next = neighbors[rand() % neighbors.size()];
                int nx = x + (next == 1 ? 1 : (next == 3 ? -1 : 0));
                int ny = y + (next == 2 ? 1 : (next == 0 ? -1 : 0));

                grid[y][x].walls[next] = false;
                grid[ny][nx].walls[(next + 2) % 4] = false;
                grid[ny][nx].visited = true;
                stack.push({nx, ny});
            } else {
                stack.pop();
            }
        }

        // Randomly remove some walls
        for (int i = 0; i < width * height / 10; ++i) {
            int x = rand() % width;
            int y = rand() % height;
            int wall = rand() % 4;
            if (grid[y][x].walls[wall]) {
                grid[y][x].walls[wall] = false;
                int nx = x + (wall == 1 ? 1 : (wall == 3 ? -1 : 0));
                int ny = y + (wall == 2 ? 1 : (wall == 0 ? -1 : 0));
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    grid[ny][nx].walls[(wall + 2) % 4] = false;
                }
            }
        }

        // After generating the maze and removing some walls, close the border
        closeBorderWalls();

        // Create entrance and exit
        grid[0][0].walls[3] = false;  // Open left wall of top-left cell
        grid[height-1][width-1].walls[1] = false;  // Open right wall of bottom-right cell
    }

    void closeBorderWalls() {
        // Close top and bottom walls
        for (int x = 0; x < width; ++x) {
            grid[0][x].walls[0] = true;  // Top wall
            grid[height-1][x].walls[2] = true;  // Bottom wall
        }
        // Close left and right walls
        for (int y = 0; y < height; ++y) {
            grid[y][0].walls[3] = true;  // Left wall
            grid[y][width-1].walls[1] = true;  // Right wall
        }
    }

    void draw() const {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int screenX = x * cellSize + offsetX;
                int screenY = y * cellSize + offsetY;
                if (grid[y][x].walls[0]) DrawLine(screenX, screenY, screenX + cellSize, screenY, WHITE);
                if (grid[y][x].walls[1]) DrawLine(screenX + cellSize, screenY, screenX + cellSize, screenY + cellSize, WHITE);
                if (grid[y][x].walls[2]) DrawLine(screenX, screenY + cellSize, screenX + cellSize, screenY + cellSize, WHITE);
                if (grid[y][x].walls[3]) DrawLine(screenX, screenY, screenX, screenY + cellSize, WHITE);
            }
        }

        // Draw start and end images
        float startScale = (float)(cellSize * 0.8) / std::max(startTexture.width, startTexture.height);
        float endScale = (float)(cellSize * 0.8) / std::max(endTexture.width, endTexture.height);
        
        float startX = offsetX + (cellSize - startTexture.width * startScale) / 2;
        float startY = offsetY + (cellSize - startTexture.height * startScale) / 2;
        float endX = offsetX + (width - 1) * cellSize + (cellSize - endTexture.width * endScale) / 2;
        float endY = offsetY + (height - 1) * cellSize + (cellSize - endTexture.height * endScale) / 2;
        
        DrawTextureEx(startTexture, {startX, startY}, 0, startScale, WHITE);
        DrawTextureEx(endTexture, {endX, endY}, 0, endScale, WHITE);
    }

    bool canMove(int x, int y, int direction) const {
        return !grid[y][x].walls[direction];
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getCellSize() const { return cellSize; }
    int getOffsetX() const { return offsetX; }
    int getOffsetY() const { return offsetY; }

    std::vector<std::pair<int, int>> findPath(int startX, int startY, int endX, int endY) const {
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        std::vector<std::vector<std::pair<int, int>>> parent(height, std::vector<std::pair<int, int>>(width, {-1, -1}));
        std::queue<std::pair<int, int>> q;
    
        q.push({startX, startY});
        visited[startY][startX] = true;
    
        const int dx[] = {0, 1, 0, -1};
        const int dy[] = {-1, 0, 1, 0};
    
        while (!q.empty()) {
            auto [x, y] = q.front();
            q.pop();
        
            if (x == endX && y == endY) {
                return reconstructPath(parent, startX, startY, endX, endY);
            }
        
            for (int i = 0; i < 4; ++i) {
                int nx = x + dx[i];
                int ny = y + dy[i];
            
                if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[ny][nx] && canMove(x, y, i)) {
                    visited[ny][nx] = true;
                    parent[ny][nx] = {x, y};
                    q.push({nx, ny});
                }
            }
        }
    
        return {};  // No path found
    }

private:
    std::vector<std::pair<int, int>> reconstructPath(const std::vector<std::vector<std::pair<int, int>>>& parent,
                                                     int startX, int startY, int endX, int endY) const {
        std::vector<std::pair<int, int>> path;
        int x = endX, y = endY;
    
        while (x != startX || y != startY) {
            path.push_back({x, y});
            auto [px, py] = parent[y][x];
            x = px;
            y = py;
        }
    
        path.push_back({startX, startY});
        std::reverse(path.begin(), path.end());
        return path;
    }

public:
    void drawPath(const std::vector<std::pair<int, int>>& path) const {
        if (path.empty()) return;

        for (size_t i = 0; i < path.size() - 1; ++i) {
            auto [x1, y1] = path[i];
            auto [x2, y2] = path[i + 1];
        
            float startX = x1 * cellSize + cellSize / 2 + offsetX;
            float startY = y1 * cellSize + cellSize / 2 + offsetY;
            float endX = x2 * cellSize + cellSize / 2 + offsetX;
            float endY = y2 * cellSize + cellSize / 2 + offsetY;
        
            DrawLineEx({startX, startY}, {endX, endY}, 3, YELLOW);
        }
    }
};

// Player class
class Player {
private:
    int x, y;
    int power;
    int score;
    int weaponsCollected;
    Texture2D texture;
    const Maze* maze;
    std::vector<std::pair<int, int>> currentPath; // Added member variable

public:
    Player(int startX, int startY, Texture2D playerTexture, const Maze* m) 
        : x(startX), y(startY), power(0), score(0), weaponsCollected(0), texture(playerTexture), maze(m) {}

    void move(int dx, int dy) {
        x += dx;
        y += dy;
    }

    void draw() const {
        int cellSize = maze->getCellSize();
        int offsetX = maze->getOffsetX();
        int offsetY = maze->getOffsetY();
        float scale = (float)(cellSize * 0.8) / std::max(texture.width, texture.height);
        float adjustedX = x * cellSize + offsetX + (cellSize - texture.width * scale) / 2;
        float adjustedY = y * cellSize + offsetY + (cellSize - texture.height * scale) / 2;
        DrawTextureEx(texture, {adjustedX, adjustedY}, 0, scale, WHITE);
    }

    void collectWeapon() {
        power += 10;
        weaponsCollected++;
    }

    void hitEnemy() {
        power -= 10;
        if (power < 0) power = 0;
    }

    void addScore(int points) {
        score += points;
    }

    int getX() const { return x; }
    int getY() const { return y; }
    int getPower() const { return power; }
    int getScore() const { return score; }
    int getWeaponsCollected() const { return weaponsCollected; }

    void setPath(const std::vector<std::pair<int, int>>& path) { // Added method
        currentPath = path;
    }

    const std::vector<std::pair<int, int>>& getPath() const { // Added method
        return currentPath;
    }

    void clearPath() { // Added method
        currentPath.clear();
    }
};

// Enemy class
class Enemy {
private:
    int x, y;
    int health;
    float moveTimer;
    const Maze* maze;
    Texture2D texture;

public:
    Enemy(int startX, int startY, const Maze* m, Texture2D enemyTexture) 
        : x(startX), y(startY), health(10), moveTimer(0), maze(m), texture(enemyTexture) {}

    void draw() const {
        int cellSize = maze->getCellSize();
        int offsetX = maze->getOffsetX();
        int offsetY = maze->getOffsetY();
        float scale = (float)(cellSize * 0.8) / std::max(texture.width, texture.height);
        float adjustedX = x * cellSize + offsetX + (cellSize - texture.width * scale) / 2;
        float adjustedY = y * cellSize + offsetY + (cellSize - texture.height * scale) / 2;
        DrawTextureEx(texture, {adjustedX, adjustedY}, 0, scale, WHITE);
    }

    void move(const Maze& maze) {
        moveTimer += GetFrameTime();
        if (moveTimer >= ENEMY_MOVE_INTERVAL) {
            moveTimer = 0;
            std::vector<int> possibleMoves;
            if (this->maze->canMove(x, y, 0)) possibleMoves.push_back(0);
            if (this->maze->canMove(x, y, 1)) possibleMoves.push_back(1);
            if (this->maze->canMove(x, y, 2)) possibleMoves.push_back(2);
            if (this->maze->canMove(x, y, 3)) possibleMoves.push_back(3);

            if (!possibleMoves.empty()) {
                int move = possibleMoves[rand() % possibleMoves.size()];
                switch (move) {
                    case 0: y--; break;
                    case 1: x++; break;
                    case 2: y++; break;
                    case 3: x--; break;
                }
            }
        }
    }

    bool isAlive() const { return health > 0; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getHealth() const { return health; }
    void damage(int amount) { health -= amount; }
};

// Weapon class
class Weapon {
private:
    int x, y;
    const Maze* maze;
    Texture2D texture;

public:
    Weapon(int startX, int startY, const Maze* m, Texture2D weaponTexture) 
        : x(startX), y(startY), maze(m), texture(weaponTexture) {}

    void draw() const {
        int cellSize = maze->getCellSize();
        int offsetX = maze->getOffsetX();
        int offsetY = maze->getOffsetY();
        float scale = (float)(cellSize * 0.6) / std::max(texture.width, texture.height);
        float adjustedX = x * cellSize + offsetX + (cellSize - texture.width * scale) / 2;
        float adjustedY = y * cellSize + offsetY + (cellSize - texture.height * scale) / 2;
        DrawTextureEx(texture, {adjustedX, adjustedY}, 0, scale, WHITE);
    }

    int getX() const { return x; }
    int getY() const { return y; }
};

// Level class
class Level {
private:
    int difficulty;
    int mazeSize;

public:
    Level(int diff) : difficulty(diff) {
        switch (difficulty) {
            case 1: mazeSize = 10; break;
            case 2: mazeSize = 15; break;
            case 3: mazeSize = 20; break;
            default: mazeSize = 10; break;
        }
    }

    int getMazeSize() const { return mazeSize; }
};

// Game class
class Game {
private:
    GameState state;
    Maze* maze;
    Player* player;
    std::vector<Enemy> enemies;
    std::vector<Weapon> weapons;
    Level* level;
    float timer;
    bool gameOver;
    bool showPath; // Added member variable

    Texture2D player1Texture;
    Texture2D player2Texture;
    Texture2D player3Texture;
    Texture2D starWarsBackground;
    Texture2D startTexture;
    Texture2D endTexture;
    Texture2D weaponTexture;
    Texture2D enemyTexture;
    Music backgroundMusic;

    int selectedCharacter;
    int selectedLevel;

public:
    Game() : state(GameState::FIRST_SCREEN), maze(nullptr), player(nullptr), level(nullptr),
             timer(0), gameOver(false), selectedCharacter(0), selectedLevel(0), showPath(false) {
        srand(time(nullptr));
        InitAudioDevice();
        LoadResources();
        PlayMusicStream(backgroundMusic);
    }

    ~Game() {
        UnloadResources();
        CloseAudioDevice();
    }

    void Run() {
        while (!WindowShouldClose()) {
            Update();
            Draw();
        }
    }

private:
    void LoadResources() {
        player1Texture = LoadTexture("src/player1.png");
        player2Texture = LoadTexture("src/player2.png");
        player3Texture = LoadTexture("src/player3.png");
        starWarsBackground = LoadTexture("src/star_wars.png");
        startTexture = LoadTexture("src/start.png");
        endTexture = LoadTexture("src/end.png");
        weaponTexture = LoadTexture("src/weapon.png");
        enemyTexture = LoadTexture("src/enemy.png");
        backgroundMusic = LoadMusicStream("src/music.mp3");
    }

    void UnloadResources() {
        UnloadTexture(player1Texture);
        UnloadTexture(player2Texture);
        UnloadTexture(player3Texture);
        UnloadTexture(starWarsBackground);
        UnloadTexture(startTexture);
        UnloadTexture(endTexture);
        UnloadTexture(weaponTexture);
        UnloadTexture(enemyTexture);
        UnloadMusicStream(backgroundMusic);
    }

    void Update() {
        UpdateMusicStream(backgroundMusic);

        switch (state) {
            case GameState::FIRST_SCREEN:
                UpdateFirstScreen();
                break;
            case GameState::CHARACTER_SELECTION:
                UpdateCharacterSelection();
                break;
            case GameState::LEVEL_SELECTION:
                UpdateLevelSelection();
                break;
            case GameState::PLAYING:
                UpdatePlaying();
                break;
            case GameState::GAME_OVER:
                UpdateGameOver();
                break;
            case GameState::VICTORY:
                UpdateVictory();
                break;
        }
    }

    void Draw() {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw background scaled to screen size
        DrawTexturePro(starWarsBackground, 
            Rectangle{ 0, 0, (float)starWarsBackground.width, (float)starWarsBackground.height },
            Rectangle{ 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT },
            Vector2{ 0, 0 }, 0.0f, WHITE);

        switch (state) {
            case GameState::FIRST_SCREEN:
                DrawFirstScreen();
                break;
            case GameState::CHARACTER_SELECTION:
                DrawCharacterSelection();
                break;
            case GameState::LEVEL_SELECTION:
                DrawLevelSelection();
                break;
            case GameState::PLAYING:
                DrawPlaying();
                break;
            case GameState::GAME_OVER:
                DrawGameOver();
                break;
            case GameState::VICTORY:
                DrawVictory();
                break;
        }

        EndDrawing();
    }

    void UpdateFirstScreen() {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            Rectangle startButton = {(float)(SCREEN_WIDTH - 200) / 2, 400, 200, 60};
            Rectangle exitButton = {(float)(SCREEN_WIDTH - 200) / 2, 500, 200, 60};

            if (CheckCollisionPointRec(mousePos, startButton)) {
                state = GameState::CHARACTER_SELECTION;
            } else if (CheckCollisionPointRec(mousePos, exitButton)) {
                CloseWindow();
            }
        }
    }

    void DrawFirstScreen() {
        const char* titleText = "Star Wars Maze";
        int titleFontSize = 70;
        int titleWidth = MeasureText(titleText, titleFontSize);
        int titleX = (SCREEN_WIDTH - titleWidth) / 2;

        DrawText(titleText, titleX, 100, titleFontSize, GOLD);

        const char* subtitleText = "Navigate through the maze to win!";
        int subtitleFontSize = 30;
        int subtitleWidth = MeasureText(subtitleText, subtitleFontSize);
        int subtitleX = (SCREEN_WIDTH - subtitleWidth) / 2;

        DrawText(subtitleText, subtitleX, 200, subtitleFontSize, RAYWHITE);

        int buttonWidth = 200, buttonHeight = 60;
        int buttonX = (SCREEN_WIDTH - buttonWidth) / 2;
        Rectangle startButton = {(float)buttonX, 400, (float)buttonWidth, (float)buttonHeight};
        DrawRectangleRounded(startButton, 0.2f, 10, DARKGREEN);
        DrawText("Start", buttonX + (buttonWidth - MeasureText("Start", 30)) / 2, 415, 30, WHITE);

        Rectangle exitButton = {(float)buttonX, 500, (float)buttonWidth, (float)buttonHeight};
        DrawRectangleRounded(exitButton, 0.2f, 10, MAROON);
        DrawText("Exit", buttonX + (buttonWidth - MeasureText("Exit", 30)) / 2, 515, 30, WHITE);

        const char* highScoreText = TextFormat("Highest Score: %d", highestScore);
        int highScoreFontSize = 30;
        int highScoreWidth = MeasureText(highScoreText, highScoreFontSize);
        int highScoreX = (SCREEN_WIDTH - highScoreWidth) / 2;
        DrawText(highScoreText, highScoreX, 600, highScoreFontSize, GOLD);
    }

    void UpdateCharacterSelection() {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            int buttonWidth = 100, buttonHeight = 100;
            int spacing = 50;
            int totalWidth = 3 * buttonWidth + 2 * spacing;
            int startX = (SCREEN_WIDTH - totalWidth) / 2;

            Rectangle player1Button = {(float)startX, 300, (float)buttonWidth, (float)buttonHeight};
            Rectangle player2Button = {(float)(startX + buttonWidth + spacing), 300, (float)buttonWidth, (float)buttonHeight};
            Rectangle player3Button = {(float)(startX + 2 * (buttonWidth + spacing)), 300, (float)buttonWidth, (float)buttonHeight};

            if (CheckCollisionPointRec(mousePos, player1Button)) {
                selectedCharacter = 1;
                state = GameState::LEVEL_SELECTION;
            } else if (CheckCollisionPointRec(mousePos, player2Button)) {
                selectedCharacter = 2;
                state = GameState::LEVEL_SELECTION;
            } else if (CheckCollisionPointRec(mousePos, player3Button)) {
                selectedCharacter = 3;
                state = GameState::LEVEL_SELECTION;
            }
        }
    }

    void DrawCharacterSelection() {
        const char* titleText = "Choose Your Character";
        int titleFontSize = 50;
        int titleWidth = MeasureText(titleText, titleFontSize);
        int titleX = (SCREEN_WIDTH - titleWidth) / 2;

        DrawText(titleText, titleX, 100, titleFontSize, GOLD);

        int buttonWidth = 100, buttonHeight = 100;
        int imageSize = 40;
        int spacing = 50;
        int totalWidth = 3 * buttonWidth + 2 * spacing;
        int startX = (SCREEN_WIDTH - totalWidth) / 2;

        Rectangle player1Button = {(float)startX, 300, (float)buttonWidth, (float)buttonHeight};
        DrawRectangleRounded(player1Button, 0.2f, 10, DARKPURPLE);
        DrawTextureEx(player1Texture, 
            {startX + (buttonWidth - imageSize) / 2.0f, 300 + (buttonHeight - imageSize) / 2.0f}, 
            0, (float)imageSize / player1Texture.width, WHITE);

        Rectangle player2Button = {(float)(startX + buttonWidth + spacing), 300, (float)buttonWidth, (float)buttonHeight};
        DrawRectangleRounded(player2Button, 0.2f, 10, DARKBLUE);
        DrawTextureEx(player2Texture, 
            {startX + buttonWidth + spacing + (buttonWidth - imageSize) / 2.0f, 300 + (buttonHeight - imageSize) / 2.0f}, 
            0, (float)imageSize / player2Texture.width, WHITE);

        Rectangle player3Button = {(float)(startX + 2 * (buttonWidth + spacing)), 300, (float)buttonWidth, (float)buttonHeight};
        DrawRectangleRounded(player3Button, 0.2f, 10, DARKGREEN);
        DrawTextureEx(player3Texture, 
            {startX + 2 * (buttonWidth + spacing) + (buttonWidth - imageSize) / 2.0f, 300 + (buttonHeight - imageSize) / 2.0f}, 
            0, (float)imageSize / player3Texture.width, WHITE);
    }

    void UpdateLevelSelection() {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            int buttonWidth = 200, buttonHeight = 60;
            int buttonX = (SCREEN_WIDTH - buttonWidth) / 2;

            Rectangle easyButton = {(float)buttonX, 300, (float)buttonWidth, (float)buttonHeight};
            Rectangle mediumButton = {(float)buttonX, 400, (float)buttonWidth, (float)buttonHeight};
            Rectangle hardButton = {(float)buttonX, 500, (float)buttonWidth, (float)buttonHeight};

            if (CheckCollisionPointRec(mousePos, easyButton)) {
                selectedLevel = 1;
                InitializeGame();
            } else if (CheckCollisionPointRec(mousePos, mediumButton)) {
                selectedLevel = 2;
                InitializeGame();
            } else if (CheckCollisionPointRec(mousePos, hardButton)) {
                selectedLevel = 3;
                InitializeGame();
            }
        }
    }

    void DrawLevelSelection() {
        const char* titleText = "Choose Difficulty";
        int titleFontSize = 50;
        int titleWidth = MeasureText(titleText, titleFontSize);
        int titleX = (SCREEN_WIDTH - titleWidth) / 2;

        DrawText(titleText, titleX, 100, titleFontSize, GOLD);

        int buttonWidth = 200, buttonHeight = 60;
        int buttonX = (SCREEN_WIDTH - buttonWidth) / 2;

        Rectangle easyButton = {(float)buttonX, 300, (float)buttonWidth, (float)buttonHeight};
        DrawRectangleRounded(easyButton, 0.2f, 10, DARKGREEN);
        DrawText("Easy", buttonX + (buttonWidth - MeasureText("Easy", 30)) /2, 315, 30, WHITE);

        Rectangle mediumButton = {(float)buttonX, 400, (float)buttonWidth, (float)buttonHeight};
        DrawRectangleRounded(mediumButton, 0.2f, 10, ORANGE);
        DrawText("Medium", buttonX + (buttonWidth - MeasureText("Medium", 30)) / 2, 415, 30, WHITE);

        Rectangle hardButton = {(float)buttonX, 500, (float)buttonWidth, (float)buttonHeight};
        DrawRectangleRounded(hardButton, 0.2f, 10, MAROON);
        DrawText("Hard", buttonX + (buttonWidth - MeasureText("Hard", 30)) / 2, 515, 30, WHITE);
    }

    void UpdatePlaying() {
        timer += GetFrameTime();

        // Player movement
        if (IsKeyPressed(KEY_UP) && maze->canMove(player->getX(), player->getY(), 0)) player->move(0, -1);
        if (IsKeyPressed(KEY_RIGHT) && maze->canMove(player->getX(), player->getY(), 1)) player->move(1, 0);
        if (IsKeyPressed(KEY_DOWN) && maze->canMove(player->getX(), player->getY(), 2)) player->move(0, 1);
        if (IsKeyPressed(KEY_LEFT) && maze->canMove(player->getX(), player->getY(), 3)) player->move(-1, 0);

        // Update enemies
        for (auto& enemy : enemies) {
            enemy.move(*maze);
        }

        // Check collisions
        CheckCollisions();

        // Check for victory
        if (player->getX() == maze->getWidth() - 1 && player->getY() == maze->getHeight() - 1) {
            state = GameState::VICTORY;
            int newScore = static_cast<int>(10000 / timer);
            if (newScore > highestScore) {
                highestScore = newScore;
            }
        }

        if (IsKeyPressed(KEY_S)) {
            showPath = !showPath;
            if (showPath) {
                std::vector<std::pair<int, int>> path = maze->findPath(player->getX(), player->getY(), maze->getWidth() - 1, maze->getHeight() - 1);
                player->setPath(path);
            } else {
                player->clearPath();
            }
        }
    }

    void DrawPlaying() {
        maze->draw();
        for (const auto& weapon : weapons) {
            weapon.draw();
        }
        for (const auto& enemy : enemies) {
            enemy.draw();
        }
        player->draw();

        DrawRectangle(0, 0, SCREEN_WIDTH, 50, Fade(BLACK, 0.5f));
        DrawText(TextFormat("Time: %.2f", timer), 10, 10, 30, WHITE);
        DrawText(TextFormat("Score: %d", player->getScore()), 200, 10, 30, WHITE);
        DrawText(TextFormat("Power: %d", player->getPower()), 400, 10, 30, WHITE);
        DrawText("Press 'S' to show/hide path", SCREEN_WIDTH - 250, 10, 20, YELLOW); // Added line
        
        // Draw the path
        if (showPath) {
            maze->drawPath(player->getPath());
        }
    }

    void UpdateGameOver() {
        if (IsKeyPressed(KEY_SPACE)) {
            RestartLevel();
        }
    }

    void DrawGameOver() {
        DrawText("Game Over!", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 30, 40, RED);
        DrawText("Press SPACE to restart", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 20, 30, WHITE);
    }

    void UpdateVictory() {
        if (IsKeyPressed(KEY_SPACE)) {
            state = GameState::FIRST_SCREEN;
        }
    }

    void DrawVictory() {
        DrawText("Victory!", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 30, 40, GREEN);
        DrawText(TextFormat("Score: %d", player->getScore()), SCREEN_WIDTH / 2 - 70, SCREEN_HEIGHT / 2 + 20, 30, WHITE);
        DrawText("Press SPACE to return to main menu", SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 + 70, 30, WHITE);
    }

    void CheckCollisions() {
        int playerX = player->getX();
        int playerY = player->getY();

        // Check weapon collisions
        weapons.erase(std::remove_if(weapons.begin(), weapons.end(),
            [this, playerX, playerY](const Weapon& weapon) {
                if (weapon.getX() == playerX && weapon.getY() == playerY) {
                    player->collectWeapon();
                    return true;
                }
                return false;
            }), weapons.end());

        // Check enemy collisions
        for (auto& enemy : enemies) {
            if (enemy.getX() == playerX && enemy.getY() == playerY) {
                if (player->getPower() > enemy.getHealth()) {
                    player->addScore(100);
                    enemy.damage(enemy.getHealth());
                    player->hitEnemy();  // Decrease player's power by 10
                } else {
                    state = GameState::GAME_OVER;
                    return;
                }
            }
        }

        // Remove defeated enemies
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),            [](const Enemy& enemy) { return !enemy.isAlive(); }), enemies.end());
    }

    void RestartLevel() {
        delete player;
        player = new Player(0, 0, GetPlayerTexture(), maze);
        
        enemies.clear();
        weapons.clear();
        GenerateWeaponsAndEnemies();

        timer = 0.0f;
        showPath = false; // Added line
        player->clearPath(); // Added line
        state = GameState::PLAYING;
    }

    void InitializeGame() {
        delete level;
        level = new Level(selectedLevel);
        int mazeSize = level->getMazeSize();
        int cellSize = std::min((SCREEN_WIDTH - 100) / mazeSize, (SCREEN_HEIGHT - 100) / mazeSize);
        
        delete maze;
        maze = new Maze(mazeSize, mazeSize, cellSize);
        maze->generate();
        maze->loadTextures(startTexture, endTexture);
        
        delete player;
        player = new Player(0, 0, GetPlayerTexture(), maze);
        
        GenerateWeaponsAndEnemies();

        timer = 0.0f;
        showPath = false; // Added line
        state = GameState::PLAYING;
    }

    void GenerateWeaponsAndEnemies() {
        int numWeapons = maze->getWidth() / 2;
        int numEnemies = maze->getWidth() / 3;

        for (int i = 0; i < numWeapons; ++i) {
            int x, y;
            do {
                x = rand() % maze->getWidth();
                y = rand() % maze->getHeight();
            } while ((x == 0 && y == 0) || (x == maze->getWidth() - 1 && y == maze->getHeight() - 1));
            weapons.emplace_back(x, y, maze, weaponTexture);
        }

        for (int i = 0; i < numEnemies; ++i) {
            int x, y;
            do {
                x = rand() % maze->getWidth();
                y = rand() % maze->getHeight();
            } while ((x == 0 && y == 0) || (x == maze->getWidth() - 1 && y == maze->getHeight() - 1));
            enemies.emplace_back(x, y, maze, enemyTexture);
        }
    }

    Texture2D GetPlayerTexture() {
        switch (selectedCharacter) {
            case 1: return player1Texture;
            case 2: return player2Texture;
            case 3: return player3Texture;
            default: return player1Texture;
        }
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Star Wars Maze");
    SetTargetFPS(60);

    Game game;
    game.Run();

    CloseWindow();
    return 0;
}

