#include "lib/libhypertext.h"

// HTTTP methods
typedef enum {
    UNKNOWN = 0,
    REQ_MOVE,
    REQ_DROP,
    REQ_ROTATE,
    REQ_STATE
} MethodHTTTP;

// mapping to string
typedef struct {
    char *string;
    MethodHTTTP method;
} MethodMapping;

// array of accepted methods
const MethodMapping HTTTP_METHODS[] = {
    {"MOVE", REQ_MOVE},
    {"DROP", REQ_DROP},
    {"ROTATE", REQ_ROTATE},
    {"STATE", REQ_STATE}
};
const int N_HTTTP_METHODS = sizeof(HTTTP_METHODS) / sizeof(MethodMapping);

// HTTTP header fields
const char *HTTTP_HEADERS[] = {
    "Content-Length",
    "Content-Type",
    "Player-Id",
    "Date"
};
const int N_HTTTP_HEADERS = sizeof(HTTTP_HEADERS) / sizeof(char *);
