#pragma once

#include "btree_record.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sqlite {

// Page and header sizes
constexpr size_t HEADER_SIZE = 100;
constexpr size_t SCHEMA_PAGE = 1;

// Record type identifiers
namespace record_type {
constexpr const char *TABLE = "table";
constexpr const char *INDEX = "index";
constexpr const char *VIEW = "view";
constexpr const char *TRIGGER = "trigger";
} // namespace record_type

// Internal table prefixes
namespace internal {
constexpr const char *PREFIX = "sqlite_";
constexpr size_t PREFIX_LENGTH = 7;
} // namespace internal

// Schema table column indices
namespace schema {
constexpr size_t TYPE = 0;
constexpr size_t NAME = 1;
constexpr size_t TBL_NAME = 2;
constexpr size_t ROOTPAGE = 3;
constexpr size_t SQL = 4;
} // namespace schema

struct Header {
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

using Row = std::vector<RecordValue>;
using QueryResult = std::vector<Row>;

} // namespace sqlite
