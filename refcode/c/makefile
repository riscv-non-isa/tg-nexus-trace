NexRv.exe : NexRv.c NexRvDeco.c NexRvEnco.c NexRvDump.c NexRvInfo.c NexRvConv.c NexRv.h NexRvMsg.h NexRvInfo.h
	gcc NexRv.c NexRvDeco.c NexRvEnco.c NexRvDump.c NexRvInfo.c NexRvConv.c -o NexRv.exe

# Below are two test-cases
#  TODO: This below is just example (t1-*.* files are not in GIT yet)

t1: 
	rem Running test t1 ...
	rem ****************************************************************
	rem ** These below are excluded as t1-info.txt is in GIT already
	rem ****************************************************************
	rem NexTr -conv t1.elf 
	rem ****************************************************************
	NexRv -deco t1-nex.bin -info t1-info.txt >t1-pc1.txt
	tail ta-pc1.txt
	NexRv -enco t1-pc1.txt -nex t1-nex2.bin


