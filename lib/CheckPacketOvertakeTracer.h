// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _CheckPacketOvertakeTracer_h_
#define _CheckPacketOvertakeTracer_h_

#include "Tracer.h"
#include "Token.h"
#include <map>
#include <vector>

namespace axe {
  class ResourceID;

  class CheckPacketOvertakeTracer : public Tracer {
    const Thread *thread;
    uint32_t pc;
    bool processedInstruction;
    struct PacketInfo {
      std::vector<Token> contents;
    };
    struct ChanEndInfo {
      std::vector<PacketInfo> outgoingPackets;
    };
    std::map<uint32_t, ChanEndInfo> chanEndMap;
    void reportMismatch(ResourceID resID, const PacketInfo &first,
                        const PacketInfo &second);
    void handleInput(ResourceID resID);
    void handleOutput(ResourceID resID, const Token &token);
    void processInstruction(const Thread &t, uint32_t pc);
  public:
    CheckPacketOvertakeTracer();
    
    void instructionBegin(const Thread &t) override;
    void instructionEnd() override;
    void regWrite(Register::Reg reg, uint32_t value) override;
  };
} // End axe namespace

#endif // _CheckPacketOvertakeTracer_h_
