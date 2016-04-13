#pragma once

#include <string>

namespace sakura {

class HTTPRequest {
  private:
    std::string path_;
    std::string method_;

  public:
    //TODO: Proxygen all the things
    HTTPRequest(std::string path, std::string method): path_(std::move(path)), method_(std::move(method)) {}
    inline const std::string& getPath() const { return path_; }
    inline const std::string& getMethod() const { return method_; }
};

}
