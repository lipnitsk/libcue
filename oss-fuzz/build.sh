#!/bin/bash -eu

cd $WORK
cmake -DCMAKE_BUILD_TYPE=Release $SRC/libcue
make

fuzzer_name=fuzz
cp ${fuzzer_name} $OUT/${fuzzer_name}
cp $SRC/libcue/oss-fuzz/cue.dict $OUT/${fuzzer_name}.dict

find $SRC/libcue/t -iname "*.cue" | \
     xargs zip $OUT/${fuzzer_name}_seed_corpus.zip
find $SRC/libcue/oss-fuzz/corpus -iname "*.cue" | \
     xargs zip $OUT/${fuzzer_name}_seed_corpus.zip

echo -en "[libfuzzer]\nclose_fd_mask=2\n" >$OUT/${fuzzer_name}.options
