#include "database.h"
#include "btree.h"
#include "debug.h"

Database::Database(const std::string &filename)
    : _reader(filename), _table_manager(_reader, _header),
      _btree(_reader, _header) {
  LOG_INFO("Opening database file: " << filename);
}

sqlite::Header Database::readHeader() {
  LOG_INFO("Reading SQLite header");
  size_t current_offset = 0;

  // Read magic header string (16 bytes)
  LOG_DEBUG("Reading magic header string at offset " << current_offset);
  auto header_bytes = _reader.readBytes(16);
  _header.header_string = std::string(
      reinterpret_cast<const char *>(header_bytes.data()), header_bytes.size());
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

  // Skip reserved space
  LOG_DEBUG("Skipping reserved space at offset " << current_offset);
  _reader.seek(current_offset + 20);
  current_offset += 20;

  // Read remaining 4-byte fields
  LOG_DEBUG("Reading final 4-byte fields at offset " << current_offset);
  _header.version_valid = _reader.readU32();
  _header.sqlite_version = _reader.readU32();

  LOG_INFO("Header reading completed successfully");
  return _header;
}

uint16_t Database::getTableCount() {
  LOG_INFO("Counting tables in database");
  uint16_t table_count = 0;
  BTreePage<PageType::LeafTable> schema_page(_reader, _header.page_size,
                                             sqlite::SCHEMA_PAGE);

  for (const auto &cell : schema_page.getCells()) {
    if (!cell.payload.empty() && _table_manager.isTableRecord(cell.payload)) {
      table_count++;
    }
  }

  LOG_INFO("Found " << table_count << " tables in database");
  return table_count;
}

std::vector<std::string> Database::getTableNames() {
  LOG_INFO("Getting table names from database");
  std::vector<std::string> table_names;
  BTreePage<PageType::LeafTable> schema_page(_reader, _header.page_size,
                                             sqlite::SCHEMA_PAGE);

  for (const auto &cell : schema_page.getCells()) {
    BTreeRecord record(cell.payload);
    SchemaRecord schema(record);
    if (_table_manager.isUserTable(schema)) {
      table_names.push_back(schema.getTableName());
    }
  }

  LOG_INFO("Found " << table_names.size() << " user tables");
  return table_names;
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
  count_row.push_back(
      static_cast<int64_t>(_table_manager.getTableRowCount(table_name)));
  results.push_back(count_row);
  return results;
}

QueryResult Database::executeSelectWithoutWhere(const SelectStatement &stmt) {
  SchemaRecord schema = _table_manager.getTableSchema(stmt.table_name);
  uint32_t root_page = _table_manager.getTableRootPage(stmt.table_name);
  std::vector<int> column_positions =
      schema.mapColumnPositions(stmt.column_names);

  QueryResult results;
  _btree.traverse(root_page, column_positions, -1, WhereClause(), results);
  return results;
}

QueryResult Database::executeSelectWithWhere(const SelectStatement &stmt) {
  try {
    uint32_t index_root_page =
        _btree.getIndexRootPage(stmt.table_name, stmt.where_clause->column);
    std::string search_value = stmt.where_clause->value;
    if (search_value.front() == '\'' && search_value.back() == '\'') {
      search_value = search_value.substr(1, search_value.length() - 2);
    }

    std::vector<uint64_t> rowids =
        _btree.scanIndex(index_root_page, search_value);
    SchemaRecord schema = _table_manager.getTableSchema(stmt.table_name);
    uint32_t root_page = _table_manager.getTableRootPage(stmt.table_name);
    return _btree.fetchRowsByIds(rowids, stmt.column_names, schema, root_page);
  } catch (const std::runtime_error &) {
    SchemaRecord schema = _table_manager.getTableSchema(stmt.table_name);
    uint32_t root_page = _table_manager.getTableRootPage(stmt.table_name);
    std::vector<int> column_positions =
        schema.mapColumnPositions(stmt.column_names);
    int where_col_pos =
        schema.findWhereColumnPosition(stmt.where_clause->column);

    QueryResult results;
    _btree.traverse(root_page, column_positions, where_col_pos,
                    *stmt.where_clause, results);
    return results;
  }
}
