#pragma once

#include <string>

#include <boost/filesystem.hpp>
#include <wangle/concurrent/GlobalExecutor.h>

#include "src/Config.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/StreamingHTTPHandler.h"

namespace nozomi {
class StreamingFileHandler : public StreamingHTTPHandler<std::string> {
 private:
  boost::filesystem::path path_;
  std::time_t ifModifiedSince_ = 0;
  size_t readBufferSize_;
  folly::Executor* ioExecutor_;
  std::string rawPath_;

 public:
  StreamingFileHandler(
      boost::filesystem::path basePath,
      size_t readBufferSize = Config::kDefaultFileReaderBufferSize,
      folly::Executor* ioExecutor = wangle::getIOExecutor().get(),
      folly::EventBase* socketEvb = nullptr)
      : StreamingHTTPHandler(socketEvb),
        path_(std::move(basePath)),
        readBufferSize_(readBufferSize),
        ioExecutor_(ioExecutor) {
    DCHECK(ioExecutor != nullptr);
  }
  virtual ~StreamingFileHandler() {}
  virtual void onRequestReceived(const HTTPRequest& request) noexcept override;
  virtual void setRequestArgs(std::string path) noexcept override;
  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
  virtual void onEOM() noexcept override;
  virtual void onRequestComplete() noexcept override;
  virtual void onUnhandledError(proxygen::ProxygenError err) noexcept override;

  static boost::filesystem::path sanitizePath(const std::string& path);
};
}
