#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <assert.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <vector>

#include "Enums.h"
#include "Exceptions.h"
#include "Order.h"

namespace mvs {
namespace orderbook {

template <Direction direction> struct MapType {};

// buy orders are stored in a map that has the highest price first
template <> struct MapType<Direction::Buy> {
  using VctT = std::vector<Order>;
  using value_type = std::map<uint32_t, VctT, std::greater<uint32_t>>;
};

// sell orders are stored in a map that has the lowest price first
template <> struct MapType<Direction::Sell> {
  using VctT = std::vector<Order>;
  using value_type = std::map<uint32_t, VctT, std::less<uint32_t>>;
};

template <Direction direction>
struct OrderSide : public MapType<direction>::value_type {
  using MapT = typename MapType<direction>::value_type;
  using VctT = typename MapT::mapped_type;
  using value_type = typename MapT::value_type;
  using iterator = typename MapT::iterator;

  OrderSide() = default;
  OrderSide(OrderSide &) = delete;
  OrderSide &operator=(OrderSide &) = delete;

  inline void handle(const OrderAction<Action::Add, direction> &oaction);
  inline void handle(const OrderAction<Action::Remove, direction> &oaction);
  inline void handle(const OrderAction<Action::Modify, direction> &oaction);

  value_type const &front() const { return *MapT::begin(); }
  value_type &front() { return *MapT::begin(); }
};

struct OrderBook {
  using BuySide = OrderSide<Direction::Buy>;
  using SellSide = OrderSide<Direction::Sell>;

  OrderBook() = default;
  OrderBook(OrderBook &) = delete;
  OrderBook &operator=(OrderBook &) = delete;

  double getMidPrice() const;

  template <Action action, typename FillsCallback>
  void handle(const OrderAction<action, Direction::Buy> &oaction,
              FillsCallback &cb);

  template <Action action, typename FillsCallback>
  void handle(const OrderAction<action, Direction::Sell> &oaction,
              FillsCallback &cb);

  template <Direction dir, typename FillsCallback>
  void match(FillsCallback &cb) noexcept;

  BuySide const &getBuySide() const { return m_buySide; }
  SellSide const &getSellSide() const { return m_sellSide; }

  BuySide m_buySide;
  SellSide m_sellSide;
};

template <Direction direction>
void OrderSide<direction>::handle(
    const OrderAction<Action::Add, direction> &oaction) {
  auto &vct = MapT::operator[](oaction.getPrice());
  auto iter =
      std::find_if(vct.begin(), vct.end(), [&oaction](const Order &order) {
        return order.getOid() == oaction.getOid();
      });
  if (unlikely(iter != vct.end())) {
    throw DuplicateOrderIdError(oaction.getOid());
  } else {
    vct.emplace_back(oaction);
  }
}

template <Direction direction>
inline void OrderSide<direction>::handle(
    const OrderAction<Action::Remove, direction> &oaction) {
  auto mIter = MapT::find(oaction.getPrice());
  if (mIter == MapT::end()) {
    // I don't even know the price .. so I definitely don't know this order.
    throw UnknownOrderIdError(oaction.getOid());
  } else {
    auto &vct = mIter->second;
    auto iter =
        std::find_if(vct.begin(), vct.end(), [&oaction](const Order &order) {
          return order.getOid() == oaction.getOid();
        });
    if (iter == vct.end()) {
      // I know the price but I don't know this order.
      throw UnknownOrderIdError(oaction.getOid());
    } else {
      if (1u == vct.size()) {
        // whole level taken out
        MapT::erase(mIter);
      } else {
        // this order taken out
        vct.erase(iter);
      }
    }
  }
}

template <Direction direction>
void OrderSide<direction>::handle(
    const OrderAction<Action::Modify, direction> &oaction) {
  // find existing
  bool found(false);
  for (auto mIter = MapT::begin(); mIter != MapT::end(); mIter++) {
    auto &vct = mIter->second;
    auto iter =
        std::find_if(vct.begin(), vct.end(), [&oaction](const Order &order) {
          return order.getOid() == oaction.getOid();
        });
    // if found, then we delete the level or individual order
    if (iter != vct.end()) {
      if (1u == vct.size()) {
        // whole level taken out
        MapT::erase(mIter);
      } else {
        // this order taken out
        vct.erase(iter);
      }
      found = true;
      break;
    }
  }
  // if not found, we're done - raise an exception
  if (!found) {
    throw UnknownOrderIdError(oaction.getOid());
  }

  // insert new
  handle(OrderAction<Action::Add, direction>(
      oaction.getOid(), oaction.getVolume(), oaction.getPrice()));
}

double OrderBook::getMidPrice() const {
  auto buyIter = m_buySide.begin();
  auto sellIter = m_sellSide.begin();
  return (buyIter != m_buySide.end() && sellIter != m_sellSide.end())
             ? (buyIter->first + sellIter->first) / 2.0
             : std::numeric_limits<double>::quiet_NaN();
}

template <Action action, typename FillsCallback>
void OrderBook::handle(const OrderAction<action, Direction::Buy> &oaction,
                       FillsCallback &cb) {
  m_buySide.handle(oaction);

  if (Action::Add == action) {
    match<Direction::Buy, FillsCallback>(cb);
  }
}

template <Action action, typename FillsCallback>
void OrderBook::handle(const OrderAction<action, Direction::Sell> &oaction,
                       FillsCallback &cb) {
  m_sellSide.handle(oaction);

  if (Action::Add == action) {
    match<Direction::Sell, FillsCallback>(cb);
  }
}

template <Direction dir, typename FillsCallback>
void OrderBook::match(FillsCallback &cb) noexcept {
  auto isCrossed = [](auto const &buySide, auto const &sellSide) {
    return !buySide.empty() && !sellSide.empty() &&
           buySide.front().first >= sellSide.front().first;
  };

  while (isCrossed(m_buySide, m_sellSide)) {

    auto reduceSize = [](auto &side, auto volume) {
      auto &orders = side.front().second;
      if (volume == orders.front().getVolume()) {
        if (1u == orders.size()) {
          side.erase(side.begin());
        } else {
          orders.erase(orders.begin());
        }
      } else {
        orders.front().reduceVolume(volume);
      }

    };

    auto &buyOrders = m_buySide.front().second;
    auto &sellOrders = m_sellSide.front().second;
    const auto volume(std::min(buyOrders.front().getVolume(),
                               sellOrders.front().getVolume()));
    const auto price(Direction::Buy == dir ? m_sellSide.front().first
                                           : m_buySide.front().first);
    const auto buyOid(m_buySide.front().second.front().getOid());
    const auto sellOid(m_sellSide.front().second.front().getOid());
    const Trade trade(buyOid, sellOid, volume, price);
    cb(trade);

    reduceSize(m_buySide, volume);
    reduceSize(m_sellSide, volume);
  }
}

template <Direction direction>
std::ostream &operator<<(std::ostream &os, const OrderSide<direction> &side) {

  auto getVolume = [](const std::vector<Order> &orders) {
    uint32_t volume(0);
    assert(std::distance(orders.begin(), orders.end()) != 0);
    std::for_each(orders.begin(), orders.end(), [&volume](const Order &order) {
      volume += order.getVolume();
    });
    return volume;
  };

  for (const auto &pair : side) {
    const auto price(pair.first);
    const auto volume(getVolume(pair.second));
    os << volume << "x" << price << " ";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const OrderBook &book) {
  os << " -- book -- " << std::endl;
  os << " -- bid : " << std::endl;
  os << book.getBuySide() << std::endl;
  os << " -- ask : " << std::endl;
  os << book.getSellSide() << std::endl;
  return os;
}

} // namespace orderbook
} // namespace mvs

#endif // ORDERBOOK_H
