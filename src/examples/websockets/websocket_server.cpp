#include <iostream>

#include "src/Config.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/Router.h"
#include "src/Server.h"
#include "src/StaticRoute.h"
#include "src/StreamingFileHandler.h"
#include "src/WebsocketHandler.h"

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

DEFINE_bool(spin, false, "");

struct EchoHandler: public WebsocketHandler<> {
  void setRequestArgs() noexcept { cout << "Set args" << endl; }
  void onMessage(std::unique_ptr<folly::IOBuf> message) noexcept {
    cout << "Got message" << endl;
  }
};

int main(int argc, char** argv) {
     gflags::ParseCommandLineFlags(&argc, &argv, true);
  Config config({make_tuple("0.0.0.0", 8080, Config::Protocol::HTTP)}, 2);
  auto router = make_router(
      {}, make_route("/", {Method::GET},
                     [](const HTTPRequest& request) {
                       return HTTPResponse::future(200);
                     }),
        make_streaming_route("/ws", {Method::GET}, []() {
            return new EchoHandler();
        }),
        make_streaming_route("/{{s:.*}}", {Method::GET}, []() {
        return new StreamingFileHandler("src/examples/websockets/public");
      }));
  Server httpServer(std::move(config), std::move(router));

  auto started = httpServer.start().get();
  if(FLAGS_spin) {
    while(true) { };
  } else {
    cout << "Press any key to stop" << endl;
    getchar();
  }
  auto stopped = httpServer.stop().get();
  return 0;
}

