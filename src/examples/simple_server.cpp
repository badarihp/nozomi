#include <iostream>

#include "src/Config.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/Router.h"
#include "src/Server.h"
#include "src/StaticRoute.h"
#include "src/StreamingFileHandler.h"

#include <boost/filesystem.hpp>
#include <folly/futures/Future.h>
#include <folly/futures/Future.h>
#include <gflags/gflags.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/SignalHandler.h>
#include <wangle/concurrent/GlobalExecutor.h>
using namespace std;
using namespace nozomi;
using Method = proxygen::HTTPMethod;

int main() {
  Config config({make_tuple("0.0.0.0", 8080, Config::Protocol::HTTP)}, 2);
  auto router = make_router(
      {}, make_route("/", {Method::GET},
                     [](const HTTPRequest& request) {
                       return HTTPResponse::future(200);
                     }),
      make_route("/user", {Method::POST},
                 [](const HTTPRequest& request) {
                   /* create new user here */
                   return HTTPResponse::future(200, "Created user!");
                 }),
      make_route("/user/{{i}}", {Method::POST},
                 [](const HTTPRequest& request, int64_t userId) {
                   /* fetch user data */
                   return HTTPResponse::future(200, "" /* user data */);
                 }),
      make_streaming_route("/{{s:.*}}", {Method::GET}, []() {
        return new StreamingFileHandler("public");
      }));
  Server httpServer(std::move(config), std::move(router));

  auto started = httpServer.start().get();
  string ignore;
  cout << "Press any key to stop" << endl;
  getchar();
  auto stopped = httpServer.stop().get();
  return 0;
}
