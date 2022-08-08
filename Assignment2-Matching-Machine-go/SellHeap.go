package main

import (
	"container/heap"
)

type SellHeap []Order

func (h *SellHeap) Init() {
	heap.Init(h)
}

func (h SellHeap) Len() int { return len(h) }

func (h SellHeap) Less(i, j int) bool {
	if h[i].detail.price == h[j].detail.price {
		return h[i].completionTime < h[j].completionTime
	}
	return h[i].detail.price < h[j].detail.price
}

func (h SellHeap) Swap(i, j int) { h[i], h[j] = h[j], h[i] }

func (h *SellHeap) Push(x interface{}) {
	n := len(*h)
	order := x.(Order)
	order.index = n
	*h = append(*h, order)
}

func (h *SellHeap) Pop() interface{} {
	old := *h
	n := len(old)
	order := old[n-1]
	order.index = -1 // for safety
	*h = old[0 : n-1]
	return order
}

func (h *SellHeap) Top() Order {
	return (*h)[0]
}

func (h *SellHeap) Delete(i int) {
	(*h)[i].detail.count = 0
	(*h)[i].detail.price = 0
	(*h)[i].completionTime = 0
	heap.Fix(h, i)
	heap.Pop(h)
}

func (h *SellHeap) cancelOrder(orderId uint32) bool {
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
