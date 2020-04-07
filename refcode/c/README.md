# Nexus Trace TG reference code
Reference code for RISC-V Nexus Trace TG recommendations.<br>

1. Why reference code?<br>
* It augments definition of Nexus messages and allows clarification of handling corner cases.<br>
* It may be used to assess compression ratio for particular program.<br>
* Later some options may customize certain aspects of Nexus trace encoding and decoding.<br>
* It may be used to create certain corner cases (as text files) and assess intended behavior.<br>
* Reference encoder (in C) can be used as guidance to generate Nexus messages from logic.<br>
* It may be used as test-bench to compare behavior with logic.<br>

2. Reference code is split into several source files:<br>
* NexRv.c		  - Main executable (option handling, opening files and calling functions).<br>
* NexRv.h	    - Header with Nexus-related constant definitions (using #define).<br>
* NexRvMsg.h	- Array defining RISC-V specific Nexus message types and fields.<br>
* NexRvDump.c	- Function to dump messages in binary Nexus file (called with -dump option).<br>
* NexRvDeco.c	- Function to decode binary Nexus file (called with -deco option).<br>
* NexRvEnco.c	- Function to encode PC sequence into Nexus binary (called with -enco option).<br>
* NexRvConv.c	- Functions for different conversions (called with -conv option).<br>
* makefile	  - allows to compile it by calling ‘make’<br>
  
3. Code is written using simple (K&R-style …) C language constructs, so can be well understood<br>

* It was not written having speed in mind but rather code simplicity – it is not intended to process large files, but rather to inspect some corner cases.<br>
* All operations are done on text files (except binary Nexus file).<br>
* No memory allocations and only few C-library functions are called.<br>
* The only ‘advanced feature’ is using ‘for (int x = …’) and ‘int x;’ declarations between instructions in few places (it may be converted to pure K&R C style if needed).<br>

4. Compilation should be done by running just running ‘make’ using ‘gcc’:

* It was compiled using gcc (i686-posix-dwarf-rev0, Built by MinGW-W64 project) 8.1.0
* Code was compiled using Visual Studio compiler as well.
* It would be possible to simplify main code and only compile appropriate sub-module.

5.	Here is usage:<br>

* NexRv -dump \<nex\> [-msg|-none] - dump Nexus file<br>
* NexRv -deco \<nex\> -info \<info\> [-stat|-all|-msg|-none] - decode trace<br>
* NexRv -enco \<pcseq\> -nex \<nex\> - encode trace<br>
* NexRv -conv - create info-file from objdump -d output (stdin => stdout)<br>

6.	Examples (to illustrate flow)<br>

* TODO<br>

7.	Format of \<info\> and \<pcseq\> files<br>
  
* TODO<br>
