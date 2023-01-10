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
// File NexRvInfo.c - Nexus RISC-V instruction info access

// Code below is written in plain C-code.
// It was compiled using VisualC, GNU and IAR C/C++ compiler.
//  1. Only standard C-types are used.
//  2. Only few standard C functions used - see notes with "#include <...>"
//  3. Only non K&R C is 'for (int x' and 'int x;' between instructions.

#include <stdio.h>  //  For NULL, 'printf', 'fopen, ...'
#include <stdlib.h> //  For 'malloc', 'free'
#include <string.h> //  For 'strcmp', 'strchr' 
//#include <ctype.h>  //  For 'isspace/isxdigit' etc.

#include "NexRvInfo.h"  //  Definition of Nexus messages

int InfoParse(const char *t, unsigned int *pAddr, unsigned int *pInfo, unsigned int *pDest);

static FILE *fInfo = NULL;     // Instruction info (text-based records)
static unsigned int prevAddr = 0xFFFFFFFF;

typedef struct INFO_REC
{
  unsigned int addr;      // Address of instruction
  unsigned int info;      // INFO for this instruction
  unsigned int dest;      // Destination address
  unsigned int _padding;  // Make it 64-bit aligned
} INFO_REC;

static int nInfoRec = 0;
static int infoLast = 0;
static INFO_REC *pInfoRec;

int InfoInit(const char *filename)
{
  prevAddr = 0xFFFFFFFF;
  fInfo = fopen(filename, "rt");
  if (fInfo == NULL) return -1; // Failed

  nInfoRec = 0;
  while (pInfoRec == NULL)      // Will run twice
  {
    if (nInfoRec > 0) // Allocate (second time ...)
    {
      pInfoRec = malloc(sizeof(INFO_REC) * nInfoRec);
    }

    nInfoRec = 0;
    fseek(fInfo, 0, SEEK_SET); // Rewind file
    char line[1000];
    while (fgets(line, sizeof(line), fInfo) != NULL)
    {
      if (line[0] == '.' && line[1] == 'e') break; // End
      if (line[0] == '.') continue; // Comment (ignore this line)
      if (line[0] == '\0' || line[0] == '\n') continue; // Ignore empty as well ...

      unsigned int a, info, dest;
      if (!InfoParse(line, &a, &info, &dest)) break;
      if (info == 0) break;

      if (pInfoRec != NULL)
      {
        pInfoRec[nInfoRec].addr = a;
        pInfoRec[nInfoRec].info = info;
        pInfoRec[nInfoRec].dest = dest;
        pInfoRec[nInfoRec]._padding = 0;
      }

      nInfoRec++;
    }

    if (nInfoRec == 0)
    {
      break;  // No records
    }
  }

  infoLast = 0;
  return 0; // OK
}

void InfoTerm(void)
{
  if (fInfo != NULL) fclose(fInfo);
  fInfo = NULL;
  if (pInfoRec) free(pInfoRec);
  pInfoRec = NULL;
  nInfoRec = 0;
  infoLast = 0;
}

int InfoParse(const char *t, unsigned int *pAddr, unsigned int *pInfo, unsigned int *pDest)
{
  if (sscanf(t, "0x%X", pAddr) != 1) return 0; // Syntax error

  t = strchr(t, ',');
  if (t == NULL) { *pInfo = 0; return 1; }
  t++;

  // We have <t><s>,<t>I<s>|<t>D<s> - convert text to bit-mask
  unsigned int info = 0;
  if (t[0] == 'L')  info = INFO_LINEAR;
  if (t[0] == 'B')  info = INFO_BRANCH;
  if (t[0] == 'J')  info = INFO_JUMP;
  if (t[0] == 'C')  info = INFO_JUMP | INFO_CALL;
  if (t[0] == 'R')  info = INFO_JUMP | INFO_INDIRECT | INFO_RET;

  if (info == 0) { *pInfo = 0; return 2; }; // At least one of above must be given

  if (t[1] == 'N')  info |= INFO_LINEAR;    // Non-executed (only BN)
  if (t[1] == 'I')  info |= INFO_INDIRECT;
  if (t[1] == '4')  info |= INFO_4;
  if (t[2] == '4')  info |= INFO_4;

  *pInfo = info;

  t = strchr(t, ','); // Destination addr is after second ','
  if (t != NULL && pDest != NULL)
  {
    if (sscanf(t + 1, "0x%X", pDest) != 1) return 0; // Syntax error
  }
  return 3;
}

unsigned int InfoGet(unsigned int addr, unsigned int *pDest)
{
  if (pInfoRec != NULL)
  {
    if (addr < pInfoRec[infoLast].addr)
    {
      // Look before ...
      for (int i = infoLast - 1; i >= 0; --i)
      {
        if (pInfoRec[i].addr == addr)
        {
          infoLast = i;
          if (pDest) *pDest = pInfoRec[i].dest;
          return pInfoRec[i].info;
        }
      }
    }
    else
    {
      // Look after ...
      for (int i = infoLast; i < nInfoRec; ++i)
      {
        if (pInfoRec[i].addr == addr)
        {
          infoLast = i;
          if (pDest) *pDest = pInfoRec[i].dest;
          return pInfoRec[i].info;
        }
      }
    }

    return 0; // Not found ...
  }

  if (fInfo != NULL)
  {
    if (addr <= prevAddr) fseek(fInfo, 0, SEEK_SET); // Rewind file (if not forward)
    prevAddr = addr;  // Save for next call (to avoid 'fseek')

    char line[1000];
    while (fgets(line, sizeof(line), fInfo) != NULL)
    {
      if (line[0] == '.' && line[1] == 'e') break; // End
      if (line[0] == '.') continue; // Comment (ignore this line)
      if (line[0] == '\0' || line[0] == '\n') continue; // Ignore empty as well ...

      unsigned int a, info;
      if (!InfoParse(line, &a, &info, pDest)) break;
      if (info == 0) break;
      if (a == addr) return info; // Found
    }
    prevAddr = 0xFFFFFFFF;
  }
  return 0; // Error (=0)
}

//****************************************************************************
// End of NexRvInfo.c file
