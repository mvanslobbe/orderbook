#ifndef ENUMS_H
#define ENUMS_H

namespace mvs {
namespace orderbook {

enum class Action : char { Add = 'A', Modify = 'M', Remove = 'X' };

enum class Direction : char { Buy = 'B', Sell = 'S' };

} // namespace orderbook
} // namespace mvs

#endif // ENUMS_H
