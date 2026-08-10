#include <stdint.h>
#include <stdbool.h>

#include "libhypertext/libhypertext.h"
#include "libhtttp.h"

// legal values
static const char *ALLOWED_METHODS[] = {
    "JOIN",
    "LEAVE",
    "START", 
    "MOVE",
    "ROTATE",
    "DROP",
    "STATE",
    NULL    // catch for loops
};

static const char *ALLOWED_HEADERS[] = {
    "Content-Type", 
    "Content-Length",
    "Player-Id",
    "Date",
    NULL    // catch for loops
};

/*
Function used to validate is a specific string exists within a list
*/
bool is_in_list(const char *str, const char *list[])
{
    if (!str) return false;
    for (int i=0; list[i] != NULL; i++) // iterate until catch 
    {
        if (strcmp(str, list[i]) == 0) return true; // find match
    }
    return false;
}

/*
Function validates a parsed request
*/
bool is_legal_request(ParsedRequestHT *req)
{
    // TODO check for error request (all NULL)

    // check version match
    if (strcmp(req->version, HTTTP_VER) == 0)
    {
        LOG_E("[is_legal_request()] HTTTP version mismatch");
        return false;
    }

    // check if method is valid
    if (!is_in_list(req->req_method, ALLOWED_METHODS))
    {
        LOG_E("[is_legal_request()] invalid HTTTP method");
        return false;
    }

    // check if headers are valid
    for (int i=0; i < req->headers->n_items; i++)
    {
        if (!is_in_list(req->headers->vals[i][0], ALLOWED_HEADERS))
        {
            LOG_E("[is_legal_request()] invalid header %s found", req->headers->vals[i][0]);
            return false;
        }
    }

    // checks complete, up to application to validate information and act
    return true;
}
