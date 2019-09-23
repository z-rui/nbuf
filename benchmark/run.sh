#!/bin/sh

make -j

for x in ./bench_??; do
	echo -e "\n=======$x======="
	$x
done

echo

gzip -kf *.bin
wc -c *.bin*
