#pragma once
#include <bit>
#include <cstdint>
#include <fstream>
#include <string>

class FileReader {
public:
  FileReader(const std::string &filename);

  // Core reading operations with big-endian conversion
  bool readBytes(void *buffer, size_t offset, size_t size);
  bool readUint16(uint16_t &value, size_t offset);
  bool readUint32(uint32_t &value, size_t offset);

  bool isOpen() const { return file.is_open(); }
  size_t getFileSize();

private:
  std::ifstream file;
};
