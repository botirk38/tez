#include "btree_record.h"
#include "debug.h"

BTreeRecord::BTreeRecord(const std::vector<uint8_t> &payload)
    : reader_(payload) {
  LOG_DEBUG("Creating BTreeRecord with payload size: " << payload.size());
  parseHeader();
  parseValues();
}

const std::vector<RecordValue> &BTreeRecord::getValues() const {
  return values_;
}

const std::vector<SerialType> &BTreeRecord::getTypes() const { return types_; }

void BTreeRecord::parseHeader() {
  size_t start_pos = reader_.position();
  LOG_DEBUG("Parsing record header at position: " << start_pos);

  auto [header_size, header_varint_size] = reader_.readVarint();
  LOG_DEBUG("Header size: " << header_size << " bytes");

  size_t bytes_read = header_varint_size;
  while (bytes_read < header_size) {
    auto [serial_type, type_bytes] = reader_.readVarint();
    bytes_read = reader_.position() - start_pos;

    if (serial_type >= 12) {
      if (serial_type % 2 == 0) {
        size_t blob_size = (serial_type - 12) / 2;
        LOG_DEBUG("Found BLOB type with size: " << blob_size);
        types_.push_back(static_cast<SerialType>(serial_type));
      } else {
        size_t string_size = (serial_type - 13) / 2;
        LOG_DEBUG("Found String type with size: " << string_size);
        types_.push_back(static_cast<SerialType>(serial_type));
      }
    } else {
      LOG_DEBUG("Found basic type: " << static_cast<int>(serial_type));
      types_.push_back(static_cast<SerialType>(serial_type));
    }
  }
  LOG_INFO("Parsed " << types_.size() << " column types");
}

void BTreeRecord::parseValues() {
  LOG_DEBUG("Starting to parse values at position: " << reader_.position());
  for (auto type : types_) {
    size_t start_pos = reader_.position();
    values_.push_back(readValue(type));
    LOG_DEBUG("Read value of type " << static_cast<int>(type) << " using "
                                    << (reader_.position() - start_pos)
                                    << " bytes");
  }
  LOG_INFO("Successfully parsed " << values_.size() << " values");
}

RecordValue BTreeRecord::readValue(SerialType type) {
  switch (type) {
  case SerialType::Null:
    LOG_DEBUG("Reading NULL value");
    return std::monostate{};
  case SerialType::Int8:
    LOG_DEBUG("Reading Int8 value");
    return static_cast<int64_t>(reader_.readU8());
  case SerialType::Int16:
    LOG_DEBUG("Reading Int16 value");
    return static_cast<int64_t>(reader_.readU16());
  case SerialType::Int32:
    LOG_DEBUG("Reading Int32 value");
    return static_cast<int64_t>(reader_.readU32());
  case SerialType::Int64:
    LOG_DEBUG("Reading Int64 value");
    return static_cast<int64_t>(reader_.readU64());
  case SerialType::Float64:
    LOG_DEBUG("Reading Float64 value");
    return reader_.readDouble();
  case SerialType::Zero:
    LOG_DEBUG("Reading Zero value");
    return static_cast<int64_t>(0);
  case SerialType::One:
    LOG_DEBUG("Reading One value");
    return static_cast<int64_t>(1);
  default:
    if (static_cast<int>(type) >= 12) {
      if (static_cast<int>(type) % 2 == 0) {
        size_t size = (static_cast<int>(type) - 12) / 2;
        LOG_DEBUG("Reading BLOB of size: " << size);
        return reader_.readBytes(size);
      } else {
        size_t size = (static_cast<int>(type) - 13) / 2;
        LOG_DEBUG("Reading String of size: " << size);
        auto bytes = reader_.readBytes(size);
        return std::string(bytes.begin(), bytes.end());
      }
    }
    LOG_ERROR("Unknown serial type: " << static_cast<int>(type));
    throw std::runtime_error("Unknown serial type");
  }
}
