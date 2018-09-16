#ifndef ORDER_H
#define ORDER_H

#include <assert.h>
#include <cinttypes>

#include "Actions.h"
#include "Common.h"
#include "Enums.h"
#include "Exceptions.h"

namespace mvs {
namespace orderbook {

struct Order {
  template <Direction dir>
  Order(const OrderAction<Action::Add, dir> &oaction)
      : m_oid(oaction.getOid()), m_volume(oaction.getVolume()) {}

  Order(Order &) = delete;
  Order &operator=(Order &) = delete;

  Order(Order &&) = default;
  Order &operator=(Order &&) = default;

  uint32_t getOid() const { return m_oid; }
  uint32_t getVolume() const { return m_volume; }
  void reduceVolume(uint32_t volume) noexcept {
    // if m_volume isnt' bigger, we'd have just removed the whole order instead
    // of trying to reduce the volume
    assert(m_volume > volume);
    m_volume -= volume;
  }

private:
  uint32_t m_oid;
  uint32_t m_volume;
};

} // namespace orderbook
} // namespace mvs

#endif // ORDER_H
