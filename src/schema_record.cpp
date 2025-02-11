#include "schema_record.h"

SchemaRecord::SchemaRecord(const BTreeRecord &record) {
  const auto &values = record.getValues();

  if (values.size() >= 5) {
    type = std::get<std::string>(values[0]);
    name = std::get<std::string>(values[1]);
    tbl_name = std::get<std::string>(values[2]);
    rootpage = std::get<int64_t>(values[3]);
    sql = std::get<std::string>(values[4]);
    parseColumns();
  }
}

void SchemaRecord::parseColumns() {
  if (sql.empty()) {
    return;
  }

  auto create_stmt = SQLParser::parseCreate(sql);
  for (size_t i = 0; i < create_stmt->columns.size(); i++) {
    const auto &col = create_stmt->columns[i];
    columns.push_back({col.name, col.type, static_cast<int>(i)});
  }
}

std::vector<int> SchemaRecord::mapColumnPositions(
    const std::vector<std::string> &column_names) const {
  std::vector<int> positions;

  for (const auto &col_name : column_names) {
    if (col_name == "id") {
      positions.push_back(-1);
      continue;
    }

    for (const auto &col_info : columns) {
      if (col_info.name == col_name) {
        positions.push_back(col_info.position);
        break;
      }
    }
  }

  return positions;
}

int SchemaRecord::findWhereColumnPosition(
    const std::string &column_name) const {
  for (const auto &col_info : columns) {
    if (col_info.name == column_name) {
      return col_info.position;
    }
  }
  return -1;
}
