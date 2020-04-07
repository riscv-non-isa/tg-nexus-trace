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

#include "NexRv.h"

// Macros to define Nexus Messages (NEXM_...)
//  NOTE: These macros refer to 'NEXUS_TCODE_...' and 'NEXUS_FLDSIZE_...'
//  It is also possible to NOT do it, but it provides less flexibility.
//
//                          name   def (marker | value)
//#define NEXM_BEG(n, t)      {#n,    0x100 | (t)                 }
#define NEXM_BEG(n, t)      {#n,    0x100 | (NEXUS_TCODE_##n)   }
//#define   NEXM_FLD(n, s)    {#n,    0x200 | (s)                 }
#define   NEXM_FLD(n, s)    {#n,    0x200 | (NEXUS_FLDSIZE_##n) }
#define   NEXM_VAR(n)       {#n,    0x400                       }
#define   NEXM_ADR(n)       {#n,    0xC00                       }
#define NEXM_END            {NULL,  1                           }

// Definition of Nexus Messages (subset applicable to RISC-V PC trace)
static struct NEXM_MSGDEF_STRU {
  const char *name; // Name of message/field
  int def;          // Definition of field (see NEXM_... above)
} nexusMsgDef[] = {

  NEXM_BEG(OwnershipTraceMessage, 2),
  NEXM_VAR(PROCESS),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(DirectBranch, 3),
  NEXM_VAR(ICNT),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(IndirectBranch, 4),
  NEXM_FLD(BTYPE, 2),
  NEXM_VAR(ICNT),
  NEXM_ADR(UADDR),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(Error, 8),
  NEXM_FLD(ETYPE, 4),
  NEXM_VAR(PAD),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(ProgramTraceSynchronization, 9),
  NEXM_FLD(SYNC, 4),
  NEXM_VAR(ICNT),
  NEXM_ADR(FADDR),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(DirectBranchwithSync, 11),
  NEXM_FLD(SYNC, 4),
  NEXM_VAR(ICNT),
  NEXM_ADR(FADDR),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(IndirectBranchwithSync, 12),
  NEXM_FLD(SYNC, 4),
  NEXM_FLD(BTYPE, 2),
  NEXM_VAR(ICNT),
  NEXM_ADR(FADDR),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(ResourceFull, 27),
  NEXM_FLD(RCODE, 4),
  NEXM_VAR(RDATA),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(IndirectBranchHistory, 28),
  NEXM_FLD(BTYPE, 2),
  NEXM_VAR(ICNT),
  NEXM_ADR(UADDR),
  NEXM_VAR(HIST),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(IndirectBranchHistorywithSync, 29),
  NEXM_FLD(SYNC, 4),
  NEXM_FLD(BTYPE, 2),
  // NEXM_FLD(CANCEL, 1),
  NEXM_VAR(ICNT),
  NEXM_ADR(FADDR),
  NEXM_VAR(HIST),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(RepeatBranch, 30),
  NEXM_VAR(BCNT),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  NEXM_BEG(ProgramTraceCorrelation, 33),
  NEXM_FLD(EVCODE, 4),
  NEXM_FLD(CDF, 2),
  NEXM_VAR(ICNT),
  NEXM_VAR(TSTAMP),
  NEXM_END,

  { NULL, 0 } // End-marker ('def == 0' is not otherwise used - see NEXM_...)
};

#endif  // NEXRVMSG_H

//****************************************************************************
// End of NexRvMsg.h file
