#ifndef _LEX_UTIL_H
#define _LEX_UTIL_H

static int id_type(void);
static int check_keyword(int start, int end, const char *str, int t);

static char next(void);
static char advance(void);
static char peek(int n);

static void skip(void);
static void nskip(int n);

static bool is_space(void);
static bool check_peek(int n, char expected);
static bool check(char expected);
static bool match(char expected);

static bool end(void);
static bool digit(char c);
static bool alpha(char c);

static void skip_line_comment(void);
static void skip_multi_line_comment(void);
static void skip_whitespace(void);
static token make_token(int t);
static token err_token(const char *err);
static token string(void);
static token number(void);
static token id(void);
static token character(void);
static token skip_comment(void);
static token strict_toke(int t);

#endif
