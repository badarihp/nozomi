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
    std::list<std::unique_ptr<BaseRoute>> staticRoutes_;
    std::list<std::unique_ptr<BaseRoute>> routes_;
    
  public:
    Router(
      std::initializer_list<std::unique_ptr<BaseRoute>> routes,
      std::initializer_list<std::pair<int, std::unique_ptr<BaseRoute>>> errorRoutes);
    //TODO: Streaming
};

}
