/* Generated from lib/nbuf.nbs.  DO NOT EDIT. */

#ifndef NBUF_NB_H
#define NBUF_NB_H

#include "nbuf.h"
#include "nbuf.nb.h"

typedef enum {
	nbuf_Kind_BOOL = 0,
	nbuf_Kind_ENUM = 1,
	nbuf_Kind_INT = 2,
	nbuf_Kind_UINT = 3,
	nbuf_Kind_FLOAT = 4,
	nbuf_Kind_STR = 5,
	nbuf_Kind_PTR = 6,
} nbuf_Kind;

typedef struct {
	struct nbuf_obj o;
} nbuf_Schema;

static inline nbuf_Schema
nbuf_get_Schema(struct nbuf_buffer *buf)
{
	nbuf_Schema o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline nbuf_Schema
nbuf_new_Schema(struct nbuf_buffer *buf)
{
	nbuf_Schema o;
	o.o = nbuf_create(buf, 1, 0, 3);
	return o;
}

typedef struct {
	struct nbuf_obj o;
} nbuf_EnumDesc;

static inline nbuf_EnumDesc
nbuf_get_EnumDesc(struct nbuf_buffer *buf)
{
	nbuf_EnumDesc o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline nbuf_EnumDesc
nbuf_new_EnumDesc(struct nbuf_buffer *buf)
{
	nbuf_EnumDesc o;
	o.o = nbuf_create(buf, 1, 4, 1);
	return o;
}

typedef struct {
	struct nbuf_obj o;
} nbuf_EnumType;

static inline nbuf_EnumType
nbuf_get_EnumType(struct nbuf_buffer *buf)
{
	nbuf_EnumType o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline nbuf_EnumType
nbuf_new_EnumType(struct nbuf_buffer *buf)
{
	nbuf_EnumType o;
	o.o = nbuf_create(buf, 1, 0, 2);
	return o;
}

typedef struct {
	struct nbuf_obj o;
} nbuf_FieldDesc;

static inline nbuf_FieldDesc
nbuf_get_FieldDesc(struct nbuf_buffer *buf)
{
	nbuf_FieldDesc o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline nbuf_FieldDesc
nbuf_new_FieldDesc(struct nbuf_buffer *buf)
{
	nbuf_FieldDesc o;
	o.o = nbuf_create(buf, 1, 12, 1);
	return o;
}

typedef struct {
	struct nbuf_obj o;
} nbuf_MsgType;

static inline nbuf_MsgType
nbuf_get_MsgType(struct nbuf_buffer *buf)
{
	nbuf_MsgType o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline nbuf_MsgType
nbuf_new_MsgType(struct nbuf_buffer *buf)
{
	nbuf_MsgType o;
	o.o = nbuf_create(buf, 1, 4, 2);
	return o;
}

static inline nbuf_MsgType
nbuf_Schema_msgTypes(const nbuf_Schema *o, size_t i)
{
	nbuf_MsgType oo;
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 0);
	t = nbuf_get_elem(&t, i);
	oo.o = t;
	return oo;
}

static inline size_t
nbuf_Schema_msgTypes_size(const nbuf_Schema *o)
{
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 0);
	return t.nelem;
}

static inline void
nbuf_Schema_init_msgTypes(const nbuf_Schema *o, size_t n)
{
	struct nbuf_obj oo = nbuf_create(o->o.buf, n, 4, 2);
	nbuf_put_ptr(&o->o, 0, oo);
}

static inline bool
nbuf_Schema_has_msgTypes(const nbuf_Schema *o)
{
	return nbuf_has_ptr(&o->o, 0);
}

static inline nbuf_EnumType
nbuf_Schema_enumTypes(const nbuf_Schema *o, size_t i)
{
	nbuf_EnumType oo;
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 1);
	t = nbuf_get_elem(&t, i);
	oo.o = t;
	return oo;
}

static inline size_t
nbuf_Schema_enumTypes_size(const nbuf_Schema *o)
{
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 1);
	return t.nelem;
}

static inline void
nbuf_Schema_init_enumTypes(const nbuf_Schema *o, size_t n)
{
	struct nbuf_obj oo = nbuf_create(o->o.buf, n, 0, 2);
	nbuf_put_ptr(&o->o, 1, oo);
}

static inline bool
nbuf_Schema_has_enumTypes(const nbuf_Schema *o)
{
	return nbuf_has_ptr(&o->o, 1);
}

static inline const char *
nbuf_Schema_pkgName(const nbuf_Schema *o, size_t *lenp)
{
	return nbuf_get_str(&o->o, 2, lenp);
}

static inline void
nbuf_Schema_set_pkgName(const nbuf_Schema *o, const char * v, size_t len)
{
	return nbuf_put_str(&o->o, 2, v, len);
}

static inline const char *
nbuf_EnumDesc_name(const nbuf_EnumDesc *o, size_t *lenp)
{
	return nbuf_get_str(&o->o, 0, lenp);
}

static inline void
nbuf_EnumDesc_set_name(const nbuf_EnumDesc *o, const char * v, size_t len)
{
	return nbuf_put_str(&o->o, 0, v, len);
}

static inline uint16_t
nbuf_EnumDesc_value(const nbuf_EnumDesc *o)
{
	return nbuf_get_int(&o->o, 0, 2);
}

static inline void
nbuf_EnumDesc_set_value(const nbuf_EnumDesc *o, uint16_t v)
{
	return nbuf_put_int(&o->o, 0, 2, v);
}

static inline const char *
nbuf_EnumType_name(const nbuf_EnumType *o, size_t *lenp)
{
	return nbuf_get_str(&o->o, 0, lenp);
}

static inline void
nbuf_EnumType_set_name(const nbuf_EnumType *o, const char * v, size_t len)
{
	return nbuf_put_str(&o->o, 0, v, len);
}

static inline nbuf_EnumDesc
nbuf_EnumType_values(const nbuf_EnumType *o, size_t i)
{
	nbuf_EnumDesc oo;
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 1);
	t = nbuf_get_elem(&t, i);
	oo.o = t;
	return oo;
}

static inline size_t
nbuf_EnumType_values_size(const nbuf_EnumType *o)
{
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 1);
	return t.nelem;
}

static inline void
nbuf_EnumType_init_values(const nbuf_EnumType *o, size_t n)
{
	struct nbuf_obj oo = nbuf_create(o->o.buf, n, 4, 1);
	nbuf_put_ptr(&o->o, 1, oo);
}

static inline bool
nbuf_EnumType_has_values(const nbuf_EnumType *o)
{
	return nbuf_has_ptr(&o->o, 1);
}

static inline const char *
nbuf_FieldDesc_name(const nbuf_FieldDesc *o, size_t *lenp)
{
	return nbuf_get_str(&o->o, 0, lenp);
}

static inline void
nbuf_FieldDesc_set_name(const nbuf_FieldDesc *o, const char * v, size_t len)
{
	return nbuf_put_str(&o->o, 0, v, len);
}

static inline nbuf_Kind
nbuf_FieldDesc_kind(const nbuf_FieldDesc *o)
{
	return nbuf_get_int(&o->o, 0, 2);
}

static inline void
nbuf_FieldDesc_set_kind(const nbuf_FieldDesc *o, nbuf_Kind v)
{
	return nbuf_put_int(&o->o, 0, 2, v);
}

static inline bool
nbuf_FieldDesc_list(const nbuf_FieldDesc *o)
{
	return nbuf_get_bit(&o->o, 16);
}

static inline void
nbuf_FieldDesc_set_list(const nbuf_FieldDesc *o, bool v)
{
	return nbuf_put_bit(&o->o, 16, v);
}

static inline uint32_t
nbuf_FieldDesc_tag0(const nbuf_FieldDesc *o)
{
	return nbuf_get_int(&o->o, 4, 4);
}

static inline void
nbuf_FieldDesc_set_tag0(const nbuf_FieldDesc *o, uint32_t v)
{
	return nbuf_put_int(&o->o, 4, 4, v);
}

static inline uint32_t
nbuf_FieldDesc_tag1(const nbuf_FieldDesc *o)
{
	return nbuf_get_int(&o->o, 8, 4);
}

static inline void
nbuf_FieldDesc_set_tag1(const nbuf_FieldDesc *o, uint32_t v)
{
	return nbuf_put_int(&o->o, 8, 4, v);
}

static inline const char *
nbuf_MsgType_name(const nbuf_MsgType *o, size_t *lenp)
{
	return nbuf_get_str(&o->o, 0, lenp);
}

static inline void
nbuf_MsgType_set_name(const nbuf_MsgType *o, const char * v, size_t len)
{
	return nbuf_put_str(&o->o, 0, v, len);
}

static inline nbuf_FieldDesc
nbuf_MsgType_fields(const nbuf_MsgType *o, size_t i)
{
	nbuf_FieldDesc oo;
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 1);
	t = nbuf_get_elem(&t, i);
	oo.o = t;
	return oo;
}

static inline size_t
nbuf_MsgType_fields_size(const nbuf_MsgType *o)
{
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 1);
	return t.nelem;
}

static inline void
nbuf_MsgType_init_fields(const nbuf_MsgType *o, size_t n)
{
	struct nbuf_obj oo = nbuf_create(o->o.buf, n, 12, 1);
	nbuf_put_ptr(&o->o, 1, oo);
}

static inline bool
nbuf_MsgType_has_fields(const nbuf_MsgType *o)
{
	return nbuf_has_ptr(&o->o, 1);
}

static inline uint16_t
nbuf_MsgType_ssize(const nbuf_MsgType *o)
{
	return nbuf_get_int(&o->o, 0, 2);
}

static inline void
nbuf_MsgType_set_ssize(const nbuf_MsgType *o, uint16_t v)
{
	return nbuf_put_int(&o->o, 0, 2, v);
}

static inline uint16_t
nbuf_MsgType_psize(const nbuf_MsgType *o)
{
	return nbuf_get_int(&o->o, 2, 2);
}

static inline void
nbuf_MsgType_set_psize(const nbuf_MsgType *o, uint16_t v)
{
	return nbuf_put_int(&o->o, 2, 2, v);
}

extern nbuf_EnumType nbuf_refl_Kind;

extern nbuf_MsgType nbuf_refl_Schema;

extern nbuf_MsgType nbuf_refl_EnumDesc;

extern nbuf_MsgType nbuf_refl_EnumType;

extern nbuf_MsgType nbuf_refl_FieldDesc;

extern nbuf_MsgType nbuf_refl_MsgType;

extern nbuf_Schema nbuf_refl_schema;

#endif  /* NBUF_NB_H */
