FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    # URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

# For MSVC
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(mx_router_tests router_tests.cpp)
target_link_libraries(mx_router_tests PRIVATE mx_router GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(mx_router_tests)