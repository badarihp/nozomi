#pragma once

#include <memory>

#include <folly/io/IOBuf.h>

#include "src/WebsocketFrame.h"

namespace nozomi {

class WebsocketFrameParser {
  private:
    bool fin = false;
    bool rsv1 = false;
    bool rsv2 = false;
    bool rsv3 = false;
    WebsocketFrame::OpCode opcode = WebsocketFrame::OpCode::ContinuationFrame;
    bool masked = false;
    std::array<int8_t, 4> maskingKey = {0,0,0,0};
    int64_t payloadLength = 0;
    int64_t totalOffset = 0;
    int8_t extraLengthBytes = 0;
    std::array<int8_t, 8> extraLengthBuffer = {0,0,0,0,0,0,0,0};
    bool complete = false;
    bool error = false;

    void reset() {
    fin = false;
    rsv1 = false;
    rsv2 = false;
    rsv3 = false;
    opcode = WebsocketFrame::OpCode::ContinuationFrame;
    masked = false;
    maskingKey = {0,0,0,0};
    payloadLength = 0;
    totalOffset = 0;
    extraLengthBytes = 0;
    extraLengthBuffer = {0,0,0,0,0,0,0,0};
    complete = false;
    error = false;
    }

    std::unique_ptr<folly::IOBuf> payloadData;
    
    uint64_t parseSingleBuffer(folly::IOBuf* data);

  public:
    WebsocketFrameParser(): payloadData(folly::IOBuf::create(0)) {}
    std::unique_ptr<folly::IOBuf> operator <<(std::unique_ptr<folly::IOBuf> data);
    inline bool isComplete() { return totalOffset >= 2 + extraLengthBytes + (masked ? 4 : 0) + payloadLength; }
    folly::Optional<WebsocketFrame> getFrame();
};


}
