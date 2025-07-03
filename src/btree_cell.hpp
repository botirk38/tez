#pragma once
#include "btree_common.hpp"
#include "byte_reader.hpp"
#include "debug.hpp"
#include "file_reader.hpp"
#include "overflow_page.hpp"
#include <cstdint>
#include <type_traits>
#include <variant>
#include <vector>

template <PageType T> class BTreeCell {
public:
  struct Data {
    // For interior table pages, we need both left pointer and row_id
    std::conditional_t<PageTraits<T>::is_interior && PageTraits<T>::is_table,
                       uint32_t, std::monostate>
        left_pointer{};

    std::conditional_t<PageTraits<T>::is_interior && PageTraits<T>::is_table,
                       uint64_t, std::monostate>
        interior_row_id{};

    // For interior index pages
    std::conditional_t<PageTraits<T>::is_interior && !PageTraits<T>::is_table,
                       uint32_t, std::monostate>
        page_number{};

    std::vector<uint8_t> payload{};

    // For leaf table pages
    std::conditional_t<PageTraits<T>::is_table && PageTraits<T>::is_leaf,
                       uint32_t, std::monostate>
        row_id{};
  };

  explicit BTreeCell(const FileReader &reader, uint16_t page_size)
      : reader_(reader), page_size_(page_size) {
    LOG_DEBUG("Created BTreeCell with page size: " << page_size);
  }

  [[nodiscard]] auto read() -> Data {
    LOG_DEBUG("Reading BTreeCell");
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
    LOG_DEBUG("Reading interior cell");
    if constexpr (PageTraits<T>::is_table) {
      cell.left_pointer = reader_.readU32();
      auto [row_id, _] = reader_.readVarint();
      cell.interior_row_id = row_id;
      LOG_DEBUG("Interior table cell: left_pointer="
                << cell.left_pointer << ", row_id=" << cell.row_id);
    } else {
      cell.page_number = reader_.readU32();
      auto [payload_size, _] = reader_.readVarint();
      auto initial_payload = readPayload(payload_size);
      cell.payload = std::move(initial_payload);
      LOG_DEBUG("Interior index cell: page_number=" << cell.page_number);
    }
  }
  void readLeafCell(Data &cell) {
    LOG_DEBUG("Reading leaf cell");
    auto [payload_size, _] = reader_.readVarint();

    if constexpr (PageTraits<T>::is_table && PageTraits<T>::is_leaf) {
      auto [row_id, row_id_bytes] = reader_.readVarint();
      cell.row_id = static_cast<uint32_t>(row_id);
      LOG_DEBUG("Leaf cell row ID: " << cell.row_id
                                     << ", payload size: " << payload_size);
    }

    auto initial_payload = readPayload(payload_size);
    cell.payload = std::move(initial_payload);
    LOG_DEBUG("Completed reading leaf cell");
  }

  [[nodiscard]] auto readPayload(uint64_t total_size) -> std::vector<uint8_t> {
    LOG_DEBUG("Reading payload of size: " << total_size);
    uint32_t usable_size = page_size_;

    // Calculate thresholds for overflow
    uint32_t X = PageTraits<T>::is_leaf ? (usable_size - 35)
                                        : ((usable_size - 12) * 64 / 255) - 23;
    uint32_t M = ((usable_size - 12) * 32 / 255) - 23;

    // Calculate local payload size
    uint32_t local_size;
    if (total_size <= X) {
      local_size = total_size;
    } else {
      uint32_t K = M + ((total_size - M) % (usable_size - 4));
      local_size = (K <= X) ? K : M;
    }

    LOG_DEBUG("Local payload size: " << local_size);

    // Read initial payload
    std::vector<uint8_t> payload;
    payload.resize(local_size);
    reader_.readBytes(payload.data(), local_size);

    // Handle overflow if needed
    if (local_size < total_size) {
      // Read overflow page pointer
      uint32_t overflow_page = reader_.readU32();
      LOG_DEBUG("Reading overflow chain starting at page: " << overflow_page);

      // Read overflow chain
      auto overflow_content =
          OverflowPage::readOverflowChain(reader_, page_size_, overflow_page);

      // Append overflow content
      size_t remaining = total_size - local_size;
      payload.insert(payload.end(), overflow_content.begin(),
                     overflow_content.begin() + remaining);
    }

    LOG_DEBUG("Complete payload size: " << payload.size());
    return payload;
  }
  const FileReader &reader_;
  const uint16_t page_size_;
};
