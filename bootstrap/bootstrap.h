/*
 * package nbuf
 */

#include "nbuf.h"

/* enum Kind {
 *   BOOL
 *   ENUM
 *   INT
 *   UINT
 *   FLOAT
 *   STR
 *   PTR
 * }
 */
typedef enum {
	nbuf_Kind_BOOL,
	nbuf_Kind_ENUM,
	nbuf_Kind_INT,
	nbuf_Kind_UINT,
	nbuf_Kind_FLOAT,
	nbuf_Kind_STR,
	nbuf_Kind_PTR,
} nbuf_Kind;

#define NBUF_DCL_MSG(_typename, _ssize, _psize) \
typedef struct { \
	struct nbuf_obj o; \
} _typename; \
\
static inline _typename \
nbuf_get_##_typename(struct nbuf_buffer *buf) \
{ \
	_typename o = {{buf}}; \
	nbuf_obj_init(&o.o, 0); \
	return o; \
} \
static inline _typename \
nbuf_new_##_typename(struct nbuf_buffer *buf) \
{ \
	_typename o; \
	o.o = nbuf_create(buf, 1, _ssize, _psize); \
	return o; \
}

#define NBUF_DCL_INT_FIELD(_msgname, _fieldname, _typ, _offs) \
static inline _typ \
_msgname##_##_fieldname(const _msgname *o) \
{ \
	return nbuf_get_int(&o->o, _offs, sizeof (_typ)); \
} \
static inline void \
_msgname##_set_##_fieldname(_msgname *o, _typ v) \
{ \
	nbuf_put_int(&o->o, _offs, sizeof (_typ), v); \
}

#define NBUF_DCL_BOOL_FIELD(_msgname, _fieldname, _offs) \
static inline bool \
_msgname##_##_fieldname(const _msgname *o) \
{ \
	return nbuf_get_bit(&o->o, _offs); \
} \
static inline void \
_msgname##_set_##_fieldname(_msgname *o, bool v) \
{ \
	nbuf_put_bit(&o->o, _offs, v); \
}

#define NBUF_DCL_STR_FIELD(_msgname, _fieldname, _idx) \
static inline char * \
_msgname##_##_fieldname(const _msgname *o, size_t *lenp) \
{ \
	return nbuf_get_str(&o->o, _idx, lenp); \
} \
static inline void \
_msgname##_set_##_fieldname(_msgname *o, const char *v, size_t len) \
{ \
	nbuf_put_str(&o->o, _idx, v, len); \
}

#define NBUF_DCL_PTR_FIELD(_msgname, _fieldname, _fieldtyp, _idx) \
static inline _fieldtyp \
_msgname##_##_fieldname(const _msgname *o) \
{ \
	_fieldtyp oo; \
	oo.o = nbuf_get_ptr(&o->o, _idx); \
	return oo; \
} \
static inline _fieldtyp \
_msgname##_init_##_fieldname(_msgname *o) \
{ \
	_fieldtyp oo; \
	oo = nbuf_new_##_fieldtyp(o->o.buf); \
	nbuf_put_ptr(&o->o, _idx, oo.o); \
	return oo; \
}

#define NBUF_DCL_PTR_LIST_FIELD(_msgname, _fieldname, _elemtyp, _idx) \
static inline _elemtyp \
_msgname##_##_fieldname(const _msgname *o, size_t i) \
{ \
	struct nbuf_obj t; \
	_elemtyp oo; \
	t = nbuf_get_ptr(&o->o, _idx); \
	oo.o = nbuf_get_elem(&t, i); \
	return oo; \
} \
static inline size_t \
_msgname##_##_fieldname##_size(const _msgname *o) \
{ \
	struct nbuf_obj t; \
	t = nbuf_get_ptr(&o->o, _idx); \
	return t.nelem; \
} \
static inline void \
_msgname##_init_##_fieldname(_msgname *o, size_t n) \
{ \
	_elemtyp oo; \
	oo = nbuf_new_##_elemtyp(o->o.buf); \
	nbuf_resize(&oo.o, n); \
	assert(oo.o.nelem == n); \
	nbuf_put_ptr(&o->o, _idx, oo.o); \
}

/* message FieldDesc {
 *   name: string		// 0
 *   kind: Kind			// 0
 *   list: bool			// 16
 *
 *   // for BOOL: bit offset
 *   // for ENUM, INT, FLOAT: byte offset
 *   // for STR, PTR, lists: index
 *   tag0: uint32		// 4
 *
 *   // for INT, UINT, FLOAT: size in bytes
 *   // for PTR: index into msgTypes
 *   // for ENUM: index into enumTypes
 *   tag1: int32		// 8
 * }	// (12, 1)
 */
NBUF_DCL_MSG(nbuf_FieldDesc, 12, 1)
NBUF_DCL_STR_FIELD(nbuf_FieldDesc, name, 0)
NBUF_DCL_BOOL_FIELD(nbuf_FieldDesc, list, 16)
NBUF_DCL_INT_FIELD(nbuf_FieldDesc, kind, int16_t, 0)
NBUF_DCL_INT_FIELD(nbuf_FieldDesc, tag0, uint32_t, 4)
NBUF_DCL_INT_FIELD(nbuf_FieldDesc, tag1, uint32_t, 8)

/* message MsgType {
 *   name: string		// 0
 *   fields: []FieldDesc	// 1
 *   ssize: uint16		// 0
 *   psize: uint16		// 2
 * }	// (4, 2)
 */
NBUF_DCL_MSG(nbuf_MsgType, 4, 2)
NBUF_DCL_STR_FIELD(nbuf_MsgType, name, 0)
NBUF_DCL_PTR_LIST_FIELD(nbuf_MsgType, fields, nbuf_FieldDesc, 1)
NBUF_DCL_INT_FIELD(nbuf_MsgType, ssize, uint16_t, 0)
NBUF_DCL_INT_FIELD(nbuf_MsgType, psize, uint16_t, 2)

/* message EnumDesc {	
 *   name: string	// 0
 *   value: uint16	// 0
 * }	// (4, 1)
 */
NBUF_DCL_MSG(nbuf_EnumDesc, 4, 1)
NBUF_DCL_STR_FIELD(nbuf_EnumDesc, name, 0)
NBUF_DCL_INT_FIELD(nbuf_EnumDesc, value, uint16_t, 0)

/* message EnumType {
 *   name: string	// 0
 *   values: []EnumDesc	// 1
 * }	// (0, 2)
 */
NBUF_DCL_MSG(nbuf_EnumType, 0, 2)
NBUF_DCL_STR_FIELD(nbuf_EnumType, name, 0)
NBUF_DCL_PTR_LIST_FIELD(nbuf_EnumType, values, nbuf_EnumDesc, 1)

/* message Schema {
 *   msgTypes: []MsgType	// 0
 *   enumTypes: []EnumType	// 1
 *   pkgName: string		// 2
 * }	// (0, 3)
 *
 */
NBUF_DCL_MSG(nbuf_Schema, 0, 3)
NBUF_DCL_PTR_LIST_FIELD(nbuf_Schema, msgTypes, nbuf_MsgType, 0)
NBUF_DCL_PTR_LIST_FIELD(nbuf_Schema, enumTypes, nbuf_EnumType, 1)
NBUF_DCL_STR_FIELD(nbuf_Schema, pkgName, 2)
