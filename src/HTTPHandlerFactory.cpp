#include "src/HTTPHandler.h"
#include "src/HTTPHandlerFactory.h"

#include <glog/logging.h>

namespace sakura {

HTTPHandlerFactory::HTTPHandlerFactory(Config config, Router router)
    : config_(std::move(config)), router_(std::move(router)) {}

void HTTPHandlerFactory::onServerStart(folly::EventBase* evb) noexcept {
  LOG(INFO) << "Started server";
}

/**
 * Invoked in each handler thread after all the connections are
 * drained from that thread. Can be used to tear down thread-local setup.
 */
void HTTPHandlerFactory::onServerStop() noexcept {
  LOG(INFO) << "Stopped server";
}

/**
 * Invoked for each new request server handles. HTTPMessage is provided
 * so that user can potentially choose among several implementation of
 * handler based on URL or something. No, need to save/copy this
 * HTTPMessage. RequestHandler will be given the HTTPMessage
 * in a separate callback.
 *
 * Some request handlers don't handle the request themselves (think filters).
 * They do take some actions based on request/response but otherwise they
 * just hand-off request to some other RequestHandler. This upstream
 * RequestHandler is given as first parameter. For the terminal RequestHandler
 * this will by nullptr.
 */
proxygen::RequestHandler* HTTPHandlerFactory::onRequest(
    proxygen::RequestHandler* previousHandler,
    proxygen::HTTPMessage* message) noexcept {
  DCHECK(message != nullptr);
  return new HTTPHandler(router_, router_.getHandler(message));
}
}
