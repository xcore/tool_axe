find_package(LLVM REQUIRED)

add_executable(genJitGlobalMap genJitGlobalMap.cpp)

llvm_map_components_to_libraries(
  LLVM_LIBNAMES
  bitreader)

foreach(LLVM_LIBNAME ${LLVM_LIBNAMES})
  # Don't cache the library location as we are searching for a different library
  # each time around the loop.
  set(LLVM_LIBRARY ${LLVM_LIBNAME}-NOTFOUND)
  find_library(LLVM_LIBRARY ${LLVM_LIBNAME}
               PATHS ${LLVM_LIBRARY_DIRS})
  list(APPEND LLVM_LIBRARIES ${LLVM_LIBRARY})
endforeach()

target_link_libraries(
  genJitGlobalMap
  ${LLVM_LIBRARIES}
  )

target_include_directories(
  genJitGlobalMap
  PRIVATE
  ${LLVM_INCLUDE_DIRS}
  )

target_compile_definitions(
  genJitGlobalMap PRIVATE
  ${LLVM_DEFINITIONS}
  )

link_system_libs(genJitGlobalMap)
