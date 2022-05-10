SPEC1 := RISC-V-Trace-Control-Interface
HEADER1_SOURCE := $(SPEC1).adoc
PDF1_RESULT := $(SPEC1).pdf

SPEC2 := RISC-V-Trace-Connectors
HEADER2_SOURCE := $(SPEC2).adoc
PDF2_RESULT := $(SPEC2).pdf

all: buildctrl buildcon 

buildctrl:
	@echo "Building PDFs with control"
	asciidoctor-pdf \
		--attribute=mathematical-format=svg \
		--attribute=pdf-fontsdir=docs-resources/fonts \
		--attribute=pdf-style=docs-resources/themes/riscv-pdf.yml \
		--failure-level=ERROR \
		--out-file=$(PDF1_RESULT) \
		$(HEADER1_SOURCE)

buildcon:
	@echo "Building PDF with connectors"
	asciidoctor-pdf \
		--attribute=mathematical-format=svg \
		--attribute=pdf-fontsdir=docs-resources/fonts \
		--attribute=pdf-style=docs-resources/themes/riscv-pdf.yml \
		--failure-level=ERROR \
		--out-file=$(PDF2_RESULT) \
		$(HEADER2_SOURCE)

clean:
	rm $(PDF1_RESULT) rm $(PDF2_RESULT)
