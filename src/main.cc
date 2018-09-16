#include <assert.h>
#include <cstring>
#include <fstream>
#include <iostream>

#include "Actions.h"
#include "Enums.h"
#include "Exceptions.h"
#include "Order.h"
#include "OrderBook.h"
#include "Processor.h"

int main(int argc, char **argv) {
  std::string line;
  std::ifstream ifs(argv[1], std::ifstream::in);
  const bool silent(argc == 3 && strncmp("silent", argv[2], 6) == 0);

  using BookT = mvs::orderbook::OrderBook;
  using ProcessorT = mvs::orderbook::Processor<mvs::orderbook::OrderBook>;

  auto cb = [silent](const mvs::orderbook::Trade &trade) {
    if (!silent) {
      std::cout << "Trade " << trade << std::endl;
    }
  };

  BookT book;
  ProcessorT processor(book);

  uint32_t numLines(0);
  uint32_t duplicateOrderIdErrors(0);
  uint32_t unknownOrderIdErrors(0);
  uint32_t parseErrors(0);

  while (std::getline(ifs, line)) {
    numLines++;
    if (!silent) {
      std::cout << line << std::endl;
    }

    try {
      processor.process(line, cb);
    } catch (const mvs::orderbook::DuplicateOrderIdError &e) {
      std::cerr << e.what() << std::endl;
      duplicateOrderIdErrors++;
    } catch (const mvs::orderbook::UnknownOrderIdError &e) {
      std::cerr << e.what() << std::endl;
      unknownOrderIdErrors++;
    } catch (const mvs::orderbook::ParseError &e) {
      std::cerr << e.what() << std::endl;
      parseErrors++;
    }
    if (!silent) {
      std::cout << book << std::endl;
    }
  }

  std::cout << numLines << " lines" << std::endl;
  std::cout << duplicateOrderIdErrors << " duplicate order ids" << std::endl;
  std::cout << unknownOrderIdErrors << " unknown order ids" << std::endl;
  std::cout << parseErrors << " parse errors" << std::endl;
}
