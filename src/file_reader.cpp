#include "file_reader.h"

FileReader::FileReader(const std::string &filename) {
  file.open(filename, std::ios::binary);
}

bool FileReader::readBytes(void *buffer, size_t offset, size_t size) {
  if (!file.is_open())
    return false;

  file.seekg(offset);
  file.read(static_cast<char *>(buffer), size);
  return file.good();
}

bool FileReader::readUint16(uint16_t &value, size_t offset) {
  uint16_t buffer;
  if (!readBytes(&buffer, offset, sizeof(buffer)))
    return false;

  // Convert from big-endian to native
  value = std::byteswap(buffer);
  return true;
}

bool FileReader::readUint32(uint32_t &value, size_t offset) {
  uint32_t buffer;
  if (!readBytes(&buffer, offset, sizeof(buffer)))
    return false;

  // Convert from big-endian to native
  value = std::byteswap(buffer);
  return true;
}

size_t FileReader::getFileSize() {
  auto current = file.tellg();
  file.seekg(0, std::ios::end);
  auto size = file.tellg();
  file.seekg(current);
  return size;
}

