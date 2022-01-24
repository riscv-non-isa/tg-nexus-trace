SPEC := RISC-V-Trace-Control-Interface
HEADER_SOURCE := $(SPEC).adoc
PDF_RESULT := $(SPEC).pdf

all: build

build:

	@echo "Building asciidoc"
	asciidoctor-pdf \
		--attribute=mathematical-format=svg \
		--attribute=pdf-fontsdir=docs-resources/fonts \
		--attribute=pdf-style=docs-resources/themes/riscv-pdf.yml \
		--failure-level=ERROR \
		--out-file=$(PDF_RESULT) \
		$(HEADER_SOURCE)

clean:
	rm $(PDF_RESULT)
