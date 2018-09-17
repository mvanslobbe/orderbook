#ifndef ENUMS_H
#define ENUMS_H

#include <ostream>

namespace mvs {
namespace orderbook {

enum class Action : char { Add = 'A', Modify = 'M', Remove = 'X' };

enum class Direction : char { Buy = 'B', Sell = 'S' };

std::ostream &operator<<(std::ostream &os, Action action) {
  switch (action) {
  case Action::Add:
    os << "Add";
    break;
  case Action::Modify:
    os << "Modify";
    break;
  case Action::Remove:
    os << "Remove";
    break;
  default:
    os << "Unknown";
    break;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, Direction direction) {
  switch (direction) {
  case Direction::Buy:
    os << "Buy";
    break;
  case Direction::Sell:
    os << "Sell";
    break;
  default:
    os << "Unknown";
    break;
  }
  return os;
}

} // namespace orderbook
} // namespace mvs

#endif // ENUMS_H
