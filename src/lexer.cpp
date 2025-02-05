#include "lexer.h"
#include "debug.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(std::string input) : input_(std::move(input)) {}

void Lexer::skipWhitespace() {
  while (position_ < input_.length() && std::isspace(input_[position_])) {
    position_++;
  }
}

bool Lexer::matchKeyword(const std::string &keyword, TokenType type) {
  if (position_ + keyword.length() > input_.length()) {
    return false;
  }

  for (size_t i = 0; i < keyword.length(); i++) {
    if (std::tolower(input_[position_ + i]) != std::tolower(keyword[i])) {
      return false;
    }
  }

  size_t next_pos = position_ + keyword.length();
  if (next_pos < input_.length() && std::isalnum(input_[next_pos])) {
    return false;
  }

  position_ += keyword.length();
  return true;
}

Token Lexer::readIdentifier() {
  std::string identifier;
  while (position_ < input_.length() &&
         (std::isalnum(input_[position_]) || input_[position_] == '_' ||
          input_[position_] == '*')) {
    identifier += input_[position_];
    position_++;
  }
  return Token(TokenType::Identifier, identifier);
}

Token Lexer::readString() {
  position_++; // Skip opening quote
  std::string value;
  while (position_ < input_.length() && input_[position_] != '\'') {
    value += input_[position_];
    position_++;
  }
  if (position_ < input_.length()) {
    position_++; // Skip closing quote
  }
  return Token(TokenType::String, value);
}

Token Lexer::readOperator() {
  std::string op;
  op += input_[position_];
  position_++;
  return Token(TokenType::Operator, op);
}

Token Lexer::nextToken() {
  skipWhitespace();

  if (position_ >= input_.length()) {
    return Token(TokenType::Eof);
  }

  if (input_[position_] == '(') {
    position_++;
    return Token(TokenType::LParen);
  }

  if (input_[position_] == ')') {
    position_++;
    return Token(TokenType::RParen);
  }

  if (input_[position_] == ',') {
    position_++;
    return Token(TokenType::Comma);
  }

  if (input_[position_] == '\'') {
    return readString();
  }

  if (input_[position_] == '=' || input_[position_] == '<' ||
      input_[position_] == '>') {
    return readOperator();
  }

  // Keywords
  if (matchKeyword("SELECT", TokenType::Select)) {
    return Token(TokenType::Select);
  }
  if (matchKeyword("FROM", TokenType::From)) {
    return Token(TokenType::From);
  }
  if (matchKeyword("WHERE", TokenType::Where)) {
    return Token(TokenType::Where);
  }
  if (matchKeyword("COUNT", TokenType::Count)) {
    return Token(TokenType::Count);
  }
  if (matchKeyword("CREATE", TokenType::Create)) {
    return Token(TokenType::Create);
  }
  if (matchKeyword("TABLE", TokenType::Table)) {
    return Token(TokenType::Table);
  }
  if (matchKeyword("PRIMARY", TokenType::Primary)) {
    return Token(TokenType::Primary);
  }
  if (matchKeyword("KEY", TokenType::Key)) {
    return Token(TokenType::Key);
  }

  return readIdentifier();
}

std::unique_ptr<SelectStatement>
SQLParser::parseSelect(const std::string &sql) {
  LOG_DEBUG("Parsing SELECT statement: " << sql);
  Lexer lexer(sql);
  return parseSelectStatement(lexer);
}

std::unique_ptr<CreateTableStatement>
SQLParser::parseCreate(const std::string &sql) {
  LOG_DEBUG("Parsing CREATE statement: " << sql);
  Lexer lexer(sql);
  return parseCreateStatement(lexer);
}

std::unique_ptr<SelectStatement> SQLParser::parseSelectStatement(Lexer &lexer) {
  LOG_DEBUG("Starting SELECT statement parse");
  auto stmt = std::make_unique<SelectStatement>();

  auto token = lexer.nextToken();
  if (token.type() != TokenType::Select) {
    LOG_ERROR("Expected SELECT, got: " << static_cast<int>(token.type()));
    throw std::runtime_error("Expected SELECT");
  }

  token = lexer.nextToken();
  if (token.type() == TokenType::Count) {
    LOG_DEBUG("Parsing COUNT(*) expression");
    stmt->is_count_star = true;

    token = lexer.nextToken();
    if (token.type() != TokenType::LParen) {
      LOG_ERROR("Expected (, got: " << static_cast<int>(token.type()));
      throw std::runtime_error("Expected ( after COUNT");
    }

    token = lexer.nextToken();
    if (token.value() != "*") {
      LOG_ERROR("Expected *, got: " << token.value());
      throw std::runtime_error("Expected * in COUNT(*)");
    }

    token = lexer.nextToken();
    if (token.type() != TokenType::RParen) {
      LOG_ERROR("Expected ), got: " << static_cast<int>(token.type()));
      throw std::runtime_error("Expected ) after *");
    }

    token = lexer.nextToken();
    if (token.type() != TokenType::From) {
      LOG_ERROR("Expected FROM, got: " << static_cast<int>(token.type()));
      throw std::runtime_error("Expected FROM after COUNT(*)");
    }
  } else {
    LOG_DEBUG("Parsing column list");
    while (true) {
      if (token.type() == TokenType::Identifier) {
        LOG_DEBUG("Found column: " << token.value());
        stmt->column_names.push_back(token.value());
      } else if (token.type() == TokenType::From) {
        break;
      } else {
        LOG_ERROR("Expected column name or FROM, got: "
                  << static_cast<int>(token.type()));
        throw std::runtime_error("Expected column name or FROM");
      }

      token = lexer.nextToken();
      if (token.type() == TokenType::From) {
        break;
      }
      if (token.type() != TokenType::Comma) {
        LOG_ERROR("Expected comma, got: " << static_cast<int>(token.type()));
        throw std::runtime_error("Expected comma between columns");
      }
      token = lexer.nextToken();
    }
  }

  token = lexer.nextToken();
  if (token.type() != TokenType::Identifier) {
    LOG_ERROR("Expected table name, got: " << static_cast<int>(token.type()));
    throw std::runtime_error("Expected table name");
  }
  LOG_DEBUG("Found table name: " << token.value());
  stmt->table_name = token.value();

  token = lexer.nextToken();
  if (token.type() == TokenType::Where) {
    LOG_DEBUG("Parsing WHERE clause");
    stmt->where_clause = parseWhereClause(lexer);
  }

  LOG_DEBUG("Completed parsing SELECT statement");
  return stmt;
}

std::unique_ptr<CreateTableStatement>
SQLParser::parseCreateStatement(Lexer &lexer) {
  LOG_DEBUG("Starting CREATE TABLE statement parse");
  auto stmt = std::make_unique<CreateTableStatement>();

  auto token = lexer.nextToken();
  while (token.type() != TokenType::Identifier) {
    token = lexer.nextToken();
  }

  LOG_DEBUG("Found table name: " << token.value());
  stmt->table_name = token.value();

  token = lexer.nextToken();
  while (token.type() != TokenType::LParen) {
    token = lexer.nextToken();
  }

  LOG_DEBUG("Parsing column definitions");
  while (true) {
    token = lexer.nextToken();
    if (token.type() == TokenType::RParen) {
      break;
    }

    if (token.type() == TokenType::Identifier) {
      Column col;
      col.name = token.value();
      LOG_DEBUG("Found column name: " << col.name);

      token = lexer.nextToken();
      if (token.type() != TokenType::Identifier) {
        LOG_ERROR(
            "Expected column type, got: " << static_cast<int>(token.type()));
        throw std::runtime_error("Expected column type");
      }
      col.type = token.value();
      LOG_DEBUG("Found column type: " << col.type);

      stmt->columns.push_back(std::move(col));

      token = lexer.nextToken();
      while (token.type() != TokenType::Comma &&
             token.type() != TokenType::RParen) {
        token = lexer.nextToken();
      }

      if (token.type() == TokenType::RParen) {
        break;
      }
    }
  }

  LOG_DEBUG("Completed parsing CREATE TABLE statement");
  return stmt;
}

std::optional<WhereClause> SQLParser::parseWhereClause(Lexer &lexer) {
  LOG_DEBUG("Starting WHERE clause parse");
  WhereClause clause;

  auto token = lexer.nextToken();
  if (token.type() != TokenType::Identifier) {
    LOG_ERROR("Expected column name, got: " << static_cast<int>(token.type()));
    throw std::runtime_error("Expected column name in WHERE clause");
  }
  LOG_DEBUG("Found WHERE column: " << token.value());
  clause.column = token.value();

  token = lexer.nextToken();
  if (token.type() != TokenType::Operator) {
    LOG_ERROR("Expected operator, got: " << static_cast<int>(token.type()));
    throw std::runtime_error("Expected operator in WHERE clause");
  }
  LOG_DEBUG("Found WHERE operator: " << token.value());
  clause.operator_type = token.value();

  token = lexer.nextToken();
  if (token.type() != TokenType::Identifier &&
      token.type() != TokenType::String) {
    LOG_ERROR("Expected value, got: " << static_cast<int>(token.type()));
    throw std::runtime_error("Expected value in WHERE clause");
  }
  LOG_DEBUG("Found WHERE value: " << token.value());
  clause.value = token.value();

  LOG_DEBUG("Completed parsing WHERE clause");
  return clause;
}
