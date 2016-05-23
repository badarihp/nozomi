#include "src/StreamingFileHandler.h"

#include <time.h>

#include <iostream>

#include <glog/logging.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>

namespace nozomi {

boost::filesystem::path StreamingFileHandler::sanitizePath(
    const std::string& path) {
  boost::filesystem::path rawPath(path);
  boost::filesystem::path ret;

  for (const auto& component : rawPath) {
    if (component == "..") {
      ret.remove_filename();
    } else if (component == ".") {
      continue;
    } else {
      ret /= component;
    }
  }
  return ret;
}

void StreamingFileHandler::onRequestReceived(
    const HTTPRequest& request) noexcept {
  // . and .. are filtered out by proxygen
  LOG(INFO) << "onRequestReceived";
  path_ /= sanitizePath(rawPath_);

  auto ifModifiedSinceStr =
      request.getHeaders()
          [proxygen::HTTPHeaderCode::HTTP_HEADER_IF_MODIFIED_SINCE];
  if (ifModifiedSinceStr) {
    struct tm expireTime;
    // TDOO: Locale will probably screw with the date parsing
    if (strptime((*ifModifiedSinceStr).c_str(), "%a, %d %b %Y %T",
                 &expireTime) != nullptr) {
      ifModifiedSince_ = mktime(&expireTime);
    }
  }
}

void StreamingFileHandler::setRequestArgs(std::string path) noexcept {
  LOG(INFO) << "setRequestArgs ";
  rawPath_ = std::move(path);
}

void StreamingFileHandler::onBody(std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(INFO) << "onBody";
}

void StreamingFileHandler::onEOM() noexcept {
  LOG(INFO) << "onEOM";
  if (!boost::filesystem::is_regular_file(path_)) {
    sendResponseHeaders(HTTPResponse(404));
    sendEOF();
  } else {
    boost::system::error_code ec;
    time_t lastModifiedTime = boost::filesystem::last_write_time(path_, ec);
    if (!ec && ifModifiedSince_ > 0 && ifModifiedSince_ >= lastModifiedTime) {
      sendResponseHeaders(HTTPResponse(304));
      sendEOF();
    } else {
      via(ioExecutor_,
          [this]() {
            std::ifstream fin(path_.string());
            if (!fin.good()) {
              sendResponseHeaders(HTTPResponse(404));
              return;
            }

            sendResponseHeaders(HTTPResponse(200));
            while (!fin.eof()) {
              auto buf = folly::IOBuf::create(readBufferSize_);
              if (buf->length() < readBufferSize_) {
                // IOBuf::create() always seems to set the tailroom == capacity,
                // length() == 0
                buf->append(buf->tailroom());
              }
              fin.read(reinterpret_cast<char*>(buf->writableData()),
                       readBufferSize_);
              auto readBytes = fin.gcount();
              DCHECK(readBytes >= 0);
              if (static_cast<size_t>(readBytes) < readBufferSize_) {
                LOG(INFO) << "Resized buffer from " << readBufferSize_ << " to "
                          << readBytes << ". Trimming "
                          << readBufferSize_ - readBytes << " bytes"
                          << " from buffer of size " << buf->length();
                buf->trimEnd(readBufferSize_ - readBytes);
              }
              sendBody(std::move(buf));
            }
          })
          .onError([this](const std::exception& e) {
            LOG(INFO) << "Error reading " << path_.string() << ": " << e.what();
          })
          .then([this]() { sendEOF(); });
    }
  }
}

void StreamingFileHandler::onRequestComplete() noexcept {
  LOG(INFO) << "onRequestComplete";
}

void StreamingFileHandler::onUnhandledError(
    proxygen::ProxygenError) noexcept {
  LOG(INFO) << "onUnhandledError";
}
}
