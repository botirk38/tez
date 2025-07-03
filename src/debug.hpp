#pragma once

#include <iostream>

// Define debug level
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3

// Set current log level - change this to control logging
#define CURRENT_LOG_LEVEL LOG_LEVEL_NONE

// Logging macros
#if CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(x) std::cout << "[DEBUG] " << x << std::endl
#else
#define LOG_DEBUG(x)
#endif

#if CURRENT_LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(x) std::cout << "[INFO] " << x << std::endl
#else
#define LOG_INFO(x)
#endif

#if CURRENT_LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(x) std::cerr << "[ERROR] " << x << std::endl
#else
#define LOG_ERROR(x)
#endif
