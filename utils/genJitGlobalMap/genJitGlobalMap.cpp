// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "llvm-c/BitReader.h"
#include "llvm-c/Core.h"
#include <iostream>
#include <cstdlib>
#include <vector>

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cerr << "Usage: genJitAddrMap input\n";
    std::exit(1);
  }
  const char *inputFile = argv[1];
  LLVMMemoryBufferRef memBuf;
  char *error;
  if (LLVMCreateMemoryBufferWithContentsOfFile(inputFile, &memBuf,
                                               &error) != 0) {
    std::cerr << "Failed to open " << inputFile << ": " << error << '\n';
    std::free(error);
    std::exit(1);
  }
  LLVMModuleRef module;
  if (LLVMParseBitcode(memBuf, &module, &error) != 0) {
    std::cerr << "Failed to parse bitcode: " << error << '\n';
    std::free(error);
    std::exit(1);
  }
  std::vector<std::string> functions;
  for (LLVMValueRef it = LLVMGetFirstFunction(module); it;
       it = LLVMGetNextFunction(it)) {
    if (!LLVMIsDeclaration(it) || LLVMGetIntrinsicID(it) != 0)
      continue;
    std::cout << "DO_FUNCTION(" << LLVMGetValueName(it) << ")\n";
  }
  return 0;
}
