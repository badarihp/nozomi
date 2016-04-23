#pragma once

#include <memory>
#include <string>

#include <folly/io/IOBuf.h>

namespace sakura {
inline std::string to_string(const std::unique_ptr<folly::IOBuf>& buffer) {
  std::string ret;
  ret.reserve(buffer->computeChainDataLength());
  auto offset = 0;
  for (const auto& buf : *buffer) {
    auto len = buf.size();
    ret.insert(offset, reinterpret_cast<const char*>(buf.data()), len);
    offset += len;
  }
  return ret;
}
}
