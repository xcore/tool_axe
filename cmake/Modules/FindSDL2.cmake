# Locate SDL2
# This module defines
#  SDL2_FOUND - System has SDL2
#  SDL2_CONFIG_EXECUTABLE - The sdl2-config executable
#  SDL2_INCLUDE_DIRS - SDL2 include directories
#  SDL2_DEFINITIONS - SDL2 compiler definitions
#  SDL2_LIBRARIES - Libraries needed to link against SDL2

find_program(SDL2_CONFIG_EXECUTABLE
  NAMES sdl2-config
  DOC "sdl2-config executable")

include(FindPackageHandleStandardArgs)
# Sets SDL2_FOUND
find_package_handle_standard_args(
  SDL2 DEFAULT_MSG SDL2_CONFIG_EXECUTABLE)

if (SDL2_FOUND)
  if (MINGW AND ${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles")
    execute_process(
      COMMAND sh -c "${SDL2_CONFIG_EXECUTABLE} --cflags"
      OUTPUT_VARIABLE SDL2_CFLAGS
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
      COMMAND sh -c "${SDL2_CONFIG_EXECUTABLE} --libs"
      OUTPUT_VARIABLE SDL2_LIBRARIES
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  else()
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

  # Workaround SDL bug 2029 (-XCClinker isn't recognised by MinGW gcc).
  if (MINGW)
    STRING(REGEX REPLACE "-XCClinker " "" SDL2_LIBRARIES ${SDL2_LIBRARIES})
  endif()

  # Extract include directories / defines
  string(REPLACE " " ";" SDL2_CFLAGS_LIST ${SDL2_CFLAGS})
  foreach(FLAG ${SDL2_CFLAGS_LIST})
    if(${FLAG} MATCHES "^-I")
      STRING(REGEX REPLACE "^-I" "" FLAG ${FLAG})
      list(APPEND SDL2_INCLUDE_DIRS ${FLAG})
    elseif(${FLAG} MATCHES "^-D")
      STRING(REGEX REPLACE "^-D" "" FLAG ${FLAG})
      list(APPEND SDL2_DEFINITIONS ${FLAG})
    endif()
  endforeach()
endif()

mark_as_advanced(SDL2_CONFIG_EXECUTABLE)
