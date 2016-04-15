#pragma once

namespace sakura {

class HTTPResponse {
  private:
    int statusCode_;

  public:
    HTTPResponse(int statusCode): statusCode_(statusCode) {}
    inline int getStatusCode() { return statusCode_; }
};

}
