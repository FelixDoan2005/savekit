include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Keep gtest's CRT consistent with ours on MSVC, and don't install it.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(savekit_tests
    test_smoke.cpp
    # Add one file per area as you go, e.g.:
    # test_roundtrip.cpp
    # test_header.cpp
    # test_versioning.cpp
)
target_link_libraries(savekit_tests PRIVATE savekit::savekit GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(savekit_tests DISCOVERY_MODE PRE_TEST)
