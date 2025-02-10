#pragma once
#include "btree_cell.h"
#include "btree_page.h"
#include "file_reader.h"
#include "schema_record.h"
#include <cstdint>
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

using Row = std::vector<RecordValue>;
using QueryResult = std::vector<Row>;

class Database {
public:
  explicit Database(const std::string &filename);

  SqliteHeader readHeader();
  uint16_t getTableCount();
  bool isTableRecord(const std::vector<uint8_t> &payload);
  bool isUserTable(const SchemaRecord &record);
  std::vector<std::string> getTableNames();
  uint32_t getTableRootPage(const std::string &table_name);

  QueryResult executeSelect(const SelectStatement &stmt);

private:
  FileReader _reader;
  SqliteHeader _header;

  uint32_t getTableRowCount(const std::string &table_name);
  uint32_t countLeafPageCells(uint32_t page_num);
  SchemaRecord getTableSchema(const std::string &table_name);
  QueryResult executeCountStar(const std::string &table_name);
  QueryResult executeSelectWithoutWhere(const SelectStatement &stmt);
  QueryResult executeSelectWithWhere(const SelectStatement &stmt);
  std::vector<int>
  mapColumnPositions(const std::vector<std::string> &column_names,
                     const SchemaRecord &schema);
  int findWhereColumnPosition(const std::string &column_name,
                              const SchemaRecord &schema);
  bool matchesWhereCondition(const std::vector<RecordValue> &values,
                             int where_col_pos, const WhereClause &where);

  void traverseBTree(uint32_t page_num,
                     const std::vector<int> &column_positions,
                     int where_col_pos, const WhereClause &where,
                     QueryResult &results);

  void processLeafPage(const BTreePage<PageType::LeafTable> &page,
                       const std::vector<int> &column_positions,
                       int where_col_pos, const WhereClause &where,
                       QueryResult &results);

  void processInteriorPage(const BTreePage<PageType::InteriorTable> &page,
                           const std::vector<int> &column_positions,
                           int where_col_pos, const WhereClause &where,
                           QueryResult &results);

  int64_t getIndexRootPage(const std::string &table_name,
                           const std::string &column_name);
  std::vector<uint64_t> scanIndex(uint32_t index_root_page,
                                  const std::string &search_value);
  void traverseIndexBTree(uint32_t page_num, const std::string &search_value,
                          std::vector<uint64_t> &rowids);
  QueryResult fetchRowsByIds(const std::string &table_name,
                             const std::vector<uint64_t> &rowids,
                             const std::vector<std::string> &columns);
  void findRowInBTree(uint32_t page_num, uint64_t target_rowid,
                      const std::vector<int> &column_positions,
                      QueryResult &results);
};
