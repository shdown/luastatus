Just for laughs, we fuzzed the JSON escaping function (see FUZZING.md), and AFL hasn't identified any issues.

The harness can be found in `fuzz/harness.c`.

The workflow is something like this:
1. Build AFL, set `AFL_ROOT` environment variable accordingly.
2. Compile the harness: `CC=$AFL_ROOT/afl-gcc ./fuzz/build.sh`
3. Add some test cases to `./fuzz/testcases/`
4. Create a directory for findings: `mkdir ./fuzz/findings`
5. Run the fuzzer: `$AFL_ROOT/afl-fuzz -i ./fuzz/testcases -o ./fuzz/findings ./fuzz/harness @@`
