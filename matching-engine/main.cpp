#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <thread>
#include <unordered_map>
#include <vector>

enum class Side { Bid, Ask };

typedef uint64_t OrderId;
typedef uint64_t TradeId;

typedef struct {
  OrderId id;
  uint64_t quantity;
} Order;

typedef struct {
  OrderId id;
  uint64_t quantity;
  uint64_t price;
  Side side;
} OrderMetadata;

class OrderBook {
private:
  std::map<uint64_t, std::vector<Order>> bids;
  std::map<uint64_t, std::vector<Order>> asks;
  std::unordered_map<OrderId, OrderMetadata> metadatas;
  OrderId current_order_id;
  TradeId current_trade_id;

  auto next_order_id() { return current_order_id++; }
  auto next_trade_id() { return current_trade_id++; }

  void match() {
    auto best_bid_price = bids.rbegin()->first;
    auto best_bid_orders = bids.rbegin()->second;
    auto best_ask_price = asks.begin()->first;
    auto best_ask_orders = asks.begin()->second;

    if (best_bid_price == best_bid_price) {
      /* for (auto it = ) */
      for (auto bid_order : best_bid_orders)
        for (auto ask_order : best_ask_orders) {
          auto trade_id = next_trade_id();
          auto trade_price = best_bid_price;
          auto trade_quantity = std::min(bid_order.quantity, ask_order.quantity);
          bid_order.quantity -= trade_quantity;
          ask_order.quantity -= trade_quantity;
          emit(); // TODO: Emit trade
        }
    }
  }

  void emit() {}

public:
  OrderBook() {
    bids = std::map<uint64_t, std::vector<Order>>();
    asks = std::map<uint64_t, std::vector<Order>>();
    metadatas = std::unordered_map<OrderId, OrderMetadata>();
    current_order_id = 0;
    current_trade_id = 0;
  }

  void insert(Side side, uint64_t quantity, uint64_t price) {
    auto id = next_order_id();
    auto order = Order{id, quantity};
    metadatas[id] = OrderMetadata{id, quantity, price, side};
    switch (side) {
    case Side::Bid:
      if (!bids.count(price))
        bids[price] = std::vector<Order>{};
      bids[price].push_back(order);
      break;
    case Side::Ask:
      if (!bids.count(price))
        bids[price] = std::vector<Order>{};
      bids[price].push_back(order);
      break;
    }
    emit(); // TODO: Emit order created
  }

  void amend() {}

  void remove(OrderId id) {
    if (!metadatas.count(id)) {
      emit(); // TODO: Emit error
      return;
    }

    auto metadata = metadatas[id];
    auto orders = metadata.side == Side::Ask ? asks[metadata.price]
                                             : bids[metadata.price];
    for (auto it = orders.begin(); it != orders.end(); it++)
      if (it->id == id)
        orders.erase(it);

    switch (metadata.side) {
    case Side::Bid:
      bids[metadata.price] = orders;
      break;
    case Side::Ask:
      asks[metadata.price] = orders;
      break;
    }

    metadatas.erase(id);

    emit(); // TODO: Emit order cancelled
  }
};

int main() {

  // Subscribing to redis channels (for orders)
  while (true) {
    std::cout << "Hi\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  return 0;
}
