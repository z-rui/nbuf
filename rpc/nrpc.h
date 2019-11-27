#include "nbuf.h"
#include <uv.h>

typedef void (*rpc_rd_cb)(uv_stream_t *, ssize_t nread, struct nbuf_buffer);
typedef void (*rpc_wr_cb)(uv_write_t *, int status, struct nbuf_buffer);

extern int nrpc_read(uv_stream_t *, struct nbuf_buffer, rpc_rd_cb);
extern int nrpc_write(uv_write_t *, uv_stream_t *, struct nbuf_buffer, rpc_wr_cb);
