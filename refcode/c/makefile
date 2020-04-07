NexRv.exe : NexRv.c NexRvDeco.c NexRvEnco.c NexRvDump.c NexRvInfo.c NexRvConv.c NexRv.h NexRvMsg.h NexRvInfo.h
	gcc NexRv.c NexRvDeco.c NexRvEnco.c NexRvDump.c NexRvInfo.c NexRvConv.c -o NexRv.exe

# Below are two test-cases
#  TODO: This below is just example (t1-*.* files are not in GIT yet)

t1: 
	rem Running test t1 ...
	rem ****************************************************************
	rem ** These below are excluded as t1-info.txt is in GIT already
	rem ****************************************************************
	rem NOTE: '@' below should be replaced by redirection character!
	rem riscv-objdump -d t1.elf @t1-objd.txt
	rem NexTr -conv @t1-objd.txt @t1-info.txt 
	rem ****************************************************************
	NexRv -deco t1-nex.bin -info t1-info.txt >t1-pc1.txt
	tail ta-pc1.txt
	NexRv -enco t1-pc1.txt -nex t1-nex2.bin
	NexRv -deco t1-nex2.bin -info t1-info.txt >t1-pc2.txt
	tail ta-pc2.txt
	cmp t1-pc1.txt t1-pc2.txt
