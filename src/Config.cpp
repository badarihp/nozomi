#include "src/Config.h"

namespace fs = boost::filesystem;

namespace nozomi {

const size_t Config::kDefaultFileReaderBufferSize;
const int64_t Config::kDefaultRequestTimeoutMs;

void Config::setHTTPAddresses(
    std::vector<proxygen::HTTPServer::IPConfig> httpAddresses) {
  if (httpAddresses.size() == 0) {
    throw std::invalid_argument("At least one address must be provided");
  }
  httpAddresses_ = std::move(httpAddresses);
}

void Config::setWorkerThreads(size_t workerThreads) {
  if (workerThreads == 0) {
    throw std::invalid_argument(folly::sformat(
        "Number of threads ({}) must be greater than zero", workerThreads));
  }
  workerThreads_ = workerThreads;
}

void Config::setRequestTimeout(std::chrono::milliseconds requestTimeout) {
  if (requestTimeout.count() <= 0) {
    throw std::invalid_argument(
        folly::sformat("Timeout ({}) must be greater than zero milliseconds",
                       requestTimeout.count()));
  }
  requestTimeout_ = requestTimeout;
}

void Config::setFileReaderBufferSize(size_t size) {
  if (size == 0 || size > 1024 * 1024 * 1024) {
    throw std::invalid_argument(folly::sformat(
        "The file reader buffer size ({}) must be > 0 and less than 1GB",
        size));
  }
  fileReaderBufferSize_ = size;
}

void Config::setPublicDir(const folly::Optional<std::string>& path) {
  if (!path) {
    addPublicDirectoryHandler_ = false;
    publicDir_ = "";
    return;
  }

  try {
    fs::path fullPath = fs::canonical(fs::path(path.value()));
    if ((fs::is_symlink(fullPath) &&
         !fs::is_directory(fs::read_symlink(fullPath))) ||
        !fs::is_directory(fullPath)) {
      throw std::invalid_argument("Path must be a directory");
    }
    addPublicDirectoryHandler_ = true;
    publicDir_ = fullPath;
  } catch (const std::exception& e) {
    throw std::invalid_argument(folly::sformat(
        "Could not use public directory {}: {}", path.value(), e.what()));
  }
}

Config::Config(
    std::vector<std::tuple<std::string, uint16_t, Protocol>> httpAddresses,
    size_t workerThreads,
    folly::Optional<std::string> publicDir,
    std::chrono::milliseconds requestTimeout,
    size_t fileReaderBufferSize) {
  std::string host;
  uint16_t port;
  Protocol protocol;

  std::vector<proxygen::HTTPServer::IPConfig> newHTTPAddresses;
  newHTTPAddresses.reserve(httpAddresses.size());
  for (const auto& tup : httpAddresses) {
    std::tie(host, port, protocol) = tup;

    try {
      newHTTPAddresses.emplace_back(folly::SocketAddress{std::move(host), port},
                                    protocol);
    } catch (const std::system_error& e) {
      throw std::invalid_argument(e.what());
    }
  }

  setHTTPAddresses(std::move(newHTTPAddresses));
  setWorkerThreads(workerThreads);
  setPublicDir(publicDir);
  setRequestTimeout(requestTimeout);
  setFileReaderBufferSize(fileReaderBufferSize);
}

Config::Config(std::vector<proxygen::HTTPServer::IPConfig> httpAddresses,
               size_t workerThreads,
               folly::Optional<std::string> publicDir,
               std::chrono::milliseconds requestTimeout,
               size_t fileReaderBufferSize) {
  setHTTPAddresses(std::move(httpAddresses));
  setWorkerThreads(workerThreads);
  setPublicDir(publicDir);
  setRequestTimeout(requestTimeout);
  setFileReaderBufferSize(fileReaderBufferSize);
}
}
