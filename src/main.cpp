#include <iostream>

#include "src/Config.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/Router.h"
#include "src/Server.h"
#include "src/StaticRoute.h"
#include "src/StreamingFileHandler.h"

#include <folly/futures/Future.h>
#include <folly/futures/Future.h>
#include <gflags/gflags.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/SignalHandler.h>
#include <wangle/concurrent/GlobalExecutor.h>
using namespace std;
using namespace nozomi;
using Method = proxygen::HTTPMethod;

struct SampleController {
  static folly::Future<HTTPResponse> method(const HTTPRequest& request,
                                            int64_t i) {
    return HTTPResponse::future(200, "hello, world!");
  }
  static folly::Future<HTTPResponse> static_method(const HTTPRequest& request) {
    return folly::futures::sleep(std::chrono::milliseconds(1200)).then([]() {
      return HTTPResponse::future(200, "hello, world!");
    });
  }
  //static auto method() { return new StreamingFileHandler(); }
};

int main(int argc, char** argv) {
  int a = 1;
  auto router = make_router(
      {}, make_route("/", {Method::GET}, &SampleController::static_method),
      //make_streaming_route("/test1", {Method::GET},
      //                     []() { return new StreamingFileHandler(); }),
      make_static_route("/route", {Method::GET},
                        &SampleController::static_method)
      //make_streaming_static_route("/test", {Method::GET},
       //                           []() { return new StreamingFileHandler(); })
      /*
            make_route(
                    "/", {Method::GET},
                    std::function<folly::Future<HTTPResponse>(const
         HTTPRequest&)>(
                        [](const HTTPRequest& request) {
                          return
         folly::via(wangle::getIOExecutor().get()).then([]() {
                            sleep(1);
                            return HTTPResponse::future(200, "hello, world");
                          });
                        })),
            make_static_route("/test", {Method::GET},
                              [&a](const HTTPRequest& request) mutable {
                                a++;
                                return HTTPResponse::future(200);
                              }),
            make_route("/testing", {Method::GET},
                       [&a](const HTTPRequest& request) {
                         a++;
                         return HTTPResponse::future(200);
                       }),
            make_route("/{{i}}", {Method::GET},
                       [](const HTTPRequest& request, int64_t i) {
                         return HTTPResponse::future(
                             200, folly::sformat("hello, world: {}", i));
                       }),
            make_route("/test/{{i}}", {Method::GET}, &SampleController::method),
            make_route("/{{i}}", {Method::GET},
                       [](const HTTPRequest& request, int64_t i) {
                         return HTTPResponse::future(
                             200, folly::sformat("hello, world: {}", i));
                       }),
            make_route("/test/{{i}}", {Method::GET}, &SampleController::method),
            make_static_route(
                "/img/1", {Method::GET},
                std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>(
                    [](const HTTPRequest& request) {
                      return HTTPResponse::future(200, "hello, world");
                    })),
            make_static_route("/img/2", {Method::GET},
                              [](const HTTPRequest& request) {
                                return HTTPResponse::future(200, "hello, world:
         {}");
                              })
      */
      );
  google::ParseCommandLineFlags(&argc, &argv, true);
  Config config({make_tuple("0.0.0.0", 8080, Config::Protocol::HTTP)}, 2);
  Server server(config, std::move(router));
  auto started = server.start().get();
  string ignore;
  cout << "Press enter" << endl;
  cin >> ignore;
  auto stopped = server.stop().get();
  return 0;
}
