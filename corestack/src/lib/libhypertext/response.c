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
    char *parser_ptr = ht;
    
    // section 1: request line
    parser_result.version = parser_ptr;
    parser_ptr = strstr(parser_ptr, " ");
    *parser_ptr = '\0';
    parser_ptr++;
    parser_result.res_code = atoi(strdup(parser_ptr, sizeof(ResponseCodeHT)));  // converting to integer
    parser_ptr = strstr(parser_ptr, " ");
    *parser_ptr = '\0';
    parser_ptr++;
    parser_result.res_reason = parser_ptr;
    parser_ptr = strstr(parser_ptr, END_SECT);
    *parser_ptr = '\0';
    parser_ptr += 2; // matches the sep length

    // section 2: headers
    parser_result.headers = parse_hypertext_headers(&parser_ptr);
    // parse_hypertext_headers will set the ptr to start of next section

    // section 3: body
    parser_result.body = parser_ptr;

    parser_ptr = NULL;

    LOG_I("[parse_hypertext_res()] response parsed successfully");

    return parser_result;

    // returning error
    bad_hypertext:
    LOG_E("[parse_hypertext_res()] could not parse the hypertext response, bad format:\n=====BAD RESPONSE====\n%s\n=====BAD RESPONSE====", ht);
    return (ParsedResponseHT) {.version = NULL, .res_code = NULL, .res_reason = NULL, .headers = NULL, .body = NULL};
}
