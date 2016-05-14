#include "src/Config.h"

namespace nozomi {

void Config::setHTTPAddresses(
    std::vector<proxygen::HTTPServer::IPConfig> HTTPAddresses) {
  if (HTTPAddresses.size() == 0) {
    throw std::invalid_argument("At least one address must be provided");
  }
  HTTPAddresses_ = std::move(HTTPAddresses);
}

void Config::setWorkerThreads(size_t workerThreads) {
  if (workerThreads == 0) {
    throw std::invalid_argument(folly::sformat(
        "Number of threads ({}) must be greater than zero", workerThreads));
  }
  workerThreads_ = workerThreads;
}

Config::Config(
    std::vector<std::tuple<std::string, uint16_t, Protocol>> HTTPAddresses,
    size_t workerThreads) {
  std::string host;
  uint16_t port;
  Protocol protocol;

  std::vector<proxygen::HTTPServer::IPConfig> newHTTPAddresses;
  newHTTPAddresses.reserve(HTTPAddresses.size());
  for (const auto& tup : HTTPAddresses) {
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
}

Config::Config(std::vector<proxygen::HTTPServer::IPConfig> HTTPAddresses,
               size_t workerThreads) {
  setHTTPAddresses(std::move(HTTPAddresses));
  setWorkerThreads(workerThreads);
}
}
