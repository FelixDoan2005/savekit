#include <gtest/gtest.h>

#include "savekit/version.hpp"

// Proves the build, the library link and the test runner all work.
// Replace or extend with real tests as the library grows.
TEST(Smoke, LibraryVersionIsSet) {
    EXPECT_FALSE(savekit::library_version().empty());
}
