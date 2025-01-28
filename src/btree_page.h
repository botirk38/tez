#pragma once
#include "btree_cell.h"
#include "debug.h"
#include "file_reader.h"
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <variant>
#include <vector>

template <PageType T> class BTreePage {
public:
  using Cell = typename BTreeCell<T>::Data;

  struct Header {
    static constexpr PageType type = T;
    uint16_t first_freeblock{};
    uint16_t cell_count{};
    uint16_t cell_content_start{};
    uint8_t fragmented_free_bytes{};
    std::conditional_t<PageTraits<T>::is_interior, uint32_t, std::monostate>
        right_most_pointer{};
  };

  explicit BTreePage(FileReader &reader, uint16_t page_size)
      : reader_(reader), page_size_(page_size) {
    LOG_INFO("Creating BTreePage with page size: " << page_size);
    parseHeader();
    readCellPointers();
  }

  [[nodiscard]] auto getHeader() const noexcept -> const Header & {
    LOG_DEBUG("Accessing page header");
    return header_;
  }

  [[nodiscard]] auto getCells() const noexcept -> const std::vector<Cell> & {
    LOG_DEBUG("Accessing cells, count: " << cells_.size());
    return cells_;
  }

  static constexpr auto isLeaf() noexcept -> bool {
    return PageTraits<T>::is_leaf;
  }

  static constexpr auto isInterior() noexcept -> bool {
    return PageTraits<T>::is_interior;
  }

private:
  void parseHeader() {
    LOG_DEBUG("Parsing page header at position: " << reader_.position());

    uint8_t type_byte = reader_.readU8();
    if (static_cast<PageType>(type_byte) != T) {
      LOG_ERROR("Page type mismatch. Expected: "
                << static_cast<int>(T)
                << ", Got: " << static_cast<int>(type_byte));
      throw std::runtime_error("Page type mismatch");
    }

    header_.first_freeblock = reader_.readU16();
    header_.cell_count = reader_.readU16();
    header_.cell_content_start = reader_.readU16();
    header_.fragmented_free_bytes = reader_.readU8();

    LOG_INFO("Page header parsed: cells=" << header_.cell_count
                                          << ", content_start="
                                          << header_.cell_content_start);

    if constexpr (PageTraits<T>::is_interior) {
      header_.right_most_pointer = reader_.readU32();
      LOG_DEBUG(
          "Interior page right_most_pointer: " << header_.right_most_pointer);
    }
  }

  void readCellPointers() {
    LOG_DEBUG("Reading cell pointers, count: " << header_.cell_count);

    std::vector<uint16_t> cell_pointers;
    cell_pointers.reserve(header_.cell_count);

    for (uint16_t i = 0; i < header_.cell_count; ++i) {
      auto pointer = reader_.readU16();
      LOG_DEBUG("Cell pointer " << i << " at offset: " << pointer);
      cell_pointers.push_back(pointer);
    }

    cells_.reserve(header_.cell_count);
    for (auto pointer : cell_pointers) {
      LOG_DEBUG("Seeking to cell at offset: " << pointer);
      reader_.seek(pointer);
      readCell();
    }

    LOG_INFO("Read " << cells_.size() << " cells successfully");
  }

  void readCell() {
    LOG_DEBUG("Reading cell at position: " << reader_.position());
    BTreeCell<T> cell_reader(reader_);
    cells_.push_back(cell_reader.read());
  }

  FileReader &reader_;
  const uint32_t page_size_;
  Header header_{};
  std::vector<Cell> cells_{};
};

