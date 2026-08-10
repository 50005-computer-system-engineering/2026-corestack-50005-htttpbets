#include "libhypertext.h"

/*
Section 1: request creation and parsing
creation is done manually for libhypertext, this is because request creation should be handled by a separate library which publicly include libhypertext
for this test, manually generated good and bad hypertext will be put through parser
*/

HyperText good_hypertext = "GET /test HyperText/0.1\r\nHost: example.com\r\nSample-Header: sample value\r\n\r\nthis is the body of the request";
HyperText bad_hypertext = "GET HyperText\r\nHost: example.com\r\nSample-Header: sample value\r\n\r\nthis is the body of the request";   // example with incomplete request line

// parse good request and print out results, all print statements should succeed on parse
ParsedRequestHT good_parse = parse_hypertext_req(good_hypertext);
printf("ver: %d\nmethod: %d\npath %d\nheaders %d\nbody %d\n",
    good_parse.version == "Hypertext/0.1",
    good_parse.method == "GET",
    good_parse.path == "/test",
    get_dict_val(good_parse.headers, "Host") == "example.com" && get_dict_val(good_parse.headers, "Sample-Header") == "sample value",
    good_parse.body == "nthis is the body of the request"    
);
// parse bad request and print out the results. Should match to bad_hypertext within parse_hypertext_req
ParsedRequestHT bad_parse = parse_hypertext_req(bad_hypertext);
printf("ver: %d\nmethod: %d\npath %d\nheaders %d\nbody %d\n",
    bad_parse.version == NULL,
    bad_parse.method == NULL,
    bad_parse.path == NULL,
    bad_parse.headers == NULL,
    bad_parse.body == NULL  
);