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
  r0,
  r1,
  r2,
  r3,
  r4,
  r5,
  r6,
  r7,
  r8,
  r9,
  r10,
  r11,
  cp,
  dp,
  sp,
  lr,
  ed,
  et,
  kep,
  ksp,
  sr,
  spc,
  sed,
  ssr
};

const char *getRegisterName(Register reg) {
  switch (reg) {
  default:
    assert(0 && "Unexpected register");
    return "?";
  case r0: return "R0";
  case r1: return "R1";
  case r2: return "R2";
  case r3: return "R3";
  case r4: return "R4";
  case r5: return "R5";
  case r6: return "R6";
  case r7: return "R7";
  case r8: return "R8";
  case r9: return "R9";
  case r10: return "R10";
  case r11: return "R11";
  case cp: return "CP";
  case dp: return "DP";
  case sp: return "SP";
  case lr: return "LR";
  case et: return "ET";
  case ed: return "ED";
  case kep: return "KEP";
  case ksp: return "KSP";
  case spc: return "SPC";
  case sed: return "SED";
  case ssr: return "SSR";
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
  bool sync;
  bool canEvent;
  bool unimplemented;
  bool custom;
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
    custom(false)
  {
  }
  const std::string &getName() const { return name; }
  unsigned getSize() const { return size; }
  const std::vector<OpType> &getOperands() const { return operands; }
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
  unsigned indentLevel = 1;
  while (*s != '\0') {
  switch (*s) {
    case '(':
      indentLevel++;
      break;
    case ')':
      indentLevel--;
      if (indentLevel == 0)
        return s;
      break;
    case '\'':
    case '"':
      std::cerr << "error: ' and \" are not yet handled\n";
      std::exit(1);
    }
    ++s;
  }
  std::cerr << "error: no closing bracket found\n";
  std::exit(1);
}

std::string getEndLabel(const Instruction &instruction)
{
  return instruction.getName() + "_end";
}

static std::string
getOperandName(const Instruction &instruction, unsigned i)
{
  std::ostringstream buf;
  
  const std::vector<OpType> &ops = instruction.getOperands();
  const std::vector<Register> &implicitOps = instruction.getImplicitOps();
  unsigned numExplicitOperands = ops.size() - implicitOps.size();
  if (i >= numExplicitOperands) {
    if (i >= ops.size()) {
      std::cerr << "error: operand out of range\n";
      std::exit(1);
    }
    assert(ops[i] != imm);
    buf << getRegisterName(implicitOps[i - numExplicitOperands]);
  } else {
    const char *opMacro = ops.size() > 3 ? "LOP" : "OP";
    buf << opMacro << '(' << i << ')';
  }
  return buf.str();
}

static bool
isSR(const Instruction &instruction, unsigned i)
{
  const std::vector<OpType> &ops = instruction.getOperands();
  const std::vector<Register> &implicitOps = instruction.getImplicitOps();
  unsigned numExplicitOperands = ops.size() - implicitOps.size();
  
  if (i < numExplicitOperands)
    return false;
  if (i >= ops.size()) {
    std::cerr << "error: operand out of range\n";
    std::exit(1);
  }
  assert(ops[i] != imm);
  return implicitOps[i - numExplicitOperands] == sr;
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

static void
emitCode(const Instruction &instruction,
         const std::string &code,
         bool &emitEndLabel)
{
  const std::vector<OpType> operands = instruction.getOperands();
  const char *s = code.c_str();
  for (unsigned i = 0, e = code.size(); i != e; i++) {
    if (s[i] == '%') {
      if (i + 1 == e) {
        std::cerr << "error: % at end of code string\n";
        std::exit(1);
      }
      i++;
      if (s[i] == '%') {
        std::cout << '%';
      } else if (isdigit(s[i])) {
        char *endp;
        long value = std::strtol(&s[i], &endp, 10);
        i = (endp - s) - 1;
        if (value < 0 || (unsigned)value >= operands.size()) {
          std::cerr << "error: operand out of range in code string\n";
          std::exit(1);
        }
        std::cout << "op" << value;
      } else if (std::strncmp(&s[i], "pc", 2) == 0) {
        std::cout << "nextPc";
        i++;
      } else if (std::strncmp(&s[i], "exception(", 10) == 0) {
        i += 10;
        emitCycles(instruction);
        emitTraceEnd();
        std::cout << "EXCEPTION(";
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitCode(instruction, content);
        std::cout << ");\n";
        std::cout << "goto " << getEndLabel(instruction) << ";\n";
        emitEndLabel = true;
        i = (close - s);
      } else if (std::strncmp(&s[i], "pause_on(", 9) == 0) {
        i += 9;
        emitCycles(instruction);
        emitTraceEnd();
        std::cout << "PAUSE_ON(PC, ";
        const char *close = scanClosingBracket(&s[i]);
        std::string content(&s[i], close);
        emitCode(instruction, content);
        std::cout << ");\n";
        std::cout << "goto " << getEndLabel(instruction) << ";\n";
        emitEndLabel = true;
        i = (close - s);
      } else if (std::strncmp(&s[i], "next", 4) == 0) {
        i += 3;
        emitCycles(instruction);
        emitRegWriteBack(instruction);
        emitCheckEvents(instruction);
        emitTraceEnd();
        std::cout << "NEXT_THREAD(PC);\n";
        std::cout << "goto " << getEndLabel(instruction) << ";\n";
        emitEndLabel = true;
      } else if (std::strncmp(&s[i], "deschedule", 10) == 0) {
        i += 10;
        emitCycles(instruction);
        emitCheckEventsOrDeschedule(instruction);
        emitTraceEnd();
        std::cout << "goto " << getEndLabel(instruction) << ";\n";
        emitEndLabel = true;
      } else {
        std::cerr << "error: stray % in code string\n";
        std::exit(1);
      }
    } else {
      std::cout << s[i];
    }
  }
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

static void
emitInstDispatch(Instruction &instruction)
{
  if (instruction.getCustom())
    return;
  const std::string &name = instruction.getName();
  unsigned size = instruction.getSize();
  const std::vector<OpType> &operands = instruction.getOperands();
  const std::string &code = instruction.getCode();

  if (size % 2 != 0) {
    std::cerr << "error: unexpected instruction size " << size << "\n";
    std::exit(1);
  }
  std::cout << "INST(" << name << "):";
  if (instruction.getSync()) {
    std::cout << "\n"
                 "if (sys.hasTimeSliceExpired(TIME)) {\n"
                 "  NEXT_THREAD(PC);\n"
                 "} else";
  }
  std::cout << " {\n";
  // Read operands.
  for (unsigned i = 0, e = operands.size(); i != e; ++i) {
    std::cout << "UNUSED(";
    if (operands[i] == in)
      std::cout << "const ";
    if (isSR(instruction, i)) {
      std::cout << "ThreadState::sr_t op" << i << ") = thread->sr";
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
           "STORE_WORD(%0, PhyAddr);\n")
    .transform("%2 = %2 << 2;", "%2 = %2 >> 2;");
  // TSETR needs special handling as one operands is not a register on the
  // current thread.
  inst("TSETR_3r", 2, ops(imm, in, in), "set t[%2]:r%0, %1",
       "ResourceID resID(%2);\n"
       "if (Thread *t = checkThread(*thread, resID)) {\n"
       "  t->getState().reg(%0) = %1;\n"
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
          "STORE_WORD(%0, PhyAddr);\n");
  fl3r_in("ST16", "st16 %0, %1[%2]",
          "uint32_t Addr = %1 + (%2 << 1);\n"
          "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
          "if (!CHECK_ADDR_SHORT(PhyAddr)) {\n"
          "  %exception(ET_LOAD_STORE, Addr)"
          "}\n"
          "STORE_SHORT(%0, PhyAddr);\n");
  fl3r_in("ST8", "st8 %0, %1[%2]",
          "uint32_t Addr = %1 + %2;\n"
          "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
          "if (!CHECK_ADDR(PhyAddr)) {\n"
          "  %exception(ET_LOAD_STORE, Addr)"
          "}\n"
          "STORE_BYTE(%0, PhyAddr);\n");
  fl3r("MUL", "mul %0, %1, %2", "%0 = %1 * %2;");
  fl3r("DIVS", "divs %0, %1, %2",
       "if (%2 == 0 ||\n"
       "    (%1 == 0x7fffffff && %2 == 0xffffffff)) {\n"
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
       "    (%1 == 0x7fffffff && %2 == 0xffffffff)) {\n"
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
  fl2rus_in("OUTPW", "outpw res[%1], %0, %1", "").setUnimplemented();
  fl2rus("INPW", "inpw %0, res[%1], %2", "").setUnimplemented();
  fl3r_inout("CRC", "crc32 %0, %1, %2", "%0 = crc32(%0, %1, %2);");
  // TODO check destination registers don't overlap
  fl4r_inout_inout("MACCU", "maccu %0, %3, %1, %2",
       "uint64_t Result = ((uint64_t)%0 << 32 | %3) + (uint64_t)%1 * %2;\n"
       "%0 = (uint32_t)(Result >> 32);\n"
       "%3 = (uint32_t)(Result);");
  // TODO check destination registers don't overlap
  fl4r_inout_inout("MACCS", "maccs %0, %3, %1, %2",
       "uint64_t Result = ((int64_t)%0 << 32 | %3) + (int64_t)(int32_t)%1 * %2;\n"
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
       "if (%2 == 0 || (%4 >= %2)) {\n"
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
    .addImplicitOp(dp, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_out("LDWDP", "ldw %0, dp[%{dp}1]",
           "uint32_t Addr = %2 + %1;\n"
           "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
           "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
           "  %exception(ET_LOAD_STORE, Addr)\n"
           "}\n"
           "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(dp, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_out("LDWCP", "ldw %0, cp[%{cp}1]",
           "uint32_t Addr = %2 + %1;\n"
           "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
           "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
           "  %exception(ET_LOAD_STORE, Addr)\n"
           "}\n"
           "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(cp, in)
  .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_out("LDWSP", "ldw %0, sp[%1]",
           "uint32_t Addr = %2 + %1;\n"
           "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
           "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
           "  %exception(ET_LOAD_STORE, Addr)\n"
           "}\n"
           "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(sp, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_in("STWDP", "stw %0, dp[%{dp}1]",
          "uint32_t Addr = %2 + %1;\n"
          "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
          "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
          "  %exception(ET_LOAD_STORE, Addr)\n"
          "}\n"
          "STORE_WORD(%0, PhyAddr);\n")
    .addImplicitOp(dp, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_in("STWSP", "stw %0, sp[%1]",
          "uint32_t Addr = %2 + %1;\n"
          "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
          "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
          "  %exception(ET_LOAD_STORE, Addr)\n"
          "}\n"
          "STORE_WORD(%0, PhyAddr);\n")
    .addImplicitOp(sp, in)
    .transform("%1 = %1 << 2;", "%1 = %1 >> 2;");
  fru6_out("LDAWSP", "ldaw %0, sp[%1]",
           "%0 = %2 + %1;")
    .addImplicitOp(sp, in)
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
          "  %next"
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
          "  %next"
          "}")
    .transform("%1 = %pc - %1;", "%1 = %pc - %1;");
  fru6_in("BRBF_illegal", "bt %0, -%1",
          "if (!%0) {\n"
          "  %exception(ET_ILLEGAL_PC, FROM_PC(%1))\n"
          "}")
    .transform("%1 = %pc - %1;", "%1 = %pc - %1;");
  fru6_in("SETC", "setc res[%0], %1",
       "if (!setC(*thread, TIME, ResourceID(%0), %1)) {\n"
       "  %exception(ET_ILLEGAL_RESOURCE, %0)\n"
       "}\n")
    .setSync().setCanEvent();
  fu6("EXTSP", "extsp %0", "%1 = %1 - %0;")
    .addImplicitOp(sp, inout)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("EXTDP", "extdp %0", "%1 = %1 - %0;")
    .addImplicitOp(dp, inout)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("ENTSP", "entsp %0",
      "if (%0 > 0) {\n"
      "  uint32_t Addr = %1;\n"
      "  uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "  if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "    %exception(ET_LOAD_STORE, Addr)\n"
      "  }\n"
      "  STORE_WORD(%2, PhyAddr);\n"
      "  %1 = %1 - %0;\n"
      "}\n")
    .addImplicitOp(sp, inout)
    .addImplicitOp(lr, in)
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
      "%next"
      )
    .addImplicitOp(sp, inout)
    .addImplicitOp(lr, inout)
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
    .addImplicitOp(sp, inout)
    .addImplicitOp(ksp, out)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("KENTSP", "kentsp %0",
      "uint32_t PhyAddr = PHYSICAL_ADDR(%2);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, %2)\n"
      "}\n"
      "STORE_WORD(PhyAddr, %1);\n"
      "%1 = %2 - OP(0);")
    .addImplicitOp(sp, inout)
    .addImplicitOp(ksp, in)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("BRFU", "bu %0", "%pc = %0;")
    .transform("%0 = %pc + %0;", "%0 = %0 - %pc;");
  fu6("BRFU_illegal", "bu %0", "%exception(ET_ILLEGAL_PC, %0)")
    .transform("%0 = %pc + %0;", "%0 = %0 - %pc;");
  fu6("BRBU", "bu -%0", "%pc = %0;\n %next")
    .transform("%0 = %pc - %0;", "%0 = %pc - %0;");
  fu6("BRBU_illegal", "bu -%0", "%exception(ET_ILLEGAL_PC, %0)")
    .transform("%0 = %pc - %0;", "%0 = %0 - %pc;");  
  fu6("LDAWCP", "ldaw %1, cp[%{cp}0]", "%1 = %2 + %0;")
    .addImplicitOp(r11, out)
    .addImplicitOp(cp, in)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  fu6("SETSR", "setsr %0", "%1 = %1 | ThreadState::sr_t((int)%0);")
    .addImplicitOp(sr, inout)
    .setSync();
  fu6("CLRSR", "clrsr %0", "%1 = %1 & ~ThreadState::sr_t((int)%0);")
    .addImplicitOp(sr, inout)
    .setSync();
  fu6("BLAT", "blat %0", "").addImplicitOp(r11, in).setUnimplemented();
  fu6("KCALL", "kcall %0", "").setUnimplemented();
  fu6("GETSR", "getsr %1, %0", "%1 = %0 & (int) %2.to_ulong();")
    .addImplicitOp(r11, out)
    .addImplicitOp(sr, in);
  fu10("LDWCPL", "ldw %1, cp[%{cp}0]",
       "uint32_t Addr = %2 + %0;\n"
       "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
       "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
       "  %exception(ET_LOAD_STORE, Addr)\n"
       "}\n"
       "%1 = LOAD_WORD(PhyAddr);")
    .addImplicitOp(r11, out)
    .addImplicitOp(cp, in)
    .transform("%0 = %0 << 2;", "%0 = %0 >> 2;");
  // TODO could be optimised to %1 = ram_base + %0
  fu10("LDAPF", "ldap %1, %0", "%1 = FROM_PC(%pc) + %0;")
    .addImplicitOp(r11, out)
    .transform("%0 = %0 << 1;", "%0 = %0 >> 1;");
  fu10("LDAPB", "ldap %1, -%0", "%1 = FROM_PC(%pc) - %0;")
    .addImplicitOp(r11, out)
    .transform("%0 = %0 << 1;", "%0 = %0 >> 1;");
  fu10("BLRF", "bl %0",
       "%1 = FROM_PC(%pc);\n"
       "%pc = %0;")
    .addImplicitOp(lr, out)
    .transform("%0 = %pc + %0;", "%0 = %0 - %pc;");
  fu10("BLRF_illegal", "bl %0", "%exception(ET_ILLEGAL_PC, FROM_PC(%0))")
    .transform("%0 = %pc + %0;", "%0 = %0 - %pc;");
  fu10("BLRB", "bl -%0",
       "%1 = FROM_PC(%pc);\n"
       "%pc = %0;\n"
       "%next")
    .addImplicitOp(lr, out)
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
      "%next\n")
    .addImplicitOp(lr, out)
    .addImplicitOp(cp, in);
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
      "           core->allocResource(*thread, (ResourceType)IMM(OP(1))))\n"
      "  %0 = res->getID();\n"
      "else\n"
      "  %0 = 0;\n");
  f2r("GETST", "getst %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Synchroniser *sync = checkSync(*thread, resID)) {\n"
      "  if (Thread *t = core->allocThread(*thread)) {\n"
      "    sync->addChild(t->getState());\n"
      "    t->getState().setSync(*sync);\n"
      "    %0 = t->getID();\n"
      "  } else {\n"
      "    %0 = 0;\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID)\n"
      "}\n");
  f2r("PEEK", "peek %0, res[%1]", "").setUnimplemented();
  f2r("ENDIN", "endin %0, res[%1]", "").setUnimplemented();
  f2r_in("SETPSC", "setpsc res[%0], %1", "").setUnimplemented();  
  fl2r("BITREV", "bitrev %0, %1", "%0 = bitReverse(%1);");
  fl2r("BYTEREV", "byterev %0, %1", "%0 = bswap32(%1);");
  fl2r("CLZ", "clz %0, %1", "%0 = countLeadingZeros(%1);");
  fl2r_in("TINITLR", "init t[%1]:lr, %0", 
          "ResourceID resID(%1);\n"
          "Thread *t = checkThread(*thread, resID);\n"
          "if (t && t->getState().inSSync()) {\n"
          "  t->getState().reg(LR) = %0;\n"
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
       "  %0 = core->ram_base;\n"
       "  break;\n"
       "default:\n"
       "  // TODO\n"
       "  ERROR();\n"
       "}\n");
  fl2r_in("SETPS", "set %0, ps[%1]",
          "switch (%1) {\n"
          "case PS_VECTOR_BASE:\n"
          "  core->vector_base = %0;\n"
          "  break;\n"
          "default:\n"
          "  // TODO\n"
          "  ERROR();\n"
          "}\n");
  fl2r_in("SETC", "setc res[%0], %1",
          "if (!setC(*thread, TIME, ResourceID(%0), %1)) {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, %0);\n"
          "}\n").setSync().setCanEvent();
  fl2r_in("SETCLK", "setclk res[%1], %0",
          "if (!setClock(*thread, ResourceID(%1), %0, TIME)) {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, %1);\n"
          "}\n").setSync();
  fl2r_in("SETTW", "settw res[%1], %0",
          "Port *res = checkPort(*thread, ResourceID(%1));\n"
          "if (!res || !res->setTransferWidth(*thread, %0, TIME)) {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, %1);\n"
          "}\n").setSync();
  fl2r_in("SETRDY", "setrdy res[%1], %0",
          "if (!setReady(*thread, ResourceID(%1), %0, TIME)) {\n"
          "  %exception(ET_ILLEGAL_RESOURCE, %1);\n"
          "}\n").setSync();
  f2r("IN", "in %0, res[%1]",
      "ResourceID resID(%1);\n"
      "if (Resource *res = checkResource(*thread, resID)) {\n"
      "  uint32_t value;\n"
      "  switch(res->in(*thread, TIME, value)) {\n"
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
         "if (Resource *res = checkResource(*thread, resID)) {\n"
         "  switch (res->out(*thread, %0, TIME)) {\n"
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
         "Thread *t = checkThread(*thread, resID);\n"
         "if (t && t->getState().inSSync()) {\n"
         "  ThreadState &threadState = t->getState();\n"
         "  unsigned newPc = TO_PC(%0);\n"
         "  if (CHECK_PC(newPc)) {;\n"
         "    // Set pc to one previous address since it will be incremented when\n"
         "    // the thread is started with msync.\n"
         "    // TODO is this right?\n"
         "    threadState.pc = newPc - 1;\n"
         "  } else {\n"
         "    threadState.pc = core->getIllegalPCThreadAddr();\n"
         "    threadState.illegal_pc = newPc;\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n");
  f2r_in("TINITDP", "init t[%1]:dp, %0",
         "ResourceID resID(%1);\n"
         "Thread *t = checkThread(*thread, resID);\n"
         "if (t && t->getState().inSSync()) {\n"
         "  t->getState().reg(DP) = %0;\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n");
  f2r_in("TINITSP", "init t[%1]:sp, %0",
         "ResourceID resID(%1);\n"
         "Thread *t = checkThread(*thread, resID);\n"
         "if (t && t->getState().inSSync()) {\n"
         "  t->getState().reg(SP) = %0;\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n");
  f2r_in("TINITCP", "init t[%1]:cp, %0",
         "ResourceID resID(%1);\n"
         "Thread *t = checkThread(*thread, resID);\n"
         "if (t && t->getState().inSSync()) {\n"
         "  t->getState().reg(CP) = %0;\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n");
  // TODO wrong format?
  f2r_in("TSETMR", "", "").setCustom();
  
  f2r_in("SETD", "setd res[%1], %0",
         "ResourceID resID(%1);\n"
         "Resource *res = checkResource(*thread, resID);\n"
         "if (!res || !res->setData(*thread, %0, TIME)) {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "};\n").setSync();
  f2r_in("OUTCT", "outct res[%0], %1",
         "ResourceID resID(%0);\n"
         "if (Chanend *chanend = checkChanend(*thread, resID)) {\n"
         "  switch (chanend->outct(*thread, %1, TIME)) {\n"
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
          "if (Chanend *chanend = checkChanend(*thread, resID)) {\n"
          "  switch (chanend->outct(*thread, %1, TIME)) {\n"
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
         "if (Chanend *chanend = checkChanend(*thread, resID)) {\n"
         "  switch (chanend->outt(*thread, %0, TIME)) {\n"
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
      "if (Chanend *chanend = checkChanend(*thread, resID)) {\n"
      "  uint32_t value;\n"
      "  switch (chanend->intoken(*thread, TIME, value)) {\n"
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
      "if (Chanend *chanend = checkChanend(*thread, resID)) {\n"
      "  uint32_t value;\n"
      "  switch (chanend->inct(*thread, TIME, value)) {\n"
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
         "if (Chanend *chanend = checkChanend(*thread, resID)) {\n"
         "  switch (chanend->chkct(*thread, TIME, %1)) {\n"
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
          "if (Chanend *chanend = checkChanend(*thread, resID)) {\n"
          "  switch (chanend->chkct(*thread, TIME, %1)) {\n"
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
      "if (Chanend *chanend = checkChanend(*thread, resID)) {\n"
      "  bool isCt;\n"
      "  if (chanend->testct(*thread, TIME, isCt)) {\n"
      "    %0 = isCt;\n"
      "  } else {\n"
      "    %pause_on(chanend);\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n");
  f2r("TESTWCT", "testwct %0, res[%0]",
      "ResourceID resID(%1);\n"
      "if (Chanend *chanend = checkChanend(*thread, resID)) {\n"
      "  uint32_t value;\n"
      "  if (chanend->testwct(*thread, TIME, value)) {\n"
      "    %0 = value;\n"
      "  } else {\n"
      "    %pause_on(chanend);\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n");
  f2r_in("EET", "eet res[%1], %0",
         "ResourceID resID(%1);\n"
         "if (EventableResource *res = checkEventableResource(*thread, resID)) {\n"
         "  if (%0 != 0) {\n"
         "    res->eventEnable(*thread);\n"
         "  } else {\n"
         "    res->eventDisable(*thread);\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n")
    .setSync()
    .setCanEvent();
  f2r_in("EEF", "eef res[%1], %0",
         "ResourceID resID(%1);\n"
         "if (EventableResource *res = checkEventableResource(*thread, resID)) {\n"
         "  if (%0 == 0) {\n"
         "    res->eventEnable(*thread);\n"
         "  } else {\n"
         "    res->eventDisable(*thread);\n"
         "  }\n"
         "} else {\n"
         "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
         "}\n")
    .setSync()
    .setCanEvent();
  f2r_inout("INSHR", "inshr %0, res[%1]",
            "ResourceID resID(%1);\n"
            "if (Port *res = checkPort(*thread, resID)) {\n"
            "  uint32_t value;\n"
            "  Resource::ResOpResult result = res->in(*thread, TIME, value);\n"
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
            "if (Port *res = checkPort(*thread, resID)) {\n"
            "  Resource::ResOpResult result = res->out(*thread, %0, TIME);\n"
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
      "if (Port *res = checkPort(*thread, resID)) {\n"
      "  %0 = res->getTimestamp(*thread, TIME);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, %1);\n"
      "}\n").setSync();
  f2r_in("SETPT", "setpt res[%1], %0",
         "ResourceID resID(%1);\n"
         "if (Port *res = checkPort(*thread, resID)) {\n"
         "  switch (res->setPortTime(*thread, %0, TIME)) {\n"
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
    .addImplicitOp(sp, out);
  // TODO should we check the pc range?
  f1r("SETDP", "set dp, %0", "%1 = %0;")
    .addImplicitOp(dp, out);
  // TODO should we check the pc range?
  f1r("SETCP", "set cp, %0", "%1 = %0;")
    .addImplicitOp(cp, out);
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
      "if (!CHECK_PC(%0)) {\n"
      "  %exception(ET_ILLEGAL_PC, %0)\n"
      "}\n"
      "%pc = target;\n"
      "%next\n");
  f1r("BLA", "bla %0",
      "uint32_t target;\n"
      "if (%0 & 1) {\n"
      "  %exception(ET_ILLEGAL_PC, %0)\n"
      "}\n"
      "target = TO_PC(%0);\n"
      "if (!CHECK_PC(%0)) {\n"
      "  %exception(ET_ILLEGAL_PC, %0)\n"
      "}\n"
      "%1 = FROM_PC(%pc);\n"
      "%pc = target;\n"
      "%next\n")
    .addImplicitOp(lr, out);
  f1r("BRU", "bru %0",
      "uint32_t target = %pc + %0;\n"
      "if (!CHECK_PC(%0)) {\n"
      "  %exception(ET_ILLEGAL_PC, FROM_PC(target))\n"
      "}\n"
      "%pc = target;\n"
      "%next\n");
  f1r("TSTART", "start t[%0]",
      "ResourceID resID(%0);\n"
      "Thread *t = checkThread(*thread, resID);\n"
      "if (t && t->getState().inSSync() && !t->getState().getSync()) {\n"
      "  t->getState().setSSync(false);\n"
      "  t->getState().pc++;"
      "  t->getState().schedule();"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n");
  f1r_out("DGETREG", "dgetreg %0", "").setUnimplemented();
  f1r("KCALL", "kcall %0", "").setUnimplemented();
  f1r("FREER", "freer res[%0]",
      "Resource *res = checkResource(*thread, ResourceID(%0));\n"
      "if (!res || !res->free()) {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, %0);\n"
      "}\n");
  f1r("MSYNC", "msync res[%0]",
      "ResourceID resID(%0);\n"
      "if (Synchroniser *sync = checkSync(*thread, resID)) {\n"
      "  switch (sync->msync(*thread)) {\n"
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
      "if (Synchroniser *sync = checkSync(*thread, resID)) {\n"
      "  switch (sync->mjoin(*thread)) {\n"
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
      "if (EventableResource *res = checkEventableResource(*thread, resID)) {\n"
      "  uint32_t target = TO_PC(%1);\n"
      "  if (CHECK_PC(target)) {\n"
      "    res->setVector(*thread, target);\n"
      "  } else {\n"
      "    // TODO\n"
      "    ERROR();\n"
      "  }\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").addImplicitOp(r11, in).setSync();
  f1r("SETEV", "setev res[%0], %1",
      "ResourceID resID(%0);\n"
      "if (EventableResource *res = checkEventableResource(*thread, resID)) {\n"
      "  res->setEV(*thread, %1);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").addImplicitOp(r11, in).setSync();
  f1r("EDU", "edu res[%0]",
      "ResourceID resID(%0);\n"
      "if (EventableResource *res = checkEventableResource(*thread, resID)) {\n"
      "  res->eventDisable(*thread);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").setSync();
  f1r("EEU", "eeu res[%0]",
      "ResourceID resID(%0);\n"
      "if (EventableResource *res = checkEventableResource(*thread, resID)) {\n"
      "  res->eventEnable(*thread);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").setSync().setCanEvent();
  f1r("WAITET", "waitet %0",
      "if (%0) {\n"
      "  thread->enableEvents();\n"
      "  %deschedule\n"
      "}\n").setSync().setCanEvent();
  f1r("WAITEF", "waitef %0",
      "if (!%0) {\n"
      "  thread->enableEvents();\n"
      "  %deschedule\n"
      "}\n").setSync().setCanEvent();
  f1r("SYNCR", "syncr res[%0]",
      "ResourceID resID(%0);\n"
      "if (Port *port = checkPort(*thread, resID)) {\n"
      "  switch (port->sync(*thread, TIME)) {\n"
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
      "if (Port *res = checkPort(*thread, resID)) {\n"
      "  res->clearPortTime(*thread, TIME);\n"
      "} else {\n"
      "  %exception(ET_ILLEGAL_RESOURCE, resID);\n"
      "}\n").setSync();
  f0r("GETID", "get %0, id", "%0 = thread->getID();")
    .addImplicitOp(r11, out);
  f0r("GETET", "get %0, %1", "%0 = %1;")
    .addImplicitOp(r11, out)
    .addImplicitOp(et, in);
  f0r("GETED", "get %0, %1", "%0 = %1;")
    .addImplicitOp(r11, out)
    .addImplicitOp(ed, in);
  f0r("GETKEP", "get %0, %1", "%0 = %1;")
    .addImplicitOp(r11, out)
    .addImplicitOp(kep, in);
  f0r("GETKSP", "get %0, %1", "%0 = %1;")
    .addImplicitOp(r11, out)
    .addImplicitOp(ksp, in);
  // KEP is 128-byte aligned, the lower 7 bits are set to 0.
  f0r("SETKEP", "set %0, %1", "%0 = %1 & ~((1 << 7) - 1);")
    .addImplicitOp(kep, out)
    .addImplicitOp(r11, in);
  // TODO handle illegal spc
  f0r("KRET", "kret",
      "%pc = TO_PC(%0);\n"
      "%3 = %1;\n"
      "ThreadState::sr_t value((int)%2);\n"
      "value[ThreadState::WAITING] = false;\n"
      "%4 = value;")
    .addImplicitOp(spc, in)
    .addImplicitOp(sed, in)
    .addImplicitOp(ssr, in)
    .addImplicitOp(ed, out)
    .addImplicitOp(sr, out)
    .setSync();
  f0r("DRESTSP", "drestsp", "").setUnimplemented();
  f0r("LDSPC", "ldw %0, sp[1]", 
      "uint32_t Addr = %1 + (1 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(spc, out)
    .addImplicitOp(sp, in);
  f0r("LDSSR", "ldw %0, sp[2]", 
      "uint32_t Addr = %1 + (2 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(ssr, out)
    .addImplicitOp(sp, in);
  f0r("LDSED", "ldw %0, sp[3]", 
      "uint32_t Addr = %1 + (3 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(sed, out)
    .addImplicitOp(sp, in);
  f0r("LDET", "ldw %0, sp[4]", 
      "uint32_t Addr = %1 + (4 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "%0 = LOAD_WORD(PhyAddr);\n")
    .addImplicitOp(et, out)
    .addImplicitOp(sp, in);
  f0r("STSPC", "stw %0, sp[1]", 
      "uint32_t Addr = %1 + (1 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "STORE_WORD(%0, PhyAddr);\n")
    .addImplicitOp(spc, in)
    .addImplicitOp(sp, in);
  f0r("STSSR", "stw %0, sp[2]", 
      "uint32_t Addr = %1 + (2 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "STORE_WORD(%0, PhyAddr);\n")
    .addImplicitOp(ssr, in)
    .addImplicitOp(sp, in);
  f0r("STSED", "stw %0, sp[3]", 
      "uint32_t Addr = %1 + (3 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "STORE_WORD(%0, PhyAddr);\n")
    .addImplicitOp(sed, in)
    .addImplicitOp(sp, in);
  f0r("STET", "stw %0, sp[4]", 
      "uint32_t Addr = %1 + (4 << 2);\n"
      "uint32_t PhyAddr = PHYSICAL_ADDR(Addr);\n"
      "if (!CHECK_ADDR_WORD(PhyAddr)) {\n"
      "  %exception(ET_LOAD_STORE, Addr)\n"
      "}\n"
      "STORE_WORD(%0, PhyAddr);\n")
    .addImplicitOp(et, in)
    .addImplicitOp(sp, in);
  f0r("FREET", "freet", "").setCustom();
  f0r("DCALL", "dcall", "").setUnimplemented();
  f0r("DRET", "dret", "").setUnimplemented();
  f0r("DENTSP", "dentsp", "").setUnimplemented();
  f0r("CLRE", "clre", "thread->clre();\n").setSync();
  f0r("WAITEU", "waiteu",
      "thread->enableEvents();\n"
      "%deschedule\n").setSync().setCanEvent();
  f0r("SSYNC", "", "").setCustom();

  pseudoInst("ILLEGAL_PC", "", "").setCustom();
  pseudoInst("ILLEGAL_PC_THREAD", "", "").setCustom();
  pseudoInst("NO_THREADS", "", "").setCustom();
  pseudoInst("ILLEGAL_INSTRUCTION", "", "").setCustom();
  pseudoInst("DECODE", "", "").setCustom();
  pseudoInst("SYSCALL", "", "").setCustom();
  pseudoInst("EXCEPTION", "", "").setCustom();
}

int main()
{
  add();
  emitInstDispatch();
  emitInstList();
}
