#pragma once

#include "btree.h"
#include "file_reader.h"
#include "sqlite_constants.h"
#include "table_manager.h"
#include <string>

using SqliteHeader = sqlite::Header;
using QueryResult = sqlite::QueryResult;

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
  BTree _btree;

  sqlite::QueryResult executeCountStar(const std::string &table_name);
  sqlite::QueryResult executeSelectWithoutWhere(const SelectStatement &stmt);
  sqlite::QueryResult executeSelectWithWhere(const SelectStatement &stmt);
};

