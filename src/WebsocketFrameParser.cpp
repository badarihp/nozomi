#include "src/WebsocketFrameParser.h"

namespace nozomi {

uint64_t WebsocketFrameParser::parseSingleBuffer(folly::IOBuf* originalBuffer) {
  if (isComplete()) {
    LOG(INFO) << "Complete";
    return 0;
  }

  uint64_t chainOffset = 0;
  auto* data = originalBuffer->writableData();
  auto length = originalBuffer->length();
  LOG(INFO) << "Length: " << length;
  if (length == 0) {
    LOG(INFO) << "Length 0";
    return 0;
  }

  if (totalOffset == 0) {
    fin = (data[chainOffset] & 0x80) != 0;
    rsv1 = (data[chainOffset] & 0x40) != 0;
    rsv2 = (data[chainOffset] & 0x20) != 0;
    rsv3 = (data[chainOffset] & 0x10) != 0;
    opcode = WebsocketFrame::parseOpCode(data[chainOffset]);
    chainOffset += 1;
    totalOffset += 1;
    if (chainOffset >= length) {
      LOG(INFO) << "Finished after bits";
      return length;
    }
  }
  
  if (totalOffset == 1) {
    if (data[chainOffset] >= 128) {
      masked = true;
    }
    LOG(INFO) << "Mask byte: " << (long)data[chainOffset];
    payloadLength =
        static_cast<int64_t>(data[chainOffset] & 0x7F /* Bottom 7 bits */);
    if (payloadLength == 126) {
      extraLengthBytes = 2;
    } else if (payloadLength == 127) {
      extraLengthBytes = 4;
    }
    chainOffset += 1;
    totalOffset += 1;
    if (chainOffset >= length) {
      LOG(INFO) << "Finished after first offset";
      return length;
    }
  }

  if (totalOffset < 2 + extraLengthBytes && totalOffset >= 2) {
    int i = totalOffset - 2;
    for (; i < extraLengthBytes && chainOffset < length;
         i++, totalOffset++, chainOffset++) {
      extraLengthBuffer[i] = data[chainOffset];
    }

    if (totalOffset == 2 + extraLengthBytes) {
      if (extraLengthBytes == 2) {
        payloadLength = ntohs(*reinterpret_cast<int16_t*>(extraLengthBuffer.data()));
        if (payloadLength < 0) {
          // TODO: better error. Throw?
          LOG(INFO) << "Negative payloadLength";
          return chainOffset;
        }
      } else if (extraLengthBytes == 8) {
        if (payloadLength < 0) {
          // TODO: better error. Throw?
          LOG(INFO) << "Negative 8 byte payloadLength";
          return chainOffset;
        }
        payloadLength = be64toh(*reinterpret_cast<int64_t*>(extraLengthBuffer.data()));
      }
    }

    if (chainOffset >= length) {
      LOG(INFO) << "Finished after payloadLength";
      return length;
    }
  }

  LOG(INFO) << "Masked? " << masked << " Total offset: " << totalOffset << " lengthbytes " << extraLengthBytes;
  if (masked && totalOffset < 2 + extraLengthBytes + 4 &&
      totalOffset >= 2 + extraLengthBytes) {
    for (int i = totalOffset - 2 - extraLengthBytes;
         i < 4 && chainOffset < length; i++, totalOffset++, chainOffset++) {
      maskingKey[i] = data[chainOffset];
    }
    if (chainOffset >= length) {
      LOG(INFO) << "Finished during payload";
      return length;
    }
  }

  // TODO: Unshare
  auto totalLength = 2 + extraLengthBytes + (masked ? 4 : 0) + payloadLength;
  if (totalOffset < totalLength) {
    auto newBody = originalBuffer->cloneOne();
    if (masked) {
      for (auto payloadOffset = totalOffset - 4 - extraLengthBytes - 2;
           totalOffset < totalLength && chainOffset < length;
           totalOffset++, chainOffset++) {
        data[chainOffset] = data[chainOffset] ^ maskingKey[payloadOffset % 4];
      }
    }

    newBody->trimEnd(length - 1 - chainOffset);
    payloadData->prependChain(std::move(newBody));
  }
  if (totalOffset >= totalLength) {
    LOG(INFO) << "Complete now true";
  }
  LOG(INFO) << "Returning offset" << chainOffset;
  return chainOffset;
}

folly::Optional<WebsocketFrame> WebsocketFrameParser::getFrame() {
  folly::Optional<WebsocketFrame> ret;
  if (isComplete()) {
    ret.emplace(WebsocketFrame(fin, rsv1, rsv2, rsv3, opcode, masked, maskingKey, std::move(payloadData)));
    reset(); 
  }
  return ret;
}

std::unique_ptr<folly::IOBuf> WebsocketFrameParser::operator<<(
    std::unique_ptr<folly::IOBuf> data) {
  folly::IOBuf* current = data.get();
  //TODO: Handle malformed packets. If it loops too many times, kill the parser
  while (current != nullptr) {
    auto consumed = parseSingleBuffer(current);
    LOG(INFO) << "Consumed: " << consumed;
    if (consumed == 0) {
      break;
    }
    LOG(INFO) << "Length before trimStart: " << current->length();
    current->trimStart(consumed);
    LOG(INFO) << "Length after trimStart: " << current->length();
    current = current->next();
  }
  return data;
}

}
