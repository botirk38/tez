#pragma once
#include "btree_common.h"
#include "byte_reader.h"
#include "debug.h"
#include "file_reader.h"
#include "overflow_page.h"
#include <cstdint>
#include <type_traits>
#include <variant>
#include <vector>

template <PageType T> class BTreeCell {
public:
  struct Data {
    std::conditional_t<PageTraits<T>::is_interior, uint32_t, std::monostate>
        page_number{};
    std::vector<uint8_t> payload{};
    std::conditional_t<PageTraits<T>::is_table && PageTraits<T>::is_leaf,
                       uint32_t, std::monostate>
        row_id{};
  };

  explicit BTreeCell(FileReader &reader, uint16_t page_size)
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
    cell.page_number = reader_.readU32();
    auto [payload_size, _] = reader_.readVarint();
    LOG_DEBUG("Interior cell page number: "
              << cell.page_number << ", payload size: " << payload_size);

    auto initial_payload = readPayload(payload_size);
    cell.payload = std::move(initial_payload);
    LOG_DEBUG("Completed reading interior cell");
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
    uint32_t X, M;

    if constexpr (PageTraits<T>::is_table && PageTraits<T>::is_leaf) {
      X = usable_size - 35;
    } else {
      X = ((usable_size - 12) * 64 / 255) - 23;
    }
    M = ((usable_size - 12) * 32 / 255) - 23;

    uint32_t local_size;
    if (total_size <= X) {
      local_size = total_size;
    } else {
      uint32_t K = M + ((total_size - M) % (usable_size - 4));
      local_size = (K <= X) ? K : M;
    }

    LOG_DEBUG("Local payload size: " << local_size);
    std::vector<uint8_t> payload;
    payload.resize(local_size);
    reader_.readBytes(payload.data(), local_size);

    if (total_size > local_size) {
      uint32_t overflow_page = reader_.readU32();
      LOG_INFO("Reading overflow page: " << overflow_page);

      auto overflow_content =
          OverflowPage::readOverflowChain(reader_, page_size_, overflow_page);
      LOG_DEBUG("Read overflow content size: " << overflow_content.size());

      payload.insert(payload.end(), overflow_content.begin(),
                     overflow_content.end());
    }

    LOG_DEBUG("Completed reading payload, final size: " << payload.size());
    return payload;
  }

  FileReader &reader_;
  const uint16_t page_size_;
};

