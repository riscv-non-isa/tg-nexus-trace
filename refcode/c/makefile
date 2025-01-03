all: NexRv.exe

# Handle EXTRA=name option (to compile name_ext.c file)
ifdef EXTRA
FEXTRA=$(EXTRA)_ext.c
WITH_EXT=-DWITH_EXT=1
else
FEXTRA=
WITH_EXT=
endif

NexRv.exe : NexRv.c NexRvDeco.c NexRvEnco.c NexRvDump.c NexRvInfo.c NexRvConv.c NexRv.h NexRvMsg.h NexRvInfo.h $(FEXTRA) 
	gcc -O3 $(WITH_EXT) NexRv.c NexRvDeco.c NexRvEnco.c NexRvDump.c NexRvInfo.c NexRvConv.c $(FEXTRA) -o NexRv.exe

