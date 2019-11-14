# Python API

`b2py.py` generates Python API from the binary schema.  This feature may be merged into `nbufc` in the future.

At the moment, the generated code does not enforce the package name.
The user should put the generated code in `<pkgName>.py` for correct importing.

For each enum type, there is a corresponding generated class, which is a subclass of `enum.IntEnum`.
Same for message type, but subclass of `nbuf.obj`.
Refer to am enum value using `<enumType>.<enumValue>` such as `Color.RED`.
Alternatively, simply use the integer value (`enum.IntEnum` allows this).

The relationship between nbuf types and Python types are as follows.

| Type          | Python Type |
|---------------|-------------|
| integer       | `int`       |
| float/double  | `float`     |
| string        | `str`       |
| boolean       | `bool`      |
| enum          | generated enum type    |
| message       | generated message type |

## Read object from buffer
Use `<msgType>.from_buf(buf)` to extract a message from the buffer, where `buf` is a `bytes`-like object.
If `buf` is mutable (e.g., `bytearray`), then setters are allowed.  Otherwise, only getters are allowed.

## Getters

The getter is named `<fieldName>`

For array fields, the return value is a array-like object (`__getitem__` and `__len__` are provided).
If the element type is scalar or string, `__setitem__` is also provided, so one can assign to the
elements.

## Create an object in buffer
Use `<msgType>.new(buf)` to create a new message, where `buf` should be a `bytearray`-like object.

## Setters

* For scalar or string fields, the setter is named `set_<fieldName>`.
* For message fields, the method `init_<fieldName>` initializes this field.
* For array fields, the method `init_<fieldName>` initializes the array with a specified length.
