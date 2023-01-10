all: 	RISC-V-Trace-Control-Interface.pdf RISC-V-Trace-Connectors.pdf 

# This will build any PDF (we have 4 of them ...)
%.pdf: %.adoc
	@echo "Building $* PDF"
	asciidoctor-pdf \
		--attribute=mathematical-format=svg \
		--attribute=pdf-fontsdir=docs-resources/fonts \
		--attribute=pdf-style=docs-resources/themes/riscv-pdf.yml \
		--failure-level=ERROR \
		--require=asciidoctor-diagram \
		--require=asciidoctor-mathematical \
		--out-file=$*.pdf \
		$*.adoc

clean:
	rm *.pdf
