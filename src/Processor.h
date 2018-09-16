#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <sstream>
#include <string>
#include <type_traits>

#include "Actions.h"
#include "Common.h"
#include "Exceptions.h"
#include "OrderBook.h"

namespace mvs {
namespace orderbook {

class splitter : public std::string {};

// we can parse absolutely anything .. as long as it's an unsigned integral
// value
template <typename T>
typename std::enable_if<
    std::is_unsigned<T>::value && std::is_integral<T>::value, T>::type
parse(const std::string &input) {
  T output(0);
  for (char c : input) {
    if (c == ' ' || c == '\r') {
      return output;
    } else if (unlikely(c < '0' || c > '9')) {
      throw ParseError("invalid number");
    } else {
      output *= 10;
      output += (c - '0');
    }
  }
  return output;
}

std::istream &operator>>(std::istream &is, splitter &output) {
  std::getline(is, output, ',');
  return is;
}

template <typename BookT = OrderBook> struct Processor {
  using SelfT = Processor<BookT>;
  Processor(BookT &book) : m_book(book) {}
  Processor(SelfT &) = delete;
  Processor operator=(SelfT &) = delete;

  template <Action action, typename FillsCallback>
  void process(const uint32_t oid, const Direction dir, const uint32_t volume,
               const uint32_t price, FillsCallback &cb);

  template <typename FillsCallback>
  void process(const std::string &ln, FillsCallback &cb);

private:
  BookT &m_book;
};

template <typename BookT>
template <Action action, typename FillsCallback>
void Processor<BookT>::process(const uint32_t oid, const Direction dir,
                               const uint32_t volume, const uint32_t price,
                               FillsCallback &cb) {
  switch (dir) {
  case Direction::Buy: {
    OrderAction<action, Direction::Buy> oaction(oid, volume, price);
    m_book.handle(oaction, cb);
  } break;
  case Direction::Sell: {
    OrderAction<action, Direction::Sell> oaction(oid, volume, price);
    m_book.handle(oaction, cb);
  } break;
  default:
    throw ParseError("dir mismatch");
    break;
  }
}

template <typename BookT>
template <typename FillsCallback>
void Processor<BookT>::process(const std::string &ln, FillsCallback &cb) {
  // strip out any comments
  const auto comments = ln.find("//");
  const auto line =
      (comments != std::string::npos) ? ln.substr(0, comments) : ln;

  // tokenize
  std::istringstream iss(line);
  const std::vector<std::string> results((std::istream_iterator<splitter>(iss)),
                                         std::istream_iterator<splitter>());
  if (unlikely(4 > results.size())) {
    throw ParseError(line);
  }

  // the first character should match our known 'actions'
  // add and modify look the same, remove is different
  Action action = (Action)results[0][0];
  switch (action) {
  case Action::Add:
  case Action::Modify: {
    if (unlikely(5 != results.size())) {
      throw ParseError(line);
    }
    const uint32_t oid = parse<uint32_t>(results[1]);
    const Direction dir = (Direction)results[2][0];
    const uint32_t volume = parse<uint32_t>(results[3]);
    const uint32_t price = parse<uint32_t>(results[4]);
    if (Action::Add == action) {
      process<Action::Add, FillsCallback>(oid, dir, volume, price, cb);
    } else if (Action::Modify == action) {
      process<Action::Modify, FillsCallback>(oid, dir, volume, price, cb);
    }
  } break;
  case Action::Remove: {
    if (unlikely(4 != results.size())) {
      throw ParseError(line);
    }
    const uint32_t oid = parse<uint32_t>(results[1]);
    const Direction dir = (Direction)results[2][0];
    const uint32_t price = parse<uint32_t>(results[3]);
    // volume is not supplied and not needed - set it to 0
    process<Action::Remove, FillsCallback>(oid, dir, 0, price, cb);
  } break;
  default:
    throw ParseError("action mismatch");
  }
}

} // namespace orderbook
} // namespace mvs

#endif // PROCESSOR_H
