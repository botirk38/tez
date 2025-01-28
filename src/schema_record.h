#pragma once
#include "btree_record.h"

struct SchemaRecord {
  std::string type;
  std::string name;
  std::string tbl_name;
  int64_t rootpage;
  std::string sql;

  static SchemaRecord fromRecord(const BTreeRecord &record) {
    const auto &values = record.getValues();
    SchemaRecord schema;

    if (values.size() >= 5) {
      schema.type = std::get<std::string>(values[0]);
      schema.name = std::get<std::string>(values[1]);
      schema.tbl_name = std::get<std::string>(values[2]);
      schema.rootpage = std::get<int64_t>(values[3]);
      schema.sql = std::get<std::string>(values[4]);
    }

    return schema;
  }
};
