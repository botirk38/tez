#pragma once
#include "file_reader.hpp"
#include "schema_record.hpp"

class TableManager {
public:
  explicit TableManager(FileReader &reader, const sqlite::Header &header);

  bool isTableRecord(const std::vector<uint8_t> &payload) const;
  bool isUserTable(const SchemaRecord &record) const;
  uint32_t getTableRootPage(const std::string &table_name) const;
  uint32_t getTableRowCount(const std::string &table_name) const;
  SchemaRecord getTableSchema(const std::string &table_name) const;

private:
  FileReader &_reader;
  const sqlite::Header &_header;
};
