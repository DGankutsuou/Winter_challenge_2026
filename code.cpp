#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

struct cord
{
    int x;
    int y;
};

class MySnakebot;
class OppSnakebot;

class MySnakebot {
public:
  MySnakebot(int i, int d) : id(i), dr(d) {}
  int id;
  int dr;
  vector <cord> chain;

  void move(const string &dir) {
    cout << id << " " << dir;
    dr = dir[0];
  }

  void move() {
    string dir[4] = {"UP", "DOWN", "RIGHT", "LEFT"};
    int i = rand() % 4;
    cout << id << " " << dir[i];
    dr = dir[i][0];
  }

  void body_to_chain(string& body)
  {
    chain.clear();
    size_t pos = 0;
    while ((pos = body.find(':')) != string::npos) {
        string token = body.substr(0, pos);
        size_t comma_pos = token.find(',');
        cord c;
        c.x = stoi(token.substr(0, comma_pos));
        c.y = stoi(token.substr(comma_pos + 1));
        chain.push_back(c);
        body.erase(0, pos + 1);
    }
    // last token
    size_t comma_pos = body.find(',');
    cord c;
    c.x = stoi(body.substr(0, comma_pos));
    c.y = stoi(body.substr(comma_pos + 1));
    chain.push_back(c);
  }
};

class OppSnakebot {
public:
  OppSnakebot(int i, int d) : id(i), dr(d) {}
  int id;
  int dr;
  vector <cord> chain;

  void body_to_chain(string& body)
  {
    chain.clear();
    size_t pos = 0;
    while ((pos = body.find(':')) != string::npos) {
        string token = body.substr(0, pos);
        size_t comma_pos = token.find(',');
        cord c;
        c.x = stoi(token.substr(0, comma_pos));
        c.y = stoi(token.substr(comma_pos + 1));
        chain.push_back(c);
        body.erase(0, pos + 1);
    }
    // last token
    size_t comma_pos = body.find(',');
    cord c;
    c.x = stoi(body.substr(0, comma_pos));
    c.y = stoi(body.substr(comma_pos + 1));
    chain.push_back(c);
  }
};

class Grid {
public:
  int width;
  int height;
  char **grid2d;
  Grid() : width(0), height(0), grid2d(nullptr) {}
  Grid(int w, int h) : width(w), height(h)
  {
    grid2d = new char*[height];
    for (int i = 0; i < height; i++) {
      grid2d[i] = new char[width];
      for (int j = 0; j < width; j++) {
        grid2d[i][j] = '.';
      }
    }
  }
  ~Grid() {
    if (grid2d != nullptr)
    {
      for (int i = 0; i < height; i++) {
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
      for (int i = 0; i < height; i++) {
        if (grid2d[i] != nullptr)
          delete[] grid2d[i];
      }
      delete[] grid2d;
    }
    width = w;
    height = h;
    grid2d = new char*[height];
  }

  void refresh_grid()
  {
    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        if (grid2d[i][j] != '#')
          grid2d[i][j] = '.';
      }
    }
  }

  void print_grid()
  {
    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        cerr << grid2d[i][j];
      }
      cerr << endl;
    }
  }

  void insertMySnakebot(MySnakebot& snakebot)
  {
    for (cord c : snakebot.chain)
    {
      grid2d[c.y][c.x] = snakebot.id + '0';
    } 
  }

  void insertOppSnakebot(OppSnakebot& snakebot)
  {
    for (cord c : snakebot.chain)
    {
      grid2d[c.y][c.x] = snakebot.id + '0';
    } 
  }
};

vector<MySnakebot> my_snakebots;
vector<MySnakebot> my_snakebots_old;

vector<OppSnakebot> opp_snakebots;
vector<OppSnakebot> opp_snakebots_old;

Grid grid;

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
    cerr << row << endl;
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
    my_snakebots_old.push_back(MySnakebot(my_snakebot_id, 'U'));
  }
  for (int i = 0; i < snakebots_per_player; i++) {
    int opp_snakebot_id;
    cin >> opp_snakebot_id;
    cin.ignore();
    opp_snakebots_old.push_back(OppSnakebot(opp_snakebot_id, 'U'));
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
    }
    int snakebot_count;
    cin >> snakebot_count;
    cin.ignore();
    for (int i = 0; i < snakebot_count; i++) {
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
    for (int i = 0; i < snakebots_per_player; i++) {
      my_snakebots[i].move();
      if (i < snakebots_per_player - 1)
        cout << ";";
    }
    cout << endl;
    // cout << "wait" << endl;
    my_snakebots_old = my_snakebots;
    my_snakebots.clear();
    grid.refresh_grid();
  }
}
