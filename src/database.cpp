#include "database.h"
#include "btree_page.h"
#include "btree_record.h"
#include "debug.h"
#include "sqlite_constants.h"
#include <algorithm>
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
  std::vector<int> column_positions =
      mapColumnPositions(stmt.column_names, schema);

  // Start traversing from root page
  traverseBTree(root_page, column_positions, -1, WhereClause(), results);

  return results;
}

std::vector<int>
Database::mapColumnPositions(const std::vector<std::string> &column_names,
                             const SchemaRecord &schema) {
  std::vector<int> positions;

  for (const auto &col_name : column_names) {
    if (col_name == "id") {
      positions.push_back(-1); // Special marker for id/rowid column
      continue;
    }

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

void Database::traverseBTree(uint32_t page_num,
                             const std::vector<int> &column_positions,
                             int where_col_pos, const WhereClause &where,
                             QueryResult &results) {
  LOG_DEBUG("Traversing B-tree page: " << page_num);

  // First read the page type without consuming it
  _reader.seekToPage(page_num, _header.page_size);
  uint8_t page_type = _reader.readU8();

  // Reset position
  _reader.seek(_header.page_size * (page_num - 1));

  if (static_cast<PageType>(page_type) == PageType::InteriorTable) {
    LOG_DEBUG("Processing interior page: " << page_num);
    BTreePage<PageType::InteriorTable> page(_reader, _header.page_size,
                                            page_num);
    processInteriorPage(page, column_positions, where_col_pos, where, results);
  } else {
    LOG_DEBUG("Processing leaf page: " << page_num);
    BTreePage<PageType::LeafTable> page(_reader, _header.page_size, page_num);
    processLeafPage(page, column_positions, where_col_pos, where, results);
  }
}

void Database::processLeafPage(const BTreePage<PageType::LeafTable> &page,
                               const std::vector<int> &column_positions,
                               int where_col_pos, const WhereClause &where,
                               QueryResult &results) {
  LOG_DEBUG("Processing leaf page cells");

  for (const auto &cell : page.getCells()) {
    BTreeRecord record(cell.payload);
    const auto &values = record.getValues();

    LOG_DEBUG("Checking record against where condition");

    // Otherwise check the where condition
    if (where_col_pos == -1 ||
        matchesWhereCondition(values, where_col_pos, where)) {
      Row row;
      for (int pos : column_positions) {
        if (pos == -1) {
          row.push_back(static_cast<int64_t>(cell.row_id));
        } else if (pos < values.size()) {
          row.push_back(values[pos]);
        }
      }
      results.push_back(row);
    }
  }
}
void Database::processInteriorPage(
    const BTreePage<PageType::InteriorTable> &page,
    const std::vector<int> &column_positions, int where_col_pos,
    const WhereClause &where, QueryResult &results) {
  LOG_DEBUG("Processing interior page cells");

  for (const auto &cell : page.getCells()) {
    uint32_t child_page = cell.left_pointer;
    LOG_DEBUG("Traversing child page: " << child_page);
    traverseBTree(child_page, column_positions, where_col_pos, where, results);
  }

  if (page.getHeader().right_most_pointer) {
    LOG_DEBUG("Traversing right-most pointer: "
              << page.getHeader().right_most_pointer);
    traverseBTree(page.getHeader().right_most_pointer, column_positions,
                  where_col_pos, where, results);
  }
}

int64_t Database::getIndexRootPage(const std::string &table_name,
                                   const std::string &column_name) {
  LOG_INFO("Looking for table: " << table_name);
  LOG_INFO("Index column: " << column_name);

  BTreePage<PageType::LeafTable> schema_page(_reader, _header.page_size,
                                             sqlite::SCHEMA_PAGE);

  LOG_DEBUG("Reading sqlite_schema (page 1), found "
            << schema_page.getHeader().cell_count << " entries");

  int64_t index_root_page = 0;

  for (const auto &cell : schema_page.getCells()) {
    BTreeRecord record(cell.payload);
    const auto &values = record.getValues();

    if (values.empty() || !std::holds_alternative<std::string>(values[0])) {
      continue;
    }

    std::string type = std::get<std::string>(values[0]);
    LOG_DEBUG("Examining entry type: " << type);

    // Check for index entries
    if (type == "index" && values.size() >= 5) {
      std::string tbl_name = std::get<std::string>(values[2]);
      LOG_INFO("TABLE NAME: " << tbl_name);

      std::string sql = std::get<std::string>(values[4]);
      LOG_INFO("SQL: " << sql);

      if (tbl_name == table_name &&
          sql.find(column_name) != std::string::npos) {
        index_root_page = std::get<int64_t>(values[3]);
        LOG_INFO("Found matching index! Root page: " << index_root_page);
        break;
      }
    }
  }

  if (index_root_page == 0) {
    throw std::runtime_error("Index not found for column: " + column_name);
  }

  LOG_INFO("Search complete - Index root: " << index_root_page);
  return index_root_page;
}

std::vector<uint64_t> Database::scanIndex(uint32_t index_root_page,
                                          const std::string &search_value) {
  LOG_INFO("Scanning index starting at root page: " << index_root_page);
  LOG_DEBUG("Searching for value: " << search_value);

  std::vector<uint64_t> rowids;
  traverseIndexBTree(index_root_page, search_value, rowids);

  LOG_INFO("Found " << rowids.size() << " matching rows");
  return rowids;
}

void Database::traverseIndexBTree(uint32_t page_num,
                                  const std::string &search_value,
                                  std::vector<uint64_t> &rowids) {
  LOG_DEBUG("Traversing index B-tree page: " << page_num);

  _reader.seekToPage(page_num, _header.page_size);
  uint8_t page_type = _reader.readU8();
  _reader.seek(_header.page_size * (page_num - 1));

  if (static_cast<PageType>(page_type) == PageType::LeafIndex) {
    LOG_DEBUG("Processing leaf index page");
    BTreePage<PageType::LeafIndex> page(_reader, _header.page_size, page_num);

    for (const auto &cell : page.getCells()) {
      BTreeRecord record(cell.payload);
      const auto &values = record.getValues();

      if (values.size() >= 2 &&
          std::holds_alternative<std::string>(values[0]) &&
          std::holds_alternative<int64_t>(values[1])) {

        std::string key = std::get<std::string>(values[0]);
        LOG_DEBUG("Comparing key: " << key
                                    << " with search value: " << search_value);

        if (key == search_value) {
          uint64_t rowid = static_cast<uint64_t>(std::get<int64_t>(values[1]));
          LOG_DEBUG("Found matching rowid: " << rowid);
          rowids.push_back(rowid);
        }
      }
    }
  } else {
    LOG_DEBUG("Processing interior index page");
    BTreePage<PageType::InteriorIndex> page(_reader, _header.page_size,
                                            page_num);

    for (const auto &cell : page.getCells()) {
      BTreeRecord record(cell.payload);
      const auto &values = record.getValues();

      if (!values.empty() && std::holds_alternative<std::string>(values[0])) {
        std::string key = std::get<std::string>(values[0]);
        LOG_DEBUG("Checking interior node key: " << key);

        if (key >= search_value) {
          LOG_DEBUG("Following child pointer to page: " << cell.page_number);
          traverseIndexBTree(cell.page_number, search_value, rowids);
          return;
        }
      }
    }

    if (page.getHeader().right_most_pointer) {
      LOG_DEBUG("Following right-most pointer to page: "
                << page.getHeader().right_most_pointer);
      traverseIndexBTree(page.getHeader().right_most_pointer, search_value,
                         rowids);
    }
  }
}

QueryResult Database::executeSelectWithWhere(const SelectStatement &stmt) {
  LOG_INFO("Executing SELECT with WHERE clause for table: " << stmt.table_name);

  if (!stmt.where_clause) {
    LOG_INFO("No WHERE clause found, executing regular SELECT");
    return executeSelectWithoutWhere(stmt);
  }

  try {
    // Try to use index first
    LOG_INFO("Using index for WHERE clause on column: "
             << stmt.where_clause->column);
    uint32_t index_root_page =
        getIndexRootPage(stmt.table_name, stmt.where_clause->column);

    std::string search_value = stmt.where_clause->value;
    if (search_value.front() == '\'' && search_value.back() == '\'') {
      search_value = search_value.substr(1, search_value.length() - 2);
    }

    LOG_INFO("Scanning index for value: " << search_value);
    std::vector<uint64_t> rowids = scanIndex(index_root_page, search_value);
    LOG_INFO("Found " << rowids.size() << " matching rowids");

    return fetchRowsByIds(stmt.table_name, rowids, stmt.column_names);
  } catch (const std::runtime_error &) {
    // No index available, fall back to full table scan
    LOG_INFO("No index found, performing full table scan");
    SchemaRecord schema = getTableSchema(stmt.table_name);
    uint32_t root_page = getTableRootPage(stmt.table_name);
    std::vector<int> column_positions =
        mapColumnPositions(stmt.column_names, schema);
    int where_col_pos =
        findWhereColumnPosition(stmt.where_clause->column, schema);

    QueryResult results;
    traverseBTree(root_page, column_positions, where_col_pos,
                  *stmt.where_clause, results);
    return results;
  }
}

QueryResult Database::fetchRowsByIds(const std::string &table_name,
                                     const std::vector<uint64_t> &rowids,
                                     const std::vector<std::string> &columns) {
  LOG_INFO("Fetching rows by IDs for table: " << table_name);
  LOG_DEBUG("Number of rowids to fetch: " << rowids.size());

  QueryResult results;
  SchemaRecord schema = getTableSchema(table_name);
  uint32_t root_page = getTableRootPage(table_name);
  std::vector<int> column_positions = mapColumnPositions(columns, schema);

  LOG_DEBUG("Table root page: " << root_page);
  LOG_DEBUG("Number of columns to fetch: " << column_positions.size());

  // Create sorted copy of rowids
  std::vector<uint64_t> sorted_rowids = rowids;
  std::sort(sorted_rowids.begin(), sorted_rowids.end());

  for (uint64_t rowid : sorted_rowids) {
    LOG_DEBUG("Searching for rowid: " << rowid);
    findRowInBTree(root_page, rowid, column_positions, results);
  }

  LOG_INFO("Found " << results.size() << " rows");
  return results;
}

void Database::findRowInBTree(uint32_t page_num, uint64_t target_rowid,
                              const std::vector<int> &column_positions,
                              QueryResult &results) {
  _reader.seekToPage(page_num, _header.page_size);
  uint8_t page_type = _reader.readU8();
  _reader.seekToPage(page_num, _header.page_size);

  if (static_cast<PageType>(page_type) == PageType::InteriorTable) {
    BTreePage<PageType::InteriorTable> page(_reader, _header.page_size,
                                            page_num);
    const auto &cells = page.getCells();

    // Find the appropriate child page to follow
    uint32_t child_page = page.getHeader().right_most_pointer;

    for (size_t i = 0; i < cells.size(); i++) {
      if (target_rowid < cells[i].interior_row_id) {
        child_page = cells[i].left_pointer;
        break;
      }
    }

    findRowInBTree(child_page, target_rowid, column_positions, results);
  } else {
    BTreePage<PageType::LeafTable> page(_reader, _header.page_size, page_num);

    for (const auto &cell : page.getCells()) {
      if (cell.row_id == target_rowid) {
        BTreeRecord record(cell.payload);
        const auto &values = record.getValues();

        Row row;
        for (int pos : column_positions) {
          if (pos == -1) {
            row.push_back(static_cast<int64_t>(cell.row_id));
          } else if (pos < values.size()) {
            row.push_back(values[pos]);
          }
        }
        results.push_back(row);
        return;
      }
    }
  }
}
