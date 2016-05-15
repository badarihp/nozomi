#include "src/Server.h"

#include <glog/logging.h>

#include <folly/futures/Promise.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <proxygen/httpserver/SignalHandler.h>

#include "src/HTTPHandlerFactory.h"

using folly::Future;
using folly::Optional;
using folly::Promise;
using folly::Unit;
using proxygen::HTTPServer;
using proxygen::HTTPServerOptions;
using proxygen::RequestHandlerFactory;
using std::thread;
using std::unique_ptr;
using std::vector;
using folly::exception_wrapper;

namespace nozomi {

HTTPServerOptions getHTTPServerOptions(const Config& config, Router router) {
  HTTPServerOptions options;
  options.threads = config.getWorkerThreads();
  vector<unique_ptr<RequestHandlerFactory>> handlerFactories;
  handlerFactories.push_back(
      std::make_unique<HTTPHandlerFactory<>>(config, std::move(router)));
  options.handlerFactories = std::move(handlerFactories);
  options.enableContentCompression = true;
  return options;
}

Server::Server(Config config, Router router)
    : config_(std::move(config)),
      server_(getHTTPServerOptions(config_, std::move(router))) {}

folly::Future<Unit> Server::start() {
  CHECK(!mainThread_) << "A server can only be started once";
  auto addresses = config_.getHTTPAddresses();
  server_.bind(addresses);
  auto startPromise = std::make_shared<Promise<Unit>>();
  mainThread_ = thread([this, startPromise] {
    LOG(INFO) << "Starting server";
    server_.start(
        [startPromise]() {
          LOG(INFO) << "Started server";
          startPromise->setValue();
        },
        [startPromise](std::exception_ptr e) {
          LOG(INFO) << "Could not start server";
          startPromise->setException(exception_wrapper(e));
        });
  });
  return startPromise->getFuture();
}

folly::Future<Unit> Server::stop() {
  auto p = std::make_shared<Promise<Unit>>();
  Future<Unit> f = p->getFuture();
  thread t([p, this] {
    // TODO: Make this more threadsafe
    if (!mainThread_) {
      p->setValue();
      return;
    }

    server_.stop();
    mainThread_->join();
    mainThread_.clear();
    p->setValue();
  });
  t.detach();
  return f;
}
}
