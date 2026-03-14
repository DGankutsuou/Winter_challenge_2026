#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#undef _GLIBCXX_DEBUG // disable run-time bound checking, etc
#pragma GCC optimize(                                                          \
    "Ofast,inline") // Ofast =
                    // O3,fast-math,allow-store-data-races,no-protect-parens

#ifndef __POPCNT__ // not march=generic
#endif

#pragma GCC target("bmi,bmi2,lzcnt,popcnt") // bit manipulation
#pragma GCC target("movbe")                 // byte swap
#pragma GCC target("aes,pclmul,rdrnd")      // encryption
#pragma GCC target("avx,avx2,f16c,fma,sse3,ssse3,sse4.1,sse4.2") // SIMD
#pragma GCC optimize "Ofast,unroll-loops,omit-frame-pointer,inline"
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#pragma GCC target("rdrnd", "popcnt", "avx", "bmi2")

using namespace std;

constexpr int MAX_CHAIN_LENGTH = 100;
constexpr int MAX_SNAKEBOTS = 4;

int my_snakebot_count = 0;
int opp_snakebot_count = 0;

struct s_cord {
  int x;
  int y;
  bool operator==(const s_cord &cord) { return x == cord.x && y == cord.y; }
};

enum e_direction { UP, DOWN, RIGHT, LEFT };

string direction_to_string(e_direction dir) {
  switch (dir) {
  case UP:
    return "UP";
  case DOWN:
    return "DOWN";
  case RIGHT:
    return "RIGHT";
  case LEFT:
    return "LEFT";
  default:
    return "";
  }
}

struct s_direction {
  s_direction() : dir(UP) {}
  s_direction(e_direction d) : dir(d) {}
  e_direction dir;

  bool operator==(const s_direction &sdr) { return dir == sdr.dir; }
  bool operator!=(const s_direction &sdr) {
    if (dir == UP && sdr.dir == DOWN)
      return true;
    if (dir == DOWN && sdr.dir == UP)
      return true;
    if (dir == RIGHT && sdr.dir == LEFT)
      return true;
    if (dir == LEFT && sdr.dir == RIGHT)
      return true;
    return false;
  }

  static bool are_opposite(const s_direction &sdr1, const s_direction &sdr2) {
    if (sdr1.dir == UP && sdr2.dir == DOWN)
      return true;
    if (sdr1.dir == DOWN && sdr2.dir == UP)
      return true;
    if (sdr1.dir == RIGHT && sdr2.dir == LEFT)
      return true;
    if (sdr1.dir == LEFT && sdr2.dir == RIGHT)
      return true;
    return false;
  }

  static bool are_opposite(const e_direction &dir1, const e_direction &dir2) {
    if (dir1 == UP && dir2 == DOWN)
      return true;
    if (dir1 == DOWN && dir2 == UP)
      return true;
    if (dir1 == RIGHT && dir2 == LEFT)
      return true;
    if (dir1 == LEFT && dir2 == RIGHT)
      return true;
    return false;
  }
};

struct s_move {
  int id;
  e_direction dir;
};

struct MySnakebot;
struct OppSnakebot;

struct MySnakebot {
public:
  MySnakebot() : id(-1), sdr(UP) {}
  MySnakebot(int i, e_direction d) : id(i), sdr(d) {}
  int id;
  int length = 0;
  s_direction sdr;
  int is_alive = 0;
  s_cord chain[MAX_CHAIN_LENGTH];
  s_direction possible_moves[4];
  int possible_moves_count = 0;

  void move(e_direction dir) {
    string directions[4] = {"UP", "DOWN", "RIGHT", "LEFT"};
    cout << id << " " << directions[dir];
    if (dir == UP && sdr.dir == DOWN)
      return;
    if (dir == DOWN && sdr.dir == UP)
      return;
    if (dir == RIGHT && sdr.dir == LEFT)
      return;
    if (dir == LEFT && sdr.dir == RIGHT)
      return;
    sdr.dir = dir;
  }

  void move() {
    if (possible_moves_count == 0)
    {
      cout << id << " " << direction_to_string(sdr.dir);
      return;
    }
    int i = rand() % possible_moves_count;
    cout << id << " " << direction_to_string(possible_moves[i].dir);
    sdr.dir = possible_moves[i].dir;
  }

  void body_to_chain(string &body) {
    length = 0;
    size_t pos = 0;
    while ((pos = body.find(':')) != string::npos) {
      string token = body.substr(0, pos);
      size_t comma_pos = token.find(',');
      s_cord c;
      c.x = stoi(token.substr(0, comma_pos));
      c.y = stoi(token.substr(comma_pos + 1));
      chain[length++] = c;
      body.erase(0, pos + 1);
    }
    // last token
    size_t comma_pos = body.find(',');
    s_cord c;
    c.x = stoi(body.substr(0, comma_pos));
    c.y = stoi(body.substr(comma_pos + 1));
    chain[length++] = c;
  }

  void generate_possible_moves(char **grid, int width, int height) {
    possible_moves_count = 0;
    int dx[4] = {0, 0, 1, -1};
    int dy[4] = {-1, 1, 0, 0};
    for (int i = 0; i < 4; i++) {
      int new_x = chain[0].x + dx[i];
      int new_y = chain[0].y + dy[i];
      if (new_x >= 0 && new_x < width && new_y >= 0 &&
          (new_y < height && grid[new_y][new_x] == '.' || grid[new_y][new_x] == '@') &&
        !s_direction::are_opposite(static_cast<e_direction>(i), sdr.dir)) {
        possible_moves[possible_moves_count++] = s_direction(static_cast<e_direction>(i));
      }
    }
  }
};

struct OppSnakebot {
public:
  OppSnakebot() : id(-1), edr(UP) {}
  OppSnakebot(int i, e_direction d) : id(i), edr(d) {}
  int id;
  int length = 0;
  e_direction edr;
  s_direction sdr;
  int is_alive = 0;
  s_cord chain[MAX_CHAIN_LENGTH];
  s_direction possible_moves[4];
  int possible_moves_count = 0;

  void body_to_chain(string &body) {
    length = 0;
    size_t pos = 0;
    while ((pos = body.find(':')) != string::npos) {
      string token = body.substr(0, pos);
      size_t comma_pos = token.find(',');
      s_cord c;
      c.x = stoi(token.substr(0, comma_pos));
      c.y = stoi(token.substr(comma_pos + 1));
      chain[length++] = c;
      body.erase(0, pos + 1);
    }
    // last token
    size_t comma_pos = body.find(',');
    s_cord c;
    c.x = stoi(body.substr(0, comma_pos));
    c.y = stoi(body.substr(comma_pos + 1));
    chain[length++] = c;
  }

  void generate_possible_moves(char **grid, int width, int height) {
    possible_moves_count = 0;
    int dx[4] = {0, 0, 1, -1};
    int dy[4] = {-1, 1, 0, 0};
    for (int i = 0; i < 4; i++) {
      int new_x = chain[0].x + dx[i];
      int new_y = chain[0].y + dy[i];
      if (new_x >= 0 && new_x < width && new_y >= 0 &&
          (new_y < height && grid[new_y][new_x] == '.' || grid[new_y][new_x] == '@') &&
        !s_direction::are_opposite(static_cast<e_direction>(i), sdr.dir)) {
        possible_moves[possible_moves_count++] = s_direction(static_cast<e_direction>(i));
      }
    }
  }
};

struct Grid {
public:
  int width;
  int height;
  char **grid2d;
  Grid() : width(0), height(0), grid2d(nullptr) {}
  Grid(int w, int h) : width(w), height(h) {
    grid2d = new char *[height];
    for (int i = 0; i < height; i++) {
      grid2d[i] = new char[width];
      for (int j = 0; j < width; j++) {
        grid2d[i][j] = '.';
      }
    }
  }
  ~Grid() {
    if (grid2d != nullptr) {
      for (int i = 0; i < height; i++) {
        if (grid2d[i] != nullptr)
          delete[] grid2d[i];
      }
      delete[] grid2d;
    }
  }

  void recreate_grid(int w, int h) {
    if (grid2d != nullptr) {
      for (int i = 0; i < height; i++) {
        if (grid2d[i] != nullptr)
          delete[] grid2d[i];
      }
      delete[] grid2d;
    }
    width = w;
    height = h;
    grid2d = new char *[height];
    for (int i = 0; i < height; i++) {
      grid2d[i] = new char[width];
    }
  }

  void refresh_grid() {
    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        if (grid2d[i][j] != '#')
          grid2d[i][j] = '.';
      }
    }
  }

  void print_grid() {
    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        cerr << grid2d[i][j];
      }
      cerr << endl;
    }
  }

  void insertMySnakebot(MySnakebot &snakebot) {
    for (int i = 0; i < snakebot.length; i++) {
      s_cord c = snakebot.chain[i];
      grid2d[snakebot.chain[i].y][snakebot.chain[i].x] = snakebot.id + 'A';
    }
  }

  void insertOppSnakebot(OppSnakebot &snakebot) {
    for (int i = 0; i < snakebot.length; i++) {
      grid2d[snakebot.chain[i].y][snakebot.chain[i].x] = snakebot.id + 'a';
    }
  }
};

struct SMySnakebots {
  MySnakebot arr[MAX_SNAKEBOTS];

  MySnakebot *getSnakebotById(int id) {
    for (int i = 0; i < MAX_SNAKEBOTS; i++) {
      if (arr[i].id == id)
        return &(arr[i]);
    }
    return nullptr;
  }

  void killAllSnakebots() {
    for (int i = 0; i < MAX_SNAKEBOTS; i++) {
      arr[i].is_alive = 0;
    }
  }
};

struct SOppSnakebots {
  OppSnakebot arr[MAX_SNAKEBOTS];

  OppSnakebot *getSnakebotById(int id) {
    for (int i = 0; i < MAX_SNAKEBOTS; i++) {
      if (arr[i].id == id)
        return &(arr[i]);
    }
    return nullptr;
  }

  void killAllSnakebots() {
    for (int i = 0; i < MAX_SNAKEBOTS; i++) {
      arr[i].is_alive = 0;
    }
  }
};

SMySnakebots my_snakebots;
SOppSnakebots opp_snakebots;

Grid grid;

// this function will generate all possible combinations of moves for my
// snakebots and return them as a vector of vector of s_move
struct game_state
{
  int total_power_sources;
  int my_snakebot_count;
  int opp_snakebot_count;
  int my_lengths_total;
  int opp_lengths_total;
};

int main() {
  int my_id;
  cin >> my_id;
  cin.ignore();
  int width;
  cin >> width;
  cin.ignore();
  int height;
  cin >> height;
  cin.ignore();
  grid.recreate_grid(width, height);
  for (int i = 0; i < height; i++) {
    string row;
    getline(cin, row);
    for (int j = 0; j < width; j++) {
      grid.grid2d[i][j] = row[j];
    }
  }
  int snakebots_per_player;
  cin >> snakebots_per_player;
  cin.ignore();
  for (int i = 0; i < snakebots_per_player; i++) {
    int my_snakebot_id;
    cin >> my_snakebot_id;
    cin.ignore();
    my_snakebots.arr[i] = MySnakebot(my_snakebot_id, UP);
    my_snakebot_count++;
  }
  for (int i = 0; i < snakebots_per_player; i++) {
    int opp_snakebot_id;
    cin >> opp_snakebot_id;
    cin.ignore();
    opp_snakebots.arr[i] = OppSnakebot(opp_snakebot_id, UP);
    opp_snakebot_count++;
  }

  // game loop
  while (1) {
    int power_source_count;
    cin >> power_source_count;
    cin.ignore();
    for (int i = 0; i < power_source_count; i++) {
      int x;
      int y;
      cin >> x >> y;
      cin.ignore();
      grid.grid2d[y][x] = '@';
    }
    int snakebot_count;
    cin >> snakebot_count;
    cin.ignore();
    for (int i = 0; i < snakebot_count; i++) {
      int snakebot_id;
      string body;
      cin >> snakebot_id >> body;
      cin.ignore();
      MySnakebot *my_snakebot = my_snakebots.getSnakebotById(snakebot_id);
      if (my_snakebot != nullptr) {
        my_snakebot->body_to_chain(body);
        grid.insertMySnakebot(*my_snakebot);
        my_snakebot->is_alive = 1;
        my_snakebot_count++;
      } else {
        OppSnakebot *opp_snakebot = opp_snakebots.getSnakebotById(snakebot_id);
        if (opp_snakebot != nullptr) {
          opp_snakebot->body_to_chain(body);
          grid.insertOppSnakebot(*opp_snakebot);
          opp_snakebot->is_alive = 1;
          opp_snakebot_count++;
        }
      }
    }

    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;
    grid.print_grid();
    for (int i = 0; i < MAX_SNAKEBOTS; i++) {
      if (my_snakebots.arr[i].is_alive) {
        my_snakebots.arr[i].generate_possible_moves(grid.grid2d, grid.width, grid.height);
        my_snakebots.arr[i].move();

        if (i < MAX_SNAKEBOTS - 1)
          cout << ";";
      }
    }
    cout << endl;
    // cout << "wait" << endl;
    my_snakebots.killAllSnakebots();
    opp_snakebots.killAllSnakebots();
    my_snakebot_count = 0;
    opp_snakebot_count = 0;
    grid.refresh_grid();
  }
}
