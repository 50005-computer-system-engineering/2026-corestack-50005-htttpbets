#ifndef LIBHT_RES_H
#define LIBHT_RES_H

typedef struct {
    unsigned char *version;
    unsigned char *res_code;
    unsigned char *res_reason;
    struct Dictionary *headers;
    unsigned char *body;        // member parsed by application
} ParsedResponseHT;

ParsedResponseHT parse_hypertext_res(const HyperText ht);

#endif