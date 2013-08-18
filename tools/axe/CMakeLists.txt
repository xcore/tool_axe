include_directories(${AXE_SOURCE_DIR}/lib;${AXE_BINARY_DIR}/lib)
include_directories(${AXE_SOURCE_DIR}/thirdparty/boost)

add_executable(axe-bin
  Options.h
  Options.cpp
  main.cpp)
target_link_libraries(axe-bin axe)
set_target_properties(axe-bin PROPERTIES OUTPUT_NAME axe)
set_target_properties(axe-bin PROPERTIES LINK_FLAGS ${LLVM_LDFLAGS})

install(TARGETS axe-bin DESTINATION bin)
