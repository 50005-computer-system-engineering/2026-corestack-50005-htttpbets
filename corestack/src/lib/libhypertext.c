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
