#include "database.h"
#include "btree_page.h"
#include "btree_record.h"

Database::Database(const std::string &filename) : reader(filename) {
  LOG_INFO("Opening database file: " << filename);
}

SqliteHeader Database::readHeader() {
  LOG_INFO("Reading SQLite header");
  size_t current_offset = 0;

  // Read magic header string (16 bytes)
  LOG_DEBUG("Reading magic header string at offset " << current_offset);
  auto header_bytes = reader.readBytes(16);
  header.header_string = std::string(
      reinterpret_cast<const char *>(header_bytes.data()), header_bytes.size());
  current_offset += 16;

  // Read page size (2 bytes)
  LOG_DEBUG("Reading page size at offset " << current_offset);
  header.page_size = reader.readU16();
  LOG_INFO("Database page size: " << header.page_size);
  current_offset += 2;

  // Read single byte fields
  LOG_DEBUG("Reading single byte fields at offset " << current_offset);
  header.write_version = reader.readU8();
  header.read_version = reader.readU8();
  header.reserved_bytes = reader.readU8();
  header.max_payload_fraction = reader.readU8();
  header.min_payload_fraction = reader.readU8();
  header.leaf_payload_fraction = reader.readU8();
  current_offset += 6;

  // Read 4-byte fields
  LOG_DEBUG("Reading 4-byte fields at offset " << current_offset);
  header.file_change_counter = reader.readU32();
  header.db_size_pages = reader.readU32();
  header.first_freelist_trunk = reader.readU32();
  header.total_freelist_pages = reader.readU32();
  header.schema_cookie = reader.readU32();
  header.schema_format = reader.readU32();
  header.page_cache_size = reader.readU32();
  header.vacuum_page = reader.readU32();
  header.text_encoding = reader.readU32();
  header.user_version = reader.readU32();
  header.increment_vacuum = reader.readU32();
  header.application_id = reader.readU32();
  current_offset += 48;

  // Skip reserved space (20 bytes)
  LOG_DEBUG("Skipping reserved space at offset " << current_offset);
  reader.seek(current_offset + 20);
  current_offset += 20;

  // Read remaining 4-byte fields
  LOG_DEBUG("Reading final 4-byte fields at offset " << current_offset);
  header.version_valid = reader.readU32();
  header.sqlite_version = reader.readU32();

  LOG_INFO("Header reading completed successfully");
  return header;
}

uint16_t Database::getTableCount() {
  LOG_INFO("Counting tables in database");
  constexpr size_t HEADER_SIZE = 100;
  uint16_t table_count = 0;

  LOG_DEBUG("Seeking to schema page at offset " << HEADER_SIZE);
  reader.seek(HEADER_SIZE);

  // Create a B-tree page for the schema
  LOG_DEBUG("Creating B-tree page for schema with page size "
            << header.page_size);
  BTreePage<PageType::LeafTable> schema_page(reader, header.page_size);

  // Iterate through cells and verify each is a table
  LOG_DEBUG("Examining " << schema_page.getCells().size() << " cells");
  for (const auto &cell : schema_page.getCells()) {
    if (cell.payload.empty())
      continue;
    if (isTableRecord(cell.payload)) {
      table_count++;
      LOG_DEBUG("Found table record, current count: " << table_count);
    }
  }

  LOG_INFO("Found " << table_count << " tables in database");
  return table_count;
}

bool Database::isTableRecord(const std::vector<uint8_t> &payload) {
  LOG_DEBUG("Analyzing record payload of size " << payload.size());

  // Use our new BTreeRecord parser
  BTreeRecord record(payload);
  const auto &values = record.getValues();

  // Schema table records should have at least one value for the type
  if (values.empty()) {
    LOG_DEBUG("Empty record, not a table");
    return false;
  }

  // First value should be a string containing the type
  if (!std::holds_alternative<std::string>(values[0])) {
    LOG_DEBUG("First value is not a string type");
    return false;
  }

  // Check if the type is "table"
  const std::string &type = std::get<std::string>(values[0]);
  LOG_DEBUG("Record type: " << type);

  return type == "table";
}
