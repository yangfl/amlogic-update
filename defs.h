#ifndef AML_DEFS_H
#define AML_DEFS_H

#include <endian.h>
#include <stdint.h>


#define aml_min(a, b) ((a) < (b) ? (a) : (b))
#define aml_max(a, b) ((a) > (b) ? (a) : (b))


#define TYPE_AT(type, ptr) (*(type *) (ptr))

#define GET_U8(ptr) (TYPE_AT(uint8_t, ptr))
#define GET_U16(ptr) (le16toh(TYPE_AT(uint16_t, ptr)))
#define GET_U32(ptr) (le32toh(TYPE_AT(uint32_t, ptr)))
#define GET_U64(ptr) (le64toh(TYPE_AT(uint64_t, ptr)))

#define SET_U8(ptr, value) (TYPE_AT(uint8_t, ptr) = (value))
#define SET_U16(ptr, value) (TYPE_AT(uint16_t, ptr) = htole16(value))
#define SET_U32(ptr, value) (TYPE_AT(uint32_t, ptr) = htole32(value))
#define SET_U64(ptr, value) (TYPE_AT(uint64_t, ptr) = htole64(value))


#ifndef __attr_format
#  define __attr_format(x) __attribute__((__format__ x))
#endif


#endif  // AML_DEFS_H
