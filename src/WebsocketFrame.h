#pragma once

#include <memory>

#include <folly/io/IOBuf.h>
#include <folly/Optional.h>

namespace nozomi {
/*
class WebsocketFrame {
 public:
  enum class OpCode : int32_t {
    ContinuationFrame = 0x00,
    TextFrame = 0x01,
    BinaryFrame = 0x02,
    ReservedNonControlFrame3 = 0x03,
    ReservedNonControlFrame4 = 0x04,
    ReservedNonControlFrame5 = 0x05,
    ReservedNonControlFrame6 = 0x06,
    ReservedNonControlFrame7 = 0x07,
    CloseConnection = 0x08,
    Ping = 0x09,
    Pong = 0xA0,
    ReservedControlFrameB = 0xB0,
    ReservedControlFrameC = 0xC0,
    ReservedControlFrameD = 0xD0,
    ReservedControlFrameE = 0xE0,
    ReservedControlFrameF = 0xF0,
  };

  enum class Source : bool {
    Client,
    Server,
  };

  // TODO: Make sure we're only dong big endian conversions
 private:
  bool fin_ = 0;
  bool rsv1_ = 0;  // Don't suppot extensions right now, so if 1,2,3 are 0, fail
  bool rsv2_ = 0;
  bool rsv3_ = 0;
  OpCode opcode_ = OpCode::ContinuationFrame;
  bool masked_ = false;
  int32_t maskingKey_ = 0;
  std::unique_ptr<folly::IOBuf> payloadData_;

 public:
  WebsocketFrame(bool fin,
                 bool rsv1,
                 bool rsv2,
                 bool rsv3,
                 OpCode opcode,
                 bool masked,
                 int32_t maskingKey,
                 std::unique_ptr<folly::IOBuf> payloadData);
  std::unique_ptr<folly::IOBuf> toBinary() const;
  bool addNextFrame(std::unique_ptr<folly::IOBuf> payloadData);
  static folly::Optional<WebsocketFrame> parse(
      std::unique_ptr<folly::IOBuf> rawData, Source source);

  inline bool getFin() const noexcept { return fin_; }
  inline bool getRsv1() const noexcept { return rsv1_; }
  inline bool getRsv2() const noexcept { return rsv2_; }
  inline bool getRsv3() const noexcept { return rsv3_; }
  inline OpCode getOpcode() const noexcept { return opcode_; }
  inline bool getMasked() const noexcept { return masked_; }
  inline int32_t getMaskingKey() const noexcept { return maskingKey_; }
  inline std::unique_ptr<folly::IOBuf> getPayloadData() const noexcept {
    return payloadData_;
  };
};
*/
class WebsocketFrameParser {
  private:
    bool fin = false;
    bool rsv1 = false;
    bool rsv2 = false;
    bool rsv3 = false;
    int32_t opcode = 0;
    bool masked = false;
    int8_t maskingKey[4] = {};
    int64_t payloadLength = 0;
    int64_t totalOffset = 0;
    int8_t extraLengthBytes = 0;
    int8_t extraLengthBuffer[8] = {};
    bool complete = false;
    bool error = false;

    std::unique_ptr<folly::IOBuf> payloadData;
    
    std::unique_ptr<folly::IOBuf> parseSingleBuffer(std::unique_ptr<folly::IOBuf> data);

  public:
    WebsocketFrameParser(): payloadData(folly::IOBuf::create(0)) {}
    operator std::unique_ptr<folly::IOBuf> <<(std::unique_ptr<folly::IOBuf> data);
    bool isComplete();
    folly::Optional<WebsocketFrame> getFrame();
}
/*
class WebsocketMessageBuilder {
  private:
    std::vector<WebsocketFrame> frames;
    unique_ptr<folly::IOBuf> message;
  public:
  WebsocketMessageBuilder();
  WebsocketMessageBuilder& addFrame(WebsocketFrame frame); 
};
*/
}
