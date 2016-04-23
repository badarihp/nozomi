#pragma once

#include <utility>

namespace sakura {

template <typename Container>
inline void push_back_move(Container& container) {}

template <typename Container, typename Value, typename... Values>
inline void push_back_move(Container& container,
                           Value&& value,
                           Values&&... values) {
  container.push_back(std::move(value));
  push_back_move(container, std::move(values)...);
}
}
