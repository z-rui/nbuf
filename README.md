# Naïve buffers
![Naïve!](https://upload.wikimedia.org/wikipedia/commons/thumb/4/44/Moha_example.svg/240px-Moha_example.svg.png)

This is an experiment on binary file formats.  A naïve format was designed (see `spec.txt`) and read/write API was implemented.

Benchmark code for Cap'n Proto, FlatBuffers, and Protocol Buffers is included.

# Schema compiler

A schema compiler is implemented in `cmd/nbufc/`.
Please refer to the `.l` and `.y` for the syntax.

The compiler compiles a text schema into binary form,
whose format is specified by the "meta"-schema `lib/nbuf.nbs`.

By default, the compiler outputs a C header file.
Code generation is implemented in `lib/b2h.c`.

# TODO

To make the project more useful, the following are to be implemented:

* Reference of the schema language.
* Reference of the generated API.
* A binary <-> text converter for any schema.
