#pragma once

#include "lexer.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Column {
  std::string name;
  std::string type;
};

struct CreateTableStatement {
  std::string table_name;
  std::vector<Column> columns;
};

struct WhereClause {
  std::string column;
  std::string operator_type;
  std::string value;
};

struct SelectStatement {
  std::string table_name;
  std::vector<std::string> column_names;
  bool is_count_star{false};
  std::optional<WhereClause> where_clause;
};

class SQLParser {
public:
  static std::unique_ptr<SelectStatement> parseSelect(const std::string &sql);
  static std::unique_ptr<CreateTableStatement>
  parseCreate(const std::string &sql);

private:
  static std::unique_ptr<SelectStatement> parseSelectStatement(Lexer &lexer);
  static std::unique_ptr<CreateTableStatement>
  parseCreateStatement(Lexer &lexer);
  static std::optional<WhereClause> parseWhereClause(Lexer &lexer);
};
