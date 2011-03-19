#include "obuf.h"
#include "term.h"
#include "common.h"
#include "uchar.h"

struct output_buffer obuf;
int screen_w = 80;
int screen_h = 24;

void buf_reset(unsigned int start_x, unsigned int width, unsigned int scroll_x)
{
	obuf.x = 0;
	obuf.start_x = start_x;
	obuf.width = width;
	obuf.scroll_x = scroll_x;
	obuf.tab_width = 8;
	obuf.tab = TAB_CONTROL;
}

// does not update obuf.x
static void buf_add_bytes(const char *str, int count)
{
	while (count) {
		unsigned int avail = obuf.alloc - obuf.count;
		if (count <= avail) {
			memcpy(obuf.buf + obuf.count, str, count);
			obuf.count += count;
			break;
		} else {
			buf_flush();
			if (count >= obuf.alloc) {
				xwrite(1, str, count);
				break;
			}
		}
	}
}

// does not update obuf.x
void buf_set_bytes(char ch, int count)
{
	while (count) {
		unsigned int avail = obuf.alloc - obuf.count;
		if (count <= avail) {
			memset(obuf.buf + obuf.count, ch, count);
			obuf.count += count;
			break;
		} else {
			if (avail) {
				memset(obuf.buf + obuf.count, ch, avail);
				obuf.count += avail;
				count -= avail;
			}
			buf_flush();
		}
	}
}

void buf_escape(const char *str)
{
	buf_add_bytes(str, strlen(str));
}

void buf_add_str(const char *str)
{
	int len = strlen(str);
	buf_add_bytes(str, len);
	obuf.x += len;
}

// width of ch must be 1
void buf_ch(char ch)
{
	if (obuf.x >= obuf.scroll_x && obuf.x < obuf.width + obuf.scroll_x) {
		if (obuf.count == obuf.alloc)
			buf_flush();
		obuf.buf[obuf.count++] = ch;
	}
	obuf.x++;
}

void buf_hide_cursor(void)
{
	if (term_cap.strings[STR_CAP_CMD_vi])
		buf_escape(term_cap.strings[STR_CAP_CMD_vi]);
}

void buf_show_cursor(void)
{
	if (term_cap.strings[STR_CAP_CMD_ve])
		buf_escape(term_cap.strings[STR_CAP_CMD_ve]);
}

void buf_move_cursor(int x, int y)
{
	buf_escape(term_move_cursor(x, y));
}

void buf_set_color(const struct term_color *color)
{
	if (!memcmp(color, &obuf.color, sizeof(*color)))
		return;

	buf_escape(term_set_color(color));
	obuf.color = *color;
}

void buf_clear_eol(void)
{
	if (obuf.x < obuf.scroll_x + obuf.width) {
		int can_clear = obuf.start_x + obuf.width == screen_w;

		if (can_clear && term_cap.strings[STR_CAP_CMD_ce] && (obuf.color.bg < 0 || term_cap.ut)) {
			buf_escape(term_cap.strings[STR_CAP_CMD_ce]);
		} else {
			buf_set_bytes(' ', obuf.scroll_x + obuf.width - obuf.x);
		}
		obuf.x = obuf.scroll_x + obuf.width;
	}
}

void buf_flush(void)
{
	if (obuf.count) {
		xwrite(1, obuf.buf, obuf.count);
		obuf.count = 0;
	}
}

static void skipped_too_much(unsigned int u)
{
	int n = obuf.x - obuf.scroll_x;

	if (obuf.alloc - obuf.count < 8)
		buf_flush();
	if (u == '\t' && obuf.tab != TAB_CONTROL) {
		char ch = ' ';
		if (obuf.tab == TAB_SPECIAL)
			ch = '-';
		memset(obuf.buf + obuf.count, ch, n);
		obuf.count += n;
	} else if (u < 0x20) {
		obuf.buf[obuf.count++] = u | 0x40;
	} else if (u == 0x7f) {
		obuf.buf[obuf.count++] = '?';
	} else if (u & U_INVALID_MASK) {
		if (n > 2)
			obuf.buf[obuf.count++] = hex_tab[(u >> 4) & 0x0f];
		if (n > 1)
			obuf.buf[obuf.count++] = hex_tab[u & 0x0f];
		obuf.buf[obuf.count++] = '>';
	} else {
		obuf.buf[obuf.count++] = '>';
	}
}

void buf_skip(unsigned int u)
{
	if (u < 0x80 || !term_utf8) {
		if (u >= 0x20 && u != 0x7f) {
			obuf.x++;
		} else if (u == '\t' && obuf.tab != TAB_CONTROL) {
			obuf.x += (obuf.x + obuf.tab_width) / obuf.tab_width * obuf.tab_width - obuf.x;
		} else {
			// control
			obuf.x += 2;
		}
	} else {
		obuf.x += u_char_width(u);
	}

	if (obuf.x > obuf.scroll_x)
		skipped_too_much(u);
}

static void print_tab(unsigned int width)
{
	char ch = ' ';

	if (obuf.tab == TAB_SPECIAL) {
		obuf.buf[obuf.count++] = '>';
		obuf.x++;
		width--;
		ch = '-';
	}
	if (width > 0) {
		memset(obuf.buf + obuf.count, ch, width);
		obuf.count += width;
		obuf.x += width;
	}
}

int buf_put_char(unsigned int u)
{
	unsigned int space = obuf.scroll_x + obuf.width - obuf.x;
	unsigned int width;

	if (!space)
		return 0;
	if (obuf.alloc - obuf.count < 8)
		buf_flush();

	if (u < 0x80 || !term_utf8) {
		if (u >= 0x20 && u != 0x7f) {
			obuf.buf[obuf.count++] = u;
			obuf.x++;
		} else if (u == '\t' && obuf.tab != TAB_CONTROL) {
			width = (obuf.x + obuf.tab_width) / obuf.tab_width * obuf.tab_width - obuf.x;
			if (width > space)
				width = space;
			print_tab(width);
		} else {
			obuf.buf[obuf.count++] = '^';
			obuf.x++;
			if (space > 1) {
				if (u == 0x7f) {
					obuf.buf[obuf.count++] = '?';
				} else {
					obuf.buf[obuf.count++] = u | 0x40;
				}
				obuf.x++;
			}
		}
	} else {
		width = u_char_width(u);
		if (width <= space) {
			u_set_char(obuf.buf, &obuf.count, u);
			obuf.x += width;
		} else if (u & U_INVALID_MASK || u <= 0x9f) {
			// invalid or unprintable (0x80 - 0x9f)
			// <xx> would not fit
			// there's enough space in the buffer so render all 4 characters
			// but increment position less
			unsigned int idx = obuf.count;
			u_set_hex(obuf.buf, &idx, u);
			obuf.count += space;
			obuf.x += space;
		} else {
			obuf.buf[obuf.count++] = '>';
			obuf.x++;
		}
	}
	return 1;
}
