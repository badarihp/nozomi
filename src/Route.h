#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "src/BaseRoute.h"
#include "src/EnumHash.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/StreamingHTTPHandler.h"
#include "src/Util.h"

#include <boost/regex.hpp>
#include <folly/Optional.h>
#include <folly/futures/Future.h>
#include <proxygen/lib/http/HTTPMethod.h>

namespace nozomi {

/**
 * Creates a BaseRoute object that matches based on regular expressions and
 * can extract parameters from the user-provided HTTP request, and send
 * them in a type safe way to the provided handler. There are helper
 * functions make_route and make_streaming_route that make construction
 * of this class easier.
 *
 * @tparam HandlerType - The type of the Handler. This can be a function,
 *                       a lambda, or a function pointer. If it's callable,
 *                       it should be able to be used.
 * @tparam IsStreaming - Whether or not HandlerType returns a StreamingHandler
 * @tparams HandlerArgs - Type of arguments that should be extracted from
 *                        the route, and passed along to the route handler.
 */
template <typename HandlerType, bool isStreaming, typename... HandlerArgs>
class Route : public BaseRoute {
 private:
  HandlerType handler_;
  boost::basic_regex<char> regex_;

 public:
  /**
   * Creates an instance of Route.
   * @param pattern - The pattern to match HTTP requests' path against.
   *                  This pattern can contain:
   *                  - Arbitrary regular expressions. These are not currently
   *                    passed into the handler.
   *                  - Typed parameters described below. If one of these is
   *                    used, it will be expected (and enforced) that handler
   *                    can accept variables of the corresponding type in
   *                    addition to a request object
   *                  - {{i}} matches int64_t strings. If the string is larger
   *                    or smaller than an int64_t can represent, int64_t max
   *                    is used.
   *                  - {{i?:<consumed>}} matches a folly::Optional<int64_t>.
   *                    <consumed> is a regular expression that is consumed
   *                    regardless of whether there is a match. This is useful
   *                    for consuming slashes if the optional portion of the
   *                    path is missing in the request.
   *                  - {{d}} matches a double. If the string is larger or
   *                    smaller than a double can fit, inf/-inf will be
   *                    used.
   *                  - {{d?:<consumed>}} matches a folly::Optional<double>.
   *                    See above for how <consumed> functions
   *                  - {{s:<regex>}} passes the string that matches <regex>
   *                    into the handler
   *                  - {{s?:<regex>:<consumed>}} passes a
   *                    folly::Optional<string> based on whether <regex>
   *                    matches. See above for how <consumed> works.
   * @param methods - The list of methods that this route is valid for
   * @param handler - A callable that does one of two things:
   *                  - Takes a const HTTPRequest& and HandlerArgs, and returns
   *                    a folly::Future<HTTPResponse>. This is valid if
   *                    IsStreaming is false
   *                  - Takes no arguments and returns a free instance
   *                    StreamingHTTPRequest<HandlerArgs>* This is valid if
   *                    IsStreaming is true
   * @throws runtime_error if the arguments of the handler do not match the
   *                       parameters in pattern
   *
   * @examples Here are some examples of Route instantiations using helper
   *           functions from below:
   *  - make_route("/user/{{i}}", {HTTPMethod::GET},
   *          [](const HTTPRequest&, int64_t userId){
   *              // fetch user data
   *              return HTTPResponse::future(200, userDataJson);
   *          })
   *  - make_route("/user", PHTTPMethod::POST},
   *         [](const HTTPRequest&) {
   *             // create user
   *             return HTTPResponse::future(200, userDataJson);
   *         })
   *  - make_route("/some_url", PHTTPMethod::POST},
   *          &SomeController::someStaticMethod);
   *  - make_route("/complex_url/{{i}}/{{i?:/}}{{s:(.*)}}",
   *               {HTTPMethod::GET},
   *               [](const HTTPRequest&, int64_t, Optional<int64_t>, string){
   *                  return HTTPResponse::future(200, "All good!");
   *               });
   *  - make_streaming_route("/.*", {HTTPMethod::GET},
   *          [](const HTTPRequest&) {
   *              return SomeStreamingFileHandler();
   *          })
   *  - !! ERROR!!
   *    make_route("/user/{{i}}", PHTTPMethod::POST},
   *         [](const HTTPRequest&) {
   *             // create user
   *             return HTTPResponse::future(200, userDataJson);
   *         })
   */
  Route(std::string pattern,
        std::unordered_set<proxygen::HTTPMethod> methods,
        HandlerType handler);
  virtual RouteMatch handler(const proxygen::HTTPMessage* request) override;
};

/**
 * Creates a non-streaming Route object from a function pointer. See Route<>
 * for description of all of the parameters.
 */
template <typename... HandlerArgs>
inline auto make_route(std::string pattern,
                       std::unordered_set<proxygen::HTTPMethod> methods,
                       folly::Future<HTTPResponse> (*handler)(
                           const HTTPRequest&, HandlerArgs...)) {
  return std::make_unique<Route<decltype(handler), false, HandlerArgs...>>(
      std::move(pattern), std::move(methods), std::move(handler));
}

/**
 * Creates a streaming Route object from a function pointer. See Route<>
 * for description of all of the parameters.
 */
template <typename RequestHandlerType>
inline auto make_streaming_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    RequestHandlerType* (*handler)()) {
  // TODO: Tests
  return make_streaming_route<decltype(handler)>(
      std::move(pattern), std::move(methods), std::move(handler));
}

/** Internal method used to help with template type deduction */
template <typename HandlerType, typename... HandlerArgs>
inline auto make_streaming_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler,
    type_sequence<HandlerArgs...>) {
  return std::make_unique<Route<HandlerType, true, HandlerArgs...>>(
      std::move(pattern), std::move(methods), std::move(handler));
}

/** Internal method used to help with template type deduction */
template <typename HandlerType, typename Request, typename... HandlerArgs>
inline auto make_route(std::string pattern,
                       std::unordered_set<proxygen::HTTPMethod> methods,
                       HandlerType handler,
                       type_sequence<Request, HandlerArgs...>) {
  return std::make_unique<Route<HandlerType, false, HandlerArgs...>>(
      std::move(pattern), std::move(methods), std::move(handler));
}

/**
 * Creates a streaming Route object from a callable (e.g. a lambda
 * or an std::function). See Route<> for description of all of the parameters.
 */
template <typename HandlerType>
inline auto make_streaming_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler) {
  // Get the type that comes out of the RequestHandler factory method
  using RequestHandler =
      typename std::remove_pointer<decltype(handler())>::type;
  // Using that type, get a pointer to the setRequestArgs method so
  // that we can rip the HandlerArgs... types out without having
  // to set them explicitly
  using CallablePointer = decltype(&RequestHandler::setRequestArgs);
  auto types = make_type_sequence<CallablePointer>();
  return make_streaming_route(std::move(pattern), std::move(methods),
                              std::move(handler), types);
}

/**
 * Creates a non-streaming Route object from a callable (e.g. a lambda
 * or an std::function). See Route<> for description of all of the parameters.
 */
template <typename HandlerType>
inline auto make_route(std::string pattern,
                       std::unordered_set<proxygen::HTTPMethod> methods,
                       HandlerType handler) {
  auto types = make_type_sequence(handler);
  return make_route(std::move(pattern), std::move(methods), std::move(handler),
                    types);
}
}

#include "src/Route-inl.h"
