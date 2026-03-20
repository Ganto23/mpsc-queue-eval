#pragma once
#include <mutex>
#include <vector>
#include <cstddef>
#include <atomic>
#include <memory>
#include <bit>

template <typename T, std::size_t Capacity = 8192>
class BasicAtomicQueueCompilerHints {
    static_assert(std::has_single_bit(Capacity), "Capacity must be a power of two for bitwise wrapping");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for atomic operations");
    static_assert(std::atomic<T>::is_always_lock_free, "T must be lock-free on this architecture");
public:
    BasicAtomicQueueCompilerHints() : _queue(std::make_unique<std::atomic<T>[]>(Capacity)), _capacity(Capacity), _head(0), _tail(0){
        for (std::size_t i = 0; i < Capacity; ++i) {
            _queue[i].store(static_cast<T>(-1), std::memory_order_relaxed);
        }
    }

    bool try_enqueue(const T& item) {
        std::size_t current_tail = _tail.load(std::memory_order_relaxed);

        while (true) {
            std::size_t current_head = _head.load(std::memory_order_acquire);

            if (current_tail - current_head == _capacity) [[unlikely]] {
                return false;
            }

            if (_tail.compare_exchange_weak(current_tail, current_tail + 1, std::memory_order_release, std::memory_order_relaxed)) {
                size_t index = current_tail & (_capacity - 1);
                while (_queue[index].load(std::memory_order_acquire) != static_cast<T>(-1)) {
                    asm volatile("yield" ::: "memory");
                }
                _queue[index].store(item, std::memory_order_release);
                return true; 
            }
        }
    }
    bool try_enqueue(T&& item) {
        std::size_t current_tail = _tail.load(std::memory_order_relaxed);

        while (true) {
            std::size_t current_head = _head.load(std::memory_order_acquire);

            if (current_tail - current_head == _capacity) [[unlikely]] {
                return false;
            }

            if (_tail.compare_exchange_weak(current_tail, current_tail + 1, std::memory_order_release, std::memory_order_relaxed)) {
                size_t index = current_tail & (_capacity - 1);
                while (_queue[index].load(std::memory_order_acquire) != static_cast<T>(-1)) {
                    asm volatile("yield" ::: "memory");
                }
                _queue[index].store(item, std::memory_order_release);
                return true; 
            }
        }
    }
    bool try_dequeue(T& item) {
        std::size_t current_head = _head.load(std::memory_order_relaxed);
        std::size_t current_tail = _tail.load(std::memory_order_acquire);

        if (current_head == current_tail) [[unlikely]] {
            return false;
        }

        std::size_t index = current_head & (_capacity - 1);
        T data;

        while ((data = _queue[index].load(std::memory_order_acquire)) == static_cast<T>(-1)) {
            asm volatile("yield" ::: "memory");
        }
        item = data;
        _queue[index].store(static_cast<T>(-1), std::memory_order_release);
        _head.store(current_head + 1, std::memory_order_relaxed);
        return true;
    }

    std::size_t capacity() const {
        return _capacity;
    }
    bool is_empty() const {
        return _head.load(std::memory_order_relaxed) == _tail.load(std::memory_order_acquire);
    }
    bool is_full() const {
        return (_tail.load(std::memory_order_relaxed) - _head.load(std::memory_order_acquire)) == _capacity;
    }

private:
    std::unique_ptr<std::atomic<T>[]> _queue;
    const std::size_t _capacity;
    std::atomic<std::size_t> _head;
    std::atomic<std::size_t> _tail;

};