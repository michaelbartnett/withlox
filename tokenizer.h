#ifndef TOKENIZER_H

#include "str.h"
#include <cstddef>

namespace tokenizer
{

struct State
{
    const char *end;
    const char *current;    
};


void init(State *state, const char *input);


inline StrLen chars_remaining(State *tokstate)
{
    std::ptrdiff_t diff = (tokstate->end - tokstate->current);
    assert(diff < STR_LENGTH_MAX);
    return (StrLen)diff;
}


namespace TokenType
{ enum Tag {
    Unknown = 0,
    String,
    Int,
    Float,
    True,
    False,
    Eof
};}


inline const char *to_string(TokenType::Tag token_type)
{
    switch (token_type)
    {
        case TokenType::Unknown: return "Unknown";
        case TokenType::String:  return "String";
        case TokenType::Int:     return "Int";
        case TokenType::Float:   return "Float";
        case TokenType::True:    return "True";
        case TokenType::False:    return "False";
        case TokenType::Eof:     return "Eof";
    }
}


struct Token
{
    TokenType::Tag type;
    StrSlice text;
};


inline bool is_whitespace(char c)
{
    // carriage returns are whitespace, deal with it
    // Stupid windows
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}


inline bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}


inline bool is_numeric(char c)
{
    return c >= '0' && c <= '9';
}


Token read_string(State *tokstate);
Token read_token(State *tokstate);

} // namespace tokenizer

#define TOKENIZER_H
#endif
