#1/bin/bash

set -e

pushd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null

mkdir -p build_fuzz
pushd build_fuzz
mkdir -p fuzz_inputs

find ../tests/ -name "*.lsl" | grep -v "/expected/" | xargs -I'{}' cp '{}' fuzz_inputs/
CXX=clang++ CC=clang cmake .. -DTAILSLIDE_BUILD_FUZZER=on
cmake --build .
./tailslide-fuzzer ./fuzz_inputs/ -fork=8
