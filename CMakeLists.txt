cmake_minimum_required(VERSION 2.6)
project(AXE)

if(CMAKE_COMPILER_IS_GNUCXX )
  set(COMPILER_IS_GCC_COMPATIBLE ON)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(COMPILER_IS_GCC_COMPATIBLE ON)
endif()

if(COMPILER_IS_GCC_COMPATIBLE)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-std=c++11" SUPPORTS_CPLUS_CLUS_11)
  if(SUPPORTS_CPLUS_CLUS_11)
    message(STATUS "-std=c++11 is supported.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  else()
    check_cxx_compiler_flag("-std=c++0x" SUPPORTS_CPLUS_CLUS_11)
    if(SUPPORTS_CPLUS_CLUS_11)
      message(STATUS "-std=c++0x is supported.")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
      message(FATAL_ERROR "C++11 is unsupported.")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -W -Wno-unused-parameter")
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
list(APPEND
  AXE_LINK_LIBRARIES
  ${LIBELF_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  ${LLVM_LIBRARIES})
include_directories(
  ${LIBELF_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR})
add_definitions(
  ${LIBXSLT_DEFINITIONS})

if(AXE_ENABLE_SDL)
  find_package(SDL)
  if(NOT SDL_FOUND)
    set(AXE_ENABLE_SDL 0)
  endif()
endif()

if(AXE_ENABLE_SDL)
  message(STATUS "SDL enabled")
  include_directories(${SDL_INCLUDE_DIR})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SDL_CFLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SDL_CFLAGS}")
  list(APPEND AXE_LINK_LIBRARIES ${SDL_LIBRARIES})
  add_definitions(-DAXE_ENABLE_SDL)
else()
  message(STATUS "SDL disabled")
endif()

add_executable(not
  not.cpp
  )

add_executable(instgen
  InstructionGen.cpp
  )

add_executable(genHex
  genHex.c
  )

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(NETWORK_LINK_TAP_FILES "NetworkLinkTapLinux.cpp")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(NETWORK_LINK_TAP_FILES "NetworkLinkTapOSX.cpp")
else()
  set(NETWORK_LINK_TAP_FILES "NetworkLinkTapDefault.cpp")
endif()

if(AXE_ENABLE_SDL)
list(APPEND
  AXE_OPTIONAL_FILES
  LCDScreen.h
  LCDScreen.cpp)
endif()

add_executable(axe
  AccessSecondIterator.h
  BitManip.h
  BootSequence.h
  BootSequence.cpp
  Chanend.h
  Chanend.cpp
  ChanEndpoint.h
  ChanEndpoint.cpp
  ClockBlock.h
  ClockBlock.cpp
  Config.h
  ConfigSchema.rng
  Core.h
  Core.cpp
  CRC.h
  DecodeCache.h
  DecodeCache.cpp
  EthernetPhy.h
  EthernetPhy.cpp
  Exceptions.h
  Exceptions.cpp
  Instruction.h
  Instruction.cpp
  InstructionBitcode.h
  InstructionBitcode.cpp
  InstructionDefinitions.cpp
  InstructionHelpers.h
  InstructionHelpers.cpp
  InstructionMacrosCommon.h
  InstructionProperties.h
  InstructionProperties.cpp
  JIT.h
  JIT.cpp
  JITInstructionFunction.h
  JITOptimize.h
  JITOptimize.cpp
  LLVMExtra.h
  LLVMExtra.cpp
  Lock.cpp
  Lock.h
  main.cpp
  NetworkLink.h
  NetworkLink.cpp
  ${NETWORK_LINK_TAP_FILES}
  Node.h
  Node.cpp
  Options.h
  Options.cpp
  Peripheral.h
  Peripheral.cpp
  PeripheralDescriptor.h
  PeripheralDescriptor.cpp
  PeripheralRegistry.h
  PeripheralRegistry.cpp
  Property.h
  Property.cpp
  Port.h
  Port.cpp
  PortAliases.h
  PortAliases.cpp
  PortArg.h
  PortArg.cpp
  PortCombiner.h
  PortCombiner.cpp
  PortConnectionManager.h
  PortConnectionManager.cpp
  PortInterface.h
  PortInterface.cpp
  PortHandleClockMixin.h
  PortHandleClockProxy.h
  PortHandleClockProxy.cpp
  PortInterface.h
  PortNames.h
  PortNames.cpp
  PortSignalTracker.h
  PortSignalTracker.cpp
  PortSplitter.h
  PortSplitter.cpp
  Register.h
  registerAllPeripherals.h
  registerAllPeripherals.cpp
  Resource.h
  Resource.cpp
  ring_buffer.h
  Runnable.h
  RunnableQueue.h
  RunnableQueue.cpp
  ScopedArray.h
  Signal.h
  SPIFlash.h
  SPIFlash.cpp
  SSwitch.h
  SSwitch.cpp
  SSwitchCtrlRegs.h
  SSwitchCtrlRegs.cpp
  SymbolInfo.h
  SymbolInfo.cpp
  Synchroniser.cpp
  Synchroniser.h
  SyscallHandler.h
  SyscallHandler.cpp
  SystemState.h
  SystemState.cpp
  TerminalColours.h
  TerminalColours.cpp
  Thread.h
  Thread.cpp
  Timer.cpp
  Timer.h
  Token.h
  Trace.h
  Trace.cpp
  UartRx.h
  UartRx.cpp
  WaveformTracer.h
  WaveformTracer.cpp
  XE.h
  XE.cpp
  XNSchema.rng
  XNTransform.xslt
  ${AXE_OPTIONAL_FILES}
  ${AXE_BINARY_DIR}/InstructionGenOutput.inc
  ${AXE_BINARY_DIR}/ConfigSchema.inc
  ${AXE_BINARY_DIR}/XNSchema.inc
  ${AXE_BINARY_DIR}/XNTransform.inc
  ${AXE_BINARY_DIR}/InstructionBitcode.inc
  )

if (COMPILER_IS_GCC_COMPATIBLE)
  set_source_files_properties(LLVMExtra.cpp PROPERTIES
                              COMPILE_FLAGS "-fno-rtti")
endif()

include_directories(${AXE_SOURCE_DIR}/thirdparty/boost)
# make sure cmake addes the binary directory for the project to the include path
include_directories(${AXE_BINARY_DIR})
# add the executable that will do the generation
get_target_property(INSTGEN_EXE instgen LOCATION)
get_target_property(GENHEX_EXE genHex LOCATION)
# add the custom command that will run the generator
add_custom_command(
 OUTPUT ${AXE_BINARY_DIR}/InstructionGenOutput.inc
 COMMAND ${INSTGEN_EXE} ${AXE_BINARY_DIR} > ${AXE_BINARY_DIR}/InstructionGenOutput.inc
 DEPENDS instgen
 )
# add the custom command that compiles InstructionDefinitions.cpp to LLVM
# bitcode
get_directory_property(INCLUDES_DIRS INCLUDE_DIRECTORIES)
set(CLANG_INCLUDE_FLAGS "")
foreach(dir ${INCLUDES_DIRS})
  set(CLANG_INCLUDE_FLAGS ${CLANG_INCLUDE_FLAGS} "-I${dir}")
endforeach()
string(REPLACE " " ";" CLANG_CXX_FLAGS ${CMAKE_CXX_FLAGS})
add_custom_command(
  OUTPUT ${AXE_BINARY_DIR}/InstructionBitcode.bc
  COMMAND ${CLANGPLUSPLUS_EXECUTABLE} ${CLANG_CXX_FLAGS} -O2 -g0 ${CLANG_INCLUDE_FLAGS}
  -c -emit-llvm ${AXE_SOURCE_DIR}/InstructionDefinitions.cpp
  -o ${AXE_BINARY_DIR}/InstructionBitcode.bc
  DEPENDS ${AXE_BINARY_DIR}/InstructionGenOutput.inc
  IMPLICIT_DEPENDS CXX $(AXE_SOURCE_DIR)/InstructionDefinitions.cpp
  )
# add the custom command that turns the LLVM bitcode into hex
add_custom_command(
  OUTPUT ${AXE_BINARY_DIR}/InstructionBitcode.inc
  COMMAND ${GENHEX_EXE} ${AXE_BINARY_DIR}/InstructionBitcode.bc ${AXE_BINARY_DIR}/InstructionBitcode.inc
  DEPENDS genHex ${AXE_BINARY_DIR}/InstructionBitcode.bc
  )

# add the custom command that turns the schema into hex
add_custom_command(
 OUTPUT ${AXE_BINARY_DIR}/ConfigSchema.inc
 COMMAND ${GENHEX_EXE} ${AXE_SOURCE_DIR}/ConfigSchema.rng ${AXE_BINARY_DIR}/ConfigSchema.inc
 DEPENDS genHex ConfigSchema.rng
 )
add_custom_command(
 OUTPUT ${AXE_BINARY_DIR}/XNSchema.inc
 COMMAND ${GENHEX_EXE} ${AXE_SOURCE_DIR}/XNSchema.rng ${AXE_BINARY_DIR}/XNSchema.inc
 DEPENDS genHex XNSchema.rng
 )
add_custom_command(
 OUTPUT ${AXE_BINARY_DIR}/XNTransform.inc
 COMMAND ${GENHEX_EXE} ${AXE_SOURCE_DIR}/XNTransform.xslt ${AXE_BINARY_DIR}/XNTransform.inc
 DEPENDS genHex XNTransform.xslt
 )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LLVM_CFLAGS}")

include_directories(
  ${LIBELF_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${LIBXSLT_INCLUDE_DIR})

target_link_libraries(
  axe ${AXE_LINK_LIBRARIES})

set_target_properties(axe PROPERTIES LINK_FLAGS ${LLVM_LDFLAGS})

install(TARGETS axe DESTINATION bin)

if (MSVC)
  SET(CPACK_GENERATOR "ZIP")
else()
  SET(CPACK_GENERATOR "TGZ")
endif()
INCLUDE(CPack)

configure_file(${CMAKE_SOURCE_DIR}/test/lit.site.cfg.in ${CMAKE_BINARY_DIR}/test/lit.site.cfg)

find_package(PythonInterp)

if (PYTHONINTERP_FOUND)
add_custom_target(check ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/thirdparty/lit/lit.py ${CMAKE_BINARY_DIR}/test)
else()
add_custom_target(check ${CMAKE_COMMAND} -E echo "Not running tests as python is missing")
endif()
