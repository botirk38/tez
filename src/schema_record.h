#pragma once
#include "btree_record.h"
#include "sql_parser.h"

struct ColumnInfo {
  std::string name;
  std::string type;
  int position;
};

struct SchemaRecord {
  std::string type;
  std::string name;
  std::string tbl_name;
  int64_t rootpage;
  std::string sql;
  std::vector<ColumnInfo> columns;

  void parseColumns() {
    if (sql.empty())
      return;

    auto create_stmt = SQLParser::parseCreate(sql);
    for (size_t i = 0; i < create_stmt->columns.size(); i++) {
      const auto &col = create_stmt->columns[i];
      columns.push_back({col.name, col.type, static_cast<int>(i)});
    }
  }

  static SchemaRecord fromRecord(const BTreeRecord &record) {
    const auto &values = record.getValues();
    SchemaRecord schema;

    if (values.size() >= 5) {
      schema.type = std::get<std::string>(values[0]);
      schema.name = std::get<std::string>(values[1]);
      schema.tbl_name = std::get<std::string>(values[2]);
      schema.rootpage = std::get<int64_t>(values[3]);
      schema.sql = std::get<std::string>(values[4]);

      schema.parseColumns();
    }

    return schema;
  }
};
