#pragma once

#include <folly/futures/Future.h>

#include <proxygen/httpserver/RequestHandler.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

namespace nozomi {
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
  RouteMatchResult result;
  std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler;
  std::function<proxygen::RequestHandler*()> streamingHandler;

  /**
   * Creates a RouteMatch object
   *
   * @result - The result of the match. See RouteMatchResult
   * @handler - A handler that returns a Future. Used when the whole request /
   *            response can reasonably be expected to fit in memory for the
   *            from the first byte being read, until the last byte is sent on
   *            the wire. Mutually exclusive with streamingHandler
   */
  RouteMatch(
      RouteMatchResult result,
      std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler =
          std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>())

      : result(result), handler(std::move(handler)) {
    DCHECK(result != RouteMatched || this->handler)
        << "Handler must be set if the route matched!";
  }

  /**
   * Creates a RouteMatch object
   *
   * @result - The result of the match. See RouteMatchResult
   * @streamingHandler - A handler that returns a StreamingHandler. This should
   *                     be used if the entire response will not likely fit in
   *                     memory. Mutually exclusive with handler
   *
   */
  RouteMatch(RouteMatchResult result,
             std::function<proxygen::RequestHandler*()> streamingHandler)
      : result(result), streamingHandler(std::move(streamingHandler)) {
    DCHECK(result != RouteMatched || this->streamingHandler)
        << "Streaming handler must be set if the route matched!";
  }
};
}
