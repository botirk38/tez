#include "table_manager.hpp"
#include "btree_page.hpp"
#include "btree_record.hpp"
#include "debug.hpp"
#include "sqlite_constants.hpp"

TableManager::TableManager(FileReader &reader, const sqlite::Header &header)
    : _reader(reader), _header(header) {}

bool TableManager::isTableRecord(const std::vector<uint8_t> &payload) const {
  LOG_DEBUG("Analyzing record payload of size " << payload.size());
  BTreeRecord record(payload);
  const auto &values = record.getValues();

  if (values.empty()) {
    LOG_DEBUG("Empty record, not a table");
    return false;
  }

  if (!std::holds_alternative<std::string>(values[0])) {
    LOG_DEBUG("First value is not a string type");
    return false;
  }

  const std::string &type = std::get<std::string>(values[0]);
  LOG_DEBUG("Record type: " << type);
  return type == "table";
}

bool TableManager::isUserTable(const SchemaRecord &record) const {
  return record.getType() == "table" &&
         record.getName().compare(0, sqlite::internal::PREFIX_LENGTH,
                                  sqlite::internal::PREFIX) != 0;
}

uint32_t TableManager::getTableRootPage(const std::string &table_name) const {
  BTreePage<PageType::LeafTable> schema_page(_reader, _header.page_size,
                                             sqlite::SCHEMA_PAGE);

  for (const auto &cell : schema_page.getCells()) {
    BTreeRecord record(cell.payload);
    SchemaRecord schema(record);

    if (schema.getType() == sqlite::record_type::TABLE &&
        schema.getName() == table_name) {
      return static_cast<uint32_t>(schema.getRootPage());
    }
  }

  throw std::runtime_error("Table not found: " + table_name);
}

uint32_t TableManager::getTableRowCount(const std::string &table_name) const {
  uint32_t root_page = getTableRootPage(table_name);
  BTreePage<PageType::LeafTable> page(_reader, _header.page_size, root_page);
  return page.getHeader().cell_count;
}

SchemaRecord TableManager::getTableSchema(const std::string &table_name) const {
  BTreePage<PageType::LeafTable> schema_page(_reader, _header.page_size,
                                             sqlite::SCHEMA_PAGE);

  for (const auto &cell : schema_page.getCells()) {
    BTreeRecord record(cell.payload);
    SchemaRecord schema(record);

    if (schema.getName() == table_name) {
      return schema;
    }
  }

  throw std::runtime_error("Table not found: " + table_name);
}

