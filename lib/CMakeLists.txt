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

include_directories(${AXE_SOURCE_DIR}/thirdparty/boost)
# make sure cmake addes the binary directory for the project to the include path
include_directories(${AXE_BINARY_DIR})

add_library(axe
  AccessSecondIterator.h
  AXE.h
  AXE.cpp
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
  InstructionOpcode.h
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
  Register.cpp
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
  -c -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/InstructionDefinitions.cpp
  -o ${AXE_BINARY_DIR}/InstructionBitcode.bc
  DEPENDS ${AXE_BINARY_DIR}/InstructionGenOutput.inc
  IMPLICIT_DEPENDS CXX InstructionDefinitions.cpp
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
 COMMAND ${GENHEX_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/ConfigSchema.rng ${AXE_BINARY_DIR}/ConfigSchema.inc
 DEPENDS genHex ConfigSchema.rng
 )
add_custom_command(
 OUTPUT ${AXE_BINARY_DIR}/XNSchema.inc
 COMMAND ${GENHEX_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/XNSchema.rng ${AXE_BINARY_DIR}/XNSchema.inc
 DEPENDS genHex XNSchema.rng
 )
add_custom_command(
 OUTPUT ${AXE_BINARY_DIR}/XNTransform.inc
 COMMAND ${GENHEX_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/XNTransform.xslt ${AXE_BINARY_DIR}/XNTransform.inc
 DEPENDS genHex XNTransform.xslt
 )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LLVM_CFLAGS}")

if (COMPILER_IS_GCC_COMPATIBLE)
  set_source_files_properties(LLVMExtra.cpp PROPERTIES
                              COMPILE_FLAGS "-fno-rtti")
endif()

target_link_libraries(
  axe ${AXE_LINK_LIBRARIES})
