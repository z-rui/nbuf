# Naïve buffers
![Naïve!](https://upload.wikimedia.org/wikipedia/commons/thumb/4/44/Moha_example.svg/240px-Moha_example.svg.png)

This is an experiment on binary file formats.  A naïve format was designed (see `spec.txt`) and read/write API was implemented.

Benchmark code for Cap'n Proto, FlatBuffers, and Protocol Buffers is included.

To make the project more useful, the following are to be implemented:
* A schema language.
* A code generator based on the schema.
* A binary <-> text converter.
