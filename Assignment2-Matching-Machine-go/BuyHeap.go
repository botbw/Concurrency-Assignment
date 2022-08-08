package main

import (
	"container/heap"
	"math"
)

type BuyHeap []Order

func (h *BuyHeap) Init() {
	heap.Init(h)
}

func (h BuyHeap) Len() int { return len(h) }

func (h BuyHeap) Less(i, j int) bool {
	if h[i].detail.price == h[j].detail.price {
		return h[i].completionTime < h[j].completionTime
	}
	return h[i].detail.price > h[j].detail.price
}

func (h BuyHeap) Swap(i, j int) { h[i], h[j] = h[j], h[i] }

func (h *BuyHeap) Push(x interface{}) {
	n := len(*h)
	order := x.(Order)
	order.index = n
	*h = append(*h, order)
}

func (h *BuyHeap) Top() Order {
	return (*h)[0]
}

func (h *BuyHeap) Pop() interface{} {
	old := *h
	n := len(old)
	order := old[n-1]
	order.index = -1 // for safety
	*h = old[0 : n-1]
	return order
}

func (h *BuyHeap) Delete(i int) {
	(*h)[i].detail.count = 0
	(*h)[i].detail.price = math.MaxUint32
	(*h)[i].completionTime = 0
	heap.Fix(h, i)
	heap.Pop(h)
}

func (h *BuyHeap) cancelOrder(orderId uint32) bool {
	for i, pending := range *h {
		if pending.detail.orderId == orderId {
			if pending.executed == true {
				return false
			}
			h.Delete(i)
			return true
		}
	}
	return false
}
