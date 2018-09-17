#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>

#include "Actions.h"
#include "Common.h"
#include "Exceptions.h"
#include "OrderBook.h"

namespace mvs {
namespace orderbook {

namespace details {

// C-style tokenizing using strtok is just faster than out of the box C++
// solutions. So, we do that here .. but still wrap it up in C++ exceptions, and
// make it look pretty by using templates.

template <typename T>
typename std::enable_if<
    std::is_unsigned<T>::value && std::is_integral<T>::value, T>::type
parse(const char *input) {
  T output(0);
  while (input) {
    const char c = *(input++);
    if (c == ' ' || c == '\r' || c == '\0') {
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

template <typename T>
typename std::enable_if<
    std::is_unsigned<T>::value && std::is_integral<T>::value, T>::type
tokenize() {
  char *token = strtok(nullptr, ",/");
  if (nullptr == token) {
    throw ParseError();
  }
  return parse<T>(token);
}

template <typename T>
typename std::enable_if<std::is_enum<T>::value, T>::type
tokenize(char *ptr = nullptr) {
  char *token = strtok(ptr, ",/");
  if (nullptr == token) {
    throw ParseError();
  }
  return static_cast<T>(*token);
}

} // namespace tokenizer

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
void Processor<BookT>::process(const std::string &line, FillsCallback &cb) {

  // we don't want to remember to free whatever we strdup - so have unique_ptr
  // do this for us
  std::unique_ptr<char, decltype(std::free) *> input{strdup(line.c_str()),
                                                     std::free};

  if (!input) {
    throw ParseError(line);
  }

  try {
    const Action action = details::tokenize<Action>(input.get());
    switch (action) {
    case Action::Add:
    case Action::Modify: {
      const uint32_t oid = details::tokenize<uint32_t>();
      const Direction dir = details::tokenize<Direction>();
      const uint32_t volume = details::tokenize<uint32_t>();
      const uint32_t price = details::tokenize<uint32_t>();

      if (Action::Add == action) {
        process<Action::Add, FillsCallback>(oid, dir, volume, price, cb);
      } else if (Action::Modify == action) {
        process<Action::Modify, FillsCallback>(oid, dir, volume, price, cb);
      }
    } break;

    case Action::Remove: {
      const uint32_t oid = details::tokenize<uint32_t>();
      const Direction dir = details::tokenize<Direction>();
      const uint32_t price = details::tokenize<uint32_t>();

      process<Action::Remove, FillsCallback>(oid, dir, 0, price, cb);
    } break;

    default:
      throw ParseError(line);
    };
  } catch (ParseError e) {
    throw ParseError(0 == strlen(e.what()) ? line : e.what());
  }
}

} // namespace orderbook
} // namespace mvs

#endif // PROCESSOR_H
