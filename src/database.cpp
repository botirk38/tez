#include "database.h"
#include "btree_page.h"
#include "btree_record.h"
#include "sqlite_constants.h"
#include <cstdint>

Database::Database(const std::string &filename) : _reader(filename) {
  LOG_INFO("Opening database file: " << filename);
}

SqliteHeader Database::readHeader() {
  LOG_INFO("Reading SQLite _header");
  size_t current_offset = 0;

  // Read magic _header string (16 bytes)
  LOG_DEBUG("Reading magic _header string at offset " << current_offset);
  auto _header_bytes = _reader.readBytes(16);
  _header.header_string =
      std::string(reinterpret_cast<const char *>(_header_bytes.data()),
                  _header_bytes.size());
  current_offset += 16;

  // Read page size (2 bytes)
  LOG_DEBUG("Reading page size at offset " << current_offset);
  _header.page_size = _reader.readU16();
  LOG_INFO("Database page size: " << _header.page_size);
  current_offset += 2;

  // Read single byte fields
  LOG_DEBUG("Reading single byte fields at offset " << current_offset);
  _header.write_version = _reader.readU8();
  _header.read_version = _reader.readU8();
  _header.reserved_bytes = _reader.readU8();
  _header.max_payload_fraction = _reader.readU8();
  _header.min_payload_fraction = _reader.readU8();
  _header.leaf_payload_fraction = _reader.readU8();
  current_offset += 6;

  // Read 4-byte fields
  LOG_DEBUG("Reading 4-byte fields at offset " << current_offset);
  _header.file_change_counter = _reader.readU32();
  _header.db_size_pages = _reader.readU32();
  _header.first_freelist_trunk = _reader.readU32();
  _header.total_freelist_pages = _reader.readU32();
  _header.schema_cookie = _reader.readU32();
  _header.schema_format = _reader.readU32();
  _header.page_cache_size = _reader.readU32();
  _header.vacuum_page = _reader.readU32();
  _header.text_encoding = _reader.readU32();
  _header.user_version = _reader.readU32();
  _header.increment_vacuum = _reader.readU32();
  _header.application_id = _reader.readU32();
  current_offset += 48;

  // Skip reserved space (20 bytes)
  LOG_DEBUG("Skipping reserved space at offset " << current_offset);
  _reader.seek(current_offset + 20);
  current_offset += 20;

  // Read remaining 4-byte fields
  LOG_DEBUG("Reading final 4-byte fields at offset " << current_offset);
  _header.version_valid = _reader.readU32();
  _header.sqlite_version = _reader.readU32();

  LOG_INFO("_header reading completed successfully");
  return _header;
}

uint16_t Database::getTableCount() {
  LOG_INFO("Counting tables in database");
  uint16_t table_count = 0;

  // Create a B-tree page for the schema
  LOG_DEBUG("Creating B-tree page for schema with page size "
            << _header.page_size);
  BTreePage<PageType::LeafTable> schema_page(_reader, _header.page_size,
                                             sqlite::SCHEMA_PAGE);

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

std::vector<std::string> Database::getTableNames() {
  LOG_INFO("Getting table names from database");
  std::vector<std::string> table_names;

  // Seek to start of first page + _header size

  BTreePage<PageType::LeafTable> schema_page(_reader, _header.page_size,
                                             sqlite::SCHEMA_PAGE);

  LOG_DEBUG("Processing " << schema_page.getCells().size()
                          << " schema records");

  for (const auto &cell : schema_page.getCells()) {
    BTreeRecord record(cell.payload);
    SchemaRecord schema = SchemaRecord::fromRecord(record);

    if (isUserTable(schema)) {
      LOG_DEBUG("Found user table: " << schema.tbl_name);
      table_names.push_back(schema.tbl_name);
    }
  }

  LOG_INFO("Found " << table_names.size() << " user tables");
  return table_names;
}

uint32_t Database::getTableRowCount(const std::string &table_name) {
  uint32_t root_page = getTableRootPage(table_name);
  BTreePage<PageType::LeafTable> page(_reader, _header.page_size, root_page);
  return page.getHeader().cell_count;
}

uint32_t Database::getTableRootPage(const std::string &table_name) {
  BTreePage<PageType::LeafTable> schema_page(_reader, _header.page_size,
                                             sqlite::SCHEMA_PAGE);

  for (const auto &cell : schema_page.getCells()) {
    BTreeRecord record(cell.payload);
    SchemaRecord schema = SchemaRecord::fromRecord(record);

    if (schema.type == "table" && schema.name == table_name) {
      return static_cast<uint32_t>(schema.rootpage);
    }
  }
  throw std::runtime_error("Table not found: " + table_name);
}

bool Database::isUserTable(const SchemaRecord &record) {
  // Check if it's a table and not an internal table
  return record.type == "table" && record.name.compare(0, 7, "sqlite_") != 0;
}

SchemaRecord Database::getTableSchema(const std::string &table_name) {
  BTreePage<PageType::LeafTable> schema_page(_reader, _header.page_size,
                                             sqlite::SCHEMA_PAGE);

  for (const auto &cell : schema_page.getCells()) {
    BTreeRecord record(cell.payload);
    auto schema = SchemaRecord::fromRecord(record);
    if (schema.name == table_name) {
      return schema;
    }
  }
  throw std::runtime_error("Table not found: " + table_name);
}

QueryResult Database::executeSelect(const SelectStatement &stmt) {
  if (stmt.is_count_star) {
    return executeCountStar(stmt.table_name);
  }

  return stmt.where_clause ? executeSelectWithWhere(stmt)
                           : executeSelectWithoutWhere(stmt);
}

QueryResult Database::executeCountStar(const std::string &table_name) {
  QueryResult results;
  Row count_row;
  count_row.push_back(static_cast<int64_t>(getTableRowCount(table_name)));
  results.push_back(count_row);
  return results;
}

QueryResult Database::executeSelectWithoutWhere(const SelectStatement &stmt) {
  QueryResult results;
  SchemaRecord schema = getTableSchema(stmt.table_name);
  uint32_t root_page = getTableRootPage(stmt.table_name);
  BTreePage<PageType::LeafTable> page(_reader, _header.page_size, root_page);

  std::vector<int> column_positions =
      mapColumnPositions(stmt.column_names, schema);

  for (const auto &cell : page.getCells()) {
    BTreeRecord record(cell.payload);
    const auto &values = record.getValues();

    Row row;
    for (int pos : column_positions) {
      if (pos < values.size()) {
        row.push_back(values[pos]);
      }
    }
    results.push_back(row);
  }

  return results;
}

QueryResult Database::executeSelectWithWhere(const SelectStatement &stmt) {
  QueryResult results;
  SchemaRecord schema = getTableSchema(stmt.table_name);
  uint32_t root_page = getTableRootPage(stmt.table_name);
  BTreePage<PageType::LeafTable> page(_reader, _header.page_size, root_page);

  std::vector<int> column_positions =
      mapColumnPositions(stmt.column_names, schema);
  int where_col_pos =
      findWhereColumnPosition(stmt.where_clause->column, schema);

  for (const auto &cell : page.getCells()) {
    BTreeRecord record(cell.payload);
    const auto &values = record.getValues();

    if (!matchesWhereCondition(values, where_col_pos, *stmt.where_clause)) {
      continue;
    }

    Row row;
    for (int pos : column_positions) {
      if (pos < values.size()) {
        row.push_back(values[pos]);
      }
    }
    results.push_back(row);
  }

  return results;
}

std::vector<int>
Database::mapColumnPositions(const std::vector<std::string> &column_names,
                             const SchemaRecord &schema) {
  std::vector<int> positions;
  for (const auto &col_name : column_names) {
    for (const auto &col_info : schema.columns) {
      if (col_info.name == col_name) {
        positions.push_back(col_info.position);
        break;
      }
    }
  }
  return positions;
}

int Database::findWhereColumnPosition(const std::string &column_name,
                                      const SchemaRecord &schema) {
  for (const auto &col_info : schema.columns) {
    if (col_info.name == column_name) {
      return col_info.position;
    }
  }
  return -1;
}

bool Database::matchesWhereCondition(const std::vector<RecordValue> &values,
                                     int where_col_pos,
                                     const WhereClause &where) {
  if (where_col_pos < 0 || where_col_pos >= values.size()) {
    return false;
  }

  const auto &cell_value = values[where_col_pos];
  if (std::holds_alternative<std::string>(cell_value)) {
    const std::string &value = std::get<std::string>(cell_value);
    std::string where_value = where.value;
    if (where_value.front() == '\'' && where_value.back() == '\'') {
      where_value = where_value.substr(1, where_value.length() - 2);
    }
    return where.operator_type == "=" && value == where_value;
  }
  return false;
}
