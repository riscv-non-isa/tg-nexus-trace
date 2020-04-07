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
// File NexRvDeco.c  - Nexus RISC-V Trace decoder reference implementation

// Code below is written in plain C-code.
// It was compiled using VisualC, GNU and IAR C/C++ compiler.
//  1. Only standard C-types are used.
//  2. Only few standard C functions used - see notes with "#include <...>"
//  3. Only non K&R C is 'for (int x' and 'int x;' between instructions.

#include <stdio.h>  //  For NULL, 'printf', 'fopen, ...'
#include <stdlib.h> //  For 'exit'
#include <string.h> //  For 'strcmp', 'strchr' 
#include <ctype.h>  //  For 'isspace/isxdigit' etc.

#include "NexRv.h"    //  Common NEXUS_... #define (RISC-V specific subset)
#include "NexRvMsg.h" //  Definition of Nexus messages
#include "NexRvInfo.h" //  Definition of Nexus messages

// Decoder works on two files and dumper on first file
extern FILE *fNex;  // Nexus messages (binary bytes)

static unsigned int nexdeco_pc        = 0;
static unsigned int nexdeco_lastAddr  = 0;
static int nInstr = 0;

static int EmitErrorMsg(const char *err)
{
  printf("\nERROR: %s\n", err);
  return -10;
}

// This function is called with -1 parameter to reach next BRANCH.
// Otherwise it is 'n' 16-bit steps (over direct JUMP/CALL as well).
// It should never step over INDIRECT instruction (RET or JUMP/CALL)
static int EmitICNT(int n, unsigned int hist, int disp)
{
  if (disp & 1) printf(". PC=0x%X, EmitICNT(n=%d,hist=0x%X)\n", nexdeco_pc, n, hist);

  // if (n >= 0 && hist != 0) n = -1;  // Prove ICNT is optional (big savings!)

  unsigned int histMask = 0;  // MSB is first in history, so we need sliding mask
  if (hist != 0)
  {
    histMask = 0x1; while (histMask < hist) histMask <<= 1; histMask >>= 2;
  }

  while (n != 0)
  {
    printf("0x%08X", nexdeco_pc);
    nInstr++; // Statistics (for compression display)

    unsigned int a;
    unsigned int info = InfoGet(nexdeco_pc, &a);
    if (info == 0) return EmitErrorMsg("info is unknown");

#if 1 // Append type of instruction to plain PC value
    if (info & INFO_CALL)         printf(",C");
    else if (info & INFO_RET)     printf(",R");
    else if (info & INFO_JUMP)    printf(",J");
    else if (info & INFO_BRANCH)  printf(",B");
    else                          printf(",L");

    // Report non-taken branch as BN
    if (info & INFO_BRANCH && !(histMask & hist)) printf("N");

    if (info & (INFO_INDIRECT) && !(info & INFO_RET)) printf("I");
    if (info & INFO_4) printf("4\n"); else printf("2\n");
#endif

    if (n > 0)
    {
      // Step over 16-bit unit
      if (info & INFO_4) n -= 2; else n -= 1;
      if (n < 0) return EmitErrorMsg("ICNT too small");
    }

    if (info & INFO_INDIRECT) // Cannot continue over indirect...
    {
      if (n > 0) {
        printf("ERROR EmitICNT with n=%d\n", n);
        return EmitErrorMsg("indirect address encountered in ICNT");
      }
      break;
    }

    if (info & INFO_BRANCH)
    {
      if (histMask & hist)
      {
        info |= INFO_JUMP;  // Force PC change below
      }
      histMask >>= 1;
    }

    if (info & INFO_JUMP)   nexdeco_pc = a;   // Direct jump/call/branch
    else if (info & INFO_4) nexdeco_pc += 4;  // Linear 4 or 2 otherwise
    else                    nexdeco_pc += 2;
  }

  return 1;
}

#define MSGFIELDS_MAX   10 // Some reasonable limit

static int          msgFieldPos = 0;
static unsigned int msgFields[MSGFIELDS_MAX];
static int          msgFieldCnt = 0;

static int NexusFieldGet(const char *name, unsigned int *p)
{
  for (int d = msgFieldPos; nexusMsgDef[d].def != 1; d++)
  {
    if (strcmp(nexusMsgDef[d].name, name) == 0)
    {
      int fi = d - msgFieldPos;
      if (fi > msgFieldCnt) *p = 0;
      else      *p = msgFields[fi];
      return 1; // OK
    }
  }
  return 0;
}

#define NEX_FLDGET(n) unsigned int n = 0; if (!NexusFieldGet(#n, &n)) return (-1)

static int MsgHandle(int disp)
{
  int TCODE = msgFields[0];
  switch (TCODE)
  {
    case NEXUS_TCODE_DirectBranch:
      {
        NEX_FLDGET(ICNT);
        if (EmitICNT(ICNT, 0x3, disp) != 1) return (-2);
      }
      break;

    case NEXUS_TCODE_IndirectBranch:
      {
        // NEX_FLDGET(BTYPE); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(UADDR);
        if (EmitICNT(ICNT, 0x0, disp) != 1) return (-2);
        nexdeco_lastAddr ^= (UADDR << NEXUS_PARAM_AddrSkip);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_ProgramTraceSynchronization:
      {
        // NEX_FLDGET(SYNC); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(FADDR);

        if (EmitICNT(ICNT, 0, disp) != 1) return (-2);
        nexdeco_lastAddr = (FADDR << NEXUS_PARAM_AddrSkip);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_DirectBranchwithSync:
      {
        // NEX_FLDGET(SYNC); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(FADDR);
        if (EmitICNT(ICNT, 0x0, disp) != 1) return (-2);
        nexdeco_lastAddr = (FADDR << NEXUS_PARAM_AddrSkip);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_IndirectBranchwithSync:
      {
        // NEX_FLDGET(SYNC); // We ignore this for now
        // NEX_FLDGET(BTYPE); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(FADDR);
        if (EmitICNT(ICNT, 0x0, disp) != 1) return (-2);
        nexdeco_lastAddr = (FADDR << NEXUS_PARAM_AddrSkip);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_IndirectBranchHistory:
      {
        // NEX_FLDGET(BTYPE); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(UADDR);
        NEX_FLDGET(HIST);

        if (EmitICNT(ICNT, HIST, disp) != 1) return (-2);
        nexdeco_lastAddr ^= (UADDR << NEXUS_PARAM_AddrSkip);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_IndirectBranchHistorywithSync:
      {
        // NEX_FLDGET(SYNC); // We ignore this for now
        // NEX_FLDGET(BTYPE); // We ignore this for now
        // NEX_FLDGET(CANCEL); // Not used
        NEX_FLDGET(ICNT);
        NEX_FLDGET(FADDR);
        NEX_FLDGET(HIST);

        if (EmitICNT(ICNT, HIST, disp) != 1) return (-2);
        nexdeco_lastAddr = (FADDR << NEXUS_PARAM_AddrSkip);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_Error:
    case NEXUS_TCODE_ResourceFull:
    case NEXUS_TCODE_ProgramTraceCorrelation:
      // We ignore these now - these are either at very end
      // or are followed by full-sync
      break;

    case NEXUS_TCODE_RepeatBranch:  // Handled differently!
    default:
      return -(100 + TCODE);  // Not handled TCODE
  }

  return 0;
}

// This function is an extension of 'NexusDump'
// It adds all fields (for each message) into fldArray and at end of each message
// it calls 'MsgHandle()' function.
int NexusDeco(int disp)
{
  int fldDef = -1;
  int fldBits = 0;
  unsigned int fldVal = 0;

  int msgCnt = 0;
  int msgBytes = 0;
  int msgErrors = 0;

  msgFieldCnt = 0;  // No fields

  unsigned char msgByte = 0;
  unsigned char prevByte = 0;
  for (;;)
  {
    prevByte = msgByte;
    if (fread(&msgByte, 1, 1, fNex) != 1) break;  // EOF

#if 1 // This will skip long sequnece of idles (visible in true captures ...)
    if (msgByte == 0xFF && prevByte == 0xFF)
    {
      continue;
    }
#endif

    if (disp & 1)
    {
      if (msgCnt > 0 && fldDef < 0)
      {
        printf(". \n");
      }
      printf(". 0x%02X ", msgByte);
      for (int b = 0x80; b != 0; b >>= 1)
      {
        if (b == 0x2) printf("_");
        if (msgByte & b) printf("1"); else printf("0");
      }
      printf(":");
    }

    unsigned int mdo = msgByte >> 2;
    unsigned int mseo = msgByte & 0x3;

    if (mseo == 0x2)
    {
      printf(" ERROR: MSEO='10' is not allowed\n");
      return -1;  // Error return
    }

    if (fldDef < 0)
    {
      if (mseo == 0x3)
      {
        if (disp & 1) printf(" IDLE\n");
        continue;
      }

      if (mseo != 0x0)
      {
        printf(" ERROR: Message must start from MSEO='00'\n");
        return -2;  // Error return
      }

      for (int d = 0; nexusMsgDef[d].def != 0; d++)
      {
        if ((nexusMsgDef[d].def & 0x100) == 0) continue;
        if ((nexusMsgDef[d].def & 0xFF) == mdo)
        {
          fldDef = d; // Found TCODE
          break;
        }
      }

      if (fldDef < 0)
      {
        printf(" ERROR: Message with TCODE=%d is not defined for RISC-V\n", mdo);
        return -3;
      }

      // Special handling for RepeatBranch message. 
      // We want to preserve previous packet, so we can
      // repeat it at end of RepeatBranch handling.
      if (mdo == NEXUS_TCODE_RepeatBranch)
      {
        // Save previous message fields
        msgFields[6] = msgFieldPos;
        msgFields[7] = msgFieldCnt;
        msgFields[8] = msgFields[0];
        msgFields[9] = msgFields[1];
      }

      // Save to allow later decoding
      msgFieldPos = fldDef;
      msgFieldCnt = 0;
      msgFields[msgFieldCnt++] = mdo;

      if (disp & 3) printf(" TCODE[6]=%d (MSG #%d) - %s\n", mdo, msgCnt, nexusMsgDef[fldDef].name);
      msgCnt++;
      msgBytes++;

      if (mdo == NEXUS_TCODE_ResourceFull || mdo == NEXUS_TCODE_Error) msgErrors++;

      fldDef++;
      fldBits = 0;
      fldVal = 0;
      continue;
    }

    // Accumulate 'mdo' to field value
    fldVal |= (mdo << fldBits);
    fldBits += 6;

    msgBytes++;

    // Process fixed size fields (there may be more than one in one MDO record)
    while ((nexusMsgDef[fldDef].def & 0x200) && fldBits >= (nexusMsgDef[fldDef].def & 0xFF))
    {
      int fldSize = nexusMsgDef[fldDef].def & 0xFF;

      msgFields[msgFieldCnt++] = fldVal & ((1 << fldSize) - 1); // Save field

      if (disp & 1) printf(" %s[%d]=0x%X", nexusMsgDef[fldDef].name, fldSize, fldVal & ((1 << fldSize) - 1));
      fldDef++;
      fldVal >>= fldSize;
      fldBits -= fldSize;
    }

    if (mseo == 0x0)
    {
      if (disp & 1) printf("\n");
      continue;
    }

    if (nexusMsgDef[fldDef].def & 0x400)
    {
      // Variable size field
      if (disp & 1) printf(" %s[%d]=0x%X\n", nexusMsgDef[fldDef].name, fldBits, fldVal);

      msgFields[msgFieldCnt++] = fldVal; // Save field

      if (mseo == 3)
      {
        int cnt = 1;

        if (msgFields[0] == NEXUS_TCODE_RepeatBranch)
        {
          // Special handling for repeat branch (which only has 1 field!)
          cnt = msgFields[1]; // Counter set in RepeatBranch message
          msgFieldPos = msgFields[6]; // Restrore previous message (saved)
          msgFieldCnt = msgFields[7];
          msgFields[0] = msgFields[8];
          msgFields[1] = msgFields[9];
        }

        while (cnt > 0) // Handle (1 or many times ...)
        {
          int err = MsgHandle(disp);
          if (err < 0) return err;
          cnt--;
        }

        fldDef = -1;
      }
      else
      {
        fldDef++;
      }
      fldBits = 0;
      fldVal = 0;
      continue;
    }

    if (fldBits > 0)
    {
      printf(" ERROR: Not enough bits for non-variable field\n");
      return -4;
    }
  }

  if (disp & 4)
  {
    printf(". Stat: %d bytes, %d messages, %d error messages", msgBytes, msgCnt, msgErrors);
    if (msgCnt > 0) printf(", %.2lf bytes/message", ((double)msgBytes) / msgCnt);
    if (nInstr > 0) printf(", %d instr, %.2lf bits/instr", nInstr, ((double)msgBytes * 8) / nInstr);
    printf("\n");
  }

  return nInstr; // Number of instructions generated
}

//****************************************************************************
// End of NexRvDeco.c file
