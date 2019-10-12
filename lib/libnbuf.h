#ifndef LIBNBUF_H
#define LIBNBUF_H

#include "nbuf.h"
#include <stdio.h>

extern void
nbuf_b2h(struct nbuf_buffer *buf, FILE *fout, const char *srcname);
extern void
nbuf_b2t(struct nbuf_buffer *buf, FILE *fout);

#endif  /* LIBNBUF_H */
