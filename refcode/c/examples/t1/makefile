all:
	../../NexRv -conv -objd test-OBJD.txt -pcinfo ./output/test-PCINFO.txt
	../../NexRv -conv -pcinfo ./output/test-PCINFO.txt -pconly test-PCONLY.txt -pcseq ./output/test-PCSEQ.txt
	echo == No Branch History Message (only Direct Branch Message)
	../../NexRv -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin -nobhm
	../../NexRv -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	diff -s -q ./test-PCONLY.txt ./output/test-PCOUT.txt
	echo == No Repeat Branch Message ...
	../../NexRv -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin -norbm
	../../NexRv -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	diff -s -q ./test-PCONLY.txt ./output/test-PCOUT.txt
	echo == With Repeat Branch Message (default)
	../../NexRv -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin
	../../NexRv -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	diff -s -q ./test-PCONLY.txt ./output/test-PCOUT.txt

ELF:
	riscv64-unknown-elf-gcc -c -g -fno-builtin -nostdlib -fsigned-char -g -ffunction-sections -fdata-sections -march=rv32imac -mabi=ilp32 -mcmodel=medlow test.c
	riscv64-unknown-elf-gcc -g -march=rv32imac -mabi=ilp32 -mcmodel=medlow -fno-builtin -nostdlib ./entry.S ./crt0.S test.o -Wl,--gc-sections -Wl,-Map,test.map -o test.elf -T./rom-20010000_ram-80000000.lds -Wl,--start-group -lc -lgcc -lm -Wl,--end-group
	
OBJD:
	riscv64-unknown-elf-objdump -d test.elf >test-OBJD.txt 
