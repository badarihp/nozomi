#include "src/WebsocketFrame.h"

#include <arpa/inet.h>
#include <stdexcept>

namespace nozomi {
/*
WebsocketFrame::WebsocketFrame(bool fin,
                               bool rsv1,
                               bool rsv2,
                               bool rsv3,
                               OpCode opcode,
                               bool masked,
                               int32_t maskingKey,
                               std::unique_ptr<folly::IOBuf> payloadData)
    : rsv1_(rsv1),
      rsv2_(rsv2),
      rsv3_(rsv3),
      opcode_(opcode),
      masked_(masked),
      maskingKey_(maskingKey),
      payloadData_(std::move(payloadData)) {}

std::unique_ptr<folly::IOBuf> WebsocketFrame::toBinary() const {}
*/

std::unique_ptr<folly::IOBuf> WebsocketFrameParser::parseSingleBuffer(std::unique_ptr<folly::IOBuf> originalBuffer) {
  if(complete) { return std::move(originalBuffer); }

    int64_t chainOffset = 0;
    auto* data = originalBuffer.data();
    auto length = originalBuffer.length();

    if(length == 0) { return std::unique_ptr<folly::IOBuf>(); }

    if(totalOffset == 0) {
      fin = data[chainOffset] & 0x80 != 0;
      rsv1 = data[chainOffset] & 0x40 != 0; 
      rsv2 = data[chainOffset] & 0x20 != 0; 
      rsv3 = data[chainOffset] & 0x10 != 0; 
      //TODO: Convert this to the enum
      opcode = ntohl(data[chainOffset] & 0x0F);
      chainOffset += 1;
      totalOffset += 1;
      if(chainOffset >= length) { return std::unique_ptr<folly::IOBuf>(); }
    } 

    if(totalOffset == 1) {
      if(data[chainOffset] < 0) {
        masked = true;
      }
      payloadLength = static_cast<int64_t>(data[chainOffset] & 0x7F /* Bottom 7 bits */);
      if(payloadLength == 126) {
        extraLengthBytes = 2;
      } else if(payloadLength == 127) {
        extraLengthBytes = 4;
      }
      chainOffset += 1;
      totalOffset += 1;
      if(chainOffset >= length) { return std::unique_ptr<folly::IOBuf>(); }
    }

    if(totalOffset < 2 + extraLengthBytes && totalOffset >= 2 ) {
      int i = totalOffset - 2;
      for(; i < extraLengthBytes && chainOffset < length; i++, totalOffset++, chainOffset++) {
        extraLengthBuffer[i] = data[chainOffset];
      }

      if(totalOffset == 2 + extraLengthBytes) {
        if(extraLengthBytes == 2) {
          payloadLength = ntohs(*reinterpret_cast<int16_t*>(extraLengthBuffer));
          if(payloadLength < 0) {
            //TODO: better error
            return std::unique_ptr<folly::IOBuf>();
          } 
        } else if(extraLengthBytes == 8) {
          if(payloadLength < 0) {
            //TODO: better error
            return std::unique_ptr<folly::IOBuf>();
          } 
          payloadLength = be64toh(*reinterpret_cast<int64_t*>(extraLengthBuffer));
        }
      }

      if(chainOffset >= length) { return std::unique_ptr<folly::IOBuf>(); }
    }

    if(masked && totalOffset < 2 + extraLengthBytes +  4 && totalOffset >= 2 + extraLengthBytes ) {
      
      for(int i = totalOffset - 2 - extraLengthBytes; i < 4 && chainOffset < length; i++, totalOffset++, chainOffset++) {
        maskingKey[i] = data[chainOffset];
      }
      if(chainOffset >= length) { return std::unique_ptr<folly::IOBuf>(); }
    }

    //TODO: Unshare
    auto totalLength = 2 + extraLengthBytes + (masked ? 4 : 0) + payloadLength; 
    if(totalOffset < totalLength) {
      auto newBody = originalBuffer->cloneOne(); 
      if(masked) { 
        for(auto payloadOffset = totalOffset - 4 - extraLengthBytes - 2; totalOffset < totalLength && chainOffset < length; totalOffset++, chainOffset++) {
          data[chainOffset] = data[chainOffset] ^ maskingKey[payloadOffset % 4];
        }
      }

      newBody->advance(chainOffset);
    payloadData.prependChain();
 
      if(totalOffset >= totalLength) {
        complete = true;
      }
      if(chainOffset < length) {
        //Keep a clone that has the entire original chain so that we can just return the result of this method if the entire buffer is not consumed.
        auto ret = originalBuffer->clone();
        ret->advance(chainOffset);
      } else {
        return std::unique_ptr<folly::IOBuf>();
      }
    } 


}

std::unique_ptr<folly::IOBuf> WebsocketFrameParser::operator <<(std::unique_ptr<folly::IOBuf> data) {
  for(const auto& buf: *data) {
    auto remainder = parseSingleBuffer(buf->clone());
    if(remainder) {
      return std::move(remainder);
    }
  }
  return folly::IOBuf::create(0);
}
/*
WebsocketMessageBuilder::WebsocketMessageBuilder() {}

WebsocketMessageBuilder& WebsocketMessageBuilder::addFrame(
    WebsocketFrame frame) {
  return *this;
}
*/
}
