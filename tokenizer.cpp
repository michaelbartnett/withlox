#include "common.h"
#include "tokenizer.h"


namespace tokenizer
{

Token unknown_token()
{
    Token result = {};
    return result;
}

Token eof_token()
{
    Token result = {TokenType::Eof, {}};
    return result;
}

void init(State *state, const char *input)
{
    ZERO_PTR(state);
    StrSlice input_slice = str_slice(input);
    state->current = input_slice.data;
    state->end = input_slice.data + input_slice.length;
}


bool advance(State *tokstate)
{
    if (tokstate->current != tokstate->end)
    {
        ++tokstate->current;
        return true;
    }

    return false;
}


bool at_one_before_end(State *tokstate)
{
    return (tokstate->current + 1) == tokstate->end;
}


bool at_end(State *tokstate)
{
    return tokstate->current == tokstate->end;
}


Token read_word(State *tokstate)
{
    const char *start = tokstate->current;

    for (;;)
    {
        if (!advance(tokstate))
        {
            break;
        }

        if (tokstate->current[0] == '"' ||
            is_whitespace(tokstate->current[0]))
        {
            break;
        }
    }

    Token result;
    result.type = TokenType::String;
    result.text = str_slice(start, tokstate->current);

    return result;
}


Token read_quoted_string(State *tokstate)
{
    assert(tokstate->current[0] == '"');
    const char *start = tokstate->current + 1;

    for (;;)
    {
        if (!advance(tokstate))
        {
            break;
        }

        if (tokstate->current[0] == '"')
        {
            break;
        }
    }

    Token result;
    result.type = TokenType::String;
    result.text = str_slice(start, tokstate->current);

    advance(tokstate);

    return result;
}


Token read_number(State *tokstate)
{
    const char *start = tokstate->current;

    if (start[0] == '-' || start[0] == '+')
    {
        // Must be able to advance to a digit
        if (!advance(tokstate))
        {
            return unknown_token();
        }
    }

    if (!is_numeric(tokstate->current[0]))
    {
        return unknown_token();
    }

    bool has_decimal = false;
    bool is_float = false;

    for (;;)
    {
        if (!advance(tokstate))
        {
            break;
        }

        char c = tokstate->current[0];

        if (c == '.')
        {
            has_decimal = true;
            break;
        }

        if (is_whitespace(c))
        {
            break;
        }

        // f suffix means float
        if (c == 'f')
        {
            if (at_one_before_end(tokstate) || is_whitespace(tokstate->current[1]))
            {
                advance(tokstate);
                is_float = true;
                break;
            }
        }

        if (!is_numeric(c))
        {
            return unknown_token();
        }
    }

    if (has_decimal)
    {
        is_float = true;
        for (;;)
        {
            if (!advance(tokstate))
            {
                break;
            }

            char c = tokstate->current[0];

            if (is_whitespace(c))
            {
                break;
            }

            // f suffix means float
            if (c == 'f')
            {
                if (at_one_before_end(tokstate) || is_whitespace(tokstate->current[1]))
                {
                    advance(tokstate);
                    is_float = true;
                    break;
                }
            }

            if (!is_numeric(c))
            {
                return unknown_token();
            }
        }
    }

    Token result;
    result.type = is_float ? TokenType::Float : TokenType::Int;
    result.text = str_slice(start, tokstate->current);

    return result;
}


void skip_whitespace(State *tokstate)
{
    for (;;)
    {
        if (!is_whitespace(tokstate->current[0]))
            break;

        if (!advance(tokstate))
            break;
    }
}


Token read_token(State *tokstate)
{
    Token result;
    if (tokstate->current == tokstate->end)
    {
        result = eof_token();
        return result;
    }

    skip_whitespace(tokstate);

    char c = tokstate->current[0];

    if (c == '"')
    {
        return read_quoted_string(tokstate);
    }

    if (c == '-' || c == '+' || is_numeric(c))
    {
        State bookmark = *tokstate;
        Token maybe_number = read_number(tokstate);

        if (maybe_number.type == TokenType::Int ||
            maybe_number.type == TokenType::Float)
        {
            return maybe_number;
        }

        // if not number, restore state
        *tokstate = bookmark;
    }

    Token token_result = read_word(tokstate);
    if (str_equal_ignore_case(token_result.text, "true"))
    {
        token_result.type = TokenType::True;
    }
    else if (str_equal_ignore_case(token_result.text, "false"))
    {
        token_result.type = TokenType::False;
    }
    return token_result;
}

} // namespace tokenizer
