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

string *grid2d;

class MySnakebot {
public:
  MySnakebot(int i, int d) : id(i), dr(d) {}
  int id;
  int dr;
  string body;
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
};

vector<MySnakebot> my_snakebots;
vector<MySnakebot> my_snakebots_old;

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
  grid2d = new string[height];
  for (int i = 0; i < height; i++) {
    string row;
    getline(cin, row);
    grid2d[i] = row;
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
      cin >> snakebot_id >> body;
      cin.ignore();
      for (int i = 0; i < my_snakebots_old.size(); i++)
      {
        if (my_snakebots_old[i].id == snakebot_id)
        {
            my_snakebots_old[i].body = body;
            my_snakebots.push_back(my_snakebots_old[i]);
        }
      }
    }

    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;

    for (int i = 0; i < snakebots_per_player; i++) {
      my_snakebots[i].move();
      if (i < snakebots_per_player - 1)
        cout << ";";
    }
    cout << endl;
    // cout << "wait" << endl;
    my_snakebots_old = my_snakebots;
    my_snakebots.clear();
  }
  delete[] grid2d;
}
