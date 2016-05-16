#include <folly/io/async/EventBase.h>
#include <glog/logging.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/Config.h"
#include "src/HTTPHandler.h"
#include "src/Router.h"

namespace nozomi {

/**
 * Factory that creates HTTPHandlers based on router matches
 *
 * @tparam HandlerType - The class to instantiate for non-streaming
 *                       Request handlers
 */
template <typename HandlerType = HTTPHandler>
class HTTPHandlerFactory : public virtual proxygen::RequestHandlerFactory {
 private:
  Config config_;
  Router router_;
  folly::EventBase* evb_;

 public:
  /**
   * Creates an HTTPHandlerFactory
   *
   * @param config - The server configuration
   * @param router - The router object to use to fetch handlers
   */
  HTTPHandlerFactory(Config config, Router router)
      : config_(std::move(config)), router_(std::move(router)) {}

  /**
   * @copydoc proxygen::RequestHandlerFactory::onServerStart()
   */
  void onServerStart(folly::EventBase* evb) noexcept {
    DCHECK(evb != nullptr);
    evb_ = evb;
    LOG(INFO) << "Started handler factory";
  }

  /**
   * @copydoc proxygen::RequestHandlerFactory::onServerStop()
   */
  void onServerStop() noexcept { LOG(INFO) << "Stopped handler factory"; }

  /**
   * @copydoc proxygen::RequestHandlerFactory::onRequest()
   */
  proxygen::RequestHandler* onRequest(proxygen::RequestHandler* previousHandler,
                                      proxygen::HTTPMessage* message) noexcept {
    DCHECK(message != nullptr);
    auto routeMatch = router_.getHandler(message);
    DCHECK((bool)routeMatch.handler || (bool)routeMatch.streamingHandler)
        << "Neither handler nor streamingHandler were set in "
           "HTTPHandlerFactory";
    // TODO: Error handling if streamingHandler() blows up, or if somehow
    // neither of those two handlers are set
    if (routeMatch.handler) {
      return new HandlerType(config_.getRequestTimeout(), &router_,
                             std::move(routeMatch.handler));
    } else if (routeMatch.streamingHandler) {
      // TODO: If streamingHandler is null, we need to instead return
      //      a default handler that returns a 500
      return routeMatch.streamingHandler();
    } else {
      return nullptr;
    }
  }
};
}
