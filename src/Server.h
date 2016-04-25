#pragma once

#include <thread>

#include <proxygen/httpserver/HTTPServer.h>
#include <folly/futures/Future.h>
#include <folly/Optional.h>

#include "src/Config.h"
#include "src/Router.h"

namespace sakura {

class Server {
  private:
    Config config_;
    folly::Optional<std::thread> mainThread_;
    proxygen::HTTPServer server_;

  public:
    Server(Config config, Router router);
    folly::Future<folly::Unit> start();
    folly::Future<folly::Unit> stop();
};

}
