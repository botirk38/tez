#pragma once
#include "btree_common.h"
#include "byte_reader.h"
#include "file_reader.h"
#include <cstdint>
#include <type_traits>
#include <variant>
#include <vector>

template <PageType T> class BTreeCell {
public:
  // Core cell data structure
  struct Data {
    std::conditional_t<PageTraits<T>::is_interior, uint32_t, std::monostate>
        page_number{};
    std::vector<uint8_t> payload{};
    std::conditional_t<PageTraits<T>::is_table && PageTraits<T>::is_leaf,
                       uint32_t, std::monostate>
        row_id{};
  };

  explicit BTreeCell(FileReader &reader) : reader_(reader) {}

  [[nodiscard]] auto read() -> Data {
    Data cell{};

    if constexpr (PageTraits<T>::is_interior) {
      readInteriorCell(cell);
    } else {
      readLeafCell(cell);
    }

    return cell;
  }

private:
  void readInteriorCell(Data &cell) {
    cell.page_number = reader_.readU32();
    auto [payload_size, _] = reader_.readVarint();
    cell.payload.resize(payload_size);
    reader_.readBytes(cell.payload.data(), payload_size);
  }

  void readLeafCell(Data &cell) {
    auto [payload_size, _] = reader_.readVarint();

    if constexpr (PageTraits<T>::is_table && PageTraits<T>::is_leaf) {
      auto [row_id, row_id_bytes] = reader_.readVarint();
      cell.row_id = static_cast<uint32_t>(row_id);
    }

    cell.payload.resize(payload_size);
    reader_.readBytes(cell.payload.data(), payload_size);
  }

  FileReader &reader_;
};
