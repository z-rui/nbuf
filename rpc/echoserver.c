#include "nrpc.h"
#include "echo.nb.h"

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

static void
on_write_end(uv_write_t *write_req, int status, struct nbuf_buffer buf)
{
	if (status < 0)
		die("write failed: %d", status);
	free(write_req);
	nbuf_free(&buf);
}

static void
on_read(uv_stream_t *stream, ssize_t nread, struct nbuf_buffer buf)
{
	EchoReq req;
	EchoResp resp;
	const char *name;
	char *msg;
	size_t namelen, msglen;
	uv_write_t *write_req;

	if (nread < 0)
		die("read failed: %d", (int) nread);

	write_req = malloc(sizeof *write_req);
	CHECK(write_req != NULL);

	req = get_EchoReq(&buf);
	name = EchoReq_name(req, &namelen);
	CHECK(name != NULL);
	if (namelen) {
		msglen = namelen + 8;
		msg = malloc(msglen + 1);
		CHECK(msg != NULL);
		sprintf(msg, "hello, %s!", name);
	} else {
		msg = "hello!";
		msglen = 6;
	}
	nbuf_clear(&buf);
	resp = new_EchoResp(&buf);
	EchoResp_set_timestamp(resp, time(NULL));
	EchoResp_set_msg(resp, msg, msglen);
	if (namelen)
		free(msg);
	CHECK_OK(nrpc_write(write_req, stream, buf, on_write_end));
}

static void
on_connect(uv_stream_t *server, int status)
{
	struct nbuf_buffer buf;
	uv_tcp_t *conn;

	if (status)
		die("connect failed: %d", status);
	conn = malloc(sizeof *conn);
	CHECK(conn != NULL);
	CHECK_OK(uv_tcp_init((uv_loop_t *) server->data, conn));
	conn->data = (void *) server;
	CHECK_OK(uv_accept(server, (uv_stream_t *) conn));
	nbuf_init_write(&buf, NULL, 0);
	CHECK_OK(nrpc_read((uv_stream_t *) conn, buf, on_read));
}

static int
run_server(uv_loop_t *loop, struct sockaddr *addr)
{
	uv_tcp_t server;

	CHECK_OK(uv_tcp_init(loop, &server));
	server.data = (void *) loop;
	CHECK_OK(uv_tcp_bind(&server, addr, /*flags=*/0));
	CHECK_OK(uv_listen((uv_stream_t *) &server, SOMAXCONN, on_connect));

	return uv_run(loop, UV_RUN_DEFAULT);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;

	if (argc != 2)
		die("usage: %s listen_port", argv[0]);

	CHECK_OK(uv_ip4_addr("0.0.0.0", atoi(argv[1]), &addr));

	return run_server(uv_default_loop(), (struct sockaddr *) &addr);
}
