all:
	../../NexRv.exe -conv -objd test-OBJD.txt -pcinfo ./output/test-PCINFO.txt
	../../NexRv.exe -conv -pcinfo ./output/test-PCINFO.txt -pconly test-PCONLY.txt -pcseq ./output/test-PCSEQ.txt
	echo No Branch History Message - only Direct Branch Message
	../../NexRv.exe -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin -nobhm
	../../NexRv.exe -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv.exe -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	../../NexRv.exe -diff -pconly ./test-PCONLY.txt -pcout ./output/test-PCOUT.txt	
	echo  No Call-stack and no Repeat Message ...
	../../NexRv.exe -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin -cs 0 -rpt 0
	../../NexRv.exe -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv.exe -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	../../NexRv.exe -diff -pconly ./test-PCONLY.txt -pcout ./output/test-PCOUT.txt	
	echo  No Call-stack and with Repeat Message ...
	../../NexRv.exe -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin  -cs 0 -rpt 2
	../../NexRv.exe -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv.exe -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	../../NexRv.exe -diff -pconly ./test-PCONLY.txt -pcout ./output/test-PCOUT.txt	
	echo  With callstack and no repeat
	../../NexRv.exe -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin -cs 8 -rpt 0
	../../NexRv.exe -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv.exe -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	../../NexRv.exe -diff -pconly ./test-PCONLY.txt -pcout ./output/test-PCOUT.txt	
	echo  With callstack and repeat -best possible compression
	../../NexRv.exe -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin -cs 8 -rpt 2
	../../NexRv.exe -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv.exe -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	../../NexRv.exe -diff -pconly ./test-PCONLY.txt -pcout ./output/test-PCOUT.txt	

ELF:
	riscv64-unknown-elf-gcc -c -g -fno-builtin -nostdlib -fsigned-char -g -ffunction-sections -fdata-sections -march=rv32imac -mabi=ilp32 -mcmodel=medlow test.c
	riscv64-unknown-elf-gcc -g -march=rv32imac -mabi=ilp32 -mcmodel=medlow -fno-builtin -nostdlib ./entry.S ./crt0.S test.o -Wl,--gc-sections -Wl,-Map,test.map -o test.elf -T./rom-20010000_ram-80000000.lds -Wl,--start-group -lc -lgcc -lm -Wl,--end-group
	
OBJD:
	riscv64-unknown-elf-objdump -d test.elf >test-OBJD.txt 
