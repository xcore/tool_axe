find_package(LLVM 3.3 REQUIRED)

add_executable(genJitGlobalMap genJitGlobalMap.cpp)

target_link_libraries(
  genJitGlobalMap
  ${LLVM_LIBRARIES}
  ${LLVM_LDFLAGS}
  )

target_compile_options(
  genJitGlobalMap PRIVATE
  ${LLVM_CFLAGS}
  )
