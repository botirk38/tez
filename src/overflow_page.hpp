#pragma once

#include "file_reader.hpp"
#include "sqlite_constants.hpp"
#include <cstdint>
#include <vector>

class OverflowPage {
public:
  explicit OverflowPage(const FileReader &reader, uint16_t page_size)
      : reader_(reader), page_size_(page_size) {}

  struct Data {
    uint32_t next_page;
    std::vector<uint8_t> content;
  };

  [[nodiscard]] auto read() -> Data {
    Data overflow;

    // Read next page pointer (4 bytes big-endian)
    overflow.next_page = reader_.readU32();

    // Calculate usable space for content
    // Page size minus 4 bytes for next page pointer
    uint32_t content_size = page_size_ - 4;

    // Read content
    overflow.content.resize(content_size);
    reader_.readBytes(overflow.content.data(), content_size);

    return overflow;
  }

  // Helper to read entire overflow chain
  [[nodiscard]] static auto
  readOverflowChain(const FileReader &reader, uint16_t page_size, uint32_t first_page)
      -> std::vector<uint8_t> {
    std::vector<uint8_t> complete_content;
    uint32_t current_page = first_page;

    while (current_page != 0) {
      // Seek to current overflow page

      // Calculate correct page offset

      // Seek to start of current overflow page
      reader.seekToPage(current_page, page_size);

      // Read this overflow page
      OverflowPage overflow_reader(reader, page_size);
      auto page_data = overflow_reader.read();

      // Append content
      complete_content.insert(complete_content.end(), page_data.content.begin(),
                              page_data.content.end());

      // Move to next page
      current_page = page_data.next_page;
    }

    return complete_content;
  }

private:
  const FileReader &reader_;
  const uint16_t page_size_;
};
