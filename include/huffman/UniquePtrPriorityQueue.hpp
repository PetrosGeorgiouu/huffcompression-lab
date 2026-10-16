#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T, typename Comp = std::less<T>>

class UniquePtrPriorityQueue
{
public:
  explicit UniquePtrPriorityQueue(std::size_t capacity, Comp comparator = Comp{}) : capacity_(capacity), compare_(std::move(comparator))
  {
    heap_.reserve(capacity);
  }

  UniquePtrPriorityQueue(const UniquePtrPriorityQueue &) = delete;
  UniquePtrPriorityQueue &operator=(const UniquePtrPriorityQueue &) = delete;

  UniquePtrPriorityQueue(UniquePtrPriorityQueue &&) = default;
  UniquePtrPriorityQueue &operator=(UniquePtrPriorityQueue &&) = default;

  void push(std::unique_ptr<T> &&element)
  {
    if (!element)
    {
      throw std::invalid_argument("Can't push a nullptr.");
    }
    if (full())
    {
      throw std::length_error("Queue at maximum capacity.");
    }
    heap_.push_back(std::move(element));
    std::push_heap(heap_.begin(), heap_.end(), pointer_compare());
  }

  [[nodiscard]] std::unique_ptr<T> extract_top()
  {
    if (empty())
    {
      throw std::length_error("Queue is empty");
    }
    std::pop_heap(heap_.begin(), heap_.end(), pointer_compare());
    auto result = std::move(heap_.back());
    heap_.pop_back();
    return result;
  }

  [[nodiscard]] bool empty() const noexcept
  {
    return heap_.empty();
  }

  [[nodiscard]] bool full() const noexcept
  {
    return heap_.size() == capacity_;
  }

  [[nodiscard]] std::size_t size() const noexcept
  {
    return heap_.size();
  }

  [[nodiscard]] std::size_t capacity() const noexcept
  {
    return capacity_;
  }

private:
  auto pointer_compare() const
  {
    return [this](const std::unique_ptr<T> &a, const std::unique_ptr<T> &b)
    {
      return compare_(*a, *b);
    };
  }
  std::size_t capacity_;
  Comp compare_;
  std::vector<std::unique_ptr<T>> heap_;
};