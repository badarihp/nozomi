#include <folly/io/async/EventBase.h>
#include <glog/logging.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/Config.h"
#include "src/HTTPHandler.h"
#include "src/Router.h"

namespace nozomi {

template <typename HandlerType = HTTPHandler>
class HTTPHandlerFactory : public virtual proxygen::RequestHandlerFactory {
 private:
  Config config_;
  Router router_;
  folly::EventBase* evb_;

 public:
  HTTPHandlerFactory(Config config, Router router)
      : config_(std::move(config)), router_(std::move(router)) {}

  void onServerStart(folly::EventBase* evb) noexcept {
    DCHECK(evb != nullptr);
    evb_ = evb;
    LOG(INFO) << "Started handler factory";
  }

  /**
   * Invoked in each handler thread after all the connections are
   * drained from that thread. Can be used to tear down thread-local setup.
   */
  void onServerStop() noexcept { LOG(INFO) << "Stopped handler factory"; }

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
  proxygen::RequestHandler* onRequest(proxygen::RequestHandler* previousHandler,
                                      proxygen::HTTPMessage* message) noexcept {
    DCHECK(message != nullptr);
    auto routeMatch = router_.getHandler(message);
    // TODO: Error handling if streamingHandler() blows up, or if somehow
    // neither
    //      of those two handlers are set
    if (routeMatch.handler) {
      return new HandlerType(config_.getRequestTimeout(), &router_,
                             std::move(routeMatch.handler));
    } else if (routeMatch.streamingHandler) {
      // TODO: If streamingHandler is null, we need to instead return
      //      a default handler that returns a 500
      return routeMatch.streamingHandler();
    }
  }
};
}
