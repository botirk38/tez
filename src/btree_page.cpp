#include "btree_page.h"

TableLeafCell::TableLeafCell(ByteReader &reader) {
  // First read the total payload size
  auto [payloadSize, bytesRead1] = reader.readVarint();

  // Then read the rowid
  auto [rowId, bytesRead2] = reader.readVarint();
  this->rowId = rowId;

  // Calculate payload bytes to store on this page
  size_t U = 4096;                       // Page size
  size_t X = U - 35;                     // Maximum payload
  size_t M = ((U - 12) * 32 / 255) - 23; // Minimum payload
  size_t K = M + ((payloadSize - M) % (U - 4));

  // Determine how much payload to read
  size_t localPayloadSize;
  if (payloadSize <= X) {
    localPayloadSize = payloadSize;
  } else {
    localPayloadSize = (K <= X) ? K : M;
  }

  // Read the local payload
  this->payload.resize(localPayloadSize);
  reader.readBytes(this->payload.data(), localPayloadSize);

  // If there's overflow, read the overflow page number
  if (payloadSize > localPayloadSize) {
    this->overflowPage = reader.readU32();
  } else {
    this->overflowPage = 0;
  }
}

size_t TableLeafCell::getSize() const { return payload.size(); }

TableInteriorCell::TableInteriorCell(ByteReader &reader) {
  leftChildPage = reader.readU32();
  auto [key, bytesRead] = reader.readVarint();
  this->key = key;
}

size_t TableInteriorCell::getSize() const {
  return sizeof(leftChildPage) + sizeof(key);
}

IndexLeafCell::IndexLeafCell(ByteReader &reader) {
  auto [keySize, bytesRead] = reader.readVarint();

  key.resize(keySize);
  reader.readBytes(key.data(), reader.position(), keySize);

  // Handle overflow if needed
  this->overflowPage = 0;
}

size_t IndexLeafCell::getSize() const { return key.size(); }

IndexInteriorCell::IndexInteriorCell(ByteReader &reader) {
  leftChildPage = reader.readU32();
  auto [keySize, bytesRead] = reader.readVarint();

  key.resize(keySize);
  reader.readBytes(key.data(), reader.position(), keySize);

  // Handle overflow if needed
  this->overflowPage = 0;
}

size_t IndexInteriorCell::getSize() const {
  return sizeof(leftChildPage) + key.size();
}
