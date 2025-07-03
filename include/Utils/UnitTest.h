#ifndef TEST_H
#define TEST_H

#include <cassert>

//Basic assertion macros

#define ASSERT_EQUALS(expected, actual) \
if ((expected) != (actual)) { \
std::cout << "Test failed: " << __FILE__ << ":" << __LINE__ << "\n" \
<< "  Expected: " << (expected) << "\n" \
<< "  Actual:   " << (actual) << "\n"; \
} else { \
std::cout << "Test passed: " << __FILE__ << ":" << __LINE__ << "\n"; \
}

#define ASSERT_LEQ(expected, actual) \
if ((expected) > (actual)) { \
std::cout << "Test failed: " << __FILE__ << ":" << __LINE__ << "\n" \
<< "  Expected: " << (expected) << "\n" \
<< "  Actual:   " << (actual) << "\n"; \
} else { \
std::cout << "Test passed: " << __FILE__ << ":" << __LINE__ << "\n"; \
}

#define ASSERT_TRUE(condition) \
if (!(condition)) { \
std::cout << "Test failed: " << __FILE__ << ":" << __LINE__ << "\n" \
<< "  Condition failed: " #condition "\n"; \
} else { \
std::cout << "Test passed: " << __FILE__ << ":" << __LINE__ << "\n"; \
}


#endif

