# Locate Clang
# This module defines
#  CLANG_FOUND - System has LLVM
#  CLANG_EXECUTABLE - The clang executable
#  CLANGPLUSPLUS_EXECUTABLE - The clang++ executable

find_program(
  CLANG_EXECUTABLE NAMES clang
  DOC "clang executable")

find_program(
  CLANGPLUSPLUS_EXECUTABLE NAMES clang++
  DOC "clang++ executable")

include(FindPackageHandleStandardArgs)
# Sets CLANG_FOUND
find_package_handle_standard_args(
  Clang DEFAULT_MSG CLANG_EXECUTABLE CLANGPLUSPLUS_EXECUTABLE)

mark_as_advanced(CLANG_EXECUTABLE)
mark_as_advanced(CLANGPLUSPLUS_EXECUTABLE)
