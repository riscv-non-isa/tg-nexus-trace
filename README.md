# TG RISC-V Nexus Trace
## Curent Status

* 2024/02/28 - more clarification changes

  * N-Trace almost done (only synchronization messages clarifications is TODO).

* 2024/02/26 - more clarification changes

  * Made all 3 PDFs same date and version (2024-02-26 and 1.0.0_rc13)  .
  * Fixed generation of Trace Control PDF (broken yesterday).
  * Fixed PDF column widths and several small fixes.
  * Links to other specifications replaced by names.
  * All PDFs (1.0.0_rc13) are here: [Build artifacts](https://github.com/riscv-non-isa/tg-nexus-trace/actions/runs/8056565103)

* 2024/02/23 - more clarification changes

* 2024/02/22 - clarification changes (before doing synchronization cleanup)

* 2023/11/27 - initial review of ARC issues provided as 3 google-doc files (for easier comments):
  
  * [Notes to N-Trace PDF](https://docs.google.com/document/d/1h__c0Kc7TQAWMh5bw9cNC9bl_IGqyY_ylPV14uc2xj0)
  * [Notes to Trace Control PDF](https://docs.google.com/document/d/1u-38MaR0gwWTkSDfgIp7YXk9xNdySPZNIA50UENZQWE)
  * [Notes to Trace Connectors PDF](https://docs.google.com/document/d/1iNbB7-nTiQQ4vQBzLqxtBDQxT51QeUQMd9MFOe0jsBs)

* Feedback from ARC provided (as set of github issues)

* 1.0.0-rc9 2023/9/10 for ARC Review

  * Updates after emails to TG (detailed PDF with report emailed to TG).
 
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
