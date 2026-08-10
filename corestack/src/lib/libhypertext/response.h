#ifndef LIBHT_RES_H
#define LIBHT_RES_H

typedef enum {
    LIB_ERROR = 0,
    HT_OK = 200,
    HT_CREATED = 201,
    HT_BAD_REQUEST = 400,
    HT_UNAUTHORIZED = 401,
    HT_FORBIDDEN = 403,
    HT_NOT_FOUND = 404,
    HT_CONFLICT = 409,
    HT_TOO_MANY_REQUESTS = 429,
    HT_INTERNAL_SERVER_ERR = 500
} ResponseCodeHT;

typedef struct {
    unsigned char *version;
    ResponseCodeHT *res_code;
    unsigned char *res_reason;
    struct Dictionary *headers;
    unsigned char *body;        // member parsed by application
} ParsedResponseHT;

ParsedResponseHT parse_hypertext_res(const HyperText ht);

HyperText form_hypertext_res(const ResponseCodeHT code, const struct Dictionary *headers, const unsigned char *body);

#endif