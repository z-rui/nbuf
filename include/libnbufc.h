#ifndef LIBNBUFC_H
#define LIBNBUFC_H

#include "nbuf.h"

#include <stdio.h>

extern void nbuf_b2h(struct nbuf_buffer *buf, FILE *fout, const char *srcname);
extern void nbuf_b2r(struct nbuf_buffer *buf, FILE *fout, const char *srcname);
extern void nbuf_b2t(struct nbuf_buffer *buf, FILE *fout);

#endif  /* LIBNBUFC_H */
