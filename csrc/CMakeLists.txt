get_filename_component(CSRC_INC_DIR . ABSOLUTE)
get_filename_component(FRAMEWORK_INC_DIR ./core/framework ABSOLUTE)
get_filename_component(HOOK_INC_DIR ./hooks ABSOLUTE)


# 强制拦截业务代码中的所有编译告警
add_compile_options("-Werror")

add_library(ascend_san SHARED)
add_library(ascend_san_stub SHARED ${FRAMEWORK_INC_DIR}/communication.cpp
                                   ${FRAMEWORK_INC_DIR}/file_mapping.cpp
                                   ${FRAMEWORK_INC_DIR}/utility/domain_socket.cpp
                                   ${FRAMEWORK_INC_DIR}/utility/result.cpp
                                   ${FRAMEWORK_INC_DIR}/utility/log.cpp
                                   ${FRAMEWORK_INC_DIR}/utility/path.cpp
                                   ${FRAMEWORK_INC_DIR}/utility/file_system.cpp
                                   ${FRAMEWORK_INC_DIR}/utility/ustring.cpp
                                 )

add_subdirectory(address_sanitizer)
add_subdirectory(race_sanitizer)
add_subdirectory(sync_sanitizer)
add_subdirectory(register_sanitizer)
add_subdirectory(core)
add_subdirectory(stub_def)
add_subdirectory(hooks)
add_subdirectory(plugin)
add_subdirectory(include)

target_include_directories(
    ascend_san PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                      ${SECUREC_INC_DIR}
                      ${JSON_INC}
                      ${CSRC_INC_DIR}
                      ${SANITIZER_INC_DIR}
)
target_link_libraries(ascend_san c_sec pthread stdc++fs dl)
target_link_options(ascend_san PRIVATE "-Wl,-z,defs")

target_include_directories(
    ascend_san_stub PUBLIC ${FRAMEWORK_INC_DIR}
                           ${SECUREC_INC_DIR}
                           ${CSRC_INC_DIR}
                           ${SANITIZER_INC_DIR}
)
target_link_libraries(ascend_san_stub c_sec pthread)
target_link_options(ascend_san_stub PRIVATE "-Wl,-z,defs")

add_executable(mssanitizer ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_options(mssanitizer PRIVATE "-pie")
target_link_libraries(mssanitizer ascend_san)

set_target_properties(mssanitizer PROPERTIES OUTPUT_NAME "mssanitizer.bin")
set_target_properties(ascend_san PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(ascend_san_stub PROPERTIES VERSION ${PROJECT_VERSION})

install(TARGETS mssanitizer ascend_san ascend_san_stub ascend_hal_hook ascend_acl_hook ascendc_hook
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib64
)
install(DIRECTORY ${HOOK_INC_DIR}/api/acl
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        FILES_MATCHING PATTERN "*.h"
)
install(FILES ${HOOK_INC_DIR}/api/sanitizer.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
