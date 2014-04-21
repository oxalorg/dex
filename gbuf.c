/*
 * This code is largely based on strbuf in the GIT version control system.
 */

#include "gbuf.h"
#include "common.h"

void gbuf_grow(struct gbuf *buf, size_t more)
{
	size_t alloc = ROUND_UP(buf->len + more, 16);

	if (alloc > buf->alloc) {
		buf->alloc = alloc;
		buf->buffer = xrealloc(buf->buffer, buf->alloc);
	}
}

void gbuf_free(struct gbuf *buf)
{
	free(buf->buffer);
	gbuf_init(buf);
}

void gbuf_add_ch(struct gbuf *buf, char ch)
{
	gbuf_grow(buf, 1);
	buf->buffer[buf->len++] = ch;
}

void gbuf_add_str(struct gbuf *buf, const char *str)
{
	gbuf_add_buf(buf, str, strlen(str));
}

void gbuf_add_buf(struct gbuf *buf, const char *ptr, size_t len)
{
	if (!len)
		return;
	gbuf_grow(buf, len);
	memcpy(buf->buffer + buf->len, ptr, len);
	buf->len += len;
}

char *gbuf_steal(struct gbuf *buf)
{
	char *b = buf->buffer;
	gbuf_init(buf);
	return b;
}

char *gbuf_steal_cstring(struct gbuf *buf)
{
	gbuf_add_ch(buf, 0);
	return gbuf_steal(buf);
}

char *gbuf_cstring(struct gbuf *buf)
{
	char *b = xnew(char, buf->len + 1);
	memcpy(b, buf->buffer, buf->len);
	b[buf->len] = 0;
	return b;
}

void gbuf_make_space(struct gbuf *buf, size_t pos, size_t len)
{
	BUG_ON(pos > buf->len);
	gbuf_grow(buf, len);
	memmove(buf->buffer + pos + len, buf->buffer + pos, buf->len - pos);
	buf->len += len;
}

void gbuf_remove(struct gbuf *buf, size_t pos, size_t len)
{
	BUG_ON(pos + len > buf->len);
	memmove(buf->buffer + pos, buf->buffer + pos + len, buf->len - pos - len);
	buf->len -= len;
}
