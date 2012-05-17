#ifndef COMMON_H
#define COMMON_H

#include "libc.h"
#include "ctype.h"
#include "xmalloc.h"

extern const char hex_tab[16];
extern int term_utf8;

static inline size_t ROUND_UP(size_t x, size_t r)
{
	r--;
	return (x + r) & ~r;
}

#if DEBUG <= 0
FORMAT(1)
static inline void BUG(const char *fmt, ...)
{
}
#else
#define BUG(...) bug(__FUNCTION__, __VA_ARGS__)
#endif

#if DEBUG <= 1
FORMAT(1)
static inline void d_print(const char *fmt, ...)
{
}
#else
#define d_print(...) debug_print(__FUNCTION__, __VA_ARGS__)
#endif

#define STRINGIFY(a) #a
#define BUG_ON(a) \
	do { \
		if (unlikely(a)) \
			BUG("%s\n", STRINGIFY(a)); \
	} while (0)

static inline int str_has_prefix(const char *str, const char *prefix)
{
	return !strncmp(str, prefix, strlen(prefix));
}

int count_strings(char **strings);
unsigned int number_width(unsigned int n);
int buf_parse_long(const char *str, int size, int *posp, long *valp);
int parse_long(const char **strp, long *valp);
int str_to_long(const char *str, long *valp);
int str_to_int(const char *str, int *valp);
char *xsprintf(const char *format, ...) FORMAT(1);
ssize_t xread(int fd, void *buf, size_t count);
ssize_t xwrite(int fd, const void *buf, size_t count);
ssize_t read_file(const char *filename, char **bufp);
char *buf_next_line(char *buf, ssize_t *posp, ssize_t size);
void bug(const char *function, const char *fmt, ...) FORMAT(2) NORETURN;
void debug_print(const char *function, const char *fmt, ...) FORMAT(2);
void *xmmap(int fd, off_t offset, size_t len);
void xmunmap(void *start, size_t len);

#endif
