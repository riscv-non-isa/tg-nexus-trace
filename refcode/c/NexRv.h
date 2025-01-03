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
// File NexRv.h  - Nexus RISC-V Trace header.

// Header with common Nexus Trace definitions
// Used by NexRv*.c (dump, encode, decode, convert)

#ifndef NEXRV_H
#define NEXRV_H

#include <stdint.h> // For uint32_t and uint64_t

//****************************************************************************
// Nexus specific values (based on Nexus Standard PDF)

#define NEXUS_FLDSIZE_TCODE                       6 // This is standard

// Nexus TCODE values applicable to RISC-V
#define NEXUS_TCODE_Ownership                     2
#define NEXUS_TCODE_DirectBranch                  3
#define NEXUS_TCODE_IndirectBranch                4
#define NEXUS_TCODE_Error                         8
#define NEXUS_TCODE_ProgTraceSync                 9
#define NEXUS_TCODE_DirectBranchSync              11
#define NEXUS_TCODE_IndirectBranchSync            12
#define NEXUS_TCODE_ResourceFull                  27
#define NEXUS_TCODE_IndirectBranchHist            28
#define NEXUS_TCODE_IndirectBranchHistSync        29
#define NEXUS_TCODE_RepeatBranch                  30
#define NEXUS_TCODE_ProgTraceCorrelation          33

// End of standard values
//****************************************************************************

//****************************************************************************
// RISC-V Nexus Trace related values (most 'recommended' by Nexus)

//  Sizes of fields:
#define NEXUS_FLDSIZE_BTYPE     2 // Branch type
#define NEXUS_FLDSIZE_SYNC      4 // Synchronization code
#define NEXUS_FLDSIZE_ETYPE     4 // Error type
#define NEXUS_FLDSIZE_EVCODE    4 // Event code (correlation)
#define NEXUS_FLDSIZE_CDF       2
#define NEXUS_FLDSIZE_RCODE     4 // Resource full code size

#define NEXUS_PAR_SIZE_SRC      0 // SRC field size is defined by parameter #0

#define NEXUS_HIST_BITS         31 // Number of valid HIST bits

// Address skipping (on RISC-V LSB of PC is always 0, so it is not encoded)
#define NEXUS_PARAM_AddrSkip    1
#define NEXUS_PARAM_AddrUnit    1

#if 0   // 32-bit only version (initial reference code)
typedef uint32_t Nexus_TypeAddr;
typedef uint32_t Nexus_TypeHist;
typedef uint32_t Nexus_TypeField;   // This must be same (or bigger than Addr/Hist fields)
#else   // 64-bit (for practical use-cases)
typedef uint64_t Nexus_TypeAddr;
typedef uint32_t Nexus_TypeHist;
typedef uint64_t Nexus_TypeField;   // This must be same (or bigger than Addr/Hist fields)
#endif

// End of RISC-V related values
//****************************************************************************

#endif  // NEXRV_H

//****************************************************************************
// End of NexRv.h file
