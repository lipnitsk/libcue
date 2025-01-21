#!/bin/bash -eu

cd $WORK
cmake -DBUILD_FUZZER=ON -DCMAKE_BUILD_TYPE=Release $SRC/libcue
make

for fuzzer_name in fuzz fuzz_full_coverage; do
	cp ${fuzzer_name} $OUT/${fuzzer_name}
	cp $SRC/libcue/oss-fuzz/cue.dict $OUT/${fuzzer_name}.dict

	find $SRC/libcue/t -iname "*.cue" | \
     		xargs zip $OUT/${fuzzer_name}_seed_corpus.zip
	find $SRC/libcue/oss-fuzz/corpus -iname "*.cue" | \
     		xargs zip $OUT/${fuzzer_name}_seed_corpus.zip

	echo -en "[libfuzzer]\nclose_fd_mask=2\n" >$OUT/${fuzzer_name}.options
done
