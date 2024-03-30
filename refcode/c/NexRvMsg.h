/*
* Copyright (c) 2020 IAR Systems AB.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

//****************************************************************************
// File NexRvMsg.h  - Nexus RISC-V Trace message definitions (dump & decode)

#ifndef NEXRVMSG_H
#define NEXRVMSG_H

#ifndef NEXM_EXTERNAL   // NEXM_... are defined externally 

#include "NexRv.h"

// Macros to define Nexus Messages (NEXM_...)
//  NOTE: These macros refer to 'NEXUS_TCODE_...' and 'NEXUS_FLDSIZE_...'
//  It is also possible to NOT do it, but it provides less flexibility.
//
//                          name   def (marker | value)
//#define NEXM_BEG(n, t)      {#n,    0x100 | (t)                 }
#define NEXM_BEG(n, t)      {#n,    0x100 | (NEXUS_TCODE_##n)   },
#define NEXM_SRC(opt)
//#define   NEXM_FLD(n, s)    {#n,    0x200 | (s)                 }
#define   NEXM_FLD(n, s)    {#n,    0x200 | (NEXUS_FLDSIZE_##n) },
#define   NEXM_VAR(n)       {#n,    0x400                       },
#define   NEXM_ADR(n)       {#n,    0xC00                       },
#define NEXM_END()          {NULL,  1                           },

#define NEXM_VAR_CFG(n, opt) // TODO: Conditional field ...

// Definition of Nexus Messages (subset applicable to RISC-V PC trace)
static struct NEXM_MSGDEF_STRU {
  const char *name; // Name of message/field
  int def;          // Definition of field (see NEXM_... above)
} nexusMsgDef[] = {

#endif  // NEXM_EXTERNAL

  // Naming:
  //    NEXM=Nexus Message, BEG/END=Beginning/End of definition.
  //    SRC=Message source (system-field). Name of an option given.
  //    FLD/VAR=Fixed/variable size field.
  //    ADR=Special case of variable field (without least significant bit). 
  //    CFG=Configurable, Name of an option given.

  NEXM_BEG(Ownership, 2)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_VAR(PROCESS)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(DirectBranch, 3)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_VAR(ICNT)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(IndirectBranch, 4)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_FLD(BTYPE, 2)
    NEXM_VAR(ICNT)
    NEXM_ADR(UADDR)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(Error, 8)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_FLD(ETYPE, 4)
    NEXM_VAR(PAD)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(ProgTraceSync, 9)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_FLD(SYNC, 4)
    NEXM_VAR(ICNT)
    NEXM_ADR(FADDR)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(DirectBranchSync, 11)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_FLD(SYNC, 4)
    NEXM_VAR(ICNT)
    NEXM_ADR(FADDR)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(IndirectBranchSync, 12)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_FLD(SYNC, 4)
    NEXM_FLD(BTYPE, 2)
    NEXM_VAR(ICNT)
    NEXM_ADR(FADDR)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(ResourceFull, 27)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_FLD(RCODE, 4)
    NEXM_VAR(RDATA)
    NEXM_VAR_CFG(HREPEAT, EnaRepeatedHistory) // Configurable
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(IndirectBranchHist, 28)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_FLD(BTYPE, 2)
    NEXM_VAR(ICNT)
    NEXM_ADR(UADDR)
    NEXM_VAR(HIST)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(IndirectBranchHistSync, 29)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_FLD(SYNC, 4)
    NEXM_FLD(BTYPE, 2)
    NEXM_VAR(ICNT)
    NEXM_ADR(FADDR)
    NEXM_VAR(HIST)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(RepeatBranch, 30)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_VAR(BCNT)
    NEXM_VAR(TSTAMP)
  NEXM_END()

  NEXM_BEG(ProgTraceCorrelation, 33)
    NEXM_SRC(SrcBits)                         // Configurable
    NEXM_FLD(EVCODE, 4)
    NEXM_FLD(CDF, 2)
    NEXM_VAR(ICNT)
    NEXM_VAR(HIST)
    NEXM_VAR(TSTAMP)
  NEXM_END()

#ifndef NEXM_EXTERNAL   // NEXM_... are defined externally 

  { NULL, 0 } // End-marker ('def == 0' is not otherwise used - see NEXM_...)
};

#endif  // NEXM_EXTERNAL

#endif  // NEXRVMSG_H

//****************************************************************************
// End of NexRvMsg.h file
