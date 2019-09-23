#include <time.h>
#include <stdio.h>

#define BENCHMARK(expr, n) do { \
	int _n = (n); \
	clock_t start, stop; \
	start = clock(); \
	while (_n--) (expr); \
	stop = clock(); \
	fprintf(stderr, #expr ": %lfns/op\n", \
		(stop - start) * 1e9 / (n) / CLOCKS_PER_SEC); \
} while (0)
