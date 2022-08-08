// This file contains declarations for the main Engine class. You will
// need to add declarations to this file as you develop your Engine.

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <assert.h>

#include <chrono>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "io.h"

class Engine {
  void ConnectionThread(ClientConnection);

 public:
  void Accept(ClientConnection);
};

inline static std::chrono::microseconds::rep CurrentTimestamp() noexcept {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

typedef input __PendingOrder;

struct PendingOrder {
  // order information
  __PendingOrder detail;
  // to match the earlist arrival time
  int64_t input_time;
  // to mark whether the order has been partially matched or not
  bool isMatched;

  PendingOrder(__PendingOrder _detail, int64_t _input_time)
      : detail{_detail},
        input_time{_input_time},
        isMatched{false} {}
  PendingOrder(const PendingOrder &b)
      : detail{b.detail},
        input_time{b.input_time},
        isMatched{b.isMatched} {}
  PendingOrder &operator=(const PendingOrder &b) {
    detail = b.detail;
    input_time = b.input_time;
    isMatched = b.isMatched;
    return *this;
  }
  // priority(b) > priority(*this) <=> b.input_time < this->input_time
  bool operator<(const PendingOrder &b) const {
    return this->input_time > b.input_time;
  }
};

// concurrent version of heap offering limited methods
struct OrdersPerInstrument {
  // store all the buyers' orders
  std::vector<PendingOrder> buyerOrders;
  // protect data
  std::mutex buyerMutex;
  // store all the sellers' orders
  std::vector<PendingOrder> sellerOrders;
  // protect data
  std::mutex sellerMutex;

  OrdersPerInstrument()
      : buyerOrders{}, buyerMutex{}, sellerOrders{}, sellerMutex{} {}
  void executeAndAddOrder(PendingOrder toExecute) {
    input_type orderType = toExecute.detail.type;
    // shouldn't be called on an order to cancel
    assert(orderType != input_cancel);
    switch (orderType) {
      case input_buy:
        executeAndAddBuyerOrder(toExecute);
        break;
      case input_sell:
        executeAndAddSellerOrder(toExecute);
        break;
      default:
        assert(orderType == input_cancel);
    }
  }

  void executeAndAddBuyerOrder(PendingOrder toBuy) {
    input_type orderType = toBuy.detail.type;
    assert(orderType != input_cancel);
    assert(orderType == input_buy);
    uint64_t executionID = 0;

    std::priority_queue<std::pair<PendingOrder, size_t>> heap;
    std::vector<size_t> deletedOrderInReverseOrder;

    {
      std::unique_lock<std::mutex> lk{sellerMutex};

      for (size_t i = 0; i < sellerOrders.size(); i++) {
        if (toBuy.detail.price >= sellerOrders[i].detail.price)
          heap.emplace(sellerOrders[i], i);
      }

      while (!heap.empty() && toBuy.detail.count > 0) {
        auto top = heap.top();
        heap.pop();
        // copy of info in sellOrders
        size_t earlistOrderId = top.second;
        // increase executationID before matching
        executionID++;
        // ealistOrder fully matched toBuy
        if (sellerOrders[earlistOrderId].detail.count >= toBuy.detail.count) {
          Output::OrderExecuted(
            sellerOrders[earlistOrderId].detail.order_id,
            toBuy.detail.order_id, executionID,
            toBuy.detail.instrument,
            sellerOrders[earlistOrderId].detail.price, toBuy.detail.count,
            toBuy.input_time, CurrentTimestamp());
          // modify the order in sellOrders
          sellerOrders[earlistOrderId].detail.count -= toBuy.detail.count;
          toBuy.detail.count = 0;
          sellerOrders[earlistOrderId].isMatched = true;
          if(sellerOrders[earlistOrderId].detail.count == 0) {
            deletedOrderInReverseOrder.push_back(earlistOrderId);
          }
          break;
        } else {
          // toExecute fully matched earlistOrder
          Output::OrderExecuted(sellerOrders[earlistOrderId].detail.order_id,
                              toBuy.detail.order_id,
                              executionID,
                              toBuy.detail.instrument,
                              sellerOrders[earlistOrderId].detail.price,
                              sellerOrders[earlistOrderId].detail.count,
                              toBuy.input_time, CurrentTimestamp());
          toBuy.detail.count -= sellerOrders[earlistOrderId].detail.count;
          toBuy.isMatched = true;
          // erase earlistOrder in sellerOrders
          deletedOrderInReverseOrder.push_back(earlistOrderId);
        }
      }
      std::sort(deletedOrderInReverseOrder.begin(), deletedOrderInReverseOrder.end(), std::greater<size_t>());
      for (auto i : deletedOrderInReverseOrder) {
        sellerOrders.erase(sellerOrders.begin() + i);
      }
    }
    // unlock sellerOrders
    if (toBuy.detail.count > 0) {
      // add to buyerOrders
      addBuyerOrder(toBuy);
    }
  }

  void executeAndAddSellerOrder(PendingOrder toSell) {
    input_type orderType = toSell.detail.type;
    assert(orderType != input_cancel);
    assert(orderType == input_sell);
    uint64_t executionID = 0;

    std::priority_queue<std::pair<PendingOrder, size_t>> heap;
    std::vector<size_t> deletedOrderInReverseOrder;

    {
      std::unique_lock<std::mutex> lk{buyerMutex};

      for (size_t i = 0; i < buyerOrders.size(); i++) {
        if (toSell.detail.price <= buyerOrders[i].detail.price)
          heap.emplace(buyerOrders[i], i);
      }

      while (!heap.empty() && toSell.detail.count > 0) {
        auto top = heap.top();
        heap.pop();
        // copy of info in buyerOrders
        size_t earlistOrderId = top.second;
        // increase executationID before matching
        executionID++;
        // ealistOrder fully matched toSell
        if (buyerOrders[earlistOrderId].detail.count >= toSell.detail.count) {
          Output::OrderExecuted(
              buyerOrders[earlistOrderId].detail.order_id,
              toSell.detail.order_id, executionID,
              toSell.detail.instrument,
              buyerOrders[earlistOrderId].detail.price, toSell.detail.count,
              toSell.input_time, CurrentTimestamp());
          // modify the order in sellOrders
          buyerOrders[earlistOrderId].detail.count -= toSell.detail.count;
          toSell.detail.count = 0;
          buyerOrders[earlistOrderId].isMatched = true;
          if(buyerOrders[earlistOrderId].detail.count == 0) {
            deletedOrderInReverseOrder.push_back(earlistOrderId);
          }
          break;
        } else {
          // toExecute fully matched earlistOrder
          Output::OrderExecuted(buyerOrders[earlistOrderId].detail.order_id,
                                toSell.detail.order_id,
                                executionID,
                                toSell.detail.instrument,
                                buyerOrders[earlistOrderId].detail.price,
                                buyerOrders[earlistOrderId].detail.count,
                                toSell.input_time, CurrentTimestamp());
          toSell.detail.count -= buyerOrders[earlistOrderId].detail.count;
          toSell.isMatched = true;
          // erase earlistOrder in buyerOrders
          deletedOrderInReverseOrder.push_back(earlistOrderId);
        }
      }
      std::sort(deletedOrderInReverseOrder.begin(), deletedOrderInReverseOrder.end(), std::greater<size_t>());
      for (auto i : deletedOrderInReverseOrder) {
        buyerOrders.erase(buyerOrders.begin() + i);
      }
    }
    // unlock buyerOrders
    if (toSell.detail.count > 0) {
      // add to sellerOrders
      addSellerOrder(toSell);
    }
  }
  bool cancelOrder(PendingOrder toCancel, input_type cancelType) {
    assert(toCancel.detail.type == input_cancel);

    std::vector<PendingOrder> &orders =
        cancelType == input_buy ? buyerOrders : sellerOrders;
    std::mutex &mutex = cancelType == input_buy ? buyerMutex : sellerMutex;

    std::unique_lock<std::mutex> lk{mutex};
    for (size_t i = 0; i < orders.size(); i++) {
      if (orders[i].detail.order_id == toCancel.detail.order_id) {
        if (orders[i].isMatched) {
          return false;
        } else {
          // erase the order
          std::swap(orders[i], orders[orders.size() - 1]);
          orders.pop_back();
          return true;
        }
      }
    }
    return false;
  }

 private:
  void addBuyerOrder(PendingOrder toBuy) {
    {
      std::unique_lock<std::mutex> lk{buyerMutex};
      buyerOrders.push_back(toBuy);
      Output::OrderAdded(toBuy.detail.order_id, toBuy.detail.instrument,
                      toBuy.detail.price, toBuy.detail.count, false,
                      toBuy.input_time, CurrentTimestamp());
    }
  }

  void addSellerOrder(PendingOrder toSell) {
    {
      std::unique_lock<std::mutex> lk{sellerMutex};
      sellerOrders.push_back(toSell);
      Output::OrderAdded(toSell.detail.order_id, toSell.detail.instrument,
                      toSell.detail.price, toSell.detail.count, true,
                      toSell.input_time, CurrentTimestamp());
    }
  }
};

#endif
