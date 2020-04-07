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
int NexusDump(int disp)
{
  int fldDef  = -1;          
  int fldBits = 0;          
  unsigned int fldVal = 0;

  int msgCnt    = 0;
  int msgBytes  = 0;
  int msgErrors = 0;

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
        printf("\n");
      }
      printf("0x%02X ", msgByte);
      for (int b = 0x80; b != 0; b >>= 1)
      {
        if (b == 0x2) printf("_");
        if (msgByte & b) printf("1"); else printf("0");
      }
      printf(":");
    }

    unsigned int mdo  = msgByte >> 2;
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

      if (disp & 3) printf(" TCODE[6]=%d (MSG #%d) - %s\n", mdo, msgCnt, nexusMsgDef[fldDef].name);
      msgCnt++;
      msgBytes++;

      if (mdo == NEXUS_TCODE_ResourceFull || mdo == NEXUS_TCODE_Error) msgErrors++;

      fldDef++;
      fldBits = 0;
      fldVal  = 0;
      continue;
    }

    // Accumulate 'mdo' to field value
    fldVal  |= (mdo << fldBits);
    fldBits += 6;

    msgBytes++;

    // Process fixed size fields (there may be more than one in one MDO record)
    while ((nexusMsgDef[fldDef].def & 0x200) && fldBits >= (nexusMsgDef[fldDef].def & 0xFF))
    {
      int fldSize = nexusMsgDef[fldDef].def & 0xFF;
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
      printf(" ERROR: Not enough bits for non-variable field\n");
      return -4;
    }
  }

  if (disp & 4)
  {
    printf("\nStat: %d bytes, %d messages, %d error messages", msgBytes, msgCnt, msgErrors);
    if (msgCnt > 0) printf(", %.2lf bytes/message", ((double)msgBytes) / msgCnt);
    printf("\n");
  }

  return msgCnt; // Number of messages handled
}

#if 0

static int error(const char *err)
{
  printf("\n");
  printf("NexRvDump ERROR: %s\n", err);
  return 9;
}

static int usage(const char *err)
{
  if (err != NULL)
  {
    error(err);
  }
  printf("\n");
  printf("Usage:\n");
  printf("  NexRvDump <nex> [-all|-msg|-stat|-none] - dump Nexus file\n");
  return 1;
}

int main(int argc, char *argv[])
{
  if (argc < 2) return usage(NULL);

  fNex = fopen(argv[1], "rb");
  if (fNex == NULL) return error("Cannot open nex-file");

  int disp = 4 | 2 | 1; // Default (-all)
  if (argc > 2 && strcmp(argv[2], "-all") == 0)   disp = 4 | 2 | 1; // All
  if (argc > 2 && strcmp(argv[2], "-msg") == 0)   disp = 4 | 2;     // TCODE and stat.
  if (argc > 2 && strcmp(argv[2], "-stat") == 0)  disp = 4;         // Only statistics
  if (argc > 2 && strcmp(argv[2], "-none") == 0)  disp = 0;         // Nothing

  int ret = NexusDump(disp);
  fclose(fNex); fNex = NULL;

  if (ret <= 0) return error("Nexus Trace dump failed");

  return 0; // All OK
}

#endif

//****************************************************************************
// End of NexRvDump.c file
