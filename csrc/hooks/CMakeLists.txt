# libascend_hal.so placeholder for link
add_library(ascend_hal SHARED ${HOOK_INC_DIR}/ascend_hal_stub.cpp)
target_include_directories(ascend_hal PUBLIC
  ${SANITIZER_THIRDPARTY_INCLUDE}
)

add_subdirectory(hal_hooks)
add_subdirectory(acl_hooks)
