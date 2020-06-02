// This is simple test for Trace Compression
// 

#include <stdint.h>     // For standard types
#include <stdlib.h>     // For standard types
#include <string.h>

#include "riscvlogo.h"  // Include header with RISCV-logo (riscvlogo.bmp file)

// Some simple, but usefull code from here:
//      https://github.com/algorithm314/xrle

// It is not compiled separatelly to make it easier to compile as sigle source
#include "xrle.h"
#include "xrle.c"

// All these are global (for easier inspection by debugger)
// NOTE: Above 'xrle' is not output-size safe, so we use buffers bigger than BMP
// These buffer are 64-bit size to force 8-byte allignment
uint64_t rle_buf[sizeof(riscvlogo)/sizeof(uint64_t) + sizeof(uint64_t)];
uint64_t bmp_buf[sizeof(riscvlogo)/sizeof(uint64_t) + sizeof(uint64_t)];

// Volatile below prevent compiler to optimize these away!

size_t          img_size;       // Original image size
volatile size_t rle_size;       // After RLE compression
volatile size_t bmp_size;       // After RLE decompression

#if 0   // Library functions implemented 'in-place'

static int memcmp(const void *_p1, const void *_p2, size_t s)
{
  const uint8_t *p1 = (const uint8_t *)_p1;
  const uint8_t *p2 = (const uint8_t *)_p2;
  
  while (s > 0)
  {
    if (*p1 != *p2)
    {
      if (*p1 < *p2)    return -1;
      else              return 1;
    }
    p1++;       // Advance
    p2++;
    s--;
  }
  
  return 0;
}

#endif

#if 1	// Tricky way to not link library 'exit' function

#define exit(x) myExit(x)

int myExit(int ret)
{
  ret &= ~1;		// Make it even
  while (ret != 1)	// This will never terminate
  {
	ret += 2;
  }
  return ret;
}

#endif

void main()
{
  img_size = sizeof(riscvlogo);
  
  // Run compression and de-compression
  rle_size = xrle_compress(rle_buf, riscvlogo, img_size);
  bmp_size = xrle_decompress(bmp_buf, rle_buf, rle_size);
  
  // Check results
  if (bmp_size != img_size)
  {
    exit(1);    // Error: decompression did not provide same size
  }
  
#if 0   // Make trace smaller - 'memcpy' may bias bcompression ratio   
  if (memcmp(riscvlogo, bmp_buf, img_size) != 0)
  {
    exit(2);    // Error: decompression did not provide same data
  }
#endif
  
  exit(0);      // OK
}
