#include "libnbuf.h"
#include "nrpc.h"

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <uv.h>

#define die(fmt, ...) do { \
	fprintf(stderr, "%s:%d: " fmt "\n", __FILE__, __LINE__, ## __VA_ARGS__); \
	exit(1); \
} while (0)

#define CHECK(exp) while (!(exp)) die("CHECK: %s", #exp)
#define CHECK_OK(exp) while (exp) die("CHECK_OK: %s", #exp)

nbuf_Schema schema_;
nbuf_MsgType reqtype_, resptype_;

static void
cleanup(void)
{
	if (schema_.o.buf)
		nbuf_free(schema_.o.buf);
}

static void
load_schema(const char *path, const char *req_type, const char *resp_type)
{
	static struct nbuf_buffer buf;
	FILE *f;

	f = fopen(path, "rb");
	if (f == NULL)
		die("error opening %s: %s", path, strerror(errno));
	nbuf_init_write(&buf, NULL, 0);
	CHECK(nbuf_load_file(&buf, f));
	fclose(f);

	schema_ = nbuf_get_Schema(&buf);
	atexit(cleanup);

	if (!nbuf_Schema_msgType_by_name(&reqtype_, &schema_, req_type))
		die("undefined message type: %s", req_type);
	if (!nbuf_Schema_msgType_by_name(&resptype_, &schema_, resp_type))
		die("undefined message type: %s", resp_type);
}

static void
on_read(uv_stream_t *stream, ssize_t nread, struct nbuf_buffer buf)
{
	struct nbuf_obj o;

	if (nread < 0)
		die("read failed: %d", (int) nread);
	o.buf = &buf;
	nbuf_obj_init(&o, 0);
	nbuf_print(&o, stdout, /*indent=*/2, &schema_, &resptype_);
	nbuf_free(&buf);
}

static void
on_write_end(uv_write_t *req, int status, struct nbuf_buffer buf)
{
	if (status)
		die("write failed: %d", status);
	nbuf_clear(&buf);
	CHECK_OK(nrpc_read(req->handle, buf, on_read));
}

static void
on_connect(uv_connect_t *req, int status)
{
	struct nbuf_buffer buf;
	struct nbuf_lexer lex;
	static uv_write_t write_req;

	if (status)
		die("connect failed: %d", status);
	nbuf_lex_init(&lex, stdin);
	nbuf_init_write(&buf, NULL, 0);
	if (!nbuf_parse(&buf, &lex, &schema_, &reqtype_))
		die("failed to parse request");

	CHECK_OK(nrpc_write(&write_req, req->handle, buf, on_write_end));
}

static int
run_client(uv_loop_t *loop, struct sockaddr *addr)
{
	uv_tcp_t client;
	uv_connect_t connect_req;

	CHECK_OK(uv_tcp_init(loop, &client));
	CHECK_OK(uv_tcp_connect(&connect_req, &client, addr, on_connect));
	return uv_run(loop, UV_RUN_DEFAULT);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in addr;

	if (argc != 6)
		die("usage: %s host port bin_schema_file req_msg resp_msg",
			argv[0]);
	CHECK_OK(uv_ip4_addr(argv[1], atoi(argv[2]), &addr));
	load_schema(argv[3], argv[4], argv[5]);
	return run_client(uv_default_loop(), (struct sockaddr *) &addr);
}
