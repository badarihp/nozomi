#pragma once

#include <string>
#include <unordered_set>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/RouteMatch.h"

#include <folly/Optional.h>
#include <proxygen/lib/http/HTTPMethod.h>

namespace nozomi {

/**
 * The base type for all route objects
 */
class BaseRoute {
 protected:
  std::string originalPattern_;
  std::unordered_set<proxygen::HTTPMethod> methods_;
  bool isStaticRoute_;

  /**
   * Sets up some common properties of all routes
   * @param originalPattern - The original pattern provided to the
   *                          subclass
   * @param methods - All methods that this route is valid for if
   *                  the path matches
   * @param isStaticRoute - Whether this is a static route. This is
   *                        helpful for a Router to prioritize routes
   *                        that will have a faster handler() method
   *                        over those with a slower handler() method
   */
  BaseRoute(std::string originalPattern,
            std::unordered_set<proxygen::HTTPMethod> methods,
            bool isStaticRoute)
      : originalPattern_(std::move(originalPattern)),
        methods_(std::move(methods)),
        isStaticRoute_(isStaticRoute) {}

 public:
  virtual ~BaseRoute(){};
  /**
   * Get a RouteMatch object based on whether the provided request
   * matches this routes' path and method. Successful matches usually
   * wrap a user-provided handler. See Route and StaticRoute
   */
  virtual RouteMatch handler(const proxygen::HTTPMessage* request) = 0;
  inline bool isStaticRoute() { return isStaticRoute_; }
};
}
