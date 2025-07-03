#include "btree.hpp"
#include "btree_record.hpp"
#include "debug.hpp"
#include "schema_record.hpp"
#include <algorithm>

BTree::BTree(FileReader &reader, const sqlite::Header &header) noexcept
    : _reader(reader), _header(header) {}

void BTree::traverse(uint32_t page_num,
                     const std::vector<int> &column_positions,
                     int where_col_pos, const WhereClause &where,
                     sqlite::QueryResult &results) const {
  LOG_DEBUG("Traversing B-tree page: " << page_num);

  _reader.seekToPage(page_num, _header.page_size);
  uint8_t page_type = _reader.readU8();
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

void BTree::processLeafPage(const BTreePage<PageType::LeafTable> &page,
                            const std::vector<int> &column_positions,
                            int where_col_pos, const WhereClause &where,
                            sqlite::QueryResult &results) const {
  LOG_DEBUG("Processing leaf page cells");

  for (const auto &cell : page.getCells()) {
    BTreeRecord record(cell.payload);
    const auto &values = record.getValues();

    LOG_DEBUG("Checking record against where condition");

    if (where_col_pos == -1 ||
        matchesWhereCondition(values, where_col_pos, where)) {
      Row row;
      row.reserve(column_positions.size());
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

void BTree::processInteriorPage(const BTreePage<PageType::InteriorTable> &page,
                                const std::vector<int> &column_positions,
                                int where_col_pos, const WhereClause &where,
                                sqlite::QueryResult &results) const {
  LOG_DEBUG("Processing interior page cells");

  for (const auto &cell : page.getCells()) {
    uint32_t child_page = cell.left_pointer;
    LOG_DEBUG("Traversing child page: " << child_page);
    traverse(child_page, column_positions, where_col_pos, where, results);
  }

  if (page.getHeader().right_most_pointer) {
    LOG_DEBUG("Traversing right-most pointer: "
              << page.getHeader().right_most_pointer);
    traverse(page.getHeader().right_most_pointer, column_positions,
             where_col_pos, where, results);
  }
}

std::vector<uint64_t> BTree::scanIndex(uint32_t index_root_page,
                                       const std::string &search_value) const {
  LOG_INFO("Scanning index starting at root page: " << index_root_page);
  LOG_DEBUG("Searching for value: " << search_value);

  std::vector<uint64_t> rowids;
  traverseIndexBTree(index_root_page, search_value, rowids);

  LOG_INFO("Found " << rowids.size() << " matching rows");
  return rowids;
}

void BTree::traverseIndexBTree(uint32_t page_num,
                               const std::string &search_value,
                               std::vector<uint64_t> &rowids) const {
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
          // Check for duplicates before adding
          if (std::find(rowids.begin(), rowids.end(), rowid) == rowids.end()) {
            rowids.push_back(rowid);
          }
        }
      }
    }
  } else {
    LOG_DEBUG("Processing interior index page");
    BTreePage<PageType::InteriorIndex> page(_reader, _header.page_size,
                                            page_num);

    // Check interior cells for matching keys (they contain key + rowid payload)
    for (const auto &cell : page.getCells()) {
      BTreeRecord record(cell.payload);
      const auto &values = record.getValues();

      if (!values.empty() && std::holds_alternative<std::string>(values[0])) {
        std::string key = std::get<std::string>(values[0]);
        
        if (key == search_value && values.size() >= 2 && 
            std::holds_alternative<int64_t>(values[1])) {
          uint64_t rowid = static_cast<uint64_t>(std::get<int64_t>(values[1]));
          // Check for duplicates before adding
          if (std::find(rowids.begin(), rowids.end(), rowid) == rowids.end()) {
            rowids.push_back(rowid);
          }
        }
      }
      
      // Continue traversing child pages
      traverseIndexBTree(cell.page_number, search_value, rowids);
    }

    if (page.getHeader().right_most_pointer) {
      traverseIndexBTree(page.getHeader().right_most_pointer, search_value,
                         rowids);
    }
  }
}

void BTree::findRow(uint32_t page_num, uint64_t target_rowid,
                    const std::vector<int> &column_positions,
                    sqlite::QueryResult &results) const {
  _reader.seekToPage(page_num, _header.page_size);
  uint8_t page_type = _reader.readU8();
  _reader.seekToPage(page_num, _header.page_size);

  if (static_cast<PageType>(page_type) == PageType::InteriorTable) {
    LOG_INFO("Processing interior table page");
    BTreePage<PageType::InteriorTable> page(_reader, _header.page_size,
                                            page_num);
    const auto &cells = page.getCells();

    if (cells.empty()) {
      return;
    }
    
    if (target_rowid < cells[0].interior_row_id) {
      findRow(cells[0].left_pointer, target_rowid, column_positions, results);
      return;
    }

    auto it = std::lower_bound(cells.begin(), cells.end(), target_rowid,
                              [](const auto &cell, uint64_t rowid) {
                                return cell.interior_row_id < rowid;
                              });

    if (it == cells.end()) {
      findRow(page.getHeader().right_most_pointer, target_rowid, column_positions, results);
    } else {
      findRow(it->left_pointer, target_rowid, column_positions, results);
    }
  } else {
    BTreePage<PageType::LeafTable> page(_reader, _header.page_size, page_num);

    for (const auto &cell : page.getCells()) {
      if (cell.row_id == target_rowid) {
        BTreeRecord record(cell.payload);
        const auto &values = record.getValues();
        Row row;
        row.reserve(column_positions.size());

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

bool BTree::matchesWhereCondition(const std::vector<RecordValue> &values,
                                  int where_col_pos, const WhereClause &where) noexcept {
  if (where_col_pos < 0 || where_col_pos >= values.size()) {
    return false;
  }

  const auto &cell_value = values[where_col_pos];
  if (std::holds_alternative<std::string>(cell_value)) {
    const std::string &value = std::get<std::string>(cell_value);
    std::string_view where_value = where.value;

    if (where_value.front() == '\'' && where_value.back() == '\'') {
      where_value = where_value.substr(1, where_value.length() - 2);
    }

    return where.operator_type == "=" && value == where_value;
  }

  return false;
}

int64_t BTree::getIndexRootPage(const std::string &table_name,
                                const std::string &column_name) const {
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

    if (type == "index" && values.size() >= 5) {
      std::string tbl_name = std::get<std::string>(values[2]);
      std::string sql = std::get<std::string>(values[4]);

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

  return index_root_page;
}

QueryResult BTree::fetchRowsByIds(const std::vector<uint64_t> &rowids,
                                  const std::vector<std::string> &columns,
                                  const SchemaRecord &schema,
                                  const uint32_t root_page) const {
  LOG_INFO("Fetching rows by IDs, processing " << rowids.size() << " row IDs");
  LOG_DEBUG("Number of rowids to fetch: " << rowids.size());

  QueryResult results;
  results.reserve(rowids.size());
  const std::vector<int> column_positions = schema.mapColumnPositions(columns);

  std::vector<uint64_t> sorted_rowids;
  sorted_rowids.reserve(rowids.size());
  sorted_rowids = rowids;
  std::sort(sorted_rowids.begin(), sorted_rowids.end());

  for (uint64_t rowid : sorted_rowids) {
    LOG_DEBUG("Searching for rowid: " << rowid);
    findRow(root_page, rowid, column_positions, results);
  }

  LOG_INFO("Found " << results.size() << " rows");
  return results;
}
