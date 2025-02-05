#pragma once
#include "sqlite_constants.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

class FileReader {
public:
  explicit FileReader(const std::string &filename)
      : file_(filename, std::ios::binary) {
    if (!file_.is_open()) {
      throw std::runtime_error("Failed to open file: " + filename);
    }

    // Get file size
    file_.seekg(0, std::ios::end);
    size_ = file_.tellg();
    file_.seekg(0, std::ios::beg);
  }

  // Read methods for different integer types
  [[nodiscard]] auto readU8() -> uint8_t {
    uint8_t value;
    file_.read(reinterpret_cast<char *>(&value), sizeof(value));
    return value;
  }

  [[nodiscard]] auto readU16() -> uint16_t {
    uint16_t value;
    file_.read(reinterpret_cast<char *>(&value), sizeof(value));
    return toBigEndian(value);
  }

  [[nodiscard]] auto readU32() -> uint32_t {
    uint32_t value;
    file_.read(reinterpret_cast<char *>(&value), sizeof(value));
    return toBigEndian(value);
  }

  // Read variable-length integer
  [[nodiscard]] auto readVarint() -> std::pair<int64_t, size_t> {
    int64_t value = 0;
    size_t bytes_read = 0;

    while (bytes_read < 9) {
      uint8_t byte = readU8();
      bytes_read++;

      if (bytes_read == 9) {
        value = (value << 8) | byte;
      } else {
        value = (value << 7) | (byte & 0x7F);
        if ((byte & 0x80) == 0)
          break;
      }
    }

    return {value, bytes_read};
  }

  // Read bytes into buffer
  void readBytes(void *buffer, size_t length) {
    file_.read(reinterpret_cast<char *>(buffer), length);
  }

  // Read bytes at specific offset
  void readBytes(void *buffer, size_t offset, size_t length) {
    seek(offset);
    readBytes(buffer, length);
  }

  // Read bytes into vector
  [[nodiscard]] auto readBytes(size_t length) -> std::vector<uint8_t> {
    std::vector<uint8_t> buffer(length);
    readBytes(buffer.data(), length);
    return buffer;
  }

  // Position management
  void seek(size_t pos) { file_.seekg(pos); }

  void seekRelative(std::streamoff offset) {
    file_.seekg(offset, std::ios::cur);
  }

  [[nodiscard]] auto position() const -> size_t {
    return static_cast<size_t>(file_.tellg());
  }

  void seekToPage(uint32_t page_number, uint16_t page_size) {
    uint32_t page_offset = (page_number - 1) * page_size;

    if (page_number == 1) {
      seek(page_offset + sqlite::HEADER_SIZE);
    } else {
      seek(page_offset);
    }
  }

  [[nodiscard]] auto size() const -> size_t { return size_; }
  [[nodiscard]] uint8_t peekU8() {
    auto current_pos = position();
    uint8_t value = readU8();
    seek(current_pos);
    return value;
  }

private:
  // Endianness conversion helpers
  template <typename T> static T toBigEndian(T value) {
    if constexpr (std::endian::native == std::endian::little) {
      return byteSwap(value);
    }
    return value;
  }

  template <typename T> static T byteSwap(T value) {
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
      result = (result << 8) | (value & 0xFF);
      value >>= 8;
    }
    return result;
  }

  mutable std::ifstream file_;
  size_t size_;
};
