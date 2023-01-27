# TG RISC-V Nexus Trace
Working repository of the RISC-V Nexus Trace TG report.  Nexus Trace TG Home page is [here](https://lists.riscv.org/g/tech-nexus).

Reference C code for Nexus Trace dumper/encoder/decoder is [here](./refcode/c) - documentation as README.md file is provided.

File with Nexus Messages is [NexusTrace-TG-Messages.adoc](./docs/NexusTrace-TG-Messages.adoc).

Details of messages is provided here [NexusTrace-TG-MessageDetails.adoc](./docs/NexusTrace-TG-MessageDetails.adoc).

Proposed trace connectors are provided here [NexusTrace-TG-Connectors.adoc](./docs/NexusTrace-TG-Connectors.adoc).

RISC-V Trace Control Interface is [here](./docs/RISC-V-Trace-Control-Interface.adoc) (it was converted from original PDF version [here](https://lists.riscv.org/g/tech-nexus/files/RISC-V-Trace-Control-Interface-Proposed-20200612.pdf)).

Clicking on ADOC file in the github repo viewer will render a usable version as markdown.

For a better rendering, use "asciidoctor name.adoc".

This work is licensed under a Creative Commons Attribution 4.0
International License. See the LICENSE file for details.

### Initial Work (Preserved)

For initial document v0.01 (as PDF from MS Word), click [here](./pdfs/RISC-V-Nexus-Trace-Spec-2019-10-29.pdf).
Same file in ADOC format is here: [TG-RISC-V-Nexus-Trace.adoc](./docs/initial/RISC-V-Nexus-Trace-Spec.adoc).

### Additional Resources

- The [Nexus IEEE-ISTO-5001 Standard (2012-v3.0.1)](./docs/nexus-standard/IEEE-ISTO-5001-2012-v3.0.1-Nexus-Standard.pdf) PDF file.

### Documentation generator

PDF version of specification should be generated using Actions menu. See below for more details.

### Dependencies
The PDF built in this project uses AsciiDoctor (Ruby). For more information
on AsciiDoctor, specification guidelines, or building locally, see the 
[RISC-V Documentation Developer Guide](https://github.com/riscv/docs-dev-guide).

### Cloning the project
This project uses 
[GitHub Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) 
to include the RISC-V 
[docs-resources project](https://github.com/riscv/docs-resources)
to achieve a common look and feel.

When cloning this repository for the first time, you must either use 
`git clone --recurse-submodules` or execute `git submodule init` and 
`git submodule update` after the clone to populate the `docs-resources` 
directory. Failure to clone the submodule, will result in the PDF build 
fail with an error message like the following:

```
$ make
asciidoctor-pdf \
-a toc \
-a compress \
-a pdf-style=docs-resources/themes/riscv-pdf.yml \
-a pdf-fontsdir=docs-resources/fonts \
--failure-level=ERROR \
-o profiles.pdf profiles.adoc
asciidoctor: ERROR: could not locate or load the built-in pdf theme `docs-resources/themes/riscv-pdf.yml'; reverting to default theme
No such file or directory - notoserif-regular-subset.ttf not found in docs-resources/fonts
  Use --trace for backtrace
make: *** [Makefile:7: profiles.pdf] Error 1
```
