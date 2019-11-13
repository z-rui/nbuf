# nbuf Python3 support lib

import struct

import enum

class obj:
    def __init__(self, buf, base, nelem, ssize, psize):
        self._buf = buf
        self._base = base
        self._nelem = nelem
        self._ssize = ssize
        self._psize = psize
        self._elemsz = max(ssize + 4*psize, 1)
    def __repr__(self):
        return 'nbuf.obj(buf={}, base={}, nelem={}, ssize={}, psize={})'.format(
            id(self._buf), self._base, self._nelem, self._ssize, self._psize)
    def __len__(self):
        return self._nelem
    def __getitem__(self, i):
        if i >= self._nelem:
            raise IndexError('index out of bound')
        base = self._base + i * self._elemsz
        o = type(self)(self._buf, base, 1, self._ssize, self._psize)
        return o
    @classmethod
    def from_buf(cls, buf, base=0):
        #print('from_buf:', len(buf), base)
        nelem, ssize, psize = struct.unpack_from('IHH', buf, base)
        return cls(buf, base + 8, nelem, ssize, psize)
    @classmethod
    def new(cls, buf=None, nelem=1, ssize=0, psize=0):
        if buf == None:
            buf = bytearray()
        base = len(buf)
        elemsz = ssize + 4*psize
        totalsz = 8
        if elemsz > 0:
            totalsz += elemsz * nelem
        else:
            totalsz += (nelem + 7) >> 3
        totalsz = (totalsz + 7) >> 3 << 3
        buf.extend(bytes(totalsz))
        struct.pack_into('IHH', buf, base, nelem, ssize, psize)
        return cls(buf, base + 8, nelem, ssize, psize)
    def _scalar_ptr(self, offs, size):
        if offs + size > self._ssize:
            raise IndexError('scalar offset out of bound')
        ptr = self._base + offs
        assert ptr % size == 0
        return ptr
    def _get(self, fmt, offs, size):
        ptr = self._scalar_ptr(offs, size)
        (v,) = struct.unpack_from(fmt, self._buf, ptr)
        return v
    def _put(self, fmt, offs, size, v):
        ptr = self._scalar_ptr(offs, size)
        struct.pack_into(fmt, self._buf, ptr, v)
    @staticmethod
    def _int_fmt(size, unsigned):
        if size == 1:
            fmt = '<b'
        elif size == 2:
            fmt = '<h'
        elif size == 4:
            fmt = '<i'
        elif size == 8:
            fmt = '<q'
        else:
            raise ValueError('bad integer size: %d' % size)
        if unsigned:
            fmt = fmt.upper()
        return fmt
    def get_int(self, offs, size, unsigned):
        return self._get(obj._int_fmt(size, unsigned), offs, size) or 0
    def put_int(self, offs, size, unsigned, v):
        self._put(obj._int_fmt(size, unsigned), offs, size, v)
    @staticmethod
    def _float_fmt(size):
        if size == 4:
            fmt = '<f'
        elif size == 8:
            fmt = '<d'
        else:
            raise ValueError('bad float size: %d' % size)
        return fmt
    def get_float(self, offs, size):
        return self._get(obj._float_fmt(size), offs, size) or 0.0
    def put_float(self, offs, size):
        self._put(obj._float_fmt(size), offs, size)
    def get_bool(self, bitoffs):
        i = bitoffs >> 3
        j = bitoffs & 7
        x = self.get_int(i, 1, True)
        return (x & (1 << j)) != 0
    def put_bool(self, bitoffs, v):
        i = bitoffs >> 3
        j = bitoffs & 7
        x = self.get_int(i, 1, True)
        if v:
            x |= 1 << j
        else:
            x &= ~(1 << j)
        self.put_int(i, 1, True, x)
    def _ptr_ptr(self, idx):
        if idx >= self._psize:
            raise IndexError("pointer index out of bound")
        offs = self._ssize + 4*idx
        return self._base + offs
    def _obj_ptr(self, idx):
        ptr = self._ptr_ptr(idx)
        (rel,) = struct.unpack_from('i', self._buf, ptr)
        #print('_obj_ptr:', ptr, rel)
        if rel < 0:
            raise ValueError("negative pointer value on wire")
        if rel == 0:
            return None
        return ptr + 4*rel
    def has_obj(self, idx):
        return self._get_ptr(idx) is not None
    def get_obj(self, idx, cls=None, default=None):
        ptr = self._obj_ptr(idx)
        if ptr is None:
            return default
        return (cls or obj).from_buf(self._buf, ptr)
    def put_obj(self, idx, v):
        ptr = self._ptr_ptr(idx)
        rel = (v._base - 8 - ptr) >> 2
        if rel <= 0:
            raise ValueError("backward reference in message")
        struct.pack_into('i', self._buf, ptr, rel)
    def get_str(self, idx, encoding='utf-8'):
        o = self.get_obj(idx)
        if o is None:
            return ""
        assert o._ssize == 1 and o._psize == 0
        fmt = str(o._nelem-1) + 's'
        (v,) = struct.unpack_from(fmt, o._buf, o._base)
        if encoding:
            v = v.decode(encoding)
        return v
    def put_str(self, idx, v, encoding='utf-8'):
        if encoding:
            v = v.encode(encoding)
        o = obj.new(self._buf, len(v) + 1, 1, 0)
        fmt = str(len(v)) + 's'
        struct.pack_into(fmt, o._buf, o._base, v)
        self.put_obj(idx, o)

class bool_list(obj):
    def __getitem__(self, i):
        return super().__getitem__(i>>3).get_bool(i & 7)
    def __setitem__(self, i, v):
        super().__getitem__(i>>3).put_bool(i & 7, v)

def int_list(size, unsigned):
    class _int_list(obj):
        def __getitem__(self, i):
            return super().__getitem__(i).get_int(0, size, unsigned)
        def __setitem__(self, i, v):
            return super().__getitem__(i).put_int(0, size, unsigned, v)
    return _int_list

def enum_list(cls):
    class _enum_list(obj):
        def __getitem__(self, i):
            return cls(super().__getitem__(i).get_int(0, 2, True))
        def __setitem__(self, i, v):
            return super().__getitem__(i).put_int(0, 2, True, int(v))
    return _enum_list

class str_list(obj):
    def __getitem__(self, i):
        return super().__getitem__(i).get_str(0)
    def __setitem__(self, i, v):
        return super().__getitem__(i).put_str(0, v)

class float_list(obj):
    def __getitem__(self, i):
        return super().__getitem__(i).get_float(0, self._ssize)
    def __setitem__(self, i, v):
        return super().__getitem__(i).put_float(0, self._ssize, v)
