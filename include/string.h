#ifndef STRING_H
#define STRING_H

#include "types.h"

// String length
size_t str_len(const char *s);

// String comparison (returns 0 if equal, <0 if s1<s2, >0 if s1>s2)
int str_cmp(const char *s1, const char *s2);

// Compare first n characters
int str_ncmp(const char *s1, const char *s2, size_t n);

// Copy string (returns dest)
char* str_cpy(char *dest, const char *src);

// Copy up to n characters
char* str_ncpy(char *dest, const char *src, size_t n);

// Concatenate strings
char* str_cat(char *dest, const char *src);

// Find character in string (returns pointer or NULL)
char* str_chr(const char *s, int c);

// Find last occurrence of character
char* str_rchr(const char *s, int c);

// Memory set
void* mem_set(void *s, int c, size_t n);

// Memory copy
void* mem_cpy(void *dest, const void *src, size_t n);

// Memory compare
int mem_cmp(const void *s1, const void *s2, size_t n);

// Convert integer to string (returns length)
int int_to_str(int value, char *buf);

// Convert unsigned integer to string
int uint_to_str(uint32_t value, char *buf);

#endif
