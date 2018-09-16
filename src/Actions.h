#ifndef ACTIONS_H
#define ACTIONS_H

#include <cinttypes>
#include <ostream>

#include "Enums.h"

namespace mvs {
namespace orderbook {

template <Action action, Direction direction> struct OrderAction {
  using SelfT = OrderAction<action, direction>;
  OrderAction(const uint32_t oid, const uint32_t volume, const uint32_t price)
      : m_oid(oid), m_volume(volume), m_price(price) {}

  OrderAction(SelfT &) = delete;
  OrderAction &operator=(SelfT &) = delete;

  uint32_t getOid() const { return m_oid; }
  uint32_t getVolume() const { return m_volume; }
  uint32_t getPrice() const { return m_price; }

  static constexpr Direction dir = direction;

private:
  const uint32_t m_oid;
  const uint32_t m_volume;
  const uint32_t m_price;
};

struct Trade {
  Trade(uint32_t buyOid, uint32_t sellOid, uint32_t volume, uint32_t price)
      : m_buyOid(buyOid), m_sellOid(sellOid), m_volume(volume), m_price(price) {
  }

  Trade(const Trade &) = delete;
  Trade &operator=(const Trade &) = delete;

  Trade(Trade &&) = default;
  Trade &operator=(Trade &&) = default;

  uint32_t getBuyOid() const { return m_buyOid; }
  uint32_t getSellOid() const { return m_sellOid; }
  uint32_t getVolume() const { return m_volume; }
  uint32_t getPrice() const { return m_price; }

private:
  const uint32_t m_buyOid;
  const uint32_t m_sellOid;
  const uint32_t m_volume;
  const uint32_t m_price;
};

std::ostream &operator<<(std::ostream &os, const Trade &trade) {
  os << "T buy_oid:" << trade.getBuyOid()
     << " vs sell_oid:" << trade.getSellOid() << " price:" << trade.getPrice()
     << " volume:" << trade.getVolume();
  return os;
}

} // namespace orderbook
} // namespace mvs

#endif // ACTIONS_H
