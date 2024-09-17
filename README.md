# TG RISC-V Nexus Trace

## Tracking Ratification Ready

Public Review period closed on Sep 14-th.

Summary of **Public Review** notes with short status for each issue or PR (DONE=merged, TODO=not yet, NEW=not analyzed, OK=nothing done - spec clarifies it already):

As of 2024/09/16 (**Public Review** closed, all known notes addressed).

* #64, TYPO: Fix typos, Status=**DONE**.
* #65, CLARIFY: B-TYPE=0 for some SYNC messages, Status=**DONE**.
* #66, TYPO: Fix typos: . and ( and :, Status=**DONE**.
* #67, NO CHANGE: Should read pointer be used for SMEM mode, Status=**OK** (explanation provided, issue closed).
* #68, TYPO: HIST field bit in example pseudo-code are reversed, Status=**DONE**.
* #69, NO CHANGE: Question why SRC ID is not 15-bit, Status=**OK** (explanation provided, issue closed).

## Status of each PDF

**2024/08/14 status:**

PDFs 1.0_rc50: **Frozen**.

Inside of [./pdfs](./pdfs) directory:

* **N-Trace PDF**: Version 1.0_rc50. **Frozen**.
* **Controls PDF**: Version 1.0_rc50. **Frozen**.
* **Connectors PDF**: Version 1.0_rc50. **Frozen**.

## TODO (after ratification approval)

* TODO: Change status to Ratified.
* TODO: Change date.
* TODO: Remove _rc and make version 1.0.
* TODO: Make official release.
* TODO: Place all 3 PDFs into 'public' place (and update/add links).

## Repository Overview

Working repository of the RISC-V Nexus Trace TG report.  Nexus Trace TG Home page is [here](https://lists.riscv.org/g/tech-nexus).

* Main N-Trace specification is defined here: [RISC-V-N-Trace.adoc](./docs/RISC-V-N-Trace.adoc) and PDF version is here: [PDF](./pdfs/RISC-V-N-Trace.pdf).

* Trace Control Interface is defined here: [RISC-V-Trace-Control-Interface.adoc](./docs/RISC-V-Trace-Control-Interface.adoc)  and PDF version is here: [PDF](./pdfs/RISC-V-Trace-Control-Interface.pdf).

* Trace connectors are defined here: [RISC-V-Trace-Connectors.adoc](./docs/RISC-V-Trace-Connectors.adoc) and PDF version is here: [PDF](./pdfs/RISC-V-Trace-Connectors.pdf).

Clicking on ADOC file in the github repo viewer will render a usable version as markdown.

For a better rendering to PDF, use Actions in main menu above.

Reference C code for Nexus Trace dumper/encoder/decoder is [here](./refcode/c) - documentation as README.md file is provided.

This work is licensed under a Creative Commons Attribution 4.0
International License. See the LICENSE file for details.

## Initial Work (Preserved)

For initial document v0.01 (as PDF from MS Word), click [here](./pdfs/RISC-V-Nexus-Trace-Spec-2019-10-29.pdf).
Same file in ADOC format is here: [TG-RISC-V-Nexus-Trace.adoc](./docs/initial/RISC-V-Nexus-Trace-Spec.adoc).

## Additional Resources

- The [Nexus IEEE-ISTO-5001 Standard (2012-v3.0.1)](./docs/nexus-standard/IEEE-ISTO-5001-2012-v3.0.1-Nexus-Standard.pdf) PDF file.

## Documentation generator

PDF version of specification should be generated using Actions menu. See below for more details.

## Dependencies
The PDF built in this project uses AsciiDoctor (Ruby). For more information
on AsciiDoctor, specification guidelines, or building locally, see the 
[RISC-V Documentation Developer Guide](https://github.com/riscv/docs-dev-guide).

## Cloning the project
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
