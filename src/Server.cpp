#include "database.h"
#include <cstdint>
#include <iostream>

int main(int argc, char *argv[]) {
  // Set stdout and stderr to flush immediately
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  if (argc != 3) {
    std::cerr << "Expected two arguments" << std::endl;
    return 1;
  }

  // Create database instance with provided file path
  Database db(argv[1]);

  std::string command = argv[2];
  if (command == ".dbinfo") {
    SqliteHeader db_header = db.readHeader();
    std::cout << "database page size: " << db_header.page_size << std::endl;

    uint16_t num_tables = db.getTableCount();

    std::cout << "number of tables: " << num_tables << std::endl;

  } else if (command == ".tables") {
  }
  return 0;
}
