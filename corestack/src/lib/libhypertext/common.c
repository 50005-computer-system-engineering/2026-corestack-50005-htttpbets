#include "common.h"

int helper_counter(unsigned char *s, unsigned char *sep)
{
    int count = 0;
    size_t s_len = strlen(s);
    size_t sep_len = strlen(sep);

    // check if s is valid for sep
    if (s_len <= sep_len)
    {
        LOG_E("[helper_counter()] string is too short for separator");
        return -1;
    }

    // iterates counts matches
    for (size_t i=0; i < s_len - (sep_len - 1); i++)
    {
        if (memcmp(s+i, sep, sep_len)) count++;
    }

    LOG_I("[helper_counter()] string had %d occurences of sep \"%s\"", count, sep);

    return count;
}

/*
Function returns number of lines, intended for private use to count lines in header section
Returns number of lines
Returns -1 on error
*/
int count_headers(unsigned char *s)
{
    int occurences = helper_counter(s, END_LINE);
    return occurences;
}

/*
Function returns number of members, intended for private use to count members in body section
Returns number of members
Returns -1 on error
*/
int count_body_members(unsigned char *s)
{
    int occurences = helper_counter(s, BODY_SEP);
    return occurences;
}

/*
Function parses header section passed as parameter and returns a dictionary pointer
Used as helper in parse_hypertext_request and parse_hypertext_response
*/
struct Dictionary *parse_hypertext_headers(char *headers_tok)
{
    char *field;
    char *value;
    int n_headers = count_headers(headers_tok);
    struct Dictionary *parsed_headers = dict_init(n_headers);
    char *part_tok = strtok(headers_tok, END_LINE);

    for (int i; i<n_headers; i++)
    {
        field = strtok(part_tok, KEY_VAL_SEP);
        value = strtok(part_tok, KEY_VAL_SEP);
        if (add_key_value_pair(parsed_headers, field, value, i) < 0)
        {
            LOG_E("[parse_hypertext_headers()] could not write parsed headers");
            return NULL;
        }
        part_tok = strtok(headers_tok, END_LINE);
    }

    field = NULL;
    value = NULL;

    LOG_I("[parse_hypertext_headers()] finished parsing and returning dictionary with %d headers", parsed_headers->n_items);

    return parsed_headers;
}
