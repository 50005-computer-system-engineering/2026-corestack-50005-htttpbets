#ifndef LIBHT_COMMON_H
#define LIBHT_COMMON_H

#include <string.h>

#include "dict.h"

// buffer constants
#define HT_MAX_SIZE 512

// parsing constants
#define END_LINE "\r\n"
#define END_SECT END_LINE END_LINE
#define BODY_SEP ", "
#define KEY_VAL_SEP ": "

// HyperText structs
typedef unsigned char HyperText[HT_MAX_SIZE];
typedef struct {
    unsigned char *version;
    unsigned char *req_method;
    unsigned char *req_path;
    struct Dictionary *headers;
    struct Dictionary *body;
} ParsedHT;

// functions
ParsedHT parse_hypertext(const HyperText ht);

#endif