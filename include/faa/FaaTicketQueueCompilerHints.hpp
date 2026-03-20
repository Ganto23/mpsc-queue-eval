#pragma once
#include <cstddef>
#include <atomic>
#include <memory>
#include <bit>

template <typename T, size_t Capacity = 8192>
class FaaTicketQueueCompilerHints {
    static_assert(std::has_single_bit(Capacity), "Capacity must be a power of two for bitwise wrapping");

    struct Node {
        std::atomic<std::size_t> seq_num;
        T data;
    };

public:
    FaaTicketQueueCompilerHints() : _queue(std::make_unique<Node[]>(Capacity)), _capacity(Capacity), _head(0), _tail(0) {
        for (std::size_t i = 0; i < Capacity; i++) {
            _queue[i].seq_num.store(i, std::memory_order_relaxed);
        }
    }

    bool try_enqueue(const T& item){
        std::size_t ticket = _tail.fetch_add(1, std::memory_order_relaxed);
        std::size_t index = ticket & (_capacity - 1);
        
        while (true) {
            std::size_t seq_num = _queue[index].seq_num.load(std::memory_order_acquire);
            if (seq_num == ticket) [[likely]] {
                _queue[index].data = item;
                _queue[index].seq_num.store(ticket + 1, std::memory_order_release);
                return true;
            }
            __asm__ __volatile__("yield");
        }
    }

    bool try_enqueue(T&& item){
        std::size_t ticket = _tail.fetch_add(1, std::memory_order_relaxed);
        std::size_t index = ticket & (_capacity - 1);
        
        while (true) {
            std::size_t seq_num = _queue[index].seq_num.load(std::memory_order_acquire);
            if (seq_num == ticket) [[likely]] {
                _queue[index].data = std::move(item);
                _queue[index].seq_num.store(ticket + 1, std::memory_order_release);
                return true;
            }
            __asm__ __volatile__("yield");
        }
    }

    bool try_dequeue(T& item){
        std::size_t ticket = _head.load(std::memory_order_relaxed);
        std::size_t index = ticket & (_capacity - 1);

        std::size_t seq_num = _queue[index].seq_num.load(std::memory_order_acquire);
        if (seq_num == ticket + 1) [[likely]] {
            item = std::move(_queue[index].data);
            _head.store(ticket + 1, std::memory_order_relaxed);
            _queue[index].seq_num.store(ticket + _capacity, std::memory_order_release);
            return true;
        } else {
            return false;
        }
    }

    size_t capacity() const {
        return _capacity;
    }
    bool is_empty() const {
        return _head.load(std::memory_order_relaxed) == _tail.load(std::memory_order_acquire);
    }
    bool is_full() const {
        return (_tail.load(std::memory_order_relaxed) - _head.load(std::memory_order_acquire)) == _capacity;
    }

private:
    std::unique_ptr<Node[]> _queue;
    const std::size_t _capacity;
    std::atomic<std::size_t> _head;
    std::atomic<std::size_t> _tail;
};