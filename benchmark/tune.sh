#!/bin/sh

rm bench_{pb,fb,cp,nb}
make -j CFLAGS='-g -Wall -O2 -DNDEBUG -fprofile-generate'

echo "=======Tuning======="
for x in pb fb cp nb; do
	echo $x...
	./bench_$x
done

rm bench_{pb,fb,cp,nb}
make -j CFLAGS='-g -Wall -O2 -DNDEBUG -fprofile-use'

for x in pb fb cp nb; do
	echo -e "\n=======$x======="
	./bench_$x
	echo "size (raw): "`wc -c < $x.bin`
	echo "size (gzipped): "`gzip < $x.bin | wc -c`
done
