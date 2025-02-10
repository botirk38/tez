#include "lexer.h"
#include <cctype>

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
