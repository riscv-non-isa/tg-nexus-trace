# This is good UM for 'make': https://devhints.io/makefile

RV_OBJDUMP=/c/FS/FS-2022-08-0/SiFive/riscv64-unknown-elf-toolsuite-14.9.0-2022.08.1/bin/riscv64-unknown-elf-objdump.exe

# Shortycuts (to run 'make c' or 'make d' ...)
c: coremark
d: dhrystone
e: embench-nettle-aes

# All tests ...
all: coremark dhrystone emb more
	grep Stat *-report.txt >all.txt
	cat all.txt
	wc -l all.txt

emb: embench-aha-mont64 embench-crc32 embench-cubic embench-edn embench-huffbench embench-matmult-int embench-minver embench-nbody embench-nettle-aes embench-nettle-sha256 embench-nsichneu embench-picojpeg embench-qrduino embench-sglib-combined embench-slre embench-st embench-statemate embench-ud embench-wikisort

more: median mm mt-matmul mt-vvadd multiply qsort rsort spmv towers vvadd xrle

err: pmp br_j_asm

none: hello_world new_hw

# Options (for -enco ...)
# -nobhm
# -norbm

OPT_ENCO=-norbm
# General processing for all files ...

%: ./from_etrace/%.spike_pc_trace_filtered
	@$(MAKE) output/$*-pcout.txt

output/%-pcout.txt: ./from_etrace/%.spike_pc_trace_filtered ./from_etrace/%.riscv ../../NexRv.exe
	@echo "**** Processing $* ****"
	cp ./from_etrace/$*.spike_pc_trace_filtered ./output/$*-pconly.txt
	$(RV_OBJDUMP) -d ./from_etrace/$*.riscv > $*-objd.txt
	../../NexRv.exe -conv -objd $*-objd.txt -pcinfo output/$*-pcinfo.txt
	../../NexRv.exe -conv -pcinfo ./output/$*-pcinfo.txt -pconly ./output/$*-pconly.txt -pcseq ./output/$*-pcseq.txt
	../../NexRv.exe -enco ./output/$*-pcseq.txt -nex ./output/$*-nex.bin
	../../NexRv.exe -dump ./output/$*-nex.bin ./output/$*-dump.txt
	../../NexRv.exe -deco ./output/$*-nex.bin -pcinfo ./output/$*-pcinfo.txt -pcout ./output/$*-pcout.txt >$*-report.txt
	../../NexRv.exe -diff -pcseq ./output/$*-pcseq.txt -pcout ./output/$*-pcout.txt
	@echo
	@echo "**** $* PASSED OK ****"
	@echo

clean:
	rm -f *objd.txt output/*-*txt output/*-nex.bin
	@echo
	@echo "** Remaining in ./output **"
	ls output
	@echo

# This below is kept for reference ...

xcoremark:
	$(RV_OBJDUMP) -d ./from_etrace/coremark.riscv > coremark-objd.txt
	../../NexRv.exe -conv -objd coremark-objd.txt -pcinfo output/coremark-pcinfo.txt
	cp ./from_etrace/coremark.spike_pc_trace_filtered ./output/coremark-pconly.txt
	../../NexRv.exe -conv -pcinfo ./output/coremark-pcinfo.txt -pconly ./output/coremark-pconly.txt -pcseq ./output/coremark-pcseq.txt
	../../NexRv.exe -enco ./output/coremark-pcseq.txt -nex ./output/coremark-nex.bin
	../../NexRv.exe -dump ./output/coremark-nex.bin ./output/coremark-dump.txt
	../../NexRv.exe -deco ./output/coremark-nex.bin -pcinfo ./output/coremark-pcinfo.txt -pcout ./output/coremark-pcout.txt
	diff -i -w -s -q ./output/coremark.pcok ./output/coremark-pcout.txt
	@echo
	@echo "**** COREMARK PASSED OK ****"
	@echo


xtest:
	../../NexRv.exe -conv -objd test-OBJD.txt -pcinfo ./output/test-PCINFO.txt
	../../NexRv.exe -conv -pcinfo ./output/test-PCINFO.txt -pconly test-PCONLY.txt -pcseq ./output/test-PCSEQ.txt
	echo No Branch History Message - only Direct Branch Message
	../../NexRv.exe -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin -nobhm
	../../NexRv.exe -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv.exe -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	diff -s -q ./test-PCONLY.txt ./output/test-PCOUT.txt
	echo  No Repeat Branch Message ...
	../../NexRv.exe -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin -norbm
	../../NexRv.exe -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv.exe -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	diff -s -q ./test-PCONLY.txt ./output/test-PCOUT.txt
	echo  With Repeat Branch Message - default
	../../NexRv.exe -enco ./output/test-PCSEQ.txt -nex ./output/test-NEX.bin
	../../NexRv.exe -dump ./output/test-NEX.bin ./output/test-DUMP.txt
	../../NexRv.exe -deco ./output/test-NEX.bin -pcinfo ./output/test-PCINFO.txt -pcout ./output/test-PCOUT.txt
	diff -s -q ./test-PCONLY.txt ./output/test-PCOUT.txt

xOBJD:
	riscv64-unknown-elf-objdump -d test.elf >test-OBJD.txt 
