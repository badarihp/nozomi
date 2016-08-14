#pragma once

#include <array>
#include <memory>

#include <folly/io/IOBuf.h>
#include <folly/Optional.h>

namespace nozomi {
class WebsocketFrame {
 public:
  enum class OpCode : int8_t {
    ContinuationFrame = 0x0,
    TextFrame = 0x1,
    BinaryFrame = 0x2,
    ReservedNonControlFrame3 = 0x3,
    ReservedNonControlFrame4 = 0x4,
    ReservedNonControlFrame5 = 0x5,
    ReservedNonControlFrame6 = 0x6,
    ReservedNonControlFrame7 = 0x7,
    CloseConnection = 0x8,
    Ping = 0x9,
    Pong = 0xA,
    ReservedControlFrameB = 0xB,
    ReservedControlFrameC = 0xC,
    ReservedControlFrameD = 0xD,
    ReservedControlFrameE = 0xE,
    ReservedControlFrameF = 0xF,
  };

  static inline OpCode parseOpCode(int8_t code) {
    //The 4 bit op code is fully enumerated,
    //so a static cast from a single byte should
    //be fine.
    return static_cast<OpCode>(code & 0x0F);
  }

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
  std::array<int8_t, 4> maskingKey_ = {0,0,0,0};
  std::unique_ptr<folly::IOBuf> payloadData_;

 public:
  WebsocketFrame(bool fin,
                 bool rsv1,
                 bool rsv2,
                 bool rsv3,
                 OpCode opcode,
                 bool masked,
                 std::array<int8_t, 4> maskingKey,
                 std::unique_ptr<folly::IOBuf> payloadData);

  inline bool getFin() const noexcept { return fin_; }
  inline bool getRsv1() const noexcept { return rsv1_; }
  inline bool getRsv2() const noexcept { return rsv2_; }
  inline bool getRsv3() const noexcept { return rsv3_; }
  inline OpCode getOpcode() const noexcept { return opcode_; }
  inline bool getMasked() const noexcept { return masked_; }
  inline const std::array<int8_t, 4>& getMaskingKey() const noexcept { return maskingKey_; }
  inline std::unique_ptr<folly::IOBuf> getPayloadData() const noexcept {
    return payloadData_->clone();
  };
};
}
