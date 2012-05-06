# Locate LLVM
# This module defines
#  LLVM_FOUND - System has LLVM
#  LLVM_CONFIG_EXECUTABLE - The llvm-config executable
#  LLVM_CFLAGS - C preprocessor flags for files that include LLVM headers
#  LLVM_LDFLAGS - Linker flags needed to link against LLVM

find_program(LLVM_CONFIG_EXECUTABLE
  NAMES llvm-config
  llvm-config-3.0
  llvm-config-2.9
  DOC "llvm-config executable")

include(FindPackageHandleStandardArgs)
# Sets LLVM_FOUND
find_package_handle_standard_args(
  LLVM DEFAULT_MSG LLVM_CONFIG_EXECUTABLE)

if (LLVM_FOUND)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
    OUTPUT_VARIABLE LLVM_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
    OUTPUT_VARIABLE LLVM_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs all
    OUTPUT_VARIABLE LLVM_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

mark_as_advanced(LLVM_CONFIG_EXECUTABLE)
