#pragma once

#include "btree_cell.h"
#include "btree_page.h"
#include "file_reader.h"
#include "schema_record.h"
#include "sqlite_constants.h"
#include "table_manager.h"
#include <cstdint>
#include <string>
#include <vector>

using Row = sqlite::Row;
using QueryResult = sqlite::QueryResult;
using SqliteHeader = sqlite::Header;

class Database {
public:
  explicit Database(const std::string &filename);

  SqliteHeader readHeader();
  uint16_t getTableCount();
  std::vector<std::string> getTableNames();
  sqlite::QueryResult executeSelect(const SelectStatement &stmt);

private:
  FileReader _reader;
  SqliteHeader _header;
  TableManager _table_manager;

  sqlite::QueryResult executeCountStar(const std::string &table_name);
  sqlite::QueryResult executeSelectWithoutWhere(const SelectStatement &stmt);
  sqlite::QueryResult executeSelectWithWhere(const SelectStatement &stmt);

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
                     sqlite::QueryResult &results);

  void processLeafPage(const BTreePage<PageType::LeafTable> &page,
                       const std::vector<int> &column_positions,
                       int where_col_pos, const WhereClause &where,
                       sqlite::QueryResult &results);

  void processInteriorPage(const BTreePage<PageType::InteriorTable> &page,
                           const std::vector<int> &column_positions,
                           int where_col_pos, const WhereClause &where,
                           sqlite::QueryResult &results);

  int64_t getIndexRootPage(const std::string &table_name,
                           const std::string &column_name);
  std::vector<uint64_t> scanIndex(uint32_t index_root_page,
                                  const std::string &search_value);
  void traverseIndexBTree(uint32_t page_num, const std::string &search_value,
                          std::vector<uint64_t> &rowids);

  sqlite::QueryResult fetchRowsByIds(const std::string &table_name,
                                     const std::vector<uint64_t> &rowids,
                                     const std::vector<std::string> &columns);

  void findRowInBTree(uint32_t page_num, uint64_t target_rowid,
                      const std::vector<int> &column_positions,
                      sqlite::QueryResult &results);
};
