#pragma once

#include <glog/logging.h>
#include <openssl/sha.h>
#include <proxygen/lib/utils/CryptUtil.h>

namespace nozomi {

template <typename... HandlerArgs>
void WebsocketHandler<HandlerArgs...>::onBody(
    std::unique_ptr<folly::IOBuf> body) noexcept {
  LOG(INFO) << "WS onBody";
}

template <typename... HandlerArgs>
void WebsocketHandler<HandlerArgs...>::onEOM() noexcept {
  LOG(INFO) << "WS onEOM";
}

template <typename... HandlerArgs>
void WebsocketHandler<HandlerArgs...>::onRequestReceived(
    const HTTPRequest& request) noexcept {
  LOG(INFO) << "WS onRequestReceived";

  auto upgrade = request.getHeaders().getHeader("Upgrade");
  auto connection = request.getHeaders().getHeader("Connection");
  auto secWebsocketKey = request.getHeaders().getHeader("Sec-WebSocket-Key");
  auto secWebsocketProtocol =
      request.getHeaders().getHeader("Sec-WebSocket-Protocol");
  auto secWebsocketVersion =
      request.getHeaders().getHeader("Sec-WebSocket-Version");

  if (!upgrade) {
    LOG(INFO) << "No upgrade received";
    return;
  }
  if (!connection) {
    LOG(INFO) << "No connection received";
    return;
  }
  if (!secWebsocketKey) {
    LOG(INFO) << "No secWebsocketKey received";
    return;
  }
  // if(!secWebsocketProtocol) { LOG(INFO) << "No secWebsocketProtocol
  // received"; return; }
  if (!secWebsocketVersion) {
    LOG(INFO) << "No secWebsocketVersion received";
    return;
  }

  LOG(INFO) << "upgrade: " << upgrade.value();
  LOG(INFO) << "connection: " << connection.value();
  LOG(INFO) << "secWebsocketKey: " << secWebsocketKey.value();
  LOG(INFO) << "secWebsocketVersion: " << secWebsocketVersion.value();

  std::string toEncode =
      secWebsocketKey.value() + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  unsigned char buf[SHA_DIGEST_LENGTH];  // == 20
  SHA1(reinterpret_cast<const unsigned char*>(toEncode.data()), toEncode.size(),
       buf);
  auto encoded =
      proxygen::base64Encode(folly::ByteRange(buf, SHA_DIGEST_LENGTH));

  LOG(INFO) << "Encoded: " << encoded;
  HTTPResponse response(101, folly::IOBuf::create(0),
                        {{"Upgrade", "websocket"},
                         {"Connection", "Upgrade"},
                         {"Sec-WebSocket-Accept", encoded}});
  
  this->sendResponseHeaders(std::move(response));
}

template <typename... HandlerArgs>
void WebsocketHandler<HandlerArgs...>::onRequestComplete() noexcept {
  LOG(INFO) << "WS onRequestComplete";
}

template <typename... HandlerArgs>
void WebsocketHandler<HandlerArgs...>::onUnhandledError(
    proxygen::ProxygenError err) noexcept {
  LOG(INFO) << "WS onUnhandledError";
}

template <typename... HandlerArgs>
void WebsocketHandler<HandlerArgs...>::onUpgrade(
    proxygen::UpgradeProtocol prot) noexcept {
  LOG(INFO) << "Got connection upgrade request";
}
}
