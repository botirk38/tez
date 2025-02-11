#pragma once

#include "btree_page.h"
#include "file_reader.h"
#include "schema_record.h"
#include "sqlite_constants.h"
#include <vector>

using Row = sqlite::Row;
using QueryResult = sqlite::QueryResult;

class BTree {
public:
  BTree(FileReader &reader, const sqlite::Header &header);

  void traverse(uint32_t page_num, const std::vector<int> &column_positions,
                int where_col_pos, const WhereClause &where,
                sqlite::QueryResult &results);

  std::vector<uint64_t> scanIndex(uint32_t index_root_page,
                                  const std::string &search_value);

  void findRow(uint32_t page_num, uint64_t target_rowid,
               const std::vector<int> &column_positions,
               sqlite::QueryResult &results);
  int64_t getIndexRootPage(const std::string &table_name,
                           const std::string &column_name);
  QueryResult fetchRowsByIds(const std::vector<uint64_t> &rowids,
                             const std::vector<std::string> &columns,
                             const SchemaRecord &schema,
                             const uint32_t root_page);

private:
  FileReader &_reader;
  const sqlite::Header &_header;

  void traverseIndexBTree(uint32_t page_num, const std::string &search_value,
                          std::vector<uint64_t> &rowids);

  void processLeafPage(const BTreePage<PageType::LeafTable> &page,
                       const std::vector<int> &column_positions,
                       int where_col_pos, const WhereClause &where,
                       sqlite::QueryResult &results);

  void processInteriorPage(const BTreePage<PageType::InteriorTable> &page,
                           const std::vector<int> &column_positions,
                           int where_col_pos, const WhereClause &where,
                           sqlite::QueryResult &results);

  bool matchesWhereCondition(const std::vector<RecordValue> &values,
                             int where_col_pos, const WhereClause &where);
};
