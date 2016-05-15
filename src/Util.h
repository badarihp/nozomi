#pragma once

#include <utility>

#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/mpl/at.hpp>

namespace nozomi {

template <typename Container>
inline void push_back_move(Container& container) {}

/**
 * Pushes arguments back onto a container. Useful for functions
 * that have variable numbers of arguments that may be move
 * only
 */
template <typename Container, typename Value, typename... Values>
inline void push_back_move(Container& container,
                           Value&& value,
                           Values&&... values) {
  container.push_back(std::move(value));
  push_back_move(container, std::move(values)...);
}

/**
 * A small class that works like an integer_sequence,
 * except with types. It's sometimes easier to do type
 * deduction with this if you don't have an actual function
 * argument that uses your variadic template parameters
 * directly.
 */
template <typename... Types>
struct type_sequence {};

/**
 * A way to represent parameters' types using boost meta-programming lanaguage
 * libs
 */
template <typename CallablePointer, size_t N>
struct parameter_type {
  using args_t =
      typename boost::function_types::parameter_types<CallablePointer>::type;
  using type = typename boost::mpl::at_c<args_t, N>::type;
};

/** Internal method that skips the zeroth parameter which is 'this' */
template <typename CallableMemberPointer, size_t Zero, size_t... N>
auto make_type_sequence(type_sequence<CallableMemberPointer>,
                        std::index_sequence<Zero, N...>) {
  return type_sequence<typename parameter_type<CallableMemberPointer, N>::type...>();
}

/**
 * Takes a pointer to a member function and returns a type_sequence
 * corresponding to its parameter types
 */
template <typename CallableMemberPointer>
auto make_type_sequence() {
  constexpr int N =
      boost::function_types::function_arity<CallableMemberPointer>::value;
  return make_type_sequence(type_sequence<CallableMemberPointer>{},
                            std::make_index_sequence<N>{});
}

/** Internal method that skips the zeroth parameter which is 'this' */
template <typename Callable, size_t Zero, size_t... N>
auto make_type_sequence(const Callable&, std::index_sequence<Zero, N...>) {
  return type_sequence<
      typename parameter_type<decltype(&Callable::operator()), N>::type...>();
}

/**
 * Makes a type_sequence from an object that has an operator()
 * member function. Note that this only works for objects with
 * one operator() method, but works great for lambdas and std::functions
 */
template <typename Callable>
auto make_type_sequence(const Callable& handler) {
  constexpr int N = boost::function_types::function_arity<decltype(
      &Callable::operator())>::value;
  return make_type_sequence(handler, std::make_index_sequence<N>{});
}
}
