#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class TokenType {
  Select,
  From,
  Where,
  Count,
  Star,
  Identifier,
  Number,
  String,
  Create,
  Table,
  LParen,
  RParen,
  Comma,
  Primary,
  Key,
  Operator,
  Eof,
  Multiply,

};

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

class Token {
public:
  Token(TokenType type, std::string value = "")
      : type_(type), value_(std::move(value)) {}

  TokenType type() const { return type_; }
  const std::string &value() const { return value_; }

private:
  TokenType type_;
  std::string value_;
};

class Lexer {
public:
  explicit Lexer(std::string input);
  Token nextToken();

private:
  void skipWhitespace();
  bool matchKeyword(const std::string &keyword, TokenType type);
  Token readIdentifier();
  Token readString();
  Token readOperator();

  std::string input_;
  size_t position_{0};
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
