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
// File nexrv.c  - Nexus Trace dump/encode/decode reference implementation

// Code below is written in plain C-code.
// It was compiled using VisualC, GNU and IAR C/C++ compiler.
//  1. Only standard C-types are used.
//  2. Only few standard C functions used - see notes with "#include <...>"
//  3. Only non K&R C is 'for (int x' and 'int x;' between instructions.

#include <stdio.h>  //  For NULL, 'printf', 'fopen, ...'
#include <stdlib.h> //  For 'exit'
#include <string.h> //  For 'strcmp', 'strchr'

#include "NexRv.h"      // For Nexus_TypeAddr
#include "NexRvInfo.h"  // For 'InfoInit/InfoTerm'

// #define WITH_EXT 1      // Enable (in code, not by -DWITH_EXT=1 command line)

#ifndef WITH_EXT
#define WITH_EXT 0      // By default without QEMU trace handling
#endif

FILE *fNex  = NULL;     // Used by NexusDump/NexusDeco/NexusEnco

extern int NexusDump(FILE *f, int disp);
extern int NexusDeco(FILE *f, int disp);
extern int NexusEnco(FILE *f, int level, int disp);
#if WITH_EXT
extern int ExtProcess(int argc, char *argv[]);
#endif

extern int ConvGnuObjdump(FILE *fObjd, FILE *fPcInfo);
extern int ConvAddInfo(FILE *fIn, FILE *fOut, FILE *fComp);
extern int ConvRtlTrace(FILE *fIn, FILE *fOut);

int conf_Repeat = 0;        // 0=no repeat, 1=releat branch only, 2=repeat history

#if 1 // Callstack related

#define CALLSTACK_MAX 32    // Max depth

int conf_CallStack = 0;     // =0: No support for call stack
// int conf_CallStack = -8;    // <0: Callstack without a stack (just a counter). Max -N entries.
// int conf_CallStack = 8;     // >0: Call-stack 'N' entries deep (with storing of an address).

Nexus_TypeAddr callStack[CALLSTACK_MAX] = {0};
int callStackTop = 0;
int callStackCnt = 0;
int callStackMax = 0;

void CallStack_Init()
{
  callStackCnt = 0;
  callStackTop = 0;
  callStack[0] = 0; // Needed for logging with 'no-stack'
  if (conf_CallStack >= 0)
  {
    callStackMax = conf_CallStack;
  }
  else
  {
    callStackMax = -conf_CallStack;
  }
}

void CallStack_Push(Nexus_TypeAddr ret)
{
  if (0) printf("CallPush[%d] 0x%lX\n", callStackCnt + 1, ret);

  if (conf_CallStack <= 0)
  {
    // Callstack without storing addresses (just saturating +- counter)
    if (callStackCnt < callStackMax)
    {
      callStackCnt++; // Count (saturating at callStackMax)
    }

    return;
  }

  // Adjust 'top' (with wrap-around)
  if (callStackTop >= callStackMax)
  {
    callStackTop = 0; // Wrap-around
  }
  else
  {
    callStackTop++;   // Just next
  }

  //Store (in new top)
  callStack[callStackTop] = ret;

  // Calculate new size (saturating)
  if (callStackCnt < callStackMax)
  {
    callStackCnt++;
  }
}

Nexus_TypeAddr CallStack_Pop()
{
  if (0) printf("CallPop[%d] 0x%lX\n", callStackCnt, callStack[callStackTop]);

  // Calculate new size (and handle empty)
  if (callStackCnt == 0) return 1;  // Empty ('1' will never match 'real PC'!
  callStackCnt--;

  if (conf_CallStack <= 0)
  {
    return 0; // Any non-empty address (it will NOT be compared)
  }

  int prevTop = callStackTop;

  // Adjust 'top' (with wrap-around)
  if (callStackTop == 0)
  {
    callStackTop = (callStackMax - 1);  // Wrap around
  }
  else
  {
    callStackTop--;                     // Just previous
  }

  return callStack[prevTop];  // Return element on top (before adjustment)
}

#endif

static int error(const char *err)
{
  printf("\n");
  printf("NexRv ERROR: %s\n", err);
  return 9;
}

// For use externally (nicer name)
int NexRv_error(const char *err)
{
  return error(err);
}

static int usage(const char *err)
{
  if (err != NULL)
  {
    error(err);
  }
  printf("\n");
  printf("NexRv v1.0.0 (2025/01/02)\n");
  printf("Usage:\n");
  printf("  NexRv -dump <nex> [<dump>] [-msg|-none] - dump Nexus file\n");
  printf("  NexRv -deco <nex> -pcinfo <info> -pcout <pco> [-stat|-full|-all|-msg|-none] - decode trace\n");
  printf("  NexRv -enco <pcseq> -nex <nex> [-nobhm|-norbm|-cs [<cs>]|-rpt <m>] [-stat|-full|-all|-msg|-none] - encode trace \n");
  printf("  NexRv -conv -objd <objd> -pcinfo <pci> - create <pci> from objdump -d output <objd>\n");
  printf("  NexRv -conv -pcinfo <pci> -pconly <pco> -pcseq <pcs> - convert <pco> to <pcs> using <pci>\n");
  printf("  NexRv -conv -rtl <rtl> -pconly <pco> -  create <pco> file from <rtl> trace file\n");
  printf("  NexRv -diff -pcseq <pcs> -pcout <pco> - compare <pcs> with <pco>\n");
#if WITH_EXT
  printf("  NexRv -ext ... - extra processing (use -ext only to display extra usage)\n");
#endif
  printf("where:\n");
  printf("  -nobhm|-norbm               - do not generate Branch History/Repeat Branch Messages\n");
  printf("  -cs [<cs>]                  - enable call-stack level <cs> (0=none, 8 is default)\n");
  printf("  -rpt [<m>]                  - enable repeat detection (0=none,1=repeat branch,2=repeat history)\n");
  printf("  -stat|-full|-all|-msg|-none - verbose level\n");

#if 0
  printf("sizeof(unsigned int) = %d\n",       sizeof(unsigned int));
  printf("sizeof(unsigned long) = %d\n",      sizeof(unsigned long));
  printf("sizeof(unsigned long long) = %d\n", sizeof(unsigned long long));
  printf("sizeof((uint64_t) = %d\n",          sizeof(uint64_t));
  printf("sizeof(Nexus_TypeAddr) = %d\n",     sizeof(Nexus_TypeAddr));
#endif

  return 1;
}

int main(int argc, char *argv[])
{
  if (argc < 2) return usage(NULL);

#if WITH_EXT
  if (strcmp(argv[1], "-ext") == 0)
  {
    // Extra processing (if enabled)
    int ret = ExtProcess(argc, argv);
    return ret;
  }
#endif

  if (strcmp(argv[1], "-dump") == 0) // Dump?
  {
    if (argc < 3) return usage("Nexus bin file is expected");

    fNex = fopen(argv[2], "rb");
    if (fNex == NULL) return error("Cannot open NEX file");

    int opt = 3;
    FILE *fDump = stdout;
    if (argc > 3 && argv[3][0] != '-')
    {
      fDump = fopen(argv[3], "wt");
      if (fDump == NULL) return error("Cannot create DUMP file");
      opt = 4;
    }

    int disp = 4 | 2 | 1; // Default (all)
    if (argc > opt && strcmp(argv[opt], "-msg") == 0)   disp = 4 | 2; // TCODE and stat.
    if (argc > opt && strcmp(argv[opt], "-none") == 0)  disp = 4;     // Only statistics

    int ret = NexusDump(fDump, disp);
    fclose(fNex); fNex = NULL;
    if (fDump != stdout) fclose(fDump);

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
        ret = ConvAddInfo(pcFile, psFile, NULL);
        fclose(pcFile);
        fclose(psFile);
        InfoTerm();
      }
    }
    else
    if (argc == 6 && strcmp(argv[2], "-rtl") == 0)
    {
      // -conv -objd <objd> -pcinfo <pci>
      if (strcmp(argv[4], "-pconly") == 0)
      {
        // Syntax correct - open all files
        err = NULL;

        FILE *rtlFile = fopen(argv[3], "rt");
        if (rtlFile == NULL) return error("Cannot open RTL file");

        FILE *pcoFile = fopen(argv[5], "wt");
        if (pcoFile == NULL) return error("Cannot create PCONLY file");

        // Run conversion
        ret = ConvRtlTrace(rtlFile, pcoFile);
        fclose(pcoFile);
        fclose(rtlFile);
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

  if (strcmp(argv[1], "-diff") == 0) // Diff?
  {
    // -comp -pcseq <pcs> -pcout <pco>
    const char *err = "Incorrect -diff calling";

    int ret = 0;
    if (argc == 6 && (strcmp(argv[2], "-pcseq") == 0 || strcmp(argv[2], "-pconly") == 0))
    {
      // -diff -pcseq <pcseq> -pcout <pco>
      if (strcmp(argv[4], "-pcout") == 0)
      {
        // Syntax correct - open all files
        err = NULL;

        FILE *fPcseq = fopen(argv[3], "rt");
        if (fPcseq == NULL)  return error("Cannot open PCSEQ file");

        FILE *fPcout = fopen(argv[5], "rt");
        if (fPcout == NULL) return error("Cannot open PCOUT file");

        // Run comparison
        ret = ConvAddInfo(fPcseq, NULL, fPcout);

        fclose(fPcseq);
        fclose(fPcout);
      }
    }
    if (err != NULL) return usage(err);

    if (ret > 0)
    {
      printf("Compared OK (%d instructions)\n\n", ret);
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
    if (fPcseq == NULL)  return error("Cannot open PCSEQ file");

    fNex = fopen(argv[4], "wb");
    if (fNex == NULL) return error("Cannot create NEX file");

    int level = -1;     // Default level
    conf_CallStack = 0; // No callstack (by default)
    int disp = 4;       // Default display

    // Process options
    int ai = 5;
    while (ai < argc)
    {
      if (strcmp(argv[ai], "-nobhm") == 0) level = 10;  // Level 1.0
      else
      if (strcmp(argv[ai], "-norbm") == 0) level = 20;  // Level 2.0
      else
      if (strcmp(argv[ai], "-cs") == 0) 
      {
        int v;
        if (ai + 1 < argc && sscanf(argv[ai + 1], "%d", &v) == 1)
        {
          ai++;
          conf_CallStack = v;
        }
        else
        {
          conf_CallStack = 8;
        }

        if (abs(conf_CallStack) > CALLSTACK_MAX)
        {
          return error("Value of -cs is too big");
        }
        printf("NexRv/Callstack: %d\n", conf_CallStack);
      }
      else
      if (strcmp(argv[ai], "-rpt") == 0) 
      {
        int v;
        if (ai + 1 < argc && sscanf(argv[ai + 1], "%d", &v) == 1)
        {
          ai++;
          conf_Repeat = v;
        }
        else
        {
          conf_Repeat = 2;
        }

        printf("NexRv/Repeat: %d\n", conf_Repeat);
      }
      else
      if (strcmp(argv[ai], "-all") == 0)   disp = 4 | 2 | 1; // All
      else
      if (strcmp(argv[ai], "-msg") == 0)   disp = 4 | 2;     // TCODE and stat.
      else
      if (strcmp(argv[ai], "-stat") == 0)  disp = 4;         // Only statistics
      else
      if (strcmp(argv[ai], "-none") == 0)  disp = 0;         // Nothing
      else
      if (strcmp(argv[ai], "-full") == 0)  disp = 0xFF;      // Everything
      else
      {
        printf("ERROR: Unknown option %s\n", argv[ai]);
        return 10;
      }
      ai++;
    }

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
    conf_CallStack = CALLSTACK_MAX; // Always full call-stack

    if (argc < 7) return error("Incorrect number of parameters");
    if (strcmp(argv[3], "-pcinfo") != 0) return error("-pcinfo must be provided");
    if (strcmp(argv[5], "-pcout") != 0) return error("-pcout must be provided");

    fNex = fopen(argv[2], "rb");
    if (fNex == NULL) return error("Cannot open NEX file");
    if (InfoInit(argv[4]) < 0) return error("Cannot open PCINFO file");
    FILE *fOut = fopen(argv[6], "wt");
    if (fOut == NULL) return error("Cannot create PCOUT file");


    int disp = 4; // Default (-stat)
    if (argc > 7 && strcmp(argv[7], "-all") == 0)   disp = 4 | 2 | 1; // All
    if (argc > 7 && strcmp(argv[7], "-msg") == 0)   disp = 4 | 2;     // TCODE and stat.
    if (argc > 7 && strcmp(argv[7], "-stat") == 0)  disp = 4;         // Only statistics
    if (argc > 7 && strcmp(argv[7], "-none") == 0)  disp = 0;         // Nothing
    if (argc > 7 && strcmp(argv[7], "-full") == 0)  disp = 0xFF;      // Everything

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
// End of nexrv.c file
