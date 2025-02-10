#pragma once
#include <bit>
#include <cstdint>
#include <cstring>
#include <vector>

class ByteReader {
public:
  explicit ByteReader(std::vector<uint8_t> data)
      : data_(std::move(data)), pos_(0) {}
  ByteReader(const std::vector<uint8_t> &data, size_t offset)
      : data_(data), pos_(offset) {}

  // 8-bit reads
  uint8_t readU8() { return data_[pos_++]; }
  int8_t readI8() { return static_cast<int8_t>(readU8()); }

  // 16-bit reads
  uint16_t readU16() {
    uint16_t value;
    std::memcpy(&value, data_.data() + pos_, sizeof(uint16_t));
    pos_ += sizeof(uint16_t);
    if constexpr (std::endian::native == std::endian::little) {
      return std::byteswap(value);
    }
    return value;
  }
  int16_t readI16() { return static_cast<int16_t>(readU16()); }

  // 24-bit reads
  uint32_t readU24() {
    uint32_t value = 0;
    value |= static_cast<uint32_t>(readU8()) << 16;
    value |= static_cast<uint32_t>(readU8()) << 8;
    value |= static_cast<uint32_t>(readU8());
    return value;
  }
  int32_t readI24() {
    int32_t value = readU24();
    // Sign extend if negative
    if (value & 0x800000) {
      value |= 0xFF000000;
    }
    return value;
  }

  // 32-bit reads
  uint32_t readU32() {
    uint32_t value;
    std::memcpy(&value, data_.data() + pos_, sizeof(uint32_t));
    pos_ += sizeof(uint32_t);
    if constexpr (std::endian::native == std::endian::little) {
      return std::byteswap(value);
    }
    return value;
  }
  int32_t readI32() { return static_cast<int32_t>(readU32()); }

  // 48-bit reads
  uint64_t readU48() {
    uint64_t value = 0;
    value |= static_cast<uint64_t>(readU8()) << 40;
    value |= static_cast<uint64_t>(readU8()) << 32;
    value |= static_cast<uint64_t>(readU8()) << 24;
    value |= static_cast<uint64_t>(readU8()) << 16;
    value |= static_cast<uint64_t>(readU8()) << 8;
    value |= static_cast<uint64_t>(readU8());
    return value;
  }
  int64_t readI48() {
    int64_t value = readU48();
    // Sign extend if negative
    if (value & 0x800000000000) {
      value |= 0xFFFF000000000000;
    }
    return value;
  }

  // 64-bit reads
  uint64_t readU64() {
    uint64_t value;
    std::memcpy(&value, data_.data() + pos_, sizeof(uint64_t));
    pos_ += sizeof(uint64_t);
    if constexpr (std::endian::native == std::endian::little) {
      return std::byteswap(value);
    }
    return value;
  }
  int64_t readI64() { return static_cast<int64_t>(readU64()); }

  // Float reads
  float readFloat() {
    uint32_t bits = readU32();
    float value;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
  }

  double readDouble() {
    uint64_t bits = readU64();
    double value;
    std::memcpy(&value, &bits, sizeof(double));
    return value;
  }

  // Existing byte reading methods
  void readBytes(void *buffer, size_t length) {
    std::memcpy(buffer, data_.data() + pos_, length);
    pos_ += length;
  }

  void readBytes(void *buffer, size_t offset, size_t length) {
    std::memcpy(buffer, data_.data() + offset, length);
    pos_ = offset + length;
  }

  std::vector<uint8_t> readBytes(size_t length) {
    std::vector<uint8_t> result(length);
    readBytes(result.data(), length);
    return result;
  }

  // Varint reading
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

  uint64_t read24() {
    uint64_t value = 0;
    value |= static_cast<uint64_t>(readU8()) << 16;
    value |= static_cast<uint64_t>(readU8()) << 8;
    value |= readU8();
    return value;
  }

  uint64_t read48() {
    uint64_t value = 0;
    value |= static_cast<uint64_t>(readU8()) << 40;
    value |= static_cast<uint64_t>(readU8()) << 32;
    value |= static_cast<uint64_t>(readU8()) << 24;
    value |= static_cast<uint64_t>(readU8()) << 16;
    value |= static_cast<uint64_t>(readU8()) << 8;
    value |= readU8();
    return value;
  }

  // Position management
  void seek(size_t pos) { pos_ = pos; }
  size_t position() const { return pos_; }
  void skip(size_t bytes) { pos_ += bytes; }
  bool eof() const { return pos_ >= data_.size(); }
  size_t remaining() const { return data_.size() - pos_; }

  // Data access
  const std::vector<uint8_t> &getData() const { return data_; }
  const uint8_t *data() const { return data_.data(); }
  size_t size() const { return data_.size(); }

private:
  std::vector<uint8_t> data_;
  size_t pos_;
};
