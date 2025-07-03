#pragma once

#include "btree.hpp"
#include "file_reader.hpp"
#include "sqlite_constants.hpp"
#include "table_manager.hpp"
#include <memory>
#include <string>

using SqliteHeader = sqlite::Header;
using QueryResult = sqlite::QueryResult;

class Database {
public:
  explicit Database(const std::string &filename);
  ~Database() = default;
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;
  Database(Database&&) = default;
  Database& operator=(Database&&) = default;

  SqliteHeader readHeader();
  uint16_t getTableCount() const;
  std::vector<std::string> getTableNames() const;
  sqlite::QueryResult executeSelect(const SelectStatement &stmt) const;

private:
  FileReader _reader;
  SqliteHeader _header;
  TableManager _table_manager;
  BTree _btree;

  sqlite::QueryResult executeCountStar(const std::string &table_name) const;
  sqlite::QueryResult executeSelectWithoutWhere(const SelectStatement &stmt) const;
  sqlite::QueryResult executeSelectWithWhere(const SelectStatement &stmt) const;
};

