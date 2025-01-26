#pragma once

#include "byte_reader.h"
#include <memory>
#include <vector>

enum class PageType : uint8_t {
  INTERIOR_INDEX = 0x02,
  INTERIOR_TABLE = 0x05,
  LEAF_INDEX = 0x0a,
  LEAF_TABLE = 0x0d
};

// Base interfaces
class BTreeCell {
public:
  virtual ~BTreeCell() = default;
  virtual size_t getSize() const = 0;
};

class BTreePage {
public:
  virtual ~BTreePage() = default;
  virtual std::vector<std::unique_ptr<BTreeCell>> getCells() = 0;
  virtual bool isLeaf() const = 0;
  virtual bool isTable() const = 0;
};

// Concrete cell implementations
class TableLeafCell : public BTreeCell {
public:
  TableLeafCell(ByteReader &reader);
  size_t getSize() const override;

  int64_t rowId;
  std::vector<uint8_t> payload;
  uint32_t overflowPage;
};

class TableInteriorCell : public BTreeCell {
public:
  TableInteriorCell(ByteReader &reader);
  size_t getSize() const override;

  uint32_t leftChildPage;
  int64_t key;
};

class IndexLeafCell : public BTreeCell {
public:
  IndexLeafCell(ByteReader &reader);
  size_t getSize() const override;

  std::vector<uint8_t> key;
  uint32_t overflowPage;
};

class IndexInteriorCell : public BTreeCell {
public:
  IndexInteriorCell(ByteReader &reader);
  size_t getSize() const override;

  uint32_t leftChildPage;
  std::vector<uint8_t> key;
  uint32_t overflowPage;
};

template <typename CellType> class BTreePageImpl : public BTreePage {
public:
  // B-tree page header offsets as per SQLite format spec
  static constexpr uint16_t OFFSET_CELL_COUNT = 3;
  static constexpr uint16_t OFFSET_CELL_CONTENT = 5;
  static constexpr uint16_t HEADER_SIZE = 8; // For leaf pages
  //
  explicit BTreePageImpl(std::vector<uint8_t> data) : reader_(data) {
    // Read page header
    PageType pageType = static_cast<PageType>(reader_.readU8());
    uint16_t cellPointerOffset = HEADER_SIZE; // Header size for leaf pages

    // Read number of cells
    reader_.seek(OFFSET_CELL_COUNT);
    cellCount_ = reader_.readU16();

    // Read cell content area offset
    reader_.seek(OFFSET_CELL_CONTENT);
    uint16_t cellContentOffset = reader_.readU16();
    if (cellContentOffset == 0)
      cellContentOffset = std::numeric_limits<uint16_t>::max();

    // Read cell pointers
    std::vector<uint16_t> cellPointers(cellCount_);
    reader_.seek(cellPointerOffset);
    for (uint16_t i = 0; i < cellCount_; i++) {
      cellPointers[i] = reader_.readU16();
    }

    // Create cells by reading from their offsets
    cells_.reserve(cellCount_);
    for (uint16_t offset : cellPointers) {
      reader_.seek(offset);
      cells_.push_back(std::make_unique<CellType>(reader_));
    }
  }

  std::vector<std::unique_ptr<BTreeCell>> getCells() override {
    std::vector<std::unique_ptr<BTreeCell>> result;
    result.reserve(cells_.size());
    for (auto &cell : cells_) {
      result.push_back(std::move(cell));
    }
    return result;
  }

  bool isLeaf() const override {
    return static_cast<PageType>(reader_.getData()[0]) ==
               PageType::LEAF_TABLE ||
           static_cast<PageType>(reader_.getData()[0]) == PageType::LEAF_INDEX;
  }

  bool isTable() const override {
    return static_cast<PageType>(reader_.getData()[0]) ==
               PageType::LEAF_TABLE ||
           static_cast<PageType>(reader_.getData()[0]) ==
               PageType::INTERIOR_TABLE;
  }

private:
  ByteReader reader_;
  uint16_t cellCount_;
  std::vector<std::unique_ptr<CellType>> cells_;
};

using TableLeafPage = BTreePageImpl<TableLeafCell>;
using TableInteriorPage = BTreePageImpl<TableInteriorCell>;
using IndexLeafPage = BTreePageImpl<IndexLeafCell>;
using IndexInteriorPage = BTreePageImpl<IndexInteriorCell>;
