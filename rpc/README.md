# RPC example

This is an example demonstrating RPC via TCP using the naïve buffer format.
The code depends on [libuv](https://libuv.org).

```
./echoserver 12345
```
starts an echo server on port 12345.  To make RPC calls, use the CLI tool:

```
./nrpc_cli 127.0.0.1 12345 echo.bnbs EchoReq EchoResp <<EOF
> name: "super cow"
> EOF
```

The CLI tool creates an `EchoReq` message with the `name` field set to
"super cow" and sends the message to the echo server.
The echo server processes the request, and generates an `EchoResp` message:

```
timestamp: 123456789
msg: "hello, super cow!"
```
