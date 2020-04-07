NexRv.exe : NexRv.c NexRvDeco.c NexRvEnco.c NexRvDump.c NexRvInfo.c NexRvConv.c NexRv.h NexRvMsg.h NexRvInfo.h
	gcc NexRv.c NexRvDeco.c NexRvEnco.c NexRvDump.c NexRvInfo.c NexRvConv.c -o NexRv.exe

# Below are two test-cases
#  We 

t1: 
	rem Running test t1 ...
	rem ****************************************************************
	rem ** These below are excluded as t1-info.txt is in GIT already
	rem ****************************************************************
	rem NexTr -conv t1.elf 
	rem ****************************************************************
	NexRv -deco nohist-nex.bin -info nohist-info.txt >nohist-pc1.txt
	tail nohist-pc1.txt
	NexRv -enco nohist-pc1.txt -nex  nohist-nex2.bin


