#pragma once
#include <concepts>
#include <cstddef>

template <typename Q, typename T>
concept hasTryEnqueueCopy = requires(Q queue, const T& item){
    {queue.try_enqueue(item)} -> std::same_as<bool>;
};

template <typename Q, typename T>
concept hasTryEnqueueMove = requires(Q queue, T&& item){
    {queue.try_enqueue(std::move(item))} -> std::same_as<bool>;
};

template <typename Q, typename T>
concept hasTryDequeue = requires(Q queue, T& item){
    {queue.try_dequeue(item)} -> std::same_as<bool>;
};

template <typename Q>
concept hasCapacity = requires(const Q const_queue){
    {const_queue.capacity()} -> std::same_as<size_t>;
};

template <typename Q>
concept hasIsEmpty = requires(const Q const_queue){
    {const_queue.is_empty()} -> std::same_as<bool>;
};

template <typename Q>
concept hasIsFull = requires(const Q const_queue){
    {const_queue.is_full()} -> std::same_as<bool>;
};


template <typename Q, typename T>
concept MPSCQueue = hasTryEnqueueCopy<Q,T> && hasTryEnqueueMove<Q,T> && 
        hasTryDequeue<Q,T> && hasCapacity<Q> && hasIsEmpty<Q> && hasIsFull<Q>;