#pragma once
#include "byte_reader.hpp"
#include <string>
#include <variant>
#include <vector>

enum class SerialType {
  Null = 0,
  Int8 = 1,
  Int16 = 2,
  Int24 = 3,
  Int32 = 4,
  Int48 = 5,
  Int64 = 6,
  Float64 = 7,
  Zero = 8,
  One = 9,
  Reserved1 = 10,
  Reserved2 = 11
};

using RecordValue = std::variant<std::monostate,      // NULL
                                 int64_t,             // Integer types
                                 double,              // Float64
                                 std::string,         // Text
                                 std::vector<uint8_t> // BLOB
                                 >;

class BTreeRecord {
public:
  explicit BTreeRecord(const std::vector<uint8_t> &payload);

  [[nodiscard]] const std::vector<RecordValue> &getValues() const;
  [[nodiscard]] const std::vector<SerialType> &getTypes() const;

private:
  void parseHeader();
  void parseValues();
  RecordValue readValue(SerialType type);

  ByteReader reader_;
  std::vector<SerialType> types_;
  std::vector<RecordValue> values_;
};
