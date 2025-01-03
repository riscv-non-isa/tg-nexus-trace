# This is good UM for 'make': https://devhints.io/makefile

# TODO: Fix this hard-coded path somehow
RV_OBJDUMP=./from_etrace/bin_ubuntu/riscv64-unknown-elf-objdump

ifdef TST
ALLF=all$(TST)
else
ALLF=all
endif

# All tests ...
all: coremark dhrystone emb more
	grep Stat *-report.txt >$(ALLF).txt
	cat $(ALLF).txt
	wc -l $(ALLF).txt

# Shortycuts (to run 'make c' or 'make d' ...)
c: coremark
d: dhrystone
e: embench-nettle-aes

# 'make t' will allows to run some isolated test (for specific case)
t: t_embench-wikisort

tst:
	@$(MAKE) all TST=0
	@$(MAKE) all TST=1
	@$(MAKE) all TST=2
	@$(MAKE) all TST=3
	@$(MAKE) all TST=4

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
# Default test
ENCO_OPT=-cs 8 -rpt 2 
ifdef TST

ifeq ($(TST), 0)
ENCO_OPT=-nobhm
endif

ifeq ($(TST), 1)
ENCO_OPT=-cs 0 -rpt 0 
endif

ifeq ($(TST), 2)
ENCO_OPT=-cs 0 -rpt 2 
endif

ifeq ($(TST), 3)
ENCO_OPT=-cs 8 -rpt 0 
endif

endif
# ENCO_OPT=-norbm

# General processing for all files ...

#	tar --keep-newer-files --directory=./from_etrace -xvjf ./from_etrace/spike.tar.gz $*.spike_pc_trace_filtered 

%.spike_pc_trace_filtered: ./from_etrace/spike/spike.tar.gz
	echo Extracting $*.spike_pc_trace_filtered ...
	tar -xjf ./from_etrace/spike/spike.tar.gz $*.spike_pc_trace_filtered
	touch $*.spike_pc_trace_filtered

%: ./from_etrace/spike/spike.tar.gz ./from_etrace/test_files/%.riscv ../../NexRv.exe
	@echo "**** Processing $* ****"
	@$(MAKE) $*.spike_pc_trace_filtered
	cp ./$*.spike_pc_trace_filtered ./output/$*-pconly.txt
	$(RV_OBJDUMP) -d ./from_etrace/test_files/$*.riscv > $*-objd.txt
	../../NexRv.exe -conv -objd $*-objd.txt -pcinfo output/$*-pcinfo.txt
	../../NexRv.exe -conv -pcinfo ./output/$*-pcinfo.txt -pconly ./output/$*-pconly.txt -pcseq ./output/$*-pcseq.txt
	../../NexRv.exe -enco ./output/$*-pcseq.txt -nex ./output/$*-nex.bin $(ENCO_OPT)
	../../NexRv.exe -dump ./output/$*-nex.bin ./output/$*-dump.txt
	../../NexRv.exe -deco ./output/$*-nex.bin -pcinfo ./output/$*-pcinfo.txt -pcout ./output/$*-pcout.txt >$*-report.txt
	../../NexRv.exe -diff -pcseq ./output/$*-pcseq.txt -pcout ./output/$*-pcout.txt
	@echo
	@echo "**** $* PASSED OK ****"
	@echo

# You may need to modify this below ...
t_%:
	../../NexRv.exe -enco ./output/$*-pcseq.txt -nex ./output/$*-nex.bin $(ENCO_OPT) -full >x.txt
	../../NexRv.exe -dump ./output/$*-nex.bin ./output/$*-dump.txt
	../../NexRv.exe -deco ./output/$*-nex.bin -pcinfo ./output/$*-pcinfo.txt -pcout ./output/$*-pcout.txt -full >$*-report.txt
	../../NexRv.exe -diff -pcseq ./output/$*-pcseq.txt -pcout ./output/$*-pcout.txt

clean:
	rm -f *objd.txt *.spike_pc_trace_filtered *-report.txt output/*-*txt output/*-nex.bin
	@echo
	@echo "** Remaining in ./output **"
	ls output
	@echo
