#include <raylib.h>
#include <vector>
#include <stack>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>

const int SCREEN_WIDTH = 1536;
const int SCREEN_HEIGHT = 864;

int highestScore = 0;

class Cell {
private:
    int x, y;
    bool walls[4];

public:
    Cell(int x = 0, int y = 0) : x(x), y(y) {
        for (int i = 0; i < 4; i++) {
            walls[i] = true;
        }
    }

    void Draw(int cellSize, Color wallColor, int offsetX, int offsetY) {
        int screenX = x * cellSize + offsetX;
        int screenY = y * cellSize + offsetY;

        if (walls[0]) DrawLine(screenX, screenY, screenX + cellSize, screenY, wallColor);
        if (walls[1]) DrawLine(screenX + cellSize, screenY, screenX + cellSize, screenY + cellSize, wallColor);
        if (walls[2]) DrawLine(screenX, screenY + cellSize, screenX + cellSize, screenY + cellSize, wallColor);
        if (walls[3]) DrawLine(screenX, screenY, screenX, screenY + cellSize, wallColor);
    }

    void RemoveWall(int direction) {
        walls[direction] = false;
    }

    bool HasWall(int direction) const {
        return walls[direction];
    }
};

class Maze {
private:
    std::vector<std::vector<Cell>> grid;
    int width, height;
    int cellSize;
    std::vector<Vector2> solution;
    Texture2D startTexture;
    Texture2D endTexture;

    void GenerateMaze() {
        std::stack<std::pair<int, int>> stack;
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));

        stack.push({0, 0});
        visited[0][0] = true;

        while (!stack.empty()) {
            int x = stack.top().first;
            int y = stack.top().second;

            std::vector<int> directions = {0, 1, 2, 3};
            std::random_shuffle(directions.begin(), directions.end());

            bool moved = false;

            for (int direction : directions) {
                int newX = x + (direction == 1 ? 1 : (direction == 3 ? -1 : 0));
                int newY = y + (direction == 2 ? 1 : (direction == 0 ? -1 : 0));

                if (newX >= 0 && newX < width && newY >= 0 && newY < height && !visited[newY][newX]) {
                    grid[y][x].RemoveWall(direction);
                    grid[newY][newX].RemoveWall((direction + 2) % 4);

                    visited[newY][newX] = true;
                    stack.push({newX, newY});
                    moved = true;
                    break;
                }
            }

            if (!moved) {
                stack.pop();
            }
        }
    }

    void FindSolution() {
        std::queue<Vector2> queue;
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        std::vector<std::vector<Vector2>> parent(height, std::vector<Vector2>(width, {-1, -1}));

        queue.push({0, 0});
        visited[0][0] = true;

        while (!queue.empty()) {
            Vector2 current = queue.front();
            queue.pop();

            if (current.x == width - 1 && current.y == height - 1) {
                // Found the exit, reconstruct the path
                Vector2 pos = current;
                while (pos.x != -1 && pos.y != -1) {
                    solution.push_back(pos);
                    pos = parent[pos.y][pos.x];
                }
                std::reverse(solution.begin(), solution.end());
                return;
            }

            for (int direction = 0; direction < 4; direction++) {
                if (!grid[current.y][current.x].HasWall(direction)) {
                    int newX = current.x + (direction == 1 ? 1 : (direction == 3 ? -1 : 0));
                    int newY = current.y + (direction == 2 ? 1 : (direction == 0 ? -1 : 0));

                    if (!visited[newY][newX]) {
                        visited[newY][newX] = true;
                        parent[newY][newX] = current;
                        queue.push({(float)newX, (float)newY});
                    }
                }
            }
        }
    }

public:
    Maze(int width, int height, int cellSize) : width(width), height(height), cellSize(cellSize) {
        grid.resize(height, std::vector<Cell>(width));
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x] = Cell(x, y);
            }
        }
        GenerateMaze();
        FindSolution();
        startTexture = LoadTexture("src/start.png");
        endTexture = LoadTexture("src/end.png");
    }

    ~Maze() {
        UnloadTexture(startTexture);
        UnloadTexture(endTexture);
    }

    void Draw(Color wallColor, int offsetX, int offsetY) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].Draw(cellSize, wallColor, offsetX, offsetY);
            }
        }

        // Draw start icon
        DrawTextureEx(startTexture, {(float)offsetX, (float)offsetY}, 0, (float)cellSize / startTexture.width, WHITE);

        // Draw end icon
        DrawTextureEx(endTexture, 
            {(float)(offsetX + (width - 1) * cellSize), (float)(offsetY + (height - 1) * cellSize)}, 
            0, (float)cellSize / endTexture.width, WHITE);
    }

    void DrawSolution(int offsetX, int offsetY) {
        for (size_t i = 0; i < solution.size() - 1; i++) {
            Vector2 start = solution[i];
            Vector2 end = solution[i + 1];
            DrawLine(
                start.x * cellSize + cellSize / 2 + offsetX,
                start.y * cellSize + cellSize / 2 + offsetY,
                end.x * cellSize + cellSize / 2 + offsetX,
                end.y * cellSize + cellSize / 2 + offsetY,
                RED
            );
        }
    }

    bool CanMove(int x, int y, int direction) const {
        return !grid[y][x].HasWall(direction);
    }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    int GetCellSize() const { return cellSize; }
};

class Player {
private:
    float x, y;
    Texture2D texture;

public:
    Player(int startX, int startY, Texture2D playerTexture) : x(startX), y(startY), texture(playerTexture) {}

    void Draw(int cellSize, int offsetX, int offsetY) {
        float scale = (float)cellSize / std::max(texture.width, texture.height);
        DrawTextureEx(texture, {x * cellSize + offsetX, y * cellSize + offsetY}, 0, scale, WHITE);
    }

    void Move(float dx, float dy) {
        x += dx;
        y += dy;
    }

    int GetX() const { return static_cast<int>(x); }
    int GetY() const { return static_cast<int>(y); }
};

class Level {
private:
    int difficulty;
    int mazeSize;

public:
    Level(int difficulty) : difficulty(difficulty) {
        switch (difficulty) {
            case 1: mazeSize = 10; break;
            case 2: mazeSize = 15; break;
            case 3: mazeSize = 20; break;
            default: mazeSize = 10; break;
        }
    }

    int GetMazeSize() const { return mazeSize; }
};

enum class GameState {
    FIRST_SCREEN,
    CHARACTER_SELECTION,
    LEVEL_SELECTION,
    PLAYING,
    GAME_OVER,
    VICTORY
};

GameState currentState = GameState::FIRST_SCREEN;
int selectedCharacter = 0;
int selectedLevel = 0;
float gameTimer = 0.0f;
int lastScore = 0;
bool showSolution = false;

Maze* maze = nullptr;
Player* player = nullptr;
Level* level = nullptr;

Texture2D player3Texture;
Texture2D player1Texture;
Texture2D player2Texture;
Texture2D starWarsBackground;

Music spaceMusic;

void DrawFirstScreen() {
    DrawTexture(starWarsBackground, 0, 0, WHITE);

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

    if (CheckCollisionPointRec(GetMousePosition(), startButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentState = GameState::CHARACTER_SELECTION;
    }
    if (CheckCollisionPointRec(GetMousePosition(), exitButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        CloseWindow();
    }
}

void DrawCharacterSelection() {
    DrawTexture(starWarsBackground, 0, 0, WHITE);

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

    Rectangle player3Button = {(float)startX, 300, (float)buttonWidth, (float)buttonHeight};
    DrawRectangleRounded(player3Button, 0.2f, 10, DARKPURPLE);
    DrawTextureEx(player3Texture, 
        {startX + (buttonWidth - imageSize) / 2.0f, 300 + (buttonHeight - imageSize) / 2.0f}, 
        0, (float)imageSize / player3Texture.width, WHITE);

    Rectangle player1Button = {(float)(startX + buttonWidth + spacing), 300, (float)buttonWidth, (float)buttonHeight};
    DrawRectangleRounded(player1Button, 0.2f, 10, DARKBLUE);
    DrawTextureEx(player1Texture, 
        {startX + buttonWidth + spacing + (buttonWidth - imageSize) / 2.0f, 300 + (buttonHeight - imageSize) / 2.0f}, 
        0, (float)imageSize / player1Texture.width, WHITE);

    Rectangle player2Button = {(float)(startX + 2 * (buttonWidth + spacing)), 300, (float)buttonWidth, (float)buttonHeight};
    DrawRectangleRounded(player2Button, 0.2f, 10, DARKGREEN);
    DrawTextureEx(player2Texture, 
        {startX + 2 * (buttonWidth + spacing) + (buttonWidth - imageSize) / 2.0f, 300 + (buttonHeight - imageSize) / 2.0f}, 
        0, (float)imageSize / player2Texture.width, WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), player3Button) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedCharacter = 1;
        currentState = GameState::LEVEL_SELECTION;
    }
    if (CheckCollisionPointRec(GetMousePosition(), player1Button) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedCharacter = 2;
        currentState = GameState::LEVEL_SELECTION;
    }
    if (CheckCollisionPointRec(GetMousePosition(), player2Button) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedCharacter = 3;
        currentState = GameState::LEVEL_SELECTION;
    }
}

void DrawLevelSelection() {
    DrawTexture(starWarsBackground, 0, 0, WHITE);

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

    if (CheckCollisionPointRec(GetMousePosition(), easyButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedLevel = 1;
        currentState = GameState::PLAYING;
    }
    if (CheckCollisionPointRec(GetMousePosition(), mediumButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedLevel = 2;
        currentState = GameState::PLAYING;
    }
    if (CheckCollisionPointRec(GetMousePosition(), hardButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedLevel = 3;
        currentState = GameState::PLAYING;
    }

    if (currentState == GameState::PLAYING) {
        delete maze;
        delete player;
        delete level;

        level = new Level(selectedLevel);
        int mazeSize = level->GetMazeSize();
        int cellSize = std::min((SCREEN_WIDTH - 100) / mazeSize, (SCREEN_HEIGHT - 100) / mazeSize);
        maze = new Maze(mazeSize, mazeSize, cellSize);
        
        Texture2D playerTexture;
        switch (selectedCharacter) {
            case 1: playerTexture = player3Texture; break;
            case 2: playerTexture = player1Texture; break;
            case 3: playerTexture = player2Texture; break;
            default: playerTexture = player3Texture; break;
        }
        player = new Player(0, 0, playerTexture);
        
        gameTimer = 0.0f;
        showSolution = false;
    }
}

void DrawGameScreen() {
    DrawTexture(starWarsBackground, 0, 0, WHITE);

    if (maze && player) {
        gameTimer += GetFrameTime();

        int offsetX = (SCREEN_WIDTH - maze->GetWidth() * maze->GetCellSize()) / 2;
        int offsetY = (SCREEN_HEIGHT - maze->GetHeight() * maze->GetCellSize()) / 2;

        maze->Draw(LIGHTGRAY, offsetX, offsetY);
        if (showSolution) {
            maze->DrawSolution(offsetX, offsetY);
        }
        player->Draw(maze->GetCellSize(), offsetX, offsetY);

        DrawRectangle(0, 0, SCREEN_WIDTH, 50, Fade(BLACK, 0.5f));
        DrawText(TextFormat("Time: %.2f", gameTimer), 10, 10, 30, WHITE);

        if (IsKeyPressed(KEY_UP) && maze->CanMove(player->GetX(), player->GetY(), 0)) player->Move(0, -1);
        if (IsKeyPressed(KEY_RIGHT) && maze->CanMove(player->GetX(), player->GetY(), 1)) player->Move(1, 0);
        if (IsKeyPressed(KEY_DOWN) && maze->CanMove(player->GetX(), player->GetY(), 2)) player->Move(0, 1);
        if (IsKeyPressed(KEY_LEFT) && maze->CanMove(player->GetX(), player->GetY(), 3)) player->Move(-1, 0);

        if (IsKeyPressed(KEY_SPACE)) {
            showSolution = !showSolution;
        }

        if (player->GetX() == maze->GetWidth() - 1 && player->GetY() == maze->GetHeight() - 1) {
            currentState = GameState::VICTORY;
            lastScore = static_cast<int>(10000 / gameTimer);
            if (lastScore > highestScore) {
                highestScore = lastScore;
            }
        }
    }
}

void DrawVictoryScreen() {
    DrawTexture(starWarsBackground, 0, 0, WHITE);

    const char* victoryText = "Congratulations! You Won!";
    int victoryFontSize = 60;
    int victoryWidth = MeasureText(victoryText, victoryFontSize);
    int victoryX = (SCREEN_WIDTH - victoryWidth) / 2;

    DrawText(victoryText, victoryX, 100, victoryFontSize, GOLD);

    const char* scoreText = TextFormat("Your Score: %d", lastScore);
    int scoreFontSize = 40;
    int scoreWidth = MeasureText(scoreText, scoreFontSize);
    int scoreX = (SCREEN_WIDTH - scoreWidth) / 2;

    DrawText(scoreText, scoreX, 200, scoreFontSize, WHITE);

    int buttonWidth = 250, buttonHeight = 60;
    int buttonX = (SCREEN_WIDTH - buttonWidth) / 2;

    Rectangle playAgainButton = {(float)buttonX, 300, (float)buttonWidth, (float)buttonHeight};
    DrawRectangleRounded(playAgainButton, 0.2f, 10, DARKGREEN);
    DrawText("Play Again", buttonX + (buttonWidth - MeasureText("Play Again", 30)) / 2, 315, 30, WHITE);

    Rectangle mainMenuButton = {(float)buttonX, 400, (float)buttonWidth, (float)buttonHeight};
    DrawRectangleRounded(mainMenuButton, 0.2f, 10, MAROON);
    DrawText("Main Menu", buttonX + (buttonWidth - MeasureText("Main Menu", 30)) / 2, 415, 30, WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), playAgainButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentState = GameState::LEVEL_SELECTION;
    }
    if (CheckCollisionPointRec(GetMousePosition(), mainMenuButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentState = GameState::FIRST_SCREEN;
    }
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Star Wars Maze");
    SetTargetFPS(60);

    player3Texture = LoadTexture("src/player3.png");
    player1Texture = LoadTexture("src/player1.png");
    player2Texture = LoadTexture("src/player2.png");
    starWarsBackground = LoadTexture("src/star_wars.png");

    InitAudioDevice();
    spaceMusic = LoadMusicStream("src/music.mp3");
    PlayMusicStream(spaceMusic);

    while (!WindowShouldClose()) {
        UpdateMusicStream(spaceMusic);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentState) {
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
                DrawGameScreen();
                break;
            case GameState::VICTORY:
                DrawVictoryScreen();
                break;
        }

        EndDrawing();
    }

    UnloadTexture(player3Texture);
    UnloadTexture(player1Texture);
    UnloadTexture(player2Texture);
    UnloadTexture(starWarsBackground);
    UnloadMusicStream(spaceMusic);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}
