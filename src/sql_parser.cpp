#include "sql_parser.h"
#include "debug.h"

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

  // Skip CREATE TABLE keywords
  auto token = lexer.nextToken();
  LOG_DEBUG("Parsing CREATE TABLE tokens");
  while (token.type() != TokenType::Table) {
    LOG_DEBUG("Skipping token: " << static_cast<int>(token.type()));
    token = lexer.nextToken();
  }

  // Get table name
  token = lexer.nextToken();
  LOG_DEBUG(
      "Looking for table name token, got: " << static_cast<int>(token.type()));
  if (token.type() != TokenType::Identifier &&
      token.type() != TokenType::String) {
    LOG_ERROR("Expected table name, got: " << static_cast<int>(token.type()));
    throw std::runtime_error("Expected table name");
  }
  stmt->table_name = token.value();
  LOG_DEBUG("Found table name: " << stmt->table_name);

  // Expect opening parenthesis
  token = lexer.nextToken();
  LOG_DEBUG("Looking for opening parenthesis, got: "
            << static_cast<int>(token.type()));
  if (token.type() != TokenType::LParen) {
    LOG_ERROR("Expected opening parenthesis, got: "
              << static_cast<int>(token.type()));
    throw std::runtime_error("Expected ( after table name");
  }

  // Parse column definitions
  LOG_DEBUG("Starting column definitions parse");
  while (true) {
    token = lexer.nextToken();
    LOG_DEBUG("Processing token: " << static_cast<int>(token.type()));

    if (token.type() == TokenType::RParen) {
      LOG_DEBUG("Found closing parenthesis, ending column definitions");
      break;
    }

    Column col;

    // Column name
    if (token.type() != TokenType::Identifier) {
      LOG_ERROR(
          "Expected column name, got: " << static_cast<int>(token.type()));
      throw std::runtime_error("Expected column name");
    }
    col.name = token.value();
    LOG_DEBUG("Parsing column name: " << col.name);

    // Column type
    token = lexer.nextToken();
    LOG_DEBUG(
        "Looking for column type, got: " << static_cast<int>(token.type()));
    if (token.type() != TokenType::Identifier) {
      LOG_ERROR(
          "Expected column type, got: " << static_cast<int>(token.type()));
      throw std::runtime_error("Expected column type");
    }
    col.type = token.value();
    LOG_DEBUG("Found column type: " << col.type);

    // Parse additional column constraints
    LOG_DEBUG("Parsing column constraints");
    token = lexer.nextToken();
    while (token.type() != TokenType::Comma &&
           token.type() != TokenType::RParen) {
      LOG_DEBUG(
          "Processing constraint token: " << static_cast<int>(token.type()));
      if (token.type() == TokenType::Primary ||
          token.type() == TokenType::Key ||
          token.type() == TokenType::Identifier) {
        token = lexer.nextToken();
        continue;
      }
      LOG_ERROR(
          "Invalid column constraint: " << static_cast<int>(token.type()));
      throw std::runtime_error("Invalid column constraint");
    }

    LOG_DEBUG("Adding column to statement: " << col.name);
    stmt->columns.push_back(std::move(col));

    if (token.type() == TokenType::RParen) {
      LOG_DEBUG("Found closing parenthesis, ending column definitions");
      break;
    }
  }

  LOG_DEBUG("Successfully completed parsing CREATE TABLE statement");
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
