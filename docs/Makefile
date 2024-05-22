# Makefile for RISC-V Doc Template
#
# This work is licensed under the Creative Commons Attribution-ShareAlike 4.0
# International License. To view a copy of this license, visit
# http://creativecommons.org/licenses/by-sa/4.0/ or send a letter to
# Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
#
# SPDX-License-Identifier: CC-BY-SA-4.0
#
# Description:
# 
# This Makefile is designed to automate the process of building and packaging 
# the Doc Template for RISC-V Extensions.

DOCKER_RUN := docker run --rm -v ${PWD}:/build -w /build \
riscvintl/riscv-docs-base-container-image:latest

ASCIIDOCTOR_PDF := asciidoctor-pdf
OPTIONS := --trace \
		   -v \
           -a compress \
           -a mathematical-format=svg \
           -a pdf-fontsdir=docs-resources/fonts \
           -a pdf-style=./riscv-pdf-ISA.yml \
           --failure-level=ERROR
REQUIRES := --require=asciidoctor-bibtex \
            --require=asciidoctor-diagram \
            --require=asciidoctor-mathematical

.PHONY: all build clean

# Update all to include the new PDF files
all: RISC-V-Trace-Control-Interface.pdf RISC-V-Trace-Connectors.pdf RISC-V-N-Trace.pdf

# Define the build rules
build: all

# Clean up
clean:
	@echo "Cleaning up generated files..."
	rm -f RISC-V-Trace-Control-Interface.pdf RISC-V-Trace-Connectors.pdf RISC-V-N-Trace.pdf
	@echo "Cleanup completed."

# Generalized rule for building any PDF
%.pdf: %.adoc
	@echo "Building $* PDF"
	@if command -v docker >/dev/null 2>&1 ; then \
		echo "Docker is available, building inside Docker container..."; \
		$(DOCKER_RUN) /bin/sh -c "$(ASCIIDOCTOR_PDF) $(OPTIONS) $(REQUIRES) --out-file=$*.pdf $*.adoc"; \
	else \
		echo "Docker is not available, building without Docker..."; \
		$(ASCIIDOCTOR_PDF) $(OPTIONS) $(REQUIRES) --out-file=$*.pdf $*.adoc; \
	fi
