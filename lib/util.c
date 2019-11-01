#include "nbuf.h"

#include <stdbool.h>
#include <stdio.h>

bool
nbuf_load_file(struct nbuf_buffer *buf, FILE *fin)
{
	int ch;
	while ((ch = getc(fin)) != EOF) {
		nbuf_reserve(buf, 1);
		buf->base[buf->len++] = ch;
	}
	return !ferror(fin);
}
