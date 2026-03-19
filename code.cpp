#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#undef _GLIBCXX_DEBUG // disable run-time bound checking, etc
#pragma GCC optimize( \
    "Ofast,inline") // Ofast =
                    // O3,fast-math,allow-store-data-races,no-protect-parens

#ifndef __POPCNT__ // not march=generic
#endif

#pragma GCC target("bmi,bmi2,lzcnt,popcnt")                      // bit manipulation
#pragma GCC target("movbe")                                      // byte swap
#pragma GCC target("aes,pclmul,rdrnd")                           // encryption
#pragma GCC target("avx,avx2,f16c,fma,sse3,ssse3,sse4.1,sse4.2") // SIMD
#pragma GCC optimize "Ofast,unroll-loops,omit-frame-pointer,inline"
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#pragma GCC target("rdrnd", "popcnt", "avx", "bmi2")

using namespace std;

constexpr int MAX_CHAIN_LENGTH = 100;
constexpr int MAX_SNAKEBOTS = 4;

int my_snakebot_count = 0;
int opp_snakebot_count = 0;
int power_source_count = 0;
int grid_width = 0;
int grid_height = 0;


struct s_cord
{
  int x;
  int y;
  bool operator==(const s_cord &cord) { return x == cord.x && y == cord.y; }

  bool is_bound() { return x >= 0 && x < grid_width && y >= 0 && y < grid_height; }
};

enum e_direction
{
  UP,
  DOWN,
  RIGHT,
  LEFT
};

bool is_bound(int x, int y)
{
  return x >= 0 && x < grid_width && y >= 0 && y < grid_height;
}

string direction_to_string(e_direction dir)
{
  switch (dir)
  {
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

struct s_direction
{
  s_direction() : dir(UP) {}
  s_direction(e_direction d) : dir(d) {}
  e_direction dir;

  bool operator==(const s_direction &sdr) { return dir == sdr.dir; }
  bool operator!=(const s_direction &sdr)
  {
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

  static bool are_opposite(const s_direction &sdr1, const s_direction &sdr2)
  {
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

  static bool are_opposite(const e_direction &dir1, const e_direction &dir2)
  {
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

struct s_move
{
  int id;
  e_direction dir;
};

struct MySnakebot;
struct OppSnakebot;

struct MySnakebot
{
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

  void move(e_direction dir)
  {
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

  void move()
  {
    if (possible_moves_count == 0)
    {
      cout << id << " " << direction_to_string(sdr.dir);
      return;
    }
    int i = rand() % possible_moves_count;
    cout << id << " " << direction_to_string(possible_moves[i].dir);
    sdr.dir = possible_moves[i].dir;
  }

  void body_to_chain(string &body)
  {
    length = 0;
    size_t pos = 0;
    while ((pos = body.find(':')) != string::npos)
    {
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

  void generate_possible_moves(char grid[50][50], int width, int height)
  {
    possible_moves_count = 0;
    int dx[4] = {0, 0, 1, -1};
    int dy[4] = {-1, 1, 0, 0};
    for (int i = 0; i < 4; i++)
    {
      int new_x = chain[0].x + dx[i];
      int new_y = chain[0].y + dy[i];
      if (new_x >= 0 && new_x < width && new_y >= 0 &&
          (new_y < height && grid[new_y][new_x] == '.' || grid[new_y][new_x] == '@') &&
          !s_direction::are_opposite(static_cast<e_direction>(i), sdr.dir))
      {
        possible_moves[possible_moves_count++] = s_direction(static_cast<e_direction>(i));
      }
    }
  }
};

struct OppSnakebot
{
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

  void body_to_chain(string &body)
  {
    length = 0;
    size_t pos = 0;
    while ((pos = body.find(':')) != string::npos)
    {
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

  void generate_possible_moves(char **grid, int width, int height)
  {
    possible_moves_count = 0;
    int dx[4] = {0, 0, 1, -1};
    int dy[4] = {-1, 1, 0, 0};
    for (int i = 0; i < 4; i++)
    {
      int new_x = chain[0].x + dx[i];
      int new_y = chain[0].y + dy[i];
      if (new_x >= 0 && new_x < width && new_y >= 0 &&
          (new_y < height && grid[new_y][new_x] == '.' || grid[new_y][new_x] == '@') &&
          !s_direction::are_opposite(static_cast<e_direction>(i), sdr.dir))
      {
        possible_moves[possible_moves_count++] = s_direction(static_cast<e_direction>(i));
      }
    }
  }
};

struct Grid
{
public:
  int turn = 0;
  int width;
  int height;
  struct s_grid2d
  {
    char map[50][50];
  };
  s_grid2d grid2d;

  int total_power_sources;
  int my_snakebot_count;
  int opp_snakebot_count;
  int my_lengths_total;
  int opp_lengths_total;
  int my_score;
  int opp_score;

  Grid() : width(0), height(0) {}
  Grid(int w, int h) : width(w), height(h)
  {
    for (int i = 0; i < height; i++)
    {
      for (int j = 0; j < width; j++)
      {
        grid2d.map[i][j] = '.';
      }
    }
  }
  ~Grid()
  {
  }

  void recreate_grid(int w, int h)
  {
    width = w;
    height = h;
  }

  void refresh_grid()
  {
    total_power_sources = 0;
    my_snakebot_count = 0;
    opp_snakebot_count = 0;
    my_lengths_total = 0;
    opp_lengths_total = 0;
    my_score = 0;
    opp_score = 0;
    for (int i = 0; i < height; i++)
    {
      for (int j = 0; j < width; j++)
      {
        if (grid2d.map[i][j] != '#')
          grid2d.map[i][j] = '.';
      }
    }
  }

  void print_grid()
  {
    for (int i = 0; i < height; i++)
    {
      for (int j = 0; j < width; j++)
      {
        cerr << grid2d.map[i][j];
      }
      cerr << endl;
    }
  }

  void insertMySnakebot(MySnakebot &snakebot)
  {
    for (int i = 0; i < snakebot.length; i++)
    {
      s_cord c = snakebot.chain[i];
      if (is_bound(c.x, c.y))
        grid2d.map[c.y][c.x] = snakebot.id + 'A';
    }
  }

  void insertOppSnakebot(OppSnakebot &snakebot)
  {
    for (int i = 0; i < snakebot.length; i++)
    {
      if (is_bound(snakebot.chain[i].x, snakebot.chain[i].y))
        grid2d.map[snakebot.chain[i].y][snakebot.chain[i].x] = snakebot.id + 'a';
    }
  }

  void score(int my_snakebots, int opp_snakebots)
  {
    my_snakebot_count = my_snakebots;
    opp_snakebot_count = opp_snakebots;
    for (int i = 0; i < height; i++)
    {
      for (int j = 0; j < width; j++)
      {
        if (grid2d.map[i][j] == '@')
          total_power_sources++;
        if (grid2d.map[i][j] >= 'A' && grid2d.map[i][j] <= 'D')
          my_lengths_total++;
        if (grid2d.map[i][j] >= 'a' && grid2d.map[i][j] <= 'd')
          opp_lengths_total++;
      }
    }
    my_score = my_snakebot_count * my_lengths_total - opp_snakebot_count * opp_lengths_total;
    opp_score = opp_snakebot_count * opp_lengths_total - my_snakebot_count * my_lengths_total;
  }
};

struct SMySnakebots
{
  MySnakebot arr[MAX_SNAKEBOTS];
  int count = 0;

  int countAlive()
  {
    count = 0;
    for (int i = 0; i < MAX_SNAKEBOTS; i++)
    {
      if (arr[i].is_alive)
        count++;
    }
    return count;
  }

  MySnakebot *getSnakebotById(int id)
  {
    for (int i = 0; i < MAX_SNAKEBOTS; i++)
    {
      if (arr[i].id == id)
        return &(arr[i]);
    }
    return nullptr;
  }

  void killAllSnakebots()
  {
    for (int i = 0; i < MAX_SNAKEBOTS; i++)
    {
      arr[i].is_alive = 0;
    }
  }
};

struct SOppSnakebots
{
  OppSnakebot arr[MAX_SNAKEBOTS];
  int count = 0;

  int countAlive()
  {
    count = 0;
    for (int i = 0; i < MAX_SNAKEBOTS; i++)
    {
      if (arr[i].is_alive)
        count++;
    }
    return count;
  }

  OppSnakebot *getSnakebotById(int id)
  {
    for (int i = 0; i < MAX_SNAKEBOTS; i++)
    {
      if (arr[i].id == id)
        return &(arr[i]);
    }
    return nullptr;
  }

  void killAllSnakebots()
  {
    for (int i = 0; i < MAX_SNAKEBOTS; i++)
    {
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

void SimulateTurn(Grid &grid, SMySnakebots &my_bots, SOppSnakebots &opp_bots)
{
  int dx[4] = {0, 0, 1, -1};
  int dy[4] = {-1, 1, 0, 0};

  // --- 1. الحركة والماكلة ---
  for (int i = 0; i < MAX_SNAKEBOTS; i++)
  {
    if (my_bots.arr[i].is_alive)
    {
      s_cord new_head = {my_bots.arr[i].chain[0].x + dx[my_bots.arr[i].sdr.dir], my_bots.arr[i].chain[0].y + dy[my_bots.arr[i].sdr.dir]};
      if (new_head.x >= 0 && new_head.x < grid.width && new_head.y >= 0 && new_head.y < grid.height && grid.grid2d.map[new_head.y][new_head.x] == '@')
      {
        my_bots.arr[i].length++;
      }
      for (int j = my_bots.arr[i].length - 1; j > 0; j--)
        my_bots.arr[i].chain[j] = my_bots.arr[i].chain[j - 1];
      my_bots.arr[i].chain[0] = new_head;
    }

    if (opp_bots.arr[i].is_alive)
    {
      s_cord new_head = {opp_bots.arr[i].chain[0].x + dx[opp_bots.arr[i].edr], opp_bots.arr[i].chain[0].y + dy[opp_bots.arr[i].edr]};
      if (new_head.x >= 0 && new_head.x < grid.width && new_head.y >= 0 && new_head.y < grid.height && grid.grid2d.map[new_head.y][new_head.x] == '@')
      {
        opp_bots.arr[i].length++;
      }
      for (int j = opp_bots.arr[i].length - 1; j > 0; j--)
        opp_bots.arr[i].chain[j] = opp_bots.arr[i].chain[j - 1];
      opp_bots.arr[i].chain[0] = new_head;
    }
  }

  // مسح الطاقة بعدما كولشي تحرك
  for (int i = 0; i < MAX_SNAKEBOTS; i++)
  {
    if (my_bots.arr[i].is_alive && my_bots.arr[i].chain[0].x >= 0 && my_bots.arr[i].chain[0].x < grid.width && my_bots.arr[i].chain[0].y >= 0 && my_bots.arr[i].chain[0].y < grid.height)
    {
      if (grid.grid2d.map[my_bots.arr[i].chain[0].y][my_bots.arr[i].chain[0].x] == '@')
        grid.grid2d.map[my_bots.arr[i].chain[0].y][my_bots.arr[i].chain[0].x] = '.';
    }
    if (opp_bots.arr[i].is_alive && opp_bots.arr[i].chain[0].x >= 0 && opp_bots.arr[i].chain[0].x < grid.width && opp_bots.arr[i].chain[0].y >= 0 && opp_bots.arr[i].chain[0].y < grid.height)
    {
      if (grid.grid2d.map[opp_bots.arr[i].chain[0].y][opp_bots.arr[i].chain[0].x] == '@')
        grid.grid2d.map[opp_bots.arr[i].chain[0].y][opp_bots.arr[i].chain[0].x] = '.';
    }
  }

  // --- 2. الاصطدامات وقطع الريوس ---
  bool my_hit[MAX_SNAKEBOTS] = {false};
  bool opp_hit[MAX_SNAKEBOTS] = {false};

  for (int i = 0; i < MAX_SNAKEBOTS; i++)
  {
    if (!my_bots.arr[i].is_alive)
      continue;
    s_cord head = my_bots.arr[i].chain[0];

    if (head.x >= 0 && head.x < grid.width && head.y >= 0 && head.y < grid.height)
    {
      if (grid.grid2d.map[head.y][head.x] == '#')
      {
        my_hit[i] = true;
        continue;
      }
    }

    for (int j = 0; j < MAX_SNAKEBOTS; j++)
    {
      if (my_bots.arr[j].is_alive)
      {
        for (int k = (i == j ? 1 : 0); k < my_bots.arr[j].length; k++)
        {
          if (head == my_bots.arr[j].chain[k])
          {
            my_hit[i] = true;
            break;
          }
        }
      }
      if (my_hit[i])
        break;
      if (opp_bots.arr[j].is_alive)
      {
        for (int k = 0; k < opp_bots.arr[j].length; k++)
        {
          if (head == opp_bots.arr[j].chain[k])
          {
            my_hit[i] = true;
            break;
          }
        }
      }
      if (my_hit[i])
        break;
    }
  }

  for (int i = 0; i < MAX_SNAKEBOTS; i++)
  {
    if (!opp_bots.arr[i].is_alive)
      continue;
    s_cord head = opp_bots.arr[i].chain[0];

    if (head.x >= 0 && head.x < grid.width && head.y >= 0 && head.y < grid.height)
    {
      if (grid.grid2d.map[head.y][head.x] == '#')
      {
        opp_hit[i] = true;
        continue;
      }
    }

    for (int j = 0; j < MAX_SNAKEBOTS; j++)
    {
      if (my_bots.arr[j].is_alive)
      {
        for (int k = 0; k < my_bots.arr[j].length; k++)
        {
          if (head == my_bots.arr[j].chain[k])
          {
            opp_hit[i] = true;
            break;
          }
        }
      }
      if (opp_hit[i])
        break;
      if (opp_bots.arr[j].is_alive)
      {
        for (int k = (i == j ? 1 : 0); k < opp_bots.arr[j].length; k++)
        {
          if (head == opp_bots.arr[j].chain[k])
          {
            opp_hit[i] = true;
            break;
          }
        }
      }
      if (opp_hit[i])
        break;
    }
  }

  for (int i = 0; i < MAX_SNAKEBOTS; i++)
  {
    if (my_hit[i])
    {
      if (my_bots.arr[i].length <= 3)
        my_bots.arr[i].is_alive = 0;
      else
      {
        my_bots.arr[i].length--;
        for (int j = 0; j < my_bots.arr[i].length; j++)
          my_bots.arr[i].chain[j] = my_bots.arr[i].chain[j + 1];
      }
    }
    if (opp_hit[i])
    {
      if (opp_bots.arr[i].length <= 3)
        opp_bots.arr[i].is_alive = 0;
      else
      {
        opp_bots.arr[i].length--;
        for (int j = 0; j < opp_bots.arr[i].length; j++)
          opp_bots.arr[i].chain[j] = opp_bots.arr[i].chain[j + 1];
      }
    }
  }

  // --- 3. الجاذبية ---
  bool something_fell = true;
  while (something_fell)
  {
    something_fell = false;

    // أ. السقوط الفردي
    bool individual_fell = true;
    while (individual_fell)
    {
      individual_fell = false;

      auto check_individual_fall = [&](auto &bots, auto &other_bots)
      {
        for (int i = 0; i < MAX_SNAKEBOTS; i++)
        {
          if (!bots.arr[i].is_alive)
            continue;
          bool can_fall = true;
          for (int k = 0; k < bots.arr[i].length; k++)
          {
            s_cord under = {bots.arr[i].chain[k].x, bots.arr[i].chain[k].y + 1};
            if (under.x >= 0 && under.x < grid.width && under.y >= 0 && under.y < grid.height)
            {
              if (grid.grid2d.map[under.y][under.x] == '#' || grid.grid2d.map[under.y][under.x] == '@')
              {
                can_fall = false;
                break;
              }
            }
            for (int j = 0; j < MAX_SNAKEBOTS; j++)
            {
              if (bots.arr[j].is_alive && i != j)
              {
                for (int l = 0; l < bots.arr[j].length; l++)
                  if (bots.arr[j].chain[l] == under)
                  {
                    can_fall = false;
                    break;
                  }
              }
              if (!can_fall)
                break;
              if (other_bots.arr[j].is_alive)
              {
                for (int l = 0; l < other_bots.arr[j].length; l++)
                  if (other_bots.arr[j].chain[l] == under)
                  {
                    can_fall = false;
                    break;
                  }
              }
              if (!can_fall)
                break;
            }
            if (!can_fall)
              break;
          }
          if (can_fall)
          {
            individual_fell = true;
            something_fell = true;
            bool out_of_bounds = true;
            for (int k = 0; k < bots.arr[i].length; k++)
            {
              bots.arr[i].chain[k].y++;
              if (bots.arr[i].chain[k].y < grid.height)
                out_of_bounds = false;
            }
            if (out_of_bounds)
              bots.arr[i].is_alive = 0;
          }
        }
      };

      check_individual_fall(my_bots, opp_bots);
      check_individual_fall(opp_bots, my_bots);
    }

    // ب. السقوط المتشابك
    bool is_alive_bot[8] = {false};
    int bot_lens[8] = {0};
    s_cord *bot_chains[8];
    int *bot_alive_ptr[8];

    for (int i = 0; i < MAX_SNAKEBOTS; i++)
    {
      if (my_bots.arr[i].is_alive)
      {
        is_alive_bot[i] = true;
        bot_lens[i] = my_bots.arr[i].length;
        bot_chains[i] = my_bots.arr[i].chain;
        bot_alive_ptr[i] = &my_bots.arr[i].is_alive;
      }
      if (opp_bots.arr[i].is_alive)
      {
        is_alive_bot[i + 4] = true;
        bot_lens[i + 4] = opp_bots.arr[i].length;
        bot_chains[i + 4] = opp_bots.arr[i].chain;
        bot_alive_ptr[i + 4] = &opp_bots.arr[i].is_alive;
      }
    }

    bool visited[8] = {false};
    for (int i = 0; i < 8; i++)
    {
      if (!is_alive_bot[i] || visited[i])
        continue;

      int group[8];
      int group_size = 0;
      int stack[8];
      int stack_size = 0;

      stack[stack_size++] = i;
      while (stack_size > 0)
      {
        int cur = stack[--stack_size];
        if (visited[cur])
          continue;
        visited[cur] = true;
        group[group_size++] = cur;

        for (int j = 0; j < 8; j++)
        {
          if (!is_alive_bot[j] || visited[j])
            continue;
          bool touching = false;
          for (int a = 0; a < bot_lens[cur]; a++)
          {
            for (int b = 0; b < bot_lens[j]; b++)
            {
              if (abs(bot_chains[cur][a].x - bot_chains[j][b].x) + abs(bot_chains[cur][a].y - bot_chains[j][b].y) == 1)
              {
                touching = true;
                break;
              }
            }
            if (touching)
              break;
          }
          if (touching)
            stack[stack_size++] = j;
        }
      }

      if (group_size <= 1)
        continue;

      bool can_fall = true;
      for (int g = 0; g < group_size; g++)
      {
        int idx = group[g];
        for (int k = 0; k < bot_lens[idx]; k++)
        {
          s_cord under = {bot_chains[idx][k].x, bot_chains[idx][k].y + 1};
          if (under.x >= 0 && under.x < grid.width && under.y >= 0 && under.y < grid.height)
          {
            if (grid.grid2d.map[under.y][under.x] == '#' || grid.grid2d.map[under.y][under.x] == '@')
            {
              can_fall = false;
              break;
            }
          }
          for (int j = 0; j < 8; j++)
          {
            if (!is_alive_bot[j])
              continue;
            bool in_group = false;
            for (int q = 0; q < group_size; q++)
              if (group[q] == j)
              {
                in_group = true;
                break;
              }
            if (in_group)
              continue;

            for (int l = 0; l < bot_lens[j]; l++)
            {
              if (bot_chains[j][l] == under)
              {
                can_fall = false;
                break;
              }
            }
            if (!can_fall)
              break;
          }
          if (!can_fall)
            break;
        }
        if (!can_fall)
          break;
      }

      if (can_fall)
      {
        something_fell = true;
        for (int g = 0; g < group_size; g++)
        {
          int idx = group[g];
          bool out_of_bounds = true;
          for (int k = 0; k < bot_lens[idx]; k++)
          {
            bot_chains[idx][k].y++;
            if (bot_chains[idx][k].y < grid.height)
              out_of_bounds = false;
          }
          if (out_of_bounds)
            *bot_alive_ptr[idx] = 0;
        }
      }
    }
  }

  // --- 4. تحديث الشبكة ---
  grid.refresh_grid();
  for (int i = 0; i < MAX_SNAKEBOTS; i++)
  {
    if (my_bots.arr[i].is_alive)
      grid.insertMySnakebot(my_bots.arr[i]);
    if (opp_bots.arr[i].is_alive)
      grid.insertOppSnakebot(opp_bots.arr[i]);
  }
}

struct Node {
  int score = 0;
  int visits = 0;
  e_direction move; // الحركة لي ختارينا فهاد العقدة
  Node *parent = NULL;
  Node *children[3] = {NULL, NULL, NULL};
  int children_count = 0;
};

/*
دالة smitsimax():
    // 1. البداية: صايب الجذور (Root Nodes)
    لكل حنش (ديالي وديال الخصم) عايش:
        Root_Nodes[id] = عقدة جديدة () // هادي هي نقطة الانطلاق ديالو فهاد التور

    // 2. حلقة البحث (ما حد الوقت باقي ماسالاش - مثلا 40ms)
    ما حد (باقي الوقت):

        // أ. استرجاع الحالة الأصلية
        // (خبينا الـ Grid والحناش الأصليين باش نرجعو ليهم فكل سيميولايشن)
        sim_grid = grid
        sim_my_bots = my_snakebots
        sim_opp_bots = opp_snakebots

        // ليستة باش نعقلو على العقد لي دزنا منهم فهاد اللوب باش نرجعو ليهم السكور فاللخر
        traversed_nodes[MAX_SNAKEBOTS] = مصفوفة خاوية

        // ب. النزول فالشجرة (العمق)
        depth = 0
        ما حد (depth < MAX_DEPTH و sim_grid.total_power_sources > 0):

            // اختيار الحركات لكل حنش
            لكل حنش عايش فـ (sim_my_bots و sim_opp_bots):
                node = Root_Nodes[id]

                // يلا كنا باقين الفوق، نولدو ولاد (حركات) جداد يلا ماكانوش
                يلا (node.children_count == 0):
                    جبد الحركات الممكنة ديال هاد الحنش (generate_possible_moves)
                    لكل حركة ممكنة:
                        node.children[i] = عقدة جديدة(move = ديك الحركة)

                // نختارو ولد (إما عشوائيا يلا visits==0، أو UCB يلا كلشي مزار)
                child = ختار_أحسن_ولد(node)

                child.visits++
                زيد child فـ traversed_nodes[id] // عقلنا عليه

                // طبق الحركة فالحنش الوهمي ديال السيميولايشن
                حنش.sdr.dir = child.move
                // أو حنش.edr = child.move بالنسبة للخصم

            // ج. المحاكاة
            // دابا كلشي عندو حركة، نديرو محاكاة ديال دورة وحدة
            SimulateTurn(sim_grid, sim_my_bots, sim_opp_bots)
            depth++

        // د. التقييم (Evaluation)
        sim_grid.score(sim_my_bots.countAlive(), sim_opp_bots.countAlive())
        my_reward = sim_grid.my_score
        opp_reward = sim_grid.opp_score

        // هـ. الرجوع وتحديث النقط (Backpropagation)
        لكل حنش ديالي:
            لكل عقدة فـ traversed_nodes[id]:
                عقدة.score += my_reward

        لكل حنش ديال الخصم:
            لكل عقدة فـ traversed_nodes[id]:
                عقدة.score += opp_reward

    // 3. سالا الوقت -> طبع النتيجة
    لكل حنش ديالي عايش:
        best_move = الولد ديال Root_Nodes[id] لي عندو أكبر عدد ديال visits
        طبع (id + best_move.move)
*/

// 1. حط هادو برا الدالة (Global Variables)
constexpr int MAX_NODES = 100000; // طابلو كيهز 100 ألف عقدة
Node node_pool[MAX_NODES];
int node_count = 0;

// 2. دالة خفيفة باش تجبد عقدة جديدة من الطابلو
Node* get_new_node() {
  if (node_count >= MAX_NODES) return nullptr; // حماية باش مانفوتوش الحد

  Node* n = &node_pool[node_count++]; // هزينا بلاصة فالميموري

  // ضروري نرجعو القيم للصفر حيت هاد الميموري غنكونو استعملناها فالتور لي فات
  n->score = 0;
  n->visits = 0;
  n->children_count = 0;
  n->parent = nullptr;
  n->children[0] = nullptr; n->children[1] = nullptr; n->children[2] = nullptr;

  return n;
}

// 3. الدالة ديالك دابا
void smitsimax() {
  // تصفير العداد فبداية كل دورة (Turn) باش نرجعو نستعملو الطابلو من الزيرو
  node_count = 0;

  Node *MyRoot_Nodes[MAX_SNAKEBOTS] = {nullptr, nullptr, nullptr, nullptr};
  Node *OppRoot_Nodes[MAX_SNAKEBOTS] = {nullptr, nullptr, nullptr, nullptr};

  for (int i = 0; i < MAX_SNAKEBOTS; i++) {
    if (my_snakebots.arr[i].is_alive)
      MyRoot_Nodes[i] = get_new_node(); // عوض new Node()
  }

  for (int i = 0; i < MAX_SNAKEBOTS; i++) {
    if (opp_snakebots.arr[i].is_alive)
      OppRoot_Nodes[i] = get_new_node(); // عوض new Node()
  }

  // الخطوة الجاية: لوب ديال الوقت (while time left)
  // ...
}

int main()
{
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
  grid_width = width;
  grid_height = height;
  for (int i = 0; i < height; i++)
  {
    string row;
    getline(cin, row);
    for (int j = 0; j < width; j++)
    {
      grid.grid2d.map[i][j] = row[j];
    }
  }
  int snakebots_per_player;
  cin >> snakebots_per_player;
  cin.ignore();
  for (int i = 0; i < snakebots_per_player; i++)
  {
    int my_snakebot_id;
    cin >> my_snakebot_id;
    cin.ignore();
    my_snakebots.arr[i] = MySnakebot(my_snakebot_id, UP);
    my_snakebot_count++;
  }
  for (int i = 0; i < snakebots_per_player; i++)
  {
    int opp_snakebot_id;
    cin >> opp_snakebot_id;
    cin.ignore();
    opp_snakebots.arr[i] = OppSnakebot(opp_snakebot_id, UP);
    opp_snakebot_count++;
  }

  // game loop
  while (1)
  {
    grid.turn++;
    int power_source_count;
    cin >> power_source_count;
    cin.ignore();
    grid.total_power_sources = power_source_count;
    for (int i = 0; i < power_source_count; i++)
    {
      int x;
      int y;
      cin >> x >> y;
      cin.ignore();
      grid.grid2d.map[y][x] = '@';
    }
    int snakebot_count;
    cin >> snakebot_count;
    cin.ignore();
    for (int i = 0; i < snakebot_count; i++)
    {
      int snakebot_id;
      string body;
      cin >> snakebot_id >> body;
      cin.ignore();
      MySnakebot *my_snakebot = my_snakebots.getSnakebotById(snakebot_id);
      if (my_snakebot != nullptr)
      {
        my_snakebot->body_to_chain(body);
        grid.insertMySnakebot(*my_snakebot);
        my_snakebot->is_alive = 1;
        my_snakebot_count++;
        grid.my_lengths_total += my_snakebot->length;
      }
      else
      {
        OppSnakebot *opp_snakebot = opp_snakebots.getSnakebotById(snakebot_id);
        if (opp_snakebot != nullptr)
        {
          opp_snakebot->body_to_chain(body);
          grid.insertOppSnakebot(*opp_snakebot);
          opp_snakebot->is_alive = 1;
          opp_snakebot_count++;
        }
      }
    }

    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;
    for (int i = 0; i < MAX_SNAKEBOTS; i++)
    {
      if (my_snakebots.arr[i].is_alive)
      {
        my_snakebots.arr[i].generate_possible_moves(grid.grid2d.map, grid.width, grid.height);
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
