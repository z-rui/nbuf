# Generated from nbuf.bnbs, DO NOT EDIT.

import enum

import nbuf

class Kind(enum.IntEnum):
	BOOL = 0
	ENUM = 1
	INT = 2
	UINT = 3
	FLOAT = 4
	STR = 5
	PTR = 6

class Schema(nbuf.obj):
	@classmethod
	def new(cls, buf=None, nelem=1):
		return super().new(buf, nelem, 0, 3)
	def msgTypes(self):
		return super().get_obj(0, MsgType, [])
	def init_msgTypes(self, n):
		o = MsgType.new(self._buf, n)
		super().put_obj(0, o)
		return o
	def enumTypes(self):
		return super().get_obj(1, EnumType, [])
	def init_enumTypes(self, n):
		o = EnumType.new(self._buf, n)
		super().put_obj(1, o)
		return o
	def pkgName(self):
		return super().get_str(2)
	def set_pkgName(self, v):
		return super().put_str(2, v)

class EnumDesc(nbuf.obj):
	@classmethod
	def new(cls, buf=None, nelem=1):
		return super().new(buf, nelem, 4, 1)
	def name(self):
		return super().get_str(0)
	def set_name(self, v):
		return super().put_str(0, v)
	def value(self):
		return super().get_int(0, 2, True)
	def set_value(self, v):
		return super().put_int(0, 2, True, v)

class EnumType(nbuf.obj):
	@classmethod
	def new(cls, buf=None, nelem=1):
		return super().new(buf, nelem, 0, 2)
	def name(self):
		return super().get_str(0)
	def set_name(self, v):
		return super().put_str(0, v)
	def values(self):
		return super().get_obj(1, EnumDesc, [])
	def init_values(self, n):
		o = EnumDesc.new(self._buf, n)
		super().put_obj(1, o)
		return o

class FieldDesc(nbuf.obj):
	@classmethod
	def new(cls, buf=None, nelem=1):
		return super().new(buf, nelem, 12, 1)
	def name(self):
		return super().get_str(0)
	def set_name(self, v):
		return super().put_str(0, v)
	def kind(self):
		return Kind(super().get_int(0, 2, True))
	def set_kind(self, v):
		return super().put_int(0, 2, True, int(v))
	def list(self):
		return super().get_bool(16)
	def set_list(self, v):
		return super().put_bool(16, v)
	def tag0(self):
		return super().get_int(4, 4, True)
	def set_tag0(self, v):
		return super().put_int(4, 4, True, v)
	def tag1(self):
		return super().get_int(8, 4, True)
	def set_tag1(self, v):
		return super().put_int(8, 4, True, v)

class MsgType(nbuf.obj):
	@classmethod
	def new(cls, buf=None, nelem=1):
		return super().new(buf, nelem, 4, 2)
	def name(self):
		return super().get_str(0)
	def set_name(self, v):
		return super().put_str(0, v)
	def fields(self):
		return super().get_obj(1, FieldDesc, [])
	def init_fields(self, n):
		o = FieldDesc.new(self._buf, n)
		super().put_obj(1, o)
		return o
	def ssize(self):
		return super().get_int(0, 2, True)
	def set_ssize(self, v):
		return super().put_int(0, 2, True, v)
	def psize(self):
		return super().get_int(2, 2, True)
	def set_psize(self, v):
		return super().put_int(2, 2, True, v)
