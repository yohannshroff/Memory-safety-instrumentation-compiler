# Direct unit tests for the C runtime library. No LLVM involved.
add_executable(test_runtime test_runtime.c)
target_link_libraries(test_runtime PRIVATE memsafety_runtime)

add_test(NAME test_runtime COMMAND test_runtime)
