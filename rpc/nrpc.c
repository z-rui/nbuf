#include "nbuf.h"
#include "nrpc.h"

struct rpc_rd_ctx {
	struct nbuf_buffer buf;
	rpc_rd_cb cb;
	void *data;
	size_t buffer_len;
	char buffer_base[0];
};

static void
on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *uvbuf)
{
	struct rpc_rd_ctx *ctx;
	struct nbuf_buffer *buf;
	struct nbuf_buffer buf1;
	size_t msg_len = 0;
	rpc_rd_cb read_done;

	if (nread < 0)
		goto done;
	ctx = (struct rpc_rd_ctx *) stream->data;
	buf = &ctx->buf;
	nbuf_reserve(buf, nread);
	memcpy(buf->base + buf->len, uvbuf->base, nread);
	buf->len += nread;

	if (buf->len >= NBUF_WORD_SZ)
		msg_len = nbuf_read_int_safe(buf, 0, NBUF_WORD_SZ);
	if (nread > 0 && (!msg_len || buf->len < msg_len))
		return;  /* more to read */
	if (msg_len == 0 || buf->len != msg_len || uv_read_stop(stream) != 0) {
		nread = -1;
		goto done;
	}
	nread = msg_len;
	nbuf_write_int_safe(buf, 0, NBUF_WORD_SZ, 1);
done:
	read_done = ctx->cb;
	buf1 = *buf;
	stream->data = ctx->data;
	free(ctx);
	read_done(stream, nread, buf1);
}

static void
alloc_cb(uv_handle_t *handle, size_t suggested_size, struct uv_buf_t *buf)
{
	struct rpc_rd_ctx *ctx;
	ctx = (struct rpc_rd_ctx *) handle->data;
	buf->base = ctx->buffer_base;
	buf->len = ctx->buffer_len;
}

int
nrpc_read(uv_stream_t *stream, struct nbuf_buffer buf, rpc_rd_cb cb)
{
	struct rpc_rd_ctx *ctx;
	const size_t kBufferSize = 0x10000;

	ctx = malloc(sizeof *ctx + kBufferSize);
	if (ctx == NULL)
		return -1;

	ctx->buf = buf;
	ctx->cb = cb;
	ctx->buffer_len = kBufferSize;
	ctx->data = stream->data;
	stream->data = (void *) ctx;
	return uv_read_start(stream, alloc_cb, on_read);
}

struct rpc_wr_ctx {
	uv_buf_t buf;
	rpc_wr_cb cb;
	void *data;
};

static void
on_write_end(uv_write_t *req, int status)
{
	struct rpc_wr_ctx *ctx = (struct rpc_wr_ctx *) req->data;
	struct nbuf_buffer buf;
	rpc_wr_cb write_done = ctx->cb;

	nbuf_init_write(&buf, ctx->buf.base, ctx->buf.len);
	req->data = ctx->data;
	free(ctx);
	write_done(req, status, buf);
}

int
nrpc_write(uv_write_t *req, uv_stream_t *stream, struct nbuf_buffer buf, rpc_wr_cb cb)
{
	struct rpc_wr_ctx *ctx;

	ctx = malloc(sizeof *ctx);
	if (ctx == NULL)
		return -1;

	ctx->buf.base = buf.base;
	ctx->buf.len = buf.len;
	ctx->cb = cb;
	ctx->data = req->data;
	req->data = (void *) ctx;
	nbuf_write_int_safe(&buf, 0, NBUF_WORD_SZ, buf.len);
	return uv_write(req, stream, &ctx->buf, /*num_bufs=*/1, on_write_end);
}
