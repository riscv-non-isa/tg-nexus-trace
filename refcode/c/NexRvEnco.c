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
// File NexRvEnco.c  - Nexus RISC-V Trace encoder reference implementation

// Code below is written in plain C-code.
// It was compiled using VisualC, GNU and IAR C/C++ compiler.
//  1. Only standard C-types are used.
//  2. Only few standard C functions used - see notes with "#include <...>"
//  3. Only non K&R C is 'for (int x' and 'int x;' between instructions.

#include <stdio.h>  //  For NULL, 'printf', 'fopen, ...'
#include <stdlib.h> //  For 'exit'
#include <string.h> //  For 'strcmp', 'strchr' 
#include <ctype.h>  //  For 'isspace/isxdigit' etc.

#include "NexRv.h"      //  Common NEXUS_... #define (RISC-V specific subset)
#include "NexRvInfo.h"  

extern FILE *fNex; // Nexus messages (binary bytes)

static int encoStat_MsgBytes  = 0;
static int encoStat_MsgCnt    = 0;
static int encoStat_InstrCnt  = 0;

static unsigned int encoNextEmit = 0;
static unsigned int encoICNT;
static unsigned int encoHIST;
static unsigned int encoADDR;
static unsigned int encoBCNT;

static unsigned int prevICNT;
static unsigned int prevHIST;

static int AddVar(unsigned int v, int nPrev, unsigned char *msg, int pos)
{
  if (nPrev > 0)
  { 
    msg[pos - 1] |= (v << (8 - nPrev)) & 0xFF; // Append to previous MDO
    v >>= nPrev;
  }
  while (v != 0 || nPrev < 0)
  {
    msg[pos++] = (v & 0x3F) << 2; // Add 6 bits at a time
    v >>= 6;
    nPrev = 0;  // Will stop at 0-value
  }
  msg[pos - 1] |= 1; // Set MSEO='01' at last byte
  return pos;
}

// Handle retired instruction (it may be implemented as HW pipeline)
static int HandleRetired(unsigned int addr, unsigned int info, int level, int disp)
{
  if (disp & 2) printf("Enco: pc=0x%08X,info=0x%X\n", addr, info);

  if (encoStat_MsgBytes == 0)
  {
    // Initialize encoder state
    encoICNT = 0;
    encoHIST = 1;
    encoADDR = 0;
    encoBCNT = 0;

    prevICNT = 0;
    prevHIST = 0; // Will never match ...

    encoNextEmit = NEXUS_TCODE_ProgramTraceSynchronization;
  }

  if (info == 0 && encoICNT > 0)  // Flush requested
  {
    encoNextEmit = NEXUS_TCODE_IndirectBranchHistory;
    prevHIST = 0; // Make sure repeat will be generated
  }

  if (encoNextEmit != 0)
  {
    unsigned char msg[40];
    int  pos = 0;

    if (level >= 21)  // This piece of code detects and generates Repeat Branch message
    {
      if ((encoADDR == addr) && (encoNextEmit == NEXUS_TCODE_IndirectBranchHistory) && (prevHIST == encoHIST) && (prevICNT == encoICNT))
      {
        // IndirectBranchHistory message back to same address
        encoBCNT++;
        encoNextEmit = 0;  // Will be skipped below
      }
      else
      {
        // Different message type or body ...
        if (encoBCNT > 0)
        {
          // We must produce RepeatBranch message before this one ...
          msg[pos++] = NEXUS_TCODE_RepeatBranch << 2;
          pos = AddVar(encoBCNT, 0, msg, pos);
          msg[pos - 1] |= 3; // Set MSEO='11' at last byte

          encoStat_MsgCnt++;

          encoBCNT = 0; // One time
        }
      }
    }

    msg[pos++] = encoNextEmit << 2;
    if (encoNextEmit == NEXUS_TCODE_ProgramTraceSynchronization)
    {
      msg[pos++] = 0x1 << 2;  // SYNC:4=1 (always)
      pos = AddVar(encoICNT, 6 - 4, msg, pos);
      pos = AddVar(addr >> NEXUS_PARAM_AddrSkip, 0, msg, pos);
      prevHIST = 0; // Will never match
    }
    else if (encoNextEmit == NEXUS_TCODE_IndirectBranchHistory || encoNextEmit == NEXUS_TCODE_IndirectBranch)
    {
      prevHIST = encoHIST;  // Save to check for repeat next time ...
      prevICNT = encoICNT;

      msg[pos++] = 0x0 << 2;  // BTYPE:2=0 (always)
      pos = AddVar(encoICNT, 6 - 2, msg, pos);
      pos = AddVar((encoADDR ^ addr) >> NEXUS_PARAM_AddrSkip, -1, msg, pos);

      if (encoNextEmit == NEXUS_TCODE_IndirectBranchHistory)
      {
        pos = AddVar(encoHIST, 0, msg, pos);
      }
    }
    else if (encoNextEmit == NEXUS_TCODE_DirectBranch)
    {
      pos = AddVar(encoICNT, -1, msg, pos);
    }

    msg[pos - 1] |= 3; // Set MSEO='11' at last byte

    if (pos > 1)
    {
      if (fwrite(msg, 1, pos, fNex) != pos) return -1;
      encoStat_MsgBytes += pos;
      encoStat_MsgCnt++;
    }

    if (encoNextEmit != NEXUS_TCODE_DirectBranch)
    {
      encoADDR = addr;  // This is new address
    }
    encoNextEmit = 0;   // Only one time
    encoHIST = 1;
    encoICNT = 0;
  }

  // This is key state update (ICNT and HIST fields)
  encoICNT += (info & INFO_4) ? 2 : 1;

  if (level >= 20)
  {
    // NEXUS_TCODE_IndirectBranchHistory messages are allowed
    if (info & INFO_BRANCH)
    {
      if (info & INFO_LINEAR) encoHIST = (encoHIST << 1) | 0; // Not taken jump
      else                    encoHIST = (encoHIST << 1) | 1; // Taken jump
    }
    else if (info & INFO_INDIRECT)
    {
      encoNextEmit = NEXUS_TCODE_IndirectBranchHistory; // Emit on next address time ...
    }

    // Make sure we do not make fields too big (this was never checked ...)
    // if (encoICNT >= (0x3FF - 2))  encoNextEmit = NEXUS_TCODE_IndirectBranchHistory;
    if (encoHIST & (0x1 << 31))   encoNextEmit = NEXUS_TCODE_IndirectBranchHistory;
  }
  else
  {
    // Only NEXUS_TCODE_DirectBranch messages are allowed
    if (info & INFO_BRANCH)
    {
      if (info & INFO_LINEAR)
      {
        // Not-taken branches are reported as linear instructions, so nothing special
      }
      else
      {
        // This is taken direct branch
        encoNextEmit = NEXUS_TCODE_DirectBranch;
      }
    }
    else if (info & INFO_INDIRECT)
    {
      encoNextEmit = NEXUS_TCODE_IndirectBranch; // Emit on next address time ...
    }
  }

  return 0; // OK
}

int NexusEnco(FILE *f, int level, int disp)
{
  encoStat_MsgBytes = 0;
  encoStat_MsgCnt   = 0;
  encoStat_InstrCnt = 0;

  unsigned int a, info;
  char line[1000];
  while (fgets(line, sizeof(line), f) != NULL)
  {
    if (disp & 1) printf("%s", line);
    if (line[0] == '.' && line[1] == 'e') break; // End
    if (line[0] == '.') continue; // Comment (ignore this line)
    if (line[0] == '\0' || line[0] == '\n') continue; // Ignore empty as well ...

    if (!InfoParse(line, &a, &info, NULL))  return -1;
    if (info == 0)                          return -2;

    encoStat_InstrCnt++;
    int ret = HandleRetired(a, info, level, disp);
    if (ret < 0)  return ret;
  }

  int ret = HandleRetired(a, 0, level, disp);  // Flush ...
  if (ret < 0)  return ret;

  if (disp & 4)
  {
    printf("Stat: %d instr, level=%d.%d => %d bytes, %d messages", encoStat_InstrCnt, level / 10, level % 10, encoStat_MsgBytes, encoStat_MsgCnt);
    if (encoStat_InstrCnt > 0) printf(", %.2lf bits/instr", ((double)encoStat_MsgBytes * 8) / encoStat_InstrCnt);
    printf("\n");
  }

  return encoStat_MsgCnt;
}

//****************************************************************************
// End of NexRvEnco.c file
