# Naïve buffers
![Naïve!](https://upload.wikimedia.org/wikipedia/commons/thumb/4/44/Moha_example.svg/240px-Moha_example.svg.png)

This is an experiment on binary file formats.

The following features are implemented:

* In-buffer (non-copying) read / write operations.
* Types: scalar, string, array.
* A schema language and schema compiler (`nbufc`).
* C/C++ API generator.
* Human-readable message format, printer and parser
  as an library or standalone commands (`nbufc e`) / (`nbufc d`).

## Benchmark 

Benchmark code for Cap'n Proto, FlatBuffers, and Protocol Buffers is included.

Although naïve, the implementation is actually reasonably fast!
Here's the data from a single run on my computer:

```
=======./bench_cp=======
test_write(buf): 380.244500ns/op
test_read(*buf): 167.331500ns/op

=======./bench_fb=======
test_write(buf): 261.171200ns/op
test_verify(buf): 44.627600ns/op
test_read(buf): 42.948000ns/op

=======./bench_nb=======
test_write(&buf): 53.542400ns/op
test_read(&buf): 70.423200ns/op

=======./bench_pb=======
test_create(&arena): 389.898000ns/op
test_serialize(&buf): 240.234000ns/op
test_deserialize(buf): 751.118000ns/op
test_use(): 290.291000ns/op
```

## TODO

To make the project more useful, the following are to be implemented:

* Reference of the schema language.
* Reference of the generated API.
* Union ("oneof") type.
