#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <folly/Format.h>
#include <folly/SocketAddress.h>
#include <proxygen/httpserver/HTTPServer.h>

namespace nozomi {

class Config {
 private:
  std::vector<proxygen::HTTPServer::IPConfig> HTTPAddresses_;
  size_t workerThreads_;

  void setHTTPAddresses(
      std::vector<proxygen::HTTPServer::IPConfig> HTTPAddresses);
  void setWorkerThreads(size_t workerThreads);

 public:
  using Protocol = proxygen::HTTPServer::Protocol;

  Config(std::vector<std::tuple<std::string, uint16_t, Protocol>> HTTPAddresses,
         size_t workerThreads);
  Config(std::vector<proxygen::HTTPServer::IPConfig> HTTPAddresses,
         size_t workerThreads);

  inline size_t getWorkerThreads() const { return workerThreads_; }
  inline std::vector<proxygen::HTTPServer::IPConfig> getHTTPAddresses() const {
    return HTTPAddresses_;
  };
};
}
