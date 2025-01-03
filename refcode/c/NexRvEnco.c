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

extern int conf_Repeat;

#if 1 // Callstack related
extern int conf_CallStack;
extern void CallStack_Init();
extern void CallStack_Push(Nexus_TypeAddr ret);
extern Nexus_TypeAddr CallStack_Pop();

static unsigned int checkRetNext = 1; // Impossible to match with real-pc
#endif

static int encoStat_MsgBytes  = 0;
static int encoStat_MsgCnt    = 0;
static int encoStat_InstrCnt  = 0;

static unsigned int encoNextEmit = 0;
static unsigned int encoICNT;
static Nexus_TypeHist encoHIST;
static Nexus_TypeAddr encoADDR;
static unsigned int encoBCNT;

static unsigned int prevICNT;
static Nexus_TypeHist prevHIST;

static int AddVar(Nexus_TypeField v, int nPrev, unsigned char *msg, int pos)
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

// *********************************************************
// A little bit HIST pattern detection (maybe ***)
//
//  1_prev[30..0]
//  1_hist[30..0]   ==> if (prev == hist) ==> count = 1, no emit
//                  ==> if prev[29..0] == ((hist[28..0] << 1) | prev[30]) && (hist[31..29] == prev[1..0]) ==> histStop = 30,
//                  ==> if prev[27..0] == ((hist[28..0] << 1) | prev[30]) && (hist[31..29] == prev[1..0]) ==>
//
//    Maybe for later
// *********************************************************

// This is how HIST patterns are detected
//  Overflow is generated with 32-bit HIST
//  First it is stored (as 32-bit number)
//  If next overflow is coming, we compare it. If the same, we start counting.
//  If different and this is first time, we try to see if we have 30-bit of 28-bit match.
//  30=2*3*5        divisors: 1,2,3,5,6,10,15,30
//  28=2*2*7        divisors: 1,2,4,7,14,28
//              all divisors: 1,2,3,4,5,6,7,10,14,15,28,30 (all numbers 1-7!)
//
// NOTE: 31 HIST bits is most efficient (for size of RCODE=4 when there is no repeated pattern):
//
//  Byte  31-bit:             30-bit  28:bit
//  0:    TTTTTT_00 - TCODE   TTTTTT  TTTTTT
//  1:    HHRRRR_00 - RCODE   hhRRRR  hhRRRR
//  2:    HHHHHH_00 - HIST    hhhhhh  hhhhhh
//  3:    HHHHHH_00 - HIST    hhhhhh  hhhhhh
//  4:    HHHHHH_00 - HIST    hhhhhh  hhhhhh
//  5:    HHHHHH_00 - HIST    hhhhhh  hhhhhh
//  6:    1HHHHH_00 - HIST    *1hhhh  ***1hh
//                    CNT     nnnnnn  nnnnnn
//
// Other sizes (30/28) are wasting bits, so should be only used when repeated.

static int            histRepeat_Bits   = 0;  // Can be 0 or 31 or 30 or 28
static Nexus_TypeHist histRepeat_Prev   = 0;  // Pattern to compare (with stop-bit!)
static int            histRepeat_Shift  = 0;  // Shift before we compare (0,1,3)

// This function is detecting 30/28 bit pattern in two consecutive 32-bit HIST records 
//
//  It is a bit mind-twisting as first bit is on MSB!
//  First version of code was on LSB and it did not work.
//
// 32-    3 322           ... 
// bit#   1 098               7654 3210
//
//  Here are patterns for 30-bit:
// HIST                 1     2222 2223                  
// Bit#:    012 3456 7890     3456 7890
//        1_aaa_aaaa_aaaa_..._aaaa_aaaB  <- prev32 (<a:30> is 30-bit value)
//          xyy_yyyy_yyyy_..._yyyy_yyyX   <- compare 'x' and 'X'
//        1_BBB_BBBB_BBBB_..._BBBB_BBcc  <- hist32 (<B:30> is second 30-bit), <c:2> is left over
//          YYY_YYYY_YYYY_..._YYYY_YY     <- compare 'y..y' and 'Y..Y'
//
//  Here are patterns for 28-bit:
//
// HIST                 1     2222 2223                  
// Bit#:    012 3456 7890     3456 7890
//        1_aaa_aaaa_aaaa_..._aaaa_aBBB  <- prev32 (<a:28> is 28-bit value)
//          XXX_yyyy_yyyy_..._yyyy_yXXX
//          xxx_yyyy_yyyy_..._yyyy_yXXX   <- compare 'xxx' and 'XXX'
//        1_BBB_BBBB_BBBB_....BBcc_cccc  <- hist32 (<B:28> is second 28-bit), <c:6> is left over
//          YYY_YYYY_YYYY_..._YY          <- compare 'y..y' and 'Y..Y'

static int IsHistMatch_30_28(unsigned int prev32, unsigned int hist32)
{
  // This is complex in SW, but 'easy' in logic ...

#if 1 // Allow 28-bit pattern 
  if (((prev32 & 7) == ((prev32 >> 28) & 7)) &&                 // Is prev[28..30] == prev[0..2]  - 3-bit compare (first 3 bits)
      ((prev32 & 0x0FFFFFF8) == ((hist32 >> 3) & 0x0FFFFFF8)))  // Is prev[3..27] == hist[0..24]  - 25-bit compare (remaining 25 bits)
  {
    return 28;  // 28 bits match, so 6 bits in 'cc'
  }
#endif

#if 0 // This may be faster (28-bit pattern) 
  if (((prev32 ^ (prev32 >> 28)) & 7) == 0 &&         // Is prev[28..30] == prev[0..2]  - 3-bit compare (first 3 bits)
      ((prev32 ^ (hist32 >> 3)) & 0x0FFFFFF8) == 0)   // Is prev[3..27] == hist[0..24]  - 25-bit compare (remaining 25 bits)
  {
    return 28;  // 28 bits match, so 6 bits in 'cc'
  }
#endif

#if 1 // Allow 30-bit pattern 
  if (((prev32 & 1) == ((prev32 >> 30) & 1)) &&                 // Is prev[30] == prev[0]         - 1-bit compare (first bit)
      ((prev32 & 0x3FFFFFFE) == ((hist32 >> 1) & 0x3FFFFFFE)))  // Is prev[1..29] == hist[0..28]  - 29-bit compare (remaining 29 bits)
  {
    return 30;  // 30 bits match, so 2 bits in 'cc'
  }
#endif

  return 0;     // Not a match
}

// Handle retired instruction (it may be implemented as HW pipeline)
static int HandleRetired(Nexus_TypeAddr addr, unsigned int info, int level, int disp)
{
  if (disp & 2) printf("Enco: pc=0x%lX,info=0x%X\n", addr, info);

  if (encoStat_MsgBytes == 0)
  {
    // Initialize encoder state
    encoICNT = 0;
    encoHIST = 1;
    encoADDR = 0;
    encoBCNT = 0;

    prevICNT = 0;
    prevHIST = 0; // Will never match ...

    encoNextEmit = NEXUS_TCODE_ProgTraceSync;
  }

  if (conf_CallStack != 0 && (checkRetNext & 1) == 0)
  {
    if (conf_CallStack < 0 || (checkRetNext == addr))
    {
      if (0) printf("CallMatch: 0x%lX\n", addr);
      encoNextEmit = 0; // Return address is matching, so emit nothing
    }
    checkRetNext = 1;   // This is one time deal
  }

  if (info == 0 && (encoICNT > 0 || histRepeat_Bits != 0))  // Flush requested
  {
    if (encoNextEmit == 0) encoNextEmit = NEXUS_TCODE_ProgTraceCorrelation;
    if (encoBCNT > 0)
    {
      prevHIST = 0; // Make sure repeat will be generated
    }
  }

  if (encoNextEmit != 0)
  {
    if (disp & 8) printf("Enco: EMIT=%d, hist=0x%X, encoICNT=%d\n", encoNextEmit, encoHIST, encoICNT);


    unsigned char msg[40];
    int  pos = 0;

    if (level >= 21)  // This piece of code detects and generates Repeat Branch message
    {
      int repeatNow = 0;

      // Detect 24/30-bit ResourceFull pattern match      
      if ((conf_Repeat & 2)  && encoNextEmit == NEXUS_TCODE_ResourceFull)
      {
        // if (1) printf("Enco: FULL(%d) = 0x%X\n", histRepeat_Bits, encoHIST);
        if (histRepeat_Bits == 0)
        {
          // This is first time ...
          histRepeat_Bits   = 31; // NEXUS_HIST_BITS;
          histRepeat_Shift  = 0;  // No shift
          histRepeat_Prev   = encoHIST;   // TODO: We could re-use 'prevHIST' ...

          encoBCNT          = 0;  // Reset counter
          repeatNow         = 1;  // Will not generate message (but increase counter)

          encoHIST          = 1;  // We consumed all HIST bits
        }
        else
        if ((encoHIST >> histRepeat_Shift) == histRepeat_Prev)
        {
          // It is matching previous pattern (must be repeated)
          // Keep HIST LSB bits
          encoHIST = (((Nexus_TypeHist)1u) << histRepeat_Shift) | (encoHIST & ((((Nexus_TypeHist)1u) << histRepeat_Shift) - 1u));
          repeatNow = 1;                  // Will not generate message (but increase counter)
        }
        else
        {
          // This is not matching ...
          if (encoBCNT == 1)
          {
            // This is second 31-bit HIST - we may want to trim it to 30 or 28 bits ...
            int match = IsHistMatch_30_28(histRepeat_Prev, encoHIST);
            if (match != 0)
            {
              // We have 30 or 28-bit match (and NOT 31-bit match)
              histRepeat_Bits = match; // NEXUS_HIST_BITS;
              histRepeat_Shift = (31 - match);      // This will be 1 or 3
              histRepeat_Prev >>= histRepeat_Shift; // Remove 1 or 3 LSB bits (these match)

              // Keep HIST 'histRepeat_Shift' LSB bits
              encoHIST = (((Nexus_TypeHist)1u) << (2 * histRepeat_Shift)) | (encoHIST & ((((Nexus_TypeHist)1u) << (2 * histRepeat_Shift)) - 1u));

              repeatNow = 1;                  // Will not generate message (but increase counter)

              if (0) printf("Enco: MATCH%d = 0x%X\n", histRepeat_Bits, histRepeat_Prev);
            }
            else
            {
              // Second word is not matching (it will be handled below)
            }
          }
          else
          {
            // This word is not matching (it will be handled below)
          }
        }
      }
      else
      // Repeat of 'IndirectBranchHistory' (and Direct/IndirectBranch as well)
      if ((conf_Repeat & 1) && (encoNextEmit != NEXUS_TCODE_ResourceFull) && (prevHIST == encoHIST) && (encoADDR == addr) && (prevICNT == encoICNT))
      {
        // IndirectBranchHistory message back to same address
        repeatNow = 1;
        encoICNT = 0; // Reset, so new one can be generated ...
        encoHIST = 1;
      }

      if (repeatNow)
      {
        encoBCNT++;
        encoNextEmit = 0;  // Will be skipped below
        // if (1) printf("Enco: FULL_AFTER(%d*%d) = LEFT = 0x%X\n", encoBCNT, histRepeat_Bits, encoHIST);
      }
      else
      {
        // Message will NOT be repeated
#if 1       
        if (encoBCNT > 0 && histRepeat_Bits != 0)
        {
          // We must emit previous
          msg[pos++] = NEXUS_TCODE_ResourceFull << 2;
          if (encoBCNT > 1)
          {
            msg[pos++] = 0x2 << 2; // RCODE:N=2 (HIST full with REPEAT)
          }
          else
          {
            msg[pos++] = 0x1 << 2; // RCODE:N=0 (HIST full)
          }
          pos = AddVar(histRepeat_Prev, 6 - NEXUS_FLDSIZE_RCODE, msg, pos);

          if (encoBCNT > 1)
          {
            pos = AddVar(encoBCNT, 0, msg, pos);  // Repeat count ...
          }
          msg[pos - 1] |= 3; // Set MSEO='11' at last byte

          // if (1) printf("Enco: FULL_EMIT(%d) = 0x%X\n", histRepeat_Bits, histRepeat_Prev);

          encoStat_MsgCnt++;

          // Make sure this one will be compared next time
          if (encoNextEmit == NEXUS_TCODE_ResourceFull)
          {
            histRepeat_Bits   = 31; // NEXUS_HIST_BITS;
            histRepeat_Shift  = 0;  // No shift
            histRepeat_Prev   = encoHIST;   // TODO: We could re-use 'prevHIST' ...

            encoBCNT          = 1;  // Count this message

            encoNextEmit      = 0;     // Will be skipped below
            encoHIST          = 1;  // Full history collection starts
          }
          else
          {
            encoBCNT        = 0;  // No more 'repeat' (we handle it above)
            histRepeat_Bits = 0;  // Some other message flushed ResourceFull (it will be appended)
          }
        }
        else
#endif
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
      // if (1) printf("Enco: FULL_AFTER(%d*%d)=0x%X, HIST=0x%X\n", histRepeat_Bits, encoBCNT, histRepeat_Prev, encoHIST);
    }

#if 1 // A bit of improvement (saves 1 byte)
    if (encoNextEmit == NEXUS_TCODE_IndirectBranchHist && encoHIST == 0x1)
    {
      encoNextEmit = NEXUS_TCODE_IndirectBranch; // Empty history ...
    }
#endif

    if (encoBCNT == 0)
    {
      prevHIST = 0; // Only messages which should be repeated will set it
    }

    if (encoNextEmit != 0)
    {
      msg[pos++] = encoNextEmit << 2;
    }

    if (encoNextEmit == NEXUS_TCODE_ProgTraceSync)
    {
      msg[pos++] = 0x1 << 2;  // SYNC:4=1 (always)
      pos = AddVar(encoICNT, 6 - 4, msg, pos);
      encoICNT = 0; // Reset after sending
      pos = AddVar(addr >> NEXUS_PARAM_AddrSkip, 0, msg, pos);
      encoADDR = addr;  // This is new address
    }
    else if (encoNextEmit == NEXUS_TCODE_IndirectBranchHist || encoNextEmit == NEXUS_TCODE_IndirectBranch)
    {
      prevHIST = encoHIST;  // Save to check for repeat next time ...
      prevICNT = encoICNT;

      msg[pos++] = 0x0 << 2;  // BTYPE:2=0 (always)
      pos = AddVar(encoICNT, 6 - 2, msg, pos);
      encoICNT = 0; // Reset after sending
      pos = AddVar((encoADDR ^ addr) >> NEXUS_PARAM_AddrSkip, -1, msg, pos);
      encoADDR = addr;  // This is new address

      if (encoNextEmit == NEXUS_TCODE_IndirectBranchHist)
      {
        pos = AddVar(encoHIST, 0, msg, pos);
      }
    }
    else if (encoNextEmit == NEXUS_TCODE_DirectBranch)
    {
      pos = AddVar(encoICNT, -1, msg, pos);
      encoICNT = 0; // Reset after sending
    } else if (encoNextEmit == NEXUS_TCODE_ResourceFull)
    {
      prevHIST = encoHIST;  // Save to check for repeat next time ...
      prevICNT = encoICNT;

      // TODO See if this is ICNT or HIST. For now only HIST is handled
      if (NEXUS_FLDSIZE_RCODE != 0)
      {
        msg[pos++] = 0x1 << 2; // RCODE:N=1 (HIST full)
      }
      pos = AddVar(encoHIST, 6 - NEXUS_FLDSIZE_RCODE, msg, pos);
    } else if (encoNextEmit == NEXUS_TCODE_ProgTraceCorrelation)
    {
      msg[pos++] = (0x0 << 2);         // EVCODE:4=0 (debug)
      if (encoHIST > 1)
      {
        msg[pos - 1] |= 1 << (4 + 2); // CDF=1
      }
      pos = AddVar(encoICNT, -1, msg, pos);
      encoICNT = 0; // Reset after sending
      if (encoHIST > 1)
      {
        pos = AddVar(encoHIST, 0, msg, pos);
      }
    }

    if (pos > 1)
    {
      msg[pos - 1] |= 3; // Set MSEO='11' at last byte
      
      if (fwrite(msg, 1, pos, fNex) != pos) return -1;
      encoStat_MsgBytes += pos;
      encoStat_MsgCnt++;
    }

    encoNextEmit = 0;   // Only one time
    if (histRepeat_Bits == 0)
    {
      encoHIST = 1; // histRepeat handle 'encoHist' differently ... 
    }
    // encoICNT = 0;
  }

  // This is key state update (ICNT and HIST fields)
  encoICNT += (info & INFO_4) ? 2 : 1;

  if (info & INFO_BRANCH)
  {
    if (level >= 20)
    {
      if (info & INFO_LINEAR) encoHIST = (encoHIST << 1) | 0; // Not taken jump
      else                    encoHIST = (encoHIST << 1) | 1; // Taken jump

      if (encoHIST & (((Nexus_TypeHist)1u) << NEXUS_HIST_BITS))
      {
        encoNextEmit = NEXUS_TCODE_ResourceFull;
      }
    }
    else
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
  }
  else if (info & INFO_INDIRECT)
  {
    if (conf_CallStack != 0 && (info & INFO_RET))
    {
      checkRetNext = CallStack_Pop(); // We will check this address on next instruction
    }

    if (level >= 20)
      encoNextEmit = NEXUS_TCODE_IndirectBranchHist;  // Emit on next address time ...
    else
      encoNextEmit = NEXUS_TCODE_IndirectBranch;      // Emit on next address time ...
  }
  else
  {
    // Nothing special (just ICNT updated)
  }

  if (conf_CallStack != 0 && (info & INFO_CALL))
  {
    // This is call (direct or indirect). Push address after this 'jal[r]) to the stack
    Nexus_TypeAddr ret = addr + ((info & INFO_4) ? 4 : 2);
    CallStack_Push(ret);
  }

  return 0; // OK
}

int NexusEnco(FILE *f, int level, int disp)
{
  encoStat_MsgBytes = 0;
  encoStat_MsgCnt   = 0;
  encoStat_InstrCnt = 0;

  CallStack_Init();

  printf("NexusEnco(level=%d, ...)\n", level);

  Nexus_TypeAddr a;
  unsigned int info;
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
    if (encoStat_InstrCnt > 0) printf(", %.3lf bits/instr", ((double)encoStat_MsgBytes * 8) / encoStat_InstrCnt);
    printf("\n");
  }

  return encoStat_MsgCnt;
}

//****************************************************************************
// End of NexRvEnco.c file
