// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cassert>

// TODO emit line markers.

enum OpType {
  in,
  out,
  inout,
  imm
};

enum Register {
  R0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  CP,
  DP,
  SP,
  LR,
  ED,
  ET,
  KEP,
  KSP,
  SR,
  SPC,
  SED,
  SSR
};

const char *getRegisterName(Register reg) {
  switch (reg) {
  default:
    assert(0 && "Unexpected register");
    return "?";
  case R0: return "R0";
  case R1: return "R1";
  case R2: return "R2";
  case R3: return "R3";
  case R4: return "R4";
  case R5: return "R5";
  case R6: return "R6";
  case R7: return "R7";
  case R8: return "R8";
  case R9: return "R9";
  case R10: return "R10";
  case R11: return "R11";
  case CP: return "CP";
  case DP: return "DP";
  case SP: return "SP";
  case LR: return "LR";
  case ET: return "ET";
  case ED: return "ED";
  case KEP: return "KEP";
  case KSP: return "KSP";
  case SPC: return "SPC";
  case SED: return "SED";
  case SSR: return "SSR";
  }
}

std::vector<OpType> ops()
{
  return std::vector<OpType>();
}

std::vector<OpType> ops(OpType op0)
{
  std::vector<OpType> operands;
  operands.push_back(op0);
  return operands;
}

std::vector<OpType> ops(OpType op0, OpType op1)
{
  std::vector<OpType> operands;
  operands.push_back(op0);
  operands.push_back(op1);
  return operands;
}

std::vector<OpType> ops(OpType op0, OpType op1, OpType op2)
{
  std::vector<OpType> operands;
  operands.push_back(op0);
  operands.push_back(op1);
  operands.push_back(op2);
  return operands;
}

std::vector<OpType> ops(OpType op0, OpType op1, OpType op2, OpType op3)
{
  std::vector<OpType> operands;
  operands.push_back(op0);
  operands.push_back(op1);
  operands.push_back(op2);
  operands.push_back(op3);
  return operands;
}

std::vector<OpType> ops(OpType op0, OpType op1, OpType op2, OpType op3,
                        OpType op4)
{
  std::vector<OpType> operands;
  operands.push_back(op0);
  operands.push_back(op1);
  operands.push_back(op2);
  operands.push_back(op3);
  operands.push_back(op4);
  return operands;
}

std::vector<OpType> ops(OpType op0, OpType op1, OpType op2, OpType op3,
                        OpType op4, OpType op5)
{
  std::vector<OpType> operands;
  operands.push_back(op0);
  operands.push_back(op1);
  operands.push_back(op2);
  operands.push_back(op3);
  operands.push_back(op4);
  operands.push_back(op5);
  return operands;
}

class Instruction {
private:
  std::string name;
  unsigned size;
  std::vector<OpType> operands;
  std::vector<Register> implicitOps;
  std::string format;
  std::string code;
  std::string transformStr;
  std::string reverseTransformStr;
  unsigned cycles;
  bool sync:1;
  bool canEvent:1;
  bool unimplemented:1;
  bool custom:1;
  bool canJit:1;
  bool mayBranch:1;
  bool mayLoad:1;
  bool mayStore:1;
  bool usesPc:1;
  bool mayExcept:1;
  bool mayKCall:1;
  bool mayPauseOn:1;
  bool mayYield:1;
  bool mayDeschedule:1;
public:
  Instruction(const std::string &n,
              unsigned s,
              const std::vector<OpType> &ops,
              const std::string &f,
              const std::string &c,
              unsigned cy) :
    name(n),
    size(s),
    operands(ops),
    format(f),
    code(c),
    cycles(cy),
    sync(false),
    canEvent(false),
    unimplemented(false),
    custom(false),
    canJit(false),
    mayBranch(false),
    mayLoad(false),
    mayStore(false),
    usesPc(false),
    mayExcept(false),
    mayKCall(false),
    mayPauseOn(false),
    mayYield(false),
    mayDeschedule(false)
  {
  }
  const std::string &getName() const { return name; }
  unsigned getSize() const { return size; }
  const std::vector<OpType> &getOperands() const { return operands; }
  unsigned getNumExplicitOperands() const {
    return operands.size() - implicitOps.size();
  }
  const std::vector<Register> &getImplicitOps() const { return implicitOps; }
  const std::string &getFormat() const { return format; }
  const std::string &getCode() const { return code; }
  const std::string &getTransform() const { return transformStr; }
  const std::string &getReverseTransform() const { return reverseTransformStr; }
  unsigned getCycles() const { return cycles; }
  bool getSync() const { return sync; }
  bool getCanEvent() const { return canEvent; }
  bool getUnimplemented() const { return unimplemented; }
  bool getCustom() const { return custom; }
  bool getCanJit() const { return canJit; }
  bool getMayBranch() const {
    // In future we might want to exclude instructions which use the PC without
    // writing it.
    return usesPc;
  }
  bool getMayStore() const { return mayStore; }
  bool getMayLoad() const { return mayLoad; }
  bool getUsesPc() const { return usesPc; }
  bool getMayExcept() const { return mayExcept; }
  bool getMayKCall() const { return mayKCall; }
  bool getMayPauseOn() const { return mayPauseOn; }
  bool getMayYield() const { return mayYield; }
  bool getMayDeschedule() const { return mayDeschedule; }
  Instruction &addImplicitOp(Register reg, OpType type) {
    assert(type != imm);
    implicitOps.push_back(reg);
    operands.push_back(type);
    return *this;
  }
  Instruction &transform(const std::string &t, const std::string &rt) {
    transformStr = t;
    reverseTransformStr = rt;
    return *this;
  }
  Instruction &setCycles(unsigned value) {
    cycles = value;
    return *this;
  }
  Instruction &setSync() {
    sync = true;
    return *this;
  }
  Instruction &setCanEvent() {
    canEvent = true;
    return *this;
  }
  Instruction &setUnimplemented() {
    unimplemented = true;
    return *this;
  }
  Instruction &setCustom() {
    custom = true;
    return *this;
  }
  Instruction &setCanJit() {
    canJit = true;
    return *this;
  }
  Instruction &setMayLoad() {
    mayLoad = true;
    return *this;
  }
  Instruction &setMayStore() {
    mayStore = true;
    return *this;
  }
  Instruction &setUsesPc() {
    usesPc = true;
    return *this;
  }
  Instruction &setMayExcept() {
    mayExcept = true;
    return *this;
  }
  Instruction &setMayKCall() {
    mayKCall = true;
    return *this;
  }
  Instruction &setMayPauseOn() {
    mayPauseOn = true;
    return *this;
  }
  Instruction &setMayYield() {
    mayYield = true;
    return *this;
  }
  Instruction &setMayDeschedule() {
    mayDeschedule = true;
    return *this;
  }
};

class InstructionRefs {
  std::vector<Instruction*> refs;
public:
  void add(Instruction *i) { refs.push_back(i); }
  InstructionRefs &addImplicitOp(Register reg, OpType type);
  InstructionRefs &transform(const std::string &t, const std::string &rt);
  InstructionRefs &setCycles(unsigned value);
  InstructionRefs &setSync();
  InstructionRefs &setCanEvent();
  InstructionRefs &setUnimplemented();
};

InstructionRefs &InstructionRefs::
addImplicitOp(Register reg, OpType type)
{
  for (std::vector<Instruction*>::iterator it = refs.begin(), e = refs.end();
       it != e; ++it) {
    (*it)->addImplicitOp(reg, type);
  }
  return *this;
}

InstructionRefs &InstructionRefs::
transform(const std::string &t, const std::string &rt)
{
  for (std::vector<Instruction*>::iterator it = refs.begin(), e = refs.end();
       it != e; ++it) {
    (*it)->transform(t, rt);
  }
  return *this;
}

InstructionRefs &InstructionRefs::
setCycles(unsigned value)
{
  for (std::vector<Instruction*>::iterator it = refs.begin(), e = refs.end();
       it != e; ++it) {
    (*it)->setCycles(value);
  }
  return *this;
}

InstructionRefs &InstructionRefs::
setSync()
{
  for (std::vector<Instruction*>::iterator it = refs.begin(), e = refs.end();
       it != e; ++it) {
    (*it)->setSync();
  }
  return *this;
}

InstructionRefs &InstructionRefs::
setCanEvent()
{
  for (std::vector<Instruction*>::iterator it = refs.begin(), e = refs.end();
       it != e; ++it) {
    (*it)->setCanEvent();
  }
  return *this;
}

InstructionRefs &InstructionRefs::
setUnimplemented()
{
  for (std::vector<Instruction*>::iterator it = refs.begin(), e = refs.end();
       it != e; ++it) {
    (*it)->setUnimplemented();
  }
  return *this;
}

std::vector<Instruction*> instructions;

Instruction &inst(const std::string &name,
                  unsigned size,
                  const std::vector<OpType> &operands,
                  const std::string &format,
                  const std::string &code,
                  unsigned cycles)
{
  instructions.push_back(new Instruction(name,
                                         size,
                                         operands,
                                         format,
                                         code,
                                         cycles));
  return *instructions.back();
}


static void
quote(std::ostream &os, const std::string &s)
{
  os << '\"';
  for (unsigned i = 0, e = s.size(); i != e; i++) {
    switch (s[i]) {
      default:
        os << s[i];
        break;
      case '\'':
        os << "\\\'";
        break;
      case '\"':
        os << "\\\"";
        break;
      case '\\':
        os << "\\\\";
        break;
      case '\n':
        os << "\\\n";
        break;
    }
  }
  os << '\"';
}

static const char *
scanClosingBracket(const char *s)
{
  const char *p = s;
  unsigned indentLevel = 1;
  while (*p != '\0') {
  switch (*p) {
    case '(':
      indentLevel++;
      break;
    case ')':
      indentLevel--;
      if (indentLevel == 0)
        return p;
      break;
    case '\'':
    case '"':
      std::cerr << "error: ' and \" are not yet handled\n";
      std::exit(1);
    }
    ++p;
  }
  std::cerr << "error: no closing bracket found in string:\n";
  std::cerr << s << '\n';
  std::exit(1);
}

std::string getEndLabel(const Instruction &instruction)
{
  return instruction.getName() + "_end";
}

static bool
isFixedRegister(const Instruction &inst, unsigned i, Register &value)
{
  const std::vector<OpType> &ops = inst.getOperands();
  const std::vector<Register> &implicitOps = inst.getImplicitOps();
  unsigned numExplicitOperands = ops.size() - implicitOps.size();
  if (i < numExplicitOperands)
    return false;
  if (i >= ops.size()) {
    std::cerr << "error: operand out of range\n";
    std::exit(1);
  }
  assert(ops[i] != imm);
  value = implicitOps[i - numExplicitOperands];
  return true;
}

static std::string
getOperandName(const Instruction &inst, unsigned i)
{
  Register reg;
  if (isFixedRegister(inst, i, reg)) {
    return getRegisterName(reg);
  }
  std::ostringstream buf;
  const char *opMacro = inst.getOperands().size() > 3 ? "LOP" : "OP";
  buf << opMacro << '(' << i << ')';
  return buf.str();
}

static bool
isSR(const Instruction &inst, unsigned i)
{
  Register value;
  if (!isFixedRegister(inst, i, value))
    return false;
  return value == SR;
}

static void
emitCycles(const Instruction &instruction)
{
  std::cout << "TIME += " << instruction.getCycles() << ";\n";
}

static void
emitRegWriteBack(const Instruction &instruction)
{
  const std::vector<OpType> &operands = instruction.getOperands();
  bool writeSR = false;
  unsigned numSR = 0;

  // Write operands.
  for (unsigned i = 0, e = operands.size(); i != e; ++i) {
    switch (operands[i]) {
      default:
        break;
      case out:
      case inout:
        if (isSR(instruction, i)) {
          writeSR = true;
          numSR = i;
        } else {
          std::cout << "REG(" << getOperandName(instruction, i) << ')'
          << " = op" << i << ";\n";
          std::cout << "TRACE_REG_WRITE((Register)" << getOperandName(instruction, i)
                    << ", " << "op" << i << ");\n";
        }
        break;
    }
  }
  std::cout << "PC = nextPc;\n";
  if (writeSR) {
    std::cout << "SETSR(op" << numSR << ", PC);\n";
  }
}

static void
emitCheckEvents(const Instruction &instruction)
{
  if (!instruction.getCanEvent())
    return;

  std::cout << "if (sys.hasPendingEvent()) {\n"
            << "  TAKE_EVENT(PC);\n"
            << "}\n";
}

static void
emitCheckEventsOrDeschedule(const Instruction &instruction)
{
  if (!instruction.getCanEvent()) {
    std::cout << "DESCHEDULE(PC);\n";
    return;
  }
  std::cout << "if (sys.hasPendingEvent()) {\n"
            << "  TAKE_EVENT(PC);\n"
            << "} else {\n"
            << "  DESCHEDULE(PC);\n"
            << "}\n";
}

static void
emitTraceEnd()
{
  std::cout << "TRACE_END();\n";
}

static void
emitCode(const Instruction &instruction,
         const std::string &code,
         bool &emitEndLabel);

static void
emitCode(const Instruction &instruction,
         const std::string &code)
{
  bool dummy;
  emitCode(instruction, code, dummy);
}

class CodeEmitter {
public:
  void emit(const std::string &code);
protected:
  void emitNested(const std::string &code);
  virtual void emitBegin() = 0;
  virtual void emitRaw(const std::string &s) = 0;
  void emitRaw(char c) { emitRaw(std::string(1, c)); }
  virtual void emitOp(unsigned num) = 0;
  virtual void emitNextPc() = 0;
  virtual void emitException(const std::string &args) = 0;
  virtual void emitKCall(const std::string &args) = 0;
  virtual void emitPauseOn(const std::string &args) = 0;
  virtual void emitYield() = 0;
  virtual void emitDeschedule() = 0;
  virtual void emitStoreWord(const std::string &args) = 0;
  virtual void emitStoreShort(const std::string &args) = 0;
  virtual void emitStoreByte(const std::string &args) = 0;
  virtual void emitLoadWord(const std::string &args) = 0;
  virtual void emitLoadShort(const std::string &args) = 0;
  virtual void emitLoadByte(const std::string &args) = 0;
};

void CodeEmitter::emit(const std::string &code)
{
  emitBegin();
  emitNested(code);
}

void CodeEmitter::emitNested(const std::string &code)
{
  const char *s = code.c_str();
  for (unsigned i = 0, e = code.size(); i != e; i++) {
    if (s[i] == '%') {
      if (i + 1 == e) {
        std::cerr << "error: % at end of code string\n";
        std::exit(1);
      }
      i++;
      if (s[i] == '%') {
        emitRaw('%');
      } else if (isdigit(s[i])) {
        char *endp;
        long value = std::strtol(&s[i], &endp, 10);
        i = (endp - s) - 1;
        if (value < 0) {
          std::cerr << "error: operand out of range in code string\n";
          std::exit(1);
        }
        emitOp(value);
      } else if (std::strncmp(&s[i], "pc", 2) == 0) {
        emitNextPc();
        i++;
      } else if (std::strncmp(&s[i], "exception(", 10) == 0) {
        i += 10;
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitException(content);
        i = (close - s);
      } else if (std::strncmp(&s[i], "kcall(", 6) == 0) {
        i += 6;
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitKCall(content);
        i = (close - s);
      } else if (std::strncmp(&s[i], "pause_on(", 9) == 0) {
        i += 9;
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitPauseOn(content);
        i = (close - s);
      } else if (std::strncmp(&s[i], "yield", 5) == 0) {
        i += 4;
        emitYield();
      } else if (std::strncmp(&s[i], "deschedule", 10) == 0) {
        i += 10;
        emitDeschedule();
      } else if (std::strncmp(&s[i], "store_word(", 11) == 0) {
        i += 11;
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitStoreWord(content);
        i = (close - s);
      } else if (std::strncmp(&s[i], "store_short(", 12) == 0) {
        i += 12;
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitStoreShort(content);
        i = (close - s);
      } else if (std::strncmp(&s[i], "store_byte(", 11) == 0) {
        i += 11;
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitStoreByte(content);
        i = (close - s);
      } else if (std::strncmp(&s[i], "load_word(", 10) == 0) {
        i += 10;
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitLoadWord(content);
        i = (close - s);
      } else if (std::strncmp(&s[i], "load_short(", 11) == 0) {
        i += 11;
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitLoadShort(content);
        i = (close - s);
      } else if (std::strncmp(&s[i], "load_byte(", 10) == 0) {
        i += 10;
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitLoadByte(content);
        i = (close - s);
      } else {
        std::cerr << "error: stray % in code string\n";
        std::exit(1);
      }
    } else {
      emitRaw(s[i]);
    }
  }
}

static void emitUpdateExecutionFrequency(const Instruction &inst)
{
  if (inst.getMayBranch()) {
    std::cout << "if (!tracing) {\n";
    std::cout << "  CORE.updateExecutionFrequency(PC);\n";
    std::cout << "}\n";
  }
}

class InlineCodeEmitter : public CodeEmitter {
  bool emitEndLabel;
  const Instruction *inst;
public:
  InlineCodeEmitter() : emitEndLabel(false) {}
  void setInstruction(const Instruction &i) { inst = &i; }
  bool getEmitEndLabel() const { return emitEndLabel; }
protected:
  virtual void emitBegin();
  virtual void emitRaw(const std::string &s);
  virtual void emitOp(unsigned);
  virtual void emitNextPc();
  virtual void emitException(const std::string &args);
  virtual void emitKCall(const std::string &args);
  virtual void emitPauseOn(const std::string &args);
  virtual void emitYield();
  virtual void emitDeschedule();
  virtual void emitStoreWord(const std::string &args);
  virtual void emitStoreShort(const std::string &args);
  virtual void emitStoreByte(const std::string &args);
  virtual void emitLoadWord(const std::string &args);
  virtual void emitLoadShort(const std::string &args);
  virtual void emitLoadByte(const std::string &args);
};

void InlineCodeEmitter::emitBegin()
{
  emitEndLabel = false;
}

void InlineCodeEmitter::emitRaw(const std::string &s)
{
  std::cout << s;
}

void InlineCodeEmitter::emitOp(unsigned num)
{
  if (num >= inst->getOperands().size()) {
    std::cerr << "error: operand out of range in code string\n";
    std::exit(1);
  }
  std::cout << "op" << num;
}

void InlineCodeEmitter::emitNextPc()
{
  std::cout << "nextPc";
}

void InlineCodeEmitter::emitException(const std::string &args)
{
  emitCycles(*inst);
  emitTraceEnd();
  std::cout << "EXCEPTION(";
  emitNested(args);
  std::cout << ");\n";
  std::cout << "goto " << getEndLabel(*inst) << ";\n";
  emitEndLabel = true;
}

void InlineCodeEmitter::emitKCall(const std::string &args)
{
  emitCycles(*inst);
  emitRegWriteBack(*inst);
  emitTraceEnd();
  std::cout << "EXCEPTION(ET_KCALL, ";
  emitNested(args);
  std::cout << ");\n";
  std::cout << "goto " << getEndLabel(*inst) << ";\n";
  emitEndLabel = true;
}

void InlineCodeEmitter::emitPauseOn(const std::string &args)
{
  emitCycles(*inst);
  emitTraceEnd();
  std::cout << "PAUSE_ON(PC, ";
  emitNested(args);
  std::cout << ");\n";
  std::cout << "goto " << getEndLabel(*inst) << ";\n";
  emitEndLabel = true;
}

void InlineCodeEmitter::emitYield()
{
  emitCycles(*inst);
  emitRegWriteBack(*inst);
  emitCheckEvents(*inst);
  emitTraceEnd();
  emitUpdateExecutionFrequency(*inst);
  std::cout << "YIELD(PC);\n";
  std::cout << "goto " << getEndLabel(*inst) << ";\n";
  emitEndLabel = true;
}

void InlineCodeEmitter::emitDeschedule()
{
  emitCycles(*inst);
  emitCheckEventsOrDeschedule(*inst);
  emitTraceEnd();
  std::cout << "goto " << getEndLabel(*inst) << ";\n";
  emitEndLabel = true;
}

void InlineCodeEmitter::emitStoreWord(const std::string &args)
{
  std::cout << "STORE_WORD(";
  emitNested(args);
  std::cout << ")";
}

void InlineCodeEmitter::emitStoreShort(const std::string &args)
{
  std::cout << "STORE_SHORT(";
  emitNested(args);
  std::cout << ")";
}

void InlineCodeEmitter::emitStoreByte(const std::string &args)
{
  std::cout << "STORE_BYTE(";
  emitNested(args);
  std::cout << ")";
}

void InlineCodeEmitter::emitLoadWord(const std::string &args)
{
  std::cout << "LOAD_WORD(";
  emitNested(args);
  std::cout << ")";
}

void InlineCodeEmitter::emitLoadShort(const std::string &args)
{
  std::cout << "LOAD_SHORT(";
  emitNested(args);
  std::cout << ")";
}

void InlineCodeEmitter::emitLoadByte(const std::string &args)
{
  std::cout << "LOAD_BYTE(";
  emitNested(args);
  std::cout << ")";
}

class FunctionCodeEmitter : public CodeEmitter {
public:
  const Instruction *inst;
  void emitCycles();
  void emitRegWriteback();
  void setInstruction(const Instruction &i) { inst = &i; }
protected:
  virtual void emitBegin();
  virtual void emitRaw(const std::string &s);
  virtual void emitOp(unsigned);
  virtual void emitNextPc();
  virtual void emitException(const std::string &args);
  virtual void emitKCall(const std::string &args);
  virtual void emitPauseOn(const std::string &args);
  virtual void emitYield();
  virtual void emitDeschedule();
  virtual void emitStoreWord(const std::string &args);
  virtual void emitStoreShort(const std::string &args);
  virtual void emitStoreByte(const std::string &args);
  virtual void emitLoadWord(const std::string &args);
  virtual void emitLoadShort(const std::string &args);
  virtual void emitLoadByte(const std::string &args);
};

void FunctionCodeEmitter::emitCycles()
{
  std::cout << "THREAD.time += " << inst->getCycles() << ";\n";
}

void FunctionCodeEmitter::emitRegWriteback()
{
  const std::vector<OpType> &operands = inst->getOperands();
  for (unsigned i = 0, e = operands.size(); i != e; ++i) {
    switch (operands[i]) {
    default:
      break;
    case out:
    case inout:
      std::cout << "THREAD.regs[" << getOperandName(*inst, i);
      std::cout << "] = ";
      std::cout << "op" << i << ";\n";
      break;
    }
  }
  std::cout << "THREAD.pc = nextPc;\n";
}

void FunctionCodeEmitter::emitBegin()
{
}

void FunctionCodeEmitter::emitRaw(const std::string &s)
{
  std::cout << s;
}

void FunctionCodeEmitter::emitOp(unsigned num)
{
  if (num >= inst->getOperands().size()) {
    std::cerr << "error: operand out of range in code string\n";
    std::exit(1);
  }
  std::cout << "op" << num;
}

void FunctionCodeEmitter::emitNextPc()
{
  std::cout << "nextPc";
}

void FunctionCodeEmitter::emitException(const std::string &args)
{
  std::cout << "THREAD.pc = exception(THREAD, THREAD.pc, ";
  emitNested(args);
  std::cout << ");\n";
  emitCycles();
  std::cout << "return JIT_RETURN_YIELD;\n";
}

void FunctionCodeEmitter::emitKCall(const std::string &args)
{
  assert(0);
}

void FunctionCodeEmitter::emitPauseOn(const std::string &args)
{
  assert(0);
}

void FunctionCodeEmitter::emitYield()
{
  emitRegWriteback();
  emitCycles();
  std::cout << "return JIT_RETURN_YIELD;\n";
}

void FunctionCodeEmitter::emitDeschedule()
{
  assert(0);
}

void FunctionCodeEmitter::emitStoreWord(const std::string &args)
{
  std::cout << "if (STORE_WORD(";
  emitNested(args);
  std::cout << ")) {\n";
  std::cout << "retval = JIT_RETURN_END_TRACE;\n";
  std::cout << "}\n";
}

void FunctionCodeEmitter::emitStoreShort(const std::string &args)
{
  std::cout << "if (STORE_SHORT(";
  emitNested(args);
  std::cout << ")) {\n";
  std::cout << "retval = JIT_RETURN_END_TRACE;\n";
  std::cout << "}\n";
}

void FunctionCodeEmitter::emitStoreByte(const std::string &args)
{
  std::cout << "if (STORE_BYTE(";
  emitNested(args);
  std::cout << ")) {\n";
  std::cout << "retval = JIT_RETURN_END_TRACE;\n";
  std::cout << "}\n";
}

void FunctionCodeEmitter::emitLoadWord(const std::string &args)
{
  std::cout << "LOAD_WORD(";
  emitNested(args);
  std::cout << ")";
}

void FunctionCodeEmitter::emitLoadShort(const std::string &args)
{
  std::cout << "LOAD_SHORT(";
  emitNested(args);
  std::cout << ")";
}

void FunctionCodeEmitter::emitLoadByte(const std::string &args)
{
  std::cout << "LOAD_BYTE(";
  emitNested(args);
  std::cout << ")";
}

class CodePropertyExtractor : public CodeEmitter {
  Instruction *inst;
protected:
  virtual void emitBegin() {}
  virtual void emitRaw(const std::string &s) {}
  virtual void emitOp(unsigned) {}
  virtual void emitNextPc() {
    inst->setUsesPc();
  }
  virtual void emitException(const std::string &args) {
    inst->setMayExcept();
    emitNested(args);
  }
  virtual void emitKCall(const std::string &args) {
    inst->setMayKCall();
    emitNested(args);
  }
  virtual void emitPauseOn(const std::string &args) {
    inst->setMayPauseOn();
    emitNested(args);
  }
  virtual void emitYield() { inst->setMayYield(); }
  virtual void emitDeschedule() { inst->setMayDeschedule(); }
  virtual void emitStoreWord(const std::string &args) {
    inst->setMayStore();
    emitNested(args);
  }
  virtual void emitStoreShort(const std::string &args) {
    inst->setMayStore();
    emitNested(args);
  }
  virtual void emitStoreByte(const std::string &args) {
    inst->setMayStore();
    emitNested(args);
  }
  virtual void emitLoadWord(const std::string &args) {
    inst->setMayLoad();
    emitNested(args);
  }
  virtual void emitLoadShort(const std::string &args) {
    inst->setMayLoad();
    emitNested(args);
  }
  virtual void emitLoadByte(const std::string &args) {
    inst->setMayLoad();
    emitNested(args);
  }
public:
  void setInstruction(Instruction &i) { inst = &i; }
  CodePropertyExtractor() : inst(0) {}
};

static void
emitCode(const Instruction &instruction,
         const std::string &code,
         bool &emitEndLabel)
{
  InlineCodeEmitter emitter;
  emitter.setInstruction(instruction);
  emitter.emit(code);
  emitEndLabel = emitter.getEmitEndLabel();
}

static std::string
quote(const std::string &s)
{
  std::ostringstream buf;
  quote(buf, s);
  return buf.str();
}

static void
scanFormatArgs(const char *format, Instruction &instruction,
               std::vector<std::string> &args)
{
  const std::vector<OpType> &operands = instruction.getOperands();

  std::ostringstream buf;
  for (const char *p = format; *p != '\0'; ++p) {
    switch (*p) {
    default:
      buf << *p;
      break;
    case '%':
      if (*++p == '\0') {
        std::cerr << "error: % at end of format string\n";
        std::exit(1);
      }
      if (*p == '%') {
        buf << '%';
      } else {
        enum {
          RELATIVE_NONE,
          DP_RELATIVE,
          CP_RELATIVE,
        } relType = RELATIVE_NONE;
        if (std::strncmp(p, "{dp}", 4) == 0) {
          relType = DP_RELATIVE;
          p += 4;
        } else if (std::strncmp(p, "{cp}", 4) == 0) {
          relType = CP_RELATIVE;
          p += 4;
        }
        if (isdigit(*p)) {
          char *endp;
          long value = std::strtol(p, &endp, 10);
          p = endp - 1;
          if (value < 0 || (unsigned)value >= operands.size()) {
            std::cerr << "error: operand out of range in format string\n";
            std::exit(1);
          }
          if (!buf.str().empty()) {
            args.push_back(quote(buf.str()));
            buf.str("");
          }
          switch (operands[value]) {
          default: assert(0 && "Unexpected operand type");
          case out:
            buf << "Register(" << getOperandName(instruction, value) << ')';
            break;
          case in:
          case inout:
            buf << "SrcRegister(" << getOperandName(instruction, value) << ')';
            break;
          case imm:
            switch (relType) {
            case RELATIVE_NONE:
              buf << "op" << value;
              break;
            case CP_RELATIVE:
              buf << "CPRelOffset(op" << value << ')';
              break;
            case DP_RELATIVE:
              buf << "DPRelOffset(op" << value << ')';
              break;
            }
            break;
          }
          args.push_back(buf.str());
          buf.str("");
        } else {
          std::cerr << "error: stray % in format string\n";
          std::exit(1);
        }
      }
      break;
    }
  }
  if (!buf.str().empty()) {
    args.push_back(quote(buf.str()));
    buf.str("");
  }  
}

static void
emitTrace(Instruction &instruction)
{
  const std::string &format = instruction.getFormat();

  std::vector<std::string> args;
  scanFormatArgs(format.c_str(), instruction, args);
  
  std::cout << "if (tracing) {\n";
  if (!instruction.getReverseTransform().empty()) {
    emitCode(instruction, instruction.getReverseTransform());
    std::cout << '\n';
  }
  std::cout << "TRACE(";
  bool needComma = false;
  for (std::vector<std::string>::iterator it = args.begin(), e = args.end();
       it != e; ++it) {
    if (needComma)
      std::cout << ", ";
    std::cout << *it;
    needComma = true;
  }
  std::cout << ");\n";
  if (!instruction.getTransform().empty()) {
    emitCode(instruction, instruction.getTransform());
    std::cout << '\n';
  }
  std::cout << "}\n";
}

static std::string getInstFunctionName(Instruction &inst)
{
  return "Instruction_" + inst.getName();
}

static void
emitInstDispatch(Instruction &instruction)
{
  if (instruction.getCustom())
    return;
  const std::string &name = instruction.getName();
  std::cout << "INST(" << name << "):";

  unsigned size = instruction.getSize();
  const std::vector<OpType> &operands = instruction.getOperands();
  const std::string &code = instruction.getCode();


  if (size % 2 != 0) {
    std::cerr << "error: unexpected instruction size " << size << "\n";
    std::exit(1);
  }
  if (instruction.getSync()) {
    std::cout << "\n"
                 "if (sys.hasTimeSliceExpired(TIME)) {\n"
                 "  YIELD(PC);\n"
                 "} else";
  }
  std::cout << " {\n";
  // Read operands.
  for (unsigned i = 0, e = operands.size(); i != e; ++i) {
    std::cout << "UNUSED(";
    if (operands[i] == in)
      std::cout << "const ";
    if (isSR(instruction, i)) {
      std::cout << "Thread::sr_t op" << i << ") = this->sr";
    } else {
      std::cout << "uint32_t op" << i << ')';
      switch (operands[i]) {
        default:
          break;
        case in:
        case inout:
          std::cout << " = REG(" << getOperandName(instruction, i) << ')';
          break;
        case imm:
          std::cout << " = IMM(" << getOperandName(instruction, i) << ')';
          break;
      }
    }
    std::cout << ";\n";
  }
  if (!instruction.getUnimplemented()) {
    std::cout << "uint32_t nextPc = PC + " << size / 2 << ";\n";
  }
  emitTrace(instruction);

  // Do operation.
  bool emitEndLabel = false;
  if (instruction.getUnimplemented()) {
    std::cout << "ERROR();\n";
  } else {
    emitCode(instruction, code, emitEndLabel);
    std::cout << '\n';
    // Write operands.
    emitCycles(instruction);
    emitRegWriteBack(instruction);
    emitCheckEvents(instruction);
    emitTraceEnd();
    emitUpdateExecutionFrequency(instruction);
  }
  std::cout << "}\n";
  if (emitEndLabel)
    std::cout << getEndLabel(instruction) << ":;\n";
  std::cout << "ENDINST;\n";
}

static void emitInstDispatch()
{
  std::cout << "#ifdef EMIT_INSTRUCTION_DISPATCH\n";
  for (std::vector<Instruction*>::iterator it = instructions.begin(),
       e = instructions.end(); it != e; ++it) {
    emitInstDispatch(**it);
  }
  std::cout << "#endif //EMIT_INSTRUCTION_DISPATCH\n";
}

static void emitInstFunction(Instruction &inst)
{
  if (!inst.getCanJit())
    return;
  std::cout << "extern \"C\" JITReturn " << getInstFunctionName(inst) << '(';
  std::cout << "Thread &thread, uint32_t nextPc";
  for (unsigned i = 0, e = inst.getNumExplicitOperands(); i != e; ++i) {
    std::cout << ", uint32_t field" << i;
  }
  std::cout << ") {\n";
  // Read operands.
  const std::vector<OpType> &operands = inst.getOperands();
  for (unsigned i = 0, e = operands.size(); i != e; ++i) {
    std::cout << "UNUSED(";
    if (operands[i] == in)
      std::cout << "const ";
    std::cout << "uint32_t op" << i << ')';
    switch (operands[i]) {
    default:
      break;
    case in:
    case inout:
      std::cout << " = THREAD.regs[" << getOperandName(inst, i) << ']';
      break;
    case imm:
      std::cout << " = field" << i;
      break;
    }
    std::cout << ";\n";
  }
  if (inst.getMayStore()) {
    std::cout << "JITReturn retval = JIT_RETURN_CONTINUE;\n";
  }
  FunctionCodeEmitter emitter;
  emitter.setInstruction(inst);
  emitter.emit(inst.getCode());
  std::cout << '\n';
  // Write operands.
  emitter.emitRegWriteback();
  emitter.emitCycles();
  if (inst.getMayStore()) {
    std::cout << "return retval;\n";
  } else {
    std::cout << "return JIT_RETURN_CONTINUE;\n";
  }
  std::cout << "}\n";
}

static void emitInstFunctions()
{
  std::cout << "#ifdef EMIT_INSTRUCTION_FUNCTIONS\n";
  for (std::vector<Instruction*>::iterator it = instructions.begin(),
       e = instructions.end(); it != e; ++it) {
    emitInstFunction(**it);
  }
  std::cout << "#endif //EMIT_INSTRUCTION_FUNCTIONS\n";
}

static void emitInstList(Instruction &instruction)
{
  std::cout << "DO_INSTRUCTION(" << instruction.getName() << ")\n";
}

static void emitInstList()
{
  std::cout << "#ifdef EMIT_INSTRUCTION_LIST\n";
  for (std::vector<Instruction*>::iterator it = instructions.begin(),
       e = instructions.end(); it != e; ++it) {
    emitInstList(**it);
  }
  std::cout << "#endif //EMIT_INSTRUCTION_LIST\n";
}

static void analyzeInst(Instruction &inst) {
  if (inst.getCustom() || inst.getUnimplemented())
    return;
  CodePropertyExtractor propertyExtractor;
  propertyExtractor.setInstruction(inst);
  propertyExtractor.emit(inst.getCode());
  if (inst.getCanEvent() || inst.getSync())
    return;
  const std::vector<OpType> &operands = inst.getOperands();
  for (unsigned i = 0, numOps = operands.size(); i != numOps; ++i) {
    if (isSR(inst, i)) {
      return;
    }
  }
  if (inst.getMayKCall() ||
      inst.getMayPauseOn() ||
      inst.getMayDeschedule())
    return;
  inst.setCanJit();
}

static void analyze()
{
  CodePropertyExtractor propertyExtractor;
  for (std::vector<Instruction*>::iterator it = instructions.begin(),
       e = instructions.end(); it != e; ++it) {
    analyzeInst(**it);
  }
}

static void emitInstFlag(const char *name, bool &emittedFlag)
{
  if (emittedFlag)
    std::cout << " | ";
  std::cout << "InstructionProperties::" << name;
  emittedFlag = true;
}

static void emitInstFlags(Instruction &inst)
{
  bool emittedFlag = false;
  if (inst.getMayBranch()) {
    emitInstFlag("MAY_BRANCH", emittedFlag);
  }
  if (inst.getMayYield() || inst.getMayExcept()) {
    emitInstFlag("MAY_YIELD", emittedFlag);
  }
  if (inst.getMayStore())
    emitInstFlag("MAY_END_TRACE", emittedFlag);
  if (!emittedFlag)
    std::cout << 0;
}

static void emitInstProperties(Instruction &inst)
{
  std::cout << "{ ";
  if (inst.getCanJit())
    std::cout << '"' << getInstFunctionName(inst) << '"';
  else
    std::cout << '0';
  std::cout << ", " << inst.getSize();
  std::cout << ", " << inst.getNumExplicitOperands();
  std::cout << ", ";
  emitInstFlags(inst);
  std::cout << " }";
}

static void emitInstProperties()
{
  std::cout << "#ifdef EMIT_INSTRUCTION_PROPERTIES\n";
  bool needComma = false;
  for (std::vector<Instruction*>::iterator it = instructions.begin(),
       e = instructions.end(); it != e; ++it) {
    if (needComma)
      std::cout << ",\n";
    emitInstProperties(**it);
    needComma = true;
  }
  std::cout << '\n';
  std::cout << "#endif //EMIT_INSTRUCTION_PROPERTIES\n";
}

#define INSTRUCTION_CYCLES 4
/// Time to execute a divide instruction in 400MHz clock cycles. This is
/// approximate. The XCore divide unit divides 1 bit per cycle and is shared
/// between threads.
#define DIV_CYCLES 32

Instruction &
f3r(const std::string &name,
    const std::string &format,
    const std::string &code)
{
  return inst(name + "_3r", 2, ops(out, in, in), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
f2rus(const std::string &name,
      const std::string &format,
      const std::string &code)
{
  return inst(name + "_2rus", 2, ops(out, in, imm), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
f2rus_in(const std::string &name,
         const std::string &format,
         const std::string &code)
{
  return inst(name + "_2rus", 2, ops(in, in, imm), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl3r(const std::string &name,
     const std::string &format,
     const std::string &code)
{
  return inst(name + "_l3r", 4, ops(out, in, in), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl3r_in(const std::string &name,
        const std::string &format,
        const std::string &code)
{
  return inst(name + "_l3r", 4, ops(in, in, in), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl3r_inout(const std::string &name,
           const std::string &format,
           const std::string &code)
{
  return inst(name + "_l3r", 4, ops(inout, in, in), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl2rus(const std::string &name,
       const std::string &format,
       const std::string &code)
{
  return inst(name + "_l2rus", 4, ops(out, in, imm), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl2rus_in(const std::string &name,
          const std::string &format,
          const std::string &code)
{
  return inst(name + "_l2rus", 4, ops(in, in, imm), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl4r_inout_inout(const std::string &name,
                 const std::string &format,
                 const std::string &code)
{
  return inst(name + "_l4r", 4, ops(inout, in, in, inout), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl4r_out_inout(const std::string &name,
                 const std::string &format,
                 const std::string &code)
{
  return inst(name + "_l4r", 4, ops(out, in, in, inout), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl5r(const std::string &name,
     const std::string &format,
     const std::string &code)
{
  return inst(name + "_l5r", 4, ops(out, in, in, out, in), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl6r(const std::string &name,
     const std::string &format,
     const std::string &code)
{
  return inst(name + "_l6r", 4, ops(out, in, in, out, in, in), format, code,
              INSTRUCTION_CYCLES);
}

InstructionRefs
fru6_out(const std::string &name,
         const std::string &format,
         const std::string &code)
{
  InstructionRefs refs;
  refs.add(&inst(name + "_ru6", 2, ops(out, imm), format, code,
                 INSTRUCTION_CYCLES));
  refs.add(&inst(name + "_lru6", 4, ops(out, imm), format, code,
                 INSTRUCTION_CYCLES));
  return refs;
}

InstructionRefs
fru6_in(const std::string &name,
        const std::string &format,
        const std::string &code)
{
  InstructionRefs refs;
  refs.add(&inst(name + "_ru6", 2, ops(in, imm), format, code,
                 INSTRUCTION_CYCLES));
  refs.add(&inst(name + "_lru6", 4, ops(in, imm), format, code,
                 INSTRUCTION_CYCLES));
  return refs;
}

InstructionRefs
fu6(const std::string &name,
    const std::string &format,
    const std::string &code)
{
  InstructionRefs refs;
  refs.add(&inst(name + "_u6", 2, ops(imm), format, code, INSTRUCTION_CYCLES));
  refs.add(&inst(name + "_lu6", 4, ops(imm), format, code, INSTRUCTION_CYCLES));
  return refs;
}

InstructionRefs
fu10(const std::string &name,
     const std::string &format,
     const std::string &code)
{
  InstructionRefs refs;
  refs.add(&inst(name + "_u10", 2, ops(imm), format, code, INSTRUCTION_CYCLES));
  refs.add(&inst(name + "_lu10", 4, ops(imm), format, code, INSTRUCTION_CYCLES));
  return refs;
}

Instruction &
f2r(const std::string &name,
    const std::string &format,
    const std::string &code)
{
  return inst(name + "_2r", 2, ops(out, in), format, code, INSTRUCTION_CYCLES);
}

Instruction &
f2r_in(const std::string &name,
       const std::string &format,
       const std::string &code)
{
  return inst(name + "_2r", 2, ops(in, in), format, code, INSTRUCTION_CYCLES);
}

Instruction &
f2r_inout(const std::string &name,
          const std::string &format,
          const std::string &code)
{
  return inst(name + "_2r", 2, ops(inout, in), format, code, INSTRUCTION_CYCLES);
}

Instruction &
frus(const std::string &name,
     const std::string &format,
     const std::string &code)
{
  return inst(name + "_rus", 2, ops(out, imm), format, code, INSTRUCTION_CYCLES);
}

Instruction &
frus_in(const std::string &name,
        const std::string &format,
        const std::string &code)
{
  return inst(name + "_rus", 2, ops(in, imm), format, code, INSTRUCTION_CYCLES);
}

Instruction &
frus_inout(const std::string &name,
           const std::string &format,
           const std::string &code)
{
  return inst(name + "_rus", 2, ops(inout, imm), format, code,
              INSTRUCTION_CYCLES);
}

Instruction &
fl2r(const std::string &name,
     const std::string &format,
     const std::string &code)
{
  return inst(name + "_l2r", 4, ops(out, in), format, code, INSTRUCTION_CYCLES);
}

Instruction &
fl2r_in(const std::string &name,
        const std::string &format,
        const std::string &code)
{
  return inst(name + "_l2r", 4, ops(in, in), format, code, INSTRUCTION_CYCLES);
}

Instruction &
f1r(const std::string &name,
    const std::string &format,
    const std::string &code)
{
  return inst(name + "_1r", 2, ops(in), format, code, INSTRUCTION_CYCLES);
}

Instruction &
f1r_out(const std::string &name,
        const std::string &format,
        const std::string &code)
{
  return inst(name + "_1r", 2, ops(out), format, code, INSTRUCTION_CYCLES);
}

Instruction &
f0r(const std::string &name,
    const std::string &format,
    const std::string &code)
{
  return inst(name + "_0r", 2, ops(), format, code, INSTRUCTION_CYCLES);
}

Instruction &
pseudoInst(const std::string &name,
           const std::string &format,
           const std::string &code)
{
  return inst(name, 0, ops(), format, code, 0);
}

void add()
{
  f3r("ADD", "add %0, %1, %2", "%0 = %1 + %2;");
  f2rus("ADD", "add %0, %1, %2", "%0 = %1 + %2;");
  f2rus("ADD_mov", "mov %0, %1", "%0 = %1;");
  f3r("SUB", "sub %0, %1, %2", "%0 = %1 - %2;");
  f2rus("SUB", "sub %0, %1, %2", "%0 = %1 - %2;");
  f3r("EQ", "eq %0, %1, %2", "%0 = %1 == %2;");
  f2rus("EQ", "eq %0, %1, %2", "%0 = %1 == %2;");
  f3r("LSS", "lss %0, %1, %2", "%0 = (int32_t)%1 < (int32_t)%2;");
  f3r("LSU", "lsu %0, %1, %2", "%0 = %1 < %2;");
  f3r("AND", "and %0, %1, %2", "%0 = %1 & %2;");
  f3r("OR", "or %0, %1, %2", "%0 = %1 | %2;");
  f3r("SHL", "shl %0, %1, %2",
      "if (%2 >= 32) {\n"
      "  %0 = 0;\n"
      "} else {\n"
      "  %0 = %1 << %2;\n"
      "}");
  f2rus("SHL", "shl %0, %1, %2",
        "assert(%2 != 32 && \"SHL_32_2rus should be used for shl by immediate 32\");\n"
        "%0 = %1 << %2;");
  f2rus("SHL_32", "shl %0, %1, 32", "%0 = 0;");
  f3r("SHR", "shr %0, %1, %2",
      "if (%2 >= 32) {\n"
      "  %0 = 0;\n"
      "} else {\n"
      "  %0 = %1 >> %2;\n"
      "}");
  f2rus("SHR", "shr %0, %1, %2",
        "assert(%2 != 32 && \"SHR_32_2rus should be used for shr by immediate 32\");\n"
        "%0 = %1 >> %2;");
  f2rus("SHR_32", "shr %0, %1, 32", "%0 = 0;");
  f3r("LDW", "ldw %0, %1[%2]",
      "uint32_t Addr = %1 + (%2 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)"
      "}\n"
      "%0 = LOAD_WORD(PhyAddr);\n");
  f2rus("LDW", "ldw %0, %1[%2]",
        "uint32_t Addr = %1 + %2;\n"
        "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
        "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
        "  %exception(ET_LOAD_STORE, Addr)"
        "}\n"
        "%0 = LOAD_WORD(PhyAddr);\n")
    .transform("%2 = %2 << 2;", "%2 = %2 >> 2;");
  f3r("LD16S", "ld16s %0, %1[%2]",
      "uint32_t Addr = %1 + (%2 << 1);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_SHORT(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)"
      "}\n"
      "%0 = signExtend(LOAD_SHORT(PhyAddr), 16);\n");
  f3r("LD8U", "ld8u %0, %1[%2]",
      "uint32_t Addr = %1 + %2;\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)"
      "}\n"
      "%0 = LOAD_BYTE(PhyAddr);\n");
  f2rus_in("STW", "stw %0, %1[%2]",
           "uint32_t Addr = %1 + %2;\n"
           "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
           "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
           "  %exception(ET_LOAD_STORE, Addr)"
           "}\n"
           "%store_word(%0, PhyAddr);\n")
    .transform("%2 = %2 << 2;", "%2 = %2 >> 2;");
  // TSETR needs special handling as one operands is not a register on the
  // current thread.
  inst("TSETR_3r", 2, ops(imm, in, in), "set t[%2]:r%0, %1",
       "ResourceID resID(%2);\n"
       "if (Thread *t = checkThread(CORE, resID)) {\n"
       "  t->reg(%0) = %1;\n"
       "} else {\n"
       "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
       "}",
       INSTRUCTION_CYCLES);
  fl3r("LDAWF", "ldaw %0, %1[%2]", "%0 = %1 + (%2 << 2);");
  fl2rus("LDAWF", "ldaw %0, %1[%2]", "%0 = %1 + %2;")
         .transform("%2 = %2 << 2;", "%2 = %2 >> 2;");
  fl3r("LDAWB", "ldaw %0, %1[-%2]", "%0 = %1 - (%2 << 2);");
  fl2rus("LDAWB", "ldaw %0, %1[-%2]", "%0 = %1 - %2;")
         .transform("%2 = %2 << 2;", "%2 = %2 >> 2;");
  fl3r("LDA16F", "lda16 %0, %1[%2]", "%0 = %1 + (%2 << 1);");
  fl3r("LDA16B", "lda16 %0, %1[-%2]", "%0 = %1 - (%2 << 1);");
  fl3r_in("STW", "stw %0, %1[%2]",
          "uint32_t Addr = %1 + (%2 << 2);\n"
          "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
          "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
          "  %exception(ET_LOAD_STORE, Addr)"
          "}\n"
          "%store_word(%0, PhyAddr);\n");
  fl3r_in("ST16", "st16 %0, %1[%2]",
          "uint32_t Addr = %1 + (%2 << 1);\n"
          "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
          "if (!CHECK_ADDR_SHORT(PhyAddr)) {\n"
          "  %exception(ET_LOAD_STORE, Addr)"
          "}\n"
          "%store_short(%0, PhyAddr);\n");
  fl3r_in("ST8", "st8 %0, %1[%2]",
          "uint32_t Addr = %1 + %2;\n"
          "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
          "if (!CHECK_ADDR(PhyAddr)) {\n"
          "  %exception(ET_LOAD_STORE, Addr)"
          "}\n"
          "%store_byte(%0, PhyAddr);\n");
  fl3r("MUL", "mul %0, %1, %2", "%0 = %1 * %2;");
  fl3r("DIVS", "divs %0, %1, %2",
       "if (%2 == 0 ||\n"
       "    (%1 == 0x80000000 && %2 == 0xffffffff)) {\n"
       "  %exception(ET_ARITHMETIC, 0)"
       "}\n"
       "%0 = (int32_t)%1 / (int32_t)%2;")
    .setCycles(DIV_CYCLES);
  fl3r("DIVU", "divu %0, %1, %2",
       "if (%2 == 0) {\n"
       "  %exception(ET_ARITHMETIC, 0)"
       "}\n"
       "%0 = %1 / %2;")
  .setCycles(DIV_CYCLES);
  fl3r("REMS", "rems %0, %1, %2",
       "if (%2 == 0 ||\n"
       "    (%1 == 0x80000000 && %2 == 0xffffffff)) {\n"
       "  %exception(ET_ARITHMETIC, 0)"
       "}\n"
       "%0 = (int32_t)%1 %% (int32_t)%2;")
    .setCycles(DIV_CYCLES);
  fl3r("REMU", "remu %0, %1, %2",
       "if (%2 == 0) {\n"
       "  %exception(ET_ARITHMETIC, 0)"
       "}\n"
       "%0 = %1 %% %2;")
    .setCycles(DIV_CYCLES);
  fl3r("XOR", "xor %0, %1, %2", "%0 = %1 ^ %2;");
  fl3r("ASHR", "ashr %0, %1, %2",
       "if (%2 >= 32) {\n"
       "  %0 = (int32_t)%1 >> 31;\n"
       "} else {\n"
       "  %0 = (int32_t)%1 >> %2\n;"
       "}");
  fl2rus("ASHR", "ashr %0, %1, %2",
         "assert(%2 != 32 && \"ASHR_32_2rus should be used for ashr by immediate 32\");\n"
         "%0 = (int32_t)%1 >> %2;");
  fl2rus("ASHR_32", "ashr %0, %1, 32", "%0 = (int32_t)%1 >> 31;");
  fl2rus_in("OUTPW", "outpw res[%1], %0, %2",
            "ResourceID resID(%1);\n"
            "if (Port *res = checkPort(CORE, resID)) {\n"
            "  switch (res->outpw(THREAD, %0, %2, TIME)) {\n"
            "  default: assert(0 && \"Unexpected outpw result\");\n"
            "  case Resource::CONTINUE:\n"
            "    break;\n"
            "  case Resource::DESCHEDULE:\n"
            "    %pause_on(res);\n"
            "  case Resource::ILLEGAL:\n"
            "    %exception(ET_ILLEGAL_RESOURCE, resID);\n"
            "  }\n"
            "} else {\n"
            "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
            "}\n")
    .setSync();
  fl2rus("INPW", "inpw %0, res[%1], %2",
         "ResourceID resID(%1);\n"
         "if (Port *res = checkPort(CORE, resID)) {\n"
         "  uint32_t value;\n"
         "  switch (res->inpw(THREAD, %2, TIME, value)) {\n"
         "  default: assert(0 && \"Unexpected inpw result\");\n"
         "  case Resource::CONTINUE:\n"
         "    %0 = value;\n"
         "    break;\n"
         "  case Resource::DESCHEDULE:\n"
         "    %pause_on(res);\n"
         "  case Resource::ILLEGAL:\n"
         "    %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n")
  .setSync();
  fl3r_inout("CRC", "crc32 %0, %1, %2", "%0 = crc32(%0, %1, %2);");
  // TODO check destination registers don't overlap
  fl4r_inout_inout("MACCU", "maccu %0, %3, %1, %2",
       "uint64_t Result = ((uint64_t)%0 << 32 | %3) + (uint64_t)%1 * %2;\n"
       "%0 = (uint32_t)(Result >> 32);\n"
       "%3 = (uint32_t)(Result);");
  // TODO check destination registers don't overlap
  fl4r_inout_inout("MACCS", "maccs %0, %3, %1, %2",
       "uint64_t Result = ((int64_t)%0 << 32 | %3);\n"
       "Result += (int64_t)(int32_t)%1 * (int32_t)%2;\n"
       "%0 = (int32_t)(Result >> 32);\n"
       "%3 = (uint32_t)(Result);");
  // TODO check destination registers don't overlap.
  // Note op0 and op3 are swapped.
  fl4r_out_inout("CRC8", "crc8 %3, %0, %1, %2",
                 "%3 = crc8(%3, (uint8_t)%1, %2);\n"
                 "%0 = %1 >> 8;");
  // TODO check destination registers don't overlap
  // Note op0 and op3 are swapped.
  fl5r("LADD", "ladd %3, %0, %1, %2, %4",
       "uint64_t Result = (uint64_t)%1 + (uint64_t)%2 + (%4&1);\n"
       "%3 = (uint32_t)(Result >> 32);"
       "%0 = (uint32_t)(Result);\n");
  // TODO check destination registers don't overlap
  // Note op0 and op3 are swapped.
  fl5r("LSUB", "lsub %3, %0, %1, %2, %4",
       "uint64_t Result = (uint64_t)%1 - (uint64_t)%2 - (%4&1);\n"
       "%3 = (uint32_t)(Result >> 32);"
       "%0 = (uint32_t)(Result);\n");

  // Five operand long.
  // Note operands are reordered.
  fl5r("LDIVU", "ldivu %0, %3, %4, %1, %2",
       "uint64_t dividend;\n"
       "if (%4 >= %2) {\n"
       "  %exception(ET_ARITHMETIC, 0)\n"
       "}\n"
       "dividend = %1 + ((uint64_t)%4 << 32);\n"
       "%0 = (uint32_t)(dividend / %2);\n"
       "%3 = (uint32_t)(dividend %% %2);\n")
    .setCycles(DIV_CYCLES);
  fl6r("LMUL", "lmul %0, %3, %1, %2, %4, %5",
       "uint64_t Result = (uint64_t)%1 * %2 + %4 + %5;\n"
       "%0 = (int32_t)(Result >> 32);"
       "%3 = (uint32_t)Result;");
  fru6_out("LDAWDP", "ldaw %0, dp[%{dp}1]",
           "%0 = %2 + %1;")
    .addImplicitOp(DP, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_out("LDWDP", "ldw %0, dp[%{dp}1]",
           "uint32_t Addr = %2 + %1;\n"
           "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
           "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
           "  %exception(ET_LOAD_STORE, Addr)\n"
           "}\n"
           "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(DP, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_out("LDWCP", "ldw %0, cp[%{cp}1]",
           "uint32_t Addr = %2 + %1;\n"
           "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
           "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
           "  %exception(ET_LOAD_STORE, Addr)\n"
           "}\n"
           "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(CP, in)
  .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_out("LDWSP", "ldw %0, sp[%1]",
           "uint32_t Addr = %2 + %1;\n"
           "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
           "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
           "  %exception(ET_LOAD_STORE, Addr)\n"
           "}\n"
           "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(SP, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_in("STWDP", "stw %0, dp[%{dp}1]",
          "uint32_t Addr = %2 + %1;\n"
          "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
          "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
          "  %exception(ET_LOAD_STORE, Addr)\n"
          "}\n"
          "%store_word(%0, PhyAddr);\n")
    .addImplicitOp(DP, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_in("STWSP", "stw %0, sp[%1]",
          "uint32_t Addr = %2 + %1;\n"
          "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
          "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
          "  %exception(ET_LOAD_STORE, Addr)\n"
          "}\n"
          "%store_word(%0, PhyAddr);\n")
    .addImplicitOp(SP, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_out("LDAWSP", "ldaw %0, sp[%1]",
           "%0 = %2 + %1;")
    .addImplicitOp(SP, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_out("LDC", "ldc %0, %1", "%0 = %1;");
  fru6_in("BRFT", "bt %0, %1",
          "if (%0) {\n"
          "  %pc = %1;\n"
          "}")
    .transform("%1 = %pc + %1;", "%1 = %1 - %pc;");
  fru6_in("BRFT_illegal", "bt %0, %1",
          "if (%0) {\n"
          "  %exception(ET_ILLEGAL_PC, FROM_PC(%1))\n"
          "}")
    .transform("%1 = %pc + %1;", "%1 = %1 - %pc;");
  fru6_in("BRBT", "bt %0, -%1",
          "if (%0) {\n"
          "  %pc = %1;\n"
          "  %yield"
          "}")
    .transform("%1 = %pc - %1;", "%1 = %pc - %1;");
  fru6_in("BRBT_illegal", "bt %0, -%1",
          "if (%0) {\n"
          "  %exception(ET_ILLEGAL_PC, FROM_PC(%1))\n"
          "}")
    .transform("%1 = %pc - %1;", "%1 = %pc - %1;");
  fru6_in("BRFF", "bt %0, %1",
          "if (!%0) {\n"
          "  %pc = %1;\n"
          "}")
    .transform("%1 = %pc + %1;", "%1 = %1 - %pc;");
  fru6_in("BRFF_illegal", "bt %0, %1",
          "if (!%0) {\n"
          "  %exception(ET_ILLEGAL_PC, FROM_PC(%1))\n"
          "}")
    .transform("%1 = %pc + %1;", "%1 = %1 - %pc;");
  fru6_in("BRBF", "bt %0, -%1",
          "if (!%0) {\n"
          "  %pc = %1;\n"
          "  %yield"
          "}")
    .transform("%1 = %pc - %1;", "%1 = %pc - %1;");
  fru6_in("BRBF_illegal", "bt %0, -%1",
          "if (!%0) {\n"
          "  %exception(ET_ILLEGAL_PC, FROM_PC(%1))\n"
          "}")
    .transform("%1 = %pc - %1;", "%1 = %pc - %1;");
  fru6_in("SETC", "setc res[%0], %1",
       "if (!setC(TIME, ResourceID(%0), %1)) {\n"
       "  %exception(ET_ILLEGAL_RESOURCE, %0)\n"
       "}\n")
    .setSync().setCanEvent();
  fu6("EXTSP", "extsp %0", "%1 = %1 - %0;")
    .addImplicitOp(SP, inout)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("EXTDP", "extdp %0", "%1 = %1 - %0;")
    .addImplicitOp(DP, inout)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("ENTSP", "entsp %0",
      "if (%0 > 0) {\n"
      "  uint32_t Addr = %1;\n"
      "  uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "  if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "    %exception(ET_LOAD_STORE, Addr)\n"
      "  }\n"
      "  %store_word(%2, PhyAddr);\n"
      "  %1 = %1 - %0;\n"
      "}\n")
    .addImplicitOp(SP, inout)
    .addImplicitOp(LR, in)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("RETSP", "retsp %0",
      "uint32_t target;\n"
      "if (%0 > 0) {\n"
      "  uint32_t Addr = %1 + %0;\n"
      "  uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "  if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "    %exception(ET_LOAD_STORE, Addr)\n"
      "  }\n"
      "  %1 = Addr;\n"
      "  %2 = LOAD_WORD(PhyAddr);\n"
      "}\n"
      "target = TO_PC(%2);\n"
      "if (!CHECK_PC(target)) {\n"
      // Documentation doesn't mention this.
      "  %exception(ET_ILLEGAL_PC, %2)\n"
      "}\n"
      "%pc = target;\n"
      "%yield"
      )
    .addImplicitOp(SP, inout)
    .addImplicitOp(LR, inout)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;")
    // retsp always causes an fnop.
    .setCycles(2 * INSTRUCTION_CYCLES);
  fu6("KRESTSP", "krestsp %0",
      "uint32_t Addr = %1 + %0;\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%2 = Addr;\n"
      "%1 = LOAD_WORD(PhyAddr);")
    .addImplicitOp(SP, inout)
    .addImplicitOp(KSP, out)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("KENTSP", "kentsp %0",
      "uint32_t PhyAddr = PHYSICAL_ADDR(%2);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, %2)\n"
      "}\n"
      "%store_word(%1, PhyAddr);\n"
      "%1 = %2 - OP(0);")
    .addImplicitOp(SP, inout)
    .addImplicitOp(KSP, in)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("BRFU", "bu %0", "%pc = %0;")
    .transform("%0 = %pc + %0;", "%0 = %0 - %pc;");
  fu6("BRFU_illegal", "bu %0", "%exception(ET_ILLEGAL_PC, %0)")
    .transform("%0 = %pc + %0;", "%0 = %0 - %pc;");
  fu6("BRBU", "bu -%0", "%pc = %0;\n %yield")
    .transform("%0 = %pc - %0;", "%0 = %pc - %0;");
  fu6("BRBU_illegal", "bu -%0", "%exception(ET_ILLEGAL_PC, %0)")
    .transform("%0 = %pc - %0;", "%0 = %0 - %pc;");  
  fu6("LDAWCP", "ldaw %1, cp[%{cp}0]", "%1 = %2 + %0;")
    .addImplicitOp(R11, out)
    .addImplicitOp(CP, in)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("SETSR", "setsr %0", "%1 = %1 | Thread::sr_t((int)%0);")
    .addImplicitOp(SR, inout)
    .setSync();
  fu6("CLRSR", "clrsr %0", "%1 = %1 & ~Thread::sr_t((int)%0);")
    .addImplicitOp(SR, inout)
    .setSync();
  fu6("BLAT", "blat %0",
      "uint32_t Addr = %2 + (%0<<2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "uint32_t value;\n"
      "uint32_t target;\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)"
      "}\n"
      "value = LOAD_WORD(PhyAddr);\n"
      "if (value & 1) {\n"
      "  %exception(ET_ILLEGAL_PC, value)\n"
      "}\n"
      "target = TO_PC(value);\n"
      "if (!CHECK_PC(target)) {\n"
      "  %exception(ET_ILLEGAL_PC, value)\n"
      "}\n"
      "%1 = FROM_PC(%pc);\n"
      "%pc = target;\n"
      "%yield")
    .addImplicitOp(LR, out)
    .addImplicitOp(R11, in)
    // BLAT always causes an fnop.
    .setCycles(2 * INSTRUCTION_CYCLES);
  fu6("KCALL", "kcall %0", "%kcall(%0)");
  fu6("GETSR", "getsr %1, %0", "%1 = %0 & (uint32_t) %2.to_ulong();")
    .addImplicitOp(R11, out)
    .addImplicitOp(SR, in);
  fu10("LDWCPL", "ldw %1, cp[%{cp}0]",
       "uint32_t Addr = %2 + %0;\n"
       "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
       "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
       "  %exception(ET_LOAD_STORE, Addr)\n"
       "}\n"
       "%1 = LOAD_WORD(PhyAddr);")
    .addImplicitOp(R11, out)
    .addImplicitOp(CP, in)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  // TODO could be optimised to %1 = ram_base + %0
  fu10("LDAPF", "ldap %1, %0", "%1 = FROM_PC(%pc) + %0;")
    .addImplicitOp(R11, out)
    .transform("%0 = %0 << 1;", "%0 = %0 >> 1;");
  fu10("LDAPB", "ldap %1, -%0", "%1 = FROM_PC(%pc) - %0;")
    .addImplicitOp(R11, out)
    .transform("%0 = %0 << 1;", "%0 = %0 >> 1;");
  fu10("BLRF", "bl %0",
       "%1 = FROM_PC(%pc);\n"
       "%pc = %0;")
    .addImplicitOp(LR, out)
    .transform("%0 = %pc + %0;", "%0 = %0 - %pc;");
  fu10("BLRF_illegal", "bl %0", "%exception(ET_ILLEGAL_PC, FROM_PC(%0))")
    .transform("%0 = %pc + %0;", "%0 = %0 - %pc;");
  fu10("BLRB", "bl -%0",
       "%1 = FROM_PC(%pc);\n"
       "%pc = %0;\n"
       "%yield")
    .addImplicitOp(LR, out)
    .transform("%0 = %pc - %0;", "%0 = %pc - %0;");
  fu10("BLRB_illegal", "bl -%0", "%exception(ET_ILLEGAL_PC, FROM_PC(%0))")
    .transform("%0 = %pc - %0;", "%0 = %pc - %0;");
  fu10("BLACP", "bla cp[%{cp}0]", 
      "uint32_t Addr = %2 + (%0<<2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "uint32_t value;\n"
      "uint32_t target;\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)"
      "}\n"
      "value = LOAD_WORD(PhyAddr);\n"
      "if (value & 1) {\n"
      "  %exception(ET_ILLEGAL_PC, value)\n"
      "}\n"
      "target = TO_PC(value);\n"
      "if (!CHECK_PC(target)) {\n"
      "  %exception(ET_ILLEGAL_PC, value)\n"
      "}\n"
      "%1 = FROM_PC(%pc);\n"
      "%pc = target;\n"
      "%yield\n")
    .addImplicitOp(LR, out)
    .addImplicitOp(CP, in)
    // BLACP always causes an fnop.
    .setCycles(2 * INSTRUCTION_CYCLES);
  f2r("NOT", "not %0, %1", "%0 = ~%1;");
  f2r("NEG", "neg %0, %1", "%0 = -%1;");
  frus_inout("SEXT", "sext %0, %1", "%0 = signExtend(%0, %1);");
  f2r_inout("SEXT", "sext %0, %1", "%0 = signExtend(%0, %1);");
  frus_inout("ZEXT", "zext %0, %1", "%0 = zeroExtend(%0, %1);");
  f2r_inout("ZEXT", "zext %0, %1", "%0 = zeroExtend(%0, %1);");
  f2r_inout("ANDNOT", "andnot %0, %1", "%0 = %0 & ~%1;");
  f2r("MKMSK", "mkmsk %0, %1", "%0 = makeMask(%1);");
  frus("MKMSK", "mkmsk %0, %1", "%0 = %1;")
    .transform("%1 = makeMask(%1);", "%1 = maskSize(%1);");
  frus("GETR", "getr %0, %1",
      "if (%1 > (uint32_t)LAST_STD_RES_TYPE)\n"
      "  %0 = 1;\n"
      "else if (Resource *res =\n"
      "           CORE.allocResource(THREAD, (ResourceType)%1))\n"
      "  %0 = res->getID();\n"
      "else\n"
      "  %0 = 0;\n");
  f2r("GETST", "getst %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Synchroniser *sync = checkSync(CORE, resID)) {\n"
      "  if (Thread *t = CORE.allocThread(THREAD)) {\n"
      "    sync->addChild(*t);\n"
      "    t->setSync(*sync);\n"
      "    %0 = t->getID();\n"
      "  } else {\n"
      "    %0 = 0;\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID)\n"
      "}\n");
  f2r("PEEK", "peek %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Port *res = checkPort(CORE, resID)) {\n"
      "  %0 = res->peek(THREAD, TIME);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n")
    .setSync();
  f2r("ENDIN", "endin %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Port *res = checkPort(CORE, resID)) {\n"
      "  uint32_t value;\n"
      "  switch (res->endin(THREAD, TIME, value)) {\n"
      "  default: assert(0 && \"Unexpected endin result\");\n"
      "  case Resource::CONTINUE:\n"
      "    %0 = value;\n"
      "    break;\n"
      "  case Resource::ILLEGAL:\n"
      "    %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n")
    .setSync();
  f2r_in("SETPSC", "setpsc res[%1], %0",
         "ResourceID resID(%1);\n"
         "if (Port *res = checkPort(CORE, resID)) {\n"
         "  switch (res->setpsc(THREAD, %0, TIME)) {\n"
         "  default: assert(0 && \"Unexpected setpsc result\");\n"
         "  case Resource::CONTINUE:\n"
         "    break;\n"
         "  case Resource::ILLEGAL:\n"
         "    %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n")
    .setSync();  
  fl2r("BITREV", "bitrev %0, %1", "%0 = bitReverse(%1);");
  fl2r("BYTEREV", "byterev %0, %1", "%0 = bswap32(%1);");
  fl2r("CLZ", "clz %0, %1", "%0 = countLeadingZeros(%1);");
  fl2r_in("TINITLR", "init t[%1]:lr, %0", 
          "ResourceID resID(%1);\n"
          "Thread *t = checkThread(CORE, resID);\n"
          "if (t && t->inSSync()) {\n"
          "  t->reg(LR) = %0;\n"
          "} else {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
          "}\n");
  fl2r("GETD", "getd %0, res[%1]", "").setUnimplemented();
  fl2r("TESTLCL", "testlcl %0, res[%1]", "").setUnimplemented();
  fl2r_in("SETN", "setn res[%1], %0", "").setUnimplemented();
  fl2r("GETN", "getn %0, res[%1]", "").setUnimplemented();
  fl2r("GETPS", "get %0, ps[%1]",
       "switch (%1) {\n"
       "case PS_RAM_BASE:\n"
       "  %0 = CORE.ram_base;\n"
       "  break;\n"
       "default:\n"
       "  // TODO\n"
       "  ERROR();\n"
       "}\n");
  fl2r_in("SETPS", "set %0, ps[%1]",
          "switch (%1) {\n"
          "case PS_VECTOR_BASE:\n"
          "  CORE.vector_base = %0;\n"
          "  break;\n"
          "default:\n"
          "  // TODO\n"
          "  ERROR();\n"
          "}\n");
  fl2r_in("SETC", "setc res[%0], %1",
          "if (!setC(TIME, ResourceID(%0), %1)) {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, %0);\n"
          "}\n").setSync().setCanEvent();
  fl2r_in("SETCLK", "setclk res[%1], %0",
          "if (!setClock(ResourceID(%1), %0, TIME)) {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, %1);\n"
          "}\n").setSync();
  fl2r_in("SETTW", "settw res[%1], %0",
          "Port *res = checkPort(CORE, ResourceID(%1));\n"
          "if (!res || !res->setTransferWidth(THREAD, %0, TIME)) {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, %1);\n"
          "}\n").setSync();
  fl2r_in("SETRDY", "setrdy res[%1], %0",
          "if (!threadSetReady(ResourceID(%1), %0, TIME)) {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, %1);\n"
          "}\n").setSync();
  f2r("IN", "in %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Resource *res = checkResource(CORE, resID)) {\n"
      "  uint32_t value;\n"
      "  switch(res->in(THREAD, TIME, value)) {\n"
      "  default: assert(0 && \"Unexpected in result\");\n"
      "  case Resource::CONTINUE:\n"
      "    %0 = value;\n"
      "    break;\n"
      "  case Resource::DESCHEDULE:\n"
      "    %pause_on(res);\n"
      "  case Resource::ILLEGAL:\n"
      "    %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n")
    .setSync();
  f2r_in("OUT", "out %0, res[%1]",
         "ResourceID resID(%1);\n"
         "if (Resource *res = checkResource(CORE, resID)) {\n"
         "  switch (res->out(THREAD, %0, TIME)) {\n"
         "  default: assert(0 && \"Unexpected out result\");\n"
         "  case Resource::CONTINUE:\n"
         "    break;\n"
         "  case Resource::DESCHEDULE:\n"
         "    %pause_on(res);\n"
         "  case Resource::ILLEGAL:\n"
         "    %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n")
    .setSync()
    .setCanEvent();
  f2r_in("TINITPC", "init t[%1]:pc, %0",
         "ResourceID resID(%1);\n"
         "Thread *t = checkThread(CORE, resID);\n"
         "if (t && t->inSSync()) {\n"
         "  Thread &threadState = *t;\n"
         "  unsigned newPc = TO_PC(%0);\n"
         "  if (CHECK_PC(newPc)) {;\n"
         "    // Set pc to one previous address since it will be incremented when\n"
         "    // the thread is started with msync.\n"
         "    // TODO is this right?\n"
         "    threadState.pc = newPc - 1;\n"
         "  } else {\n"
         "    threadState.pc = CORE.getIllegalPCThreadAddr();\n"
         "    threadState.illegal_pc = newPc;\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n");
  f2r_in("TINITDP", "init t[%1]:dp, %0",
         "ResourceID resID(%1);\n"
         "Thread *t = checkThread(CORE, resID);\n"
         "if (t && t->inSSync()) {\n"
         "  t->reg(DP) = %0;\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n");
  f2r_in("TINITSP", "init t[%1]:sp, %0",
         "ResourceID resID(%1);\n"
         "Thread *t = checkThread(CORE, resID);\n"
         "if (t && t->inSSync()) {\n"
         "  t->reg(SP) = %0;\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n");
  f2r_in("TINITCP", "init t[%1]:cp, %0",
         "ResourceID resID(%1);\n"
         "Thread *t = checkThread(CORE, resID);\n"
         "if (t && t->inSSync()) {\n"
         "  t->reg(CP) = %0;\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n");
  // TODO wrong format?
  f2r_in("TSETMR", "", "").setCustom();
  
  f2r_in("SETD", "setd res[%1], %0",
         "ResourceID resID(%1);\n"
         "Resource *res = checkResource(CORE, resID);\n"
         "if (!res || !res->setData(THREAD, %0, TIME)) {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "};\n").setSync();
  f2r_in("OUTCT", "outct res[%0], %1",
         "ResourceID resID(%0);\n"
         "if (Chanend *chanend = checkChanend(CORE, resID)) {\n"
         "  switch (chanend->outct(THREAD, %1, TIME)) {\n"
         "  default: assert(0 && \"Unexpected outct result\");\n"
         "  case Resource::CONTINUE:\n"
         "    break;\n"
         "  case Resource::DESCHEDULE:\n"
         "    %pause_on(chanend);\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n")
    .setSync()
    .setCanEvent();
  frus_in("OUTCT", "outct res[%0], %1",
          "ResourceID resID(%0);\n"
          "if (Chanend *chanend = checkChanend(CORE, resID)) {\n"
          "  switch (chanend->outct(THREAD, %1, TIME)) {\n"
          "  default: assert(0 && \"Unexpected outct result\");\n"
          "  case Resource::CONTINUE:\n"
          "    break;\n"
          "  case Resource::DESCHEDULE:\n"
          "    %pause_on(chanend);\n"
          "  }\n"
          "} else {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
          "}\n")
    .setSync()
    .setCanEvent();
  f2r_in("OUTT", "outt res[%1], %0",
         "ResourceID resID(%1);\n"
         "if (Chanend *chanend = checkChanend(CORE, resID)) {\n"
         "  switch (chanend->outt(THREAD, %0, TIME)) {\n"
         "  default: assert(0 && \"Unexpected outct result\");\n"
         "  case Resource::CONTINUE:\n"
         "    break;\n"
         "  case Resource::DESCHEDULE:\n"
         "    %pause_on(chanend);\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n")
    .setSync()
    .setCanEvent();
  f2r("INT", "int %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Chanend *chanend = checkChanend(CORE, resID)) {\n"
      "  uint32_t value;\n"
      "  switch (chanend->intoken(THREAD, TIME, value)) {\n"
      "    default: assert(0 && \"Unexpected int result\");\n"
      "    case Resource::DESCHEDULE:\n"
      "      %pause_on(chanend);\n"
      "    case Resource::ILLEGAL:\n"
      "      %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "    case Resource::CONTINUE:\n"
      "      %0 = value;\n"
      "      break;\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n");
  f2r("INCT", "inct %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Chanend *chanend = checkChanend(CORE, resID)) {\n"
      "  uint32_t value;\n"
      "  switch (chanend->inct(THREAD, TIME, value)) {\n"
      "    default: assert(0 && \"Unexpected int result\");\n"
      "    case Resource::DESCHEDULE:\n"
      "      %pause_on(chanend);\n"
      "    case Resource::ILLEGAL:\n"
      "      %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "      break;\n"
      "    case Resource::CONTINUE:\n"
      "      %0 = value;\n"
      "      break;\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "};\n");
  f2r_in("CHKCT", "chkct res[%0], %1",
         "ResourceID resID(%0);\n"
         "if (Chanend *chanend = checkChanend(CORE, resID)) {\n"
         "  switch (chanend->chkct(THREAD, TIME, %1)) {\n"
         "    default: assert(0 && \"Unexpected chkct result\");\n"
         "    case Resource::DESCHEDULE:\n"
         "      %pause_on(chanend);\n"
         "    case Resource::ILLEGAL:\n"
         "      %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "    case Resource::CONTINUE:\n"
         "      break;\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n");
  frus_in("CHKCT", "chkct res[%0], %1",
          "ResourceID resID(%0);\n"
          "if (Chanend *chanend = checkChanend(CORE, resID)) {\n"
          "  switch (chanend->chkct(THREAD, TIME, %1)) {\n"
          "    default: assert(0 && \"Unexpected chkct result\");\n"
          "    case Resource::DESCHEDULE:\n"
          "      %pause_on(chanend);\n"
          "    case Resource::ILLEGAL:\n"
          "      %exception(ET_ILLEGAL_RESOURCE, resID);\n"
          "    case Resource::CONTINUE:\n"
          "      break;\n"
          "  }\n"
          "} else {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
          "}\n");
  f2r("TESTCT", "testct %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Chanend *chanend = checkChanend(CORE, resID)) {\n"
      "  bool isCt;\n"
      "  if (chanend->testct(THREAD, TIME, isCt)) {\n"
      "    %0 = isCt;\n"
      "  } else {\n"
      "    %pause_on(chanend);\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n");
  f2r("TESTWCT", "testwct %0, res[%0]",
      "ResourceID resID(%1);\n"
      "if (Chanend *chanend = checkChanend(CORE, resID)) {\n"
      "  uint32_t value;\n"
      "  if (chanend->testwct(THREAD, TIME, value)) {\n"
      "    %0 = value;\n"
      "  } else {\n"
      "    %pause_on(chanend);\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n");
  f2r_in("EET", "eet res[%1], %0",
         "ResourceID resID(%1);\n"
         "if (EventableResource *res = checkEventableResource(CORE, resID)) {\n"
         "  if (%0 != 0) {\n"
         "    res->eventEnable(THREAD);\n"
         "  } else {\n"
         "    res->eventDisable(THREAD);\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n")
    .setSync()
    .setCanEvent();
  f2r_in("EEF", "eef res[%1], %0",
         "ResourceID resID(%1);\n"
         "if (EventableResource *res = checkEventableResource(CORE, resID)) {\n"
         "  if (%0 == 0) {\n"
         "    res->eventEnable(THREAD);\n"
         "  } else {\n"
         "    res->eventDisable(THREAD);\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n")
    .setSync()
    .setCanEvent();
  f2r_inout("INSHR", "inshr %0, res[%1]",
            "ResourceID resID(%1);\n"
            "if (Port *res = checkPort(CORE, resID)) {\n"
            "  uint32_t value;\n"
            "  Resource::ResOpResult result = res->in(THREAD, TIME, value);\n"
            "  switch (result) {\n"
            "  default: assert(0 && \"Unexpected in result\");\n"
            "  case Resource::CONTINUE:\n"
            "    {\n"
            "      unsigned width = res->getTransferWidth();\n"
            "      %0 = (%0 >> width) | (value << (32 - width));\n"
            "      break;\n"
            "    }\n"
            "  case Resource::DESCHEDULE:\n"
            "    %pause_on(res);\n"
            "  }\n"
            "} else {\n"
            "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
            "}\n")
    .setSync();
  f2r_inout("OUTSHR", "outshr %0, res[%1]",
            "ResourceID resID(%1);\n"
            "if (Port *res = checkPort(CORE, resID)) {\n"
            "  Resource::ResOpResult result = res->out(THREAD, %0, TIME);\n"
            "  switch (result) {\n"
            "  default: assert(0 && \"Unexpected outshr result\");\n"
            "  case Resource::CONTINUE:\n"
            "    %0 = %0 >> res->getTransferWidth();\n"
            "    break;\n"
            "  case Resource::DESCHEDULE:\n"
            "    %pause_on(res);\n"
            "  }\n"
            "} else {\n"
            "  %exception(ET_ILLEGAL_RESOURCE, %1);\n"
            "}\n")
    .setSync();
  f2r("GETTS", "getts %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Port *res = checkPort(CORE, resID)) {\n"
      "  %0 = res->getTimestamp(THREAD, TIME);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, %1);\n"
      "}\n").setSync();
  f2r_in("SETPT", "setpt res[%1], %0",
         "ResourceID resID(%1);\n"
         "if (Port *res = checkPort(CORE, resID)) {\n"
         "  switch (res->setPortTime(THREAD, %0, TIME)) {\n"
         "  default: assert(0 && \"Unexpected setPortTime result\");\n"
         "  case Resource::CONTINUE:\n"
         "    break;\n"
         "  case Resource::DESCHEDULE:\n"
         "    %pause_on(res);\n"
         "    break;\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, REG(OP(1)));\n"
         "}\n").setSync();

  f1r("SETSP", "set sp, %0", "%1 = %0;")
    .addImplicitOp(SP, out);
  // TODO should we check the pc range?
  f1r("SETDP", "set dp, %0", "%1 = %0;")
    .addImplicitOp(DP, out);
  // TODO should we check the pc range?
  f1r("SETCP", "set cp, %0", "%1 = %0;")
    .addImplicitOp(CP, out);
  f1r("ECALLT", "ecallt %0",
      "if (%0) {\n"
      "  %exception(ET_ECALL, 0)"
      "}");
  f1r("ECALLF", "ecallf %0",
      "if (!%0) {\n"
      "  %exception(ET_ECALL, 0)"
      "}");
  f1r("BAU", "bau %0",
      "uint32_t target;\n"
      "if (%0 & 1) {\n"
      "  %exception(ET_ILLEGAL_PC, %0)\n"
      "}\n"
      "target = TO_PC(%0);\n"
      "if (!CHECK_PC(target)) {\n"
      "  %exception(ET_ILLEGAL_PC, %0)\n"
      "}\n"
      "%pc = target;\n"
      "%yield\n");
  f1r("BLA", "bla %0",
      "uint32_t target;\n"
      "if (%0 & 1) {\n"
      "  %exception(ET_ILLEGAL_PC, %0)\n"
      "}\n"
      "target = TO_PC(%0);\n"
      "if (!CHECK_PC(target)) {\n"
      "  %exception(ET_ILLEGAL_PC, %0)\n"
      "}\n"
      "%1 = FROM_PC(%pc);\n"
      "%pc = target;\n"
      "%yield\n")
    .addImplicitOp(LR, out);
  f1r("BRU", "bru %0",
      "uint32_t target = %pc + %0;\n"
      "if (!CHECK_PC(target)) {\n"
      "  %exception(ET_ILLEGAL_PC, FROM_PC(target))\n"
      "}\n"
      "%pc = target;\n"
      "%yield\n");
  f1r("TSTART", "start t[%0]",
      "ResourceID resID(%0);\n"
      "Thread *t = checkThread(CORE, resID);\n"
      "if (t && t->inSSync() && !t->getSync()) {\n"
      "  t->setSSync(false);\n"
      "  t->pc++;"
      "  t->schedule();"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n");
  f1r_out("DGETREG", "dgetreg %0", "").setUnimplemented();
  f1r("KCALL",  "kcall %0", "%kcall(%0)");
  f1r("FREER", "freer res[%0]",
      "Resource *res = checkResource(CORE, ResourceID(%0));\n"
      "if (!res || !res->free()) {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, %0);\n"
      "}\n");
  f1r("MSYNC", "msync res[%0]",
      "ResourceID resID(%0);\n"
      "if (Synchroniser *sync = checkSync(CORE, resID)) {\n"
      "  switch (sync->msync(THREAD)) {\n"
      "  default: assert(0 && \"Unexpected sync result\");\n"
      "  case Synchroniser::SYNC_CONTINUE:\n"
      "    break;\n"
      "  case Synchroniser::SYNC_DESCHEDULE:\n"
      "    %pause_on(sync);\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n");
  f1r("MJOIN", "mjoin res[%0]",
      "ResourceID resID(%0);\n"
      "if (Synchroniser *sync = checkSync(CORE, resID)) {\n"
      "  switch (sync->mjoin(THREAD)) {\n"
      "    default: assert(0 && \"Unexpected mjoin result\");\n"
      "    case Synchroniser::SYNC_CONTINUE:\n"
      "      break;\n"
      "    case Synchroniser::SYNC_DESCHEDULE:\n"
      "      %pause_on(sync);\n"
      "      break;\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n");
  f1r("SETV", "setv res[%0], %1",
      "ResourceID resID(%0);\n"
      "if (EventableResource *res = checkEventableResource(CORE, resID)) {\n"
      "  uint32_t target = TO_PC(%1);\n"
      "  if (CHECK_PC(target)) {\n"
      "    res->setVector(THREAD, target);\n"
      "  } else {\n"
      "    // TODO\n"
      "    ERROR();\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").addImplicitOp(R11, in).setSync();
  f1r("SETEV", "setev res[%0], %1",
      "ResourceID resID(%0);\n"
      "if (EventableResource *res = checkEventableResource(CORE, resID)) {\n"
      "  res->setEV(THREAD, %1);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").addImplicitOp(R11, in).setSync();
  f1r("EDU", "edu res[%0]",
      "ResourceID resID(%0);\n"
      "if (EventableResource *res = checkEventableResource(CORE, resID)) {\n"
      "  res->eventDisable(THREAD);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").setSync();
  f1r("EEU", "eeu res[%0]",
      "ResourceID resID(%0);\n"
      "if (EventableResource *res = checkEventableResource(CORE, resID)) {\n"
      "  res->eventEnable(THREAD);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").setSync().setCanEvent();
  f1r("WAITET", "waitet %0",
      "if (%0) {\n"
      "  THREAD.enableEvents();\n"
      "  %deschedule\n"
      "}\n").setSync().setCanEvent();
  f1r("WAITEF", "waitef %0",
      "if (!%0) {\n"
      "  THREAD.enableEvents();\n"
      "  %deschedule\n"
      "}\n").setSync().setCanEvent();
  f1r("SYNCR", "syncr res[%0]",
      "ResourceID resID(%0);\n"
      "if (Port *port = checkPort(CORE, resID)) {\n"
      "  switch (port->sync(THREAD, TIME)) {\n"
      "  default: assert(0 && \"Unexpected syncr result\");\n"
      "  case Resource::CONTINUE: PC++; break;\n"
      "  case Resource::DESCHEDULE:\n"
      "    %pause_on(port);\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n"
      ).setSync();
  f1r("CLRPT", "clrpt res[%0]",
      "ResourceID resID(%0);\n"
      "if (Port *res = checkPort(CORE, resID)) {\n"
      "  res->clearPortTime(THREAD, TIME);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").setSync();
  f0r("GETID", "get %0, id", "%0 = THREAD.getNum();")
    .addImplicitOp(R11, out);
  f0r("GETET", "get %0, %1", "%0 = %1;")
    .addImplicitOp(R11, out)
    .addImplicitOp(ET, in);
  f0r("GETED", "get %0, %1", "%0 = %1;")
    .addImplicitOp(R11, out)
    .addImplicitOp(ED, in);
  f0r("GETKEP", "get %0, %1", "%0 = %1;")
    .addImplicitOp(R11, out)
    .addImplicitOp(KEP, in);
  f0r("GETKSP", "get %0, %1", "%0 = %1;")
    .addImplicitOp(R11, out)
    .addImplicitOp(KSP, in);
  // KEP is 128-byte aligned, the lower 7 bits are set to 0.
  f0r("SETKEP", "set %0, %1", "%0 = %1 & ~((1 << 7) - 1);")
    .addImplicitOp(KEP, out)
    .addImplicitOp(R11, in);
  // TODO handle illegal spc
  f0r("KRET", "kret",
      "%pc = TO_PC(%0);\n"
      "%3 = %1;\n"
      "Thread::sr_t value((int)%2);\n"
      "value[Thread::WAITING] = false;\n"
      "%4 = value;")
    .addImplicitOp(SPC, in)
    .addImplicitOp(SED, in)
    .addImplicitOp(SSR, in)
    .addImplicitOp(ED, out)
    .addImplicitOp(SR, out)
    .setSync();
  f0r("DRESTSP", "drestsp", "").setUnimplemented();
  f0r("LDSPC", "ldw %0, sp[1]", 
      "uint32_t Addr = %1 + (1 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(SPC, out)
    .addImplicitOp(SP, in);
  f0r("LDSSR", "ldw %0, sp[2]", 
      "uint32_t Addr = %1 + (2 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(SSR, out)
    .addImplicitOp(SP, in);
  f0r("LDSED", "ldw %0, sp[3]", 
      "uint32_t Addr = %1 + (3 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(SED, out)
    .addImplicitOp(SP, in);
  f0r("LDET", "ldw %0, sp[4]", 
      "uint32_t Addr = %1 + (4 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(ET, out)
    .addImplicitOp(SP, in);
  f0r("STSPC", "stw %0, sp[1]", 
      "uint32_t Addr = %1 + (1 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%store_word(%0, PhyAddr);\n")
    .addImplicitOp(SPC, in)
    .addImplicitOp(SP, in);
  f0r("STSSR", "stw %0, sp[2]", 
      "uint32_t Addr = %1 + (2 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%store_word(%0, PhyAddr);\n")
    .addImplicitOp(SSR, in)
    .addImplicitOp(SP, in);
  f0r("STSED", "stw %0, sp[3]", 
      "uint32_t Addr = %1 + (3 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%store_word(%0, PhyAddr);\n")
    .addImplicitOp(SED, in)
    .addImplicitOp(SP, in);
  f0r("STET", "stw %0, sp[4]", 
      "uint32_t Addr = %1 + (4 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%store_word(%0, PhyAddr);\n")
    .addImplicitOp(ET, in)
    .addImplicitOp(SP, in);
  f0r("FREET", "freet", "").setCustom();
  f0r("DCALL", "dcall", "").setUnimplemented();
  f0r("DRET", "dret", "").setUnimplemented();
  f0r("DENTSP", "dentsp", "").setUnimplemented();
  f0r("CLRE", "clre", "THREAD.clre();\n").setSync();
  f0r("WAITEU", "waiteu",
      "THREAD.enableEvents();\n"
      "%deschedule\n").setSync().setCanEvent();
  f0r("SSYNC", "", "").setCustom();

  pseudoInst("ILLEGAL_PC", "", "").setCustom();
  pseudoInst("ILLEGAL_PC_THREAD", "", "").setCustom();
  pseudoInst("ILLEGAL_INSTRUCTION", "", "").setCustom();
  pseudoInst("DECODE", "", "").setCustom();
  pseudoInst("SYSCALL", "", "").setCustom();
  pseudoInst("EXCEPTION", "", "").setCustom();
  pseudoInst("INITIALIZE", "", "").setCustom();
  pseudoInst("JIT_INSTRUCTION", "", "").setCustom();
}

int main()
{
  add();
  analyze();
  emitInstFunctions();
  emitInstDispatch();
  emitInstList();
  emitInstProperties();
}
