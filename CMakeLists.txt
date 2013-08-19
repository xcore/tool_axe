cmake_minimum_required(VERSION 2.6)
project(AXE)
include(Version.txt)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_COMPILER_IS_GNUCXX )
  set(COMPILER_IS_GCC_COMPATIBLE ON)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(COMPILER_IS_GCC_COMPATIBLE ON)
endif()

if(COMPILER_IS_GCC_COMPATIBLE)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-std=c++11" SUPPORTS_CXX11)
  if(SUPPORTS_CXX11)
    message(STATUS "-std=c++11 is supported.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  else()
    check_cxx_compiler_flag("-std=c++0x" SUPPORTS_CXX0X)
    if(SUPPORTS_CXX0X)
      message(STATUS "-std=c++0x is supported.")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
      message(FATAL_ERROR "C++11 is unsupported.")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W -Wno-unused-parameter")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Wno-unused-parameter")
  endif()
elseif(MSVC)
# Disable the following warnings:
# C4355 'this' : used in base member initializer list.
# Deprecation warnings for less secure CRT functions.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4355 /D_CRT_SECURE_NO_WARNINGS=1")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_SECURE_NO_WARNINGS=1")
endif()

# Add path for custom modules
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules"
  )

option(AXE_ENABLE_SDL "Use SDL if available" ON)

find_package(LibXml2 REQUIRED)
find_package(LibXslt REQUIRED)
find_package(LibElf REQUIRED)
find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)

llvm_map_components_to_libraries(LLVM_LIBRARIES scalaropts engine bitreader)

list(APPEND
  AXE_LINK_LIBRARIES
  ${LIBELF_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  ${LLVM_LIBRARIES})
include_directories(
  ${LIBELF_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(
  ${LIBXSLT_DEFINITIONS}
  ${LLVM_DEFINITIONS})
if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  # On Linux LibXslt requires librt.
  find_package(LibRt REQUIRED)
  list(APPEND AXE_LINK_LIBRARIES ${LIBRT_LIBRARIES})
endif()

if(AXE_ENABLE_SDL)
  find_package(SDL2)
  if(NOT SDL2_FOUND)
    set(AXE_ENABLE_SDL 0)
  endif()
endif()

if(AXE_ENABLE_SDL)
  message(STATUS "SDL enabled")
  include_directories(${SDL2_INCLUDE_DIR})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SDL2_CFLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SDL2_CFLAGS}")
  list(APPEND AXE_LINK_LIBRARIES ${SDL2_LIBRARIES})
  add_definitions(-DAXE_ENABLE_SDL)
else()
  message(STATUS "SDL disabled")
endif()

add_subdirectory(utils/instgen)
add_subdirectory(utils/not)
add_subdirectory(utils/genHex)
add_subdirectory(lib)
add_subdirectory(tools/axe)

include_directories(
  ${LIBELF_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${LIBXSLT_INCLUDE_DIR})

if (MSVC)
  SET(CPACK_GENERATOR "ZIP")
else()
  SET(CPACK_GENERATOR "TGZ")
endif()
set(CPACK_PACKAGE_VERSION
    "${AXE_VERSION_MAJOR}.${AXE_VERSION_MINOR}${AXE_VERSION_TWEAK}")
set(CPACK_PACKAGE_NAME "axe")
INCLUDE(CPack)

configure_file(${CMAKE_SOURCE_DIR}/test/lit.site.cfg.in ${CMAKE_BINARY_DIR}/test/lit.site.cfg)

find_package(PythonInterp)

if (PYTHONINTERP_FOUND)
add_custom_target(check ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/thirdparty/lit/lit.py ${CMAKE_BINARY_DIR}/test)
else()
add_custom_target(check ${CMAKE_COMMAND} -E echo "Not running tests as python is missing")
endif()
