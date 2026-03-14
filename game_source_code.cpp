#include <fstream>
/*
 * LOCAL REFEREE for WinterChallenge2026 (SnakeBird)
 *
 * Usage: ./referee ./bot1 ./bot2 [seed]
 *
 * Communicates with two bot processes via stdin/stdout pipes,
 * using the exact same protocol as the CodinGame referee.
 * Logs each turn's state to stderr for analysis.
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <random>
#include <set>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>

using namespace std;

// ═══════════════════════════════════════════════════════
// Grid & Game structures
// ═══════════════════════════════════════════════════════
struct Coord {
    int x, y;
    bool operator==(const Coord& o) const { return x == o.x && y == o.y; }
    bool operator<(const Coord& o) const { return y < o.y || (y == o.y && x < o.x); }
    Coord add(int dx, int dy) const { return {x + dx, y + dy}; }
    int manhattan(const Coord& o) const { return abs(x - o.x) + abs(y - o.y); }
};

enum Dir { NORTH, EAST, SOUTH, WEST, UNSET };
const int DX[] = {0, 1, 0, -1, 0};
const int DY[] = {-1, 0, 1, 0, 0};
const char* DIR_STR[] = {"UP", "RIGHT", "DOWN", "LEFT", "UNSET"};

Dir parseDir(const string& s) {
    if (s == "UP" || s == "N")    return NORTH;
    if (s == "DOWN" || s == "S")  return SOUTH;
    if (s == "LEFT" || s == "W")  return WEST;
    if (s == "RIGHT" || s == "E") return EAST;
    return UNSET;
}

Dir oppositeDir(Dir d) {
    switch(d) {
        case NORTH: return SOUTH;
        case SOUTH: return NORTH;
        case EAST:  return WEST;
        case WEST:  return EAST;
        default:    return UNSET;
    }
}

struct Bird {
    int id;
    int owner; // 0 or 1
    bool alive;
    vector<Coord> body; // head = body[0]
    Dir direction;

    Coord headPos() const { return body[0]; }

    Dir getFacing() const {
        if (body.size() < 2) return UNSET;
        int dx = body[0].x - body[1].x;
        int dy = body[0].y - body[1].y;
        if (dy == -1) return NORTH;
        if (dy == 1) return SOUTH;
        if (dx == 1) return EAST;
        if (dx == -1) return WEST;
        return UNSET;
    }
};

struct Game {
    int width, height;
    vector<vector<int>> grid; // 0=empty, 1=wall
    vector<Coord> apples;
    vector<Bird> birds;
    int losses[2] = {0, 0};
    int turn = 0;

    bool inBounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < width && y < height;
    }

    bool isWall(int x, int y) const {
        if (!inBounds(x, y)) return true;
        return grid[y][x] == 1;
    }

    vector<Bird*> liveBirds() {
        vector<Bird*> result;
        for (auto& b : birds) if (b.alive) result.push_back(&b);
        return result;
    }

    Coord opposite(Coord c) const {
        return {width - c.x - 1, c.y};
    }

    // ── doMoves ──
    void doMoves() {
        for (auto& bird : birds) {
            if (!bird.alive) continue;
            if (bird.direction == UNSET) bird.direction = bird.getFacing();

            Coord newHead = bird.headPos().add(DX[bird.direction], DY[bird.direction]);
            bool willEat = false;
            for (auto& a : apples) {
                if (a == newHead) { willEat = true; break; }
            }

            if (!willEat) {
                bird.body.pop_back(); // remove tail
            }
            bird.body.insert(bird.body.begin(), newHead); // add head
        }
    }

    // ── doEats ──
    void doEats() {
        set<int> eaten;
        for (auto& bird : birds) {
            if (!bird.alive) continue;
            for (int i = 0; i < (int)apples.size(); i++) {
                if (apples[i] == bird.headPos()) {
                    eaten.insert(i);
                }
            }
        }
        // Remove eaten apples (reverse order)
        vector<int> toRemove(eaten.begin(), eaten.end());
        sort(toRemove.rbegin(), toRemove.rend());
        for (int idx : toRemove) {
            apples.erase(apples.begin() + idx);
        }
    }

    // ── doBeheadings ──
    void doBeheadings() {
        vector<int> toBehead;
        auto live = liveBirds();

        for (auto* bird : live) {
            Coord head = bird->headPos();
            bool isInWall = inBounds(head.x, head.y) && grid[head.y][head.x] == 1;
            bool isInBird = false;

            for (auto* other : live) {
                if (!count(other->body.begin(), other->body.end(), head)) {
                    continue;
                }
                if (other->id != bird->id) {
                    isInBird = true;
                    break;
                }
                if (count(other->body.begin() + 1, other->body.end(), head)) {
                    isInBird = true;
                    break;
                }
            }

            if (isInWall || isInBird) toBehead.push_back(bird->id);
        }

        for (int bid : toBehead) {
            for (auto& b : birds) {
                if (b.id != bid || !b.alive) continue;
                if ((int)b.body.size() <= 3) {
                    // Die
                    losses[b.owner] += b.body.size();
                    b.alive = false;
                } else {
                    // Behead (remove head)
                    b.body.erase(b.body.begin());
                    losses[b.owner]++;
                }
            }
        }
    }

    // ── somethingSolidUnder ──
    bool somethingSolidUnder(Coord c, const vector<Coord>& ignoreBody) {
        Coord below = c.add(0, 1);
        for (auto& ig : ignoreBody) if (ig == below) return false;
        if (inBounds(below.x, below.y) && grid[below.y][below.x] == 1) return true;
        for (auto* b : liveBirds()) {
            for (auto& part : b->body) {
                if (part == below) return true;
            }
        }
        for (auto& a : apples) {
            if (a == below) return true;
        }
        return false;
    }

    // ── birdsAreTouching ──
    bool birdsAreTouching(Bird* a, Bird* b) {
        for (auto& c1 : a->body) {
            for (auto& c2 : b->body) {
                if (c1.manhattan(c2) == 1) return true;
            }
        }
        return false;
    }

    // ── doFalls ──
    void doFalls() {
        bool somethingFell = true;
        while (somethingFell) {
            // Individual falls
            while (somethingFell) {
                somethingFell = false;
                for (auto& bird : birds) {
                    if (!bird.alive) continue;
                    bool canFall = true;
                    for (auto& c : bird.body) {
                        if (somethingSolidUnder(c, bird.body)) {
                            canFall = false;
                            break;
                        }
                    }
                    if (canFall) {
                        somethingFell = true;
                        for (auto& c : bird.body) c.y++;
                        // OOB check
                        bool allOut = true;
                        for (auto& c : bird.body) {
                            if (c.y < height + 1) { allOut = false; break; }
                        }
                        if (allOut) bird.alive = false;
                    }
                }
            }
            // Intercoiled falls
            somethingFell = doIntercoiledFalls();
        }
    }

    bool doIntercoiledFalls() {
        bool fell = false;
        bool somethingFell = true;
        while (somethingFell) {
            somethingFell = false;
            // Find intercoiled groups
            auto live = liveBirds();
            vector<bool> visited(live.size(), false);
            for (int i = 0; i < (int)live.size(); i++) {
                if (visited[i]) continue;
                vector<int> group;
                vector<int> stack = {i};
                while (!stack.empty()) {
                    int cur = stack.back(); stack.pop_back();
                    if (visited[cur]) continue;
                    visited[cur] = true;
                    group.push_back(cur);
                    for (int j = 0; j < (int)live.size(); j++) {
                        if (!visited[j] && birdsAreTouching(live[cur], live[j])) {
                            stack.push_back(j);
                        }
                    }
                }
                if (group.size() <= 1) continue;

                // Build meta-body
                vector<Coord> metaBody;
                for (int idx : group) {
                    for (auto& c : live[idx]->body) metaBody.push_back(c);
                }

                bool canFall = true;
                for (auto& c : metaBody) {
                    if (somethingSolidUnder(c, metaBody)) {
                        canFall = false; break;
                    }
                }
                if (canFall) {
                    somethingFell = true;
                    fell = true;
                    for (int idx : group) {
                        for (auto& c : live[idx]->body) c.y++;
                        if (live[idx]->headPos().y >= height) {
                            live[idx]->alive = false;
                        }
                    }
                }
            }
        }
        return fell;
    }

    // ── Full turn ──
    void performTurn() {
        doMoves();
        doEats();
        doBeheadings();
        doFalls();
    }

    bool isGameOver() {
        if (apples.empty()) return true;
        for (int p = 0; p < 2; p++) {
            bool hasAlive = false;
            for (auto& b : birds) {
                if (b.owner == p && b.alive) { hasAlive = true; break; }
            }
            if (!hasAlive) return true;
        }
        return false;
    }

    // Score = sum of alive birds' body size
    int scoreFor(int player) {
        int s = 0;
        for (auto& b : birds) {
            if (b.owner == player && b.alive) s += b.body.size();
        }
        return s;
    }
};

// ═══════════════════════════════════════════════════════
// Grid generation (simplified C++ port of GridMaker.java)
// ═══════════════════════════════════════════════════════

int g_override_height = -1;
int g_override_snakes = -1;

void generateGrid(Game& game, mt19937& rng) {
    uniform_real_distribution<double> rdist(0.0, 1.0);
    double skew = 0.3;
    int height = 10 + (int)llround(pow(rdist(rng), skew) * (24 - 10));
    if (g_override_height > 0) {
        height = g_override_height;
    }
    int width = (int)(height * 1.8f);
    if (width % 2 != 0) width++;

    game.width = width;
    game.height = height;
    game.grid.assign(height, vector<int>(width, 0));

    // Bottom row = wall
    for (int x = 0; x < width; x++) game.grid[height-1][x] = 1;

    // Procedural generation
    double b = 5 + rdist(rng) * 10;

    for (int y = height - 2; y >= 0; y--) {
        double yNorm = (double)(height - 1 - y) / (height - 1);
        double blockChance = 1.0 / (yNorm + 0.1) / b;
        for (int x = 0; x < width; x++) {
            if (rdist(rng) < blockChance) {
                game.grid[y][x] = 1;
            }
        }
    }

    // Horizontal symmetry
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int ox = width - 1 - x;
            game.grid[y][ox] = game.grid[y][x];
        }
    }

    // Fill small islands (BFS flood fill)
    vector<vector<bool>> visited(height, vector<bool>(width, false));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (visited[y][x] || game.grid[y][x] == 1) { visited[y][x] = true; continue; }
            vector<Coord> island;
            vector<Coord> stack = {{x, y}};
            visited[y][x] = true;
            while (!stack.empty()) {
                Coord c = stack.back(); stack.pop_back();
                island.push_back(c);
                for (int d = 0; d < 4; d++) {
                    int nx = c.x + DX[d], ny = c.y + DY[d];
                    if (nx >= 0 && ny >= 0 && nx < width && ny < height && !visited[ny][nx] && game.grid[ny][nx] == 0) {
                        visited[ny][nx] = true;
                        stack.push_back({nx, ny});
                    }
                }
            }
            if ((int)island.size() < 10) {
                for (auto& c : island) game.grid[c.y][c.x] = 1;
            }
        }
    }

    // Re-symmetrize after island fill
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            int ox = width - 1 - x;
            game.grid[y][ox] = game.grid[y][x];
        }
    }

    bool somethingDestroyed = true;
    while (somethingDestroyed) {
        somethingDestroyed = false;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (game.grid[y][x] == 1) continue;

                vector<Coord> neighbourWalls;
                for (int d = 0; d < 4; d++) {
                    int nx = x + DX[d];
                    int ny = y + DY[d];
                    if (nx >= 0 && ny >= 0 && nx < width && ny < height && game.grid[ny][nx] == 1) {
                        neighbourWalls.push_back({nx, ny});
                    }
                }
                if (neighbourWalls.size() < 3) continue;

                vector<Coord> destroyable;
                for (const Coord& wall : neighbourWalls) {
                    if (wall.y <= y) {
                        destroyable.push_back(wall);
                    }
                }
                if (destroyable.empty()) continue;

                shuffle(destroyable.begin(), destroyable.end(), rng);
                Coord destroyed = destroyable.front();
                game.grid[destroyed.y][destroyed.x] = 0;
                Coord opp = game.opposite(destroyed);
                game.grid[opp.y][opp.x] = 0;
                somethingDestroyed = true;
            }
        }
    }

    vector<Coord> lowestIsland;
    if (game.grid[height - 1][0] == 1) {
        vector<vector<bool>> seen(height, vector<bool>(width, false));
        vector<Coord> queue = {{0, height - 1}};
        seen[height - 1][0] = true;
        for (size_t index = 0; index < queue.size(); index++) {
            Coord c = queue[index];
            lowestIsland.push_back(c);
            for (int d = 0; d < 4; d++) {
                int nx = c.x + DX[d];
                int ny = c.y + DY[d];
                if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                if (seen[ny][nx] || game.grid[ny][nx] != 1) continue;
                seen[ny][nx] = true;
                queue.push_back({nx, ny});
            }
        }
    }

    int lowerBy = 0;
    bool canLower = !lowestIsland.empty();
    while (canLower) {
        for (int x = 0; x < width; x++) {
            Coord c = {x, height - 1 - (lowerBy + 1)};
            if (!count(lowestIsland.begin(), lowestIsland.end(), c)) {
                canLower = false;
                break;
            }
        }
        if (canLower) lowerBy++;
    }
    if (lowerBy >= 2) {
        uniform_int_distribution<int> lowerDist(2, lowerBy);
        lowerBy = lowerDist(rng);
    }
    if (lowerBy > 0) {
        for (const Coord& c : lowestIsland) {
            game.grid[c.y][c.x] = 0;
            Coord opp = game.opposite(c);
            game.grid[opp.y][opp.x] = 0;
        }
        for (const Coord& c : lowestIsland) {
            Coord lowered = {c.x, c.y + lowerBy};
            if (lowered.x >= 0 && lowered.y >= 0 && lowered.x < width && lowered.y < height) {
                game.grid[lowered.y][lowered.x] = 1;
                Coord opp = game.opposite(lowered);
                game.grid[opp.y][opp.x] = 1;
            }
        }
    }

    // Spawn apples (2.5% chance on left half, mirrored)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            if (game.grid[y][x] == 0 && rdist(rng) < 0.025) {
                game.apples.push_back({x, y});
                game.apples.push_back({width - 1 - x, y});
            }
        }
    }

    // Convert lone walls to apples (8-neighbor check)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (game.grid[y][x] != 1) continue;
            int wallNeighbors = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && ny >= 0 && nx < width && ny < height && game.grid[ny][nx] == 1) {
                        wallNeighbors++;
                    }
                }
            }
            if (wallNeighbors == 0) {
                game.grid[y][x] = 0;
                int ox = width - 1 - x;
                game.grid[y][ox] = 0;
                game.apples.push_back({x, y});
                if (ox != x) game.apples.push_back({ox, y});
            }
        }
    }

    // Deduplicate apples
    sort(game.apples.begin(), game.apples.end());
    game.apples.erase(unique(game.apples.begin(), game.apples.end()), game.apples.end());
    // Remove any apple on a wall
    game.apples.erase(remove_if(game.apples.begin(), game.apples.end(), [&](const Coord& c) {
        return game.grid[c.y][c.x] == 1;
    }), game.apples.end());

    // Find spawn locations: walls with 3 free cells above
    vector<Coord> spawns;
    for (int y = 3; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            if (game.grid[y][x] != 1) continue;
            bool ok = true;
            for (int dy = 1; dy <= 3; dy++) {
                if (y - dy < 0 || game.grid[y - dy][x] != 0) { ok = false; break; }
            }
            if (ok) spawns.push_back({x, y});
        }
    }

    shuffle(spawns.begin(), spawns.end(), rng);

    int desiredSpawns = 4;
    if (height <= 15) desiredSpawns--;
    if (height <= 10) desiredSpawns--;
    desiredSpawns = min(desiredSpawns, (int)spawns.size());

    // Create birds: each spawn = 1 bird per player (3 body parts above spawn wall)
    int birdId = 0;
    vector<vector<Coord>> spawnBodies;
    set<Coord> usedSpawnCells;

    for (auto& sp : spawns) {
        if ((int)spawnBodies.size() >= desiredSpawns) break;

        // Check not too close to center or existing spawns
        bool tooClose = false;
        vector<Coord> body;
        for (int dy = 1; dy <= 3; dy++) {
            Coord c = {sp.x, sp.y - dy};
            if (c.x == width / 2 - 1 || c.x == width / 2) { tooClose = true; break; }
            // 8-neighbor check against used spawn cells
            for (auto& used : usedSpawnCells) {
                if (abs(c.x - used.x) <= 1 && abs(c.y - used.y) <= 1) {
                    tooClose = true; break;
                }
            }
            if (tooClose) break;
            body.push_back(c);
        }
        if (tooClose || (int)body.size() < 3) continue;

        spawnBodies.push_back(body);
        for (auto& c : body) usedSpawnCells.insert(c);
        // Remove apples at spawn positions
        for (auto& c : body) {
            game.apples.erase(remove_if(game.apples.begin(), game.apples.end(), [&](const Coord& a) {
                return a == c || a == game.opposite(c);
            }), game.apples.end());
        }
    }

    // Ensure at least 1 spawn
    if (spawnBodies.empty()) {
        // Fallback: find any wall with space above
        for (int y = 3; y < height; y++) {
            for (int x = 1; x < width / 2 - 1; x++) {
                if (game.grid[y][x] != 1) continue;
                bool ok = true;
                for (int dy = 1; dy <= 3; dy++) {
                    if (game.grid[y-dy][x] != 0) { ok = false; break; }
                }
                if (ok) {
                    vector<Coord> body;
                    for (int dy = 1; dy <= 3; dy++) body.push_back({x, y-dy});
                    spawnBodies.push_back(body);
                    break;
                }
            }
            if (!spawnBodies.empty()) break;
        }
    }

    // Create birds from spawns
    for (auto& body : spawnBodies) {
        // Player 0
        Bird b0;
        b0.id = birdId++;
        b0.owner = 0;
        b0.alive = true;
        b0.body = body;
        b0.direction = UNSET;
        game.birds.push_back(b0);

        // Player 1 (mirrored)
        Bird b1;
        b1.id = birdId++;
        b1.owner = 1;
        b1.alive = true;
        for (auto& c : body) b1.body.push_back(game.opposite(c));
        b1.direction = UNSET;
        game.birds.push_back(b1);
    }

    // Ensure enough apples
    if ((int)game.apples.size() < 5) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width / 2; x++) {
                if (game.grid[y][x] == 0 && rdist(rng) < 0.08) {
                    bool onBird = false;
                    for (auto& b : game.birds) {
                        for (auto& c : b.body) {
                            if (c == Coord{x,y} || c == game.opposite(Coord{x,y})) { onBird = true; break; }
                        }
                        if (onBird) break;
                    }
                    if (!onBird) {
                        game.apples.push_back({x, y});
                        game.apples.push_back({width - 1 - x, y});
                    }
                }
            }
        }
        sort(game.apples.begin(), game.apples.end());
        game.apples.erase(unique(game.apples.begin(), game.apples.end()), game.apples.end());
    }
}

// ═══════════════════════════════════════════════════════
// Process management — fork + pipe communication
// ═══════════════════════════════════════════════════════
struct BotProcess {
    pid_t pid;
    int stdin_fd;   // write to bot
    int stdout_fd;  // read from bot
    FILE* write_fp;
    FILE* read_fp;

    void launch(const char* cmd) {
        int in_pipe[2], out_pipe[2];
        pipe(in_pipe);
        pipe(out_pipe);

        pid = fork();
        if (pid == 0) {
            // Child
            close(in_pipe[1]);
            close(out_pipe[0]);
            dup2(in_pipe[0], STDIN_FILENO);
            dup2(out_pipe[1], STDOUT_FILENO);
            close(in_pipe[0]);
            close(out_pipe[1]);
            execl("/bin/sh", "sh", "-c", (string("exec ") + cmd).c_str(), nullptr);
            _exit(1);
        }
        // Parent
        close(in_pipe[0]);
        close(out_pipe[1]);
        stdin_fd = in_pipe[1];
        stdout_fd = out_pipe[0];
        write_fp = fdopen(stdin_fd, "w");
        read_fp = fdopen(stdout_fd, "r");
        setlinebuf(write_fp);
    }

    void sendLine(const string& line) {
        fprintf(write_fp, "%s\n", line.c_str());
        fflush(write_fp);
    }

    string readLine(int timeout_ms = 5000) {
        // Use poll for timeout
        struct pollfd pfd;
        pfd.fd = stdout_fd;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, timeout_ms);
        if (ret <= 0) {
            return "__TIMEOUT__";
        }

        char buf[4096];
        if (fgets(buf, sizeof(buf), read_fp) == nullptr) {
            return "__EOF__";
        }
        string line(buf);
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            line.pop_back();
        return line;
    }

    void kill_proc() {
        if (pid > 0) {
            // Close pipes first so bot gets EOF/SIGPIPE
            if (write_fp) { fclose(write_fp); write_fp = nullptr; stdin_fd = -1; }
            if (read_fp) { fclose(read_fp); read_fp = nullptr; stdout_fd = -1; }
            // Give bot 10ms to exit gracefully, then SIGKILL
            usleep(10000);
            kill(pid, SIGTERM);
            usleep(10000);
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            pid = -1;
        }
    }
};

// Global bot refs for cleanup on unexpected exit
BotProcess* g_bots[2] = {nullptr, nullptr};
void cleanup_handler(int) {
    for (int i = 0; i < 2; i++) {
        if (g_bots[i]) g_bots[i]->kill_proc();
    }
    _exit(1);
}

// ═══════════════════════════════════════════════════════
// Protocol — send init and frame data to bots
// ═══════════════════════════════════════════════════════
void sendInitData(BotProcess& bot, int playerIndex, Game& game) {
    bot.sendLine(to_string(playerIndex));
    bot.sendLine(to_string(game.width));
    bot.sendLine(to_string(game.height));

    for (int y = 0; y < game.height; y++) {
        string row;
        for (int x = 0; x < game.width; x++) {
            row += (game.grid[y][x] == 1) ? '#' : '.';
        }
        bot.sendLine(row);
    }

    // birds per player
    int birdsPerPlayer = 0;
    for (auto& b : game.birds) if (b.owner == 0) birdsPerPlayer++;
    bot.sendLine(to_string(birdsPerPlayer));

    // My bird IDs
    for (auto& b : game.birds) {
        if (b.owner == playerIndex) bot.sendLine(to_string(b.id));
    }
    // Opponent bird IDs
    for (auto& b : game.birds) {
        if (b.owner != playerIndex) bot.sendLine(to_string(b.id));
    }
}

void sendFrameData(BotProcess& bot, Game& game) {
    // Apple count + positions
    bot.sendLine(to_string(game.apples.size()));
    for (auto& a : game.apples) {
        bot.sendLine(to_string(a.x) + " " + to_string(a.y));
    }

    // Live birds
    auto live = game.liveBirds();
    bot.sendLine(to_string(live.size()));
    for (auto* b : live) {
        string body;
        for (int i = 0; i < (int)b->body.size(); i++) {
            if (i > 0) body += ":";
            body += to_string(b->body[i].x) + "," + to_string(b->body[i].y);
        }
        bot.sendLine(to_string(b->id) + " " + body);
    }
}

// ═══════════════════════════════════════════════════════
// Parse bot commands
// ═══════════════════════════════════════════════════════
void parseCommands(const string& output, Game& game, int playerIndex) {
    istringstream ss(output);
    string token;
    int reasonableLimitForActions = 30;
    while (getline(ss, token, ';')) {
        if (reasonableLimitForActions-- <= 0) {
            return;
        }
        // Trim
        while (!token.empty() && token[0] == ' ') token.erase(0, 1);
        if (token.empty() || token == "WAIT") continue;

        istringstream ts(token);
        int birdId;
        string dirStr;
        if (!(ts >> birdId >> dirStr)) {
            continue;
        }

        // Find uppercase
        for (auto& c : dirStr) c = toupper(c);
        Dir dir = parseDir(dirStr);
        if (dir == UNSET) {
            continue;
        }

        for (auto& b : game.birds) {
            if (b.id == birdId && b.owner == playerIndex && b.alive) {
                if (b.direction != UNSET) {
                    break;
                }
                Dir facing = b.getFacing();
                if (dir != oppositeDir(facing)) {
                    b.direction = dir;
                }
                break;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════
// Visualization helpers
// ═══════════════════════════════════════════════════════
void printGrid(Game& game) {
    vector<string> display(game.height, string(game.width, '.'));
    for (int y = 0; y < game.height; y++)
        for (int x = 0; x < game.width; x++)
            if (game.grid[y][x] == 1) display[y][x] = '#';

    for (auto& a : game.apples) {
        if (game.inBounds(a.x, a.y)) display[a.y][a.x] = '*';
    }

    for (auto& b : game.birds) {
        if (!b.alive) continue;
        char ch = (b.owner == 0) ? 'A' + b.id : 'a' + b.id;
        for (int i = 0; i < (int)b.body.size(); i++) {
            Coord c = b.body[i];
            if (game.inBounds(c.x, c.y)) {
                display[c.y][c.x] = (i == 0) ? toupper(ch) : tolower(ch);
            }
        }
    }

    for (auto& row : display) cerr << row << endl;
}

// ═══════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    string replay_json = "[";

    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <bot1_cmd> <bot2_cmd> [seed] [--quiet] [--height H] [--snakes S]" << endl;
        cerr << "Example: ./referee ./bot ./bot 42" << endl;
        cerr << "         ./referee ./bot ./random_bot 12345 --quiet" << endl;
        return 1;
    }

    int seed = time(nullptr);
    if (argc >= 4 && string(argv[3]).find("--") != 0) seed = atoi(argv[3]);
    
    bool quiet = false;
    for (int i = 3; i < argc; i++) {
        if (string(argv[i]) == "--quiet") quiet = true;
        if (string(argv[i]) == "--height" && i+1 < argc) g_override_height = atoi(argv[i+1]);
        if (string(argv[i]) == "--snakes" && i+1 < argc) g_override_snakes = atoi(argv[i+1]);
    }

    mt19937 rng(seed);

    // Generate game
    Game game;
    generateGrid(game, rng);

    if (!quiet) {
        cerr << "=== GAME START === seed=" << seed << " grid=" << game.width << "x" << game.height
             << " apples=" << game.apples.size() << " birds=" << game.birds.size() << endl;
        printGrid(game);
        cerr << endl;
    }

    // Launch bots
    BotProcess bots[2];
    g_bots[0] = &bots[0];
    g_bots[1] = &bots[1];
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);
    signal(SIGPIPE, SIG_IGN);
    bots[0].launch(argv[1]);
    bots[1].launch(argv[2]);

    // Send init data
    sendInitData(bots[0], 0, game);
    sendInitData(bots[1], 1, game);

    const int MAX_TURNS = 200;
    int winner = -1; // -1 = tie

    for (int turn = 1; turn <= MAX_TURNS; turn++) {
        game.turn = turn;

        // Send frame data to both bots
        sendFrameData(bots[0], game);
        sendFrameData(bots[1], game);

        // Read commands (with timeout)
        int timeout = 10000; // generous timeout for local deep benchmarking
        string cmd0 = bots[0].readLine(timeout);
        string cmd1 = bots[1].readLine(timeout);

        if (cmd0 == "__TIMEOUT__") {
            cerr << "!!! Player 0 TIMEOUT on turn " << turn << endl;
            // Set default directions
        } else if (cmd0 == "__EOF__") {
            cerr << "!!! Player 0 CRASHED on turn " << turn << endl;
            winner = 1;
            break;
        } else {
            parseCommands(cmd0, game, 0);
        }

        if (cmd1 == "__TIMEOUT__") {
            cerr << "!!! Player 1 TIMEOUT on turn " << turn << endl;
        } else if (cmd1 == "__EOF__") {
            cerr << "!!! Player 1 CRASHED on turn " << turn << endl;
            winner = 0;
            break;
        } else {
            parseCommands(cmd1, game, 1);
        }

        // Set default direction for any bird that wasn't given a command
        for (auto& b : game.birds) {
            if (b.alive && b.direction == UNSET) {
                b.direction = b.getFacing();
            }
        }

        // Simulate
        game.performTurn();

        // Reset directions for next turn
        for (auto& b : game.birds) b.direction = UNSET;

        if (!quiet) {
            cerr << "--- Turn " << turn << " ---" << endl;
            if (cmd0 != "__TIMEOUT__" && cmd0 != "__EOF__") cerr << "P0: " << cmd0 << endl;
            if (cmd1 != "__TIMEOUT__" && cmd1 != "__EOF__") cerr << "P1: " << cmd1 << endl;
            printGrid(game);

            // Show scores
            cerr << "Score: P0=" << game.scoreFor(0) << " P1=" << game.scoreFor(1)
                 << " Apples=" << game.apples.size() << endl;
            cerr << endl;
        }

        if (game.isGameOver()) {
            if (!quiet) cerr << "Game over on turn " << turn << endl;
            break;
        }
    }

    // Final scores
    int s0 = game.scoreFor(0);
    int s1 = game.scoreFor(1);

    if (winner == -1) {
        if (s0 > s1) winner = 0;
        else if (s1 > s0) winner = 1;
        else {
            // Tiebreak: fewer losses
            if (game.losses[0] < game.losses[1]) winner = 0;
            else if (game.losses[1] < game.losses[0]) winner = 1;
            else winner = -1; // true tie
        }
    }

    // Output result (stdout for scripting)
    string result;
    if (winner == 0) result = "P0_WIN";
    else if (winner == 1) result = "P1_WIN";
    else result = "TIE";

    cout << result << " " << s0 << " " << s1
         << " losses=" << game.losses[0] << "," << game.losses[1]
         << " turns=" << game.turn << " seed=" << seed << endl;

    if (!quiet) {
        cerr << "=== RESULT: " << result << " ===" << endl;
        cerr << "P0 score=" << s0 << " losses=" << game.losses[0] << endl;
        cerr << "P1 score=" << s1 << " losses=" << game.losses[1] << endl;
    }

    // Cleanup
    bots[0].kill_proc();
    bots[1].kill_proc();
    
    replay_json += "\n]\n";
    ofstream out("replay.json");
    out << replay_json;
    out.close();
    return 0;
}
