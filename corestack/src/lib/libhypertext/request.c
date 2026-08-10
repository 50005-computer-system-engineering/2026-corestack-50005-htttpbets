#include "request.h"
#include "common.h"

/*
Function parses raw HyperText from a message
Returns a ParsedHT struct
If invalid HyperText, all members will be NULL
*/
ParsedRequestHT parse_hypertext_req(const HyperText ht)
{
    // parse by sections
    ParsedRequestHT parser_result;
    char *sect_tok;
    char *part_tok;

    // part 1: request line
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
    parser_result.headers = parse_hypertext_headers(sect_tok);

    // part 3: body
    sect_tok = strtok(ht, END_SECT);
    parser_result.body = sect_tok;

    LOG_I("[parse_hypertext()] successfully parsed a message");

    return parser_result;

    // returning error
    bad_hypertext:
    LOG_E("[parse_hypertext()] could not parse the hypertext message, bad format:\n=====BAD MESSAGE====\n%s\n=====BAD MESSAGE====", ht);
    return (ParsedRequestHT) {.version = NULL, .req_method = NULL, .req_path = NULL, .headers = NULL, .body = NULL};
}
