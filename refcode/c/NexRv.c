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
// File nextr.c  - Nexus Trace dump/encode/decode reference implementation

// Code below is written in plain C-code.
// It was compiled using VisualC, GNU and IAR C/C++ compiler.
//  1. Only standard C-types are used.
//  2. Only few standard C functions used - see notes with "#include <...>"
//  3. Only non K&R C is 'for (int x' and 'int x;' between instructions.

#include <stdio.h>  //  For NULL, 'printf', 'fopen, ...'
#include <stdlib.h> //  For 'exit'
#include <string.h> //  For 'strcmp', 'strchr' 

#include "NexRvInfo.h"  // For 'InfoInit/InfoTerm'

FILE *fNex  = NULL;     // Used by NexusDump/NexusDeco/NexuEnco

extern int NexusDump(int disp);
extern int NexusDeco(int disp);
extern int NexusEnco(FILE *f, int disp);
extern int ConvGnuObjdump(FILE *f);

static int error(const char *err)
{
  printf("\n");
  printf("NEXTR ERROR: %s\n", err);
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
  printf("  NexRv -dump <nex> [-msg|-none] - dump Nexus file\n");
  printf("  NexRv -deco <nex> -info <info> [-stat|-all|-msg|-none] - decode trace\n");
  printf("  NexRv -enco <pcseq> -nex <nex> - encode trace\n");
  printf("  NexRv -conv - create info-file from objdump -d output (stdin => stdout)\n");
  return 1;
}

int main(int argc, char *argv[])
{
  if (argc < 2) return usage(NULL);

  if (strcmp(argv[1], "-dump") == 0) // Dump?
  {
    if (argc < 3) return usage("Nexus bin-file is expected");

    fNex = fopen(argv[2], "rb");
    if (fNex == NULL) return error("Cannot open nex-file");

    int disp = 4 | 2 | 1; // Default (all)
    if (argc > 3 && strcmp(argv[3], "-msg") == 0)   disp = 4 | 2; // TCODE and stat.
    if (argc > 3 && strcmp(argv[3], "-none") == 0)  disp = 4;     // Only statistics

    int ret = NexusDump(disp);
    fclose(fNex); fNex = NULL;

    if (ret <= 0) return error("Nexus Trace dump failed");

    return 0; // All OK
  }

  if (strcmp(argv[1], "-conv") == 0) // Convert?
  {
    FILE *binFile = NULL;
    if (argc >= 3)
    {
      return usage("Options are not handled (yet)");
    }

    int ret = ConvGnuObjdump(stdin);

    if (ret > 0)
    {
      printf(".end ; Converted OK (%d instructions)\n", ret);
      ret = 0;
    }
    else
    {
      printf("ERROR: Conversion failed with error code #%d\n", -ret);
      ret = 9;
    }

    return ret;
  }

  if (strcmp(argv[1], "-enco") == 0) // Encode?
  {
    if (argc < 5) return error("Incorrect number of parameters");

    if (strcmp(argv[3], "-nex") != 0) return error("-nex must be provided");

    FILE *fPcseq = fopen(argv[2], "rt");
    if (fPcseq == NULL)  return error("Cannot open pcseq-file");

    fNex = fopen(argv[4], "wb");
    if (fNex == NULL) return error("Cannot create nex-file");

    int disp = 4 | 2 | 1; // Default (all)
    if (argc > 5 && strcmp(argv[5], "-msg") == 0)   disp = 4 | 2; // TCODE and stat.
    if (argc > 5 && strcmp(argv[5], "-stat") == 0)  disp = 4;     // Only statistics
    if (argc > 5 && strcmp(argv[5], "-none") == 0)  disp = 0;     // Nothing

    int ret = NexusEnco(fPcseq, disp);
    fclose(fNex); fNex = NULL;
    fclose(fPcseq); fPcseq = NULL;

    if (ret > 0)
    {
      printf("Encoded OK (%d messages)\n", ret);
      ret = 0;
    }
    else
    {
      printf("ERROR: Encoding failed with error code #%d\n", -ret);
      ret = 9;
    }

    return ret;
  }

  if (strcmp(argv[1], "-deco") == 0) // Decode?
  {
    if (argc < 5) return error("Incorrect number of parameters");
    if (strcmp(argv[3], "-info") != 0) return error("-info must be provided");

    fNex = fopen(argv[2], "rb");
    if (fNex == NULL) return error("Cannot open nex-file");
    if (InfoInit(argv[4]) < 0) error("Cannot open info-file");

    int disp = 4; // Default (-stat)
    if (argc > 5 && strcmp(argv[5], "-all") == 0)   disp = 4 | 2 | 1; // All
    if (argc > 5 && strcmp(argv[5], "-msg") == 0)   disp = 4 | 2;     // TCODE and stat.
    if (argc > 5 && strcmp(argv[5], "-stat") == 0)  disp = 4;         // Only statistics
    if (argc > 5 && strcmp(argv[5], "-none") == 0)  disp = 0;         // Nothing

    int ret = NexusDeco(disp);
    fclose(fNex); fNex = NULL;
    InfoTerm();

    if (ret > 0)
    {
      printf(".end ; Decoded OK (%d instructions)\n", ret);
      ret = 0;
    }
    else
    {
      printf("ERROR: Decoding failed with error code #%d\n", -ret);
      ret = 9;
    }

    return ret;
  }

  return usage("Unkown option");
}

//****************************************************************************
// End of nextr.c file
