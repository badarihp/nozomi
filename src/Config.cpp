#include "src/Config.h"

namespace nozomi {

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

Config::Config(
    std::vector<std::tuple<std::string, uint16_t, Protocol>> httpAddresses,
    size_t workerThreads) {
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
}

Config::Config(std::vector<proxygen::HTTPServer::IPConfig> httpAddresses,
               size_t workerThreads) {
  setHTTPAddresses(std::move(httpAddresses));
  setWorkerThreads(workerThreads);
}
}
