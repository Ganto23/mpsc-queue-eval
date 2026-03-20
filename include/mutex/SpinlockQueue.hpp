#pragma once
#include <atomic>
#include <vector>
#include <cstddef>

class Spinlock {
public:
    void lock() noexcept {
        while (flag.test_and_set(std::memory_order_acquire)) {asm volatile("yield" ::: "memory");}//flag.wait(true, std::memory_order_relaxed);
    }
    void unlock() noexcept {
        flag.clear(std::memory_order_release);
        //flag.notify_one();
    }
private:
    std::atomic_flag flag{};
};

template <typename T, std::size_t Capacity = 8192>
class SpinlockQueue {
    static_assert(std::has_single_bit(Capacity), "Capacity must be a power of two for bitwise wrapping");
public:
    SpinlockQueue() : _queue(Capacity), _capacity(Capacity), _head(0), _tail(0){}

    bool try_enqueue(const T& item) {
        std::lock_guard<Spinlock> lock(_mtx);
        if ((_tail - _head) == _capacity) {
            return false;
        }
        _queue[_tail & (_capacity - 1)] = item;
        _tail++;
        return true;
    }
    bool try_enqueue(T&& item) {
        std::lock_guard<Spinlock> lock(_mtx);
        if ((_tail - _head) == _capacity) {
            return false;
        }
        _queue[_tail & (_capacity - 1)] = std::move(item);
        _tail++;
        return true;
    }
    bool try_dequeue(T& item) {
        std::lock_guard<Spinlock> lock(_mtx);
        if (_head == _tail) {
            return false;
        }
        item = std::move(_queue[_head & (_capacity - 1)]);
        _head++;
        return true;
    }

    std::size_t capacity() const {
        return _capacity;
    }
    bool is_empty() const {
        std::lock_guard<Spinlock> lock(_mtx);
        return _head == _tail;
    }
    bool is_full() const {
        std::lock_guard<Spinlock> lock(_mtx);
        return (_tail - _head) == _capacity;
    }

private:
    mutable Spinlock _mtx;
    std::vector<T> _queue;
    const std::size_t _capacity;
    std::size_t _head;
    std::size_t _tail;

};