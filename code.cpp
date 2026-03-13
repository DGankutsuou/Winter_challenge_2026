#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#undef _GLIBCXX_DEBUG				 // disable run-time bound checking, etc
#pragma GCC optimize("Ofast,inline") // Ofast = O3,fast-math,allow-store-data-races,no-protect-parens

#ifndef __POPCNT__ // not march=generic
#endif

#pragma GCC target("bmi,bmi2,lzcnt,popcnt")						 // bit manipulation
#pragma GCC target("movbe")										 // byte swap
#pragma GCC target("aes,pclmul,rdrnd")							 // encryption
#pragma GCC target("avx,avx2,f16c,fma,sse3,ssse3,sse4.1,sse4.2") // SIMD
#pragma GCC optimize "Ofast,unroll-loops,omit-frame-pointer,inline"
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#pragma GCC target("rdrnd", "popcnt", "avx", "bmi2")

using namespace std;

struct s_cord
{
  int x;
  int y;
  bool operator==(const s_cord& cord) {return x==cord.x && y==cord.y;}
};

enum e_direction
{
  UP,
  DOWN,
  RIGHT,
  LEFT
};

struct s_move
{
  int id;
  e_direction dir;
};

class MySnakebot;
class OppSnakebot;

class MySnakebot
{
public:
  MySnakebot(int i, e_direction d) : id(i), edr(d) {}
  int id;
  enum e_direction edr;
  vector<s_cord> chain;

  void move(e_direction dir)
  {
    string directions[4] = {"UP", "DOWN", "RIGHT", "LEFT"};
    cout << id << " " << directions[dir];
    if (dir == UP && edr == DOWN)
      return;
    if (dir == DOWN && edr == UP)
      return;
    if (dir == RIGHT && edr == LEFT)
      return;
    if (dir == LEFT && edr == RIGHT)
      return;
    edr = dir;
  }

  void move(char **grid)
  {
    string dir[4] = {"UP", "DOWN", "RIGHT", "LEFT"};
    int i = rand() % 4;
    cout << id << " " << dir[i];
    if (edr == i)
      return;
    if (i == UP && edr == DOWN)
      return;
    if (i == DOWN && edr == UP)
      return;
    if (i == RIGHT && edr == LEFT)
      return;
    if (i == LEFT && edr == RIGHT)
      return;
    edr = static_cast<e_direction>(i);
  }

  void body_to_chain(string &body)
  {
    chain.clear();
    size_t pos = 0;
    while ((pos = body.find(':')) != string::npos)
    {
      string token = body.substr(0, pos);
      size_t comma_pos = token.find(',');
      s_cord c;
      c.x = stoi(token.substr(0, comma_pos));
      c.y = stoi(token.substr(comma_pos + 1));
      chain.push_back(c);
      body.erase(0, pos + 1);
    }
    // last token
    size_t comma_pos = body.find(',');
    s_cord c;
    c.x = stoi(body.substr(0, comma_pos));
    c.y = stoi(body.substr(comma_pos + 1));
    chain.push_back(c);
  }

  MySnakebot &getSnakebotById(int id, vector<MySnakebot> &my_snakebots)
  {
    for (MySnakebot &snakebot : my_snakebots)
    {
      if (snakebot.id == id)
        return snakebot;
    }
    return my_snakebots[0];
  }
};

class OppSnakebot
{
public:
  OppSnakebot(int i, int d) : id(i), dr(d) {}
  int id;
  int dr;
  vector<s_cord> chain;

  void body_to_chain(string &body)
  {
    chain.clear();
    size_t pos = 0;
    while ((pos = body.find(':')) != string::npos)
    {
      string token = body.substr(0, pos);
      size_t comma_pos = token.find(',');
      s_cord c;
      c.x = stoi(token.substr(0, comma_pos));
      c.y = stoi(token.substr(comma_pos + 1));
      chain.push_back(c);
      body.erase(0, pos + 1);
    }
    // last token
    size_t comma_pos = body.find(',');
    s_cord c;
    c.x = stoi(body.substr(0, comma_pos));
    c.y = stoi(body.substr(comma_pos + 1));
    chain.push_back(c);
  }

  OppSnakebot &getSnakebotById(int id, vector<OppSnakebot> &opp_snakebots)
  {
    for (OppSnakebot &snakebot : opp_snakebots)
    {
      if (snakebot.id == id)
        return snakebot;
    }
    return opp_snakebots[0];
  }
};

class Grid
{
public:
  int width;
  int height;
  char **grid2d;
  Grid() : width(0), height(0), grid2d(nullptr) {}
  Grid(int w, int h) : width(w), height(h)
  {
    grid2d = new char *[height];
    for (int i = 0; i < height; i++)
    {
      grid2d[i] = new char[width];
      for (int j = 0; j < width; j++)
      {
        grid2d[i][j] = '.';
      }
    }
  }
  ~Grid()
  {
    if (grid2d != nullptr)
    {
      for (int i = 0; i < height; i++)
      {
        if (grid2d[i] != nullptr)
          delete[] grid2d[i];
      }
      delete[] grid2d;
    }
  }

  void recreate_grid(int w, int h)
  {
    if (grid2d != nullptr)
    {
      for (int i = 0; i < height; i++)
      {
        if (grid2d[i] != nullptr)
          delete[] grid2d[i];
      }
      delete[] grid2d;
    }
    width = w;
    height = h;
    grid2d = new char *[height];
    for (int i = 0; i < height; i++)
    {
      grid2d[i] = new char[width];
    }
  }

  void refresh_grid()
  {
    for (int i = 0; i < height; i++)
    {
      for (int j = 0; j < width; j++)
      {
        if (grid2d[i][j] != '#')
          grid2d[i][j] = '.';
      }
    }
  }

  void print_grid()
  {
    for (int i = 0; i < height; i++)
    {
      for (int j = 0; j < width; j++)
      {
        cerr << grid2d[i][j];
      }
      cerr << endl;
    }
  }

  void insertMySnakebot(MySnakebot &snakebot)
  {
    for (s_cord &c : snakebot.chain)
    {
      grid2d[c.y][c.x] = snakebot.id + 'A';
    }
  }

  void insertOppSnakebot(OppSnakebot &snakebot)
  {
    for (s_cord &c : snakebot.chain)
    {
      grid2d[c.y][c.x] = snakebot.id + 'A';
    }
  }
};

// this function will generate all possible combinations of moves for my snakebots and return them as a vector of vector of s_move
vector<vector<s_move>> generateAllMyMoves(vector<MySnakebot> &my_snakebots)
{
  vector<vector<s_move>> all_moves;
  vector<s_move> moves;
  s_move move;
  for (int i = 0; i < my_snakebots.size(); i++)
  {
    move.id = my_snakebots[i].id;
    move.dir = UP;
    moves.push_back(move);
  }
  while (1)
  {
    break;
  }
  return all_moves;
}

vector<MySnakebot> my_snakebots;
vector<MySnakebot> my_snakebots_old;

vector<OppSnakebot> opp_snakebots;
vector<OppSnakebot> opp_snakebots_old;

Grid grid;

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
  for (int i = 0; i < height; i++)
  {
    string row;
    getline(cin, row);
    for (int j = 0; j < width; j++)
    {
      grid.grid2d[i][j] = row[j];
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
    my_snakebots_old.push_back(MySnakebot(my_snakebot_id, UP));
  }
  for (int i = 0; i < snakebots_per_player; i++)
  {
    int opp_snakebot_id;
    cin >> opp_snakebot_id;
    cin.ignore();
    opp_snakebots_old.push_back(OppSnakebot(opp_snakebot_id, UP));
  }

  // game loop
  while (1)
  {
    int power_source_count;
    cin >> power_source_count;
    cin.ignore();
    for (int i = 0; i < power_source_count; i++)
    {
      int x;
      int y;
      cin >> x >> y;
      cin.ignore();
      grid.grid2d[y][x] = 'p';
    }
    int snakebot_count;
    cin >> snakebot_count;
    cin.ignore();
    for (int i = 0; i < snakebot_count; i++)
    {
      int snakebot_id;
      string body;
      bool is_found = false;
      cin >> snakebot_id >> body;
      cin.ignore();
      for (int i = 0; i < my_snakebots_old.size(); i++)
      {
        if (my_snakebots_old[i].id == snakebot_id)
        {
          my_snakebots_old[i].body_to_chain(body);
          my_snakebots.push_back(my_snakebots_old[i]);
          grid.insertMySnakebot(my_snakebots_old[i]);
          is_found = true;
          break;
        }
      }
      if (!is_found)
      {
        for (int i = 0; i < opp_snakebots_old.size(); i++)
        {
          if (opp_snakebots_old[i].id == snakebot_id)
          {
            opp_snakebots_old[i].body_to_chain(body);
            opp_snakebots.push_back(opp_snakebots_old[i]);
            grid.insertOppSnakebot(opp_snakebots_old[i]);
            break;
          }
        }
      }
    }

    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;
    grid.print_grid();
    for (int i = 0; i < my_snakebots.size(); i++)
    {
      my_snakebots[i].move(grid.grid2d);
      if (i < my_snakebots.size() - 1)
        cout << ";";
    }
    cout << endl;
    // cout << "wait" << endl;
    my_snakebots_old = my_snakebots;
    my_snakebots.clear();
    grid.refresh_grid();
  }
}
