# 设置默认构建类型为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug/Release)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 安全规范编译命令
add_compile_options("-fPIE")
add_compile_options("-fPIC")
add_compile_options("-fstack-protector-all")
add_compile_options("-Wall")
add_compile_options("-fno-strict-aliasing")

# 根据CMAKE_BUILD_TYPE选择编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build: -O0, strip disabled")
    add_compile_options("-O0")
else()
    message(STATUS "Release build: -O2, strip enabled")
    add_compile_options("-O2")
    add_compile_options("-D_FORTIFY_SOURCE=2")
    add_link_options("-s")  # strip symbols
endif()

add_link_options("-Wl,-z,now")
add_link_options("-Wl,-z,relro")
add_link_options("-Wl,-z,noexecstack")
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_CXX_FLAGS "-std=c++11")

# 设置工具提交 ID
find_package(Git REQUIRED)
execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
  OUTPUT_VARIABLE MSSANITIZER_COMMIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions(-D__MSSANITIZER_COMMIT_REVISION__="${MSSANITIZER_COMMIT_REVISION}")

# 从version.info获取版本信息
file(READ "${ROOT_DIR}/package/conf/version.info" FILE_CONTENT)
if(FILE_CONTENT MATCHES "\\[PACKAGE\\]([^\\[]+)")
    set(TMP_SECTION "${CMAKE_MATCH_1}")
    if(TMP_SECTION MATCHES "Version=([^\\n]+)")
        string(STRIP "${CMAKE_MATCH_1}" PACKAGE_VERSION)
        message(STATUS "Version: ${PACKAGE_VERSION}")
    endif()
endif()

add_definitions(-D__PACKAGE_VERSION__="${PACKAGE_VERSION}")