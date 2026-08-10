#include "response.h"
#include "common.h"

/*
Almost identacal to request, with different response line ordering instead
Function parses raw HyperText from a message
Returns a ParsedHT struct
If invalid HyperText, all members will be NULL
*/
ParsedResponseHT parse_hypertext_res(const HyperText ht)
{
    // parse by sections
    ParsedResponseHT parser_result;
    char *sect_tok;
    char *part_tok;

    // part 1: request line
    sect_tok = strtok(ht, END_SECT);
    part_tok = strtok(sect_tok, " ");
    if (part_tok == NULL) goto bad_hypertext;
    else
    {
        parser_result.version = part_tok;
    }
 
    part_tok = strtok(sect_tok, " ");
    if (part_tok != NULL) goto bad_hypertext;
    else
    {
        parser_result.res_code = part_tok;
    }

    part_tok = strtok(sect_tok, " ");
    if (part_tok != NULL) goto bad_hypertext;
    else
    {
        parser_result.res_reason = part_tok;
    }

    // part 2: headers
    sect_tok = strtok(ht, END_SECT);
    parser_result.headers = parse_hypertext_headers(sect_tok);

    // part 3: body
    sect_tok = strtok(ht, END_SECT);
    parser_result.body = sect_tok;

    LOG_I("[parse_hypertext_res()] successfully parsed a response");

    return parser_result;

    // returning error
    bad_hypertext:
    LOG_E("[parse_hypertext_res()] could not parse the hypertext response, bad format:\n=====BAD RESPONSE====\n%s\n=====BAD RESPONSE====", ht);
    return (ParsedRequestHT) {.version = NULL, .res_code = NULL, .res_reason = NULL, .headers = NULL, .body = NULL};
}
