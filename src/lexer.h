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
