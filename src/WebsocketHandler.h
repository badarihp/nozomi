#pragma once

#include "src/StreamingHTTPHandler.h"
#include "src/WebsocketFrame.h"

namespace nozomi {

template<typename... HandlerArgs>
class WebsocketHandler: public StreamingHTTPHandler<HandlerArgs...> {
  private:
  WebsocketFrameParser parser_;
  public:
  void closeConnection() noexcept;
  void sendMessage(std::unique_ptr<folly::IOBuf> message) noexcept;

  virtual void setRequestArgs(HandlerArgs... args) noexcept override = 0;
  virtual void onMessage(std::unique_ptr<folly::IOBuf> message) noexcept = 0;

  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept final;
  virtual void onEOM() noexcept final;
  virtual void onRequestReceived(const HTTPRequest& request) noexcept final;
  virtual void onRequestComplete() noexcept final;
  virtual void onUnhandledError(proxygen::ProxygenError err) noexcept final;

  virtual void onUpgrade(proxygen::UpgradeProtocol prot) noexcept override;
};

}

#include "src/WebsocketHandler-inl.h"
