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
  ./hooks/*.cpp
  ./plugin/*.cpp)
list(SORT TEST_SOURCE)
target_sources(mssanitizer_test PRIVATE ${TEST_SOURCE})
target_sources(mssanitizer_test PRIVATE ${TEST_ORIGINAL_SOURCE})
target_compile_options(mssanitizer_test PRIVATE "-D__BUILD_TESTS__")

set(test_utility_header ./test_utility)
target_include_directories(mssanitizer_test PUBLIC ${asan_test_header} ${gtest_header} ${test_utility_header}
  ${hooks_header} ${JSON_INC} ${SANITIZER_THIRDPARTY_INCLUDE} ${SANITIZER_INC_DIR})
target_link_libraries(mssanitizer_test gtest_main c_sec stdc++fs dl)

# 仅布局桥接 TU 使用 common 的 Host 协议；产品目标不引入这组头路径。
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/plugin/peer_host_layout_fixture.cpp" PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/msopscommon/csrc;${CMAKE_SOURCE_DIR}/msopscommon/csrc/include/thirdparty")
