# Naïve buffers
![Naïve!](https://upload.wikimedia.org/wikipedia/commons/thumb/4/44/Moha_example.svg/240px-Moha_example.svg.png)

This is an experiment on binary file formats.

## Schema compiler

A schema compiler is implemented in `nbufc/`.
Please refer to the `.l` and `.y` for the syntax.

The compiler compiles a text schema into binary form,
whose format is specified by the "meta"-schema `lib/nbuf.nbs`.

Basic schema structure looks like this:
```
package pkgName

enum EnumType1 {
	name1 = value1
	name2 = value2
	...
}

...

message MsgType1 {
	field1: type1
	field2: type2
	...
}
...
```

Type can be any message type, enum type, or one of the following
built-in types:

* `bool`
* `{uint,int}{8,16,32,64}`
* `float`, `double`
* `string`

Or, a list of `type` can be specified as `[]type`.

## Generated C API

The schema compiler can generate a C header which contains inline functions
for accessing the data types defined in the schema.

Use `nbuf_init_read` or `nbuf_init_write` to create a buffer.
Then use `pkgName_get_MsgType()` to get an object of that type from a read
buffer, or `pkgName_new_MsgType()` to create an object to be written
to the write buffer.

* Enum types are named like `pkgName_EnumType1`, and its values are named like
`pkgName_EnumType1_value1`, etc.
* Message types are named like `pkgName_MsgType1`.
* The getter of `field1` is named `pkgName_MsgType1_field1()`.
* The setter is named `pkgName_MsgType1_set_field1()`.
* For list fields, there is an additional argument in the getter/setter
to indicate the index.  Additionally, `pkgName_MsgType1_field1_size()`
returns the size of the list.
* For string fields, there is an additional argument in the getter/setter
for the length.  Set the argument to NULL in the getter to ignore the length,
and set to -1 in the setter to let the setter determine the length.
* If the field is a submessage, there is no setter, but one can get the
submessage and then use its setter to set the fields in the submessage.

Note: if package name is undefined, then drop the `pkgName_` part.

## Generated C++ API

The schema compiler can also generate a C++ header.
This is mostly the same as the C API, expect some syntactic changes.

* `pkgName` becomes a namespace, so `pkgName_xxx` now becomes `pkgName::xxx`.
  * The user can use `using namespace` to avoid repeating `pkgName::`.
* The enum types are defined as `enum class`es.
  * `pkgName::EnumType1` refers to the type.
  * `pkgName::EnumType1::value1` refers to the value.
* The message types now have methods.
  * `pkgName_MsgType1_xxx()` in the C API becomes `pkgName::MsgType1::xxx()`.
  * For example, `pkgName_MsgType1_set_field1(&msg, val)` now becomes
    `msg.set_field1(val)`, which is less clumsy.

## Reflection

Experimental and incomplete.  Refer to `example/` for usage.

## Benchmark 

Benchmark code for Cap'n Proto, FlatBuffers, and Protocol Buffers is included.

## TODO

To make the project more useful, the following are to be implemented:

* Reference of the schema language.
* Reference of the generated API.
* A binary <-> text converter for any schema.
