#pragma once

#include <memory>
#include <string>

#include <folly/io/IOBuf.h>

namespace nozomi {
inline std::string to_string(const std::unique_ptr<folly::IOBuf>& buffer, uint64_t initialOffset = 0) {
  std::string ret;
  uint64_t totalLength = buffer->computeChainDataLength();
  if(initialOffset > totalLength) {
    throw std::runtime_error("Invalid initial offset");
  }
  ret.reserve(buffer->computeChainDataLength());
  auto offset = 0;
  for (const auto& buf : *buffer) {
    auto len = buf.size();
    if(initialOffset) {
      uint64_t delta = std::min(initialOffset, len);
      ret.insert(offset, reinterpret_cast<const char*>(buf.data()) + delta, len - delta);
      initialOffset -= delta;
    } else {
      ret.insert(offset, reinterpret_cast<const char*>(buf.data()), len);
    }
    offset += len;
  }
  return ret;
}
}
