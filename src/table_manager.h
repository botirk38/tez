#pragma once
#include "file_reader.h"
#include "schema_record.h"

class TableManager {
public:
  explicit TableManager(FileReader &reader, const sqlite::Header &header);

  bool isTableRecord(const std::vector<uint8_t> &payload);
  bool isUserTable(const SchemaRecord &record);
  uint32_t getTableRootPage(const std::string &table_name);
  uint32_t getTableRowCount(const std::string &table_name);
  SchemaRecord getTableSchema(const std::string &table_name);

private:
  FileReader &_reader;
  const sqlite::Header &_header;
};
