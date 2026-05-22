include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# ── Test binaries ──────────────────────────────────────────────────────────

add_executable(test_parser   test_parser.cpp)
add_executable(test_anomaly  test_anomaly.cpp)
add_executable(test_buffer   test_buffer.cpp)

foreach(target IN ITEMS test_parser test_anomaly test_buffer)
    target_link_libraries(${target} PRIVATE nmea_core GTest::gtest_main)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
    gtest_discover_tests(${target})
endforeach()