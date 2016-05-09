#pragma once

#include <folly/futures/Future.h>

#include <proxygen/httpserver/RequestHandler.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

namespace sakura {
enum RouteMatchResult {
  PathNotMatched,    // Path didn't match
  MethodNotMatched,  // Path matched, method didn't
  RouteMatched,      // The route matched entirely
};

/*
 * Determines whether a Route matches a given pattern, and if not,
 * why it doesn't. Mutable so that we can easily move member
 * functions out and pass them to handlers
 */
struct RouteMatch {
  RouteMatch(
      RouteMatchResult result,
      std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler =
          std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>(),

      std::function<proxygen::RequestHandler*()> streamingHandler =

          std::function<proxygen::RequestHandler*()>())
      : result(result),
        handler(std::move(handler)),
        streamingHandler(std::move(streamingHandler)) {
    DCHECK(result != RouteMatched || (bool)this->streamingHandler ||
           (bool)this->handler)
        << "Either streaming handler or handler must be set if the route "
           "matched!";
  }

  RouteMatchResult result;
  std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler;
  std::function<proxygen::RequestHandler*()> streamingHandler;
};
}
