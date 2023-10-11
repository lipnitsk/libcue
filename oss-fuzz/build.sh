#!/bin/bash -eu

mkdir bin
cd bin
cmake -DBUILD_FUZZER=ON -DCMAKE_BUILD_TYPE=Release ../
make

fuzzer_name=fuzz
cp ${fuzzer_name} $OUT/${fuzzer_name}
cp $SRC/libcue/oss-fuzz/cue.dict $OUT/${fuzzer_name}.dict

find $SRC/libcue/t -iname "*.cue" | \
     xargs zip $OUT/${fuzzer_name}_seed_corpus.zip

echo -en "[libfuzzer]\nclose_fd_mask=2\n" >$OUT/${fuzzer_name}.options
