#ifndef LIBHT_REQ_H
#define LIBHT_REQ_H

#include "common.h"

typedef struct {
    unsigned char *version;
    unsigned char *req_method;
    unsigned char *req_path;
    struct Dictionary *headers;
    unsigned char *body;        // member parsed by application
} ParsedRequestHT;

ParsedRequestHT parse_hypertext_req(const HyperText ht);

#endif