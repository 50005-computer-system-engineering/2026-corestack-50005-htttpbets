#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h> 
#include <string.h> 
#include <strings.h>

#include "lib/libhypertext.h"
#include "utils/logger.h"

// parser functions
/*
helper function for parsing
moves to next delimiter specified and replaces it with null bytes
returns the pointer to first byte after delimiter
*/
char *next_token(char *s, char *delim)
{
    // move to next delim
    LOG_D("[next_token()] finding token \'%s\' from string \"%s\"", delim, s);

    s = strstr(s, delim);

    // check if delim exists
    if (s == NULL)
    {
        LOG_E("[next_token()] could not find token \'%s\' from string", delim);
        return s;
    }

    // ptr at delim, replace with null bytes
    for (int i=0; i<strlen(delim); i++)
    {
        *s = '\0';
        s++;
    }

    LOG_I("[next_token()] found matching token \'%s\', returning new position", delim);

    // return new pointer location
    return s;
}

/*
helper function for counting the number of headers in section
expects pointer at the start of the header section
*/
int count_headers(char *headers_start)
{
    char *headers_end = strstr(headers_start, "\r\n\r\n");
    if (!headers_end)
    {
        LOG_E("[count_headers()] could not find an end to section");
    }
    char *ptr = headers_start;
    int n_headers = 0;
    
    // count occurences of HT_END_LINE within section
    while (ptr < headers_end)
    {
        ptr = strstr(ptr, HT_END_LINE);
        if (!ptr) 
        {
            LOG_E("[count_headers()] exceeded section");
            break;
        }
        ptr += 2;
        n_headers++;
    }

    LOG_I("[count_headers()] found %d headers", n_headers);
    return n_headers;
}

/*
Function parses a message and returns the completed version to a pointer
Returns 0 on success, -1 on failure
*/
int parse_hypertext(HyperText ht, ParsedMsgHT *parser_result)
{
    // request line
    char *ptr = ht;
    parser_result->token1 = ptr;
    ptr = next_token(ptr, HT_TOKEN_SEP);
    parser_result->token2 = ptr;
    ptr = next_token(ptr, HT_TOKEN_SEP);
    parser_result->token3 = ptr;

    // headers
    ptr = next_token(ptr, HT_END_LINE);
    char *field;
    char *value;
    parser_result->n_headers = count_headers(ptr); 
    for (int i=0; i<parser_result->n_headers; i++)
    {
        // find the values for the header
        field = ptr;
        ptr = next_token(ptr, HT_HEADER_SEP);
        value = ptr;
        ptr = next_token(ptr, HT_END_LINE);
        
        // assign the strings
        parser_result->headers[i].field = field;
        parser_result->headers[i].value = value;
    }
    
    // body
    ptr = next_token(ptr, HT_END_LINE); // only end line left after the last loop iteration
    parser_result->body = ptr;

    fail:
    return -1;
}

// builder functions
/*
function initialises a ParsedMsgHT struct
*/
int msg_init(ParsedMsgHT *new_msg, const char *t1, const char *t2, const char *t3)
{
    // assign values;
    new_msg->token1 = t1;
    new_msg->token2 = t2;
    new_msg->token3 = t3;

    return 0;
}

int req_init(ParsedMsgHT *new_req, const char *method, const char *path, const char *ver)
{
    return msg_init(new_req, method, path, ver);
}

int res_init(ParsedMsgHT *new_res, const char *ver, const char *code, const char *reason)
{
    return msg_init(new_res, ver, code, reason);
}

const char *ILLEGAL_CHARS = "\r\n:";

/*
helper function which checks for illegal characters in a string
*/
bool has_illegal_chars(const char *s)
{
    return strpbrk(s, ILLEGAL_CHARS) != NULL;
}

/*
Function adds header to the message if there are no illegal characters
*/
int msg_add_header(ParsedMsgHT *msg, const char *field, const char *value)
{
    // check for illegal characters
    if (has_illegal_chars(field) || has_illegal_chars(value))
    {
        LOG_E("[msg_add_header()] field name or value has illegal characters");
        return -1;
    }
    
    // find index suitable for new header
    int idx = msg->n_headers++;

    // assign new header
    msg->headers[idx].field = field;
    msg->headers[idx].value = value;

    LOG_I("[msg_add_header()] added new header {%s: %s}, now has %d headers", field, value, msg->n_headers);
    return 0;
}

/*
Function adds body to message if it does not drastically exceed the maximum buffer size
*/
int msg_add_body(ParsedMsgHT *msg, const char *body)
{
    // check if definitely too long
    if (strlen(body) >= MAX_BUF)
    {
        LOG_E("[msg_add_body()] body passed well exceeds total message size");
        return -1;
    }
    
    // assign
    msg->body = body;
    return 0;
}
