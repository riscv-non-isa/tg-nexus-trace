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
// File NexRvDump.c  - Nexus RISC-V Trace dumper

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

// Decoder works on two files and dumper on first file
extern FILE *fNex; // Nexus messages (binary bytes)

// Dump all Nexus messages (from 'fNex' file)
//  disp  - display options bit-mask (1-packets, 2-only TCODE+names, 4-summary)
int NexusDump(FILE *f, int disp)
{
  int fldDef  = -1;          
  int fldBits = 0;          
  Nexus_TypeField fldVal = 0;

  int msgCnt    = 0;
  int msgBytes  = 0;
  int msgErrors = 0;
  int idleCnt   = 0;

  unsigned char msgByte = 0;
  unsigned char prevByte = 0;
  for (;;)
  {
    prevByte = msgByte;
    if (fread(&msgByte, 1, 1, fNex) != 1) break;  // EOF

#if 1 // This will skip long sequnece of idles (visible in true captures ...)
    if (msgByte == 0xFF && prevByte == 0xFF)
    {
      idleCnt++;
      continue;
    }
#endif

    if (disp & 1)
    { 
      if (msgCnt > 0 && fldDef < 0)
      {
        fprintf(f, "\n");
      }
      fprintf(f, "0x%02X ", msgByte);
      for (int b = 0x80; b != 0; b >>= 1)
      {
        if (b == 0x2) fprintf(f, "_");
        if (msgByte & b) fprintf(f, "1"); else fprintf(f, "0");
      }
      fprintf(f, ":");
    }

    unsigned int mdo  = msgByte >> 2;
    unsigned int mseo = msgByte & 0x3;

    if (mseo == 0x2)
    {
      printf(" ERROR: At offset %d: MSEO='10' is not allowed\n", msgBytes + idleCnt);
      return -1;  // Error return
    }

    if (fldDef < 0)
    {
      if (mseo == 0x3) 
      {
        if (disp & 1) fprintf(f, " IDLE\n");
        idleCnt++;
        continue;
      }

      if (mseo != 0x0)
      {
        printf(" ERROR: At offset %d: Message must start from MSEO='00'\n", msgBytes + idleCnt);
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
        printf(" ERROR: At offset %d: Message with TCODE=%d is not defined for N-Trace\n", msgBytes + idleCnt, mdo);
        return -3;
      }

      if (disp & 3) fprintf(f, " TCODE[6]=%d (MSG #%d) - %s\n", mdo, msgCnt, nexusMsgDef[fldDef].name);
      msgCnt++;
      msgBytes++;

      if (mdo == NEXUS_TCODE_Error) msgErrors++;

      fldDef++;
      fldBits = 0;
      fldVal  = 0;
      continue;
    }

    // Accumulate 'mdo' to field value
    fldVal  |= (((Nexus_TypeField)mdo) << fldBits);
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
      if (disp & 1) fprintf(f, " %s[%d]=0x%lX", nexusMsgDef[fldDef].name, fldSize, fldVal & ((((Nexus_TypeField)1) << fldSize) - 1));
      fldDef++;
      fldVal >>= fldSize;
      fldBits -= fldSize;
    }

    if (mseo == 0x0)
    {
      if (disp & 1) fprintf(f, "\n");
      continue;
    }

    if (nexusMsgDef[fldDef].def & 0x400)
    {
      // Variable size field
      if (disp & 1) fprintf(f, " %s[%d]=0x%lX\n", nexusMsgDef[fldDef].name, fldBits, fldVal);

      if (mseo == 3)
      {
        fldDef = -1;
      }
      else
      {
        fldDef++;
      }
      fldBits = 0;
      fldVal  = 0;
      continue;
    }

    if (fldBits > 0)
    {
      printf(" ERROR: At offset %d: Not enough bits for non-variable field\n", msgBytes + idleCnt);
      return -4;
    }
  }

  if (disp & 4)
  {
    printf("\nStat: %d bytes, %d idles, %d messages, %d error messages", msgBytes, idleCnt, msgCnt, msgErrors);
    if (msgCnt > 0) printf(", %.2lf bytes/message", ((double)msgBytes) / msgCnt);
    printf("\n");
  }

  return msgCnt; // Number of messages handled
}

//****************************************************************************
// End of NexRvDump.c file
