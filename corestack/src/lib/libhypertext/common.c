#include "common.h"
#include "utils/logger.h"

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
Function parses raw HyperText from a message
Returns a ParsedHT struct
If invalid HyperText, all members will be NULL
*/
ParsedHT parse_hypertext(const HyperText ht)
{
    // parse by sections
    ParsedHT parser_result;
    char *sect_tok;
    char *part_tok;

    // part 1: method, path, version
    sect_tok = strtok(ht, END_SECT);
    part_tok = strtok(sect_tok, " ");
    if (part_tok == NULL) goto bad_hypertext;
    else
    {
        parser_result.req_method = part_tok;
    }
 
    part_tok = strtok(sect_tok, " ");
    if (part_tok != NULL) goto bad_hypertext;
    else
    {
        parser_result.req_path = part_tok;
    }

    part_tok = strtok(sect_tok, " ");
    if (part_tok != NULL) goto bad_hypertext;
    else
    {
        parser_result.version = part_tok;
    }

    // part 2: headers
    sect_tok = strtok(ht, END_SECT);
    char *field;
    char *value;
    int n_headers = count_headers(ht);
    parser_result.headers = dict_init(n_headers);
    part_tok = strtok(sect_tok, END_LINE);

    for (int i; i<n_headers, i++)
    {
        field = strok(part_tok, ": ");
        value = strtok(part_tok, ": ");
        if (add_key_value_pair(parser_result.headers, field, value, i) < 0)
        {
            LOG_E("[parse_hypertext()] could not parse and write headers");
            goto bad_hypertext;
        }
    }

    // part 3: body
    // TODO parse body

    // TODO return good parse 

    // returning error
    bad_hypertext:
    LOG_E("[parse_hypertext] could not parse the hypertext message, bad format:\n=====BAD MESSAGE====\n%s\n=====BAD MESSAGE====", ht);
    return (ParsedHT) {.version = NULL, .req_method = NULL, .req_path = NULL, .headers = NULL, .body = NULL};
}