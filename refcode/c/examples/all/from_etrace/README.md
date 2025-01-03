# from_etrace - Directory with files taken from E-Trace tests

This is set of files take as-is from E-Trace referenceFlow directory:

https://github.com/riscv-non-isa/riscv-trace-spec/tree/main/referenceFlow

N-Trace reference code only needs some files after spike was run.

File 'spike.tar.gz' (less than 7MB total) is stored in repo, so tests can be re-run.

It was created by running:

    tar -cvjf spike.tar.gz *filtered
    
    in referenceFlow/regression_20241211_154001/spike directory.
 
