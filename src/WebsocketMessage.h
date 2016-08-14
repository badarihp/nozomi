#pragma once

namespace nozomi {

class WebsocketMessage {
  struct CloseReason {
    uint16_t code;
    std::string reason;
  };


 private:
  std::vector<WebsocketFrame> frames_;
  std::unique_ptr<folly::IOBuf> combinedData_;

 public:
  WebsocketMessage(std::vector<WebsocketFrame> frames)
      : frames_(std::move(frames)), combinedData_(folly::IOBuf::create(0)) {
    for (const auto& frame : frames) {
      combinedData_->prependChain(frame->getPayloadData());
    }
  }

  std::unique_ptr<folly::IOBuf> getBody();
  folly::Optional<CloseReason> getCloseReason();
};
}
