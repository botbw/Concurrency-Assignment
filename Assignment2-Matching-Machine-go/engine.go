package main

import "C"
import (
	"container/heap"
	"context"
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"time"
)

func min(a, b uint32) uint32 {
	if a < b {
		return a
	}
	return b
}

const CHAN_CAP = 1000

type Engine struct {
	orderBooks map[string]*OrderBook
	instrument map[uint32]string
	orderChan  chan Order
}

func (e *Engine) distributeOrderDaemon() {
	for {
		order := <-e.orderChan
		if order.detail.orderType == inputCancel {
			order.detail.instrument = e.instrument[order.detail.orderId]
		} else {
			e.instrument[order.detail.orderId] = order.detail.instrument
		}
		orderBook, ok := e.orderBooks[order.detail.instrument]
		if !ok {
			orderBook = newOrderBook()
			e.orderBooks[order.detail.instrument] = orderBook
		}
		e.orderBooks[order.detail.instrument].orderChan <- order
	}
}

func newEngine() *Engine {
	e := &Engine{make(map[string]*OrderBook), make(map[uint32]string), make(chan Order, CHAN_CAP)}
	go e.distributeOrderDaemon()
	return e
}

type Order struct {
	detail         input
	completionTime int64 // input timestamp
	index          int	// for heap to use
	executed       bool	// to check the order can be cancelled or not
}

type OrderBook struct {
	buy       BuyHeap
	sell      SellHeap
	typeOf    map[uint32]inputType
	orderChan chan Order
}

func newOrderBook() *OrderBook {
	o := &OrderBook{make(BuyHeap, 0), make(SellHeap, 0), make(map[uint32]inputType), make(chan Order, CHAN_CAP)}
	go o.handleOrderDaemon()
	return o
}

func (o *OrderBook) handleOrderDaemon() {
	for {
		order := <-o.orderChan
		switch order.detail.orderType {
		case inputBuy:
			o.typeOf[order.detail.orderId] = order.detail.orderType
			o.handleBuyOrder(order)
		case inputSell:
			o.typeOf[order.detail.orderId] = order.detail.orderType
			o.handleSellOrder(order)
		case inputCancel:
			orderType, ok := o.typeOf[order.detail.orderId]
			if !ok {
				fmt.Print("ID not exist")
				outputOrderDeleted(order.detail, false, order.completionTime, GetCurrentTimestamp())
				continue
			}
			o.handleCancelOrder(order, orderType)
		}
	}
}

func (o *OrderBook) handleBuyOrder(order Order) {
	var executionID uint32 = 0
	for order.detail.count > 0 && o.sell.Len() > 0 {
		if order.detail.price < o.sell[0].detail.price {
			break
		}
		matchCount := min(order.detail.count, o.sell[0].detail.count)
		executionID++
		order.executed = true
		o.sell[0].executed = true
		order.detail.count -= matchCount
		o.sell[0].detail.count -= matchCount
		outputOrderExecuted(
			o.sell[0].detail.orderId,
			order.detail.orderId,
			executionID,
			o.sell[0].detail.price,
			matchCount,
			order.completionTime,
			GetCurrentTimestamp(),
		)
		if o.sell[0].detail.count == 0 {
			heap.Pop(&o.sell)
		}
	}
	if order.detail.count != 0 {
		heap.Push(&o.buy, order)
		outputOrderAdded(order.detail, order.completionTime, GetCurrentTimestamp())
	}
}

func (o *OrderBook) handleSellOrder(order Order) {
	var executionID uint32 = 0
	for order.detail.count > 0 && o.buy.Len() > 0 {
		if order.detail.price > o.buy[0].detail.price {
			break
		}
		matchCount := min(order.detail.count, o.buy[0].detail.count)
		order.executed = true
		o.buy[0].executed = true
		executionID++
		o.buy[0].detail.count -= matchCount
		order.detail.count -= matchCount
		outputOrderExecuted(
			o.buy[0].detail.orderId,
			order.detail.orderId,
			executionID,
			o.buy[0].detail.price,
			matchCount,
			order.completionTime,
			GetCurrentTimestamp(),
		)
		if o.buy[0].detail.count == 0 {
			heap.Pop(&o.buy)
		}
	}
	if order.detail.count != 0 {
		heap.Push(&o.sell, order)
		outputOrderAdded(order.detail, order.completionTime, GetCurrentTimestamp())
	}
}

func (o *OrderBook) handleCancelOrder(order Order, orderType inputType) error {
	var isAccepted bool
	switch orderType {
	case inputBuy:
		isAccepted = o.buy.cancelOrder(order.detail.orderId)
	case inputSell:
		isAccepted = o.sell.cancelOrder(order.detail.orderId)
	default:
		return errors.New("case not matched")
	}
	outputOrderDeleted(order.detail, isAccepted, order.completionTime, GetCurrentTimestamp())
	return nil
}

func (e *Engine) accept(ctx context.Context, conn net.Conn) {
	go func() {
		<-ctx.Done()
		conn.Close()
	}()
	go handleConn(conn, e)
}

func handleConn(conn net.Conn, e *Engine) {
	defer conn.Close()
	for {
		in, err := readInput(conn)
		if err != nil {
			if err != io.EOF {
				_, _ = fmt.Fprintf(os.Stderr, "Error reading input: %v\n", err)
			}
			return
		}
		timeNow := GetCurrentTimestamp()
		e.orderChan <- Order{in, timeNow, -1, false}
	}
}

func GetCurrentTimestamp() int64 {
	return time.Now().UnixMicro()
}
