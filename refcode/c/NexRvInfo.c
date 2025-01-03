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
#include <ctype.h>  //  For 'isspace/isxdigit' etc.
#include <inttypes.h>   //  For scan formats SCNx64

#include "NexRvInfo.h"  //  Definition of Nexus messages

// int InfoParse(const char *t, InfoAddr *pAddr, unsigned int *pInfo, InfoAddr *pDest);

static FILE *fInfo = NULL;     // Instruction info (text-based records)
static InfoAddr prevAddr = 0xFFFFFFFF;

typedef struct INFO_REC
{
  InfoAddr addr;          // Address of instruction
  InfoAddr dest;          // Destination address
  unsigned int  info;     // INFO for this instruction
  unsigned int _padding;  // Make it 64-bit aligned
} INFO_REC;

typedef struct INFO_ADDR
{
  unsigned int  info;     // INFO for this instruction
  InfoAddr      dest;     // Destination address
} INFO_ADDR;

static int nInfoRec = 0;
static int infoLast = 0;
static INFO_REC *pInfoRec   = NULL;

InfoAddr infoAddr_min = 0;
InfoAddr infoAddr_max = 0;
static INFO_ADDR *pInfoAddr  = NULL;

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

      InfoAddr a, dest;
      unsigned int info;
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

#if 1 // Generate table with all INFO for all addresses
  if (pInfoRec != NULL)
  {
    // Get span of addresses ...
    infoAddr_min = pInfoRec[0].addr;
    infoAddr_max = pInfoRec[nInfoRec - 1].addr;

    if ((infoAddr_max - infoAddr_min) <= 3 * (nInfoRec * 4))
    {
      printf("NexRv/Info: amin=0x%lX, amax=0x%lX, nRec=%d\n", infoAddr_min, infoAddr_max, nInfoRec);

      // This is not so big (not more than 3x bigger than original)
      pInfoAddr = malloc(sizeof(INFO_ADDR) * (infoAddr_max - infoAddr_min + 1));

      for (int a = 0; a < (infoAddr_max - infoAddr_min + 1); a++)
      {
        pInfoAddr[a].info = 0;
        pInfoAddr[a].dest = 0;
      }

      for (int i = 0; i < nInfoRec; i++)
      {
        pInfoAddr[pInfoRec[i].addr - infoAddr_min].info = pInfoRec[i].info;
        pInfoAddr[pInfoRec[i].addr - infoAddr_min].dest = pInfoRec[i].dest;
      }
    }
  }
#endif

  infoLast = 0;
  return 0; // OK
}

void InfoTerm(void)
{
  if (fInfo != NULL) fclose(fInfo);
  fInfo = NULL;
  if (pInfoRec) free(pInfoRec);
  pInfoRec = NULL;
  if (pInfoAddr) free(pInfoAddr);
  pInfoAddr = NULL;
  nInfoRec = 0;
  infoLast = 0;
}

int InfoParse(const char *t, InfoAddr *pAddr, unsigned int *pInfo, InfoAddr *pDest)
{
  if (sscanf(t, "%" SCNx64, pAddr) != 1) return 0; // Syntax error

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
    if (sscanf(t + 1, "%" SCNx64, pDest) != 1) return 0; // Syntax error
  }
  return 3;
}

unsigned int InfoGet(InfoAddr addr, InfoAddr *pDest)
{
  if (pInfoAddr != NULL)
  {
    if (addr < infoAddr_min || addr > infoAddr_max)
    {
      return 0; // Incorrect address (no INFO)
    }
    *pDest = pInfoAddr[addr - infoAddr_min].dest;
    return pInfoAddr[addr - infoAddr_min].info;
  }

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

      InfoAddr a;
      unsigned int info;
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
