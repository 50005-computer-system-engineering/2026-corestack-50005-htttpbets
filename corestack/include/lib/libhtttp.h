#include "libhypertext.h"
#include "libhtttp/payload.h"

#define CURRENT_VER "HTTTP/1.0"

// HTTTP methods
typedef enum {
    UNKNOWN = 0,
    REQ_MOVE,
    REQ_DROP,
    REQ_ROTATE,
    REQ_STATE,
    REQ_ATTACK
} MethodHTTTP;

// mapping to string
typedef struct {
    char* string;
    MethodHTTTP method;
} MethodMapping;

// array of accepted methods
const MethodMapping HTTTP_METHODS[] = {
    {"MOVE", REQ_MOVE},
    {"DROP", REQ_DROP},
    {"ROTATE", REQ_ROTATE},
    {"STATE", REQ_STATE},
    {"ATTACK", REQ_ATTACK}};
const int N_HTTTP_METHODS = sizeof(HTTTP_METHODS) / sizeof(MethodMapping);

// HTTTP header fields
const char* htttp_headers[] = {
    "Content-Length",
    "Content-Type",
    "Player-Id",
    "Date"};
const int N_HTTTP_HEADERS = sizeof(htttp_headers) / sizeof(char*);

// important request builders
void req_create_action(uint32_t id, MethodHTTTP method, InputPayload* payload, ParsedMsgHT* formatted_msg);
void req_create_state(uint32_t id, StatePayload* payload, ParsedMsgHT* formatted_msg);
void req_create_attack(uint32_t id, AttackPayload* payload, ParsedMsgHT* formatted_msg);
