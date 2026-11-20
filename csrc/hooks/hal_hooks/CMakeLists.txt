add_library(ascend_hal_hook SHARED hal_hooks.cpp)

target_sources(ascend_hal_hook PRIVATE
  ${FRAMEWORK_INC_DIR}/communication.cpp
  ${FRAMEWORK_INC_DIR}/utility/domain_socket.cpp
  ${FRAMEWORK_INC_DIR}/utility/result.cpp
  ${FRAMEWORK_INC_DIR}/utility/ustring.cpp
  ${FRAMEWORK_INC_DIR}/utility/log.cpp
  ${FRAMEWORK_INC_DIR}/utility/file_system.cpp
  ${FRAMEWORK_INC_DIR}/utility/path.cpp
  ${HOOK_INC_DIR}/basic_hooks.cpp
  ${HOOK_INC_DIR}/hook_report.cpp
  ${HOOK_INC_DIR}/hooks_verify.cpp
)
target_include_directories(ascend_hal_hook PUBLIC
  ${CSRC_INC_DIR}
  ${FRAMEWORK_INC_DIR}
  ${SECUREC_INC_DIR}
  ${HOOK_INC_DIR}
  ${SANITIZER_THIRDPARTY_INCLUDE}
  ${SANITIZER_INC_DIR}
)
target_link_libraries(ascend_hal_hook ascend_hal c_sec dl pthread)
target_link_options(ascend_hal_hook PRIVATE "-Wl,-z,defs")
