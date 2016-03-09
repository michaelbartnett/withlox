#ifndef PRETTY_H

// -*- c++ -*-
#include "types.h"
#include "tokenizer.h"

void pretty_print(TypeDescriptor *type_desc, int indent = 0);

inline void pretty_print(TypeDescriptorRef typedesc_ref, int indent = 0)
{
    TypeDescriptor *td = get_typedesc(typedesc_ref);
    pretty_print(td, indent);
}


void pretty_print(TypeDescriptor *type_desc, int indent);
void pretty_print(Value *value);
void pretty_print(tokenizer::Token token);

#define PRETTY_H
#endif
