# This is good UM for 'make': https://devhints.io/makefile

# TODO: Fix this hard-coded path somehow
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

# Embench tests
emb: embench-aha-mont64 embench-crc32 embench-cubic embench-edn embench-huffbench embench-matmult-int embench-minver embench-nbody embench-nettle-aes embench-nettle-sha256 embench-nsichneu embench-picojpeg embench-qrduino embench-sglib-combined embench-slre embench-st embench-statemate embench-ud embench-wikisort

# More tests
more: median mm mt-matmul mt-vvadd multiply qsort rsort spmv towers vvadd xrle

# Errors (as use exceptions/interrupts)
err: pmp br_j_asm

# These 2 use 'pk' - need to handle this
none: hello_world new_hw

# Options (for -enco ...)
# -nobhm
# -norbm

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
