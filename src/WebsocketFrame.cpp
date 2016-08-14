#include "src/WebsocketFrame.h"

#include <arpa/inet.h>
#include <stdexcept>

#include "src/StringUtils.h"

namespace nozomi {
WebsocketFrame::WebsocketFrame(bool fin,
                               bool rsv1,
                               bool rsv2,
                               bool rsv3,
                               OpCode opcode,
                               bool masked,
                               std::array<int8_t, 4> maskingKey,
                               std::unique_ptr<folly::IOBuf> payloadData)
    : fin_(fin),
      rsv1_(rsv1),
      rsv2_(rsv2),
      rsv3_(rsv3),
      opcode_(opcode),
      masked_(masked),
      maskingKey_(std::move(maskingKey)),
      payloadData_(std::move(payloadData)) {}

folly::Optional<WebsocketFrame::CloseReason> WebsocketFrame::getCloseReason() {
  folly::Optional<WebsocketFrame::CloseReason> ret;
  if(opcode_ == OpCode::CloseConnection && payloadData_->length() >= 2) {
    uint16_t code = ntohs(*reinterpret_cast<const uint16_t*>(payloadData_->data()));
    ret.emplace(WebsocketFrame::CloseReason { code, to_string(payloadData_, 2) }); 
  }
  return ret;
}


/*
WebsocketMessageBuilder::WebsocketMessageBuilder() {}

WebsocketMessageBuilder& WebsocketMessageBuilder::addFrame(
    WebsocketFrame frame) {
  return *this;
}
*/
}
