# The runtime is plain C with zero LLVM dependency. It is built as a static
# archive that instrumented programs link against (see driver/memsafec).
add_library(memsafety_runtime STATIC memsafety_runtime.c)
target_include_directories(memsafety_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(memsafety_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)
