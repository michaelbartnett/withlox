#ifndef PRETTY_H

// -*- c++ -*-
#include "types.h"

#include "tokenizer.h"
#include "logging.h"
#include "formatbuffer.h"

void pretty_print(TypeDescriptor *type_desc, int indent = 0);
void pretty_print(TypeDescriptor *type_desc, int indent);
void pretty_print(TypeDescriptor *type_desc, FormatBuffer *fmt_buf, int indent);
void pretty_print(Value *value, int indent = 0);
void pretty_print(Value *value, FormatBuffer *fmt_buf, int indent = 0);
void pretty_print(tokenizer::Token token);
void pretty_print(tokenizer::Token token, FormatBuffer *fmt_buf);

inline void pretty_print(TypeRef typeref, int indent = 0)
{
    TypeDescriptor *td = get_typedesc(typeref);
    pretty_print(td, indent);
}

inline void pretty_print(TypeRef typeref, FormatBuffer *fmt_buf, int indent = 0)
{
    TypeDescriptor *td = get_typedesc(typeref);
    pretty_print(td, fmt_buf, indent);
}


#define PRETTY_H
#endif
