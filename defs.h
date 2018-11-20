#ifndef DEFS_H
#define DEFS_H

#define TYPE_AT(type, base, offset) (*(type*)((char*)(base) + (offset)))

#define CHAR_AT(base, offset) (TYPE_AT(char, base, offset))
#define SHORT_AT(base, offset) (le16toh(TYPE_AT(short, base, offset)))
#define INT_AT(base, offset) (le32toh(TYPE_AT(int, base, offset)))
#define LONG_AT(base, offset) (le64toh(TYPE_AT(long long, base, offset)))

#define SET_CHAR_AT(base, offset, value) (TYPE_AT(char, base, offset) = (value))
#define SET_SHORT_AT(base, offset, value) (TYPE_AT(short, base, offset) = htole16(value))
#define SET_INT_AT(base, offset, value) (TYPE_AT(int, base, offset) = htole32(value))
#define SET_LONG_AT(base, offset, value) (TYPE_AT(long long, base, offset) = htole64(value))

#endif //DEFS_H
