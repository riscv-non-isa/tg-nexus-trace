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
// File NexRvInfo.h  - Instruction information access

#ifndef NEXRVINFO_H
#define NEXRVINFO_H

#define INFO_LINEAR   0x1   // Linear (plain instruction or not taken BRANCH)
#define INFO_4        0x2   // If not 4, it must be 2 on RISC-V
//                    0x4   // Reserved (for exception or so ...)
#define INFO_INDIRECT 0x8   // Possible for most types above
#define INFO_BRANCH   0x10  // Always direct on RISC-V (may have LINEAR too)
#define INFO_JUMP     0x20  // Direct or indirect
#define INFO_CALL     0x40  // Direct or indirect (and always a jump)
#define INFO_RET      0x80  // Return (always indirect and always a jump)

extern int InfoParse(const char *t, unsigned int *pAddr, unsigned int *pInfo, unsigned int *pDest);
extern int InfoInit(const char *filename);
extern unsigned int InfoGet(unsigned int addr, unsigned int *pDest);
extern void InfoTerm(void);

#endif  // NEXRVINFO_H

//****************************************************************************
// End of NexRvInfo.h file
