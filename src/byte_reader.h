#pragma once
#include <bit>
#include <cstdint>
#include <cstring>
#include <vector>

class ByteReader {
public:
  explicit ByteReader(std::vector<uint8_t> data) : data_(data), pos_(0) {}

  ByteReader(const std::vector<uint8_t> &data, size_t offset)
      : data_(data), pos_(offset) {}

  uint8_t readU8() { return data_[pos_++]; }

  uint16_t readU16() {
    uint16_t value = *reinterpret_cast<uint16_t *>(data_.data() + pos_);
    pos_ += sizeof(uint16_t);
    if constexpr (std::endian::native == std::endian::little) {
      return std::byteswap(value);
    }
    return value;
  }

  uint32_t readU32() {
    uint32_t value = *reinterpret_cast<uint32_t *>(data_.data() + pos_);
    pos_ += sizeof(uint32_t);
    if constexpr (std::endian::native == std::endian::little) {
      return std::byteswap(value);
    }
    return value;
  }

  // Add readBytes method with three variants:

  // 1. Read to pre-allocated buffer at current position
  void readBytes(void *buffer, size_t length) {
    std::memcpy(buffer, data_.data() + pos_, length);
    pos_ += length;
  }

  // 2. Read to pre-allocated buffer at specific offset
  void readBytes(void *buffer, size_t offset, size_t length) {
    std::memcpy(buffer, data_.data() + offset, length);
    pos_ = offset + length;
  }

  // 3. Read bytes and return as vector
  std::vector<uint8_t> readBytes(size_t length) {
    std::vector<uint8_t> result(length);
    readBytes(result.data(), length);
    return result;
  }

  std::pair<int64_t, size_t> readVarint() {
    int64_t value = 0;
    size_t bytes = 0;

    while (bytes < 9) {
      uint8_t byte = readU8();
      bytes++;

      if (bytes == 9) {
        value = (value << 8) | byte;
      } else {
        value = (value << 7) | (byte & 0x7F);
        if ((byte & 0x80) == 0)
          break;
      }
    }

    return {value, bytes};
  }

  void seek(size_t pos) { pos_ = pos; }
  size_t position() const { return pos_; }
  const std::vector<uint8_t> &getData() const { return data_; }
  const uint8_t *data() const { return data_.data(); }
  size_t size() const { return data_.size(); }

private:
  std::vector<uint8_t> data_;
  size_t pos_;
};
