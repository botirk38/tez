#include "database.h"
#include "btree_page.h"
#include <array>
#include <iostream>
#include <memory>

Database::Database(const std::string &filename) : reader(filename) {}

SqliteHeader Database::readHeader() {
  SqliteHeader header;

  // Read header string (16 bytes)
  std::array<char, 16> header_str;
  reader.readBytes(header_str.data(), 0, 16);
  header.header_string = std::string(header_str.data(), 16);

  // Read all numeric fields
  reader.readUint16(header.page_size, 16);

  uint8_t temp;
  reader.readBytes(&temp, 18, 1);
  header.write_version = temp;

  reader.readBytes(&temp, 19, 1);
  header.read_version = temp;

  reader.readBytes(&temp, 20, 1);
  header.reserved_bytes = temp;

  reader.readBytes(&temp, 21, 1);
  header.max_payload_fraction = temp;

  reader.readBytes(&temp, 22, 1);
  header.min_payload_fraction = temp;

  reader.readBytes(&temp, 23, 1);
  header.leaf_payload_fraction = temp;

  reader.readUint32(header.file_change_counter, 24);
  reader.readUint32(header.db_size_pages, 28);
  reader.readUint32(header.first_freelist_trunk, 32);
  reader.readUint32(header.total_freelist_pages, 36);
  reader.readUint32(header.schema_cookie, 40);
  reader.readUint32(header.schema_format, 44);
  reader.readUint32(header.page_cache_size, 48);
  reader.readUint32(header.vacuum_page, 52);
  reader.readUint32(header.text_encoding, 56);
  reader.readUint32(header.user_version, 60);
  reader.readUint32(header.increment_vacuum, 64);
  reader.readUint32(header.application_id, 68);
  reader.readUint32(header.version_valid, 92);
  reader.readUint32(header.sqlite_version, 96);

  return header;
}

size_t Database::countTables() {
  auto header = readHeader();

  std::cout << "=== SQLite Header Info ===" << std::endl;
  std::cout << "Page size: " << header.page_size << " bytes" << std::endl;
  std::cout << "File format version: " << static_cast<int>(header.read_version)
            << std::endl;
  std::cout << "Number of pages: " << header.db_size_pages << std::endl;

  const size_t SQLITE_HEADER_SIZE = 100;
  const size_t pageOffset = SQLITE_HEADER_SIZE;

  std::cout << "\n=== Reading Page 1 (sqlite_schema) ===" << std::endl;
  std::cout << "Reading from offset: " << pageOffset << std::endl;

  std::vector<uint8_t> pageData(header.page_size);
  reader.readBytes(pageData.data(), pageOffset, header.page_size);

  ByteReader pageReader(pageData);
  auto pageType = static_cast<PageType>(pageReader.readU8());

  std::cout << "Page type: 0x" << std::hex << static_cast<int>(pageType)
            << std::dec << " ("
            << (pageType == PageType::LEAF_TABLE       ? "LEAF_TABLE"
                : pageType == PageType::INTERIOR_TABLE ? "INTERIOR_TABLE"
                                                       : "UNKNOWN")
            << ")" << std::endl;

  std::unique_ptr<BTreePage> page;
  switch (pageType) {
  case PageType::LEAF_TABLE:
    page = std::make_unique<TableLeafPage>(pageData);
    break;
  case PageType::INTERIOR_TABLE:
    page = std::make_unique<TableInteriorPage>(pageData);
    break;
  default:
    throw std::runtime_error("Invalid page type for sqlite_schema");
  }

  auto cells = page->getCells();
  std::cout << "\n=== Processing Cells ===" << std::endl;
  std::cout << "Number of cells found: " << cells.size() << std::endl;

  size_t tableCount = 0;
  size_t cellIndex = 0;

  for (const auto &cell : cells) {
    std::cout << "\nCell #" << ++cellIndex << ":" << std::endl;

    if (auto tableCell = dynamic_cast<TableLeafCell *>(cell.get())) {
      std::cout << "Row ID: " << tableCell->rowId << std::endl;
      std::cout << "Payload size: " << tableCell->payload.size() << " bytes"
                << std::endl;

      ByteReader recordReader(tableCell->payload);
      auto [typeLen, bytesRead] = recordReader.readVarint();

      std::string type(
          reinterpret_cast<const char *>(tableCell->payload.data() + bytesRead),
          typeLen);

      std::cout << "Entry type: '" << type << "' (length: " << typeLen << ")"
                << std::endl;

      if (type == "table") {
        tableCount++;
        std::cout << "✓ Found table definition" << std::endl;
      }
    } else {
      std::cout << "Not a table leaf cell" << std::endl;
    }
  }

  std::cout << "\n=== Summary ===" << std::endl;
  std::cout << "Total tables found: " << tableCount << std::endl;

  return tableCount;
}
