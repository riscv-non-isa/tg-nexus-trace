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
// File NexRvConv.c  - Nexus RISC-V Trace converter (to aid decoding)

// Code below is written in plain C-code.
// It was compiled using VisualC, GNU and IAR C/C++ compiler.
//  1. Only standard C-types are used.
//  2. Only few standard C functions used - see notes with "#include <...>"
//  3. Only non K&R C is 'for (int x' and 'int x;' between instructions.

#include <stdio.h>  //  For NULL, 'printf', 'fopen, ...'
#include <stdlib.h> //  For 'exit'
#include <string.h> //  For 'strcmp', 'strchr' 
#include <ctype.h>  //  For 'isspace/isxdigit' etc.

#include "NexRv.h"  //  Common NEXUS_... #define (RISC-V specific subset)

// It converst GNU-objdump file (with -d option) to info-file

// This is example line (<t> denotes TAB) - line must start from space!
// 166:<t>0ba000ef          <t>jal<t>ra, 220 <c_ecall_handler_v2>

static unsigned int GetParAddr(const char *l)
{
  // Skip over opcode (and '/t' following it)
  while (!(*l == '\t' || *l == '\0')) l++;
  if (*l++ != '\t') return 1;

  // First parameter or after last ','
  const char *parAddr = l;
  while (!(*l == '\t' || *l == '\0'))
  {
    if (*l == ',') parAddr = l + 1;
    l++;
  }

  if (!isxdigit(*parAddr)) return 3;

  unsigned int addr;
  if (sscanf(parAddr, "%X", &addr) != 1) return 5;
  return addr;
}

int ConvGnuObjdump(FILE *objdumpFile)
{
  char line[1000];

  int nInstr = 0;
  while (fgets(line, sizeof(line), objdumpFile) != NULL)
  {
    const char *l = line;
    while (isspace(*l)) l++;
    if (!isxdigit(*l))
    {
      continue;
    }

    unsigned int addr;
    if (sscanf(l, "%x", &addr) != 1) return -1;
    while (isxdigit(*l)) l++;
    if (*l++ != ':') continue;
    if (*l++ != '\t') continue;

    int size = 4;
    if (l[4] == ' ') size = 2;
    unsigned int code;
    if (sscanf(l, "%x", &code) != 1) return -21;

    while (!(*l == '\t' || *l == '\0')) l++;
    if (*l++ != '\t') return -22;

    // Here we need to recognize the following opcodes
    //  b??/j/jal (with address)
    //  jalr/jr/ret/mret (without address)

    int disp = 0;
    const char *instr = l;

    if (instr[0] == 'j' || instr[0] == 'b' || instr[0] == 'r' || instr[0] == 'm')
    {
      // j <a> or jal <reg>,<a>
      disp = 0; // For debugging 
    }

    if (disp)
    {
      printf("addr=0x%X,code=0x%X,size=%d,instr=%s\n", addr, code, size, instr);
    }

    // Determine instruction type based on opcode of instruction
    const char *iType = "L";
    if (instr[0] == 'j' || instr[0] == 'b')
    {
      // "j <a>" or "jal <r>,<a>" or "jr <r>" or "jalr <r>"
      // "b?? ...<a>
      if (instr[0] =='j' && instr[1] == 'r')
      {
        // jr does not have an address
        iType = "JI"; // Jump indirect
      }
      else
      if (instr[0] == 'j' && instr[1] == 'a' && instr[3] == 'r')
      {
        // jalr does not have an adrress either
        iType = "CI"; // Call indirect
      }
      else
      {
        if (instr[0] == 'b')                    iType = "BD"; // Branch direct
        if (instr[0] == 'j' && instr[1] != 'a') iType = "JD"; // Jump direct
        if (instr[0] == 'j' && instr[1] == 'a') iType = "CD"; // Call direct
      }
    }
    else
    {
      if (instr[0] == 'm') instr++; // To allow 'mret'
      if (instr[0] == 'r' && instr[1] == 'e' && instr[2] == 't')
      {
        iType = "R";  // This 'ret' or 'mret'
      }
    }

    // Produce output record
    printf("0x%0X,%s%d", addr, iType, size);

    if (iType[1] == 'D')
    {
      // Direct (branch/call/jump) - we need to extract destination address
      // from parameters. Address may be first or after last ','
      unsigned int destAddr = GetParAddr(instr);
      if (destAddr & 1) return -(30 + (int)destAddr);
      printf(",0x%X", destAddr);
    }
    printf("\n");

    nInstr++;
  }

  return nInstr;
}

static int ConvBin4(FILE *fIn, FILE *fOut)
{
  // Flip nibbles (in-place) - it may compress buffer as well, what will speed-up processing

  //  FF  FF
  //  1F  21
  //  72  47
  //  F4  FF
  //  BA  BA 
  //  C7  C7

  //  FF  FF
  //  1F  21
  //  72  47
  //  F4  FF
  //  AF  AB 
  //  CD  CD

  //  FF 
  //  FF
  //  ab
  //  c7
  //  1F
  //  
  int status = 0;

  unsigned char b;
  unsigned char nb;
  int nOk = 0;

  int nWr = 0;
  while (nOk || fread(&b, 1, 1, fIn) == 1)
  {
    if (nOk) { b = nb; nOk = 0; } // Get next (if given)

    if (status == 0)  // We had idles recently
    {
      if (b == 0xFF)
      {
        continue;     // Skip many idle bytes
      }
      if ((b & 0xF) == 0xF)
      {
        status = 1; // This will flip as we have '1F' now
      }
      else
      {
        status = 2; // This is normal-case byte
      }
    }

    if (status == 1)  // Flip
    {
      nOk = 1;
      if (fread(&nb, 1, 1, fIn) != 1) return -1;

      //    MSB                LSB
      b = ((nb & 0xF) << 4) | (b >> 4);
      if ((b & 0x3) == 0x3 && (nb >> 4) == 0xF)
      {
        nb = 0xFF;
        status = 0; // Idle start
      }
    }
    else
    {
      // This is normal byte (not flipped)
      if ((b & 3) == 0x3)
      {
        nOk = 1;
        if (fread(&nb, 1, 1, fIn) != 1) return -1;

        if (nb == 0xFF)
        {
          status = 0;
        }
        else if ((nb & 0xF) == 0xF)
        {
          status = 1;
        }
      }
    }

    if (fwrite(&b, 1, 1, fOut) != 1) return -2;
    nWr++;
  }

  return nWr;
}

//****************************************************************************
// End of NexRvConv.c file
