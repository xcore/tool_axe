cmake_minimum_required(VERSION 2.6)
project(AXE)

if(CMAKE_COMPILER_IS_GNUCXX )
  set(COMPILER_IS_GCC_COMPATIBLE ON)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(COMPILER_IS_GCC_COMPATIBLE ON)
endif()

# Add path for custom modules
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules"
  )

find_package(LibXml2 REQUIRED)
find_package(LibElf REQUIRED)
find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)

add_executable(not
  not.cpp
  )

add_executable(instgen
  InstructionGen.cpp
  )

add_executable(genHex
  genHex.c
  )

add_executable(axe
  main.cpp
  Core.h
  Core.cpp
  Runnable.h
  RunnableQueue.h
  RunnableQueue.cpp
  Thread.h
  Thread.cpp
  SyscallHandler.h
  SyscallHandler.cpp
  Exceptions.h
  Exceptions.cpp
  XE.h
  XE.cpp
  SymbolInfo.h
  SymbolInfo.cpp
  Resource.h
  Resource.cpp
  ChanEndpoint.h
  ChanEndpoint.cpp
  Chanend.h
  Chanend.cpp
  ClockBlock.h
  ClockBlock.cpp
  CRC.h
  InstructionBitcode.h
  InstructionBitcode.cpp
  JIT.h
  JIT.cpp
  JITInstructionFunction.h
  JITOptimize.h
  JITOptimize.cpp
  Lock.cpp
  Lock.h
  LLVMExtra.h
  LLVMExtra.cpp
  Signal.h
  Synchroniser.cpp
  Synchroniser.h
  Timer.cpp
  Timer.h
  PortInterface.h
  PortSplitter.h
  PortSplitter.cpp
  Port.h
  Port.cpp
  PortArg.h
  PortArg.cpp
  PortNames.h
  PortNames.cpp
  Register.h
  Trace.h
  Trace.cpp
  BitManip.h
  Config.h
  ScopedArray.h
  ring_buffer.h
  Instruction.h
  Instruction.cpp
  InstructionDefinitions.cpp
  InstructionHelpers.h
  InstructionHelpers.cpp
  InstructionMacrosCommon.h
  InstructionProperties.h
  InstructionProperties.cpp
  TerminalColours.h
  TerminalColours.cpp
  Node.h
  Node.cpp
  SystemState.h
  SystemState.cpp
  Token.h
  SSwitch.h
  SSwitch.cpp
  WaveformTracer.h
  WaveformTracer.cpp
  UartRx.h
  UartRx.cpp
  PeripheralRegistry.h
  PeripheralRegistry.cpp
  registerAllPeripherals.h
  registerAllPeripherals.cpp
  Peripheral.h
  Peripheral.cpp
  PeripheralDescriptor.h
  PeripheralDescriptor.cpp
  Property.h
  Property.cpp
  AccessSecondIterator.h
  ConfigSchema.rng
  ${AXE_BINARY_DIR}/InstructionGenOutput.inc
  ${AXE_BINARY_DIR}/ConfigSchema.inc
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
add_custom_command(
  OUTPUT ${AXE_BINARY_DIR}/InstructionBitcode.bc
  COMMAND ${CLANGPLUSPLUS_EXECUTABLE} ${CMAKE_CXX_FLAGS} -O2 -g0 ${CLANG_INCLUDE_FLAGS}
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

if(COMPILER_IS_GCC_COMPATIBLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W -Wno-unused-parameter")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Wno-unused-parameter")
elseif(MSVC)
# Disable the following warnings:
# C4355 'this' : used in base member initializer list.
# Deprecation warnings for less secure CRT functions.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4355 /D_CRT_SECURE_NO_WARNINGS=1")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_SECURE_NO_WARNINGS=1")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LLVM_CFLAGS}")

include_directories(
  ${LIBELF_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR})

if (MSVC)
  find_path(LIBICONV_INCLUDE_DIR iconv.h REQUIRED)
  include_directories(${LIBICONV_INCLUDE_DIR})
endif()

target_link_libraries(
  axe ${LIBELF_LIBRARIES} ${LIBXML2_LIBRARIES} ${LLVM_LIBRARIES})

set_target_properties(axe PROPERTIES LINK_FLAGS ${LLVM_LDFLAGS})

install(TARGETS axe DESTINATION bin)
if (MSVC)
  find_file(LIBZLIB_DLL zlib1.dll REQUIRED)
  find_file(LIBICONV_DLL iconv.dll REQUIRED)
  find_file(LIBXML2_DLL libxml2.dll REQUIRED)
  find_file(LIBELF_DLL libelf.dll REQUIRED)
  install(FILES ${LIBZLIB_DLL} ${LIBICONV_DLL} ${LIBXML2_DLL} ${LIBELF_DLL} DESTINATION bin)
endif()

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
