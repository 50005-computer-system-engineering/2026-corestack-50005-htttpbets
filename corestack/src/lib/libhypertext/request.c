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
    char *parser_ptr = ht;
    
    // section 1: request line
    parser_result.req_method = parser_ptr;
    parser_ptr = strstr(parser_ptr, " ");
    *parser_ptr = "\0";
    parser_ptr++;
    parser_result.req_path = parser_ptr;
    parser_ptr = strstr(parser_ptr, " ");
    *parser_ptr = "\0";
    parser_ptr++;
    parser_result.version = parser_ptr;
    parser_ptr = strstr(parser_ptr, END_SECT);
    *parser_ptr = "\0";
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
    LOG_E("[parse_hypertext()] could not parse the hypertext request, bad format:\n=====BAD REQUEST====\n%s\n=====BAD REQUEST====", ht);
    return (ParsedRequestHT) {.version = NULL, .req_method = NULL, .req_path = NULL, .headers = NULL, .body = NULL};
}
