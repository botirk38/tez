# <p align="center">🚀 Tez</p>

<p align="center">
    <em>A lightweight embedded database written in C++</em>
</p>

<p align="center">
    <img src="https://img.shields.io/github/license/botirk38/tez?style=default&logo=opensourceinitiative&logoColor=white&color=0080ff" alt="license">
    <img src="https://img.shields.io/github/last-commit/botirk38/tez?style=default&logo=git&logoColor=white&color=0080ff" alt="last-commit">
    <img src="https://img.shields.io/github/languages/top/botirk38/tez?style=default&color=0080ff" alt="repo-top-language">
    <img src="https://img.shields.io/github/languages/count/botirk38/tez?style=default&color=0080ff" alt="repo-language-count">
</p>

## Table of Contents
- [Overview](#overview)
- [Getting Started](#getting-started)
- [Installation](#installation)
- [Building & Deployment](#building--deployment)
- [Contributing](#contributing)
- [License](#license)

## Overview
### What is Tez?
**Tez** is a **C++ database system** built from the ground up. It includes:
- A **server** that handles database connections and queries
- A **custom SQL parser** to process structured queries
- A **storage engine** for managing data efficiently

This project is designed to be lightweight, fast, and self-contained.

## Getting Started
To try **Tez**, follow these steps:

1. Clone the repository:
   ```sh
   git clone https://github.com/botirk38/tez.git
   cd tez
   ```
2. Build the project:
   ```sh
   cmake -B build
   cmake --build build
   ```
3. Start the database server:
   ```sh
   ./build/tez_server
   ```
4. Run the SQL parser:
   ```sh
   ./build/tez_sql_parser
   ```

## Installation
### Prerequisites
Before installing, ensure you have:
- **C++20 or newer**  
- **CMake** (for building)  
- **A C++ compiler** (GCC, Clang, or MSVC)  
- **Linux, macOS, or Windows**  

### Build Instructions
```sh
git clone https://github.com/botirk38/tez.git
cd tez
cmake -B build
cmake --build build
```

## Building & Deployment

### Building the Project
```sh
./your_program.sh
```

## Contributing
We welcome contributions! Follow these steps:
1. Fork the repository  
2. Create a new branch (`git checkout -b feature-branch`)  
3. Commit your changes (`git commit -m "Add feature"`)  
4. Push to your branch (`git push origin feature-branch`)  
5. Open a pull request  

## License
This project is licensed under the [MIT License](LICENSE).

