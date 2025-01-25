#pragma once
#include "file_reader.h"
#include <string>

class Database {
public:
  explicit Database(const std::string &filename);

  uint16_t getPageSize();
  bool isOpen() const { return reader.isOpen(); }
  size_t getDatabaseSize() { return reader.getFileSize(); }

private:
  FileReader reader;
};
