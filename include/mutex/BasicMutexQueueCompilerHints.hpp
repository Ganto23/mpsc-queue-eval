#pragma once
#include <mutex>
#include <vector>
#include <cstddef>

template <typename T, std::size_t Capacity = 8192>
class BasicMutexQueueCompilerHints {
    static_assert(std::has_single_bit(Capacity), "Capacity must be a power of two for bitwise wrapping");
public:
    BasicMutexQueueCompilerHints() : _queue(Capacity), _capacity(Capacity), _head(0), _tail(0){}

    bool try_enqueue(const T& item) {
        std::lock_guard<std::mutex> lock(_mtx);
        if ((_tail - _head) == _capacity) [[unlikely]] {
            return false;
        }
        _queue[_tail & (_capacity - 1)] = item;
        _tail++;
        return true;
    }
    bool try_enqueue(T&& item) {
        std::lock_guard<std::mutex> lock(_mtx);
        if ((_tail - _head) == _capacity) [[unlikely]] {
            return false;
        }
        _queue[_tail & (_capacity - 1)] = std::move(item);
        _tail++;
        return true;
    }
    bool try_dequeue(T& item) {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_head == _tail) [[unlikely]] {
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
        std::lock_guard<std::mutex> lock(_mtx);
        return _head == _tail;
    }
    bool is_full() const {
        std::lock_guard<std::mutex> lock(_mtx);
        return (_tail - _head) == _capacity;
    }

private:
    mutable std::mutex _mtx;
    std::vector<T> _queue;
    const std::size_t _capacity;
    std::size_t _head;
    std::size_t _tail;

};