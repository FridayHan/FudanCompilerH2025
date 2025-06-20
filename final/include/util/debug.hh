#ifndef _DEBUG_HH
#define _DEBUG_HH

#include <iostream>

#ifdef DEBUG
#define DEBUG_PRINT(msg) do { std::cerr << msg << std::endl; } while (0)
#define DEBUG_PRINT2(msg, val) do { std::cerr << msg << " " << val << std::endl; } while (0)
#else
#define DEBUG_PRINT(msg) do { } while (0)
#define DEBUG_PRINT2(msg, val) do { } while (0)
#endif

#define CHECK_NULLPTR(node) if (!node) { \
    std::cerr << "Error: Null pointer at " << __FILE__ << ":" << __LINE__ << std::endl; \
    exit(-1); \
}

#endif
