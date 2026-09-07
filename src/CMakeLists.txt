# The instrumentation pass, built as a MODULE library that `opt` and `clang`
# load with -load-pass-plugin. It must NOT link LLVM libraries statically -
# it resolves LLVM symbols from the loading tool at runtime.
add_library(MemSafety MODULE
  MemSafety.cpp
  IRScanner.cpp
  CheckInjector.cpp
)

target_include_directories(MemSafety SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(MemSafety PRIVATE ${LLVM_DEFINITIONS})

# Match LLVM's own build flags (RTTI is off in most LLVM packages).
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(MemSafety PRIVATE -fno-rtti)
endif()

# Keep default symbol visibility so the loader can find llvmGetPassPluginInfo.
set_target_properties(MemSafety PROPERTIES PREFIX "lib")

# On macOS a MODULE bundle needs to allow undefined symbols (resolved by opt).
if(APPLE)
  target_link_options(MemSafety PRIVATE -Wl,-undefined,dynamic_lookup)
endif()
