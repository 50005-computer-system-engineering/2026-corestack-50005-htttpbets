#ifndef LIBHT_COMMON_H
#define LIBHT_COMMON_H

#include <string.h>

#include "dict.h"
#include "utils/logger.h"

// buffer constants
#define HT_MAX_SIZE 512

// parsing constants
#define END_LINE "\r\n"
#define END_SECT END_LINE END_LINE
#define BODY_SEP ", "
#define KEY_VAL_SEP ": "

// HyperText structs
typedef unsigned char HyperText[HT_MAX_SIZE];

// functions
int helper_counter(unsigned char *s, unsigned char *sep);
int count_headers(unsigned char *s);
struct Dictionary *parse_hypertext_headers(char **headers);

#endif