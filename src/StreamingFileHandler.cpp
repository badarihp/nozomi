#include "src/StreamingFileHandler.h"

#include <glog/logging.h>

namespace nozomi {
void StreamingFileHandler::onRequest(const HTTPRequest& request) noexcept {
  // . and .. are filtered out by proxygen
  LOG(INFO) << "onRequest";
  path_ = basePath_ / boost::filesystem::path(request.getPath());

  auto ifModifiedSinceStr =  request.getHeaders[HTTP_HEADER_IF_MODIFIED_SINCE];
  if(ifModifiedSinceStr) {
    struct tm expireTime;
    //TDOO: Locale will probably screw with the date parsing
    if(strptime((*ifModifiedSinceStr).c_str(), "%a, %d %b %Y %T", &expireTime) != nullptr) {
      ifModifiedSince_ = mktime(expireTime);
    }
      
  }
}

void StreamingFileHandler::setRequestArgs() noexcept {
  LOG(INFO) << "setRequestArgs ";
}

void StreamingFileHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {
  LOG(INFO) << "onBody";
}

void StreamingFileHandler::onEOM() noexcept {
  LOG(INFO) << "onEOM";
  // TODO: When sending the file, make sure that we implement 304
  if(!boost::filesystem::is_regular_file(path)) {
    sendResponseHeaders(HTTPResponse(404));
  } else {
    boost::system::error_code ec;
    time_t lastModifiedTime = boost::filesystem::last_write_time(path_, ec);
    if(!ec && ifModifiedSince_ > 0 && ifModifiedSince_ >= lastModifiedTime) {
      sendResponseHeaders(HTTPResponse(304));
    } else {
      sendResponseHeaders(HTTPResponse(500, "Not implemented"));
    }
  }

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
