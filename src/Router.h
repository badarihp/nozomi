#pragma once

#include <initializer_list>
#include <list>
#include <string>
#include <unordered_map>

#include <boost/regex.hpp>

#include "src/HTTPResponse.h"
#include "src/HTTPRequest.h"
#include "src/Route.h"

namespace sakura {

class Router {
  private:
    // list of (route, method -> Route)
    std::list<std::pair<std::string, std::unordered_map<std::string, Route>>> staticRoutes_;
    std::list<std::pair<boost::basic_regex<char>, std::unordered_map<std::string, Route>>> routes_;
  public:
    Router(
      std::initializer_list<Route> routes,
      std::initializer_list<std::pair<int, Route>> errorRoutes);
    const Route& getRoute(std::string path, std::string method) const;
    //TODO: Streaming
};

}
