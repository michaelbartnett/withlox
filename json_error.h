#ifndef JSON_ERROR_H

#include "json.h"

inline const char* json_error_code_string(size_t error_code)
{
    switch (error_code)
    {
    // no error occurred (huzzah!)
    case json_parse_error_none:
        return "json_parse_error_none";

    // expected a comma where there was none!
    case json_parse_error_expected_comma:
        return "json_parse_error_expected_comma";

    // colon separating name/value pair was missing!
    case json_parse_error_expected_colon:
        return "json_parse_error_expected_colon";

    // expected string to begin with '"'!
    case json_parse_error_expected_opening_quote:
        return "json_parse_error_expected_opening_quote";

    // invalid escaped sequence in string!
    case json_parse_error_invalid_string_escape_sequence:
        return "json_parse_error_invalid_string_escape_sequence";

    // invalid number format!
    case json_parse_error_invalid_number_format:
        return "json_parse_error_invalid_number_format";

    // invalid value!
    case json_parse_error_invalid_value:
        return "json_parse_error_invalid_value";

    // reached end of buffer before object/array was complete!
    case json_parse_error_premature_end_of_buffer:
        return "json_parse_error_premature_end_of_buffer";

    // string was malformed!
    case json_parse_error_invalid_string:
        return "json_parse_error_invalid_string";

    // catch-all error for everything else that exploded (real bad chi!)
    case json_parse_error_unknown:
        return "json_parse_error_unknow";
    }

    assert(0); // shouldn't happen
}

#define JSON_ERROR_H
#endif
