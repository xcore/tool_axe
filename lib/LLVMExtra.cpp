// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "LLVMExtra.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/Target.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
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

static Function* cloneFunctionDecl(Module &Dst, const Function &F,
                                   ValueToValueMapTy &VMap) {
  assert(F.getParent() != &Dst && "Can't copy decl over existing function.");
  Function *NewF =
    Function::Create(cast<FunctionType>(F.getType()->getElementType()),
                     F.getLinkage(), F.getName(), &Dst);
  NewF->copyAttributesFrom(&F);

  VMap[&F] = NewF;
  auto NewArgI = NewF->arg_begin();
  for (auto ArgI = F.arg_begin(), ArgE = F.arg_end(); ArgI != ArgE;
       ++ArgI, ++NewArgI)
    VMap[ArgI] = NewArgI;

  return NewF;
}

static Function *cloneFunction(Module &Dst, const Function &OrigF,
                               ValueToValueMapTy &VMap,
                               ValueMaterializer &Materializer) {
  Function *NewF = cloneFunctionDecl(Dst, OrigF, VMap);
  SmallVector<ReturnInst *, 8> Returns; // Ignore returns cloned.
  CloneFunctionInto(NewF, &OrigF, VMap, /*ModuleLevelChanges=*/true, Returns,
                    "", nullptr, nullptr, &Materializer);
  return NewF;
}

static GlobalVariable*
cloneGlobalVariableDecl(Module &Dst, const GlobalVariable &GV,
                        ValueToValueMapTy &VMap) {
  assert(GV.getParent() != &Dst && "Can't copy decl over existing global var.");
  GlobalVariable *NewGV =
    new GlobalVariable(Dst, GV.getType()->getElementType(), GV.isConstant(),
                       GV.getLinkage(), nullptr, GV.getName(), nullptr,
                       GV.getThreadLocalMode(),
                       GV.getType()->getAddressSpace());
  NewGV->copyAttributesFrom(&GV);
    VMap[&GV] = NewGV;
  return NewGV;
}

class CrossModuleValueMaterializer : public ValueMaterializer {
  Module &dstModule;
  ValueToValueMapTy &VMap;
public:
  CrossModuleValueMaterializer(Module &dstModule, ValueToValueMapTy &VMap) :
    dstModule(dstModule), VMap(VMap) {}
  Value *materializeValueFor(Value *V) override {
    if (Function *F = dyn_cast<Function>(V)) {
      return cloneFunctionDecl(dstModule, *F, VMap);
    }
    if (GlobalVariable *GV = dyn_cast<GlobalVariable>(V))
      return cloneGlobalVariableDecl(dstModule, *GV, VMap);
    return nullptr;
  }
};

LLVMValueRef LLVMExtraExtractFunctionIntoNewModule(LLVMValueRef f_) {
  Function *f = static_cast<Function*>(unwrap(f_));
  Module *srcModule = f->getParent();
  Module *dstModule = new Module("", srcModule->getContext());
  ValueToValueMapTy VMap;
  CrossModuleValueMaterializer materializer(*dstModule, VMap);
  Function *newF = cloneFunction(*dstModule, *f, VMap, materializer);
  return wrap(newF);
}
