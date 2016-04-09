#include "src/Router.h"

#include <stdexcept>

using std::initializer_list;
using std::string;
using std::pair;

namespace sakura {

    Router::Router(
      std::initializer_list<Route> routes,
      std::initializer_list<std::pair<int, Route>> errorRoutes) { }
    const Route& Router::getRoute(std::string path, std::string method) const { throw std::runtime_error("Not implemented"); }

}

