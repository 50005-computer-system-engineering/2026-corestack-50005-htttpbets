#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h> 
#include <string.h> 
#include <strings.h>
#include <time.h>

#include "lib/libhtttp.h"
#include "utils/logger.h"

/*
Function generates date
*/
void get_date_str(char *buf) {
    time_t now = time(NULL);
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);
    strftime(buf, 30, "%a, %d %b %Y %H:%M:%S GMT", &tm_utc);
}

/*
Function finds the correct HTTTP method
Returns enum value if found, else returns UNKOWN=0
*/
MethodHTTTP string_to_method(char *s)
{
    // case where string is empty
    if (!s)
    {
        LOG_E("[string_to_method()] string not found");
        return UNKNOWN;
    }
    
    // iterate until matching string
    for (int i=0; i<N_HTTTP_METHODS; i++)
    {
        // matching string in legal methods
        if (strcmp(s, HTTTP_METHODS[i].string) == 0)
        {
            LOG_I("[string_to_method()] string \"%s\" maps to method %d", s, HTTTP_METHODS[i].method);
            return HTTTP_METHODS[i].method;
        }
    }

    // catch if none found
    LOG_I("[string_to_method()] string could not be mapped to any method, returning UNKOWN = 0");
    return UNKNOWN;
}

/*
Simple boolean function to determine if method is legal
*/
bool is_legal_method(char *s)
{
    return (string_to_method(s) != UNKNOWN);
}

/*
Function returns the string corresponding to method enum value
*/
char *method_to_string(MethodHTTTP v)
{
    // case where string is empty
    if (v == UNKNOWN)
    {
        LOG_E("[string_to_method()] string not found");
        return NULL;
    }
    
    // iterate until matching string
    for (int i=0; i<N_HTTTP_METHODS; i++)
    {
        // matching string in legal methods
        if (v == HTTTP_METHODS[i].method)
        {
            return HTTTP_METHODS[i].string;
        }
    }
}

/*
Returns if header is legal
*/
bool is_legal_header_field(char *s)
{
    // case where string is empty
    if (!s)
    {
        LOG_E("[is_legal_header_field()] string not found");
        return false;
    }
    
    // iterate until matching string
    for (int i=0; i<N_HTTTP_HEADERS; i++)
    {
        // matching string in legal headers
        if (strcmp(s, HTTTP_HEADERS) == 0)
        {
            LOG_I("[is_legal_header_field()] string \"%s\" is a header", s);
            return true;
        }
    }

    return false;
}

