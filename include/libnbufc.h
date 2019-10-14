#ifndef LIBNBUFC_H
#define LIBNBUFC_H

#include "nbuf.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void nbuf_b2h(struct nbuf_buffer *buf, FILE *fout, const char *srcname);
extern void nbuf_b2hh(struct nbuf_buffer *buf, FILE *fout, const char *srcname);
extern void nbuf_b2c(struct nbuf_buffer *buf, FILE *fout, const char *srcname);
extern void nbuf_b2t(struct nbuf_buffer *buf, FILE *fout);

#ifdef __cplusplus
}
#endif

#endif  /* LIBNBUFC_H */
