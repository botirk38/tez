#include "database.h"
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

  if (!db.isOpen()) {
    std::cerr << "Failed to open database file" << std::endl;
    return 1;
  }

  std::string command = argv[2];
  if (command == ".dbinfo") {
    SqliteHeader db_header = db.readHeader();
    std::cout << "database page size: " << db_header.page_size << std::endl;

    size_t num_tables = db.countTables();

    std::cout << "Number of tables: " << num_tables << std::endl;

  } else if (command == ".tables") {
    size_t num_tables = db.countTables();

    std::cout << "Number of tables: " << num_tables << std::endl;
  }
  return 0;
}
