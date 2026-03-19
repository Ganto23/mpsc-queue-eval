#include <cstddef>
#include <atomic>
#include <memory>
#include <bit>

template <typename T, size_t Capacity = 8192>
class VyukovStateMachine {
    static_assert(std::has_single_bit(Capacity), "Capacity must be a power of two for bitwise wrapping");

    struct Node {
        std::atomic<std::size_t> seq_num;
        T data;
    };

public:
    VyukovStateMachine() : _queue(std::make_unique<Node[]>(Capacity)), _capacity(Capacity), _head(0), _tail(0) {
        for (std::size_t i = 0; i < Capacity; i++) {
            _queue[i].seq_num.store(i, std::memory_order_relaxed);
        }
    }

    bool try_enqueue(const T& item){
        while (true) {
            std::size_t current_tail = _tail.load(std::memory_order_relaxed);
            std::size_t index = current_tail & (_capacity - 1);
            std::size_t seq_num = _queue[index].seq_num.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq_num - (intptr_t)current_tail;

            if (diff == 0){
                if (_tail.compare_exchange_weak(current_tail, current_tail + 1, std::memory_order_relaxed)) {
                    _queue[index].data = item;
                    _queue[index].seq_num.store(current_tail + 1, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                return false;
            }
        }
    }

    bool try_enqueue(T&& item){
        while (true) {
            std::size_t current_tail = _tail.load(std::memory_order_relaxed);
            std::size_t index = current_tail & (_capacity - 1);
            std::size_t seq_num = _queue[index].seq_num.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq_num - (intptr_t)current_tail;

            if (diff == 0){
                if (_tail.compare_exchange_weak(current_tail, current_tail + 1, std::memory_order_relaxed)) {
                    _queue[index].data = std::move(item);
                    _queue[index].seq_num.store(current_tail + 1, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                return false;
            }
        }
    }

    bool try_dequeue(T& item){
        std::size_t current_head = _head.load(std::memory_order_relaxed);
        std::size_t index = current_head & (_capacity - 1);
        std::size_t seq_num = _queue[index].seq_num.load(std::memory_order_acquire);
        intptr_t diff = (intptr_t)seq_num - (intptr_t)(current_head + 1);
        if (diff == 0){
            item = std::move(_queue[index].data);
            _queue[index].seq_num.store(current_head + _capacity, std::memory_order_release);
            _head.store(current_head + 1, std::memory_order_relaxed);
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