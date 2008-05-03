#ifndef OPTIONS_H
#define OPTIONS_H

struct options {
	int auto_indent;
	int expand_tab;
	int indent_width;
	int move_wraps;
	int tab_width;
	int trim_whitespace;
};

extern struct options options;

void set_option(const char *name, const char *value);
void collect_options(const char *prefix);
void collect_option_values(const char *name, const char *prefix);

#endif