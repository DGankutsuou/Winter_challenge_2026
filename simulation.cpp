#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

struct SimSnakeRef {
  bool is_my = false;
  MySnakebot *my = nullptr;
  OppSnakebot *opp = nullptr;

  int &length() { return is_my ? my->length : opp->length; }
  int &alive() { return is_my ? my->is_alive : opp->is_alive; }
  s_direction &direction() { return is_my ? my->sdr : opp->sdr; }
  s_cord *chain() { return is_my ? my->chain : opp->chain; }
};

long long pack_coord(int x, int y) {
  return (static_cast<long long>(y) << 32) ^ static_cast<unsigned int>(x);
}

bool in_bounds(const Grid &grid, int x, int y) {
  return x >= 0 && x < grid.width && y >= 0 && y < grid.height;
}

s_cord move_head(const s_cord &head, e_direction dir) {
  s_cord next = head;
  if (dir == UP)
    --next.y;
  else if (dir == DOWN)
    ++next.y;
  else if (dir == RIGHT)
    ++next.x;
  else if (dir == LEFT)
    --next.x;
  return next;
}

} // namespace

void simulation(Grid &grid, SMySnakebots &my_snakes, SOppSnakebots &opp_snakes) {
  std::unordered_set<long long> power_cells;
  for (int y = 0; y < grid.height; ++y) {
    for (int x = 0; x < grid.width; ++x) {
      if (grid.grid2d[y][x] == '@') {
        power_cells.insert(pack_coord(x, y));
      }
    }
  }

  std::vector<SimSnakeRef> snakes;
  snakes.reserve(MAX_SNAKEBOTS * 2);

  for (int i = 0; i < MAX_SNAKEBOTS; ++i) {
    if (my_snakes.arr[i].id != -1 && my_snakes.arr[i].is_alive &&
        my_snakes.arr[i].length > 0) {
      snakes.push_back(SimSnakeRef{true, &my_snakes.arr[i], nullptr});
    }
  }
  for (int i = 0; i < MAX_SNAKEBOTS; ++i) {
    if (opp_snakes.arr[i].id != -1 && opp_snakes.arr[i].is_alive &&
        opp_snakes.arr[i].length > 0) {
      snakes.push_back(SimSnakeRef{false, nullptr, &opp_snakes.arr[i]});
    }
  }

  std::vector<std::vector<s_cord>> next_bodies(snakes.size());
  std::unordered_set<long long> eaten_powers;

  for (size_t i = 0; i < snakes.size(); ++i) {
    SimSnakeRef &snake = snakes[i];
    const s_cord new_head = move_head(snake.chain()[0], snake.direction().dir);

    bool eats_power = false;
    if (in_bounds(grid, new_head.x, new_head.y)) {
      eats_power = power_cells.count(pack_coord(new_head.x, new_head.y)) > 0;
    }

    std::vector<s_cord> body;
    body.reserve(std::min(MAX_CHAIN_LENGTH, snake.length() + (eats_power ? 1 : 0)));
    body.push_back(new_head);

    int body_to_copy = snake.length() - (eats_power ? 0 : 1);
    if (body_to_copy < 0) {
      body_to_copy = 0;
    }
    for (int seg = 0; seg < body_to_copy && static_cast<int>(body.size()) < MAX_CHAIN_LENGTH;
         ++seg) {
      body.push_back(snake.chain()[seg]);
    }

    next_bodies[i] = std::move(body);
    if (eats_power) {
      eaten_powers.insert(pack_coord(new_head.x, new_head.y));
    }
  }

  for (size_t i = 0; i < snakes.size(); ++i) {
    SimSnakeRef &snake = snakes[i];
    snake.length() = static_cast<int>(next_bodies[i].size());
    for (int seg = 0; seg < snake.length(); ++seg) {
      snake.chain()[seg] = next_bodies[i][seg];
    }
  }

  for (long long cell : eaten_powers) {
    power_cells.erase(cell);
  }

  std::unordered_map<long long, int> occupied_cells;
  for (size_t i = 0; i < snakes.size(); ++i) {
    SimSnakeRef &snake = snakes[i];
    if (!snake.alive() || snake.length() == 0) {
      continue;
    }
    for (int seg = 0; seg < snake.length(); ++seg) {
      const s_cord &part = snake.chain()[seg];
      occupied_cells[pack_coord(part.x, part.y)]++;
    }
  }

  std::vector<size_t> beheaded_snakes;
  for (size_t i = 0; i < snakes.size(); ++i) {
    SimSnakeRef &snake = snakes[i];
    if (!snake.alive() || snake.length() == 0) {
      continue;
    }

    const s_cord &head = snake.chain()[0];
    const bool hit_wall = !in_bounds(grid, head.x, head.y) ||
                          grid.grid2d[head.y][head.x] == '#';

    bool hit_body = false;
    if (!hit_wall) {
      auto it = occupied_cells.find(pack_coord(head.x, head.y));
      hit_body = it != occupied_cells.end() && it->second > 1;
    }

    if (hit_wall || hit_body) {
      beheaded_snakes.push_back(i);
    }
  }

  for (size_t index : beheaded_snakes) {
    SimSnakeRef &snake = snakes[index];
    if (!snake.alive()) {
      continue;
    }
    if (snake.length() <= 3) {
      snake.length() = 0;
      snake.alive() = 0;
      continue;
    }
    for (int seg = 1; seg < snake.length(); ++seg) {
      snake.chain()[seg - 1] = snake.chain()[seg];
    }
    snake.length()--;
  }

  bool something_fell = true;
  while (something_fell) {
    something_fell = false;

    for (size_t i = 0; i < snakes.size(); ++i) {
      SimSnakeRef &snake = snakes[i];
      if (!snake.alive() || snake.length() == 0) {
        continue;
      }

      bool can_fall = true;
      for (int seg = 0; seg < snake.length() && can_fall; ++seg) {
        const int below_x = snake.chain()[seg].x;
        const int below_y = snake.chain()[seg].y + 1;

        if (in_bounds(grid, below_x, below_y) && grid.grid2d[below_y][below_x] == '#') {
          can_fall = false;
          break;
        }

        if (in_bounds(grid, below_x, below_y) &&
            power_cells.count(pack_coord(below_x, below_y)) > 0) {
          can_fall = false;
          break;
        }

        for (size_t other = 0; other < snakes.size(); ++other) {
          if (other == i) {
            continue;
          }
          SimSnakeRef &other_snake = snakes[other];
          if (!other_snake.alive() || other_snake.length() == 0) {
            continue;
          }
          for (int other_seg = 0; other_seg < other_snake.length(); ++other_seg) {
            const s_cord &part = other_snake.chain()[other_seg];
            if (part.x == below_x && part.y == below_y) {
              can_fall = false;
              break;
            }
          }
          if (!can_fall) {
            break;
          }
        }
      }

      if (!can_fall) {
        continue;
      }

      something_fell = true;
      for (int seg = 0; seg < snake.length(); ++seg) {
        snake.chain()[seg].y++;
      }

      bool is_outside = true;
      for (int seg = 0; seg < snake.length(); ++seg) {
        if (snake.chain()[seg].y < grid.height) {
          is_outside = false;
          break;
        }
      }
      if (is_outside) {
        snake.length() = 0;
        snake.alive() = 0;
      }
    }
  }

  grid.refresh_grid();
  for (long long cell : power_cells) {
    const int y = static_cast<int>(cell >> 32);
    const int x = static_cast<int>(static_cast<unsigned int>(cell));
    if (in_bounds(grid, x, y)) {
      grid.grid2d[y][x] = '@';
    }
  }

  for (int i = 0; i < MAX_SNAKEBOTS; ++i) {
    if (my_snakes.arr[i].id != -1 && my_snakes.arr[i].is_alive && my_snakes.arr[i].length > 0) {
      grid.insertMySnakebot(my_snakes.arr[i]);
    }
  }
  for (int i = 0; i < MAX_SNAKEBOTS; ++i) {
    if (opp_snakes.arr[i].id != -1 && opp_snakes.arr[i].is_alive && opp_snakes.arr[i].length > 0) {
      grid.insertOppSnakebot(opp_snakes.arr[i]);
    }
  }
}
