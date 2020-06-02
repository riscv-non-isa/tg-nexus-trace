# t1 - Initial trace test/demo (example of NexRv tool usage)
It is RLE compressing and decompressing [RISC-V Logo](./riscvlogo.png) in [BMP format](./riscvlogo.bmp).

I could not find any other simple enough, but usable program.

Programs provided by [Embench](https://github.com/embench/embench-iot) will be also utilized but these are too complex for initial test/demo.

## Results (Nexus Trace compression as displayed by NexRv tool)

* Nexus Level1.0 (only Direct Branch messages):	  0.63 bits/instr (164959 instructions, 12980 Nexus bytes)
* Nexus Level2.0 (with Branch History messages):  0.24 bits/instr (164959 instructions,  4994 Nexus bytes)
* Nexus Level2.1 (with Repeat Branch messages):   0.24 bits/instr (164959 instructions,  4906 Nexus bytes)

## Files
  Sources:
  
    riscvlogo.png  - Original (compressed as PNG) RISC-V logo
    riscvlogo.bmp  - Part of logo without border as non-compressed BMP
    test.c         - Test program
    riscvlogo.h    - BMP file as buffer to be processed by 'test.c' code
    crt0.S         - Needed to link test.elf
    entry.S        - Needed to link test.elf
    rom-20010000_ram-80000000.lds - linker definition file needed to link 'test.elf'

  NexRv tool related:
  
    makefile           - Use 'make' to create all output files
    makelog.txt        - Output from running 'make >makelog.txt'
    test.elf           - ELF file (compiled by GNU)
    ./test-OBJD.txt    - Result of objdump -d test.elf >./test-OBJD.txt
    ./test-PCLIST.txt  - Plain PC sequence (from 'main' till call to 'exit(0)')
                         It was created from trace saved by IAR simulator.
			 
  NexRv tool output (all created by running 'make'):

    ./output/test-PCINFO.txt - INFO file (created from ELF/OBJD file)
    ./output/test-PCSEQ.txt  - PCSEQ file (PC + type for all instructions)
    ./output/test-NEX.bin    - Binary Nexus trace
    ./output/test-DUMP.txt   - Dump of binary Nexus file
    ./output/test-PCOUT.txt  - Decoder output (identical as ./test-PCLIST.txt)

## Compile example code (optional as ELF and OBJD files are provided):

* Get xrle.c and xrle.h from here: https://github.com/algorithm314/xrle
* Make sure you have RISC-V GNU tools in path
* Run 'make test.elf' to compile and link 'test.elf'
* Run 'make OBJD' to run objdump -d utility

