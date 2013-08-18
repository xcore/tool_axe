# Locate SDL2
# This module defines
#  SDL2_FOUND - System has SDL2
#  SDL2_CONFIG_EXECUTABLE - The sdl2-config executable
#  SDL2_CFLAGS - C preprocessor flags for files that include SDL2 headers
#  SDL2_LIBRARIES - Libraries needed to link against SDL2

find_program(SDL2_CONFIG_EXECUTABLE
  NAMES sdl2-config
  DOC "sdl2-config executable")

include(FindPackageHandleStandardArgs)
# Sets SDL2_FOUND
find_package_handle_standard_args(
  SDL2 DEFAULT_MSG SDL2_CONFIG_EXECUTABLE)

if (SDL2_FOUND)
  execute_process(
    COMMAND ${SDL2_CONFIG_EXECUTABLE} --cflags
    OUTPUT_VARIABLE SDL2_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${SDL2_CONFIG_EXECUTABLE} --libs
    OUTPUT_VARIABLE SDL2_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

endif()

mark_as_advanced(SDL2_CONFIG_EXECUTABLE)
