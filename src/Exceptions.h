#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <boost/format.hpp>

#include <stdexcept>

namespace mvs {
namespace orderbook {

struct DuplicateOrderIdError : std::runtime_error {
  DuplicateOrderIdError(uint32_t oid)
      : std::runtime_error((boost::format("duplicate oid %1%") % oid).str()){};
};

struct UnknownOrderIdError : std::runtime_error {
  UnknownOrderIdError(uint32_t oid)
      : std::runtime_error((boost::format("unknown oid %1%") % oid).str()){};
};

struct ParseError : std::runtime_error {
  ParseError() : std::runtime_error("") {}

  ParseError(const std::string &line)
      : std::runtime_error((boost::format("parse error: '%1%'") % line).str()) {
  }
};

} // namespace orderbook
} // namespace mvs

#endif // EXCEPTIONS_H
