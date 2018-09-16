#include <gtest/gtest.h>

#include <cmath>
#include <tuple>

#include "../Actions.h"
#include "../Exceptions.h"
#include "../Order.h"
#include "../OrderBook.h"
#include "../Processor.h"

using namespace mvs::orderbook;

auto dummyCallback = [](const Trade &) {};

TEST(ExceptionsTests, DuplicateOrderIdError) {
  ASSERT_STREQ("duplicate oid 388075", DuplicateOrderIdError(388075).what());
}

TEST(ExceptionsTests, UnknownOrderIdError) {
  ASSERT_STREQ("unknown oid 388075", UnknownOrderIdError(388075).what());
}

TEST(ExceptionsTests, ParseError) {
  ASSERT_STREQ("parse error: 'FOO BAR'", ParseError("FOO BAR").what());
}

TEST(ActionTests, AddAction) {
  using ActionT = OrderAction<Action::Add, Direction::Buy>;
  ActionT action(12, 34, 45.0);
  ASSERT_EQ(12, action.getOid());
  ASSERT_EQ(34, action.getVolume());
  ASSERT_EQ(45.0, action.getPrice());
}

TEST(ActionTests, ModifyAction) {
  using ActionT = OrderAction<Action::Modify, Direction::Buy>;
  ActionT action(12, 34, 45.0);
  ASSERT_EQ(12, action.getOid());
  ASSERT_EQ(34, action.getVolume());
  ASSERT_EQ(45.0, action.getPrice());
}

TEST(ActionTests, RemoveAction) {
  using ActionT = OrderAction<Action::Remove, Direction::Buy>;
  ActionT action(12, 34, 45.0);
  ASSERT_EQ(12, action.getOid());
  ASSERT_EQ(34, action.getVolume());
  ASSERT_EQ(45.0, action.getPrice());
}

TEST(OrderTests, Basic) {
  using ActionT = OrderAction<Action::Add, Direction::Buy>;
  ActionT action(12, 34, 45.0);
  // Order doesn't know it's own price - it's kept by the book and the book
  // knows this.
  Order order(action);
  ASSERT_EQ(12, order.getOid());
  ASSERT_EQ(34, order.getVolume());

  order.reduceVolume(1);
  ASSERT_EQ(33, order.getVolume());
}

struct MockBook {
  template <Action action, Direction direction, typename Callback>
  void handle(OrderAction<action, direction> &oaction, Callback &) {
    store.emplace_back(action, direction, oaction.getOid(), oaction.getVolume(),
                       oaction.getPrice());
  }

  using StoredActionT =
      std::tuple<Action, Direction, uint32_t, uint32_t, double>;
  using StoredActionVctT = std::vector<StoredActionT>;

  StoredActionVctT store;
};

TEST(ProcessorTests, Basic) {
  using BookT = MockBook;
  using ProcessorT = Processor<MockBook>;
  MockBook book;
  ProcessorT processor(book);

  using TestcaseT = std::tuple<std::string, MockBook::StoredActionT>;
  for (TestcaseT const &tc :
       {TestcaseT("A,12345,S,1,75",
                  MockBook::StoredActionT(Action::Add, Direction::Sell, 12345,
                                          1, 75)),
        TestcaseT(
            "A,54321,B,3,77",
            MockBook::StoredActionT(Action::Add, Direction::Buy, 54321, 3, 77)),
        TestcaseT(
            "A,54321,B,3,77//comment",
            MockBook::StoredActionT(Action::Add, Direction::Buy, 54321, 3, 77)),
        TestcaseT(
            "A,54321,B,3,77   //comment",
            MockBook::StoredActionT(Action::Add, Direction::Buy, 54321, 3, 77)),
        TestcaseT("M,54321,B,3,77",
                  MockBook::StoredActionT(Action::Modify, Direction::Buy, 54321,
                                          3, 77)),
        TestcaseT("M,54321,S,5,1077",
                  MockBook::StoredActionT(Action::Modify, Direction::Sell,
                                          54321, 5, 1077)),
        TestcaseT("X,54321,S,1077", MockBook::StoredActionT(
                                        Action::Remove, Direction::Sell, 54321,
                                        0 /* dummy volume */, 1077))}) {
    processor.process(std::get<0>(tc), dummyCallback);
    ASSERT_EQ(std::get<1>(tc), book.store.back());
  }
}

TEST(ProcessorTests, Errors) {
  using BookT = MockBook;
  using ProcessorT = Processor<MockBook>;
  MockBook book;
  ProcessorT processor(book);
  ASSERT_THROW(processor.process("", dummyCallback), ParseError);
  ASSERT_THROW(processor.process("foo", dummyCallback), ParseError);
  ASSERT_THROW(processor.process("Q,12345,S,1,1075", dummyCallback),
               ParseError);
  ASSERT_THROW(processor.process("A,12a45,S,1,1075", dummyCallback),
               ParseError);
  ASSERT_THROW(processor.process("A,-12345,S,1,1075", dummyCallback),
               ParseError);
  ASSERT_THROW(processor.process("A,12345,X,1,1075", dummyCallback),
               ParseError);
  ASSERT_THROW(processor.process("A,12345,S,-1,1075", dummyCallback),
               ParseError);
  ASSERT_THROW(processor.process("A,12345,S,1,a1075", dummyCallback),
               ParseError);
}

TEST(OrderBookTests, Basic) {
  OrderBook book;
  ASSERT_TRUE(std::isnan(book.getMidPrice()));

  using BuyActionT = OrderAction<Action::Add, Direction::Buy>;
  using SellActionT = OrderAction<Action::Add, Direction::Sell>;

  {
    BuyActionT action(12, 34, 45.0);
    book.handle(action, dummyCallback);
  }

  // one-sided book, which we have decided is Nan
  ASSERT_TRUE(std::isnan(book.getMidPrice()));

  {
    SellActionT action(13, 34, 46.0);
    book.handle(action, dummyCallback);
  }

  // two-sided and actual mid price
  ASSERT_EQ(45.5, book.getMidPrice());
}

TEST(OrderBookTests, AddRemove) {
  OrderBook book;

  using AddActionT = OrderAction<Action::Add, Direction::Sell>;
  using RemoveActionT = OrderAction<Action::Remove, Direction::Sell>;

  // add one order
  {
    AddActionT action(12, 34, 45.0);
    book.handle(action, dummyCallback);
  }

  // add another
  {
    AddActionT action(13, 12, 45.0);
    book.handle(action, dummyCallback);
  }

  ASSERT_EQ(1u, book.getSellSide().size()); // 1 price level
                                            // 2 orders at this level
  ASSERT_EQ(2u, book.getSellSide().front().second.size());

  // remove non existing order
  {
    RemoveActionT action(14, 12, 45.0);
    ASSERT_THROW(book.handle(action, dummyCallback), UnknownOrderIdError);
  }

  // nothing changed
  ASSERT_EQ(1u, book.getSellSide().size()); // 1 price level
                                            // 2 orders at this level
  ASSERT_EQ(2u, book.getSellSide().front().second.size());

  // remove existing order
  {
    RemoveActionT action(13, 12, 45.0);
    book.handle(action, dummyCallback);
  }

  // order got removed - 1 order remaining
  ASSERT_EQ(1u, book.getSellSide().size());
  // 1 order at this level
  ASSERT_EQ(1u, book.getSellSide().front().second.size());

  // remove remaining order
  {
    RemoveActionT action(12, 34, 45.0);
    book.handle(action, dummyCallback);
    ASSERT_TRUE(book.getSellSide().empty());
  }
}

TEST(OrderBookTests, Modify) {
  OrderBook book;

  using AddActionT = OrderAction<Action::Add, Direction::Sell>;
  using ModifyAction = OrderAction<Action::Modify, Direction::Sell>;

  // add one order
  {
    AddActionT action(12, 34, 45.0);
    book.handle(action, dummyCallback);
  }
  ASSERT_EQ(1u, book.getSellSide().size());
  ASSERT_EQ(45.0, book.getSellSide().front().first);

  // can't modify it if we can't find it
  {
    using WrongModifyAction = OrderAction<Action::Modify, Direction::Buy>;
    WrongModifyAction action(12, 34, 45.0);
    ASSERT_THROW(book.handle(action, dummyCallback), UnknownOrderIdError);
  }

  // so nothing changed
  ASSERT_EQ(1u, book.getSellSide().size());
  ASSERT_EQ(45.0, book.getSellSide().front().first);
  ASSERT_EQ(34, book.getSellSide().front().second.front().getVolume());

  // this is valid ( same side and we know the oid )
  {
    ModifyAction action(12, 35, 46.0);
    book.handle(action, dummyCallback);
  }

  // and now the price and volume changed
  ASSERT_EQ(1u, book.getSellSide().size());
  ASSERT_EQ(46.0, book.getSellSide().front().first);
  ASSERT_EQ(35, book.getSellSide().front().second.front().getVolume());

  // and this is still wrong - we don't know this oid on the sell side
  {
    ModifyAction action(10, 34, 45.0);
    ASSERT_THROW(book.handle(action, dummyCallback), UnknownOrderIdError);
  }
}

TEST(OrderBookTests, MultipleOrdersSameLevel) {
  OrderBook book;

  using BuyActionT = OrderAction<Action::Add, Direction::Buy>;
  using SellActionT = OrderAction<Action::Add, Direction::Sell>;

  {
    BuyActionT action(12, 34, 45.0);
    book.handle(action, dummyCallback);
  }

  {
    BuyActionT action(13, 12, 45.0);
    book.handle(action, dummyCallback);
  }

  {
    BuyActionT action(14, 12, 43.0);
    book.handle(action, dummyCallback);
  }

  ASSERT_EQ(2u, book.getBuySide().size());  // 2 different price levels
  ASSERT_EQ(0u, book.getSellSide().size()); // 2 different price levels
  {
    const auto &firstBuyLevel(book.getBuySide().front().second);
    ASSERT_EQ(45.0, book.getBuySide().front().first);
    ASSERT_EQ(2u, firstBuyLevel.size());
    ASSERT_EQ(34, firstBuyLevel.front().getVolume());
    ASSERT_EQ(12, firstBuyLevel.back().getVolume());
  }
}

TEST(OrderBookTests, ExpectedCross) {
  OrderBook book;
  ASSERT_TRUE(std::isnan(book.getMidPrice()));

  using BuyActionT = OrderAction<Action::Add, Direction::Buy>;
  using SellActionT = OrderAction<Action::Add, Direction::Sell>;

  {
    BuyActionT action(12, 34, 45.0);
    book.handle(action, dummyCallback);
  }

  // one-sided book, which we have decided is Nan
  ASSERT_TRUE(std::isnan(book.getMidPrice()));

  // sell for less than the top bid - this should cross
  {
    SellActionT action(13, 34, 44.0);
    book.handle(action, dummyCallback);
  }
}

TEST(OrderBookTests, CrossedBook) {
  OrderBook book;
  using BuyActionT = OrderAction<Action::Add, Direction::Buy>;
  using SellActionT = OrderAction<Action::Add, Direction::Sell>;

  std::vector<Trade> trades;
  auto onTrade = [&trades](const Trade &trade) {
    trades.emplace_back(trade.getBuyOid(), trade.getSellOid(),
                        trade.getVolume(), trade.getPrice());
  };

  {
    BuyActionT action(11, 10, 45.0);
    book.handle(action, onTrade);
  }
  ASSERT_EQ(0u, trades.size());

  {
    BuyActionT action(12, 3, 46.0);
    book.handle(action, onTrade);
  }
  ASSERT_EQ(2u, book.getBuySide().size()); // 2 different price levels
  ASSERT_EQ(0u, trades.size());

  {
    SellActionT action(13, 10, 47.0);
    book.handle(action, onTrade);
  }
  ASSERT_EQ(1u, book.getSellSide().size()); // 1 price level
  ASSERT_EQ(0u, trades.size());

  {
    const auto &firstBuyLevel(book.getBuySide().front().second);
    ASSERT_EQ(46.0, book.getBuySide().front().first);
    ASSERT_EQ(3, firstBuyLevel.front().getVolume());
  }
  {
    SellActionT action(14, 10, 44.0);
    book.handle(action, onTrade);
  }
  // we traded!
  ASSERT_EQ(2u, trades.size());

  // most aggressive price ( 46.0 ) traded first, for whole volume ( 3 )
  ASSERT_EQ(12, trades.front().getBuyOid());
  ASSERT_EQ(14, trades.front().getSellOid());
  ASSERT_EQ(3, trades.front().getVolume());

  // we can't delete the order to buy at 46.0 any more ( oid 12 ) because it
  // doesn't exist
  {
    using ActionT = OrderAction<Action::Remove, Direction::Buy>;
    ActionT action(12, 3, 46.0);
    ASSERT_THROW(book.handle(action, dummyCallback), UnknownOrderIdError);
  }

  // next price ( 45.0 ) traded next, for 7 volume because that's all that's
  // left of the sell order
  ASSERT_EQ(11, trades.back().getBuyOid());
  ASSERT_EQ(14, trades.back().getSellOid());
  ASSERT_EQ(7, trades.back().getVolume());

  ASSERT_EQ(1u, book.getSellSide().size()); // 1 price level left
  ASSERT_EQ(1u, book.getBuySide().size());  // 1 level taken out completely
  {
    const auto &firstBuyLevel(book.getBuySide().front().second);
    ASSERT_EQ(45.0, book.getBuySide().front().first);
    ASSERT_EQ(3, firstBuyLevel.front().getVolume());
  }

  // take out exactly what's left on the offer
  {
    BuyActionT action(15, 10, 47.0);
    book.handle(action, onTrade);
  }
  // we traded!
  ASSERT_EQ(3u, trades.size());

  ASSERT_EQ(15, trades.back().getBuyOid());
  ASSERT_EQ(13, trades.back().getSellOid());
  ASSERT_EQ(10, trades.back().getVolume());
  ASSERT_EQ(0u, book.getSellSide().size()); // cleaned up completely

  ASSERT_EQ(1u, book.getBuySide().size());
  {
    using ActionT = OrderAction<Action::Remove, Direction::Buy>;
    ActionT action(11, 3, 45.0);
    book.handle(action, dummyCallback);
  }
  ASSERT_EQ(0u, book.getBuySide().size());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
