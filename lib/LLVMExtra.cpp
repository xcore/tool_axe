// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "LLVMExtra.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/Target.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/ExecutionEngine/Orc/IndirectionUtils.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/PassManager.h"
#include <iostream>

using namespace llvm;

LLVMBool LLVMExtraInlineFunction(LLVMValueRef call)
{
  InlineFunctionInfo IFI;
  return InlineFunction(CallSite(unwrap(call)), IFI);
}

#if 0
class JitDisassembler : public JITEventListener {
  LLVMDisasmContextRef DC;
public:
  JitDisassembler(const char *triple) {
    LLVMInitializeX86Disassembler();
    DC = LLVMCreateDisasm(triple, 0, 0, 0, 0);
  }
  ~JitDisassembler() {
    LLVMDisasmDispose(DC);
  }
  void NotifyFunctionEmitted(const Function &f, void *code, size_t size,
                             const EmittedFunctionDetails &) override {
    uint8_t *bytes = (uint8_t*)code;
    uint8_t *end = bytes + size;
    char buf[256];
    while (bytes < end) {
      size_t instSize = LLVMDisasmInstruction(DC, bytes, end - bytes,
                                              (uint64_t)bytes, &buf[0], 256);
      if (instSize == 0) {
        std::cerr << "\t???\n";
        return;
      }
      bytes += instSize;
      std::cerr << buf << '\n';
    }
  }
};
#endif

void
LLVMExtraRegisterJitDisassembler(LLVMExecutionEngineRef EE,
                                 const char *triple) {
#if 0
  static JitDisassembler disassembler(triple);
  unwrap(EE)->RegisterJITEventListener(&disassembler);
#endif
}

void LLVMDisableSymbolSearching(LLVMExecutionEngineRef EE, LLVMBool Disable)
{
  unwrap(EE)->DisableSymbolSearching(Disable);
}

class CrossModuleValueMaterializer : public ValueMaterializer {
  Module &dstModule;
  ValueToValueMapTy &VMap;
public:
  CrossModuleValueMaterializer(Module &dstModule, ValueToValueMapTy &VMap) :
    dstModule(dstModule), VMap(VMap) {}
  Value *materializeValueFor(Value *V) override {
    if (Function *F = dyn_cast<Function>(V))
      return orc::cloneFunctionDecl(dstModule, *F, &VMap);
    if (GlobalVariable *GV = dyn_cast<GlobalVariable>(V))
      return orc::cloneGlobalVariableDecl(dstModule, *GV, &VMap);
    return nullptr;
  }
};

LLVMValueRef LLVMExtraExtractFunctionIntoNewModule(LLVMValueRef f_) {
  Function *f = static_cast<Function*>(unwrap(f_));
  Module *srcModule = f->getParent();
  Module *dstModule = new Module("", srcModule->getContext());
  // First copy the function declaration.
  ValueToValueMapTy VMap;
  Function *newF = orc::cloneFunctionDecl(*dstModule, *f, &VMap);
  // Then copy the body.
  CrossModuleValueMaterializer materializer(*dstModule, VMap);
  orc::moveFunctionBody(*f, VMap, &materializer, newF);
  return wrap(newF);
}
