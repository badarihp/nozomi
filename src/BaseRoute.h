#pragma once

#include <string>
#include <unordered_set>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/RouteMatch.h"

#include <folly/Optional.h>
#include <proxygen/lib/http/HTTPMethod.h>

namespace nozomi {

class BaseRoute {
 protected:
  std::string originalPattern_;
  std::unordered_set<proxygen::HTTPMethod> methods_;
  bool isStaticRoute_;
  // TODO: This should take a path and a method, not a request

  BaseRoute(std::string originalPattern,
            std::unordered_set<proxygen::HTTPMethod> methods,
            bool isStaticRoute)
      : originalPattern_(std::move(originalPattern)),
        methods_(std::move(methods)),
        isStaticRoute_(isStaticRoute) {}

 public:
  virtual ~BaseRoute(){};
  virtual RouteMatch handler(const proxygen::HTTPMessage* request) = 0;
  inline bool isStaticRoute() { return isStaticRoute_; }
};
}
