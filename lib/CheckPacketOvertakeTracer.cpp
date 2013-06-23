// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "CheckPacketOvertakeTracer.h"
#include "Token.h"
#include "Resource.h"
#include "Thread.h"
#include "Instruction.h"
#include "InstructionOpcode.h"

#include "llvm/Support/raw_ostream.h"

// This tracer checks for possible packet overtaking as follows:
// For each channel end it keeps track of the unacknowledge outgoing packets
// that have been sent. The list of unacknowledge outgoing packets is emptied
// when an incoming packet is received, on the assumption that the incoming
// packet is an acknowledgement. If there are are ever multiple unacknowledged
// outgoing packets with different contents a possible overtaking issue is
// reported.
// This heuristic might report false positives (there might be some other
// synchronization apart from an acknowledgement sent to the same channel end)
// and it might not detect all overtaking issues (since we assume that the
// incoming packet is an acknowledgement without checking that it was was sent
// after the outgoing packets were received).

using namespace axe;

CheckPacketOvertakeTracer::CheckPacketOvertakeTracer() :
  thread(nullptr)
{
}

void CheckPacketOvertakeTracer::
reportMismatch(ResourceID resID, const PacketInfo &first,
               const PacketInfo &second)
{
  llvm::outs() << "warning: packet sent by channel end 0x" << resID.id;
  llvm::outs() << " may overtake previous packet\n";
}

void CheckPacketOvertakeTracer::handleInput(ResourceID resID)
{
  if (!resID.isChanend())
    return;
  ChanEndInfo &info = chanEndMap[resID];
  info.outgoingPackets.clear();
}

void CheckPacketOvertakeTracer::
handleOutput(ResourceID resID, const Token &token)
{
  if (!resID.isChanend())
    return;
  ChanEndInfo &info = chanEndMap[resID];
  if (info.outgoingPackets.empty()) {
    info.outgoingPackets.push_back(PacketInfo());
  }
  info.outgoingPackets.back().contents.push_back(token);
  if (token.isCtEnd() || token.isCtPause()) {
    if (token.isCtPause()) {
      info.outgoingPackets.back().contents.pop_back();
    }
    unsigned numPackets = info.outgoingPackets.size();
    if (numPackets > 1) {
      const PacketInfo &previous = info.outgoingPackets[numPackets - 2];
      const PacketInfo &current = info.outgoingPackets.back();
      if (previous.contents != current.contents) {
        reportMismatch(resID, previous, current);
      }
    }
    info.outgoingPackets.push_back(PacketInfo());
  }
}

void CheckPacketOvertakeTracer::processInstruction(const Thread &t, uint32_t pc)
{
  // Disassemble instruction.
  InstructionOpcode opcode;
  Operands ops;
  instructionDecode(t.getParent(), pc, opcode, ops, true);
  switch (opcode) {
  default:
    break;
  case IN_2r:
  case INCT_2r:
  case INT_2r:
    {
      ResourceID resID = t.regs[ops.ops[1]];
      handleInput(resID);
    }
    break;
  case CHKCT_2r:
  case CHKCT_rus:
    {
      ResourceID resID = t.regs[ops.ops[0]];
      handleInput(resID);
    }
    break;
  case OUT_2r:
    {
      ResourceID resID = t.regs[ops.ops[1]];
      uint32_t data = t.regs[ops.ops[0]];
      Token tokens[] = {
        Token((data >> 24) & 0xff),
        Token((data >> 16) & 0xff),
        Token((data >> 8) & 0xff),
        Token(data & 0xff)
      };
      for (unsigned i = 0; i < 4; i++) {
        handleOutput(resID, tokens[i]);
      }
    }
    break;
  case OUTCT_rus:
    {
      ResourceID resID = t.regs[ops.ops[0]];
      Token token(ops.ops[1], true);
      handleOutput(resID, token);
    }
    break;
  case OUTT_2r:
  case OUTCT_2r:
    {
      bool isCt = opcode == OUTCT_2r;
      ResourceID resID = t.regs[ops.ops[1]];
      Token token(t.regs[ops.ops[0]], isCt);
      handleOutput(resID, token);
    }
    break;
  case SETD_2r:
    {
      ResourceID resID = t.regs[ops.ops[1]];
      if (!resID.isChanend())
        return;
      chanEndMap[resID].outgoingPackets.clear();
    }
    break;
  }
}

void CheckPacketOvertakeTracer::instructionBegin(const Thread &t)
{
  assert(!thread);
  thread = &t;
  pc = t.getRealPc();
  processedInstruction = false;
}

void CheckPacketOvertakeTracer::instructionEnd()
{
  assert(thread);
  if (!processedInstruction) {
    processInstruction(*thread, pc);
  }
  thread = nullptr;
}

void CheckPacketOvertakeTracer::regWrite(Register::Reg reg, uint32_t value)
{
  assert(thread);
  if (!processedInstruction) {
    processInstruction(*thread, pc);
  }
  processedInstruction = true;
}
