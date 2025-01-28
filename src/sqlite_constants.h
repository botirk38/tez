#pragma once

#include <cstddef>
#include <cstdint>

namespace sqlite {

// Page and header sizes
constexpr size_t HEADER_SIZE = 100;
constexpr size_t SCHEMA_PAGE = 1;

// Record type identifiers
namespace record_type {
constexpr const char *TABLE = "table";
constexpr const char *INDEX = "index";
constexpr const char *VIEW = "view";
constexpr const char *TRIGGER = "trigger";
} // namespace record_type

// Internal table prefixes
namespace internal {
constexpr const char *PREFIX = "sqlite_";
constexpr size_t PREFIX_LENGTH = 7;
} // namespace internal

// Schema table column indices
namespace schema {
constexpr size_t TYPE = 0;
constexpr size_t NAME = 1;
constexpr size_t TBL_NAME = 2;
constexpr size_t ROOTPAGE = 3;
constexpr size_t SQL = 4;
} // namespace schema

} // namespace sqlite
