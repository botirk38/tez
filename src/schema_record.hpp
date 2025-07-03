#pragma once
#include "btree_record.hpp"
#include "sql_parser.hpp"
#include <string>
#include <vector>

class ColumnInfo {
public:
  std::string name;
  std::string type;
  int position;
};

class SchemaRecord {
public:
  explicit SchemaRecord(const BTreeRecord &record);

  std::vector<int>
  mapColumnPositions(const std::vector<std::string> &column_names) const;
  int findWhereColumnPosition(const std::string &column_name) const;

  // Getters
  const std::string &getType() const { return type; }
  const std::string &getName() const { return name; }
  const std::string &getTableName() const { return tbl_name; }
  int64_t getRootPage() const { return rootpage; }
  const std::string &getSql() const { return sql; }
  const std::vector<ColumnInfo> &getColumns() const { return columns; }

private:
  std::string type;
  std::string name;
  std::string tbl_name;
  int64_t rootpage;
  std::string sql;
  std::vector<ColumnInfo> columns;

  void parseColumns();
};
