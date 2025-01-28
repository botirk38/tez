#pragma once
#include "file_reader.h"
#include "schema_record.h"
#include <stdatomic.h>
#include <string>
#include <vector>

struct SqliteHeader {
  std::string header_string;     // 16 bytes
  uint16_t page_size;            // 2 bytes
  uint8_t write_version;         // 1 byte
  uint8_t read_version;          // 1 byte
  uint8_t reserved_bytes;        // 1 byte
  uint8_t max_payload_fraction;  // 1 byte
  uint8_t min_payload_fraction;  // 1 byte
  uint8_t leaf_payload_fraction; // 1 byte
  uint32_t file_change_counter;  // 4 bytes
  uint32_t db_size_pages;        // 4 bytes
  uint32_t first_freelist_trunk; // 4 bytes
  uint32_t total_freelist_pages; // 4 bytes
  uint32_t schema_cookie;        // 4 bytes
  uint32_t schema_format;        // 4 bytes
  uint32_t page_cache_size;      // 4 bytes
  uint32_t vacuum_page;          // 4 bytes
  uint32_t text_encoding;        // 4 bytes
  uint32_t user_version;         // 4 bytes
  uint32_t increment_vacuum;     // 4 bytes
  uint32_t application_id;       // 4 bytes
  uint32_t version_valid;        // 4 bytes
  uint32_t sqlite_version;       // 4 bytes
};

class Database {
public:
  explicit Database(const std::string &filename);

  SqliteHeader readHeader();
  uint16_t getTableCount();
  bool isTableRecord(const std::vector<uint8_t> &payload);
  bool isUserTable(const SchemaRecord &record);
  std::vector<std::string> getTableNames();

private:
  FileReader reader;
  SqliteHeader header;
};
