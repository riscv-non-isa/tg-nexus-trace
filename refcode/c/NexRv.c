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

FILE *fNex  = NULL;     // Used by NexusDump/NexusDeco/NexusEnco

extern int NexusDump(int disp);
extern int NexusDeco(FILE *f, int disp);
extern int NexusEnco(FILE *f, int level, int disp);
extern int ConvGnuObjdump(FILE *fObjd, FILE *fPcInfo);
extern int ConvAddInfo(FILE *fIn, FILE *fOut);

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
  printf("  NexRv -deco <nex> -pcinfo <info> [-stat|-all|-msg|-none] - decode trace\n");
  printf("  NexRv -enco <pcseq> -nex <nex> [-nobhm|-norbm] [-stat|-all|-msg|-none] - encode trace \n");
  printf("  NexRv -conv -objd <objd> -pcinfo <pci> - create <pci> from objdump -d output <objd>\n");
  printf("  NexRv -conv -pcinfo <pci> -pconly <pco> -pcseq <pcs> - convert <pco> to <pcs> using <pci>\n");
  printf("where:\n");
  printf("  -nobhm|-norbm         - do not generate Branch History/Repeat Branch Messages\n");
  printf("  -stat|-all|-msg|-none - verbose level\n");
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
    // -conv -objd <objd> -pcinfo <pci>
    // -conv -pcinfo <pci> -pconly <pc> -pcseq <ps>
    const char *err = "Incorrect -conv calling";

    int ret = 0;
    if (argc == 6 && strcmp(argv[2], "-objd") == 0)
    {
      // -conv -objd <objd> -pcinfo <pci>
      if (strcmp(argv[4], "-pcinfo") == 0)
      {
        // Syntax correct - open all files
        err = NULL;

        FILE *objdFile = fopen(argv[3], "rt");
        if (objdFile == NULL) return error("Cannot open OBJD file");

        FILE *pciFile = fopen(argv[5], "wt");
        if (pciFile == NULL) return error("Cannot create PCINFO file");

        // Run conversion
        ret = ConvGnuObjdump(objdFile, pciFile);
        fclose(pciFile);
        fclose(objdFile);
      }
    }
    else
    if (argc == 8 && strcmp(argv[2], "-pcinfo") == 0)
    {
      // -conv -pcinfo <pci> -pconly <pc> -pcseq <ps>
      if (strcmp(argv[4], "-pconly") == 0 && strcmp(argv[6], "-pcseq") == 0)
      {
        // Syntax correct - open all files
        err = NULL;

        if (InfoInit(argv[3]) < 0) return error("Cannot open PCINFO file");

        FILE *pcFile = fopen(argv[5], "rt");
        if (pcFile == NULL) return error("Cannot open PCONLY file");

        FILE *psFile = fopen(argv[7], "wt");
        if (psFile == NULL) return error("Cannot create PCSEQ file");

        // Run conversion
        ret = ConvAddInfo(pcFile, psFile);
        fclose(pcFile);
        fclose(psFile);
        InfoTerm();
      }
    }
    if (err != NULL) return usage(err);

    if (ret > 0)
    {
      printf("Converted OK (%d instructions)\n\n", ret);
      ret = 0;
    }
    else
    {
      printf("ERROR: Conversion failed with error code #%d\n\n", -ret);
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

    int level = -1; // Default level
    if (argc > 5 && strcmp(argv[5], "-nobhm") == 0) level = 10;  // Level 1.0
    if (argc > 5 && strcmp(argv[5], "-norbm") == 0) level = 20;  // Level 2.0

    int disp = 4; // Default (stat)

    int dispPos = 5;
    if (level >= 0) dispPos++; // Skip over level parameter
    if (argc > dispPos && strcmp(argv[dispPos], "-all") == 0)   disp = 4 | 2 | 1; // All
    if (argc > dispPos && strcmp(argv[dispPos], "-msg") == 0)   disp = 4 | 2;     // TCODE and stat.
    if (argc > dispPos && strcmp(argv[dispPos], "-stat") == 0)  disp = 4;         // Only statistics
    if (argc > dispPos && strcmp(argv[dispPos], "-none") == 0)  disp = 0;         // Nothing

    if (level < 0) level = 21;  // Level 2.1 is default
    int ret = NexusEnco(fPcseq, level, disp);
    fclose(fNex); fNex = NULL;
    fclose(fPcseq); fPcseq = NULL;

    if (ret > 0)
    {
      printf("Encoded OK (%d messages)\n\n", ret);
      ret = 0;
    }
    else
    {
      printf("ERROR: Encoding failed with error code #%d\n\n", -ret);
      ret = 9;
    }

    return ret;
  }

  if (strcmp(argv[1], "-deco") == 0) // Decode?
  {
    if (argc < 5) return error("Incorrect number of parameters");
    if (strcmp(argv[3], "-pcinfo") != 0) return error("-pcinfo must be provided");
    if (strcmp(argv[5], "-pcout") != 0) return error("-pcout must be provided");

    fNex = fopen(argv[2], "rb");
    if (fNex == NULL) return error("Cannot open NEX-file");
    if (InfoInit(argv[4]) < 0) return error("Cannot open PCINFO-file");
    FILE *fOut = fopen(argv[6], "wt");
    if (fOut == NULL) return error("Cannot create PCOUT-file");

    int disp = 4; // Default (-stat)
    if (argc > 7 && strcmp(argv[7], "-all") == 0)   disp = 4 | 2 | 1; // All
    if (argc > 7 && strcmp(argv[7], "-msg") == 0)   disp = 4 | 2;     // TCODE and stat.
    if (argc > 7 && strcmp(argv[7], "-stat") == 0)  disp = 4;         // Only statistics
    if (argc > 7 && strcmp(argv[7], "-none") == 0)  disp = 0;         // Nothing

    int ret = NexusDeco(fOut, disp);
    fclose(fOut);
    fclose(fNex); fNex = NULL;
    InfoTerm();

    if (ret > 0)
    {
      printf("Decoded OK (%d instructions)\n\n", ret);
      ret = 0;
    }
    else
    {
      printf("ERROR: Decoding failed with error code #%d\n\n", -ret);
      ret = 9;
    }

    return ret;
  }

  return usage("Unkown option");
}

//****************************************************************************
// End of nextr.c file
