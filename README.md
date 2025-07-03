# Tez SQLite Database Engine

A high-performance SQLite-compatible database engine implementation written in C++. Tez provides efficient query execution with advanced B-tree index scanning and SQL parsing capabilities.

## Features

### Core Database Operations
- **Table Scanning**: Full table traversal with WHERE clause filtering
- **Index Scanning**: Optimized B-tree index traversal for fast lookups
- **Query Execution**: Support for SELECT statements with column projection and filtering

### SQL Support
- `SELECT` statements with column selection
- `WHERE` clauses with equality operators
- `COUNT(*)` aggregate functions
- `CREATE TABLE` statement parsing
- `.tables` meta-command support

### Advanced B-tree Implementation
- **Comprehensive Index Scanning**: Scans both interior and leaf index pages
- **Efficient Row Retrieval**: Binary search-based row lookup in table B-trees
- **SQLite Format Compatibility**: Full compatibility with SQLite database files
- **Duplicate Prevention**: Automatic deduplication of results from index scans

## Architecture

### Key Components

#### B-tree Engine (`src/btree.cpp`)
- **Index Scanning**: Traverses both interior and leaf index pages to find all matching records
- **Table Scanning**: Efficient traversal of table B-tree structures
- **Row Retrieval**: Fast lookup of specific rows by row ID

#### SQL Parser (`src/sql_parser.cpp`)
- **Query Parsing**: Converts SQL strings into executable query structures
- **Schema Parsing**: Handles CREATE TABLE statements with optional column types
- **WHERE Clause Processing**: Parses and validates filtering conditions

#### Database Manager (`src/database.cpp`)
- **Query Routing**: Decides between index scan and table scan based on available indexes
- **Schema Management**: Handles table and index metadata
- **Result Assembly**: Combines data from multiple sources into final query results

#### File Reader (`src/file_reader.cpp`)
- **SQLite File Format**: Reads and interprets SQLite database file structures
- **Page Management**: Handles database page reading and navigation
- **Binary Data Processing**: Converts raw database bytes into structured data

## Technical Highlights

### Index Scan Optimization
Unlike traditional implementations that only scan leaf pages, Tez implements a comprehensive B-tree index scan that:
- Examines both interior and leaf index pages for matching keys
- Handles SQLite's unique index structure where data can exist in interior cells
- Prevents duplicate results through efficient deduplication
- Ensures complete result sets even when records are distributed across multiple page types

### Query Processing Pipeline
1. **SQL Parsing**: Convert SQL string to structured query object
2. **Index Detection**: Check for available indexes on WHERE clause columns
3. **Execution Strategy**: Choose between index scan or table scan
4. **Data Retrieval**: Fetch matching rows using optimal strategy
5. **Result Assembly**: Project requested columns and format output

## Usage

### Command Line Interface
```bash
# Execute a SELECT query
./your_program.sh database.db "SELECT id, name FROM companies WHERE country = 'eritrea'"

# List all tables
./your_program.sh database.db .tables

# Count records
./your_program.sh database.db "SELECT COUNT(*) FROM companies"
```

### Example Queries
```sql
-- Basic selection with WHERE clause
SELECT id, name FROM companies WHERE country = 'eritrea'

-- Column projection
SELECT name FROM companies WHERE country = 'usa'

-- Count aggregation
SELECT COUNT(*) FROM companies WHERE country = 'canada'
```

## Building

### Prerequisites
- C++17 compatible compiler
- CMake 3.10+
- Standard C++ libraries

### Build Steps
```bash
# Clone the repository
git clone https://github.com/botirk38/tez.git
cd tez

# Build with CMake
cmake .
make

# Run tests
./your_program.sh sample.db .tables
```

## Performance

### Index Scanning Efficiency
- **Complete Results**: Finds all matching records regardless of B-tree structure
- **Optimal Traversal**: Minimizes unnecessary page reads while ensuring completeness
- **Memory Efficiency**: Processes results incrementally without loading entire datasets

### Query Optimization
- **Automatic Index Selection**: Uses indexes when available for WHERE clauses
- **Fallback Strategy**: Gracefully falls back to table scan when indexes are unavailable
- **Result Deduplication**: Prevents duplicate records in final results

## File Structure

```
src/
├── btree.cpp              # B-tree operations and index scanning
├── btree.hpp              # B-tree interface definitions
├── database.cpp           # Database management and query execution
├── database.hpp           # Database interface
├── sql_parser.cpp         # SQL parsing and query structure
├── sql_parser.hpp         # Parser interface
├── file_reader.cpp        # SQLite file format handling
├── file_reader.hpp        # File reader interface
├── schema_record.cpp      # Database schema management
├── table_manager.cpp      # Table metadata handling
└── Server.cpp             # Main application entry point
```

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make your changes with appropriate tests
4. Commit your changes: `git commit -m "Add feature description"`
5. Push to the branch: `git push origin feature-name`
6. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by SQLite's efficient B-tree implementation
- Thanks to the CodeCrafters SQLite challenge for providing test cases and database samples
- Special recognition to the SQLite documentation for B-tree structure insights