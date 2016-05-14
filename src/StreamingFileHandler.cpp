#include "src/StreamingFileHandler.h"

#include <glog/logging.h>

namespace nozomi {
void StreamingFileHandler::onRequest(const HTTPRequest& request) noexcept {
  LOG(INFO) << "onRequest";
}

void StreamingFileHandler::setRequestArgs(int64_t i) noexcept {
  LOG(INFO) << "setRequestArgs " << i;
}

void StreamingFileHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {
  LOG(INFO) << "onBody";
}

void StreamingFileHandler::onEOM() noexcept {
  LOG(INFO) << "onEOM";
  // TODO: When sending the file, make sure that we implement 304
  sendResponseHeaders(HTTPResponse(500, "Not implemented"));
  sendEOF();
}

void StreamingFileHandler::onRequestComplete() noexcept {
  LOG(INFO) << "onRequestComplete";
}

void StreamingFileHandler::onUnhandledError(
    proxygen::ProxygenError err) noexcept {
  LOG(INFO) << "onUnhandledError";
}
}
