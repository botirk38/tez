#include "database.h"

Database::Database(const std::string &filename) : reader(filename) {}

uint16_t Database::getPageSize() {
  uint16_t page_size;
  if (!reader.readUint16(page_size, 16)) {
    return 0;
  }
  return page_size;
}

