#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/Config.h"

namespace sakura {

class HTTPHandlerFactory : public virtual proxygen::RequestHandlerFactory {
 private:
  Config config_;
  Router router_;

 public:
  HTTPHandlerFactory(Config config, Router router);

  /**
   * Invoked in each thread server is going to handle requests
   * before we start handling requests. Can be used to setup
   * thread-local setup for each thread (stats and such).
   */
  virtual void onServerStart(folly::EventBase* evb) noexcept override;

  /**
   * Invoked in each handler thread after all the connections are
   * drained from that thread. Can be used to tear down thread-local setup.
   */
  virtual void onServerStop() noexcept override;

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
  virtual proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler*, proxygen::HTTPMessage*) noexcept override;
};
}
