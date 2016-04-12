#include <stdlib.h>
#include <type_traits>

// Can't hash an enum class apparently
// Taken from
// http://stackoverflow.com/questions/9646297/c11-hash-function-for-any-enum-type
namespace std {
template <class E>
struct hash {
  using sfinae = typename std::enable_if<std::is_enum<E>::value, E>::type;

 public:
  size_t operator()(const E &e) const {
    return std::hash<typename std::underlying_type<E>::type>()(
        static_cast<typename std::underlying_type<E>::type>(e));
  }
};
}
