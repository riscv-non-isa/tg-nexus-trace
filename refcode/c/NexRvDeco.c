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
extern FILE *fNex;      // Nexus messages (binary bytes)

extern int conf_nSrc;   // Number of source bits

#if 1 // Callstack related
extern int conf_CallStack;
extern void CallStack_Init();
extern void CallStack_Push(Nexus_TypeAddr ret);
extern Nexus_TypeAddr CallStack_Pop();
#endif

int dispHistRepeat = 0;

static Nexus_TypeAddr nexdeco_pc        = 1;  // 1 means, that last address is unknown 
static Nexus_TypeAddr nexdeco_addrCheck = 1;  // Next PC (sent in a packet should match it)
static Nexus_TypeAddr nexdeco_lastAddr  = 1;
static int nInstr = 0;
static int resourceFull_ICNT = 0; // ICNT adjustment because of recent 'ResourceFull' message[s] (positive or negative)

static int EmitErrorMsg(const char *err)
{
  printf("\nERROR: %s\n", err);
  return -10;
}

// This function is called with -1 parameter to reach next BRANCH.
// Otherwise it is 'n' 16-bit steps (over direct JUMP/CALL as well).
// It should never step over INDIRECT instruction (RET or JUMP/CALL)
//  Unless we are in call-stack mode
static int EmitICNT(FILE *f, int n, Nexus_TypeHist hist, int disp)
{
  if (nexdeco_pc & 1) return 0;  // Not synchronized ...

  if ((nexdeco_addrCheck & 1) == 0)
  {
    if (nexdeco_pc != nexdeco_addrCheck)
    {
      printf("CALL_CHK: ERROR (pc=0x%lX, expected=0x%lX)\n", nexdeco_pc, nexdeco_addrCheck);
    }
    else
    {
      printf("CALL_CHK: OK\n");
    }
  }

  int doneICNT = 0; // Number of done ICNT steps

  if (disp & 1) printf(". PC=0x%lX, EmitICNT(n=%d,hist=0x%x)\n", nexdeco_pc, n, hist);

  // Adjust ICNT by what was handled by ResourceFull message[s] before this message (message with normal ICNT field)
  //  NOTE: resourceFull_ICNT maybe positive or negative!
  if (n >= 0 && resourceFull_ICNT != 0)
  {
    if (disp & 1) printf(". ICNT adjust: %d to %d\n", n, n + resourceFull_ICNT);

    n += resourceFull_ICNT;
    if (n < 0) return EmitErrorMsg("ICNT adjustment ERROR");

    resourceFull_ICNT = 0;    // Make adjustment 'consumed'

  }

  Nexus_TypeHist histMask = 0;  // MSB is first in history, so we need sliding mask
  if (hist != 0)
  {
    // TODO: This can be done better (starting from MSB-side)
    if (hist & (((Nexus_TypeHist)1u) << (sizeof(Nexus_TypeHist) * 8 - 1)))  // Is MSB 'stop-bit' set?
    {
      histMask = (((Nexus_TypeHist)1u) << (sizeof(Nexus_TypeHist) * 8 - 2)); // All 31/63-bits of history are valid
    }
    else
    {
      histMask = 0x1; while (histMask <= hist) histMask <<= 1; histMask >>= 2;
    }
  }

  while (n != 0)
  {
    if (f) fprintf(f, "0x%lX", nexdeco_pc);
    nInstr++; // Statistics (for compression display)

    if (disp & 0x8) printf("#%d: PC=0x%lX", nInstr, nexdeco_pc);

    Nexus_TypeAddr a;
    unsigned int info = InfoGet(nexdeco_pc, &a);
    if (info == 0)
    {
      nexdeco_pc        = 1;  // 1 means, that last address is unknown 
      nexdeco_lastAddr  = 1;
      return 0;
    }
    if (info == 0) return EmitErrorMsg("info is unknown");

    // Accumulate ICNT we generate (total is returned by this function)
    if (info & INFO_4) doneICNT += 2; else doneICNT += 1;

    if (disp & 0x10) // Append type of instruction to plain PC value
    {
      int nt = 0;
      char t[8];

      if (info & INFO_CALL)         t[nt++] = 'C';
      else if (info & INFO_RET)     t[nt++] = 'R';
      else if (info & INFO_JUMP)    t[nt++] = 'J';
      else if (info & INFO_BRANCH)  t[nt++] = 'B';
      else                          t[nt++] = 'L';

      // Report non-taken branch as BN
      if (info & INFO_BRANCH)
      {
        // This is a little more demanding as it is different if we have history messages or not
        if (hist == 0)
        {
          if (n > (int)((info & INFO_4) + 2))  // Tricky: '(info & INFO_4)' is either 2 or 0
          {
            t[nt++] = 'N';  // Only last branch is considered taken - all before are not
          }
        }
        else if (!(histMask & hist))
        {
          t[nt++] = 'N';  // Not taken branch in history
        }
      }

      if (info & (INFO_INDIRECT) && !(info & INFO_RET)) t[nt++] = 'I';
      if (info & INFO_4) t[nt++] = '4'; else t[nt++] = '2';
      t[nt] = '\0';
      if (f) fprintf(f, ",%s", t);

      if (disp & 0x8) printf(",%s", t);
    }
    if (f) fprintf(f, "\n");

    if (disp & 0x8) printf("\n");

    if (n > 0)
    {
      // Step over 16-bit unit
      if (info & INFO_4) n -= 2; else n -= 1;
      if (n < 0) return EmitErrorMsg("ICNT too small");
    }

    if (conf_CallStack > 0 && (info & INFO_CALL))
    {
      // This is call (direct or indirect) push address after '[c]jal[r]) to the stack
      Nexus_TypeAddr ret = nexdeco_pc + ((info & INFO_4) ? 4 : 2);
      CallStack_Push(ret);
    }

    if (info & INFO_INDIRECT) // Cannot continue over indirect...
    {
      // We always pop the stack if we see RET ...
      if (conf_CallStack > 0 && (info & INFO_RET))
      {
        Nexus_TypeAddr ret = CallStack_Pop();
        // nexdeco_addrCheck = ret;  // Set PC to be checked (next time)
        if (conf_CallStack > 0)
        {
          nexdeco_pc = ret;
        }
        if (n != 0)
        {
          // We should continue from PC we just pop from the stack 
          if (ret == 1)
          {
            return EmitErrorMsg("Not enough entires on callstack");
          }
          continue;
        }
      }

      if (n > 0) {
        printf("ERROR EmitICNT with n=%d\n", n);
        return EmitErrorMsg("indirect address encountered in ICNT");
      }
      break;
    }

    if (info & INFO_BRANCH)
    {
      if (hist == 0)
      {
        // This is calling as DirectBranch
        if (n == 0)
        {
          info |= INFO_JUMP;  // Force PC change below
        }
      }
      else
      {
        if (histMask & hist)
        {
          info |= INFO_JUMP;  // Force PC change below
        }
        histMask >>= 1;
        if (histMask == 0 && n < 0)
        {
          n = 0;  // This will stop the loop (when we were called with HIST only)
        }
      }
    }

    if (info & INFO_JUMP)   nexdeco_pc = a;   // Direct jump/call/branch
    else if (info & INFO_4) nexdeco_pc += 4;  // Linear 4 or 2 otherwise
    else                    nexdeco_pc += 2;
  }

  return doneICNT;  // Number of ICNT steps done (usually at least 1, but can be 0)
}

#define MSGFIELDS_MAX   10 // Some reasonable limit

static int              msgFieldPos = 0;
static Nexus_TypeField  msgFields[MSGFIELDS_MAX];
static Nexus_TypeField  savedFields[MSGFIELDS_MAX];
static int              msgFieldCnt = 0;

static int NexusFieldGet(const char *name, Nexus_TypeField *p)
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

unsigned int conf_src = 0;

#define NEX_FLDGET(n) Nexus_TypeField n = 0; if (!NexusFieldGet(#n, &n)) return (-1)

static const Nexus_TypeAddr msb_mask = ((Nexus_TypeAddr)1UL) << 49;

static Nexus_TypeAddr CalculateAddr(Nexus_TypeAddr fu_addr, int full, Nexus_TypeAddr prev_addr)
{
  fu_addr <<= NEXUS_PARAM_AddrSkip; // LSB bit is never sent
  if ((fu_addr >> 32) & 0x10000)           // Perform MSB extension
  {
    fu_addr |= 0xFFFF000000000000UL;
  }
  // Update (NEW or XOR)
  if (!full) 
    fu_addr ^= prev_addr;
  // printf("NADDR=0x%lX\n", fu_addr);
  return fu_addr;
}
static int MsgHandle(FILE *f, int disp)
{
  int doneICNT;
  
  int TCODE = msgFields[0];
  if (0 && msgFields[1] != conf_src)      // Is this SRC we are looking for
  {
    return 0; // Ignore, but mark as handled
  }

  switch (TCODE)
  {
    case NEXUS_TCODE_DirectBranch:
      {
        NEX_FLDGET(ICNT);
        doneICNT = EmitICNT(f, ICNT, 0x0, disp);
        if (doneICNT < 0) return doneICNT;
      }
      break;

    case NEXUS_TCODE_IndirectBranch:
      {
        // NEX_FLDGET(BTYPE); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(UADDR);
        doneICNT = EmitICNT(f, ICNT, 0x0, disp);
        if (doneICNT < 0) return doneICNT;
        nexdeco_lastAddr = CalculateAddr(UADDR, 0, nexdeco_lastAddr);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_ProgTraceSync:
      {
        // NEX_FLDGET(SYNC); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(FADDR);

        doneICNT = EmitICNT(f, ICNT, 0x0, disp);
        if (doneICNT < 0) return doneICNT;
        nexdeco_lastAddr = CalculateAddr(FADDR, 1, nexdeco_lastAddr);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_DirectBranchSync:
      {
        // NEX_FLDGET(SYNC); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(FADDR);
        doneICNT = EmitICNT(f, ICNT, 0x0, disp);
        if (doneICNT < 0) return doneICNT;
        nexdeco_lastAddr = CalculateAddr(FADDR, 1, nexdeco_lastAddr);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_IndirectBranchSync:
      {
        // NEX_FLDGET(SYNC); // We ignore this for now
        // NEX_FLDGET(BTYPE); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(FADDR);
        doneICNT = EmitICNT(f, ICNT, 0x0, disp);
        if (doneICNT < 0) return doneICNT;
        nexdeco_lastAddr = CalculateAddr(FADDR, 1, nexdeco_lastAddr);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_IndirectBranchHist:
      {
        // NEX_FLDGET(BTYPE); // We ignore this for now
        NEX_FLDGET(ICNT);
        NEX_FLDGET(UADDR);
        NEX_FLDGET(HIST);

        doneICNT = EmitICNT(f, ICNT, HIST, disp);
        if (doneICNT < 0) return doneICNT;
        nexdeco_lastAddr = CalculateAddr(UADDR, 0, nexdeco_lastAddr);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_IndirectBranchHistSync:
      {
        // NEX_FLDGET(SYNC); // We ignore this for now
        // NEX_FLDGET(BTYPE); // We ignore this for now
        // NEX_FLDGET(CANCEL); // Not used
        NEX_FLDGET(ICNT);
        NEX_FLDGET(FADDR);
        NEX_FLDGET(HIST);

        doneICNT = EmitICNT(f, ICNT, HIST, disp);
        if (doneICNT < 0) return doneICNT;
        nexdeco_lastAddr = CalculateAddr(FADDR, 1, nexdeco_lastAddr);
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;

    case NEXUS_TCODE_ResourceFull:
      {
        NEX_FLDGET(RCODE);
        if (RCODE == 1 || RCODE == 2)
        {
          // Determine repeat count (for RCODE=2)
          unsigned int hRepeat = 1;
          NEX_FLDGET(RDATA);
          if (RCODE == 2)
          {
            NEX_FLDGET(HREPEAT);
            hRepeat = HREPEAT;
          }
          
          if (RDATA > 1)
          {
            // Special calling to emit HIST only ...
            if (dispHistRepeat)
            {
              if (disp & 4) printf("RepeatHIST,0x%lX,%d\n", RDATA, dispHistRepeat);            
              dispHistRepeat = 0;
            }
            do
            {
              // ICNT is unknown (-1), what will process only HIST bits
              doneICNT = EmitICNT(f, -1, RDATA, disp);
              if (doneICNT < 0) return doneICNT;

              resourceFull_ICNT -= doneICNT;  // Consume, so next time ICNT will be adjusted
              hRepeat--;
            } while (hRepeat > 0);
          }
        }
        else
        if (RCODE == 0)
        {
          // This is I-CNT overflow
          NEX_FLDGET(RDATA);

          resourceFull_ICNT += (int)RDATA;  // Accumulate, so next time ICNT will be adjusted
        }
      }
      break;

    case NEXUS_TCODE_ProgTraceCorrelation:
      {
        NEX_FLDGET(EVCODE);
        NEX_FLDGET(CDF);
        NEX_FLDGET(ICNT);
        if (CDF == 1)
        {
          NEX_FLDGET(HIST);
          doneICNT = EmitICNT(f, ICNT, HIST, disp);
          if (doneICNT < 0) return doneICNT;
        }
        else
        {
          // No history ...
          doneICNT = EmitICNT(f, ICNT, 0, disp);
          if (doneICNT < 0) return doneICNT;
        }
        nexdeco_pc = nexdeco_lastAddr;
      }
      break;


    case NEXUS_TCODE_Error:
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
int NexusDeco(FILE *f, int disp)
{
  int fldDef = -1;
  int fldBits = 0;
  Nexus_TypeField fldVal = 0;

  int msgCnt = 0;
  int msgBytes = 0;
  int msgErrors = 0;

  // Make sure decoder is using real call-stack ...
  if (conf_CallStack < 0)
  {
    conf_CallStack = -conf_CallStack;
  }
  CallStack_Init();

  msgFieldCnt = 0;  // No fields

  unsigned char msgByte = 0;
  unsigned char prevByte = 0;
  for (;;)
  {
    prevByte = msgByte;
    if (fread(&msgByte, 1, 1, fNex) != 1) break;  // EOF

/*
 
. 0x70 011100_00: TCODE[6]=28 (MSG #41) - IndirectBranchHist
. 0x10 000100_00: BTYPE[2]=0x0
. 0x49 010010_01: ICNT[10]=0x121
. 0x70 011100_00:
. 0x5D 010111_01: UADDR[12]=0x5DC
. 0xF4 111101_00:
. 0xFC 111111_00:
. 0xFC 111111_00:
. 0xFF 111111_11: HIST[24]=0xFFFFFD

 */     

#if 0 // Some debug code (it make one of tests fail!)
      if (1 && msgCnt == 42 && prevByte == 0xFC && msgByte == 0xFF)
      {
        msgByte = 0x6b;
      }
#endif      

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

      // TODO: Convert to look-up table (for all TCODE values)
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
        savedFields[0] = msgFieldPos;
        savedFields[1] = msgFieldCnt;
        savedFields[2] = msgFields[0];
        savedFields[3] = msgFields[1];
        savedFields[4] = msgFields[2];
        savedFields[5] = msgFields[3];
        savedFields[6] = msgFields[4];
      }

      // Save to allow later decoding
      msgFieldPos = fldDef;
      msgFieldCnt = 0;
      msgFields[msgFieldCnt++] = mdo;

      if (disp & 3) printf(" TCODE[6]=%d (MSG #%d) - %s\n", mdo, msgCnt, nexusMsgDef[fldDef].name);
      msgCnt++;
      msgBytes++;

      if (mdo == NEXUS_TCODE_Error) msgErrors++;

      fldDef++;
      fldBits = 0;
      fldVal = 0;
      continue;
    }

    // Accumulate 'mdo' to field value
    fldVal |= (((Nexus_TypeField)mdo) << fldBits);
    fldBits += 6;

    msgBytes++;

    // Process fixed size fields (there may be more than one in one MDO record)
    while (nexusMsgDef[fldDef].def & 0x200)
    {
      int fldSize = nexusMsgDef[fldDef].def & 0xFF;
      if (fldSize & 0x80)
      {
        // Size of this field is defined by parameter ...
        fldSize = 2;
      }
      if (fldBits < fldSize)
      {
        break;  // Not enough bits for this field
      }

      msgFields[msgFieldCnt++] = fldVal & ((((Nexus_TypeField)1) << fldSize) - 1); // Save field

      if (disp & 1) printf(" %s[%d]=0x%lX", nexusMsgDef[fldDef].name, fldSize, fldVal & ((((Nexus_TypeField)1) << fldSize) - 1));
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
      if (disp & 1) printf(" %s[%d]=0x%lX\n", nexusMsgDef[fldDef].name, fldBits, fldVal);

      msgFields[msgFieldCnt++] = fldVal; // Save field

      if (mseo == 3)
      {
        int cnt = 1;

        dispHistRepeat = 0; 

        if (msgFields[0] == NEXUS_TCODE_RepeatBranch)
        {
          // Special handling for repeat branch (which only has 1 field!)
          cnt = msgFields[1]; // Counter set in RepeatBranch message
          msgFieldPos = savedFields[0]; // Restrore previous message (saved)
          msgFieldCnt = savedFields[1];
          msgFields[0] = savedFields[2];
          msgFields[1] = savedFields[3];
          msgFields[2] = savedFields[4];
          msgFields[3] = savedFields[5];
          msgFields[4] = savedFields[6];
          dispHistRepeat = cnt;
        }

        while (cnt > 0) // Handle (1 or many times ...)
        {
          int err = MsgHandle(f, disp);
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
    printf("Stat: %d bytes, %d messages, %d error messages", msgBytes, msgCnt, msgErrors);
    if (msgCnt > 0) printf(", %.2lf bytes/message", ((double)msgBytes) / msgCnt);
    if (nInstr > 0) printf(", %d instr, %.3lf bits/instr", nInstr, ((double)msgBytes * 8) / nInstr);
    printf("\n");
  }

  return nInstr; // Number of instructions generated
}

//****************************************************************************
// End of NexRvDeco.c file
