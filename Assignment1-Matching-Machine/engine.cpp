#include "engine.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>

#include "io.h"

struct {
  std::unordered_map<std::string, OrdersPerInstrument> ordersOf;
  std::unordered_map<uint32_t, std::pair<std::string, input_type>> instrumentOf;

  std::mutex mutex;

  void executeAndAddOrder(PendingOrder toExecute) {
    uint32_t orderId = toExecute.detail.order_id;
    OrdersPerInstrument* matchedInstrument;

    // get the reference of the orders of corresponding instrument
    // if it's the new instrument, assign bucket
    // only one thread doing this (modify ordersOf and instruementOf) at one
    // moment
    {
      std::unique_lock<std::mutex> lk{mutex};
      if (instrumentOf.count(orderId) > 0) {
        std::cerr << "order ID duplicated" << toExecute.detail.order_id << std::endl;
        assert(false);
        return;
      }
      instrumentOf[toExecute.detail.order_id] =
          std::make_pair(toExecute.detail.instrument, toExecute.detail.type);
      matchedInstrument = &ordersOf[toExecute.detail.instrument];
    }
    matchedInstrument->executeAndAddOrder(toExecute);
  }

  void cancelOrder(PendingOrder toCancel) {
    uint32_t orderId = toCancel.detail.order_id;
    input_type cancelType;
    OrdersPerInstrument* matchedInstrument;
    {
      std::unique_lock<std::mutex> lk{mutex};
      if (instrumentOf.count(orderId) <= 0) {
        Output::OrderDeleted(toCancel.detail.order_id, false,
                             toCancel.input_time, CurrentTimestamp());
        return;
      }
      std::pair<std::string, input_type> tmp =
          instrumentOf[toCancel.detail.order_id];
      std::string instrument = tmp.first;
      cancelType = tmp.second;
      matchedInstrument = &ordersOf[instrument];
    }
    bool isCancelled = matchedInstrument->cancelOrder(toCancel, cancelType);
    {
      Output::OrderDeleted(toCancel.detail.order_id, isCancelled,
                            toCancel.input_time, CurrentTimestamp());
    }
  }
} pendingOrders;

void Engine::Accept(ClientConnection connection) {
  std::thread thread{&Engine::ConnectionThread, this, std::move(connection)};
  thread.detach();
}
void Engine::ConnectionThread(ClientConnection connection) {
  while (true) {
    input input;
    switch (connection.ReadInput(input)) {
      case ReadResult::Error:
        std::cerr << "Error reading input" << std::endl;
      case ReadResult::EndOfFile:
        return;
      case ReadResult::Success:
        break;
    }
    int64_t input_time = CurrentTimestamp();
    // Functions for printing output actions in the prescribed format are
    // provided in the Output class:
    switch (input.type) {
      case input_cancel:
        pendingOrders.cancelOrder({input, input_time});
        break;
      default:
        pendingOrders.executeAndAddOrder({input, input_time});
        break;
    }
  }
}
