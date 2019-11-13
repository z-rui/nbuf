#!/usr/bin/env python3

# Code generator for Python3

import sys

from nbuf_nb import Kind, Schema

def outhdr(schema, srcName):
    print("# Generated from %s, DO NOT EDIT.\n" % srcName)
    print("import enum\n")
    print("import nbuf")

def outenums(schema):
    for e in schema.enumTypes():
        print("\nclass %s(enum.IntEnum):" % e.name())
        for d in e.values():
            print("\t%s = %d" % (d.name(), d.value()))

def outgetter(f, schema):
    kind = f.kind()
    is_list = f.list()
    tag0 = f.tag0()
    tag1 = f.tag1()
    print("\tdef %s(self):" % f.name())
    if kind == Kind.ENUM:
        etype = schema.enumTypes()[tag1]
    elif kind == Kind.PTR:
        mtype = schema.msgTypes()[tag1]
    if is_list:
        if kind == Kind.PTR:
            typestr = mtype.name()
        elif kind == Kind.STR:
            typestr = 'nbuf.str_list'
        elif kind == Kind.BOOL:
            typestr = 'nbuf.bool_list'
        elif kind == Kind.FLOAT:
            typestr = 'nbuf.float_list'
        elif kind == Kind.ENUM:
            typestr = 'nbuf.enum_list(%s)' % etype.name()
        else:  # INT, UINT
            unsigned = kind == Kind.UINT
            typestr = 'nbuf.int_list(%d, %s)' % (tag1, unsigned)
        print("\t\treturn super().get_obj(%d, %s, [])" % (tag0, typestr))
    else:
        if kind == Kind.PTR:
            print("\t\treturn super().get_obj(%d, %s)" % (tag0, mtype.name()))
        elif kind == Kind.STR:
            print("\t\treturn super().get_str(%d)" % tag0)
        elif kind == Kind.BOOL:
            print("\t\treturn super().get_bool(%d)" % tag0)
        elif kind == Kind.FLOAT:
            print("\t\treturn super().get_float(%d, %d)" % (tag0, tag1))
        elif kind == Kind.ENUM:
            print("\t\treturn %s(super().get_int(%d, 2, True))" % (etype.name(), tag0))
        else:  # INT, UINT
            unsigned = kind == Kind.UINT
            print("\t\treturn super().get_int(%d, %d, %s)" % (tag0, tag1, unsigned))

def outsetter(f, schema):
    kind = f.kind()
    is_list = f.list()
    tag0 = f.tag0()
    tag1 = f.tag1()
    ssize = 0
    psize = 0

    if is_list:
        print("\tdef init_%s(self, n):" % f.name())
    elif kind == Kind.PTR:
        print("\tdef init_%s(self):" % f.name())
    else:
        print("\tdef set_%s(self, v):" % f.name())

    if kind == Kind.ENUM:
        etype = schema.enumTypes()[tag1]
    elif kind == Kind.PTR:
        mtype = schema.msgTypes()[tag1]

    if is_list or kind == Kind.PTR:
        if kind == Kind.PTR:
            ssize = mtype.ssize()
            psize = mtype.psize()
        elif kind == Kind.STR:
            psize = 1
        elif kind == Kind.BOOL:
            pass
        elif kind == Kind.ENUM:
            ssize = 2
        else:  # INT, UINT, FLOAT
            ssize = tag1
    if kind == Kind.PTR:
        print("\t\to = %s.new(self._buf, %s)" % (
            mtype.name(), 'n' if is_list else '1'))
        print("\t\tsuper().put_obj(%d, o)" % tag0)
        print("\t\treturn o")
    elif is_list:
        print("\t\to = super().new(self._buf, n, %d, %d)" % (ssize, psize))
        print("\t\tsuper().put_obj(%d, o)" % tag0)
        print("\t\treturn o")
    elif kind == Kind.STR:
        print("\t\treturn super().put_str(%d, v)" % tag0)
    elif kind == Kind.BOOL:
        print("\t\treturn super().put_bool(%d, v)" % tag0)
    elif kind == Kind.FLOAT:
        print("\t\treturn super().put_float(%d, v)" % (tag0, tag1))
    elif kind == Kind.ENUM:
        print("\t\treturn super().put_int(%d, 2, True, int(v))" % tag0)
    else:  # INT, UINT
        unsigned = kind == Kind.UINT
        print("\t\treturn super().put_int(%d, %d, %s, v)" % (tag0, tag1, unsigned))

def outmsgs(schema):
    for m in schema.msgTypes():
        print("\nclass %s(nbuf.obj):" % m.name())
        print("\t@classmethod")
        print("\tdef new(cls, buf=None, nelem=1):")
        print("\t\treturn super().new(buf, nelem, %d, %d)" % (
            m.ssize(), m.psize()))
        for f in m.fields():
            outgetter(f, schema)
            outsetter(f, schema)

def outftr(schema):
    pass

def b2py(schema, srcName):
    pkgName = schema.pkgName()
    outhdr(schema, srcName)
    outenums(schema)
    outmsgs(schema)
    outftr(schema)

if __name__ == '__main__':
    srcName = sys.argv[1]
    with open(srcName, 'rb') as f:
        buf = f.read()
        schema = Schema.from_buf(buf)
        b2py(schema, srcName)
