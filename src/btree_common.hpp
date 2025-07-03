#include <cstdint>

// Page type traits
enum class PageType : uint8_t {
  InteriorIndex = 2,
  InteriorTable = 5,
  LeafIndex = 10,
  LeafTable = 13
};

template <PageType T> struct PageTraits {
  static constexpr bool is_interior =
      (T == PageType::InteriorIndex || T == PageType::InteriorTable);
  static constexpr bool is_leaf =
      (T == PageType::LeafIndex || T == PageType::LeafTable);
  static constexpr bool is_table =
      (T == PageType::InteriorTable || T == PageType::LeafTable);
  static constexpr bool is_index =
      (T == PageType::InteriorIndex || T == PageType::LeafIndex);
};
