# Locate LLVM
# This module defines
#  LLVM_FOUND - System has LLVM
#  LLVM_CONFIG_EXECUTABLE - The llvm-config executable
#  LLVM_CFLAGS - C preprocessor flags for files that include LLVM headers
#  LLVM_LDFLAGS - Linker flags needed to link against LLVM
#  LLVM_LIBRARIES - Libraries needed to link against LLVM

find_program(LLVM_CONFIG_EXECUTABLE
  NAMES llvm-config-${LLVM_FIND_VERSION} llvm-config
  DOC "llvm-config executable")

include(FindPackageHandleStandardArgs)

if (LLVM_CONFIG_EXECUTABLE)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
    OUTPUT_VARIABLE LLVM_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
    OUTPUT_VARIABLE LLVM_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
    OUTPUT_VARIABLE LLVM_CFLAGS_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE " " ";" LLVM_CFLAGS ${LLVM_CFLAGS_RAW})

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs scalaropts engine bitreader
    OUTPUT_VARIABLE LLVM_LIBNAMES_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE " " ";" LLVM_LIBNAMES_LIST ${LLVM_LIBNAMES_RAW})

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
    OUTPUT_VARIABLE LLVM_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  foreach(LLVM_LIBNAME ${LLVM_LIBNAMES_LIST})
    string(REGEX REPLACE "^-l" "" LLVM_LIBNAME ${LLVM_LIBNAME})
    # Don't cache the library location as we are searching for a different library
    # each time around the loop.
    set(LLVM_LIBRARY ${LLVM_LIBNAME}-NOTFOUND)
    find_library(LLVM_LIBRARY ${LLVM_LIBNAME}
                 PATHS ${LLVM_LIBDIR})
    list(APPEND LLVM_LIBRARIES ${LLVM_LIBRARY})
  endforeach()

endif()

find_package_handle_standard_args(
  LLVM REQUIRED_VARS LLVM_CONFIG_EXECUTABLE
       VERSION_VAR LLVM_VERSION)

mark_as_advanced(LLVM_CONFIG_EXECUTABLE)
