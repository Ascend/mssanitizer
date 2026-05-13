add_executable(mssanitizer_test)
set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage -lgcov")

add_subdirectory(core)

list(SORT TEST_ORIGINAL_SOURCE)
file(GLOB_RECURSE TEST_SOURCE
  ./address_sanitizer/*.cpp
  ./race_sanitizer/*.cpp
  ./sync_sanitizer/*.cpp
  ./register_sanitizer/*.cpp
  ./core/framework/*.cpp
  ./core/framework/utility/*.cpp
  ./plugin/*.cpp)
list(SORT TEST_SOURCE)
target_sources(mssanitizer_test PRIVATE ${TEST_SOURCE})
target_sources(mssanitizer_test PRIVATE ${TEST_ORIGINAL_SOURCE})
target_compile_options(mssanitizer_test PRIVATE "-D__BUILD_TESTS__")

set(test_utility_header ./test_utility)
target_include_directories(mssanitizer_test PUBLIC ${asan_test_header} ${gtest_header} ${test_utility_header}
  ${hooks_header} ${JSON_INC} ${SANITIZER_THIRDPARTY_INCLUDE} ${SANITIZER_INC_DIR})
target_link_libraries(mssanitizer_test gtest_main c_sec stdc++fs dl)
