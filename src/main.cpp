#include <iostream>

#include "src/Config.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/Router.h"
#include "src/Server.h"
#include "src/StaticRoute.h"

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/SignalHandler.h>
using namespace std;
using namespace sakura;
using Method = proxygen::HTTPMethod;

struct SampleController {
  static HTTPResponse method(const HTTPRequest& request, int64_t i) {
    return HTTPResponse(200, "hello, world!");
  }
  static HTTPResponse static_method(const HTTPRequest& request) {
    return HTTPResponse(200, "hello, world!");
  }
};

int main() {
  int a = 1;
  auto router = make_router(
      {}, make_route("/", {Method::GET},
                     std::function<HTTPResponse(const HTTPRequest&)>(
                         [](const HTTPRequest& request) {
                           return HTTPResponse(200, "hello, world");
                         })),
      make_static_route("/test", {Method::GET},
                        [&a](const HTTPRequest& request) mutable {
                          a++;
                          return HTTPResponse(200);
                        }),
      make_route("/testing", {Method::GET},
                 [&a](const HTTPRequest& request) {
                   a++;
                   return HTTPResponse(200);
                 }),
      make_route("/{{i}}", {Method::GET},
                 [](const HTTPRequest& request, int64_t i) {
                   return HTTPResponse(200,
                                       folly::sformat("hello, world: {}", i));
                 }),
      make_route("/test/{{i}}", {Method::GET}, &SampleController::method),
      make_static_route("/img/1", {Method::GET},
                        std::function<HTTPResponse(const HTTPRequest&)>(
                            [](const HTTPRequest& request) {
                              return HTTPResponse(200, "hello, world");
                            })),
      make_static_route("/img/2", {Method::GET},
                        [](const HTTPRequest& request) {
                          return HTTPResponse(200, "hello, world: {}");
                        }),
      make_route("/img/3", {Method::GET}, &SampleController::static_method));
  Config config({make_tuple("0.0.0.0", 8080, Config::Protocol::HTTP)}, 2);
  Server server(config, std::move(router));
  auto started = server.start().get();
  string ignore;
  cout << "Press enter" << endl;
  cin >> ignore;
  auto stopped = server.stop().get();
  return 0;
}
